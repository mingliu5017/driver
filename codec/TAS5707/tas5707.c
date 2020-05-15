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
#include <sound/tas57xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "tas5707.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void tas5707_early_suspend(struct early_suspend *h);
static void tas5707_late_resume(struct early_suspend *h);
#endif

#define TAS5707_RATES (SNDRV_PCM_RATE_8000 | \
		       SNDRV_PCM_RATE_11025 | \
		       SNDRV_PCM_RATE_16000 | \
		       SNDRV_PCM_RATE_22050 | \
		       SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000)

#define TAS5707_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

/* Power-up register defaults */
struct reg_default tas5707_reg_defaults[TAS5707_REGISTER_COUNT] = {
/*	{0x00, 0x6c},
	{0x01, 0x70},
	{0x02, 0x00},
	{0x03, 0xA0},
	{0x04, 0x05},
	{0x05, 0x40},
	{0x06, 0x00},
	{0x07, 0xFF},
	{0x08, 0x30},
	{0x09, 0x30},
	{0x0A, 0xFF},
	{0x0B, 0x00},
	{0x0C, 0x00},
	{0x0D, 0x00},
	{0x0E, 0x91},
	{0x10, 0x00},
	{0x11, 0x02},
	{0x12, 0xAC},
	{0x13, 0x54},
	{0x14, 0xAC},
	{0x15, 0x54},
	{0x16, 0x00},
	{0x17, 0x00},
	{0x18, 0x00},
	{0x19, 0x00},
	{0x1A, 0x30},
	{0x1B, 0x0F},
	{0x1C, 0x82},
	{0x1D, 0x02}*/
};

static unsigned int tas5707_EQ_table_length = 280;
static unsigned int tas5707_EQ_table[280] = {
};

static unsigned int tas5707_drc1_tko_length = 12;
static unsigned int tas5707_drc1_tko_table[12] = {

};

/* codec private data */
struct tas5707_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct tas57xx_platform_data *pdata;

	/*Platform provided EQ configuration */
	int num_eq_conf_texts;
	const char **eq_conf_texts;
	int eq_cfg;
	struct soc_enum eq_conf_enum;
	unsigned char Ch1_vol;
	unsigned char Ch2_vol;
	unsigned char master_vol;
	unsigned int mclk;
#ifdef CONFIG_HAS_EARLYSUSPEND
       struct early_suspend early_suspend;
#endif
};

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -12700, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

static const struct snd_kcontrol_new tas5707_snd_controls[] = {
	
};

static int tas5707_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);

	tas5707->mclk = freq;
	
	return 0;
}

static int tas5707_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return 0;//-EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return 0;//-EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return 0;//-EINVAL;
	}

	return 0;
}

static int tas5707_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	unsigned int rate;

	rate = params_rate(params);
	pr_debug("rate: %u\n", rate);

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

static int tas5707_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	pr_debug("level = %d\n", level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		break;
	}
	codec->component.dapm.bias_level = level;

	return 0;
}

static const struct snd_soc_dai_ops tas5707_dai_ops = {
	.hw_params = tas5707_hw_params,
	.set_sysclk = tas5707_set_dai_sysclk,
	.set_fmt = tas5707_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5707_dai = {
	.name = "tas5707",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = TAS5707_RATES,
		.formats = TAS5707_FORMATS,
	},
	.ops = &tas5707_dai_ops,
};


static int reset_tas5707_GPIO(struct snd_soc_codec *codec)
{
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = tas5707->pdata;
	int ret = 0;

	if (pdata->reset_pin < 0)
		return 0;

	ret = devm_gpio_request_one(codec->dev, pdata->reset_pin,
					    GPIOF_OUT_INIT_LOW,
					    "tas5707-reset-pin");
	if (ret < 0)
		return -1;

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	mdelay(15);

	return 0;
}



