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
#include <sound/tas58xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "tas5805.h"

#define DEV_NAME	"tas5805"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void tas5805_early_suspend(struct early_suspend *h);
static void tas5805_late_resume(struct early_suspend *h);
#endif

#define TAS5805_RATES (SNDRV_PCM_RATE_8000 | \
		       SNDRV_PCM_RATE_11025 | \
		       SNDRV_PCM_RATE_16000 | \
		       SNDRV_PCM_RATE_22050 | \
		       SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000)

#define TAS5805_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

/* Power-up register defaults */
struct reg_default tas5805_reg_defaults[TAS5805_REGISTER_COUNT] = {
	{0x00, 0x00},
	{0x7F, 0x00},
	{0x03, 0x03}
};

static unsigned int tas5805_EQ_table_length = 280;
static unsigned int tas5805_EQ_table[280] = {
	/*0x29---ch1_bq[0]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2A---ch1_bq[1]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2B---ch1_bq[2]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2C---ch1_bq[3]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2D---ch1_bq[4]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2E---ch1_bq[5]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2F---ch1_bq[6]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x30---ch2_bq[0]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x31---ch2_bq[1]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x32---ch2_bq[2]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x33---ch2_bq[3]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x34---ch2_bq[4]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x35---ch2_bq[5]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x36---ch2_bq[6]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static unsigned int tas5805_drc1_table_length = 24;
static unsigned int tas5805_drc1_table[24] = {
	/* 0x3A drc1_ae */
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3B drc1_aa */
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x3C drc1_ad */
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static unsigned int tas5805_drc1_tko_length = 12;
static unsigned int tas5805_drc1_tko_table[12] = {
	0xFD, 0xA2, 0x14, 0x90, /*0x40---drc1_t*/
	0x03, 0x84, 0x21, 0x09, /*0x41---drc1_k*/
	0x00, 0x08, 0x42, 0x10, /*0x42---drc1_o*/
};

/* codec private data */
struct tas5805_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct tas58xx_platform_data *pdata;

	/*Platform provided EQ configuration */
	int num_eq_conf_texts;
	const char **eq_conf_texts;
	int eq_cfg;
	struct soc_enum eq_conf_enum;
	unsigned char Ch1_vol;
	unsigned char Ch2_vol;
	unsigned char master_vol;
	unsigned int mclk;
	unsigned int EQ_enum_value;
	unsigned int DRC_enum_value;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static int tas5805_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5805_get_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5805_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5805_get_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 24, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 24, 1);

static const struct snd_kcontrol_new tas5805_snd_controls[] = {
	SOC_SINGLE_TLV("Master Volume", TAS5805_DIG_VOL_LEFT, 0,
			   0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", TAS5805_DIG_VOL_LEFT, 0,
			   0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", TAS5805_DIG_VOL_RIGHT, 0,
			   0xff, 1, chvol_tlv),
	SOC_SINGLE_BOOL_EXT("Set EQ Enable", 0,
			   tas5805_get_EQ_enum, tas5805_set_EQ_enum),
	SOC_SINGLE_BOOL_EXT("Set DRC Enable", 0,
			   tas5805_get_DRC_enum, tas5805_set_DRC_enum),
};

static int tas5805_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	tas5805->mclk = freq;
	//TODO
	return 0;
}

static int tas5805_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
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

static int tas5805_hw_params(struct snd_pcm_substream *substream,
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

static int tas5805_set_bias_level(struct snd_soc_codec *codec,
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

static const struct snd_soc_dai_ops tas5805_dai_ops = {
	.hw_params = tas5805_hw_params,
	.set_sysclk = tas5805_set_dai_sysclk,
	.set_fmt = tas5805_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5805_dai = {
	.name = DEV_NAME,
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = TAS5805_RATES,
		.formats = TAS5805_FORMATS,
	},
	.ops = &tas5805_dai_ops,
};

static int tas5805_set_eq(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	u8 tas5805_eq_ctl_table[] = { 0x00, 0x00, 0x00, 0x80 };

	//TODO
	pr_info("tas5805_set_eq called!\n");

	return 0;
}

static int tas5805_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	u8 tas5805_eq_ctl_table[] = { 0x00, 0x00, 0x00, 0x80 };

	tas5805->EQ_enum_value = ucontrol->value.integer.value[0];

	//TODO
	pr_info("tas5805_set_EQ_enum called!\n");

	return 0;
}
static int tas5805_get_EQ_enum(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5805->EQ_enum_value;

	return 0;
}

static int tas5805_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	u8 tas5805_drc_ctl_table[] = { 0x00, 0x00, 0x00, 0x00 };

	tas5805->DRC_enum_value = ucontrol->value.integer.value[0];

	//TODO
	pr_info("tas5805_set_DRC_enum called!\n");

	return 0;
}
static int tas5805_get_DRC_enum(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5805->DRC_enum_value;

	return 0;
}

static int reset_tas5805_GPIO(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas58xx_platform_data *pdata = tas5805->pdata;
	int ret = 0;

	if (pdata->reset_pin < 0)
		return 0;

	ret = devm_gpio_request_one(codec->dev, pdata->reset_pin,
					    GPIOF_OUT_INIT_LOW,
					    "tas5805-reset-pin");
	if (ret < 0)
		return -1;

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	mdelay(15);

	return 0;
}

