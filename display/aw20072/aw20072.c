/*
 * Copyright 2020 Xiaomi inc.
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * Driver for the AW20072 LED driver chip.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include <linux/amlogic/i2c-amlogic.h>

#define DEVICE_NAME "AW20072"
#define DRV_VERSION  "2.2"


struct leds_aw20072_platform_data {
	struct i2c_client *client;//0x3b
	struct work_struct work_update;
	struct timer_list timer;
	bool timer_run_flag;
	u8 led_position;
	int background_color;
	int frontground_color;
	u32* color_data;
	int color_data_num;
	int hwen_pin;
	unsigned int imax;
	struct mutex aw_lock;
};

#define LEDS_MAX		24

#define AW_I2C_RETRIES		2
#define AW_I2C_RETRY_DELAY	1

static struct leds_aw20072_platform_data *pdata = NULL;

static unsigned int aw_led_arry[LEDS_MAX] = {0x39,0x45,0x18,0xc,0x0,0x24,0x30,0x3c,0x3,0xf,0x1b,0x27,0x33,0x3f,0x6,0x12,0x1e,0x2a,0x36,0x21,0x9,0x15,0x42,0x2d};

#define AW20072_IMAX_NAME_MAX       32
static char aw20072_imax_name[][AW20072_IMAX_NAME_MAX] = {
	{"AW20072_IMAX_10mA"},
	{"AW20072_IMAX_20mA"},
	{"AW20072_IMAX_30mA"},
	{"AW20072_IMAX_40mA"},
	{"AW20072_IMAX_60mA"},
	{"AW20072_IMAX_80mA"},
	{"AW20072_IMAX_120mA"},
	{"AW20072_IMAX_160mA"},
	{"AW20072_IMAX_3P3mA"},
	{"AW20072_IMAX_6P7mA"},
	{"AW20072_IMAX_10mA"},
	{"AW20072_IMAX_13P3mA"},
	{"AW20072_IMAX_20mA"},
	{"AW20072_IMAX_26P7mA"},
	{"AW20072_IMAX_40mA"},
	{"AW20072_IMAX_53P6mA"},
};



/******************************************************
 *
 * aw20072 i2c write/read
 *
 ******************************************************/
static int aw20072_i2c_write(struct i2c_client *i2client, 
         unsigned char reg_addr, unsigned char reg_data)
{
    return i2c_smbus_write_byte_data(i2client, reg_addr, reg_data);
}

static int aw20072_i2c_read(struct i2c_client *i2client, 
        unsigned char reg_addr, unsigned char *reg_data)
{
	int ret = -1;
	unsigned char cnt = 0;

	while(cnt < 2) {
		ret = i2c_smbus_read_byte_data(i2client, reg_addr);
		if(ret < 0) {
			dev_err(&i2client->dev, "[AW20072] %s: i2c_read cnt=%d error=%d\n", __func__, cnt, ret);
		} else {
			*reg_data = ret;
			break;
		}
		cnt ++;
		msleep(1);
	}

	return ret;
}

static void write_led(u8 position, u32 data, struct leds_aw20072_platform_data *pdata)
{
	unsigned int dbuf[3];

	if (position >= LEDS_MAX) {
		return;
	}
	
	dbuf[0] = (data >> 16) & 0xff;dbuf[0]=dbuf[0]>>2;
	dbuf[1] = (data >>  8) & 0xff;dbuf[1]=dbuf[1]>>2;
	dbuf[2] = (data >>  0) & 0xff;dbuf[2]=dbuf[2]>>2;
	dbuf[1] = (dbuf[1]*((dbuf[1]<<2)+ 270))/1000;
	dbuf[2] = (dbuf[2] * 38)/100;
	dbuf[0] = (dbuf[0] * 86)/100;

	aw20072_i2c_write(pdata->client, 0xF0, 0xC1);
	aw20072_i2c_write(pdata->client, aw_led_arry[position] + 0, dbuf[0]);
	aw20072_i2c_write(pdata->client, aw_led_arry[position] + 1, dbuf[1]);
	aw20072_i2c_write(pdata->client, aw_led_arry[position] + 2, dbuf[2]);
}

#if 0
static u8 next_led(u8 position)
{
	if (position >= LEDS_MAX - 1) {
		return 0;
	}
	return ++position;
}
#endif