static unsigned char Speaker_table_EQ[50][4] =
{
{0x11,0x65,0x7E,0x93},{0x12,0xE5,0x7E,0X93},{0x11,0x65,0x7E,0x93},{0x11,0x7F,0x26,0xAD},{0x10,0xFE,0x50,0x35},
{0x10,0x7F,0xA6,0x32},{0x11,0xFC,0x5A,0xB0},{0x10,0x79,0x39,0x10},{0x11,0x7C,0x5A,0xB0},{0x10,0xF8,0xDF,0x42},
{0x10,0x7C,0x54,0x7E},{0x11,0xD7,0x4C,0xD5},{0x10,0x5A,0x60,0xCE},{0x11,0x57,0x4C,0xD5},{0x10,0xD6,0xB5,0x4D},
{0x10,0x52,0xBD,0x1F},{0x10,0xC9,0x3E,0xC9},{0x0F,0x30,0x17,0xCF},{0x10,0x49,0x3E,0xC9},{0x0E,0xAB,0x24,0x21},
{0x10,0x6F,0xF9,0x93},{0x11,0xDC,0xAE,0xF8},{0x10,0x54,0x68,0xF6},{0x11,0x5C,0xAE,0xF8},{0x10,0xC4,0x62,0x87},
{0x10,0x7A,0xD6,0xBA},{0x11,0xE4,0xE0,0x89},{0x10,0x55,0x18,0xE1},{0x11,0x64,0xE0,0x89},{0x10,0xCF,0xEF,0x9B},
{0x10,0x7E,0x82,0x9B},{0x11,0xFB,0xC1,0xC7},{0x10,0x79,0x6A,0x77},{0x11,0x7B,0xC1,0xC7},{0x10,0xF7,0xED,0x12},
{0x10,0x7E,0X63,0X65},{0X11,0xFD,0xCF,0x49},{0x10,0x7D,0x41,0x70},{0x11,0x7D,0xCF,0x49},{0x10,0xFB,0xA4,0xD6},
{0x11,0x00,0x13,0x59},{0x11,0xFF,0x83,0x96},{0x10,0x7E,0xE6,0x20},{0x11,0x7F,0x83,0x96},{0x10,0xFF,0x0C,0xD2},
{0x11,0x00,0x00,0x00},{0x20,0x00,0x00,0x00},{0x20,0x00,0x00,0x00},{0x20,0x00,0x00,0x00},{0x20,0x00,0x00,0x00}
};


