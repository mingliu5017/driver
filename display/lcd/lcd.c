/******************************
*: custom driver for kelcd
*author:sws
*date:20181228 1022
*
*
*
*
******************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/amlogic/pm.h>

#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

#include "../../gpio/gpiolib.h"

#include "lcd.h"

#if 1
#define DBG(x...) printk(x)
#define DBG_FIND(x...) printk(x)
#else
#define DBG(x...)
#endif

//154液晶 默认181液晶
#define SUPPORT_154
//#define SUPPORT_180

/* 液晶最大对比度*/
#define CONTRAST_MAX 63
/* 液晶默认对比度*/
#define CONTRAST_DEFAULT 36

/* 液晶128*64 */
#define LCD_WIDTH 128
#define LCD_HEIGHT 64

#define GPIO_DIRECTION_INPUT 0
#define GPIO_DIRECTION_OUTPUT 1

/* softkey 高度*/
#define SOFTKEY_HEIGHT 16

#define LABEL_kKELCD_KPWR "kaer,gpio_pwr"
#define LABEL_kKELCD_BL   "lcd-spi-bl"
#define LABEL_kKELCD_CS   "lcd-spi-cs"
#define LABEL_kKELCD_A0   "lcd-spi-a0"
#define LABEL_kKELCD_RST  "lcd-spi-rst"
#define LABEL_kKELCD_CLK  "lcd-spi-clk"
#define LABEL_kKELCD_DATA "lcd-spi-data"

#define ke_gpio_setpin(gpio, value) gpio_set_value(gpio, value)
#define ke_gpio_getpin(gpio) gpio_get_value(gpio)

int gpio_lcd_rst = -1;
int gpio_lcd_bl = -1;
int gpio_lcd_cs = -1;
int gpio_lcd_a0 = -1;
int gpio_lcd_clk = -1;
int gpio_lcd_data = -1;

/* lcd 初始化指 */
const unsigned char LCD_INIT_CODE[] =
{
		0xac,
		0x00,
		0xa2,
		0xa0,
		0xc8,
		0x40,
		0x25,
		0x81,
		0x20,
		0x2f,
		0xf8,
		0x00,
		0xa6,
		0xaf
};

int gpio_pwr = -1;
static int kelcd_major = 0;
static int kelcd_minor = 0;

static struct class *kelcd_class = NULL;
static struct kelcd_android_dev *kelcd_dev = NULL;

static int kelcd_open(struct inode *inode, struct file *filp);
static int kelcd_release(struct inode *inode, struct file *filp);
static ssize_t kelcd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t kelcd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long kelcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

//串行模式的写操作
void data_send(unsigned char dat)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		ke_gpio_setpin(gpio_lcd_clk, 0);
		ke_gpio_setpin(gpio_lcd_data, (dat >> (7 - i)) & 0x01);
		ke_gpio_setpin(gpio_lcd_clk, 1);
	}
}

//写命
void write_cmd(unsigned char cmd)
{
	ke_gpio_setpin(gpio_lcd_cs, 0); //低电平，使能
	ke_gpio_setpin(gpio_lcd_a0, 0); //低电平，写命
	data_send(cmd);
	ke_gpio_setpin(gpio_lcd_cs, 1); //高电
}

//写数
void write_data(unsigned char dat)
{
	ke_gpio_setpin(gpio_lcd_cs, 0); //低电平，使能
	ke_gpio_setpin(gpio_lcd_a0, 1);
	data_send(dat);
	ke_gpio_setpin(gpio_lcd_cs, 1); //高电
}

void back_light(unsigned char dat)
{
	printk("back_light on=%d\n", dat);
	if (dat > 0)
	{
		ke_gpio_setpin(gpio_lcd_bl, 1);
	}
	else
	{
		ke_gpio_setpin(gpio_lcd_bl, 0);
	}
}

void lcd_close(void)
{
	write_cmd(0xae);
	write_cmd(0xe2);

	ke_gpio_setpin(gpio_lcd_cs, 1);
	mdelay(3);
	ke_gpio_setpin(gpio_lcd_cs, 0);
}

#if 0
/* 
 * lcd by GPIO simulated  clear 0 routine.
 *
 * @param whichline: GPIO control line
 *
 */
static void lcd_clr(unsigned char whichline)
{
	unsigned char n, m;
	for (n = 0; n < 9; n++)
	{
		write_cmd(0xB0 + n); // Setting Page 0 -- 7
		write_cmd(0x10);	 // Column Address Ser Upper.
		write_cmd(0x00);	 //write_cmd(0x04);	// Column Address Ser Lower.  00 is first Column
		for (m = 0; m < LCD_WIDTH; m++)
		{
			write_data(0xff);
		}
	}
}
#endif