static void work_update_func(struct work_struct *work)
{	
	static u8 color_step = 0;
	u8 led_num = 0;
	struct leds_aw20072_platform_data *pdata = container_of(work, struct leds_aw20072_platform_data, work_update);
#if 0
	if (!color_step) {
		write_led(pdata->led_position, pdata->background_color, pdata);
		pdata->led_position = next_led(pdata->led_position);
	}
	write_led(pdata->led_position, pdata->color_data[color_step], pdata);
	write_led(next_led(pdata->led_position), pdata->color_data[pdata->color_data_num - color_step], pdata);
    
	if (color_step == pdata->color_data_num) {
		color_step = 0;
	} else {
		color_step ++;
	}
#else

	if (color_step == 1) {
		color_step = 0;
		for ( led_num=0; led_num<LEDS_MAX; led_num++ )
			write_led(led_num, 0xFEFEFE, pdata);
	} else {
		for ( led_num=0; led_num<LEDS_MAX; led_num++ )
			write_led(led_num, 0x000000, pdata);
		color_step ++;
	}
#endif
}

static void timer_sr(unsigned long dev_id)
{
	struct leds_aw20072_platform_data *pdata = (struct leds_aw20072_platform_data *)dev_id;

	schedule_work(&pdata->work_update);
	mod_timer(&pdata->timer, jiffies + msecs_to_jiffies(500));
}

static int leds_aw20072_reset(struct leds_aw20072_platform_data *pdata)
{
	unsigned int i;
	unsigned char reg_val = 0;

	// software reset
	aw20072_i2c_write(pdata->client, 0xF0, 0xC0);
	aw20072_i2c_write(pdata->client, 0x02, 0x01);
	msleep(5);
	aw20072_i2c_write(pdata->client, 0x03, 0x18);

	// read chip id
	aw20072_i2c_read(pdata->client, 0x00, &reg_val);
	if(reg_val == 0x18)
	{
		dev_info(&pdata->client->dev, "[AW20072] chip ID is 0x%x\n",reg_val);
	}
	else
	{
		dev_err(&pdata->client->dev, "[AW20072] got ID %x\n",reg_val);
		return -ENODEV;
	}

	// set IMAX 30mA
	pdata->imax = 0x2;
	aw20072_i2c_write(pdata->client, 0xF0, 0xC0);
	aw20072_i2c_read(pdata->client, 0x03, &reg_val);
	reg_val &= (~(0xF<<4));
	reg_val |= (pdata->imax<<4);
	aw20072_i2c_write(pdata->client, 0x03, reg_val);
	
	// set SLPR.SLEEP 0
	aw20072_i2c_write(pdata->client, 0x01, 0x0);

	//set DIM
	aw20072_i2c_write(pdata->client, 0xF0, 0xC1);
	for ( i=0x0; i<=0x47; i++ )
		aw20072_i2c_write(pdata->client, i, 0x0);

	// set all LEDs bright (FADE)
	aw20072_i2c_write(pdata->client, 0xF0, 0xC2);
	for ( i=0x0; i<=0x47; i++ ){
		aw20072_i2c_write(pdata->client, i, 0xFF);
	}

	return 0;
}

static int leds_aw20072_led_write(unsigned int led, u32 value, struct leds_aw20072_platform_data *pdata)
{
	unsigned int dbuf[3];
	unsigned int i,j;

	pdata->led_position = led;
	
	if (pdata->timer_run_flag) {
		del_timer(&pdata->timer);

		mutex_lock(&pdata->aw_lock);
		j = pdata->led_position;
		if ( j >= (LEDS_MAX - 2) )
			i = 0;
		else
			i = j+2;
		for ( j=0; j<LEDS_MAX; i++,j++ )
		{
			if ( LEDS_MAX == i )
				i = 0;
			aw20072_i2c_write(pdata->client, aw_led_arry[i] + 0, 0);
			aw20072_i2c_write(pdata->client, aw_led_arry[i] + 1, 0);
			aw20072_i2c_write(pdata->client, aw_led_arry[i] + 2, 0);
		}
		mutex_unlock(&pdata->aw_lock);

		pdata->timer_run_flag = false;
	}else{
		//RGB
		dbuf[2] = (value >> 0) & 0xff;dbuf[2]=dbuf[2]>>2;
		dbuf[1] = (value >> 8) & 0xff;dbuf[1]=dbuf[1]>>2;
		dbuf[0] = (value >> 16) & 0xff;dbuf[0]=dbuf[0]>>2;
			
		dbuf[1] = (dbuf[1]*((dbuf[1]<<2)+ 270))/1000;
		dbuf[2] = (dbuf[2] * 38)/100;
        dbuf[0] = (dbuf[0] * 84)/100;

		aw20072_i2c_write(pdata->client, aw_led_arry[led] + 0, dbuf[0]);
		aw20072_i2c_write(pdata->client, aw_led_arry[led] + 1, dbuf[1]);
		aw20072_i2c_write(pdata->client, aw_led_arry[led] + 2, dbuf[2]);
	}
	return 0;
}

