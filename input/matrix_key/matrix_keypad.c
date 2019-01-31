/*
 * drivers/amlogic/input/keyboard/matrix_keypad.c
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
#include "../../../gpio/gpiolib.h"

#undef pr_fmt
#define pr_fmt(fmt) "matrix-keypad: " fmt

#define DEFAULT_SCAN_PERION	20
#define ROW_MAX_NUMBER	9
#define COLUMN_MAX_NUMBER	9
#define KEY_MAX_NUMBER	81


struct pin_desc {
	int current_status;
	int previous_status;
	u32 code;
	const char *name;
};

struct gpio_keypad {
	int scan_period;
	int row_key_num;
	int column_key_num;
	struct gpio_desc *row_key_desc[ROW_MAX_NUMBER];
	struct gpio_desc *column_key_desc[COLUMN_MAX_NUMBER];

	int row_gpio_value;
	int column_count;

	struct pin_desc key[KEY_MAX_NUMBER];
	struct timer_list polling_timer;
	struct input_dev *input_dev;
};

static void report_key_code(struct gpio_keypad *keypad, int row_gpio_value, int column_count)
{
	int i,key_index;

	for(i = keypad->row_key_num-1; i >= 0; i--){
		key_index = (column_count*keypad->row_key_num) + i;
		if((row_gpio_value & 0x01) == 0x00)
		{
			keypad->key[key_index].current_status = 0;
			if(keypad->key[key_index].previous_status != keypad->key[key_index].current_status){
			    input_report_key(keypad->input_dev, keypad->key[key_index].code, 0);
			    dev_err(&(keypad->input_dev->dev), "key %d: %s %d down.\n", \
					key_index, keypad->key[key_index].name, keypad->key[key_index].code);
				keypad->key[key_index].previous_status = 0;
			}
		}
		else{
			keypad->key[key_index].current_status = 1;
			if(keypad->key[key_index].previous_status != keypad->key[key_index].current_status){
			    input_report_key(keypad->input_dev, keypad->key[key_index].code, 1);
			    dev_err(&(keypad->input_dev->dev), "key %d: %s %d up.\n",\
					key_index, keypad->key[key_index].name, keypad->key[key_index].code);
				keypad->key[key_index].previous_status = 1;
			}
		}
	    row_gpio_value = row_gpio_value >> 1;
	}

    input_sync(keypad->input_dev);
}

static void polling_timer_handler(unsigned long data)
{
	struct gpio_keypad *keypad;
	int i;
	int gpio_val;

	keypad = (struct gpio_keypad *)data;

	for (i = 0; i < keypad->column_key_num; i++) {
		gpiod_set_value(keypad->column_key_desc[i],1);
	}

	if(keypad->column_count >= keypad->column_key_num){
		keypad->column_count = 0;
	}
	gpiod_set_value(keypad->column_key_desc[keypad->column_count],0);

	keypad->row_gpio_value =0;
	for (i = 0; i < keypad->row_key_num; i++) {
		gpio_val = gpiod_get_value(keypad->row_key_desc[i]);
		keypad->row_gpio_value = (keypad->row_gpio_value << 1) | gpio_val;
	}

	dev_info(&(keypad->input_dev->dev),"scan ret column = %d ,row_gpio_value=0x%x ,\n",\
	        keypad->column_count, keypad->row_gpio_value);

	report_key_code(keypad, keypad->row_gpio_value, keypad->column_count);

	keypad->column_count++;

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

	ret = of_property_read_u32(pdev->dev.of_node,
		"row_num", &(keypad->row_key_num));
	if (ret) {
		dev_err(&pdev->dev, "failed to get row_key_num!\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pdev->dev.of_node,
		"column_num", &(keypad->column_key_num));
	if (ret) {
		dev_err(&pdev->dev, "failed to get column_key_num!\n");
		return -EINVAL;
	}

	for (i = 0; i < keypad->row_key_num; i++) {
		//get all row gpio desc.
		desc = devm_gpiod_get_index(&pdev->dev, "row", i, GPIOD_IN);
		if (!desc)
			return -EINVAL;
		keypad->row_key_desc[i] = desc;

		gpiod_direction_input(keypad->row_key_desc[i]);
		gpiod_set_pull(keypad->row_key_desc[i], GPIOD_PULL_UP);
	}

	for (i = 0; i < keypad->column_key_num; i++) {
		//get all column gpio desc.
		desc = devm_gpiod_get_index(&pdev->dev, "column", i, GPIOD_OUT_LOW);
		if (!desc)
			return -EINVAL;
		keypad->column_key_desc[i] = desc;

		gpiod_direction_output(keypad->column_key_desc[i],GPIOD_OUT_HIGH);
	}

	for (i = 0; i < (keypad->row_key_num*keypad->column_key_num); i++) {
		//The gpio default is high level.
		keypad->key[i].current_status = 1;
		keypad->key[i].previous_status = 1;
		ret = of_property_read_u32_index(pdev->dev.of_node,
		"key_code", i, &(keypad->key[i].code));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_code=%d finished\n", i);
			return -EINVAL;
	    }
		ret = of_property_read_string_index(pdev->dev.of_node,
			"key_name", i, &(keypad->key[i].name));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_name=%d finished\n", i);
			return -EINVAL;
		}
		dev_err(&pdev->dev, "key name:%s, key code=%d\n",keypad->key[i].name,keypad->key[i].code);
	}

	//input
	input_dev = input_allocate_device();
	if (!input_dev)
		return -EINVAL;
	set_bit(EV_KEY,  input_dev->evbit);
	for (i = 0; i < (keypad->row_key_num*keypad->column_key_num); i++) {
		set_bit(keypad->key[i].code,  input_dev->keybit);
		dev_info(&pdev->dev, "%s key(%d) registed.\n",
			keypad->key[i].name, keypad->key[i].code);
	}
	input_dev->name = "matrix_keypad";
	input_dev->phys = "matrix_keypad/input0";
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

static int meson_gpio_kp_resume(struct platform_device *dev)
{
	struct gpio_keypad *pdata;

	pdata = (struct gpio_keypad *)platform_get_drvdata(dev);
	mod_timer(&(pdata->polling_timer), jiffies+msecs_to_jiffies(5));

	if (get_resume_method() == POWER_KEY_WAKEUP) {
		pr_info("gpio keypad wakeup\n");
		input_report_key(pdata->input_dev, KEY_POWER,  1);
		input_sync(pdata->input_dev);
		input_report_key(pdata->input_dev,KEY_POWER,  0);
		input_sync(pdata->input_dev);
	}

	return 0;
}

static const struct of_device_id key_dt_match[] = {
	{	.compatible = "amlogic, matrix-keypad", },
	{},
};

static struct platform_driver meson_gpio_kp_driver = {
	.probe = meson_gpio_kp_probe,
	.remove = meson_gpio_kp_remove,
	.suspend = meson_gpio_kp_suspend,
	.resume = meson_gpio_kp_resume,
	.driver = {
		.name = "matrix-keypad",
		.of_match_table = key_dt_match,
	},
};

module_platform_driver(meson_gpio_kp_driver);
MODULE_AUTHOR("Amlogic Liu Ming");
MODULE_DESCRIPTION("GPIO Matrix Keypad Driver");
MODULE_LICENSE("GPL");