void lcd_clean(void)
{
	unsigned char n, m;
	for (n = 0; n < 9; n++)
	{
		write_cmd(0xB0 + n); // Setting Page 0 -- 7
		write_cmd(0x10);	 // Column Address Ser Upper.
		write_cmd(0x00);	 //write_cmd(0x04);	// Column Address Ser Lower.  00 is first Column
		for (m = 0; m < LCD_WIDTH; m++)
		{
			write_data(0x0);
		}
	}
}

void lcd_set_column(unsigned char col)
{
	unsigned char lsb;

	lsb = (col & 0xF);
	col = (col >> 4) & 0xF;

	// send  msb
	write_cmd(col | 0x10);
	// send  lsb
	write_cmd(lsb);
}

void lcd_set_page(unsigned char page)
{
	unsigned char cmd;

	cmd = 0xB0 + (page & 0xF);
	write_cmd(cmd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

static int kelcd_open(struct inode *inode, struct file *filp)
{
	struct kelcd_android_dev *dev;
	printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	dev = container_of(inode->i_cdev, struct kelcd_android_dev, dev);
	filp->private_data = dev;

	printk("open device kelcd is okay!\n");
	return 0;
}

static int kelcd_release(struct inode *inode, struct file *filp)
{
	printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t kelcd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t err = 0;
	struct kelcd_android_dev *dev = filp->private_data;
	int val = 0;
	printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if (down_interruptible(&(dev->sem)))
	{
		return -ERESTARTSYS;
	}

	if (copy_to_user(buf, &val, sizeof(int)))
	{
		err = -EFAULT;
		return -1;
	}
	up(&(dev->sem));
	return err;
}

static ssize_t kelcd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct kelcd_android_dev *dev = filp->private_data;
	ssize_t err = 0;
	char value = 0;

	printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if (down_interruptible(&(dev->sem)))
	{
		return -ERESTARTSYS;
	}

	if (copy_from_user(&value, buf, count))
	{
		err = -EFAULT;
		return -1;
	}

	up(&(dev->sem));
	return err;
}

static long kelcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int ret = 0;
	int val = 0;
	int reg_val = 0;

	printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	printk("cmd=\n   %ld\n   %ld\n   %ld\n   %ld\n   %ld\n", GPIO_LCD_WRITE_CMD, GPIO_LCD_WRITE_DATA, GPIO_LCD_CLOSE, GPIO_LCD_OPEN, GPIO_LCD_BACKLIGHT);
	printk("kelcd_ioctl cmd=%d arg=%ld\n", cmd, arg);
	switch (cmd)
	{
	case GPIO_LCD_WRITE_CMD:
		printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		val = *(unsigned int *)arg;
		reg_val = val & 0xff;
		write_cmd(reg_val);
		break;

	case GPIO_LCD_WRITE_DATA:
		printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		val = *(unsigned int *)arg;
		reg_val = val & 0xff;
		write_data(reg_val);
		break;

	case GPIO_LCD_CLOSE:
		printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		break;

	case GPIO_LCD_OPEN:
		printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		break;

	case GPIO_LCD_BACKLIGHT:
		printk("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
		val = *(unsigned int *)arg;
		reg_val = val & 0xff;
		back_light(reg_val);
		break;

	default:
		return -1;
	}

	return ret;
}

static struct file_operations kelcd_fops = {
	.owner = THIS_MODULE,
	.open = kelcd_open,
	.release = kelcd_release,
	.read = kelcd_read,
	.write = kelcd_write,
	.unlocked_ioctl = kelcd_ioctl,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
static int __kelcd_setup_dev(struct kelcd_android_dev *dev)
{
	int err;
	dev_t devno = MKDEV(kelcd_major, kelcd_minor);

	memset(dev, 0, sizeof(struct kelcd_android_dev));

	cdev_init(&(dev->dev), &kelcd_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &kelcd_fops;

	err = cdev_add(&(dev->dev), devno, 1);
	if (err)
	{
		return err;
	}

	sema_init(&(dev->sem), 1);

	return 0;
}

int kelcd_parse_dt1(struct platform_device *pdev)
{
	enum of_gpio_flags flags;

	struct device_node *node;

	int setGpio = -1;
	int error = -1;

	printk("kelcd_parse_dt1 start\n");
	node = pdev->dev.of_node;

	gpio_lcd_bl = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_BL, 0, &flags);
	printk("#####sws gpio_lcd_bl: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_bl, __LINE__, __FUNCTION__, __FILE__);
	if (gpio_is_valid(gpio_lcd_bl))
	{
		error = devm_gpio_request(&pdev->dev, gpio_lcd_bl, "kelcd_bl");
		if (error)
		{
			printk("failed to request gpio_lcd_bl  %d: %d\n",
				   gpio_lcd_bl, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_bl= %d \n", gpio_pwr);
	}

	gpio_lcd_cs = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_CS, 0, &flags);
	printk("#####sws gpio_lcd_cs: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_cs, __LINE__, __FUNCTION__, __FILE__);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "kelcd_cs");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_cs GPIO %d, error %d\n", setGpio, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_cs= %d \n", gpio_pwr);
	}

	gpio_lcd_rst = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_RST, 0, &flags);
	printk("#####sws gpio_lcd_rst: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_rst, __LINE__, __FUNCTION__, __FILE__);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "kelcd_rst");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_rst GPIO %d, error %d\n", setGpio, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_rst= %d \n", gpio_pwr);
	}
	gpio_lcd_a0 = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_A0, 0, &flags);
	printk("#####sws gpio_lcd_a0: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_a0, __LINE__, __FUNCTION__, __FILE__);

	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "kelcd_a0");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_a0 GPIO %d, error %d\n", setGpio, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_a0= %d \n", gpio_pwr);
	}

	gpio_lcd_clk = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_CLK, 0, &flags);
	printk("#####sws gpio_lcd_clk: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_clk, __LINE__, __FUNCTION__, __FILE__);

	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "kelcd_clk");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_clk GPIO %d, error %d\n", setGpio, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_clk= %d \n", gpio_pwr);
	}

	gpio_lcd_data = setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_DATA, 0, &flags);
	printk("#####sws gpio_lcd_data: %d --line: %d --func:%s \n file:%s \n", gpio_lcd_data, __LINE__, __FUNCTION__, __FILE__);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "kelcd_data");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_data GPIO %d, error %d\n", setGpio, error);
		}
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_data= %d \n", gpio_pwr);
	}

	return 0;
}