static ssize_t led_hwen_store(struct device* dev, struct device_attribute *attr,
                const char* buf, size_t len)
{
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);
	unsigned int databuf[1]={0};

	if ( 1 == sscanf(buf,"%x",&databuf[0]) )
	{
		if ( 1 == databuf[0] )
		{
			if ( gpio_is_valid(pdata->hwen_pin) )
			{
				gpio_set_value_cansleep(pdata->hwen_pin, 0);
				msleep(1);
				gpio_set_value_cansleep(pdata->hwen_pin, 1);
				msleep(1);
			}
		}
		else
		{
			if ( gpio_is_valid(pdata->hwen_pin) )
			{
				gpio_set_value_cansleep(pdata->hwen_pin, 0);
				msleep(1);
			}
		}
	}
	
	return len;
}

static ssize_t led_hwen_show(struct device* dev,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);

	if ( gpio_is_valid(pdata->hwen_pin) ) {
		len += snprintf(buf+len, PAGE_SIZE-len, "hwen = %d\n",
	        gpio_get_value(pdata->hwen_pin));
	} else {
		len += snprintf(buf+len, PAGE_SIZE-len, "hwen = 1\n");
	}
	
	return len;
}

static ssize_t led_imax_store(struct device* dev, struct device_attribute *attr,
                const char* buf, size_t len)
{
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);
	unsigned int databuf[1];
	unsigned char reg_val;

	sscanf(buf,"%x",&databuf[0]);
	pdata->imax = databuf[0];
	if ( pdata->imax > 0xF )
		pdata->imax = 0xF;

	aw20072_i2c_write(pdata->client, 0xF0, 0xC0);
	aw20072_i2c_read(pdata->client, 0x03, &reg_val);
	reg_val &= (~(0xF<<4));
	reg_val |= (pdata->imax<<4);
	
	aw20072_i2c_write(pdata->client, 0x03, reg_val);
	
	return len;
}

static ssize_t led_imax_show(struct device* dev,struct device_attribute *attr, char* buf)
{
	ssize_t len = 0;
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);

	len += snprintf(buf+len, PAGE_SIZE-len, "current imax = 0x%02x, value = %s\n",
        pdata->imax, aw20072_imax_name[pdata->imax]);
	
	return len;
}

static ssize_t led_control(struct device *dev,
                           struct device_attribute *attr, const char *buff, size_t count)
{
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);
	unsigned int argn;
	char *p, *para,*buf_work;
	char *argv[2];
	unsigned int arg1, arg2;
	char *stop_at = NULL;

	buf_work = kstrdup(buff, GFP_KERNEL);
	p = buf_work;

	for (argn = 0; argn < 2; argn++) {
		para = strsep(&p, " ");
		if (para == NULL) {
			break;
		}
		argv[argn] = para;
	}

	if (argn != 2) {
		dev_err(dev, "[AW20072] %s : The wrong format. (Need 2 arguments)\n", __func__);
		goto go_exit;
	}

	arg1 = simple_strtol(argv[0], &stop_at, 0);
	arg2 = simple_strtol(argv[1], &stop_at, 0);

	if (arg1 > 23) {
		dev_err(dev, "[AW20072] %s : The wrong format. (Only 0 - 23 LEDs)\n", __func__);
		goto go_exit;
	}

	aw20072_i2c_write(pdata->client, 0xF0, 0xC1);
	leds_aw20072_led_write(arg1, arg2, pdata);

go_exit:
	kfree(buf_work);
	return count;

}

static ssize_t led_usage(struct device *dev,
                         struct device_attribute *attr, char *buff)
{
	ssize_t len = 0;

	len += sprintf(buff + len, "Usage:\n");
	len += sprintf(buff + len, "\t  echo A B > led_rgb\n");
	len += sprintf(buff + len, "\t  Format:\n");
	len += sprintf(buff + len, "\t  A    : 0 - 23, Max 24 LEDs\n");
	len += sprintf(buff + len, "\t  B    : bits  0 -  7 BLUE Level\n");
	len += sprintf(buff + len, "\t       : bits  8 - 15 GREEN Level\n");
	len += sprintf(buff + len, "\t       : bits 16 - 23 RED Level\n");
	
	return len;
}

