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
#include <linux/time.h>
#include <linux/timekeeping.h>

#include "../common/drivers/gpio/gpiolib.h"

#undef pr_fmt
#define pr_fmt(fmt) "rotatry-keypad: " fmt

#define DEFAULT_SCAN_PERION	30
#define KEY_MAX_NUMBER	2


struct pin_desc {
	u32 code;
	const char *name;
};

struct gpio_keypad {
	int jetter_time;
	//int row_key_num;
	//int column_key_num;
	struct gpio_desc *up_desc;
	struct gpio_desc *down_desc;

	unsigned char	key_index;
	struct pin_desc key[KEY_MAX_NUMBER];

	struct input_dev *input_dev;

	/*Abel Add*/
	struct workqueue_struct *rotary_key_wq;
	//struct work_struct rotary_key_work;
	struct delayed_work rotary_key_work;

	struct timeval tv_prev;
	int key_prev;
	int key_count;

	int irq_number;
};


static void report_key_value(struct gpio_keypad *keypad, int value)
{
	struct timeval tv_curr;
	time_t diff;

	do_gettimeofday(&tv_curr);

	diff = (tv_curr.tv_sec - (keypad->tv_prev.tv_sec))*1000000 + tv_curr.tv_usec - (keypad->tv_prev.tv_usec);
	//dev_info(&(keypad->input_dev->dev), "key diff %ld\n", diff);
	if((diff >= 0)&&(diff < 100000)){
		if(keypad->key_count >= 5){ /*过滤算法生效*/
			keypad->key_count = 5;
			if(value != keypad->key_prev){
				keypad->tv_prev.tv_sec = tv_curr.tv_sec;
				keypad->tv_prev.tv_usec = tv_curr.tv_usec;
				goto out;
			}
		}
		else if(keypad->key_count < 3){
			if(value != keypad->key_prev){
				keypad->key_prev = value;
				keypad->tv_prev.tv_sec = tv_curr.tv_sec;
				keypad->tv_prev.tv_usec = tv_curr.tv_usec;
				goto out;
			}
		}
		else{
			keypad->key_count++;
		}
	}
	else{
		keypad->key_count = 0;
		keypad->key_prev = value;
	}
	keypad->tv_prev.tv_sec = tv_curr.tv_sec;
	keypad->tv_prev.tv_usec = tv_curr.tv_usec;

	input_report_key(keypad->input_dev, value, 1);
	input_sync(keypad->input_dev);
	input_report_key(keypad->input_dev, value, 0);
	input_sync(keypad->input_dev);
	dev_info(&(keypad->input_dev->dev), "key %d up.\n", value);

out:
	return;
}


static void report_key_code(struct gpio_keypad *keypad, int up, int down)
{
	if(0 == down){
		if(0 == up){
			report_key_value(keypad, keypad->key[0].code);
		}
		else{
			report_key_value(keypad, keypad->key[1].code);
		}
	}else{
		if(0 != up){
			report_key_value(keypad, keypad->key[0].code);
		}
		else{
			report_key_value(keypad, keypad->key[1].code);
		}
	}
	return;
}


static void rotary_key_func(struct work_struct *p)
{
	struct gpio_keypad *keypad;
	struct delayed_work *d_work;
	int gpio_up, gpio_down;

	d_work = container_of(p, struct delayed_work, work);
	keypad = container_of(d_work, struct gpio_keypad, rotary_key_work);
	//keypad = container_of(p, struct gpio_keypad, rotary_key_work);

	gpio_up = gpiod_get_value(keypad->up_desc);
	gpio_down = gpiod_get_value(keypad->down_desc);
	report_key_code(keypad, gpio_up, gpio_down);

}

static irqreturn_t rotary_key_interrupt(int irq, void *arg)
{
	struct gpio_keypad *keypad;
	//int gpio_val;
	
	keypad = (struct gpio_keypad *)arg;

	queue_delayed_work(keypad->rotary_key_wq, &keypad->rotary_key_work, keypad->jetter_time);  
	//queue_work(keypad->rotary_key_wq, &keypad->rotary_key_work);  
	
	return IRQ_HANDLED;
}