#define START_COL_OFFSET 4
#define START_ROW_OFFSET 0

void hw_lcd_update(void)
{
	/* nothing for none-DMA mode driver */
	unsigned char *pLcdbuf = (unsigned char *)RES_ICON_TIETONG_LOGO;
	unsigned char line, col;

	for(line = 0; line < 8; line++)
	{
		// set display start line
		write_cmd(START_ROW_OFFSET);
		// set page
		lcd_set_page(line);

		// set start col
		lcd_set_column(START_COL_OFFSET);

		for (col = 0; col < LCD_WIDTH; col++)
		{
			write_data(*pLcdbuf);
			pLcdbuf++;
		}
	}
}

void lcd_gpio_init(void)
{
	//液晶复位
	gpio_direction_output(gpio_lcd_rst, GPIO_DIRECTION_OUTPUT);

	//液晶片选
	gpio_direction_output(gpio_lcd_cs, GPIO_DIRECTION_OUTPUT);

	//液晶A0
	gpio_direction_output(gpio_lcd_a0, GPIO_DIRECTION_OUTPUT);

	//液晶时钟
	gpio_direction_output(gpio_lcd_clk, GPIO_DIRECTION_OUTPUT);

	//液晶数据
	gpio_direction_output(gpio_lcd_data, GPIO_DIRECTION_OUTPUT);

	//液晶背光
	gpio_direction_output(gpio_lcd_bl, GPIO_DIRECTION_OUTPUT);
}

