/*
 * drivers/amlogic/ledring/ledring.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/input.h>
#include <linux/of_platform.h>
#include <linux/delay.h>

#define addr_led_reg1     0x01    //mcu version
#define addr_led_reg2     0x02    //mcu version
#define addr_led_reg3     0x03    //input key
#define addr_led_reg4     0x04    //output led
#define addr_led_reg5     0x05    //output led
#define addr_led_reg6     0x06    //output led

#define addr_led_regR     0x0A    //output led
#define addr_led_regG     0x0B    //output led
#define addr_led_regB     0x0C    //output led

#define addr_led_regSb    0x0D    //output led
#define addr_led_duratTime 0x07

#define RELEASE_KEY       0x00
#define MUTE_KEY    	  0x78
#define VOLUP_KEY    	  0xB4
#define VOLDOWN_KEY    	  0xE1
#define PAUSE_KEY    	  0xD2
#define VOLUP_VOLDOWN_KEY 0xA5
#define MUTE_PAUSE_KEY   0x5A
#define VOLDOWN_PAUSE_KEY 0xC3
#define VOLUP_MUTE_KEY     0x3C
#define VOLUP_PAUSE_KEY   0x96
#define VOLDOWN_MUTE_KEY   0x69

#define DEFAULT_SPEED     230  //default speed 230ms

#define LED_DEVICE_NAME             "ledring"
#define LED_CHAR_DEV_NAME           "aml-led"

#define CMD_LEDRING_ARG             0x100001
#define CMD_READ_VERSION            0x100002
#define CMD_LEDRING_VOLUME          0x100003
#define CMD_LEDRING_WAKEUP          0x100004
#define CMD_LEDRING_SELFDEF         0x100005
#define CMD_LEDRING_SELFSIG         0x100006
#define MAX_NAME_LEN                50

typedef enum
{
	LED_SIG_ROLL = 0x01,     /*led 自定义无底色单色旋转*/
	LED_ROLL_BAK,            /*led 有底色单色旋转*/
	LED_ALL_FLASH,           /*自定义RGB闪烁，所有灯相同*/
	LED_ALL_ROLL,            /*自定义每颗led，所有led旋转*/
	LED_SIG_FLASH,           /*自定义每颗led闪烁，led颜色不同*/
	LED_ROLL_POSITION,      /*有背景色，带有方向旋转*/
	LED_ALL_LIGHT = 0x0f,     /*自定义led常亮，所有led颜色相同*/
	LED_SIG_LIGHT            /*自定义led常亮，led颜色不同*/
} LED_PATTERN;

typedef struct {
	int period;
	LED_PATTERN pattern;
        unsigned char duration_time;
	unsigned char position;
	unsigned char led_all_data[3];
	unsigned char led_sig_data[24];
}led_info;


static struct kobject *ledring_kobj;
static struct i2c_client *g_client;


static struct _key_led {
	struct input_dev *pca_input_dev;
	char dev_name[MAX_NAME_LEN];
	struct timer_list mtimer;
	struct work_struct list_work;
	int run_time;
	int key_num;
	int key_tmp_val;
	int key_last_val;
	int key_status;
	int action_times;
	struct class *cls;
	int major;
	int mode;
} *key_led_des;

struct _key_des {
	char name[MAX_NAME_LEN];
	unsigned int key_val;
	int pin;
	int irq;
} *exp_key;



static const struct i2c_device_id ledring_id[] = {
	{"aml_pca9557"},
	{}
};
MODULE_DEVICE_TABLE(i2c, ledring_id);

static const struct of_device_id ledring_dt_ids[] = {
	{
		.compatible = "aml, ledring",
		.data = (void *)NULL
	},
	{},
};
MODULE_DEVICE_TABLE(of, ledring_dt_ids);



static void mtimer_function(
	unsigned long data)
{
	schedule_work(&key_led_des->list_work);
	mod_timer(&key_led_des->mtimer,
		jiffies+key_led_des->run_time*HZ/1000);
}