static int meson_gpio_kp_probe(struct platform_device *pdev)
{
	struct gpio_desc *desc;
	int ret, i;
	struct input_dev *input_dev;
	struct gpio_keypad *keypad;

	dev_err(&pdev->dev, "in meson_gpio_kp_probe!\n");

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
		"jetter_time", &(keypad->jetter_time));
	if (ret)
		keypad->jetter_time = DEFAULT_SCAN_PERION;

	desc = devm_gpiod_get(&pdev->dev, "vol_up", GPIOD_IN);
	if (!desc){
 		dev_err(&pdev->dev, "devm_gpiod_get vol_up success!\n");
		goto err1;
	}
	keypad->up_desc = desc;

	gpiod_direction_input(keypad->up_desc);
	gpiod_set_pull(keypad->up_desc, GPIOD_PULL_UP);

	desc = devm_gpiod_get(&pdev->dev, "vol_down", GPIOD_OUT_LOW);
	if (!desc){
		dev_err(&pdev->dev, "devm_gpiod_get vol_down success!\n");
		goto err2;
	}
		
	keypad->down_desc = desc;

	gpiod_direction_input(keypad->down_desc);
	gpiod_set_pull(keypad->down_desc, GPIOD_PULL_UP);

	for (i = 0; i < KEY_MAX_NUMBER; i++) {
		ret = of_property_read_string_index(pdev->dev.of_node,
			"key_name", i, &(keypad->key[i].name));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_name=%d finished\n", i);
			goto err2;
		}
		ret = of_property_read_u32_index(pdev->dev.of_node,
		"key_code", i, &(keypad->key[i].code));
		if (ret < 0) {
			dev_err(&pdev->dev,
				"find key_code=%d finished\n", i);
			goto err2;
	    }
		//dev_err(&pdev->dev, "key name:%s, key code=%d\n",keypad->key[i].name,keypad->key[i].code);
	}

	keypad->rotary_key_wq = create_workqueue("rotarykey");
	if(NULL == keypad->rotary_key_wq){
		dev_err(&pdev->dev, "create_workqueue failed!\n");
		goto err3;
	}

	//INIT_WORK(&keypad->rotary_key_work, rotary_key_func);
	INIT_DELAYED_WORK(&keypad->rotary_key_work, rotary_key_func);

	//input
	input_dev = input_allocate_device();
	if (!input_dev){
		dev_err(&pdev->dev, "allocate input device failed!\n");
		goto err4;
	}
	set_bit(EV_KEY,  input_dev->evbit);
	for (i = 0; i < 2; i++) {
		set_bit(keypad->key[i].code,  input_dev->keybit);
		dev_info(&pdev->dev, "%s key(%d) registed.\n", keypad->key[i].name, keypad->key[i].code);
	}
	input_dev->name = "rotary_keypad";
	input_dev->phys = "rotary_keypad/input0";
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
	
	keypad->irq_number = gpiod_to_irq(keypad->down_desc);
	if (IS_ERR_VALUE(&(keypad->irq_number))) {
		dev_err(&pdev->dev, "goiod to irq failed!\n");
		goto err5;
	}

	
	ret = devm_request_irq(&keypad->input_dev->dev, keypad->irq_number, rotary_key_interrupt, 
						   IRQF_TRIGGER_RISING, "ROTARY_KEY_EINT", keypad);
	//ret = devm_request_irq(&keypad->input_dev->dev, keypad->irq_number, rotary_key_interrupt, 
	//		       IRQF_TRIGGER_FALLING, "ROTARY_KEY_EINT", keypad);
	if (ret) {
		dev_err(&pdev->dev, "request irq failed!\n");
		goto err5;
	}
	
	ret = input_register_device(keypad->input_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "input device register failed!\n");
		goto err5;
	}
	platform_set_drvdata(pdev, keypad);
	
	dev_err(&pdev->dev, "meson_gpio_kp_probe success!\n");

	return 0;

err5:
	input_free_device(keypad->input_dev);
err4:
	cancel_delayed_work(&keypad->rotary_key_work);  
	flush_workqueue(keypad->rotary_key_wq);
	destroy_workqueue(keypad->rotary_key_wq);
err3:
	devm_gpiod_put(&pdev->dev, keypad->down_desc);
err2:
	devm_gpiod_put(&pdev->dev, keypad->up_desc);
err1:
	devm_kfree(&pdev->dev, keypad);
	return -EINVAL;
}

static int meson_gpio_kp_remove(struct platform_device *pdev)
{
	struct gpio_keypad *keypad;

	keypad = (struct gpio_keypad *)platform_get_drvdata(pdev);

	input_unregister_device(keypad->input_dev);

	input_free_device(keypad->input_dev);
	cancel_delayed_work(&keypad->rotary_key_work); 
	flush_workqueue(keypad->rotary_key_wq);
	destroy_workqueue(keypad->rotary_key_wq);
	devm_gpiod_put(&pdev->dev, keypad->down_desc);
	devm_gpiod_put(&pdev->dev, keypad->up_desc);
	devm_kfree(&pdev->dev, keypad);

	
	return 0;
}


static const struct of_device_id key_dt_match[] = {
	{	.compatible = "amlogic, rotary-keypad", },
	{},
};

static struct platform_driver meson_gpio_kp_driver = {
	.probe = meson_gpio_kp_probe,
	.remove = meson_gpio_kp_remove,
	//.suspend = meson_gpio_kp_suspend,
	.driver = {
		.name = "rotary-keypad",
		.of_match_table = key_dt_match,
	},
};

module_platform_driver(meson_gpio_kp_driver);
MODULE_AUTHOR("Amlogic Liu Ming");
MODULE_DESCRIPTION("GPIO rotatry Keypad Driver");
MODULE_LICENSE("GPL");
