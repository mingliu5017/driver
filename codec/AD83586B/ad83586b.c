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
//#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/gpio.h>

#include "ad83586b.h"

//#define	AD83586B_REG_RAM_CHECK

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void ad83586b_early_suspend(struct early_suspend *h);
static void ad83586b_late_resume(struct early_suspend *h);
#endif

#define AD83586B_RATES (SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000 | \
		       SNDRV_PCM_RATE_64000 | \
		       SNDRV_PCM_RATE_88200 | \
		       SNDRV_PCM_RATE_96000 | \
		       SNDRV_PCM_RATE_176400 | \
		       SNDRV_PCM_RATE_192000)

#define AD83586B_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

static const struct snd_kcontrol_new ad83586b_snd_controls[] = {
	SOC_SINGLE_TLV("Digital Volume", MVOL, 0,//"Master Volume"
				0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", C1VOL, 0,
				0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", C2VOL, 0,
			0xff, 1, chvol_tlv),
};

//static struct snd_soc_codec *ad83586b_codec;

static int m_reg_tab[AD83586B_REGISTER_COUNT][2] = {
	{0x00, 0x00},//##State_Control_1
	{0x01, 0x04},//##State_Control_2
	{0x02, 0x00},//##State_Control_3
	{0x03, 0x18},//##Master_volume_control
	{0x04, 0x10},//##Channel_1_volume_control
	{0x05, 0x10},//##Channel_2_volume_control
	{0x06, 0x14},//##Channel_3_volume_control
	{0x07, 0x10},//##Bass_tone_boost_and_cut
	{0x08, 0x10},//##Treble_tone_boost_and_cut
	{0x09, 0x02},//##Bass_management_crossover_frequency
	{0x0a, 0x9c},//##State_Control_4
	{0x0b, 0x0a},//##Channel_1_configuration_registers
	{0x0c, 0x0a},//##Channel_2_configuration_registers
	{0x0d, 0x0e},//##Channel_3_configuration_registers
	{0x0e, 0x6a},//##DRC_limiter_attack/release_rate
	{0x11, 0x22},//##State_Control_5
	{0x12, 0x81},//##PVDD_under_voltage_selection
	{0x13, 0x00},//##Zero_detection_level_selection
	{0x14, 0x7e},//##Coefficient_RAM_base_address
	{0x15, 0x0f},//##Top_8-bits_of_coefficients_A1
	{0x16, 0xca},//##Middle_8-bits_of_coefficients_A1
	{0x17, 0xbb},//##Bottom_8-bits_of_coefficients_A1
	{0x18, 0x00},//##Top_8-bits_of_coefficients_A2
	{0x19, 0x00},//##Middle_8-bits_of_coefficients_A2
	{0x1a, 0x00},//##Bottom_8-bits_of_coefficients_A2
	{0x1b, 0x00},//##Top_8-bits_of_coefficients_B1
	{0x1c, 0x00},//##Middle_8-bits_of_coefficients_B1
	{0x1d, 0x00},//##Bottom_8-bits_of_coefficients_B1
	{0x1e, 0x00},//##Top_8-bits_of_coefficients_B2
	{0x1f, 0x00},//##Middle_8-bits_of_coefficients_B2
	{0x20, 0x00},//##Bottom_8-bits_of_coefficients_B2
	{0x21, 0x20},//##Top_8-bits_of_coefficients_A0
	{0x22, 0x00},//##Middle_8-bits_of_coefficients_A0
	{0x23, 0x00},//##Bottom_8-bits_of_coefficients_A0
	{0x24, 0x00},//##CfRW
	{0x2a, 0x6d},//##Power_saving_mode_switching_level
	{0x2b, 0x3f},//##Volume_fine_tune

};