static void list_work_func(
	struct work_struct *work)
{
	int key_val = 0,key_pre=0,key_last=0,key_label=0;
	key_val = i2c_smbus_read_byte_data(g_client, addr_led_reg3)&0xff;
	key_pre = (key_val&0xf0)>>4;
	key_last = key_val&0x0f;
	key_label = key_pre + key_last;
	if(key_label == 0 || key_label == 0xf){
		if (key_led_des->key_tmp_val != key_val) {
			key_led_des->key_tmp_val = key_val;
			if (key_val != 0){
				if(key_led_des->key_status == 0)
					key_led_des->key_last_val = key_val;
			}
			key_led_des->key_status = key_val;
			switch (key_led_des->key_last_val) {
				case MUTE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[0].name,exp_key[0].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[0].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[0].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[0].key_val,0);
					}
				break;
				case PAUSE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%ddown\n", exp_key[1].name,exp_key[1].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[1].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[1].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[1].key_val,0);
					}
				break;
				case VOLUP_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[2].name,exp_key[2].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[2].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[2].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[2].key_val,0);
					}
				break;
				case VOLDOWN_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[3].name,exp_key[3].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[3].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[3].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[3].key_val,0);
					}
				break;
				case VOLUP_VOLDOWN_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[4].name,exp_key[4].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[4].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[4].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[4].key_val,0);
					}
				break;
				case MUTE_PAUSE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[5].name,exp_key[5].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[5].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[5].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[5].key_val,0);
					}
					input_event(key_led_des->pca_input_dev,EV_SYN,0,0);
				break;
				case VOLDOWN_PAUSE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[6].name,exp_key[6].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[6].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[6].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[6].key_val,0);
					}
				break;

				case VOLUP_PAUSE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[7].name,exp_key[7].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[7].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[7].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[7].key_val,0);
					}
				break;
				case VOLUP_MUTE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[8].name,exp_key[8].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[8].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[8].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[8].key_val,0);
					}
				break;
				case VOLDOWN_MUTE_KEY:
					if (key_val != 0) {
						pr_info("key \"%s\" val:%d down\n", exp_key[9].name,exp_key[9].key_val);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[9].key_val,1);
					} else {
						pr_info("key \"%s\" up\n", exp_key[9].name);
						input_event(key_led_des->pca_input_dev,EV_KEY,exp_key[9].key_val,0);
					}
				break;
				default:
			//	pr_info("key value:0x%x\n", key_led_des->key_last_val);
				break;
			}
			input_event(key_led_des->pca_input_dev,EV_SYN,0,0);
		}
	}
}

static int setup_timer_task(int mode)
{
	setup_timer(&key_led_des->mtimer, mtimer_function, mode);
	mod_timer(&key_led_des->mtimer, jiffies+key_led_des->run_time*HZ/1000);
	INIT_WORK(&key_led_des->list_work, list_work_func);
	return 0;
}



