/*
 * TI touch key msp430 Driver
 *
 * Copyright (C) 2019 Amlogic
 *
 * Author: Liu Ming <ming.liu@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include "../common/drivers/gpio/gpiolib.h"

#define CAP_BUTTON_STATUS		            0x00
#define CAP_BUTTON__LONG_PRESS_STATUS		0x01
#define MUTE_STATUS                     	0x02
#define FW_VERSION                      	0x03

#define MUTE_KEY    	  0x01
#define VOLUP_KEY    	  0x02
#define PAUSE_KEY    	  0x04
#define VOLDOWN_KEY    	  0x08
#define BT_KEY    	      0x10

#define KEY_MAX_NUMBER	5
#define DEFAULT_SCAN_PERION	20

struct pin_desc {
	u32 code;
	const char *name;
};

/**
 * struct msp430_touch_key -
 * @lock - Lock for reading/writing the device
 * @client - Pointer to the I2C client
 * @input_dev - input device pointer
 * @regmap - Devices register map
 * @enable_gpio - VDDIO/EN gpio to enable communication interface
**/
struct msp430_touch_key {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct regmap *regmap;
	struct gpio_desc *enable_gpio;
	int scan_period;
	int key_num;
	struct pin_desc key[KEY_MAX_NUMBER];
	struct timer_list polling_timer;//定时器
	struct work_struct work_list;//定义工作队列
	u32 cur_key_val;
	u32 pre_key_val;
	u32 mute_key_val;
};

static struct msp430_touch_key *touch_key;

static const struct reg_default msp430_reg_defs[] = {
	{ CAP_BUTTON_STATUS, 0x00},
	{ CAP_BUTTON__LONG_PRESS_STATUS, 0x00},
	{ MUTE_STATUS, 0x00},
	{ FW_VERSION, 0x00},
};

static const struct regmap_config msp430_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = FW_VERSION,
	.reg_defaults = msp430_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(msp430_reg_defs),
	.cache_type = REGCACHE_NONE,
};

static void list_work_func(struct work_struct *work)
{
	int ret;
    u32 button_status, mute_status;
	u8  key_index;

	ret = regmap_read(touch_key->regmap,CAP_BUTTON_STATUS, &button_status);
	if (ret) {
		dev_err(&touch_key->client->dev, "Failed read msp430.\n");
	//}else{
	  //  dev_info(&touch_key->client->dev, "read button status reg =0x%x.\n",button_status);
	}
	
	if (touch_key->cur_key_val != button_status) {
		touch_key->cur_key_val = button_status;
		if (button_status != 0)
		    touch_key->pre_key_val = button_status;
		switch (touch_key->pre_key_val) {
			case MUTE_KEY:
				ret = regmap_read(touch_key->regmap,MUTE_STATUS, &mute_status);
				if (ret) {
					dev_err(&touch_key->client->dev, "Failed read msp430.\n");
				//}else{
				  //  dev_info(&touch_key->client->dev, "read button status reg =0x%x.\n",button_status);
				}
				key_index = 0;
			    break;
			case VOLDOWN_KEY:
				key_index = 1;
                break;
			case PAUSE_KEY:
				key_index = 2;
                break;
			case VOLUP_KEY:
				key_index = 3;
				break;
			case BT_KEY:
				key_index = 4;
				break;
		    default:
				key_index = 255;
				dev_info(&touch_key->input_dev->dev,"invalid key, please check!\n");
				break;
		}

		if(key_index != 255){
			if(key_index == 0){
				if(touch_key->mute_key_val != mute_status){
					touch_key->mute_key_val = mute_status;
					input_report_key(touch_key->input_dev, touch_key->key[key_index].code, mute_status);
					dev_info(&touch_key->input_dev->dev,"key %s %d %s.\n", \
						touch_key->key[key_index].name,touch_key->key[key_index].code, \
						(mute_status > 0)? "mute" : "unmute");
				}
			}else{
				if (button_status != 0) {
					input_report_key(touch_key->input_dev, touch_key->key[key_index].code, 1);
					dev_info(&touch_key->input_dev->dev,"key %s %d down.\n", \
					    touch_key->key[key_index].name,touch_key->key[key_index].code);
				} else {
					input_report_key(touch_key->input_dev, touch_key->key[key_index].code, 0);
					dev_info(&touch_key->input_dev->dev,"key %s %d up.\n", \
					    touch_key->key[key_index].name,touch_key->key[key_index].code);
				}
			}
			input_sync(touch_key->input_dev);
		}
	}
}

static void polling_timer_handler(unsigned long data)
{
	struct msp430_touch_key *touch_key = (struct msp430_touch_key *)data;

	schedule_work(&touch_key->work_list);
	mod_timer(&(touch_key->polling_timer), \
		jiffies+msecs_to_jiffies(touch_key->scan_period));
}

static void time_set(struct msp430_touch_key *touch_key)
{
    INIT_WORK(&touch_key->work_list, list_work_func);

	setup_timer(&touch_key->polling_timer, \
		polling_timer_handler, (unsigned long)touch_key);

	mod_timer(&touch_key->polling_timer, \
		jiffies+msecs_to_jiffies(touch_key->scan_period));
}

