#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/i2c-dev.h>

#include "ntp8810.h"

#define NTP8810_RATES (SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000 | \
		       SNDRV_PCM_RATE_64000 | \
		       SNDRV_PCM_RATE_88200 | \
		       SNDRV_PCM_RATE_96000 | \
		       SNDRV_PCM_RATE_176400 | \
		       SNDRV_PCM_RATE_192000)

#define NTP8810_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE)
#if 0
unsigned int spk_mute_state;

static int ntp8810_get_mute(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol);
static int ntp8810_set_mute(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);

#endif

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 1);

static const struct snd_kcontrol_new ntp8810_snd_controls[] = {
       SOC_SINGLE_TLV("Master", MVOL, 0x00, 0xff, 1, mvol_tlv),
//	   	SOC_SINGLE_BOOL_EXT("SPK unmute", 0,
//		   ntp8810_get_mute, ntp8810_set_mute),
};

static struct i2c_client *ntp8810_i2c;
static struct ntp8810_priv *ntp8810_priv_codec;

//#define EDONG_DEBUG

#define I2C_SPEED  (200*1000)

#define PEQ_NTP8810_ON

static int m_reg_tab[NTP8810_REGISTER_COUNT][2] = {

{0x00,0x00},
{0x20,0x00},
{0x09,0x20},
//{0x00,0x00},
{0x21,0x4E},
{0x22,0x4E},
{0x23,0x4E},
{0x24,0x4E},
{0x3C,0x3F},
{0x3D,0x3F},
{0x3E,0x0F},
{0x3F,0x0F},
{0x0A,0x00},
{0x2A,0xAD},
{0x2B,0x34},
//{0x04,0xFF},
//{0x05,0x00},
{0x06,0xAD},
{0x07,0xAD},
{0x08,0x6C},
{0x35,0x0B},
{0x39,0xE4},
{0x43,0x29},
//{0x31,0x00},
{0x44,0x10},
{0x45,0x22},
{0x46,0x04},
{0x40,0x0E},
{0x42,0x03},
{0x38,0x56},
//{0x30,0x00},
{0x37,0x00},
//{0x49,0x06},
//{0x5D,0x00},
{0x3A,0x02},
//{0x7E,0x01},
//{0x7E,0x00},
//{0x04,0xFF},

};

#ifdef PEQ_NTP8810_ON

static int Speaker_table_EQ[NTP8810_RAM_TABLE_COUNT][5]= {
{0x00,0x11,0x15,0xFE,0x41},
{0x01,0x12,0x95,0xFE,0x41},
{0x02,0x11,0x15,0xFE,0x41},
{0x03,0x11,0x7F,0x53,0xFB},
{0x04,0x10,0xFE,0xA9,0xC1},
{0x05,0x11,0x00,0x3E,0x83},
{0x06,0x11,0xFF,0x28,0xD5},
{0x07,0x10,0x7D,0xD7,0x6F},
{0x08,0x11,0x7F,0x28,0xD5},
{0x09,0x10,0xFE,0x54,0x77},
{0x0A,0x10,0x7F,0xD4,0x3D},
{0x0B,0x11,0xFE,0xBC,0x2D},
{0x0C,0x10,0x7D,0xB6,0x22},
{0x0D,0x11,0x7E,0xBC,0x2D},
{0x0E,0x10,0xFD,0x8A,0x60},
{0x0F,0x11,0x02,0xB9,0x99},
{0x10,0x11,0xE9,0x84,0x7F},
{0x11,0x10,0x5C,0xA7,0x24},
{0x12,0x11,0x69,0x84,0x7F},
{0x13,0x10,0xE2,0x1A,0x57},
{0x14,0x11,0x07,0x10,0xBE},
{0x15,0x11,0xF8,0xF6,0x21},
{0x16,0x10,0x66,0x09,0x5B},
{0x17,0x11,0x78,0xF6,0x21},
{0x18,0x10,0xF4,0x2A,0xD7},
{0x19,0x10,0x6D,0xE6,0xB6},
{0x1A,0x11,0xE5,0xF7,0xC1},
{0x1B,0x10,0x62,0xD1,0x3F},
{0x1C,0x11,0x65,0xF7,0xC1},
{0x1D,0x10,0xD0,0xB7,0xF6},
{0x1E,0x11,0x05,0x14,0xFC},
{0x1F,0x11,0xEB,0x53,0x95},
{0x20,0x10,0x5F,0x62,0xBE},
{0x21,0x11,0x6B,0x53,0x95},
{0x22,0x10,0xE9,0x8C,0xB7},
{0x23,0x10,0x5C,0xC6,0x8A},
{0x24,0x11,0xB2,0x61,0x07},
{0x25,0x10,0x3B,0xA3,0xD1},
{0x26,0x11,0x32,0x61,0x07},
{0x27,0x10,0x98,0x6A,0x5B},
{0x45,0x11,0x12,0xFF,0x4F},
{0x46,0x11,0xD7,0x08,0xA2},
{0x47,0x10,0x37,0x72,0x8F},
{0x48,0x11,0x57,0x08,0xA2},
{0x49,0x10,0xDD,0x71,0x2D},
{0x4F,0x10,0x7C,0xA6,0xDF},
{0x50,0x11,0xF5,0x43,0x56},
{0x51,0x10,0x71,0x33,0xFC},
{0x52,0x11,0x75,0x43,0x56},
{0x53,0x10,0xED,0xDA,0xDC},
};