static long leds_ioctl(struct file *file,
			unsigned int cmd,
			unsigned long args)
{
	int ret,val[2],buf[2];
	int i,j,m,count;
	char version[10];
	led_info ledctl;

	switch (cmd) {
	case CMD_LEDRING_ARG:
		if(copy_from_user(val,(int *)args, sizeof(val)))
			pr_err("copy from user failed\n");
		for(i=0;i<3;i++){
			ret = i2c_smbus_write_byte_data(g_client,addr_led_reg4, val[0]);
			if (ret < 0){
				pr_err("led set reg4 fail!\n");
				udelay(500);
				continue;
			}
			break;
		}
		break;
	case CMD_LEDRING_VOLUME:
		if(copy_from_user(val,(int *)args, sizeof(val)))
			pr_err("copy from user failed\n");
		for(i=0;i<3;i++){
			ret = i2c_smbus_write_byte_data(g_client,addr_led_reg6, val[0]);
			if (ret < 0){
				pr_err("led set reg6 fail!\n");
				udelay(500);
					continue;
				}
				ret = i2c_smbus_write_byte_data(g_client,addr_led_reg4, val[1]);
				if (ret < 0){
					pr_err("led set reg4 fail!\n");
					udelay(500);
					continue;
				}
				break;
			}
			break;
		case CMD_LEDRING_WAKEUP:
			if(copy_from_user(val,(int *)args, sizeof(val)))
				pr_err("copy from user failed\n");
			for(i=0;i<3;i++){
				ret = i2c_smbus_write_byte_data(g_client,addr_led_reg5, val[0]);
				if (ret < 0){
					pr_err("led set reg5 fail!\n");
					udelay(500);
					continue;
				}
				ret = i2c_smbus_write_byte_data(g_client,addr_led_reg4, val[1]);
				if (ret < 0){
					pr_err("led set reg4 fail!\n");
					udelay(500);
					continue;
				}
				break;
			}
			break;
		case CMD_LEDRING_SELFDEF:
			if(copy_from_user(&ledctl,(led_info *)args, sizeof(ledctl)))
				pr_err("copy from user failed\n");
			for(i=0;i<6;i++){
				for(j=0;j<3;j++){
					ret = i2c_smbus_write_byte_data(g_client,addr_led_regR+j, ledctl.led_all_data[j]);
					if (ret < 0){
						pr_err("led set regRGB fail!\n");
						udelay(500);
						break;
					}
				}
				if(j < 3)
					continue;
				for(j=0;j<3;j++){
					ret = i2c_smbus_write_byte_data(g_client,addr_led_duratTime, ledctl.duration_time);
					if(ret<0){
						printk("dura time fail");
						udelay(500);
						continue;
					}
					break;
				}

				if(ledctl.pattern==LED_SIG_ROLL || ledctl.pattern==LED_ALL_FLASH){
					for(j=0;j<3;j++){
						ret = i2c_smbus_write_byte_data(g_client,addr_led_reg5, ledctl.period);
						if (ret < 0){
							pr_err("led set reg5 fail!\n");
							udelay(500);
							continue;
						}
						break;
					}
					if(j>2)
						continue;
				}
				for(j=0;j<3;j++){	
					ret = i2c_smbus_write_byte_data(g_client,addr_led_reg4, ledctl.pattern);
					if (ret < 0){
						pr_err("led set reg4 fail!\n");
						udelay(500);
						continue;
					}
					break;
				}
				if(j>2)
					continue;
				break;	
			}
			break;
		case CMD_LEDRING_SELFSIG:
			if(copy_from_user(&ledctl,(led_info *)args, sizeof(ledctl)))
				pr_err("copy from user failed\n");
			for(i=0;i<6;i++){
				j=0;
				count=0;
				while(j < 24){
					ret = i2c_smbus_write_byte_data(g_client,addr_led_regSb+j, ledctl.led_sig_data[j]);
					if (ret < 0){
						pr_err("led set reg selfdef fail fail!\n");
						count++;
						if(count>10)
							break;
						udelay(500);
						continue;
					}
					j++;
				}
				if(j < 23)
					continue;
				for(j=0;j<3;j++){
					ret = i2c_smbus_write_byte_data(g_client,addr_led_duratTime, ledctl.duration_time);
					if(ret<0){
						printk("dura time fail");
						udelay(500);
						continue;
					}
					break;
				}
				if(ledctl.pattern==LED_ROLL_BAK || ledctl.pattern==LED_ROLL_POSITION){
					for(j=0;j<3;j++){
						for(m=0;m<3;m++){
							ret = i2c_smbus_write_byte_data(g_client,addr_led_reg5, ledctl.period);
							if (ret < 0){
								pr_err("led set reg5 fail!\n");
								udelay(500);
								continue;
							}
							break;
						}
						if(m>2)
							continue;

						for(m=0;m<3;m++){
							ret = i2c_smbus_write_byte_data(g_client,addr_led_regR+m, ledctl.led_all_data[m]);
							if (ret < 0){
								pr_err("led set regRGB fail!\n");
								udelay(500);
								break;
							}
						}
						if(m < 3)
							continue;
						if(ledctl.pattern == LED_ROLL_POSITION){
							for(m=0;m<3;m++){
								ret = i2c_smbus_write_byte_data(g_client,addr_led_reg6, ledctl.position);
								if (ret < 0){
									pr_err("led set reg5 fail!\n");
									udelay(500);
									continue;
								}
								break;
							}
							if(m>2)
								continue;
						}
						break;
					}
					if(j>2)
						continue;
				}
				if(ledctl.pattern==LED_ALL_ROLL || ledctl.pattern==LED_SIG_FLASH){
					for(j=0;j<3;j++){
						ret = i2c_smbus_write_byte_data(g_client,addr_led_reg5, ledctl.period);
						if (ret < 0){
							pr_err("led set reg5 fail!\n");
							udelay(500);
							continue;
						}
						break;
					}
					if(j>2)
						continue;
				}
				for(j=0;j<3;j++){	
					ret = i2c_smbus_write_byte_data(g_client,addr_led_reg4, ledctl.pattern);
					if (ret < 0){
						pr_err("led set reg4 fail!\n");
						udelay(500);
						continue;
					}
					break;
				}
				if(j>2)
					continue;
				break;
			}
		break;
	case CMD_READ_VERSION:
		for(i=0;i<3;i++){
			buf[0] = i2c_smbus_read_byte_data(g_client, addr_led_reg1);
			buf[1] = i2c_smbus_read_byte_data(g_client, addr_led_reg2);
			if ((buf[1] < 0)&&(buf[0]<0)){
				pr_err("read mcu software version fail!\n");
				udelay(500);
				continue;
			}
			break;
		}
		sprintf(version, "%d.%d",buf[0],buf[1]);
		if(copy_to_user((char *)args,version, sizeof(val)))
			pr_err("copy to usr fail\n");
		
		break;
	default:
		break;
	}
	return 0;
}

