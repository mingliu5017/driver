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
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <linux/regmap.h>
#include <sound/tas57xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "tas5805.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
static void tas5805_early_suspend(struct early_suspend *h);
static void tas5805_late_resume(struct early_suspend *h);
#endif

//int first_open = 0;
static int tas5805_init(struct snd_soc_codec *codec);
//spinlock_t tas5805_lock;
#define TAS5805_RATES (SNDRV_PCM_RATE_8000 | \
		       SNDRV_PCM_RATE_11025 | \
		       SNDRV_PCM_RATE_16000 | \
		       SNDRV_PCM_RATE_22050 | \
		       SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000 | \
			   SNDRV_PCM_RATE_96000)

#define TAS5805_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

static const u8 tas5805_reset_regs[][2] = {
	{ 0x00, 0x00 },
	{ 0x7f, 0x00 },
	{ 0x03, 0x02 },
	{ 0x01, 0x11 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
};

static const u8 tas5805_charge_regs[][2] = {
	{ 0x00, 0x00 },
	{ 0x7f, 0x00 },
	{ 0x03, 0x02 },

	{ 0x00, 0x00 },
	{ 0x7f, 0x00 },
	{ 0x03, 0x00 },
};

/* Power-up register defaults */

/* codec private data */
struct tas5805_priv {
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
	unsigned mclk;
	int vol;
	int mute_state;
};

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 0);

static const char * const Low_Volume_Eq_texts[] = {"70Hz 11dB"};
static const char * const Mid_Volume_Eq_texts[] = {"70Hz 9dB"};
static const char * const High_Volume_Eq_texts[] = {"70Hz 7dB"};

static const struct soc_enum Low_Volume_Eq_enum = SOC_ENUM_SINGLE(
		SND_SOC_NOPM,0,ARRAY_SIZE(Low_Volume_Eq_texts),
		Low_Volume_Eq_texts);
static const struct soc_enum Mid_Volume_Eq_enum = SOC_ENUM_SINGLE(
		SND_SOC_NOPM,0,ARRAY_SIZE(Mid_Volume_Eq_texts),
		Mid_Volume_Eq_texts);
static const struct soc_enum High_Volume_Eq_enum = SOC_ENUM_SINGLE(
		SND_SOC_NOPM,0,ARRAY_SIZE(High_Volume_Eq_texts),
		High_Volume_Eq_texts);

static int get_low_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int get_mid_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
	return 0;

}

static int get_high_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
	return 0;

}

static int set_low_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
#if 0
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
    struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	int i;
	for (i = 0; i < sizeof(tas5805_low_volume_eq)/2 ; i++)
		snd_soc_write(codec, tas5805_low_volume_eq[i][0],tas5805_low_volume_eq[i][1]);
#endif
	return 0;

}

static int set_mid_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
#if 0
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
    struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	int i;
	for (i = 0; i < sizeof(tas5805_mid_volume_eq)/2 ; i++)
		snd_soc_write(codec, tas5805_mid_volume_eq[i][0],tas5805_mid_volume_eq[i][1]);

#endif
	return 0;

}

static int set_high_volume_eq(struct snd_kcontrol *kcontrol , struct snd_ctl_elem_value *ucontrol)
{
#if 0
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
    struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	int i;
	for (i = 0; i < sizeof(tas5805_high_volume_eq)/2 ; i++)
		snd_soc_write(codec, tas5805_high_volume_eq[i][0],tas5805_high_volume_eq[i][1]);
#endif
	return 0;
}

static int tas5805_get_mute(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5805->mute_state;

	return 0;
}

static int tas5805_set_mute(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	u8 reg03_value = 0,reg35_value = 0;

	tas5805->mute_state = ucontrol->value.integer.value[0];

	pr_info("tas5805 set: %s\n",(tas5805->mute_state==1)? "unmute":"mute");

	if (tas5805->mute_state){
		//unmute
		reg03_value = 0x03;
		reg35_value = 0x11; 
	}else{
		//mute both left & right channels
		reg03_value = 0x0b;
		reg35_value = 0x00;
	}

	snd_soc_write(codec, 0x00, 0x00);
	snd_soc_write(codec, 0x7f, 0x00);
	snd_soc_write(codec, 0x00, 0x00);	
	snd_soc_write(codec, 0x03, reg03_value);
	snd_soc_write(codec, 0x35, reg35_value);

	return 0;
}