#endif


static int sound_on_reg_tab[NTP8810_SOUNDON_REG_COUNT][2] = {
                            //sound on
                            {0x32,0x04},  // PWM MASK ON
                            {0x31,0x00},  // PWM Switching ON
                            {0x30,0x00},   // Soft-mute OFF
                            {0x04,0xFF},   //Master volume Setting 
                     };

static int sound_off_reg_tab[NTP8810_SOUNDOFF_REG_COUNT][2] = {
                            //sound off
                            {0x30,0x03},  // PWM MASK ON
                            {0x31,0x03},  // PWM Switching ON
                            {0x32,0x06},   // Soft-mute OFF
                            {0x04,0x00},   //Master volume Setting 
                     };

/* codec private data */
struct ntp8810_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct ntp8810_platform_data *pdata;
	struct clk *mclk_tx;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static int i2c_master_send_byte(struct i2c_client *client, const char *buf, int count, int scl_rate)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count;
	msg.buf = (char *)buf;
	//msg.scl_rate = scl_rate;
	ret = i2c_transfer(adap, &msg, 1);
	return (ret == 1) ? count : ret;
}

static int ntp8810_write_reg(struct i2c_client *client,u8 addr,int *pdata,int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen;

	bytelen = 0;

	if (datalen > 125) {
		pr_err("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	//memset(tmp_buf,0,sizeof(tmp_buf));

	tmp_buf[0] = addr;
	bytelen++;
	if (datalen != 0 && pdata != NULL) {
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		
		bytelen += datalen;
		//printk("%s mdg--2 = %d, tmp_buf = 0x%s\n", __func__, bytelen,tmp_buf);
	}
	ret = i2c_master_send_byte(client, tmp_buf, bytelen, I2C_SPEED);
	//ret = i2c_master_send(client, tmp_buf, bytelen);
	
	//printk("ntp8810_write_reg ret=%d\n",ret);
	return ret;
}

#ifdef PEQ_NTP8810_ON

static int PEQ_NTP8810(struct i2c_client *client)
{
	unsigned int i;

	int buf1[1] = {0x00};
	int buf2[1] = {0x00};
	//int buf3[4] = {0x00,0x00,0x00,0x00};
	//u8 buf4[5] = {0x00,0x11,0x0F,0x62,0x24};
	char buf3[5] = {0x00,0x00,0x00,0x00,0x00};

	buf1[0] = 0x7E;
	buf2[0] = 0x01;
	ntp8810_write_reg(client, buf1[0], buf2,1);

	//  i2c_master_send(client, buf4, 5);

    for (i = 0; i < NTP8810_RAM_TABLE_COUNT; i++) {
		//printk("PEQ_NTP8810  write 0x%x = 0x%x  0x%x  0x%x  0x%x\n",Speaker_table_EQ[i][0],Speaker_table_EQ[i][1],Speaker_table_EQ[i][2],Speaker_table_EQ[i][3],Speaker_table_EQ[i][4]);

		buf3[0] = Speaker_table_EQ[i][0];
		buf3[1] = Speaker_table_EQ[i][1];
		buf3[2] = Speaker_table_EQ[i][2];
		buf3[3] = Speaker_table_EQ[i][3];
		buf3[4] = Speaker_table_EQ[i][4];

        //ntp8810_write_reg(client, buf1[0], buf3,4);
		i2c_master_send(client, buf3, 5);

		//ntp8810_write_reg(client, Speaker_table_EQ[i][0], &Speaker_table_EQ[i][1], 4);
    };

    buf1[0] = 0x7E;
    buf2[0] = 0x00;
    ntp8810_write_reg(client, buf1[0], buf2,1);   

    return 0;    
}
#endif

static int reset_ntp8810_GPIO(struct ntp8810_platform_data *pdata)
{
	if (pdata->reset_pin < 0)
		return 0;
	
       printk("reset_ntp8810_GPIO=%d\n", pdata->reset_pin);

	mdelay(100);
	gpio_direction_output(pdata->reset_pin, 1);
	mdelay(100);
	gpio_direction_output(pdata->reset_pin, 0);
	//udelay(1000);
       mdelay(100);
	gpio_direction_output(pdata->reset_pin, 1);
	mdelay(100);

	return 0;
}

static int ntp8810_reg_init(struct i2c_client *client)
{
	int i = 0;
	int buf1[1] = {0x00};
	int buf2[1] = {0x00};
	
	for (i = 0; i < NTP8810_REGISTER_COUNT; i++) {
        buf1[0] = m_reg_tab[i][0];
		buf2[0] = m_reg_tab[i][1];
		ntp8810_write_reg(client, buf1[0], buf2,1);
	};
       
	return 0;
}


static int ntp8810_sound_on(struct i2c_client *client)
{
	int i = 0;
	int buf1[1] = {0x00};
	int buf2[1] = {0x00};

	for (i = 0; i < NTP8810_SOUNDON_REG_COUNT; i++) {
		buf1[0] = sound_on_reg_tab[i][0];
		buf2[0] = sound_on_reg_tab[i][1];
		ntp8810_write_reg(client, buf1[0], buf2,1);
	};       

	return 0;
}

static int ntp8810_sound_off(struct i2c_client *client)
{
	int i = 0;
	int buf1[1] = {0x00};
	int buf2[1] = {0x00};
	for (i = 0; i < NTP8810_SOUNDOFF_REG_COUNT; i++) {
		buf1[0] = sound_off_reg_tab[i][0];
		buf2[0] = sound_off_reg_tab[i][1];
		ntp8810_write_reg(client, buf1[0], buf2,1);
	};          

	return 0;
}

static int ntp8810_init(struct i2c_client *client,struct ntp8810_platform_data *pdata)
{
	reset_ntp8810_GPIO(pdata);

		ntp8810_reg_init(client);

		ntp8810_sound_off(client);

       mdelay(2);
	/*eq and drc*/
#ifdef PEQ_NTP8810_ON
	PEQ_NTP8810(client);
#endif       
       printk("%s\n", __func__);
	/*unmute,default power-on is mute.*/
       ntp8810_sound_on(client);
	return 0;
}

static ssize_t ntp8810_store(struct kobject *kobj, struct kobj_attribute *attr, 
			const char *buf, size_t count)
{
	ntp8810_init(ntp8810_i2c, ntp8810_priv_codec->pdata);
	return count;
}

#ifdef EDONG_DEBUG 
static ssize_t ntp8810_eq_store(struct kobject *kobj, struct kobj_attribute *attr, 
			const char *buf, size_t count)
{
	
	int buf1[1] = {0x00};
	int buf2[1] = {0x00};
	//	static int i=0;
	// int buf3[4] = {0x00,0x00,0x00,0x00};
	char bufxx[]={0x00, 0x44, 0x0f, 0x62, 0x24};
	
	buf1[0] = 0x7E;
	buf2[0] = 0x01;
	ntp8810_write_reg(ntp8810_i2c, buf1[0], buf2,1);
	
	/*
	printk("PEQ_NTP8810  write 0x%x = 0x%x  0x%x	0x%x  0x%x\n",Speaker_table_EQ[i][0],Speaker_table_EQ[i][1],Speaker_table_EQ[i][2],Speaker_table_EQ[i][3],Speaker_table_EQ[i][4]);
	
	buf1[0] = Speaker_table_EQ[i][0];		//00
	buf3[0] = Speaker_table_EQ[i][1];		//11
	buf3[1] = Speaker_table_EQ[i][2];		//0f
	buf3[2] = Speaker_table_EQ[i][3];		//62
	buf3[3] = Speaker_table_EQ[i][4];		//24
*/
	i2c_master_send(ntp8810_i2c, bufxx, 5);
	//ntp8810_write_reg(ntp8810_i2c, buf1[0], buf3,4);

	buf1[0] = 0x7E;
	buf2[0] = 0x00;
	ntp8810_write_reg(ntp8810_i2c, buf1[0], buf2,1);   

	return count;
}

static int mute = 0;

static ssize_t ntp8810_mute_store(struct kobject *kobj, struct kobj_attribute *attr, 
			const char *buf, size_t count)
{

	//mute = *(int*)buf;

    printk("ntp8810_mute_store mute = %d\n", mute);
	if (mute == 1) {
		printk("%s\n", __func__);
		ntp8810_sound_on(ntp8810_i2c);
		mute = 0;
	} else if (mute == 0){
		ntp8810_sound_off(ntp8810_i2c);
		mute = 1;
		msleep(10);
	} 

	return count;
}
#endif


static struct kobj_attribute ntp8810_attr = __ATTR_WO(ntp8810);
#ifdef EDONG_DEBUG
static struct kobj_attribute ntp8810_eq_attr = __ATTR_WO(ntp8810_eq);
static struct kobj_attribute ntp8810_mute_attr = __ATTR_WO(ntp8810_mute);
#endif
#if 0
static int ntp8810_get_mute(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = spk_mute_state;

	return 0;
}

static int ntp8810_set_mute(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	spk_mute_state = ucontrol->value.integer.value[0];

	if (spk_mute_state == 1) {
		ntp8810_sound_on(ntp8810_i2c);
	} else {
		ntp8810_sound_off(ntp8810_i2c);
	}


	return 0;
}
#endif

static int ntp8810_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{	
		unsigned int rate;

	rate = params_rate(params);
	pr_debug("%s\n", __func__);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
		pr_debug("24bit\n");
	/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");

		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ntp8810_startup(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static int ntp8810_prepare(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
	pr_debug("%s\n", __func__);
	//ntp8810_init(ntp8810_i2c, ntp8810_priv_codec->pdata);
	return 0;
}

static int ntp8810_trigger(struct snd_pcm_substream *substream, int mute,
    struct snd_soc_dai *dai)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static int ntp8810_set_dai_fmt(struct snd_soc_dai *codec_dai, 
				unsigned int fmt)
{
	pr_debug("%s\n", __func__);
	return 0;
}

static struct snd_soc_dai_ops ntp8810_ops = {
	  .hw_params = ntp8810_hw_params,
	  .set_fmt = ntp8810_set_dai_fmt,
	  .startup = ntp8810_startup,
	  .prepare = ntp8810_prepare,
	  .trigger = ntp8810_trigger,
};

struct snd_soc_dai_driver ntp8810_dai = {
	.name = "ntp8810_codec",
	.playback = {
		.stream_name = "NTP8810 PCM",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE |
			    SNDRV_PCM_FMTBIT_S20_3LE |
			    SNDRV_PCM_FMTBIT_S24_LE |
			    SNDRV_PCM_FMTBIT_S32_LE),
	},
	.ops = &ntp8810_ops,
};
static int ntp8810_probe(struct snd_soc_codec *codec)
{
	ntp8810_init(ntp8810_i2c, ntp8810_priv_codec->pdata);
	return 0;
}

static int ntp8810_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static const struct snd_soc_codec_driver soc_ntp8810_codec = {
	.probe = ntp8810_probe,
	.remove = ntp8810_remove,

//	.component_driver = {
//		.controls = ntp8810_snd_controls,
//		.num_controls = ARRAY_SIZE(ntp8810_snd_controls),
//	}
};

static int ntp8810_parse_dt(
		struct ntp8810_priv *ntp8810,
		struct device_node *np)
{
		int ret = 0;
		int reset_pin = -1;
		
	
		pr_err("%s hanqy2:start to get reset pin from dts!\n", __func__);
	
		reset_pin = of_get_named_gpio(np, "reset_pin", 0);
		if (reset_pin < 0) {
			pr_err("%s fail to get reset pin from dts!\n", __func__);
			ret = -1;
		} else {
			pr_info("%s pdata->reset_pin = %d!\n", __func__,
					ntp8810->pdata->reset_pin);
		}
		ntp8810->pdata->reset_pin = reset_pin;
	
	
		return ret;
}

static int ntp8810_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct ntp8810_priv *ntp8810;
	struct ntp8810_platform_data *pdata;
	int ret;

	ntp8810 = devm_kzalloc(&i2c->dev, sizeof(struct ntp8810_priv),
			       GFP_KERNEL);
	if (!ntp8810)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ntp8810);

	ntp8810_i2c = i2c;
	pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct ntp8810_platform_data),
				GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for ntp8810 pdata\n", __func__);
		return -ENOMEM;
	}
	ntp8810->pdata = pdata;
	ntp8810_parse_dt(ntp8810, i2c->dev.of_node);

	gpio_direction_output(pdata->reset_pin, 1); // reset high

	ret = sysfs_create_file(&i2c->dev.kobj, &ntp8810_attr.attr);