static int leds_aw20072_fade_write(char rgb, u32 value, struct leds_aw20072_platform_data *pdata)
{
	unsigned int led_num;
	unsigned int fade;

	if ( value > 0xFF )
		fade = 0xFF;
	else
		fade = value;

	mutex_lock(&pdata->aw_lock);
	switch ( rgb )
	{
		case 'r':
			aw20072_i2c_write(pdata->client, 0xF0, 0xC2);
			for ( led_num=0; led_num<24; led_num++ )
				aw20072_i2c_write(pdata->client, aw_led_arry[led_num] + 0, fade);
			break;
		case 'g':
			aw20072_i2c_write(pdata->client, 0xF0, 0xC2);
			for ( led_num=0; led_num<24; led_num++ )
				aw20072_i2c_write(pdata->client, aw_led_arry[led_num] + 1, fade);
			break;
		case 'b':
			aw20072_i2c_write(pdata->client, 0xF0, 0xC2);
			for ( led_num=0; led_num<24; led_num++ )
				aw20072_i2c_write(pdata->client, aw_led_arry[led_num] + 2, fade);
			break;
		default:
			pr_info("[AW20072] unkown rgb %d\n",rgb);
			break;
	}
	mutex_unlock(&pdata->aw_lock);

	return 0;
}

static ssize_t led_fade_control(struct device *dev,
                           struct device_attribute *attr, const char *buff, size_t count)
{
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);
	char rgb;
	unsigned int value;

	if(2 == sscanf(buff, "%c %x", &rgb, &value)) {
		leds_aw20072_fade_write(rgb, value, pdata);
	}

	return count;
}

static ssize_t led_position_func(struct device *dev,
                                 struct device_attribute *attr, char *buff)
{
	struct leds_aw20072_platform_data *pdata = dev_get_drvdata(dev);

	return sprintf(buff, "%d\n", pdata->led_position);
}

static DEVICE_ATTR(led_rgb, S_IWUSR | S_IRUSR, led_usage, led_control);
static DEVICE_ATTR(led_fade, S_IWUSR | S_IRUSR, NULL, led_fade_control);
static DEVICE_ATTR(led_position, S_IRUSR, led_position_func, NULL);
static DEVICE_ATTR(led_imax, S_IWUSR | S_IRUSR, led_imax_show, led_imax_store);
static DEVICE_ATTR(led_hwen, S_IWUSR | S_IRUSR, led_hwen_show, led_hwen_store);

static struct attribute *leds_aw20072_attrs[] = {
	&dev_attr_led_rgb.attr,
	&dev_attr_led_fade.attr,
	&dev_attr_led_position.attr,
	&dev_attr_led_imax.attr,
	&dev_attr_led_hwen.attr,
	NULL
};

static const struct attribute_group leds_aw20072_attr_group = {
	.attrs = leds_aw20072_attrs,
};

static int leds_aw20072_parse_dt(struct device *dev,
                                     struct leds_aw20072_platform_data *pdata)
{
	int ret;

	ret = of_property_read_u32(dev->of_node, "color_step", &pdata->color_data_num);
	if (ret) {
		dev_err(dev, "[AW20072] %s : get color_step failed from dts\n", __func__);
		return -ENODEV;
	}
	pdata->color_data = kzalloc(sizeof(int) * pdata->color_data_num, GFP_KERNEL);
	if (!pdata->color_data) {
		dev_err(dev, "[AW20072] %s : out of memory\n", __func__);
		return -ENOMEM;
	}
	ret = of_property_read_u32_array(dev->of_node, "color_arrary", pdata->color_data + 1, pdata->color_data_num);
	if (ret) {
		dev_err(dev, "[AW20072] %s : not match color data\n", __func__);
		goto failed;
	}
	ret = of_property_read_u32(dev->of_node, "bg_color", &pdata->background_color);
	if (ret) {
		dev_err(dev, "[AW20072] %s : get bg_color failed from dts\n", __func__);
		goto failed;
	}
	ret = of_property_read_u32(dev->of_node, "fg_color", &pdata->frontground_color);
	if (ret) {
		dev_err(dev, "[AW20072] %s : get fg_color failed from dts\n", __func__);
		goto failed;
	}
	pdata->color_data[0] = pdata->frontground_color;
	return 0;

failed:
	kfree(pdata->color_data);
	return -ENODEV;
}