static const struct snd_kcontrol_new tas5805_snd_controls[] = {
	/* The below volume gain should aligned to tas5760l */
	SOC_SINGLE_RANGE_TLV("Master", 0x4c, 0, 0x14, 0xfb, 1, mvol_tlv),
	SOC_ENUM_EXT("Low Volume Eq" , Low_Volume_Eq_enum , get_low_volume_eq , set_low_volume_eq),
	SOC_ENUM_EXT("Mid Volume Eq" , Mid_Volume_Eq_enum , get_mid_volume_eq , set_mid_volume_eq),
	SOC_ENUM_EXT("High Volume Eq" , High_Volume_Eq_enum , get_high_volume_eq,set_high_volume_eq),
	SOC_SINGLE_BOOL_EXT("SPK unmute", 0,tas5805_get_mute, tas5805_set_mute),
};

static int tas5805_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);

	tas5805->mclk = freq;
	return 0;
}

static int tas5805_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
static int tas5805_trigger(struct snd_pcm_substream * substream,int cmd,struct snd_soc_dai * dai)
{

	switch(cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
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

static const struct snd_soc_dai_ops tas5805_dai_ops = {
	.hw_params = tas5805_hw_params,
	.set_sysclk = tas5805_set_dai_sysclk,
	.set_fmt = tas5805_set_dai_fmt,
	.trigger = tas5805_trigger,
};

static struct snd_soc_dai_driver tas5805_dai = {
	.name = "tas5805",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = TAS5805_RATES,
		.formats = TAS5805_FORMATS,
	},
	.ops = &tas5805_dai_ops,
};

static int reset_tas5805_GPIO(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = tas5805->pdata;

	if (pdata->pdn_pin != 0) {
		int value;
		value = pdata->pdn_pin_active_low?
			GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH;
		gpio_direction_output(pdata->pdn_pin, value);

		mdelay(50);

		value = pdata->pdn_pin_active_low?
			GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW;
		gpio_direction_output(pdata->pdn_pin, value);

		mdelay(30);
	}

	return 0;
}

static int tas5805_init(struct snd_soc_codec *codec)
{
	int i;

	dev_info(codec->dev, "tas5805_init!\n");

	reset_tas5805_GPIO(codec);

	for (i = 0; i < sizeof(tas5805_reset_regs)/2 ; i++)
		snd_soc_write(codec, tas5805_reset_regs[i][0],
				tas5805_reset_regs[i][1]);

	mdelay(40);
#if 0
	for (i = 0; i < sizeof(tas5805_charge_regs)/2 ; i++)
		snd_soc_write(codec, tas5805_charge_regs[i][0],
				tas5805_charge_regs[i][1]);

	mdelay(40);
#endif
	for (i = 0; i < sizeof(tas5805_regs)/2 ; i++)
		snd_soc_write(codec, tas5805_regs[i][0], tas5805_regs[i][1]);

	/* clear OC error */
	mdelay(20);
	snd_soc_write(codec, 0x78, 0x80);

	return 0;
}

static int tas5805_probe(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	early_suspend.suspend = tas5805_early_suspend;
	early_suspend.resume = tas5805_late_resume;
	early_suspend.param = codec;
	register_early_suspend(&early_suspend);
#endif
	tas5805_init(codec);
	//spin_lock_init(&tas5805_lock);
	return 0;
}



static int tas5805_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int tas5805_suspend(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = tas5805->pdata;

	dev_info(codec->dev, "tas5805_suspend!\n");

	if (pdata && pdata->suspend_func)
		pdata->suspend_func();

	/*save volume */
	tas5805->Ch1_vol = snd_soc_read(codec, 0x4c);
	return 0;
}

static int tas5805_resume(struct snd_soc_codec *codec)
{
	struct tas5805_priv *tas5805 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = tas5805->pdata;

	dev_info(codec->dev, "tas5805_resume!\n");

	if (pdata && pdata->resume_func)
		pdata->resume_func();

	tas5805_init(codec);
	snd_soc_write(codec, 0x4c, tas5805->Ch1_vol);
	return 0;
}
#else
#define tas5805_suspend NULL
#define tas5805_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tas5805_early_suspend(struct early_suspend *h)
{
	return;
}

static void tas5805_late_resume(struct early_suspend *h)
{
	return;
}
#endif

static const struct snd_soc_dapm_widget tas5805_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver tas5805_codec = {
	.probe = tas5805_probe,
	.remove = tas5805_remove,
	.suspend = tas5805_suspend,
	.resume = tas5805_resume,
	.component_driver = {
		.controls = tas5805_snd_controls,
		.num_controls = ARRAY_SIZE(tas5805_snd_controls),
		.dapm_widgets = tas5805_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tas5805_dapm_widgets),
	}
};