static int msp430_init(struct msp430_touch_key *touch_key)
{
	int ret;
	u32 button_status;

	if (touch_key->enable_gpio)
		gpiod_direction_output(touch_key->enable_gpio, GPIOD_OUT_HIGH);

	ret = regmap_read(touch_key->regmap,CAP_BUTTON_STATUS, &button_status);
	if (ret) {
		dev_err(&touch_key->client->dev, "Failed read msp430.\n");
		goto out;
	}else{
	    dev_info(&touch_key->client->dev, "read button status reg value=0x%x.\n",button_status);
	}
out:
	if (ret)
		if (touch_key->enable_gpio)
			gpiod_direction_output(touch_key->enable_gpio, GPIOD_OUT_LOW);
	return ret;
}

static int msp430_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret, i;
	struct input_dev *input_dev;
	unsigned int version;
	struct device_node *np = client->dev.of_node;

	touch_key = devm_kzalloc(&client->dev, sizeof(struct msp430_touch_key), GFP_KERNEL);
	if (!touch_key)
		return -ENOMEM;

	touch_key->enable_gpio = devm_gpiod_get_optional(&client->dev,
						   "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(touch_key->enable_gpio)) {
		ret = PTR_ERR(touch_key->enable_gpio);
		dev_err(&client->dev, "Failed to get enable gpio: %d\n", ret);
		return ret;
	}
	ret = of_property_read_u32(np, "scan_period", &(touch_key->scan_period));
	if (ret)
		//The default scan period is 20.
		touch_key->scan_period = DEFAULT_SCAN_PERION;
	
	ret = of_property_read_u32(np, "key_num", &(touch_key->key_num));
	if (ret) {
		dev_err(&client->dev, "failed to get key_num!\n");
		return -EINVAL;
	}

	for (i = 0; i < touch_key->key_num; i++) {
		ret = of_property_read_string_index(np, "key_name", i, &(touch_key->key[i].name));
		if (ret < 0) {
			dev_err(&client->dev, "find key_name=%d finished！\n", i);
			return -EINVAL;
		}

		ret = of_property_read_u32_index(np, "key_code", i, &(touch_key->key[i].code));
		if (ret < 0) {
			dev_err(&client->dev, "find key_code=%d finished！\n", i);
			return -EINVAL;
	    }
		//dev_info(&client->dev, "key name:%s, key code=%d\n",touch_key->key[i].name,touch_key->key[i].code);
	}

	touch_key->regmap = devm_regmap_init_i2c(client, &msp430_regmap_config);
	if (IS_ERR(touch_key->regmap)) {
		ret = PTR_ERR(touch_key->regmap);
		dev_err(&client->dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}
	touch_key->client = client;

	//input
	input_dev = input_allocate_device();
	if (!input_dev)
		return -EINVAL;
	set_bit(EV_KEY,  input_dev->evbit);
	for (i = 0; i < touch_key->key_num; i++) {
		set_bit(touch_key->key[i].code,  input_dev->keybit);
		dev_info(&client->dev, "%s key(%d) registed.\n", 
		touch_key->key[i].name, touch_key->key[i].code);
	}
	input_dev->name = "touch_keypad";
	input_dev->phys = "touch_keypad/input0";
	input_dev->dev.parent = &client->dev;
	input_dev->id.bustype = BUS_ISA;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->rep[REP_DELAY] = 0xffffffff;
	input_dev->rep[REP_PERIOD] = 0xffffffff;
	input_dev->keycodesize = sizeof(unsigned short);
	input_dev->keycodemax = 0x1ff;
	touch_key->input_dev = input_dev;
	ret = input_register_device(touch_key->input_dev);
	if (ret < 0) {
		input_free_device(touch_key->input_dev);
		return -EINVAL;
	}

	ret = regmap_read(touch_key->regmap, FW_VERSION, &version);
	if (ret) {
		dev_err(&touch_key->client->dev, "Failed read msp430 reg.\n");
	}else{
	    dev_info(&touch_key->client->dev, "probe FW version:V %d.\n",version);
	}

	msp430_init(touch_key);

	i2c_set_clientdata(client, touch_key);

	time_set(touch_key);

	return 0;
}

static int msp430_remove(struct i2c_client *client)
{
	struct msp430_touch_key *touch_key = i2c_get_clientdata(client);

	flush_work(&touch_key->work_list);
	del_timer(&touch_key->polling_timer);
	input_unregister_device(touch_key->input_dev);
	input_free_device(touch_key->input_dev);

	return 0;
}

static const struct i2c_device_id msp430_id[] = {
	{ "msp430", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, msp430_id);

static const struct of_device_id of_msp430_touch_key_match[] = {
	{ .compatible = "ti,msp430", },
	{},
};
MODULE_DEVICE_TABLE(of, of_msp430_touch_key_match);

static struct i2c_driver msp430_driver = {
	.driver = {
		.name	= "msp430",
		.of_match_table = of_match_ptr(of_msp430_touch_key_match),
		.owner = THIS_MODULE,
	},
	.probe		= msp430_probe,
	.remove		= msp430_remove,
	.id_table	= msp430_id,
};
module_i2c_driver(msp430_driver);

MODULE_DESCRIPTION("Texas Instruments touch key msp430 driver");
MODULE_AUTHOR("LiuMing <ming.liu@amlogic.com>");
MODULE_LICENSE("GPL");