static ssize_t leds_read(struct file *filp, char __user *buf,
				size_t count,loff_t *ppos)
{
	unsigned long ret;
	ret = copy_to_user(buf, &key_led_des->mode,
		sizeof(key_led_des->mode));
	return count;
}

static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.read = leds_read,
	.compat_ioctl = leds_ioctl,
	.unlocked_ioctl = leds_ioctl,
};

static int ledring_parse_child_dt(const struct device *dev, int *mode)
{
	int ret, cnt;
	int m_mode = 0;
	const char* uname;

	if(dev->of_node) {
		ret = of_property_read_u32(dev->of_node, "mode", &m_mode);
		if (ret) {
			pr_err("does not have a valid mode property\n");
			return -EINVAL;
		}
		*mode = m_mode;
		ret = of_property_read_string(dev->of_node, "key_dev_name", &uname);
		if (ret) {
			pr_err("does not have a valid key_dev_name property\n");
			return -EINVAL;
		}
		strncpy(key_led_des->dev_name, uname, MAX_NAME_LEN);
		if (m_mode == 1) {
			ret = of_property_read_u32(dev->of_node, "key_num", &key_led_des->key_num);
			if (ret) {
				pr_err("failed to get key_num!\n");
				return -EINVAL;
			}
			exp_key  = kzalloc(sizeof(struct _key_des)*key_led_des->key_num, GFP_KERNEL);
				if (!exp_key) {
					pr_err("exp_key alloc mem failed!\n");
					return -ENOMEM;
			}
			for (cnt = 0; cnt < key_led_des->key_num; cnt++) {
				ret = of_property_read_string_index(dev->of_node,
					"key_name", cnt, &uname);
				if (ret < 0) {
					pr_err("invalid key name index[%d]\n",cnt);
					return  -EINVAL;
				}
				strncpy(exp_key[cnt].name, uname, MAX_NAME_LEN);
				ret = of_property_read_u32_index(dev->of_node,
					"key_value", cnt, &exp_key[cnt].key_val);
				if (ret < 0) {
					pr_err("invalid key code index[%d]\n",cnt);
					return  -EINVAL;
				}
			}
		}
		return 0;
	}
	return -EINVAL;
}