void lcd_init_code(void)
{
#ifdef SUPPORT_180
	write_cmd(0x00);
	//write_cmd(0xaf);
	//write_cmd(0xe2);
	write_cmd(0x2c);
	write_cmd(0x2e);
	write_cmd(0x2f);
	write_cmd(0xa0);
	write_cmd(0xc8);
	write_cmd(0x60);
	write_cmd(0xa6);
	write_cmd(0x25);
	write_cmd(0x81);
	write_cmd(0x20);
	//write_cmd(0xaf);

#elif defined SUPPORT_154
	write_cmd(0x00);
	write_cmd(0xaf);
	write_cmd(0xe2);
	write_cmd(0x2c);
	write_cmd(0x2e);
	write_cmd(0x2f);
	write_cmd(0xa0);
	write_cmd(0xc8);
	write_cmd(0x40 + 0x20);
	write_cmd(0xa6);
	write_cmd(0x25);
	write_cmd(0x81);
	write_cmd(0x20);
	write_cmd(0xaf);
#else
	write_cmd(0xac);
	write_cmd(0x00);
	write_cmd(0xa2);
	write_cmd(0xa1);
	write_cmd(0xc8);
	write_cmd(0x40);
	write_cmd(0x25);
	write_cmd(0x81);
	write_cmd(0x20);
	write_cmd(0x2f);
	write_cmd(0xf8);
	write_cmd(0x00);
	write_cmd(0xa6);
#endif
}
static int kelcd_probe(struct platform_device *pdev)
{

	int err = -1;

	struct device *temp = NULL;
	dev_t dev = 0;
	printk(KERN_ERR "Initializing kelcd device.\n");

	err = alloc_chrdev_region(&dev, 0, 1, KELCD_DEVICE_NODE_NAME);
	if (err < 0)
	{
		printk(KERN_ALERT "Failed to alloc char kelcd_dev region.\n");
		goto fail;
	}

	kelcd_major = MAJOR(dev);
	kelcd_minor = MINOR(dev);

	kelcd_dev = kmalloc(sizeof(struct kelcd_android_dev), GFP_KERNEL);
	if (!kelcd_dev)
	{
		err = -ENOMEM;
		printk(KERN_ALERT "Failed to alloc kelcd_dev.\n");
		goto unregister;
	}

	err = __kelcd_setup_dev(kelcd_dev);
	if (err)
	{
		printk(KERN_ALERT "Failed to setup kelcd_dev: %d.\n", err);
		goto cleanup;
	}

	kelcd_class = class_create(THIS_MODULE, KELCD_DEVICE_CLASS_NAME);
	if (IS_ERR(kelcd_class))
	{
		err = PTR_ERR(kelcd_class);
		printk(KERN_ALERT "Failed to create kelcd class.\n");
		goto destroy_cdev;
	}

	temp = device_create(kelcd_class, NULL, dev, "%s", KELCD_DEVICE_FILE_NAME);
	if (IS_ERR(temp))
	{
		err = PTR_ERR(temp);
		printk(KERN_ALERT "Failed to create kelcd device.");
		goto destroy_class;
	}
	dev_set_drvdata(temp, kelcd_dev);

	kelcd_parse_dt1(pdev);

	lcd_gpio_init();
	printk(KERN_ERR "Succedded to initialize kelcd device.\n");

	//lcd重置
	ke_gpio_setpin(gpio_lcd_rst, 0);
	mdelay(1);
	ke_gpio_setpin(gpio_lcd_rst, 1);
	mdelay(1);
	printk(KERN_ERR "lcd rst.\n");

	lcd_init_code();
	printk(KERN_ERR "lcd lcd_init_code.\n");
	lcd_clean();
	printk(KERN_ERR "lcd lcd_clean.\n");
	hw_lcd_update();
	printk(KERN_ERR "lcd hw_lcd_update.\n");
	back_light(0);

	return 0;
destroy_class:
	class_destroy(kelcd_class);

destroy_cdev:
	cdev_del(&(kelcd_dev->dev));

cleanup:
	kfree(kelcd_dev);

unregister:
	unregister_chrdev_region(MKDEV(kelcd_major, kelcd_minor), 1);

fail:
	return err;
}

static int kelcd_remove(struct platform_device *pdev)
{
	dev_t devno = MKDEV(kelcd_major, kelcd_minor);

	printk(KERN_ALERT "Destroy kelcd device.\n");
	flush_scheduled_work();

	if (kelcd_class)
	{
		device_destroy(kelcd_class, MKDEV(kelcd_major, kelcd_minor));
		class_destroy(kelcd_class);
	}

	if (kelcd_dev)
	{
		cdev_del(&(kelcd_dev->dev));
		kfree(kelcd_dev);
	}

	unregister_chrdev_region(devno, 1);
	return 0;
}

static int kelcd_suspend(struct platform_device *dev, pm_message_t state)
{

	return 0;
}

static int kelcd_resume(struct platform_device *dev)
{

	return 0;
}

static const struct of_device_id kelcd_dt_match[] = {
	{	.compatible = "amlogic, lcd-display", },
	{},
};

static struct platform_driver kelcd_driver = {
	.probe = kelcd_probe,
	.remove = kelcd_remove,
	.suspend = kelcd_suspend,
	.resume = kelcd_resume,
	.driver = {
		.name = "lcd-display",
		.of_match_table = kelcd_dt_match,
	},
};

module_platform_driver(kelcd_driver);

MODULE_AUTHOR("Amlogic Liu Ming");
MODULE_DESCRIPTION("lcd drivers");
MODULE_VERSION("2.0.0");
MODULE_LICENSE("GPL");