static int tas5805_init(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	reset_tas5805_GPIO(codec);

	dev_info(codec->dev, "tas5805_init!\n");
	msleep(50);
	snd_soc_write(codec, TAS5805_PAGE_SEL, 0x00);
	snd_soc_write(codec, TAS5805_BOOK_SEL, 0x00);
	snd_soc_write(codec, TAS5805_DIG_VOL_LEFT, 0x60);
	snd_soc_write(codec, TAS5805_DEVICE_CTRL_2, 0x03);
	snd_soc_write(codec, TAS5805_FAULT_CLEAR, 0x80);

	return 0;
}

static int tas5805_probe(struct snd_soc_codec *codec)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	tas5805->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	tas5805->early_suspend.suspend = tas5805_early_suspend;
	tas5805->early_suspend.resume = tas5805_late_resume;
	tas5805->early_suspend.param = codec;
	register_early_suspend(&(tas5805->early_suspend));
#endif

	tas5805_init(codec);

	return 0;
}

static int tas5805_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(tas5805->early_suspend));
#endif

	return 0;
}

#ifdef CONFIG_PM
static int tas5805_suspend(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas58xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5805_suspend!\n");

	if (pdata && pdata->suspend_func)
		pdata->suspend_func();

	/*save volume */
	tas5805->Ch1_vol = snd_soc_read(codec, TAS5805_DIG_VOL_LEFT);
	tas5805->Ch2_vol = snd_soc_read(codec, TAS5805_DIG_VOL_LEFT);
	tas5805->master_vol = snd_soc_read(codec, TAS5805_DIG_VOL_LEFT);
	tas5805_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int tas5805_resume(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas58xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5805_resume!\n");

	if (pdata && pdata->resume_func)
		pdata->resume_func();

	tas5805_init(codec);
	snd_soc_write(codec, DDX_CHANNEL1_VOL, tas5805->Ch1_vol);
	snd_soc_write(codec, DDX_CHANNEL2_VOL, tas5805->Ch2_vol);
	snd_soc_write(codec, DDX_MASTER_VOLUME, tas5805->master_vol);
	tas5805_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}
#else
#define tas5805_suspend NULL
#define tas5805_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tas5805_early_suspend(struct early_suspend *h)
{
}

static void tas5805_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget tas5805_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver soc_codec_dev_tas5805 = {
	.probe = tas5805_probe,
	.remove = tas5805_remove,
	.suspend = tas5805_suspend,
	.resume = tas5805_resume,
	.set_bias_level = tas5805_set_bias_level,
	.component_driver = {
		.controls = tas5805_snd_controls,
		.num_controls = ARRAY_SIZE(tas5805_snd_controls),
		.dapm_widgets = tas5805_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tas5805_dapm_widgets),
	}
};

static const struct regmap_config tas5805_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = TAS5805_REGISTER_COUNT,
	.reg_defaults = tas5805_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(tas5805_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int tas5805_parse_dt(
	struct tas5805_priv *tas5805,
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
				tas5805->pdata->reset_pin);
	}
	tas5805->pdata->reset_pin = reset_pin;

	//TODO

	return ret;
}

static int tas5805_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct tas5805_priv *tas5805;
	struct tas58xx_platform_data *pdata;
	int ret;
	const char *codec_name;

	tas5805 = devm_kzalloc(&i2c->dev, sizeof(struct tas5805_priv),
			       GFP_KERNEL);
	if (!tas5805)
		return -ENOMEM;

	tas5805->regmap = devm_regmap_init_i2c(i2c, &tas5805_regmap);
	if (IS_ERR(tas5805->regmap)) {
		ret = PTR_ERR(tas5805->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct tas58xx_platform_data),
				GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for tas5805 pdata\n", __func__);
		return -ENOMEM;
	}
	tas5805->pdata = pdata;

	tas5805_parse_dt(tas5805, i2c->dev.of_node);

	if (of_property_read_string(i2c->dev.of_node,
			"codec_name",
				&codec_name)) {
		pr_info("no codec name\n");
		ret = -1;
	}
	pr_info("aux name = %s\n", codec_name);
	if (codec_name)
		dev_set_name(&i2c->dev, "%s", codec_name);

	i2c_set_clientdata(i2c, tas5805);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_tas5805,
				     &tas5805_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int tas5805_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static const struct i2c_device_id tas5805_i2c_id[] = {
	{ "tas5805", 0 },
	{}
};

static const struct of_device_id tas5805_of_id[] = {
	{.compatible = "ti,tas5805",},
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, tas5805_of_id);

static struct i2c_driver tas5805_i2c_driver = {
	.driver = {
		.name = DEV_NAME,
		.of_match_table = tas5805_of_id,
		.owner = THIS_MODULE,
	},
	.probe = tas5805_i2c_probe,
	.remove = tas5805_i2c_remove,
	.id_table = tas5805_i2c_id,
};
module_i2c_driver(tas5805_i2c_driver);

module_param_array(tas5805_EQ_table,
		uint, &tas5805_EQ_table_length, 0664);
MODULE_PARM_DESC(tas5805_EQ_table,
		"An array of tas5805 EQ param");

module_param_array(tas5805_drc1_table,
		uint, &tas5805_drc1_table_length, 0664);
MODULE_PARM_DESC(tas5805_drc1_table,
		"An array of tas5805 DRC table param");

module_param_array(tas5805_drc1_tko_table,
		uint, &tas5805_drc1_tko_length, 0664);
MODULE_PARM_DESC(tas5805_drc1_tko_table,
		"An array of tas5805 DRC tko table param");

MODULE_DESCRIPTION("ASoC Tas5805 driver");
MODULE_AUTHOR("CHANGHONG AI team");
MODULE_LICENSE("GPL");