static int m_ram_tab[][4] = {
	{0x00, 0xdd, 0xa0, 0x8a},//##Channel_1_EQ1_A1 
	{0x01, 0x0d, 0x86, 0x51},//##Channel_1_EQ1_A2 
	{0x02, 0x31, 0x22, 0x40},//##Channel_1_EQ1_B1 
	{0x03, 0xec, 0x0c, 0xee},//##Channel_1_EQ1_B2 
	{0x04, 0x17, 0xa9, 0xf6},//##Channel_1_EQ1_A0 
	{0x05, 0xc6, 0xd5, 0x21},//##Channel_1_EQ2_A1 
	{0x06, 0x1c, 0xbe, 0x8d},//##Channel_1_EQ2_A2 
	{0x07, 0x39, 0x2a, 0xdf},//##Channel_1_EQ2_B1 
	{0x08, 0xe3, 0xa0, 0xfb},//##Channel_1_EQ2_B2 
	{0x09, 0x1f, 0xa0, 0x77},//##Channel_1_EQ2_A0 
	{0x0a, 0xc2, 0xcd, 0xd4},//##Channel_1_EQ3_A1 
	{0x0b, 0x1e, 0x4a, 0xaa},//##Channel_1_EQ3_A2 
	{0x0c, 0x3d, 0x32, 0x2c},//##Channel_1_EQ3_B1 
	{0x0d, 0xe2, 0x46, 0xa7},//##Channel_1_EQ3_B2 
	{0x0e, 0x1f, 0x6e, 0xae},//##Channel_1_EQ3_A0 
	{0x0f, 0xc4, 0xd5, 0xa9},//##Channel_1_EQ4_A1 
	{0x10, 0x1d, 0x8f, 0xf4},//##Channel_1_EQ4_A2 
	{0x11, 0x3b, 0x2a, 0x57},//##Channel_1_EQ4_B1 
	{0x12, 0xe3, 0x3f, 0x67},//##Channel_1_EQ4_B2 
	{0x13, 0x1f, 0x30, 0xa4},//##Channel_1_EQ4_A0 
	{0x14, 0xc3, 0x34, 0xe4},//##Channel_1_EQ5_A1 
	{0x15, 0x1e, 0x6c, 0xe9},//##Channel_1_EQ5_A2 
	{0x16, 0x3c, 0xcb, 0x1c},//##Channel_1_EQ5_B1 
	{0x17, 0xe2, 0x04, 0x02},//##Channel_1_EQ5_B2 
	{0x18, 0x1f, 0x8f, 0x15},//##Channel_1_EQ5_A0 
	{0x19, 0xc1, 0x1b, 0x8c},//##Channel_1_EQ6_A1 
	{0x1a, 0x1f, 0x32, 0x31},//##Channel_1_EQ6_A2 
	{0x1b, 0x3e, 0xe4, 0x74},//##Channel_1_EQ6_B1 
	{0x1c, 0xe0, 0xfc, 0x61},//##Channel_1_EQ6_B2 
	{0x1d, 0x1f, 0xd1, 0x6e},//##Channel_1_EQ6_A0 
	{0x1e, 0xc0, 0x71, 0x1f},//##Channel_1_EQ7_A1 
	{0x1f, 0x1f, 0xa4, 0xe4},//##Channel_1_EQ7_A2 
	{0x20, 0x3f, 0x8e, 0xe1},//##Channel_1_EQ7_B1 
	{0x21, 0xe0, 0x6f, 0xba},//##Channel_1_EQ7_B2 
	{0x22, 0x1f, 0xeb, 0x62},//##Channel_1_EQ7_A0 
	{0x23, 0xd6, 0xa0, 0x33},//##Channel_1_EQ8_A1 
	{0x24, 0x12, 0xc6, 0xe7},//##Channel_1_EQ8_A2 
	{0x25, 0x3a, 0x87, 0xbe},//##Channel_1_EQ8_B1 
	{0x26, 0xe5, 0x0b, 0x8d},//##Channel_1_EQ8_B2 
	{0x27, 0x17, 0x05, 0x9b},//##Channel_1_EQ8_A0 
	{0x28, 0x00, 0x00, 0x00},//##Channel_3_EQ1_A1 
	{0x29, 0x00, 0x00, 0x00},//##Channel_3_EQ1_A2 
	{0x2a, 0x00, 0x00, 0x00},//##Channel_3_EQ1_B1 
	{0x2b, 0x00, 0x00, 0x00},//##Channel_3_EQ1_B2 
	{0x2c, 0x20, 0x00, 0x00},//##Channel_3_EQ1_A0 
	{0x2d, 0x00, 0x00, 0x00},//##Channel_3_EQ3_A1 
	{0x2e, 0x00, 0x00, 0x00},//##Channel_3_EQ3_A2 
	{0x2f, 0x00, 0x00, 0x00},//##Channel_3_EQ3_B1 
	{0x30, 0x00, 0x00, 0x00},//##Channel_3_EQ3_B2 
	{0x31, 0x20, 0x00, 0x00},//##Channel_3_EQ3_A0 
	{0x32, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A1 
	{0x33, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A2 
	{0x34, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B1 
	{0x35, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B2 
	{0x36, 0x20, 0x00, 0x00},//##Channel_2_EQ1_A0 
	{0x37, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A1 
	{0x38, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A2 
	{0x39, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B1 
	{0x3a, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B2 
	{0x3b, 0x20, 0x00, 0x00},//##Channel_2_EQ2_A0 
	{0x3c, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A1 
	{0x3d, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A2 
	{0x3e, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B1 
	{0x3f, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B2 
	{0x40, 0x20, 0x00, 0x00},//##Channel_2_EQ3_A0 
	{0x41, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A1 
	{0x42, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A2 
	{0x43, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B1 
	{0x44, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B2 
	{0x45, 0x20, 0x00, 0x00},//##Channel_2_EQ4_A0 
	{0x46, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A1 
	{0x47, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A2 
	{0x48, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B1 
	{0x49, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B2 
	{0x4a, 0x20, 0x00, 0x00},//##Channel_2_EQ5_A0 
	{0x4b, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A1 
	{0x4c, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A2 
	{0x4d, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B1 
	{0x4e, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B2 
	{0x4f, 0x20, 0x00, 0x00},//##Channel_2_EQ6_A0 
	{0x50, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A1 
	{0x51, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A2 
	{0x52, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B1 
	{0x53, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B2 
	{0x54, 0x20, 0x00, 0x00},//##Channel_2_EQ7_A0 
	{0x55, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A1 
	{0x56, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A2 
	{0x57, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B1 
	{0x58, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B2 
	{0x59, 0x20, 0x00, 0x00},//##Channel_2_EQ8_A0 
	{0x5a, 0x00, 0x00, 0x00},//##Channel_3_EQ2_A1 
	{0x5b, 0x00, 0x00, 0x00},//##Channel_3_EQ2_A2 
	{0x5c, 0x00, 0x00, 0x00},//##Channel_3_EQ2_B1 
	{0x5d, 0x00, 0x00, 0x00},//##Channel_3_EQ2_B2 
	{0x5e, 0x20, 0x00, 0x00},//##Channel_3_EQ2_A0 
	{0x5f, 0x00, 0x00, 0x00},//##Channel_3_EQ4_A1 
	{0x60, 0x00, 0x00, 0x00},//##Channel_3_EQ4_A2 
	{0x61, 0x00, 0x00, 0x00},//##Channel_3_EQ4_B1 
	{0x62, 0x00, 0x00, 0x00},//##Channel_3_EQ4_B2 
	{0x63, 0x20, 0x00, 0x00},//##Channel_3_EQ4_A0 
	{0x64, 0x7f, 0xff, 0xff},//##Channel-1_Mixer1
	{0x65, 0x00, 0x00, 0x00},//##Channel-1_Mixer2
	{0x66, 0x00, 0x00, 0x00},//##Channel-2_Mixer1
	{0x67, 0x7f, 0xff, 0xff},//##Channel-2_Mixer2
	{0x68, 0x40, 0x00, 0x00},//##Channel-3_Mixer1
	{0x69, 0x40, 0x00, 0x00},//##Channel-3_Mixer2
	{0x6a, 0x7f, 0xff, 0xff},//##Channel-1_Prescale
	{0x6b, 0x7f, 0xff, 0xff},//##Channel-2_Prescale
	{0x6c, 0x7f, 0xff, 0xff},//##Channel-1_Postscale
	{0x6d, 0x7f, 0xff, 0xff},//##Channel-2_Postscale
	{0x6e, 0x7f, 0xff, 0xff},//##Channel-3_Postscale
	{0x6f, 0x20, 0x00, 0x00},//##Channel1.2_Power_Clipping
	{0x70, 0x20, 0x00, 0x00},//##Channel-3_Power_Clipping
	{0x71, 0x0c, 0x60, 0xc3},//##CH1.2_DRC_Attack_threshold
	{0x72, 0x0a, 0xb8, 0x09},//##CH1.2_DRC_Release_threshold
	{0x73, 0x20, 0x00, 0x00},//##CH3_DRC_Attack_threshold
	{0x74, 0x08, 0x00, 0x00},//##CH3_DRC_Release_threshold
	{0x75, 0x00, 0x00, 0x1a},//##NGAL
	{0x76, 0x00, 0x00, 0x53},//##NGRL
	{0x77, 0x00, 0x80, 0x00},//##CH1.2_DRC_EC
	{0x78, 0x00, 0x20, 0x00},//##CH3_DRC_EC
	{0x79, 0xc7, 0xb6, 0x91},//##SRS_HPFA0
	{0x7a, 0x38, 0x49, 0x6e},//##SRS_HPFA1
	{0x7b, 0x0c, 0x46, 0xf8},//##SRS_HPFB1
	{0x7c, 0x0e, 0x81, 0xb9},//##SRS_LPFA0
	{0x7d, 0xf2, 0x2c, 0x12},//##SRS_LPFA1
	{0x7e, 0x0f, 0xca, 0xbb},//##SRS_LPFB1

};


/* codec private data */
struct ad83586b_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct ad83586b_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

   struct i2c_client *control_data;
};

static int ad83586b_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ad83586b_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return 0;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return 0;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return 0;
	}

	return 0;
}

static int ad83586b_hw_params(struct snd_pcm_substream *substream,
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
		
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		pr_debug("32bit\n");
		
		break;      
		
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

static int ad83586b_set_bias_level(struct snd_soc_codec *codec,
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
 //      codec->dapm.bias_level = level;
	//codec->component.dapm.bias_level = level;

	return 0;
}

static int ad83586b_set_eq_drc(struct snd_soc_codec *codec)
{
	u8 i;
	//  ram
	for (i = 0; i < AD83586B_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x01);
	}

	return 0;
}
static int ad83586b_reg_init(struct snd_soc_codec *codec)
{
	int i = 0;
	for (i = 0; i < AD83586B_REGISTER_COUNT; i++) {
              //printk("ad83586b_reg_init  write 0x%x = 0x%x\n", m_reg_tab[i][0], m_reg_tab[i][1]);
		snd_soc_write(codec, m_reg_tab[i][0], m_reg_tab[i][1]);
	};
       
	return 0;

}
#ifdef	AD83586B_REG_RAM_CHECK
static int ad83586b_reg_check(struct snd_soc_codec *codec)
{
	int i = 0;
	int reg_data = 0;
	
	for (i = 0; i < AD83586B_REGISTER_COUNT; i++) {
		reg_data = snd_soc_read(codec, m_reg_tab[i][0]);
		printk("ad83586b_reg_init  write 0x%x = 0x%x\n", m_reg_tab[i][0], reg_data);
	};
	return 0;
}

static int ad83586b_eqram_check(struct snd_soc_codec *codec)
{
	int i = 0;
	int H8_data = 0, M8_data = 0, L8_data = 0;
	
	for (i = 0; i < AD83586B_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);			// write ram addr
		snd_soc_write(codec, CFUD, 0x04);										// write read ram cmd
		
		H8_data = snd_soc_read(codec, A1CF1);
		M8_data = snd_soc_read(codec, A1CF2);
		L8_data = snd_soc_read(codec, A1CF3);
		printk("ad83586b_set_eq_drc ram1  write 0x%x = 0x%x , 0x%x , 0x%x\n", m_ram_tab[i][0], H8_data,M8_data,L8_data);
	};
	return 0;
}
#endif
static int reset_ad83586b_GPIO2(struct snd_soc_codec *codec) //reset
{
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
	struct ad83586b_platform_data *pdata = ad83586b->pdata;
//	int ret = 0;

	if (pdata->reset_pin2 < 0)
		return 0;

	//gpio_direction_output(pdata->reset_pin2, GPIOF_OUT_INIT_LOW);
	//udelay(1000);
	gpio_direction_output(pdata->reset_pin2, GPIOF_OUT_INIT_HIGH);

	return 0;
}

static int reset_ad83586b_GPIO(struct snd_soc_codec *codec)
{
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
	struct ad83586b_platform_data *pdata = ad83586b->pdata;
//	int ret = 0;

	if (pdata->reset_pin < 0)
		return 0;



	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	mdelay(20);

	return 0;
}
static int ad83586b_power_up(struct snd_soc_codec *codec, int powerup)
{
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
	struct ad83586b_platform_data *pdata = ad83586b->pdata;

	if (pdata->power_down_pin < 0)
		return 0;
	
	if(powerup)
		gpio_direction_output(pdata->power_down_pin, GPIOF_OUT_INIT_HIGH);
	else
		gpio_direction_output(pdata->power_down_pin, GPIOF_OUT_INIT_LOW);
	return 0;
}

static int ad83586b_init(struct snd_soc_codec *codec)
{
  
//	int ret;
	
	printk("%s\n", __func__);
	
  // Hardware reset AMP if reset pin have controlled by GPIO
  /*
  ret=gpio_request(123,NULL);
	printk(" gpio 123 ret=%d\n",ret);
  gpio_direction_output(123, 0);
	printk(" reset gpio output 0\n");
	mdelay(1);
	gpio_direction_output(123, 1);
	printk(" reset gpio output 1\n");
	mdelay(10);
	*/
	ad83586b_power_up(codec,1);
  	mdelay(200);
	reset_ad83586b_GPIO2(codec);
	mdelay(10);
	reset_ad83586b_GPIO(codec);
	// pull high PD pin if PD pin have controlled by GPIO
	/*
	ret= gpio_request(122,NULL);
	printk(" gpio 122 ret=%d\n",ret);	
	gpio_direction_output(122, 1);
	printk(" pd gpio output 1\n");
	*/
	
	//mdelay(20);

  printk("%s---------------%d\n", __func__,__LINE__);
  
  // software reset amp , if have not gpio controlled reset pin
	//snd_soc_write(codec, 0x11, 0x02);//--reset amp
	//mdelay(1);
	//snd_soc_write(codec, 0x11, 0x22);//--Normal operation
	//mdelay(20);
	
	// init AMP 
	dev_info(codec->dev, "ad83586b_init!\n");

 	snd_soc_write(codec, 0x02, 0x08);//--mute amp

	// write amp register
	ad83586b_reg_init(codec);

	snd_soc_write(codec, 0x02, 0x08);//--mute amp
  udelay(100);
	// write amp ram (eq and drc ... ) 
	ad83586b_set_eq_drc(codec);
	udelay(100);
	
  printk("%s\n", __func__);
  
#ifdef	AD83586B_REG_RAM_CHECK
	ad83586b_reg_check(codec);
	ad83586b_eqram_check(codec);
#endif	
	
	snd_soc_write(codec, 0x02, 0x00);//--unmute amp

	return 0; 
}

static void ad83586b_power_on(struct snd_soc_codec *codec, struct ad83586b_priv *ad83586b)
{
	if(ad83586b->pdata->reset_pin > 0)
		gpio_direction_output(ad83586b->pdata->reset_pin, GPIOF_OUT_INIT_HIGH);

	//snd_soc_write(codec, 0x02, 0x7f); // mute
	//ad82584f_reg_init(codec);
	//ad82584f_set_eq_drc(codec); //eq and drc
	
	//snd_soc_write(codec, MVOL, ad82584f->last_volume);

	mdelay(20);
	snd_soc_write(codec, 0x02, 0x00);
}


static void ad83586b_power_off(struct snd_soc_codec *codec, struct ad83586b_priv *ad83586b)
{
	//unsigned int val;

	//val = snd_soc_read(codec, MVOL);
	//if ( val > 0 ) {
	//	ad82584f->last_volume = val;
	//}
	snd_soc_write(codec, 0x02, 0x0f);		// mute
	//gpio_direction_output(ad83586b->pdata->power_down_pin, GPIOF_OUT_INIT_LOW);
	mdelay(20);
	if(ad83586b->pdata->reset_pin > 0)
		gpio_direction_output(ad83586b->pdata->reset_pin, GPIOF_OUT_INIT_LOW);
}

static int ad83586b_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	//int ret;
	struct snd_soc_codec *codec = dai->codec;
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
	printk("%s---------------%d\n", __func__,__LINE__);
	/*
	ret=gpio_request(122,NULL);				// request amp PD pin control GPIO
	printk(" gpio 122 ret=%d\n",ret);
	gpio_direction_output(122, 1);		// pull high amp PD pin
	mdelay(20);
	//ad83586b_init(codec);							// init amp again
	snd_soc_write(codec, 0x02, 0x00);   //--unmute amp
	*/
	ad83586b_power_on(codec, ad83586b);
  return 0;
}

static int ad83586b_prepare(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
	//struct snd_soc_codec *codec = dai->codec;
	
	pr_info("fun:%s\n", __func__);

	//ad83586b_init(codec);

	return 0;
}


static void ad83586b_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	//int ret;
	struct snd_soc_codec *codec = dai->codec;
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
	printk("%s---------------%d\n", __func__,__LINE__);
	
	ad83586b_power_off(codec, ad83586b);
	/*
	snd_soc_write(codec, 0x02, 0x0f);					//--mute amp
	ret=gpio_request(122,NULL);								// request amp PD pin control GPIO
	printk(" gpio 122 ret=%d\n",ret);
	gpio_direction_output(122, 0);						// pull low amp PD pin
	*/
	return;
}

static const struct snd_soc_dai_ops ad83586b_dai_ops = {
	.startup = ad83586b_startup,
	.shutdown = ad83586b_shutdown,
	.hw_params = ad83586b_hw_params,
	.set_sysclk = ad83586b_set_dai_sysclk,
	.set_fmt = ad83586b_set_dai_fmt,
	.prepare = ad83586b_prepare,
};

static struct snd_soc_dai_driver ad83586b_dai = {
	.name = "ad83586b",   
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 1,
		.channels_max = 8,
		.rates = AD83586B_RATES,
		.formats = AD83586B_FORMATS,
	},
	/*.capture = {
		.stream_name ="HIFI Capture",
		.channels_min = 1, 
		.channels_max = 8,
		.rates = AD83586B_RATES,
		.formats = AD83586B_FORMATS,
	},*/
	.ops = &ad83586b_dai_ops,
};


static int ad83586b_probe(struct snd_soc_codec *codec)
{
   int ret = 0;
   
  struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);
  struct ad83586b_platform_data *pdata = ad83586b->pdata;
  printk("%s\n", __func__);

	if (codec == NULL) {
	      dev_err(codec->dev, "Codec device not registered\n");
	      return -ENODEV;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
//	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);

	ad83586b->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ad83586b->early_suspend.suspend = ad83586b_early_suspend;
	ad83586b->early_suspend.resume = ad83586b_late_resume;
	//ad83586b->early_suspend.param = codec;
	register_early_suspend(&(ad83586b->early_suspend));
#endif

	//ad83586b_codec = codec;
	
	//ad83586b->codec = codec;
	codec->control_data = ad83586b->control_data;


	if (pdata->power_down_pin > 0){
		ret = devm_gpio_request_one(codec->dev, pdata->power_down_pin,
					    GPIOF_OUT_INIT_HIGH,
					    "ad82584f-power-down-pin");
		if (ret < 0){
			dev_err(codec->dev, "request power down pin fail!\n");
			pdata->power_down_pin = -1;
		}
	}
	if (pdata->reset_pin2 > 0){
		ret = devm_gpio_request_one(codec->dev, pdata->reset_pin2,
					    GPIOF_OUT_INIT_HIGH,
					    "ad82584f-reset-pin2");
		if (ret < 0){
			dev_err(codec->dev, "request ad82584f-reset-pin2 fail!\n");
			pdata->reset_pin2  = -1;
		}
	}
	if (pdata->reset_pin > 0){
		ret = devm_gpio_request_one(codec->dev, pdata->reset_pin,
					    GPIOF_OUT_INIT_HIGH,
					    "ad82584f-reset-pin");
		if (ret < 0){
			dev_err(codec->dev, "request ad82584f-reset-pin fail!\n");
			pdata->reset_pin2  = -1;
		}
	}

	ad83586b_init(codec);

	return 0;
}

static int ad83586b_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(ad83586b->early_suspend));
#endif
	return 0;
}

static int ad83586b_suspend(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "ad83586b_suspend!\n");
	
	snd_soc_write(codec, 0x02, 0x0f);		// mute
	
	return 0;
}

static int ad83586b_resume(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "ad83586b_resume!\n");

	// init ad83586b again
	//ad83586b_init(codec);
	
	snd_soc_write(codec, 0x02, 0x00);			// unmute
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ad83586b_early_suspend(struct early_suspend *h)
{
}

static void ad83586b_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget ad83586b_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};



/*
 * I2C Read/Write Functions
 */
static int ad83586b_i2c_read(struct i2c_client *ad83586b_client,
                u8 reg, u8 *value, int len)
{
    int err;
    int tries = 0;

    struct i2c_msg msgs[] = {
        {
         .addr = ad83586b_client->addr,
         .flags = 0,
         .len = 1,
         .buf = &reg,
         },
        {
         .addr = ad83586b_client->addr,
         .flags = I2C_M_RD,
         .len = len,
         .buf = value,
         },
    };

    do {
        err = i2c_transfer(ad83586b_client->adapter, msgs,
                            ARRAY_SIZE(msgs));
        if (err != ARRAY_SIZE(msgs))
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

    if (err != ARRAY_SIZE(msgs)) {
        dev_err(&ad83586b_client->dev, "read transfer error %d\n"
                , err);
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}
static unsigned int ad83586b_read(struct snd_soc_codec *codec, unsigned int reg)
{
   // struct ad83586b_priv *ad83586b  = snd_soc_codec_get_drvdata(codec);
    u8 data;
    int len = 1;
    int val = -EIO;

    if (ad83586b_i2c_read(codec->control_data, reg & 0xff, &data, len) == 0) {
        val = data;		//(buf[0] << 8 | buf[1]);
    }

    return val;
}
static int ad83586b_i2c_write(struct i2c_client *ad83586b_client,
                u8 *value, u8 len)
{
    int err;
    int tries = 0;
    struct i2c_msg msgs[] = {
        {
         .addr = ad83586b_client->addr,
         .flags = 0,
         .len = len,
         .buf = value,
         },
    };

    do {
        err = i2c_transfer(ad83586b_client->adapter, msgs,
                    ARRAY_SIZE(msgs));
        if (err != ARRAY_SIZE(msgs))
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

    if (err != ARRAY_SIZE(msgs)) {
        dev_err(&ad83586b_client->dev, "write transfer error\n");
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}
static int ad83586b_write(struct snd_soc_codec *codec, unsigned int reg,
                unsigned int val)
{
    u8 buf[2] = {0, 0};
    int ret;
    buf[0] = (reg & 0xff);
    buf[1] = (val & 0xff);

    ret = ad83586b_i2c_write(codec->control_data, buf, ARRAY_SIZE(buf));

    return ret;
}



static const struct snd_soc_codec_driver soc_codec_dev_ad83586b = {
	.probe = ad83586b_probe,
	.remove = ad83586b_remove,
	.suspend = ad83586b_suspend,
	.resume = ad83586b_resume,
	.read =    ad83586b_read,
  .write =   ad83586b_write,
	.set_bias_level = ad83586b_set_bias_level,
       .reg_cache_size = ARRAY_SIZE(m_reg_tab),
       .reg_word_size = sizeof(u16),
       .reg_cache_default = m_reg_tab,
	.component_driver = {
		.controls = ad83586b_snd_controls,
		.num_controls = ARRAY_SIZE(ad83586b_snd_controls),
		.dapm_widgets = ad83586b_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ad83586b_dapm_widgets),
	}
};
static int ad83586b_parse_dt(
	struct ad83586b_priv *ad83586b,
	struct device_node *np)
{
	int ret = 0;
	int reset_pin = -1;
	int power_down_pin = -1;

	reset_pin = of_get_named_gpio(np, "reset_pin", 0);
	if (reset_pin < 0) {
		pr_err("%s fail to get reset pin from dts!\n", __func__);
		ret = -1;
	} else {
		pr_info("%s pdata->reset_pin = %d!\n", __func__,reset_pin);
	}
	ad83586b->pdata->reset_pin = reset_pin;
	
	reset_pin = -1;
	reset_pin = of_get_named_gpio(np, "reset_pin2", 0);
	if (reset_pin < 0) {
		pr_err("%s fail to get reset pin from dts!\n", __func__);
		ret = -1;
	} else {
		pr_info("%s pdata->reset_pin = %d!\n", __func__,reset_pin);
	}
	ad83586b->pdata->reset_pin2 = reset_pin;


	power_down_pin = of_get_named_gpio(np, "power_down_pin", 0);
	if (power_down_pin < 0) {
		pr_err("%s fail to get power down pin from dts!\n", __func__);
		ret = -1;
	} else {
		pr_info("%s pdata->power_down_pin = %d!\n", __func__,power_down_pin);
	}
	ad83586b->pdata->power_down_pin = power_down_pin;

	return ret;
}

static int ad83586b_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct ad83586b_priv *ad83586b;
	struct ad83586b_platform_data *pdata;
	int ret;
  printk("%s---------------%d\n", __func__,__LINE__);
	ad83586b = devm_kzalloc(&i2c->dev, sizeof(struct ad83586b_priv),
			       GFP_KERNEL);
	if (!ad83586b)
		return -ENOMEM;
    /*          
	ad83586b->regmap = devm_regmap_init_i2c(i2c, &ad83586b_regmap);
	if (IS_ERR(ad83586b->regmap)) {
		ret = PTR_ERR(ad83586b->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}*/
	i2c_set_clientdata(i2c, ad83586b);

	pdata = devm_kzalloc(&i2c->dev, sizeof(struct ad83586b_platform_data), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for ad83586b pdata\n", __func__);
		return -ENOMEM;
	}
	ad83586b->pdata = pdata;
	
	ad83586b->control_data = i2c;

	ad83586b_parse_dt(ad83586b, i2c->dev.of_node);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_ad83586b, &ad83586b_dai, 1);
	if (ret != 0)
       {
       	printk("Failed to register codec (%d)\n", ret);
       }

	printk("%s---------------%d probe end\n", __func__,__LINE__);
	
	 return 0;
}

static int ad83586b_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}
/*
void ad83586b_i2c_shutdown(struct i2c_client *client)
{
	//struct snd_soc_codec *codec;
	//codec = ad83586b_codec;
	struct ad83586b_priv *ad83586b = snd_soc_codec_get_drvdata(ad83586b_codec);
	struct ad83586b_platform_data *pdata = ad83586b->pdata;

	gpio_direction_output(pdata->reset_pin, 0);

	//return 0;
}*/

static const struct i2c_device_id ad83586b_i2c_id[] = {
	{ "ad83586b", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, ad83586b_i2c_id);

static const struct of_device_id ad83586b_of_id[] = {
	{ .compatible = "ESMT, ad83586b"},
	{ },
};
//MODULE_DEVICE_TABLE(of, ad83586b_of_id);

static struct i2c_driver ad83586b_i2c_driver = {
	.driver = {
		.name = "ad83586b",
		.of_match_table = ad83586b_of_id,
		.owner = THIS_MODULE,
	},
	//.shutdown = ad83586b_i2c_shutdown,
	.probe = ad83586b_i2c_probe,
	.remove = ad83586b_i2c_remove,
	.id_table = ad83586b_i2c_id,
};

//module_i2c_driver(ad83586b_i2c_driver);

static int __init ad83586b_i2c_init(void)
{
	printk("%s\n", __func__);
	return i2c_add_driver(&ad83586b_i2c_driver);
}

static void __exit ad83586b_i2c_exit(void)
{
	i2c_del_driver(&ad83586b_i2c_driver);
}

module_init(ad83586b_i2c_init);
module_exit(ad83586b_i2c_exit);

MODULE_DESCRIPTION("ASoC ad83586b driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");