static int tas5707_set_eq(struct snd_soc_codec *codec)
{
struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
        
//unsigned int i;
snd_soc_write(codec,0x7E,0x01);

//for(i=0; i<25; i++)
// regmap_raw_write(tas5707->regmap, i,Speaker_table_EQ[i], 4);
regmap_raw_write(tas5707->regmap, 0x00,Speaker_table_EQ[0], 4);
regmap_raw_write(tas5707->regmap, 0x01,Speaker_table_EQ[1], 4);
regmap_raw_write(tas5707->regmap, 0x02,Speaker_table_EQ[2], 4);
regmap_raw_write(tas5707->regmap, 0x03,Speaker_table_EQ[3], 4);
regmap_raw_write(tas5707->regmap, 0x04,Speaker_table_EQ[4], 4);
regmap_raw_write(tas5707->regmap, 0x05,Speaker_table_EQ[5], 4);
regmap_raw_write(tas5707->regmap, 0x06,Speaker_table_EQ[6], 4);
regmap_raw_write(tas5707->regmap, 0x07,Speaker_table_EQ[7], 4);
regmap_raw_write(tas5707->regmap, 0x08,Speaker_table_EQ[8], 4);
regmap_raw_write(tas5707->regmap, 0x09,Speaker_table_EQ[9], 4);
regmap_raw_write(tas5707->regmap, 0x0A,Speaker_table_EQ[10], 4);
regmap_raw_write(tas5707->regmap, 0x0B,Speaker_table_EQ[11], 4);
regmap_raw_write(tas5707->regmap, 0x0C,Speaker_table_EQ[12], 4);
regmap_raw_write(tas5707->regmap, 0x0D,Speaker_table_EQ[13], 4);
regmap_raw_write(tas5707->regmap, 0x0E,Speaker_table_EQ[14], 4);
regmap_raw_write(tas5707->regmap, 0x0F,Speaker_table_EQ[15], 4);
regmap_raw_write(tas5707->regmap, 0x10,Speaker_table_EQ[16], 4);
regmap_raw_write(tas5707->regmap, 0x11,Speaker_table_EQ[17], 4);
regmap_raw_write(tas5707->regmap, 0x12,Speaker_table_EQ[18], 4);
regmap_raw_write(tas5707->regmap, 0x13,Speaker_table_EQ[19], 4);
regmap_raw_write(tas5707->regmap, 0x14,Speaker_table_EQ[20], 4);
regmap_raw_write(tas5707->regmap, 0x15,Speaker_table_EQ[21], 4);
regmap_raw_write(tas5707->regmap, 0x16,Speaker_table_EQ[22], 4);
regmap_raw_write(tas5707->regmap, 0x17,Speaker_table_EQ[23], 4);
regmap_raw_write(tas5707->regmap, 0x18,Speaker_table_EQ[24], 4);


//for(i=25; i<30; i++)
regmap_raw_write(tas5707->regmap, 0x4F,Speaker_table_EQ[25], 4);
regmap_raw_write(tas5707->regmap, 0x50,Speaker_table_EQ[26], 4);
regmap_raw_write(tas5707->regmap, 0x51,Speaker_table_EQ[27], 4);
regmap_raw_write(tas5707->regmap, 0x52,Speaker_table_EQ[28], 4);
regmap_raw_write(tas5707->regmap, 0x53,Speaker_table_EQ[29], 4);

//for(i=30; i<45; i++)
regmap_raw_write(tas5707->regmap, 0x19,Speaker_table_EQ[30], 4);
regmap_raw_write(tas5707->regmap, 0x1A,Speaker_table_EQ[31], 4);
regmap_raw_write(tas5707->regmap, 0x1B,Speaker_table_EQ[32], 4);
regmap_raw_write(tas5707->regmap, 0x1C,Speaker_table_EQ[33], 4);
regmap_raw_write(tas5707->regmap, 0x1D,Speaker_table_EQ[34], 4);
regmap_raw_write(tas5707->regmap, 0x1E,Speaker_table_EQ[35], 4);
regmap_raw_write(tas5707->regmap, 0x1F,Speaker_table_EQ[36], 4);
regmap_raw_write(tas5707->regmap, 0x20,Speaker_table_EQ[37], 4);
regmap_raw_write(tas5707->regmap, 0x21,Speaker_table_EQ[38], 4);
regmap_raw_write(tas5707->regmap, 0x22,Speaker_table_EQ[39], 4);
regmap_raw_write(tas5707->regmap, 0x23,Speaker_table_EQ[40], 4);
regmap_raw_write(tas5707->regmap, 0x24,Speaker_table_EQ[41], 4);
regmap_raw_write(tas5707->regmap, 0x25,Speaker_table_EQ[42], 4);
regmap_raw_write(tas5707->regmap, 0x26,Speaker_table_EQ[43], 4);
regmap_raw_write(tas5707->regmap, 0x27,Speaker_table_EQ[44], 4);
//for(i=45; i<50; i++)
regmap_raw_write(tas5707->regmap, 0x45,Speaker_table_EQ[45], 4);
regmap_raw_write(tas5707->regmap, 0x46,Speaker_table_EQ[46], 4);
regmap_raw_write(tas5707->regmap, 0x47,Speaker_table_EQ[47], 4);
regmap_raw_write(tas5707->regmap, 0x48,Speaker_table_EQ[48], 4);
regmap_raw_write(tas5707->regmap, 0x49,Speaker_table_EQ[49], 4);
snd_soc_write(codec,0x7E,0x00);

        return 0;
}


static int tas5707_init(struct snd_soc_codec *codec)
{
	
//	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);

	reset_tas5707_GPIO(codec);

	dev_info(codec->dev, "tas5707_init!\n");
	
        snd_soc_write(codec,0x02,0x01);
        snd_soc_write(codec,0x00,0x00);
        snd_soc_write(codec,0x40,0x0E);
        snd_soc_write(codec,0x45,0x23);
        snd_soc_write(codec,0x3A,0x02);
        snd_soc_write(codec,0x2A,0xA8);
        snd_soc_write(codec,0x2B,0x01);
        snd_soc_write(codec,0x08,0x7D);
        snd_soc_write(codec,0x06,0xB7);
        snd_soc_write(codec,0x07,0xB7);

        tas5707_set_eq(codec);
       
        snd_soc_write(codec,0x30,0x03);
        snd_soc_write(codec,0x3c,0x3f);
        snd_soc_write(codec,0x3d,0x3f);
        snd_soc_write(codec,0x3e,0x0f);
        snd_soc_write(codec,0x3f,0x0f);
        snd_soc_write(codec,0x30,0x00);


        snd_soc_write(codec,0x32,0x04);
        snd_soc_write(codec,0x31,0x00);
        snd_soc_write(codec,0x30,0x00);
        snd_soc_write(codec,0x04,0xff);
       
	/*drc */
//	tas5707_set_drc(codec);
	/*eq */
//	tas5707_set_eq(codec);

       
	

	return 0;
}

