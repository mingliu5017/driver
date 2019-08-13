/*
 * TI touch key aw9523 Driver
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

#define INPUT_P0		    0x00
#define INPUT_P1    		0x01
#define CONFIG_P0           0x04
#define CONFIG_P1           0x05
#define INT_P0              0x06
#define INT_P1              0x07

#define INPUT_ENABLE        0xff  //low 4 bits
#define INT_ENABLE          0x0


#define VOLUP_KEY    	  0x20
#define PAUSE_KEY    	  0x08
#define VOLDOWN_KEY    	  0x40
#define BASSUP_KEY    	  0x80
#define BASSDOWN_KEY      0x01
#define TREBLEUP_KEY      0x02
#define TREBLEDOWN_KEY    0x04

#define KEY_MAX_NUMBER	7
#define DEFAULT_SCAN_PERION	20

struct pin_desc {
	u32 code;
	const char *name;
};

/**
 * struct aw9523_i2c_key -
 * @lock - Lock for reading/writing the device
 * @client - Pointer to the I2C client
 * @input_dev - input device pointer
 * @regmap - Devices register map
 * @enable_gpio - VDDIO/EN gpio to enable communication interface
**/
struct i2c_key {
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
};

static struct i2c_key *aw9523_key;

static const struct reg_default aw9523_reg_defs[] = {
	{ INPUT_P0, 0x0f},
	{ INPUT_P1, 0x0f},
	{ CONFIG_P0, 0x00},
	{ CONFIG_P1, 0x00},
	{ INT_P0, 0x00},
	{ INT_P1, 0x00},
};

static const struct regmap_config aw9523_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = INT_P1,
	.reg_defaults = aw9523_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(aw9523_reg_defs),
	.cache_type = REGCACHE_NONE,
};

static void list_work_func(struct work_struct *work)
{
	int ret;
    u32 low_bit = 0, high_bit = 0;
	unsigned char button_status=0,key_index=0;

	ret = regmap_read(aw9523_key->regmap,INPUT_P0, &low_bit);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed read aw9523.\n");
	//}else{
	  //  dev_info(&touch_key->client->dev, "read button status reg =0x%x.\n",button_status);
	}
	ret = regmap_read(aw9523_key->regmap,INPUT_P1, &high_bit);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed read aw9523.\n");
	//}else{
	  //  dev_info(&touch_key->client->dev, "read button status reg =0x%x.\n",button_status);
	}

	button_status = ~(((high_bit | 0x01) << 4) | (low_bit & 0x0f));

	//dev_info(&aw9523_key->client->dev, "read button status reg =0x%x.\n",button_status);

#if 1	
	if (aw9523_key->cur_key_val != button_status) {
		aw9523_key->cur_key_val = button_status;
		if (button_status != 0)
		    aw9523_key->pre_key_val = button_status;
		switch (aw9523_key->pre_key_val) {
			case VOLDOWN_KEY:
				key_index = 0;
                break;
			case PAUSE_KEY:
				key_index = 1;
                break;
			case VOLUP_KEY:
				key_index = 2;
				break;
			case BASSUP_KEY:
				key_index = 3;
				break;
			case BASSDOWN_KEY:
				key_index = 4;
				break;
			case TREBLEUP_KEY:
				key_index = 5;
				break;
			case TREBLEDOWN_KEY:
				key_index = 6;
				break;
		    default:
				key_index = 255;
				dev_info(&aw9523_key->input_dev->dev,"invalid key, please check!\n");
				break;
		}

		if(key_index != 255){
			if (button_status != 0) {
					input_report_key(aw9523_key->input_dev, aw9523_key->key[key_index].code, 1);
					dev_info(&aw9523_key->input_dev->dev,"key %s %d down.\n", \
					    aw9523_key->key[key_index].name,aw9523_key->key[key_index].code);
			} else {
					input_report_key(aw9523_key->input_dev, aw9523_key->key[key_index].code, 0);
					dev_info(&aw9523_key->input_dev->dev,"key %s %d up.\n", \
					    aw9523_key->key[key_index].name,aw9523_key->key[key_index].code);
			}
			input_sync(aw9523_key->input_dev);
		}
	}
#endif
}

static void polling_timer_handler(unsigned long data)
{
	struct i2c_key *aw9523_key = (struct i2c_key *)data;

	schedule_work(&aw9523_key->work_list);
	mod_timer(&(aw9523_key->polling_timer), \
		jiffies+msecs_to_jiffies(aw9523_key->scan_period));
}

static void time_set(struct i2c_key *aw9523_key)
{
    INIT_WORK(&aw9523_key->work_list, list_work_func);

	setup_timer(&aw9523_key->polling_timer, \
		polling_timer_handler, (unsigned long)aw9523_key);

	mod_timer(&aw9523_key->polling_timer, \
		jiffies+msecs_to_jiffies(aw9523_key->scan_period));
}

