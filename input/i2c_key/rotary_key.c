/*
 * drivers/amlogic/input/keyboard/rotary_keypad.c
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/amlogic/pm.h>
#include "../common/drivers/gpio/gpiolib.h"

#undef pr_fmt
#define pr_fmt(fmt) "rotatry-keypad: " fmt

#define DEFAULT_SCAN_PERION	20
#define KEY_MAX_NUMBER	2


struct pin_desc {
	u32 code;
	const char *name;
};

struct gpio_keypad {
	int scan_period;
	int row_key_num;
	int column_key_num;
	struct gpio_desc *up_desc;
	struct gpio_desc *down_desc;
	unsigned char current_status;
	unsigned char previous_status;

	unsigned char	key_index;
	struct pin_desc key[KEY_MAX_NUMBER];
	struct timer_list polling_timer;
	struct input_dev *input_dev;
};

static void report_key_code(struct gpio_keypad *keypad, int gpio_up, int gpio_down)
{
	unsigned char key_tmp=0;

	key_tmp = (gpio_up << 1) | gpio_down;

	if(key_tmp == 0x00){
		return;
	}
	else{
	    keypad->current_status = key_tmp;
	}
	//dev_info(&(keypad->input_dev->dev),"current key = 0x%x\n",keypad->current_status);

	if(keypad->current_status != keypad->previous_status){
		if (keypad->previous_status==0x03 && keypad->current_status==0x01){ // 是左旋转否
		   keypad->key_index=0;
		   input_report_key(keypad->input_dev, keypad->key[0].code, 1);
		   dev_err(&(keypad->input_dev->dev), "left turn key %d: %s %d down.\n", \
					keypad->key_index, keypad->key[0].name, keypad->key[0].code);
		}
		else if (keypad->previous_status==0x03 && keypad->current_status==0x02){ // 是右旋转否
		   keypad->key_index=1;
		   input_report_key(keypad->input_dev, keypad->key[1].code, 1);
		   dev_err(&(keypad->input_dev->dev), "right turn key %d: %s %d down.\n", \
					keypad->key_index, keypad->key[1].name, keypad->key[1].code);
		}
		else if(keypad->current_status==0x03){
			 input_report_key(keypad->input_dev, keypad->key[keypad->key_index].code, 0);
			    dev_err(&(keypad->input_dev->dev), "key %d: %s %d up.\n",\
					keypad->key_index, keypad->key[keypad->key_index].name, keypad->key[keypad->key_index].code);
		}
		keypad->previous_status = keypad->current_status;
		input_sync(keypad->input_dev);
	}

}

static void polling_timer_handler(unsigned long data)
{
	struct gpio_keypad *keypad;
	int gpio_up=0,gpio_down=0;

	keypad = (struct gpio_keypad *)data;

	gpio_up = gpiod_get_value(keypad->up_desc);
	gpio_down = gpiod_get_value(keypad->down_desc);

	report_key_code(keypad, gpio_up, gpio_down);

	mod_timer(&(keypad->polling_timer), jiffies+msecs_to_jiffies(keypad->scan_period));

}

static int meson_gpio_kp_probe(struct platform_device *pdev)
{
	struct gpio_desc *desc;
	int ret, i;
	struct input_dev *input_dev;
	struct gpio_keypad *keypad;

	if (!(pdev->dev.of_node)) {
		dev_err(&pdev->dev,
			"pdev->dev.of_node == NULL!\n");
		return -EINVAL;
	}
	keypad = devm_kzalloc(&pdev->dev,
		sizeof(struct gpio_keypad), GFP_KERNEL);
	if (!keypad)
		return -EINVAL;

	ret = of_property_read_u32(pdev->dev.of_node,
		"scan_period", &(keypad->scan_period));
	if (ret)
		//The default scan period is 20.
		keypad->scan_period = DEFAULT_SCAN_PERION;

	desc = devm_gpiod_get(&pdev->dev, "vol_up", GPIOD_IN);
	if (!desc)
		return -EINVAL;
	keypad->up_desc = desc;

	gpiod_direction_input(keypad->up_desc);
	gpiod_set_pull(keypad->up_desc, GPIOD_PULL_UP);

	desc = devm_gpiod_get(&pdev->dev, "vol_down", GPIOD_OUT_LOW);
	if (!desc)
		return -EINVAL;
	keypad->down_desc = desc;

	gpiod_direction_input(keypad->down_desc);
	gpiod_set_pull(keypad->down_desc, GPIOD_PULL_UP);

	for (i = 0; i < KEY_MAX_NUMBER; i++) {
		ret = of_property_read_string_index(pdev->dev.of_node,
			"key_name", i, &(keypad->key[i].name));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_name=%d finished\n", i);
			return -EINVAL;
		}
		ret = of_property_read_u32_index(pdev->dev.of_node,
		"key_code", i, &(keypad->key[i].code));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_code=%d finished\n", i);
			return -EINVAL;
	    }
		dev_err(&pdev->dev, "key name:%s, key code=%d\n",keypad->key[i].name,keypad->key[i].code);
	}

	//input
	input_dev = input_allocate_device();
	if (!input_dev)
		return -EINVAL;
	set_bit(EV_KEY,  input_dev->evbit);
	for (i = 0; i < 2; i++) {
		set_bit(keypad->key[i].code,  input_dev->keybit);
		dev_info(&pdev->dev, "%s key(%d) registed.\n", keypad->key[i].name, keypad->key[i].code);
	}
	input_dev->name = "touch_keypad";
	input_dev->phys = "touch_keypad/input0";
	input_dev->dev.parent = &pdev->dev;
	input_dev->id.bustype = BUS_ISA;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->rep[REP_DELAY] = 0xffffffff;
	input_dev->rep[REP_PERIOD] = 0xffffffff;
	input_dev->keycodesize = sizeof(unsigned short);
	input_dev->keycodemax = 0x1ff;
	keypad->input_dev = input_dev;
	ret = input_register_device(keypad->input_dev);
	if (ret < 0) {
		input_free_device(keypad->input_dev);
		return -EINVAL;
	}
	platform_set_drvdata(pdev, keypad);
	setup_timer(&(keypad->polling_timer), polling_timer_handler, (unsigned long) keypad);

	mod_timer(&(keypad->polling_timer), jiffies+msecs_to_jiffies(keypad->scan_period));

	return 0;
}

static int meson_gpio_kp_remove(struct platform_device *pdev)
{
	struct gpio_keypad *keypad;

	keypad = (struct gpio_keypad *)platform_get_drvdata(pdev);
	input_unregister_device(keypad->input_dev);
	input_free_device(keypad->input_dev);
	del_timer(&(keypad->polling_timer));

	return 0;
}

static int meson_gpio_kp_suspend(struct platform_device *dev,
	pm_message_t state)
{
	struct gpio_keypad *pdata;

	pdata = (struct gpio_keypad *)platform_get_drvdata(dev);
	del_timer(&(pdata->polling_timer));

	return 0;
}

static const struct of_device_id key_dt_match[] = {
	{	.compatible = "amlogic, rotatry-keypad", },
	{},
};

static struct platform_driver meson_gpio_kp_driver = {
	.probe = meson_gpio_kp_probe,
	.remove = meson_gpio_kp_remove,
	.suspend = meson_gpio_kp_suspend,
	.driver = {
		.name = "rotatry-keypad",
		.of_match_table = key_dt_match,
	},
};

module_platform_driver(meson_gpio_kp_driver);
MODULE_AUTHOR("Amlogic Liu Ming");
MODULE_DESCRIPTION("GPIO rotatry Keypad Driver");
MODULE_LICENSE("GPL");