static int tas5805_parse_dts(struct tas5805_priv *tas5805,
		struct device_node *np)
{
	int ret = 0;
	int pin;
	enum of_gpio_flags flags;

	pin = of_get_named_gpio_flags(np, "pdn_pin", 0, &flags);
	if (pin < 0) {
		tas5805->pdata->pdn_pin = 0;
		pr_err("%s fail to get pdn pin from dts!\n", __func__);
		ret = -1;
	} else {
		gpio_request(pin, "codec_pdn");
		tas5805->pdata->pdn_pin = pin;
		tas5805->pdata->pdn_pin_active_low = flags & OF_GPIO_ACTIVE_LOW;
		pr_info("%s pdata->pdn_pin = %d!\n", __func__,
				pin);
	}

	return ret;
}

static const struct regmap_config tas5805_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static int tas5805_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct tas5805_priv *tas5805;
	struct tas57xx_platform_data *pdata;
	int ret;

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
			sizeof(struct tas57xx_platform_data),
			GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for tas5805 pdata\n", __func__);
		return -ENOMEM;
	}
	tas5805->pdata = pdata;

	tas5805_parse_dts(tas5805, i2c->dev.of_node);

	i2c_set_clientdata(i2c, tas5805);

	ret = snd_soc_register_codec(&i2c->dev, &tas5805_codec,
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

static void tas5805_i2c_shutdown(struct i2c_client *client)
{
	struct tas5805_priv *tas5805 = i2c_get_clientdata(client);
	struct tas57xx_platform_data *pdata = tas5805->pdata;

	dev_info(&client->dev, "Enter into tas5805 shutdown:\n");

	if (pdata->pdn_pin != 0) {
		int value;
		value = pdata->pdn_pin_active_low?
			GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH;
		gpio_direction_output(pdata->pdn_pin, value);

		dev_info(&client->dev, "Exit from shutdown, success!\n");
	} else
		dev_info(&client->dev, "Exit from shutdown, nothing to do\n");

	return;
}

static const struct i2c_device_id tas5805_i2c_id[] = {
	{ "tas5805", 0 },
	{}
};

static const struct of_device_id tas5805_of_id[] = {
	{ .compatible = "TI, tas5805", },
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, tas5805_of_id);

static struct i2c_driver tas5805_i2c_driver = {
	.driver = {
		.name = "tas5805",
		.of_match_table = tas5805_of_id,
		.owner = THIS_MODULE,
	},
	.probe = tas5805_i2c_probe,
	.remove = tas5805_i2c_remove,
	.shutdown = tas5805_i2c_shutdown,
	.id_table = tas5805_i2c_id,
};

module_i2c_driver(tas5805_i2c_driver);

MODULE_DESCRIPTION("ASoC Tas5805 driver");
MODULE_AUTHOR("Xiaomi Audio team");
MODULE_LICENSE("GPL");