static int tas5707_probe(struct snd_soc_codec *codec)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	tas5707->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	tas5707->early_suspend.suspend = tas5707_early_suspend;
	tas5707->early_suspend.resume = tas5707_late_resume;
	tas5707->early_suspend.param = codec;
	register_early_suspend(&(tas5707->early_suspend));
#endif

	tas5707_init(codec);

	return 0;
}

static int tas5707_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(tas5707->early_suspend));
#endif

	return 0;
}

#ifdef CONFIG_PM
static int tas5707_suspend(struct snd_soc_codec *codec)
{
//	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5707_suspend!\n");

	if (pdata && pdata->suspend_func)
		pdata->suspend_func();

	
	tas5707_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int tas5707_resume(struct snd_soc_codec *codec)
{
///	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5707_resume!\n");

	if (pdata && pdata->resume_func)
		pdata->resume_func();

	tas5707_init(codec);
	
	tas5707_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}
#else
#define tas5707_suspend NULL
#define tas5707_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tas5707_early_suspend(struct early_suspend *h)
{
}

static void tas5707_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget tas5707_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver soc_codec_dev_tas5707 = {
	.probe = tas5707_probe,
	.remove = tas5707_remove,
	.suspend = tas5707_suspend,
	.resume = tas5707_resume,
	.set_bias_level = tas5707_set_bias_level,
	.component_driver = {
		.controls = tas5707_snd_controls,
		.num_controls = ARRAY_SIZE(tas5707_snd_controls),
		.dapm_widgets = tas5707_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tas5707_dapm_widgets),
	}
};

static const struct regmap_config tas5707_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = TAS5707_REGISTER_COUNT,
	.reg_defaults = tas5707_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(tas5707_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int tas5707_parse_dt(
	struct tas5707_priv *tas5707,
	struct device_node *np)
{
	int ret = 0;
	int reset_pin = -1;

	reset_pin = of_get_named_gpio(np, "reset_pin", 0);
	if (reset_pin < 0) {
		pr_err("%s fail to get reset pin from dts!\n", __func__);
		ret = -1;
	} else {
		pr_info("%s pdata->reset_pin = %d!\n", __func__,
				tas5707->pdata->reset_pin);
	}
	tas5707->pdata->reset_pin = reset_pin;

	return ret;
}

static int tas5707_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct tas5707_priv *tas5707;
	struct tas57xx_platform_data *pdata;
	int ret;

	tas5707 = devm_kzalloc(&i2c->dev, sizeof(struct tas5707_priv),
			       GFP_KERNEL);
	if (!tas5707)
		return -ENOMEM;

	tas5707->regmap = devm_regmap_init_i2c(i2c, &tas5707_regmap);
	if (IS_ERR(tas5707->regmap)) {
		ret = PTR_ERR(tas5707->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	i2c_set_clientdata(i2c, tas5707);

	pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct tas57xx_platform_data),
				GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for tas5707 pdata\n", __func__);
		return -ENOMEM;
	}
	tas5707->pdata = pdata;

	tas5707_parse_dt(tas5707, i2c->dev.of_node);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_tas5707,
				     &tas5707_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int tas5707_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static const struct i2c_device_id tas5707_i2c_id[] = {
	{ "tas5707", 0 },
	{}
};

static const struct of_device_id tas5707_of_id[] = {
	{.compatible = "ti,tas5707",},
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, tas5707_of_id);

static struct i2c_driver tas5707_i2c_driver = {
	.driver = {
		.name = "tas5707",
		.of_match_table = tas5707_of_id,
		.owner = THIS_MODULE,
	},
	.probe = tas5707_i2c_probe,
	.remove = tas5707_i2c_remove,
	.id_table = tas5707_i2c_id,
};
module_i2c_driver(tas5707_i2c_driver);

module_param_array(tas5707_EQ_table,
		uint, &tas5707_EQ_table_length, 0664);
MODULE_PARM_DESC(tas5707_EQ_table,
		"An array of tas5707 EQ param");

MODULE_PARM_DESC(tas5707_drc1_table,
		"An array of tas5707 DRC table param");

module_param_array(tas5707_drc1_tko_table,
		uint, &tas5707_drc1_tko_length, 0664);
MODULE_PARM_DESC(tas5707_drc1_tko_table,
		"An array of tas5707 DRC tko table param");

MODULE_DESCRIPTION("ASoC Tas5707 driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");