static int leds_aw20072_module_init(struct device *dev,
                                     struct leds_aw20072_platform_data *pdata)
{
	int ret;
#if 0
	pdata->hwen_pin = of_get_named_gpio(dev->of_node, "hwen_pin", 0);
	if ( pdata->hwen_pin < 0 ) {
		dev_err(dev, "[AW20072] %s : get hwen pin error.\n", __func__);
	} else {
		// Enable chip
		if ( gpio_is_valid(pdata->hwen_pin) )
		{
			ret = devm_gpio_request_one(&pdata->client->dev, pdata->hwen_pin,
						    GPIOF_OUT_INIT_LOW, "aw20072-hwen-pin");
			gpio_set_value_cansleep(pdata->hwen_pin, 0);
	        msleep(1);
	        gpio_set_value_cansleep(pdata->hwen_pin, 1);
	        msleep(1);
		}
		else
		{
			pdata->hwen_pin = -1;
			dev_err(dev, "[AW20072] Error in get hwen pin.\n");
			return -ENODEV;
		}
	}
#endif	
	ret = leds_aw20072_reset(pdata);
	if ( ret )
	{
		dev_err(dev, "[AW20072] reset chip error...\n");
		return -ENODEV;
	}

	return 0;
}

static int leds_aw20072_i2c_probe(struct i2c_client *client,
                                      const struct i2c_device_id *id)
{
	int error;

	if (client->dev.of_node)
	{
		if ( !pdata )
		{
			pdata = kzalloc(sizeof(struct leds_aw20072_platform_data), GFP_KERNEL);
			if (!pdata)
			{
				dev_err(&client->dev, "[AW20072] Failed to allocate memroy for pdata\n");
				return -ENOMEM;
			}
			error = leds_aw20072_parse_dt(&client->dev, pdata);
			if (error)
			{
				return error;
			}
		}
	} else {
		pdata = client->dev.platform_data;
	}

	if (!pdata) {
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "[AW20072] error %s: functionality check failed\n", __func__);
		goto err_free_data;
	} else {
		if ( client->addr == 0x3b )
        	pdata->client = client;
		else {
			dev_err(&client->dev, "[AW20072] the client addr = %d\n",client->addr);
			return -EINVAL;
		}
	}

	if ( client->addr == 0x3b )
		dev_set_drvdata(&pdata->client->dev, pdata);
	else
		return -EINVAL;

	mutex_init(&pdata->aw_lock);

	// init aw20072
	error = leds_aw20072_module_init(&client->dev, pdata);
	if ( error )
	{		
		dev_err(&client->dev, "[AW20072] Failure %d init chip\n", error);
		goto err_free_data;
	}

	error = sysfs_create_group(&client->dev.kobj, &leds_aw20072_attr_group);
	if (error) {
		dev_err(&client->dev, "[AW20072] Failure %d creating sysfs group\n", error);
		goto err_free_data;
	}

	if ( pdata->client )
	{
		INIT_WORK(&pdata->work_update, work_update_func);
		setup_timer(&pdata->timer, timer_sr, (unsigned long)pdata);
		mod_timer(&pdata->timer, jiffies + msecs_to_jiffies(500));
		pdata->timer_run_flag = true;
	}

	return 0;

err_free_data:
	if ( gpio_is_valid(pdata->hwen_pin) )
		devm_gpio_free(&client->dev, pdata->hwen_pin);
	kfree(pdata);
	
	return -EINVAL;
}

static int leds_aw20072_i2c_remove(struct i2c_client *client)
{
	struct leds_aw20072_platform_data *pdata = i2c_get_clientdata(client);

	pr_info("[AW20072] %s\n", __func__);
	del_timer(&pdata->timer);
	if ( client->addr == 0x3b )
		sysfs_remove_group(&client->dev.kobj, &leds_aw20072_attr_group);

	if ( gpio_is_valid(pdata->hwen_pin) )
		devm_gpio_free(&client->dev, pdata->hwen_pin);
	
	kfree(pdata);

	return 0;
}

static const struct i2c_device_id leds_aw20072_i2c_id[] = {
	{"aw20072", 0},
	{}
};
static const struct of_device_id leds_aw20072_match_id = {
	.compatible = "xiaomi, leds-aw20072",
};
static struct i2c_driver leds_aw20072_i2c_driver = {
	.probe    = leds_aw20072_i2c_probe,
	.remove   = leds_aw20072_i2c_remove,
	.id_table = leds_aw20072_i2c_id,
	.driver = {
		.name = DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = &leds_aw20072_match_id,
	},
};

static int __init leds_aw20072_init(void)
{
	pr_info("[AW20072] LED AW20072 Driver Ver %s \n", DRV_VERSION);;
	return i2c_add_driver(&leds_aw20072_i2c_driver);
}

static void __exit leds_aw20072_exit(void)
{
	pr_info("[AW20072] %s\n", __func__);
	i2c_del_driver(&leds_aw20072_i2c_driver);
}

module_init(leds_aw20072_init);
module_exit(leds_aw20072_exit);

MODULE_AUTHOR("xiaomi");
MODULE_DESCRIPTION("AW20072 LED Driver");
MODULE_LICENSE("GPL v2");