static int ledring_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	int ret, i;
	struct device *dev = &client->dev;
	

	g_client = client;
	pr_info("%s\n", __func__);
	key_led_des = devm_kzalloc(dev, sizeof(struct _key_led), GFP_KERNEL);
	if (!key_led_des)
		return -ENOMEM;

	ret = ledring_parse_child_dt(dev, &key_led_des->mode);
	if (ret < 0) {
		pr_err("%s,ledring_parse_child_dt fail!\n", __func__);
		return -EINVAL;
	}

	key_led_des->pca_input_dev = input_allocate_device();
	if (key_led_des->pca_input_dev == NULL) {
		pr_err("input_allocate_device err!\n");
		goto err3;
	}
	set_bit(EV_SYN,key_led_des->pca_input_dev->evbit);
	set_bit(EV_KEY,key_led_des->pca_input_dev->evbit);
	for (i=0; i<key_led_des->key_num; i++) {
		set_bit(exp_key[i].key_val,key_led_des->pca_input_dev->keybit);
	}
	key_led_des->pca_input_dev->name = key_led_des->dev_name;
	ret=input_register_device(key_led_des->pca_input_dev);
	if (ret != 0) {
		pr_err("input_register_device err!\n");
		goto err2;
	}
	key_led_des->run_time = 100; //200ms
	key_led_des->key_tmp_val = 0;
	key_led_des->key_status = 0;
	setup_timer_task(key_led_des->mode);
	key_led_des->major = register_chrdev(0, LED_CHAR_DEV_NAME, &led_fops);
	key_led_des->cls = class_create(THIS_MODULE, LED_DEVICE_NAME);
	device_create(key_led_des->cls, NULL, MKDEV(key_led_des->major, 0), NULL, LED_DEVICE_NAME);

	return 0;
err2:
	input_free_device(key_led_des->pca_input_dev);
	return -ENOMEM;
err3:
	return -ENOMEM;
}

static int ledring_remove(struct i2c_client *client)
{
	pr_info("%s\n", __func__);

	device_destroy(key_led_des->cls, MKDEV(key_led_des->major, 0));
	class_destroy(key_led_des->cls);
	unregister_chrdev(key_led_des->major, LED_CHAR_DEV_NAME);
	flush_work(&key_led_des->list_work);
	del_timer(&key_led_des->mtimer);

	if (key_led_des->mode == 0) {
		kobject_put(ledring_kobj);
	} else {
		input_unregister_device(key_led_des->pca_input_dev);
		input_free_device(key_led_des->pca_input_dev);
	}
	return 0;
}


static struct i2c_driver ledring_drv = {
	.driver = {
		.name = "aml_ledring",
		.owner = THIS_MODULE,
		.of_match_table = ledring_dt_ids,
	},
	.probe = ledring_probe,
	.remove = ledring_remove,
	.id_table = ledring_id,
};

static int __init ledring_init(void)
{
	return i2c_add_driver(&ledring_drv);
}

static void __exit ledring_exit(void)
{
	i2c_del_driver(&ledring_drv);
}

arch_initcall(ledring_init);
module_exit(ledring_exit);
MODULE_AUTHOR("renjun.xu <renjun.xu@amlogic.com>");
MODULE_DESCRIPTION("i2c driver for ledring");
MODULE_LICENSE("GPL");