static int aw9523_init(struct i2c_key *aw9523_key)
{
	int ret;

	if (aw9523_key->enable_gpio)
		gpiod_direction_output(aw9523_key->enable_gpio, GPIOD_OUT_HIGH);

	ret = regmap_write(aw9523_key->regmap,CONFIG_P0, INPUT_ENABLE);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed write aw9523.\n");
		goto out;
	}else{
	    dev_info(&aw9523_key->client->dev, " reg 0x%x=0x%x.\n",CONFIG_P0,INPUT_ENABLE);
	}
	ret = regmap_write(aw9523_key->regmap,CONFIG_P0, INPUT_ENABLE);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed write aw9523.\n");
		goto out;
	}else{
	    dev_info(&aw9523_key->client->dev, " reg 0x%x=0x%x.\n",CONFIG_P0,INPUT_ENABLE);
	}
	ret = regmap_write(aw9523_key->regmap,CONFIG_P0, INPUT_ENABLE);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed write aw9523.\n");
		goto out;
	}else{
	    dev_info(&aw9523_key->client->dev, " reg 0x%x=0x%x.\n",CONFIG_P0,INT_ENABLE);
	}
	ret = regmap_write(aw9523_key->regmap,CONFIG_P0, INPUT_ENABLE);
	if (ret) {
		dev_err(&aw9523_key->client->dev, "Failed write aw9523.\n");
		goto out;
	}else{
	    dev_info(&aw9523_key->client->dev, " reg 0x%x=0x%x.\n",CONFIG_P0,INT_ENABLE);
	}

out:
	if (ret)
		if (aw9523_key->enable_gpio)
			gpiod_direction_output(aw9523_key->enable_gpio, GPIOD_OUT_LOW);
	return ret;
}

static int aw9523_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret, i;
	struct input_dev *input_dev;
	struct device_node *np = client->dev.of_node;

	aw9523_key = devm_kzalloc(&client->dev, sizeof(struct i2c_key), GFP_KERNEL);
	if (!aw9523_key)
		return -ENOMEM;

	aw9523_key->enable_gpio = devm_gpiod_get_optional(&client->dev,
						   "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(aw9523_key->enable_gpio)) {
		ret = PTR_ERR(aw9523_key->enable_gpio);
		dev_err(&client->dev, "Failed to get enable gpio: %d\n", ret);
		return ret;
	}
	ret = of_property_read_u32(np, "scan_period", &(aw9523_key->scan_period));
	if (ret)
		//The default scan period is 20.
		aw9523_key->scan_period = DEFAULT_SCAN_PERION;
	
	ret = of_property_read_u32(np, "key_num", &(aw9523_key->key_num));
	if (ret) {
		dev_err(&client->dev, "failed to get key_num!\n");
		return -EINVAL;
	}

	for (i = 0; i < aw9523_key->key_num; i++) {
		ret = of_property_read_string_index(np, "key_name", i, &(aw9523_key->key[i].name));
		if (ret < 0) {
			dev_err(&client->dev, "find key_name=%d finished！\n", i);
			return -EINVAL;
		}

		ret = of_property_read_u32_index(np, "key_code", i, &(aw9523_key->key[i].code));
		if (ret < 0) {
			dev_err(&client->dev, "find key_code=%d finished！\n", i);
			return -EINVAL;
	    }
		//dev_info(&client->dev, "key name:%s, key code=%d\n",aw9523_key->key[i].name,aw9523_key->key[i].code);
	}

	aw9523_key->regmap = devm_regmap_init_i2c(client, &aw9523_regmap_config);
	if (IS_ERR(aw9523_key->regmap)) {
		ret = PTR_ERR(aw9523_key->regmap);
		dev_err(&client->dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}
	aw9523_key->client = client;

	//input
	input_dev = input_allocate_device();
	if (!input_dev)
		return -EINVAL;
	set_bit(EV_KEY,  input_dev->evbit);
	for (i = 0; i < aw9523_key->key_num; i++) {
		set_bit(aw9523_key->key[i].code,  input_dev->keybit);
		dev_info(&client->dev, "%s key(%d) registed.\n", 
		aw9523_key->key[i].name, aw9523_key->key[i].code);
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
	aw9523_key->input_dev = input_dev;
	ret = input_register_device(aw9523_key->input_dev);
	if (ret < 0) {
		input_free_device(aw9523_key->input_dev);
		return -EINVAL;
	}

	aw9523_init(aw9523_key);

	i2c_set_clientdata(client, aw9523_key);

	time_set(aw9523_key);

	return 0;
}

static int aw9523_remove(struct i2c_client *client)
{
	struct i2c_key *aw9523_key = i2c_get_clientdata(client);

	flush_work(&aw9523_key->work_list);
	del_timer(&aw9523_key->polling_timer);
	input_unregister_device(aw9523_key->input_dev);
	input_free_device(aw9523_key->input_dev);

	return 0;
}

static const struct i2c_device_id aw9523_id[] = {
	{ "aw9523", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, aw9523_id);

static const struct of_device_id of_aw9523_key_match[] = {
	{ .compatible = "awinic,aw9523", },
	{},
};
MODULE_DEVICE_TABLE(of, of_aw9523_key_match);

static struct i2c_driver aw9523_driver = {
	.driver = {
		.name	= "aw9523",
		.of_match_table = of_match_ptr(of_aw9523_key_match),
		.owner = THIS_MODULE,
	},
	.probe		= aw9523_probe,
	.remove		= aw9523_remove,
	.id_table	= aw9523_id,
};
module_i2c_driver(aw9523_driver);

MODULE_DESCRIPTION("key aw9523 driver");
MODULE_AUTHOR("LiuMing <ming.liu@amlogic.com>");
MODULE_LICENSE("GPL");