#ifdef EDONG_DEBUG
	ret = sysfs_create_file(&i2c->dev.kobj, &ntp8810_eq_attr.attr);
	ret = sysfs_create_file(&i2c->dev.kobj, &ntp8810_mute_attr.attr);
#endif

	ntp8810_priv_codec = ntp8810;

    return snd_soc_register_codec(&i2c->dev, &soc_ntp8810_codec,
                                      &ntp8810_dai, 1);
}

static int ntp8810_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

void ntp8810_i2c_shutdown(struct i2c_client *client)
{

}

static const struct i2c_device_id ntp8810_i2c_id[] = {
	{ "ntp8810", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, ntp8810_i2c_id);

static const struct of_device_id ntp8810_of_id[] = {
	{ .compatible = "ntp, ntp8810", },
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, ntp8810_of_id);

static struct i2c_driver ntp8810_i2c_driver = {
	.driver = {
		.name = "ntp8810",
		.of_match_table = ntp8810_of_id,
		.owner = THIS_MODULE,
	},
	.shutdown = ntp8810_i2c_shutdown,
	.probe = ntp8810_i2c_probe,
	.remove = ntp8810_i2c_remove,
	.id_table = ntp8810_i2c_id,
};

module_i2c_driver(ntp8810_i2c_driver);

MODULE_DESCRIPTION("ASoC ntp8810 driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");
