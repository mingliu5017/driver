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
#include <linux/amlogic/aml_gpio_consumer.h>

#include "ad82584f.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void ad82584f_early_suspend(struct early_suspend *h);
static void ad82584f_late_resume(struct early_suspend *h);
#endif

/* codec private data */
struct ad82584f_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct ad82584f_platform_data *pdata;
	unsigned int power_enum_value;
	unsigned int last_volume;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static void ad82584_power_on(struct snd_soc_codec *codec, 
					struct ad82584f_priv *ad82584f);

static void ad82584_power_off(struct snd_soc_codec *codec, 
					struct ad82584f_priv *ad82584f);

static int ad82584f_get_power(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol);

static int ad82584f_set_power(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);

static int ad82584f_reg_init(struct snd_soc_codec *codec);

static int ad82584f_set_eq_drc(struct snd_soc_codec *codec);

#define AD82584F_RATES (SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000 | \
		       SNDRV_PCM_RATE_64000 | \
		       SNDRV_PCM_RATE_88200 | \
		       SNDRV_PCM_RATE_96000 | \
		       SNDRV_PCM_RATE_176400 | \
		       SNDRV_PCM_RATE_192000)

#define AD82584F_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 0);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

#define DEFAULT_VOL (0x0d)

static int master_vol;
int ad82584_snd_soc_put_volsw_range(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{

	master_vol = ucontrol->value.integer.value[0];
	return 0;
}

int ad82584_snd_soc_get_volsw_range(struct snd_kcontrol *kcontrol,
    struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = master_vol;
	return 0;
}

#define AD82584_SOC_SINGLE_RANGE_TLV(xname, xreg, xshift, xmin, xmax, xinvert, tlv_array) \
{   .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
    .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
         SNDRV_CTL_ELEM_ACCESS_READWRITE,\
    .tlv.p = (tlv_array), \
    .info = snd_soc_info_volsw_range, \
    .get = ad82584_snd_soc_get_volsw_range, .put = ad82584_snd_soc_put_volsw_range, \
    .private_value = (unsigned long)&(struct soc_mixer_control) \
        {.reg = xreg, .rreg = xreg, .shift = xshift, \
         .rshift = xshift, .min = xmin, .max = xmax, \
         .platform_max = xmax, .invert = xinvert} }


static const struct snd_kcontrol_new ad82584f_snd_controls[] = {
	AD82584_SOC_SINGLE_RANGE_TLV("Master Volume", MVOL, 0, 0,
				0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", C1VOL, 0,
				0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", C2VOL, 0,
			0xff, 1, chvol_tlv),
	SOC_SINGLE("Master Mute", 0x02, 6, 1, 0),
	SOC_SINGLE_BOOL_EXT("SPK unmute", 0,
		   ad82584f_get_power, ad82584f_set_power),
	SOC_SINGLE_RANGE_TLV("master vol", MVOL, 0, 0,
					0xff, 1, mvol_tlv),
};

/* Power-up register defaults */
struct reg_default ad82584f_reg_defaults[AD82584F_REGISTER_COUNT] = {
	{0x00, 0x04},//##State_Control_1
	{0x01, 0x81},//##State_Control_2
	{0x02, 0x7f},//##State_Control_3
	{0x03, 0x23},//##Master_volume_control
	{0x04, 0x1b},//##Channel_1_volume_control
	{0x05, 0x1b},//##Channel_2_volume_control
	{0x06, 0x18},//##Channel_3_volume_control
	{0x07, 0x18},//##Channel_4_volume_control
	{0x08, 0x18},//##Channel_5_volume_control
	{0x09, 0x18},//##Channel_6_volume_control
	{0x0a, 0x10},//##Bass_Tone_Boost_and_Cut
	{0x0b, 0x10},//##treble_Tone_Boost_and_Cut
	{0x0c, 0x98},//##State_Control_4
	{0x0d, 0x00},//##Channel_1_configuration_registers
	{0x0e, 0x00},//##Channel_2_configuration_registers
	{0x0f, 0x00},//##Channel_3_configuration_registers
	{0x10, 0x00},//##Channel_4_configuration_registers
	{0x11, 0x00},//##Channel_5_configuration_registers
	{0x12, 0x00},//##Channel_6_configuration_registers
	{0x13, 0x00},//##Channel_7_configuration_registers
	{0x14, 0x00},//##Channel_8_configuration_registers
	{0x15, 0x71},//##DRC1_limiter_attack/release_rate
	{0x16, 0x8a},//##DRC2_limiter_attack/release_rate
	{0x17, 0xbf},//##DRC3_limiter_attack/release_rate
	{0x18, 0x9a},//##DRC4_limiter_attack/release_rate
	{0x19, 0x06},//##Error_Delay
	{0x1a, 0x70},//##State_Control_5
	{0x1b, 0x41},//##HVUV_selection
	{0x1c, 0x00},//##State_Control_6
	{0x1d, 0x7f},//##Coefficient_RAM_Base_Address
	{0x1e, 0x00},//##Top_8-bits_of_coefficients_A1
	{0x1f, 0x00},//##Middle_8-bits_of_coefficients_A1
	{0x20, 0x00},//##Bottom_8-bits_of_coefficients_A1
	{0x21, 0x00},//##Top_8-bits_of_coefficients_A2
	{0x22, 0x00},//##Middle_8-bits_of_coefficients_A2
	{0x23, 0x00},//##Bottom_8-bits_of_coefficients_A2
	{0x24, 0x00},//##Top_8-bits_of_coefficients_B1
	{0x25, 0x00},//##Middle_8-bits_of_coefficients_B1
	{0x26, 0x00},//##Bottom_8-bits_of_coefficients_B1
	{0x27, 0x00},//##Top_8-bits_of_coefficients_B2
	{0x28, 0x00},//##Middle_8-bits_of_coefficients_B2
	{0x29, 0x00},//##Bottom_8-bits_of_coefficients_B2
	{0x2a, 0x20},//##Top_8-bits_of_coefficients_A0
	{0x2b, 0x00},//##Middle_8-bits_of_coefficients_A0
	{0x2c, 0x00},//##Bottom_8-bits_of_coefficients_A0
	{0x2d, 0x40},//##Coefficient_R/W_control
	{0x2e, 0x00},//##Protection_Enable/Disable
	{0x2f, 0x00},//##Memory_BIST_status
	{0x30, 0x00},//##Power_Stage_Status(Read_only)
	{0x31, 0x00},//##PWM_Output_Control
	{0x32, 0x00},//##Test_Mode_Control_Reg.
	{0x33, 0x6d},//##Qua-Ternary/Ternary_Switch_Level
	{0x34, 0x00},//##Volume_Fine_tune
	{0x35, 0x00},//##Volume_Fine_tune
	{0x36, 0x60},//##OC_bypass_&_GVDD_selection
	{0x37, 0x52},//##Device_ID_register
	{0x38, 0x00},//##RAM1_test_register_address
	{0x39, 0x00},//##Top_8-bits_of_RAM1_Data
	{0x3a, 0x00},//##Middle_8-bits_of_RAM1_Data
	{0x3b, 0x00},//##Bottom_8-bits_of_RAM1_Data
	{0x3c, 0x00},//##RAM1_test_r/w_control
	{0x3d, 0x00},//##RAM2_test_register_address
	{0x3e, 0x00},//##Top_8-bits_of_RAM2_Data
	{0x3f, 0x00},//##Middle_8-bits_of_RAM2_Data
	{0x40, 0x00},//##Bottom_8-bits_of_RAM2_Data
	{0x41, 0x00},//##RAM2_test_r/w_control
	{0x42, 0x00},//##Level_Meter_Clear
	{0x43, 0x00},//##Power_Meter_Clear
	{0x44, 0x47},//##TOP_of_C1_Level_Meter
	{0x45, 0x83},//##Middle_of_C1_Level_Meter
	{0x46, 0x82},//##Bottom_of_C1_Level_Meter
	{0x47, 0x32},//##TOP_of_C2_Level_Meter
	{0x48, 0xb7},//##Middle_of_C2_Level_Meter
	{0x49, 0x6d},//##Bottom_of_C2_Level_Meter
	{0x4a, 0x01},//##TOP_of_C3_Level_Meter
	{0x4b, 0x22},//##Middle_of_C3_Level_Meter
	{0x4c, 0xe0},//##Bottom_of_C3_Level_Meter
	{0x4d, 0x01},//##TOP_of_C4_Level_Meter
	{0x4e, 0x22},//##Middle_of_C4_Level_Meter
	{0x4f, 0xe0},//##Bottom_of_C4_Level_Meter
	{0x50, 0x0d},//##TOP_of_C5_Level_Meter
	{0x51, 0xce},//##Middle_of_C5_Level_Meter
	{0x52, 0x6a},//##Bottom_of_C5_Level_Meter
	{0x53, 0x0d},//##TOP_of_C6_Level_Meter
	{0x54, 0xce},//##Middle_of_C6_Level_Meter
	{0x55, 0x6a},//##Bottom_of_C6_Level_Meter
	{0x56, 0x12},//##TOP_of_C7_Level_Meter
	{0x57, 0xee},//##Middle_of_C7_Level_Meter
	{0x58, 0x6b},//##Bottom_of_C7_Level_Meter
	{0x59, 0x12},//##TOP_of_C8_Level_Meter
	{0x5a, 0xee},//##Middle_of_C8_Level_Meter
	{0x5b, 0x6b},//##Bottom_of_C8_Level_Meter
	{0x5c, 0x06},//##I2S_Data_Output_Selection_Register
	{0x5d, 0x00},//##Reserve
	{0x5e, 0x00},//##Reserve
	{0x5f, 0x00},//##Reserve
	{0x60, 0x00},//##Reserve
	{0x61, 0x00},//##Reserve
	{0x62, 0x00},//##Reserve
	{0x63, 0x00},//##Reserve
	{0x64, 0x00},//##Reserve
	{0x65, 0x00},//##Reserve
	{0x66, 0x00},//##Reserve
	{0x67, 0x00},//##Reserve
	{0x68, 0x00},//##Reserve
	{0x69, 0x00},//##Reserve
	{0x6a, 0x00},//##Reserve
	{0x6b, 0x00},//##Reserve
	{0x6c, 0x00},//##Reserve
	{0x6d, 0x00},//##Reserve
	{0x6e, 0x00},//##Reserve
	{0x6f, 0x00},//##Reserve
	{0x70, 0x00},//##Reserve
	{0x71, 0x00},//##Reserve
	{0x72, 0x00},//##Reserve
	{0x73, 0x00},//##Reserve
	{0x74, 0x30},//##Mono_Key_High_Byte
	{0x75, 0x06},//##Mono_Key_Low_Byte
	{0x76, 0x00},//##Boost_Control
	{0x77, 0x07},//##Hi-res_Item
	{0x78, 0x40},//##Test_Mode_register
	{0x79, 0x62},//##Boost_Strap_OV/UV_Selection
	{0x7a, 0x88},//##OC_Selection_2
	{0x7b, 0x55},//##MBIST_User_Program_Top_Byte_Even
	{0x7c, 0x55},//##MBIST_User_Program_Middle_Byte_Even
	{0x7d, 0x55},//##MBIST_User_Program_Bottom_Byte_Even
	{0x7e, 0x55},//##MBIST_User_Program_Top_Byte_Odd
	{0x7f, 0x55},//##MBIST_User_Program_Middle_Byte_Odd
	{0x80, 0x55},//##MBIST_User_Program_Bottom_Byte_Odd
	{0x81, 0x00},//##ERROR_clear_register
	{0x82, 0x0a},//##Minimum_duty_test
	{0x83, 0x00},//##Reserve
	{0x84, 0xfe},//##Reserve
	{0x85, 0xea},//##Reserve

};

static int m_reg_tab[AD82584F_REGISTER_COUNT][2] = {
	{0x00, 0x04},//##State_Control_1
	{0x01, 0x81},//##State_Control_2
	{0x02, 0x7f},//##State_Control_3
	{0x03, 0x18},//##Master_volume_control
	{0x04, 0x18},//##Channel_1_volume_control
	{0x05, 0x18},//##Channel_2_volume_control
	{0x06, 0x18},//##Channel_3_volume_control
	{0x07, 0x18},//##Channel_4_volume_control
	{0x08, 0x18},//##Channel_5_volume_control
	{0x09, 0x18},//##Channel_6_volume_control
	{0x0a, 0x10},//##Bass_Tone_Boost_and_Cut
	{0x0b, 0x10},//##treble_Tone_Boost_and_Cut
	{0x0c, 0x98},//##State_Control_40X98
	{0x0d, 0x00},//##Channel_1_configuration_registers0x00
	{0x0e, 0x00},//##Channel_2_configuration_registers
	{0x0f, 0x00},//##Channel_3_configuration_registers
	{0x10, 0x00},//##Channel_4_configuration_registers
	{0x11, 0x00},//##Channel_5_configuration_registers
	{0x12, 0x00},//##Channel_6_configuration_registers
	{0x13, 0x00},//##Channel_7_configuration_registers
	{0x14, 0x00},//##Channel_8_configuration_registers
	{0x15, 0x6a},//##DRC1_limiter_attack/release_rate
	{0x16, 0x6a},//##DRC2_limiter_attack/release_rate
	{0x17, 0x9e},//##DRC3_limiter_attack/release_rate
	{0x18, 0x9e},//##DRC4_limiter_attack/release_rate
	{0x19, 0x06},//##Error_Delay
	{0x1a, 0x32},//##State_Control_5
	{0x1b, 0x21},//##HVUV_selection
	{0x1c, 0x00},//##State_Control_6
	{0x1d, 0x7f},//##Coefficient_RAM_Base_Address
	{0x1e, 0x00},//##Top_8-bits_of_coefficients_A1
	{0x1f, 0x00},//##Middle_8-bits_of_coefficients_A1
	{0x20, 0x00},//##Bottom_8-bits_of_coefficients_A1
	{0x21, 0x00},//##Top_8-bits_of_coefficients_A2
	{0x22, 0x00},//##Middle_8-bits_of_coefficients_A2
	{0x23, 0x00},//##Bottom_8-bits_of_coefficients_A2
	{0x24, 0x1f},//##Top_8-bits_of_coefficients_B1
	{0x25, 0x41},//##Middle_8-bits_of_coefficients_B1
	{0x26, 0x39},//##Bottom_8-bits_of_coefficients_B1
	{0x27, 0x00},//##Top_8-bits_of_coefficients_B2
	{0x28, 0x00},//##Middle_8-bits_of_coefficients_B2
	{0x29, 0x00},//##Bottom_8-bits_of_coefficients_B2
	{0x2a, 0x00},//##Top_8-bits_of_coefficients_A0
	{0x2b, 0x5f},//##Middle_8-bits_of_coefficients_A0
	{0x2c, 0x63},//##Bottom_8-bits_of_coefficients_A0
	{0x2d, 0x40},//##Coefficient_R/W_control
	{0x2e, 0x00},//##Protection_Enable/Disable
	{0x2f, 0x00},//##Memory_BIST_status
	{0x30, 0x00},//##Power_Stage_Status(Read_only)
	{0x31, 0x00},//##PWM_Output_Control
	{0x32, 0x00},//##Test_Mode_Control_Reg.
	{0x33, 0x6d},//##Qua-Ternary/Ternary_Switch_Level
	{0x34, 0x00},//##Volume_Fine_tune
	{0x35, 0x00},//##Volume_Fine_tune
	{0x36, 0x60},//##OC_bypass_&_GVDD_selection
	{0x37, 0x52},//##Device_ID_register
	{0x38, 0x00},//##RAM1_test_register_address
	{0x39, 0x00},//##Top_8-bits_of_RAM1_Data
	{0x3a, 0x00},//##Middle_8-bits_of_RAM1_Data
	{0x3b, 0x00},//##Bottom_8-bits_of_RAM1_Data
	{0x3c, 0x00},//##RAM1_test_r/w_control
	{0x3d, 0x00},//##RAM2_test_register_address
	{0x3e, 0x00},//##Top_8-bits_of_RAM2_Data
	{0x3f, 0x00},//##Middle_8-bits_of_RAM2_Data
	{0x40, 0x00},//##Bottom_8-bits_of_RAM2_Data
	{0x41, 0x00},//##RAM2_test_r/w_control
	{0x42, 0x00},//##Level_Meter_Clear
	{0x43, 0x00},//##Power_Meter_Clear
	{0x44, 0x00},//##TOP_of_C1_Level_Meter
	{0x45, 0x17},//##Middle_of_C1_Level_Meter
	{0x46, 0x3e},//##Bottom_of_C1_Level_Meter
	{0x47, 0x00},//##TOP_of_C2_Level_Meter
	{0x48, 0x0b},//##Middle_of_C2_Level_Meter
	{0x49, 0x74},//##Bottom_of_C2_Level_Meter
	{0x4a, 0x00},//##TOP_of_C3_Level_Meter
	{0x4b, 0x00},//##Middle_of_C3_Level_Meter
	{0x4c, 0x00},//##Bottom_of_C3_Level_Meter
	{0x4d, 0x00},//##TOP_of_C4_Level_Meter
	{0x4e, 0x00},//##Middle_of_C4_Level_Meter
	{0x4f, 0x00},//##Bottom_of_C4_Level_Meter
	{0x50, 0x00},//##TOP_of_C5_Level_Meter
	{0x51, 0x00},//##Middle_of_C5_Level_Meter
	{0x52, 0x00},//##Bottom_of_C5_Level_Meter
	{0x53, 0x00},//##TOP_of_C6_Level_Meter
	{0x54, 0x00},//##Middle_of_C6_Level_Meter
	{0x55, 0x00},//##Bottom_of_C6_Level_Meter
	{0x56, 0x00},//##TOP_of_C7_Level_Meter
	{0x57, 0x17},//##Middle_of_C7_Level_Meter
	{0x58, 0x8c},//##Bottom_of_C7_Level_Meter
	{0x59, 0x00},//##TOP_of_C8_Level_Meter
	{0x5a, 0x0b},//##Middle_of_C8_Level_Meter
	{0x5b, 0x9b},//##Bottom_of_C8_Level_Meter
	{0x5c, 0x06},//##I2S_Data_Output_Selection_Register
	{0x5d, 0x00},//##Reserve
	{0x5e, 0x00},//##Reserve
	{0x5f, 0x00},//##Reserve
	{0x60, 0x00},//##Reserve
	{0x61, 0x00},//##Reserve
	{0x62, 0x00},//##Reserve
	{0x63, 0x00},//##Reserve
	{0x64, 0x00},//##Reserve
	{0x65, 0x00},//##Reserve
	{0x66, 0x00},//##Reserve
	{0x67, 0x00},//##Reserve
	{0x68, 0x00},//##Reserve
	{0x69, 0x00},//##Reserve
	{0x6a, 0x00},//##Reserve
	{0x6b, 0x00},//##Reserve
	{0x6c, 0x00},//##Reserve
	{0x6d, 0x00},//##Reserve
	{0x6e, 0x00},//##Reserve
	{0x6f, 0x00},//##Reserve
	{0x70, 0x00},//##Reserve
	{0x71, 0x00},//##Reserve
	{0x72, 0x00},//##Reserve
	{0x73, 0x00},//##Reserve
	{0x74, 0x00},//##Mono_Key_High_Byte
	{0x75, 0x00},//##Mono_Key_Low_Byte
	{0x76, 0x00},//##Boost_Control
	{0x77, 0x07},//##Hi-res_Item
	{0x78, 0x40},//##Test_Mode_register
	{0x79, 0x62},//##Boost_Strap_OV/UV_Selection
	{0x7a, 0x8c},//##OC_Selection_2
	{0x7b, 0x55},//##MBIST_User_Program_Top_Byte_Even
	{0x7c, 0x55},//##MBIST_User_Program_Middle_Byte_Even
	{0x7d, 0x55},//##MBIST_User_Program_Bottom_Byte_Even
	{0x7e, 0x55},//##MBIST_User_Program_Top_Byte_Odd
	{0x7f, 0x55},//##MBIST_User_Program_Middle_Byte_Odd
	{0x80, 0x55},//##MBIST_User_Program_Bottom_Byte_Odd
	{0x81, 0x00},//##ERROR_clear_register
	{0x82, 0x0c},//##Minimum_duty_test
	{0x83, 0x06},//##Reserve
	{0x84, 0xfe},//##Reserve
	{0x85, 0xfe},//##Reserve
};

static const int m_ram1_tab[][4] = {
	{0x00, 0xc0, 0x46, 0x83},//##Channel_1_EQ1_A1 
	{0x01, 0x1f, 0x85, 0x7a},//##Channel_1_EQ1_A2 
	{0x02, 0x3f, 0xb9, 0x7d},//##Channel_1_EQ1_B1 
	{0x03, 0xe0, 0x45, 0xc7},//##Channel_1_EQ1_B2 
	{0x04, 0x20, 0x34, 0xbe},//##Channel_1_EQ1_A0 
	{0x05, 0xc0, 0x39, 0x1b},//##Channel_1_EQ2_A1 
	{0x06, 0x1f, 0xe3, 0x72},//##Channel_1_EQ2_A2 
	{0x07, 0x3f, 0xc6, 0xc9},//##Channel_1_EQ2_B1 
	{0x08, 0xe0, 0x38, 0xfe},//##Channel_1_EQ2_B2 
	{0x09, 0x1f, 0xe3, 0x72},//##Channel_1_EQ2_A0 
	{0x0a, 0xd1, 0x02, 0xf0},//##Channel_1_EQ3_A1 
	{0x0b, 0x19, 0xd4, 0xba},//##Channel_1_EQ3_A2 
	{0x0c, 0x2e, 0xfd, 0x10},//##Channel_1_EQ3_B1 
	{0x0d, 0xe4, 0xc5, 0xed},//##Channel_1_EQ3_B2 
	{0x0e, 0x21, 0x65, 0x58},//##Channel_1_EQ3_A0 
	{0x0f, 0xe0, 0x89, 0xb2},//##Channel_1_EQ4_A1 
	{0x10, 0x0d, 0x8d, 0xd0},//##Channel_1_EQ4_A2 
	{0x11, 0x38, 0x44, 0xf3},//##Channel_1_EQ4_B1 
	{0x12, 0xe6, 0xe7, 0xf4},//##Channel_1_EQ4_B2 
	{0x13, 0x12, 0xbb, 0x96},//##Channel_1_EQ4_A0 
	{0x14, 0xc0, 0xa0, 0xde},//##Channel_1_EQ5_A1 
	{0x15, 0x1f, 0x4d, 0xd7},//##Channel_1_EQ5_A2 
	{0x16, 0x3f, 0x5f, 0x22},//##Channel_1_EQ5_B1 
	{0x17, 0xe0, 0x9d, 0xbd},//##Channel_1_EQ5_B2 
	{0x18, 0x20, 0x14, 0x6b},//##Channel_1_EQ5_A0 
	{0x19, 0xc0, 0x31, 0xfa},//##Channel_1_EQ6_A1 
	{0x1a, 0x1f, 0xe7, 0x03},//##Channel_1_EQ6_A2 
	{0x1b, 0x3f, 0xcd, 0xf0},//##Channel_1_EQ6_B1 
	{0x1c, 0xe0, 0x31, 0xe4},//##Channel_1_EQ6_B2 
	{0x1d, 0x1f, 0xe7, 0x03},//##Channel_1_EQ6_A0 
	{0x1e, 0x00, 0x00, 0x00},//##Channel_1_EQ7_A1 
	{0x1f, 0x00, 0x00, 0x00},//##Channel_1_EQ7_A2 
	{0x20, 0x00, 0x00, 0x00},//##Channel_1_EQ7_B1 
	{0x21, 0x00, 0x00, 0x00},//##Channel_1_EQ7_B2 
	{0x22, 0x20, 0x00, 0x00},//##Channel_1_EQ7_A0 
	{0x23, 0x00, 0x00, 0x00},//##Channel_1_EQ8_A1 
	{0x24, 0x00, 0x00, 0x00},//##Channel_1_EQ8_A2 
	{0x25, 0x00, 0x00, 0x00},//##Channel_1_EQ8_B1 
	{0x26, 0x00, 0x00, 0x00},//##Channel_1_EQ8_B2 
	{0x27, 0x20, 0x00, 0x00},//##Channel_1_EQ8_A0 
	{0x28, 0x00, 0x00, 0x00},//##Channel_1_EQ9_A1 
	{0x29, 0x00, 0x00, 0x00},//##Channel_1_EQ9_A2 
	{0x2a, 0x00, 0x00, 0x00},//##Channel_1_EQ9_B1 
	{0x2b, 0x00, 0x00, 0x00},//##Channel_1_EQ9_B2 
	{0x2c, 0x20, 0x00, 0x00},//##Channel_1_EQ9_A0 
	{0x2d, 0x00, 0x00, 0x00},//##Channel_1_EQ10_A1 
	{0x2e, 0x00, 0x00, 0x00},//##Channel_1_EQ10_A2 
	{0x2f, 0x00, 0x00, 0x00},//##Channel_1_EQ10_B1 
	{0x30, 0x00, 0x00, 0x00},//##Channel_1_EQ10_B2 
	{0x31, 0x20, 0x00, 0x00},//##Channel_1_EQ10_A0 
	{0x32, 0x00, 0x00, 0x00},//##Channel_1_EQ11_A1 
	{0x33, 0x00, 0x00, 0x00},//##Channel_1_EQ11_A2 
	{0x34, 0x00, 0x00, 0x00},//##Channel_1_EQ11_B1 
	{0x35, 0x00, 0x00, 0x00},//##Channel_1_EQ11_B2 
	{0x36, 0x20, 0x00, 0x00},//##Channel_1_EQ11_A0 
	{0x37, 0x00, 0x00, 0x00},//##Channel_1_EQ12_A1 
	{0x38, 0x00, 0x00, 0x00},//##Channel_1_EQ12_A2 
	{0x39, 0x00, 0x00, 0x00},//##Channel_1_EQ12_B1 
	{0x3a, 0x00, 0x00, 0x00},//##Channel_1_EQ12_B2 
	{0x3b, 0x20, 0x00, 0x00},//##Channel_1_EQ12_A0 
	{0x3c, 0x00, 0x00, 0x00},//##Channel_1_EQ13_A1 
	{0x3d, 0x00, 0x00, 0x00},//##Channel_1_EQ13_A2 
	{0x3e, 0x00, 0x00, 0x00},//##Channel_1_EQ13_B1 
	{0x3f, 0x00, 0x00, 0x00},//##Channel_1_EQ13_B2 
	{0x40, 0x20, 0x00, 0x00},//##Channel_1_EQ13_A0 
	{0x41, 0x00, 0x5f, 0x63},//##Channel_1_EQ14_A1 
	{0x42, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A2 
	{0x43, 0x1f, 0x41, 0x39},//##Channel_1_EQ14_B1 
	{0x44, 0x00, 0x00, 0x00},//##Channel_1_EQ14_B2 
	{0x45, 0x00, 0x5f, 0x63},//##Channel_1_EQ14_A0 
	{0x46, 0xe0, 0x35, 0x45},//##Channel_1_EQ15_A1 
	{0x47, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A2 
	{0x48, 0x1f, 0x95, 0x77},//##Channel_1_EQ15_B1 
	{0x49, 0x00, 0x00, 0x00},//##Channel_1_EQ15_B2 
	{0x4a, 0x1f, 0xca, 0xbb},//##Channel_1_EQ15_A0 
	{0x4b, 0x7f, 0xff, 0xff},//##Channel_1_Mixer1 
	{0x4c, 0x00, 0x00, 0x00},//##Channel_1_Mixer2 
	{0x4d, 0x7f, 0xff, 0xff},//##Channel_1_Prescale 
	{0x4e, 0x7f, 0xff, 0xff},//##Channel_1_Postscale 
	{0x4f, 0xc7, 0xb6, 0x91},//##A0_of_L_channel_SRS_HPF 
	{0x50, 0x38, 0x49, 0x6e},//##A1_of_L_channel_SRS_HPF 
	{0x51, 0x0c, 0x46, 0xf8},//##B1_of_L_channel_SRS_HPF 
	{0x52, 0x0e, 0x81, 0xb9},//##A0_of_L_channel_SRS_LPF 
	{0x53, 0xf2, 0x2c, 0x12},//##A1_of_L_channel_SRS_LPF 
	{0x54, 0x0f, 0xca, 0xbb},//##B1_of_L_channel_SRS_LPF 
	{0x55, 0x20, 0x00, 0x00},//##CH1.2_Power_Clipping 
	{0x56, 0x0c, 0xbd, 0x4b},//##CCH1.2_DRC1_Attack_threshold 
	{0x57, 0x0c, 0x06, 0xdc},//##CH1.2_DRC1_Release_threshold 
	{0x58, 0x20, 0x00, 0x00},//##CH3.4_DRC2_Attack_threshold 
	{0x59, 0x08, 0x00, 0x00},//##CH3.4_DRC2_Release_threshold 
	{0x5a, 0x0a, 0x1e, 0x89},//##CH5.6_DRC3_Attack_threshold 
	{0x5b, 0x09, 0x8d, 0xa0},//##CH5.6_DRC3_Release_threshold 
	{0x5c, 0x0d, 0x7e, 0x89},//##CH7.8_DRC4_Attack_threshold 
	{0x5d, 0x0c, 0xbd, 0x4b},//##CH7.8_DRC4_Release_threshold 
	{0x5e, 0x00, 0x00, 0x1a},//##Noise_Gate_Attack_Level 
	{0x5f, 0x00, 0x00, 0x53},//##Noise_Gate_Release_Level 
	{0x60, 0x00, 0x80, 0x00},//##DRC1_Energy_Coefficient 
	{0x61, 0x00, 0x20, 0x00},//##DRC2_Energy_Coefficient 
	{0x62, 0x00, 0x80, 0x00},//##DRC3_Energy_Coefficient 
	{0x63, 0x00, 0x80, 0x00},//##DRC4_Energy_Coefficient 
	{0x64, 0x00, 0x00, 0x00},//DRC1_Power_Meter
	{0x65, 0x00, 0x00, 0x00},//DRC3_Power_Mete
	{0x66, 0x00, 0x00, 0x00},//DRC5_Power_Meter
	{0x67, 0x00, 0x00, 0x00},//DRC7_Power_Meter
	{0x68, 0x00, 0x00, 0x00},//##Channel_1_DEQ1_A1 
	{0x69, 0x00, 0x00, 0x00},//##Channel_1_DEQ1_A2
	{0x6a, 0x00, 0x00, 0x00},//##Channel_1_DEQ1_B1
	{0x6b, 0x00, 0x00, 0x00},//##Channel_1_DEQ1_B2 
	{0x6c, 0x20, 0x00, 0x00},//##Channel_1_DEQ1_A0
	{0x6d, 0x00, 0x00, 0x00},//##Channel_1_DEQ2_A1 
	{0x6e, 0x00, 0x00, 0x00},//##Channel_1_DEQ2_A2 
	{0x6f, 0x00, 0x00, 0x00},//##Channel_1_DEQ2_B1 
	{0x70, 0x00, 0x00, 0x00},//##Channel_1_DEQ2_B2 
	{0x71, 0x20, 0x00, 0x00},//##Channel_1_DEQ2_A0 
	{0x72, 0x00, 0x00, 0x00},//##Channel_1_DEQ3_A1 
	{0x73, 0x00, 0x00, 0x00},//##Channel_1_DEQ3_A2 
	{0x74, 0x00, 0x00, 0x00},//##Channel_1_DEQ3_B1 
	{0x75, 0x00, 0x00, 0x00},//##Channel_1_DEQ3_B2 
	{0x76, 0x20, 0x00, 0x00},//##Channel_1_DEQ3_A0 
	{0x77, 0x00, 0x00, 0x00},//##Channel_1_DEQ4_A1 
	{0x78, 0x00, 0x00, 0x00},//##Channel_1_DEQ4_A2 
	{0x79, 0x00, 0x00, 0x00},//##Channel_1_DEQ4_B1 
	{0x7a, 0x00, 0x00, 0x00},//##Channel_1_DEQ4_B2 
	{0x7b, 0x20, 0x00, 0x00},//##Channel_1_DEQ4_A0 
	{0x7c, 0x00, 0x00, 0x00},//##Reserve
	{0x7d, 0x00, 0x00, 0x00},//##Reserve
	{0x7e, 0x00, 0x00, 0x00},//##Reserve
	{0x7f, 0x00, 0x00, 0x00},//##Reserve
};

static int m_ram2_tab[][4] = {
	{0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A1 
	 {0x01, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A2 
	 {0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B1 
	 {0x03, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B2 
	 {0x04, 0x20, 0x00, 0x00},//##Channel_2_EQ1_A0 
	 {0x05, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A1 
	 {0x06, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A2 
	 {0x07, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B1 
	 {0x08, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B2 
	 {0x09, 0x20, 0x00, 0x00},//##Channel_2_EQ2_A0 
	 {0x0a, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A1 
	 {0x0b, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A2 
	 {0x0c, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B1 
	 {0x0d, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B2 
	 {0x0e, 0x20, 0x00, 0x00},//##Channel_2_EQ3_A0 
	 {0x0f, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A1 
	 {0x10, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A2 
	 {0x11, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B1 
	 {0x12, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B2 
	 {0x13, 0x20, 0x00, 0x00},//##Channel_2_EQ4_A0 
	 {0x14, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A1 
	 {0x15, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A2 
	 {0x16, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B1 
	 {0x17, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B2 
	 {0x18, 0x20, 0x00, 0x00},//##Channel_2_EQ5_A0 
	 {0x19, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A1 
	 {0x1a, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A2 
	 {0x1b, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B1 
	 {0x1c, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B2 
	 {0x1d, 0x20, 0x00, 0x00},//##Channel_2_EQ6_A0 
	 {0x1e, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A1 
	 {0x1f, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A2 
	 {0x20, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B1 
	 {0x21, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B2 
	 {0x22, 0x20, 0x00, 0x00},//##Channel_2_EQ7_A0 
	 {0x23, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A1 
	 {0x24, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A2 
	 {0x25, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B1 
	 {0x26, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B2 
	 {0x27, 0x20, 0x00, 0x00},//##Channel_2_EQ8_A0 
	 {0x28, 0x00, 0x00, 0x00},//##Channel_2_EQ9_A1 
	 {0x29, 0x00, 0x00, 0x00},//##Channel_2_EQ9_A2 
	 {0x2a, 0x00, 0x00, 0x00},//##Channel_2_EQ9_B1 
	 {0x2b, 0x00, 0x00, 0x00},//##Channel_2_EQ9_B2 
	 {0x2c, 0x20, 0x00, 0x00},//##Channel_2_EQ9_A0 
	 {0x2d, 0x00, 0x00, 0x00},//##Channel_2_EQ10_A1 
	 {0x2e, 0x00, 0x00, 0x00},//##Channel_2_EQ10_A2 
	 {0x2f, 0x00, 0x00, 0x00},//##Channel_2_EQ10_B1 
	 {0x30, 0x00, 0x00, 0x00},//##Channel_2_EQ10_B2 
	 {0x31, 0x20, 0x00, 0x00},//##Channel_2_EQ10_A0 
	 {0x32, 0x00, 0x00, 0x00},//##Channel_2_EQ11_A1 
	 {0x33, 0x00, 0x00, 0x00},//##Channel_2_EQ11_A2 
	 {0x34, 0x00, 0x00, 0x00},//##Channel_2_EQ11_B1 
	 {0x35, 0x00, 0x00, 0x00},//##Channel_2_EQ11_B2 
	 {0x36, 0x20, 0x00, 0x00},//##Channel_2_EQ11_A0 
	 {0x37, 0x00, 0x00, 0x00},//##Channel_2_EQ12_A1 
	 {0x38, 0x00, 0x00, 0x00},//##Channel_2_EQ12_A2 
	 {0x39, 0x00, 0x00, 0x00},//##Channel_2_EQ12_B1 
	 {0x3a, 0x00, 0x00, 0x00},//##Channel_2_EQ12_B2 
	 {0x3b, 0x20, 0x00, 0x00},//##Channel_2_EQ12_A0 
	 {0x3c, 0x00, 0x00, 0x00},//##Channel_2_EQ13_A1 
	 {0x3d, 0x00, 0x00, 0x00},//##Channel_2_EQ13_A2 
	 {0x3e, 0x00, 0x00, 0x00},//##Channel_2_EQ13_B1 
	 {0x3f, 0x00, 0x00, 0x00},//##Channel_2_EQ13_B2 
	 {0x40, 0x20, 0x00, 0x00},//##Channel_2_EQ13_A0 
	 {0x41, 0x00, 0x5f, 0x63},//##Channel_2_EQ14_A1 
	 {0x42, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A2 
	 {0x43, 0x1f, 0x41, 0x39},//##Channel_2_EQ14_B1 
	 {0x44, 0x00, 0x00, 0x00},//##Channel_2_EQ14_B2 
	 {0x45, 0x00, 0x5f, 0x63},//##Channel_2_EQ14_A0 
	 {0x46, 0xe0, 0x35, 0x45},//##Channel_2_EQ15_A1 
	 {0x47, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A2 
	 {0x48, 0x1f, 0x95, 0x77},//##Channel_2_EQ15_B1 
	 {0x49, 0x00, 0x00, 0x00},//##Channel_2_EQ15_B2 
	 {0x4a, 0x1f, 0xca, 0xbb},//##Channel_2_EQ15_A0 
	 {0x4b, 0x00, 0x00, 0x00},//##Channel_2_Mixer1 
	 {0x4c, 0x7f, 0xff, 0xff},//##Channel_2_Mixer2 
	 {0x4d, 0x7f, 0xff, 0xff},//##Channel_2_Prescale 
	 {0x4e, 0x7f, 0xff, 0xff},//##Channel_2_Postscale 
	 {0x4f, 0xc7, 0xb6, 0x91},//##A0_of_R_channel_SRS_HPF 
	 {0x50, 0x38, 0x49, 0x6e},//##A1_of_R_channel_SRS_HPF 
	 {0x51, 0x0c, 0x46, 0xf8},//##B1_of_R_channel_SRS_HPF 
	 {0x52, 0x0e, 0x81, 0xb9},//##A0_of_R_channel_SRS_LPF 
	 {0x53, 0xf2, 0x2c, 0x12},//##A1_of_R_channel_SRS_LPF 
	 {0x54, 0x0f, 0xca, 0xbb},//##B1_of_R_channel_SRS_LPF 
	 {0x55, 0x00, 0x00, 0x00},//##Reserve
	 {0x56, 0x00, 0x00, 0x00},//##Reserve
	 {0x57, 0x00, 0x00, 0x00},//##Reserve
	 {0x58, 0x00, 0x00, 0x00},//##Reserve
	 {0x59, 0x00, 0x00, 0x00},//##Reserve
	 {0x5a, 0x00, 0x00, 0x00},//##Reserve
	 {0x5b, 0x00, 0x00, 0x00},//##Reserve
	 {0x5c, 0x00, 0x00, 0x00},//##Reserve
	 {0x5d, 0x00, 0x00, 0x00},//##Reserve
	 {0x5e, 0x00, 0x00, 0x00},//##Reserve
	 {0x5f, 0x00, 0x00, 0x00},//##Reserve
	 {0x60, 0x00, 0x00, 0x00},//##Reserve
	 {0x61, 0x00, 0x00, 0x00},//##Reserve
	 {0x62, 0x00, 0x00, 0x00},//##Reserve
	 {0x63, 0x00, 0x00, 0x00},//##Reserve
	 {0x64, 0x00, 0x00, 0x00},//DRC2_Power_Meter
	 {0x65, 0x00, 0x00, 0x00},//DRC4_Power_Mete
	 {0x66, 0x00, 0x00, 0x00},//DRC6_Power_Meter
	 {0x67, 0x00, 0x00, 0x00},//DRC8_Power_Meter
	 {0x68, 0x00, 0x00, 0x00},//##Channel_2_DEQ1_A1 
	 {0x69, 0x00, 0x00, 0x00},//##Channel_2_DEQ1_A2
	 {0x6a, 0x00, 0x00, 0x00},//##Channel_2_DEQ1_B1
	 {0x6b, 0x00, 0x00, 0x00},//##Channel_2_DEQ1_B2 
	 {0x6c, 0x20, 0x00, 0x00},//##Channel_2_DEQ1_A0
	 {0x6d, 0x00, 0x00, 0x00},//##Channel_2_DEQ2_A1 
	 {0x6e, 0x00, 0x00, 0x00},//##Channel_2_DEQ2_A2 
	 {0x6f, 0x00, 0x00, 0x00},//##Channel_2_DEQ2_B1 
	 {0x70, 0x00, 0x00, 0x00},//##Channel_2_DEQ2_B2 
	 {0x71, 0x20, 0x00, 0x00},//##Channel_2_DEQ2_A0 
	 {0x72, 0x00, 0x00, 0x00},//##Channel_2_DEQ3_A1 
	 {0x73, 0x00, 0x00, 0x00},//##Channel_2_DEQ3_A2 
	 {0x74, 0x00, 0x00, 0x00},//##Channel_2_DEQ3_B1 
	 {0x75, 0x00, 0x00, 0x00},//##Channel_2_DEQ3_B2 
	 {0x76, 0x20, 0x00, 0x00},//##Channel_2_DEQ3_A0 
	 {0x77, 0x00, 0x00, 0x00},//##Channel_2_DEQ4_A1 
	 {0x78, 0x00, 0x00, 0x00},//##Channel_2_DEQ4_A2 
	 {0x79, 0x00, 0x00, 0x00},//##Channel_2_DEQ4_B1 
	 {0x7a, 0x00, 0x00, 0x00},//##Channel_2_DEQ4_B2 
	 {0x7b, 0x20, 0x00, 0x00},//##Channel_2_DEQ4_A0 
	 {0x7c, 0x00, 0x00, 0x00},//##Reserve
	 {0x7d, 0x00, 0x00, 0x00},//##Reserve
	 {0x7e, 0x00, 0x00, 0x00},//##Reserve
	 {0x7f, 0x00, 0x00, 0x00},//##Reserve
};

static void ad82584_power_on(struct snd_soc_codec *codec, struct ad82584f_priv *ad82584f)
{
//	int vol = ad82584f->last_volume;
	
	if (ad82584f->pdata->reset_pin > 0) {
		gpio_direction_output(ad82584f->pdata->reset_pin, GPIOF_OUT_INIT_LOW);
		mdelay(1);
		gpio_direction_output(ad82584f->pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	}
	if (ad82584f->pdata->power_down_pin > 0) {
		gpio_direction_output(ad82584f->pdata->power_down_pin, GPIOF_OUT_INIT_HIGH);
	}
	mdelay(20);

	snd_soc_write(codec, 0x1a, 0x12);//reset
	mdelay(10);
	snd_soc_write(codec, 0x1a, 0x32);

	snd_soc_write(codec, 0x02, 0x7f); // mute
	ad82584f_reg_init(codec);
	ad82584f_set_eq_drc(codec); //eq and drc
	
	snd_soc_write(codec, 0x02, 0x00);
}

static void ad82584_power_off(struct snd_soc_codec *codec, struct ad82584f_priv *ad82584f)
{
	unsigned int val;

	val = snd_soc_read(codec, MVOL);
	if ( val > 0 ) {
		ad82584f->last_volume = val;
	}
	snd_soc_write(codec, 0x02, 0x7f);		// mute
	//if (ad82584f->pdata->power_down_pin > 0) {
		//gpio_direction_output(ad82584f->pdata->power_down_pin, GPIOF_OUT_INIT_LOW);
	//}
	mdelay(35);
	if (ad82584f->pdata->reset_pin > 0) {
		gpio_direction_output(ad82584f->pdata->reset_pin, GPIOF_OUT_INIT_LOW);
 	}
}

static int ad82584f_get_power(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = ad82584f->power_enum_value;

	return 0;
}

/*
 * GVA : boot up initialization sequence
 * 1.ad82584f_set_power (SPK unmute)
 * 2.enter play mode, will call prepare function
 * so we can set /PD high in ad82584f_init called by prepare
*/
static int ad82584f_set_power(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);

	ad82584f->power_enum_value = ucontrol->value.integer.value[0];

	//pr_info(">>>>>>>>%s  %d\n", __func__, ad82584f->power_enum_value);

	if (ad82584f->power_enum_value == 1) {
		ad82584_power_on(codec, ad82584f); //power on prepare function.
	} else {
		ad82584_power_off(codec, ad82584f);
	}

	return 0;
}


static int ad82584f_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ad82584f_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
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

static int ad82584f_hw_params(struct snd_pcm_substream *substream,
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

static int ad82584f_set_bias_level(struct snd_soc_codec *codec,
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

static int init_ad82584f_power_pin(struct snd_soc_codec *codec)
{
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);
	struct ad82584f_platform_data *pdata = ad82584f->pdata;
	int ret = 0;

	if (pdata->power_down_pin < 0)
		return 0;

	ret = devm_gpio_request_one(codec->dev, pdata->power_down_pin,
					    GPIOF_OUT_INIT_HIGH,
					    "ad82584f-power-down-pin");
	if (ret < 0)
		return -1;

	gpio_direction_output(pdata->power_down_pin, GPIOF_OUT_INIT_HIGH);

	return 0;
}

static int set_ad82584f_power(struct snd_soc_codec *codec, int on)
{
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);
	struct ad82584f_platform_data *pdata = ad82584f->pdata;

	if (pdata->power_down_pin < 0)
		return 0;

	if (on)
		gpio_direction_output(pdata->power_down_pin,
							GPIOF_OUT_INIT_HIGH);
	else
		gpio_direction_output(pdata->power_down_pin,
							GPIOF_OUT_INIT_LOW);

	return 0;
}

static int ad82584f_startup(struct snd_pcm_substream *substream,
							struct snd_soc_dai *dai)
{
	pr_info("%s, %d\n", __func__, __LINE__);
	return 0;
}

static int ad82584f_trigger(struct snd_pcm_substream *substream, int trigger,
                  struct snd_soc_dai *dai)
{
    pr_info("%s, %d trigger:%d\n", __func__, __LINE__, trigger);
    return 0;
}

static int ad82584f_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_codec *codec = dai->codec;

    pr_info("%s, %d mute:%d, stream:%d\n", __func__, __LINE__, mute, stream);

	if(1 == mute){
		snd_soc_write(codec, 0x02, 0x7f); // mute
	}else{
		snd_soc_write(codec, 0x02, 0x00); // unmute
	}

    return 0;
}

static void ad82584f_shutdown(struct snd_pcm_substream *substream,
						struct snd_soc_dai *dai)
{
	pr_info("%s, %d\n", __func__, __LINE__);
}
#if 0
static int ad82584f_prepare(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);
	
	pr_info("%s\n", __func__);

	ad82584_power_on(codec, ad82584f);

	return 0;
}
#endif
static const struct snd_soc_dai_ops ad82584f_dai_ops = {
	.hw_params = ad82584f_hw_params,
	.set_sysclk = ad82584f_set_dai_sysclk,
	.set_fmt = ad82584f_set_dai_fmt,
	.startup = ad82584f_startup,
	.trigger = ad82584f_trigger,
	.mute_stream = ad82584f_mute_stream,
	.shutdown = ad82584f_shutdown,
	.prepare = NULL,
};

static struct snd_soc_dai_driver ad82584f_dai = {
	.name = "ad82584f",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = AD82584F_RATES,
		.formats = AD82584F_FORMATS,
	},
	.ops = &ad82584f_dai_ops,
};

static int ad82584f_set_eq_drc(struct snd_soc_codec *codec)
{
	u8 i;

	for (i = 0; i < AD82584F_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram1_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram1_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram1_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram1_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x01);
	}
	for (i = 0; i < AD82584F_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram2_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram2_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram2_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram2_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x41);
	}
	return 0;
}

static int reset_ad82584f_GPIO(struct snd_soc_codec *codec)
{
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);
	struct ad82584f_platform_data *pdata = ad82584f->pdata;
	int ret = 0;

	if (pdata->reset_pin < 0)
		return 0;

	ret = devm_gpio_request_one(codec->dev, pdata->reset_pin,
					    GPIOF_OUT_INIT_HIGH,
					    "ad82584f-reset-pin");
	if (ret < 0)
		return -1;

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	mdelay(15);

	return 0;
}

static int ad82584f_reg_init(struct snd_soc_codec *codec)
{
	int i = 0;

	for (i = 0; i < AD82584F_REGISTER_COUNT; i++) {
		snd_soc_write(codec, m_reg_tab[i][0], m_reg_tab[i][1]);
	};
	return 0;

}
static int ad82584f_init(struct snd_soc_codec *codec)
{
	reset_ad82584f_GPIO(codec);
	init_ad82584f_power_pin(codec);

	set_ad82584f_power(codec, 1);
	mdelay(20);

	snd_soc_write(codec, 0x1a, 0x12);
	mdelay(10);
	snd_soc_write(codec, 0x1a, 0x32);

	snd_soc_write(codec, 0x02, 0x7f);		// mute
	
	ad82584f_reg_init(codec);

	/*eq and drc*/
	ad82584f_set_eq_drc(codec);
	dev_info(codec->dev, "ad82584f_suspend!\n");
	snd_soc_write(codec, 0x02, 0x00);

	return 0;
}

static int ad82584f_probe(struct snd_soc_codec *codec)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);

	ad82584f->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ad82584f->early_suspend.suspend = ad82584f_early_suspend;
	ad82584f->early_suspend.resume = ad82584f_late_resume;
	ad82584f->early_suspend.param = codec;
	register_early_suspend(&(ad82584f->early_suspend));
#endif
	ad82584f_init(codec);

	return 0;
}

static int ad82584f_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct ad82584f_priv *ad82584f = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(ad82584f->early_suspend));
#endif
	return 0;
}

#ifdef CONFIG_PM
static int ad82584f_suspend(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "ad82584f_suspend!\n");

	return 0;
}

static int ad82584f_resume(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "ad82584f_resume!\n");

	return 0;
}
#else
#define ad82584f_suspend NULL
#define ad82584f_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ad82584f_early_suspend(struct early_suspend *h)
{
}

static void ad82584f_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget ad82584f_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver soc_codec_dev_ad82584f = {
	.probe = ad82584f_probe,
	.remove = ad82584f_remove,
	.suspend = ad82584f_suspend,
	.resume = ad82584f_resume,
	.set_bias_level = ad82584f_set_bias_level,
	.component_driver = {
		.controls = ad82584f_snd_controls,
		.num_controls = ARRAY_SIZE(ad82584f_snd_controls),
		.dapm_widgets = ad82584f_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ad82584f_dapm_widgets),
	}
};

static const struct regmap_config ad82584f_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = AD82584F_REGISTER_COUNT,
	.reg_defaults = ad82584f_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ad82584f_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int ad82584f_parse_dt(
	struct ad82584f_priv *ad82584f,
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
	ad82584f->pdata->reset_pin = reset_pin;

	power_down_pin = of_get_named_gpio(np, "power_down_pin", 0);
	if (power_down_pin < 0) {
		pr_err("%s fail to get power down pin from dts!\n", __func__);
		ret = -1;
	} else {
		pr_info("%s pdata->power_down_pin = %d!\n", __func__,power_down_pin);
	}
	ad82584f->pdata->power_down_pin = power_down_pin;

	return ret;
}

static int ad82584f_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct ad82584f_priv *ad82584f;
	struct ad82584f_platform_data *pdata;
	int ret;

	ad82584f = devm_kzalloc(&i2c->dev, sizeof(struct ad82584f_priv),
			       GFP_KERNEL);
	if (!ad82584f)
		return -ENOMEM;

	ad82584f->regmap = devm_regmap_init_i2c(i2c, &ad82584f_regmap);
	if (IS_ERR(ad82584f->regmap)) {
		ret = PTR_ERR(ad82584f->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	i2c_set_clientdata(i2c, ad82584f);

	pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct ad82584f_platform_data),
				GFP_KERNEL);
	if (!pdata) {
		pr_err("%s failed to kzalloc for ad82584f pdata\n", __func__);
		return -ENOMEM;
	}

	ad82584f->pdata = pdata;

	ad82584f_parse_dt(ad82584f, i2c->dev.of_node);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_ad82584f,
				     &ad82584f_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int ad82584f_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static const struct i2c_device_id ad82584f_i2c_id[] = {
	{ " ad82584f", 0 },
	{}
};

static const struct of_device_id ad82584f_of_id[] = {
	{ .compatible = "ESMT, ad82584f", },
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, ad82584f_of_id);

static struct i2c_driver ad82584f_i2c_driver = {
	.driver = {
		.name = "ad82584f",
		.of_match_table = ad82584f_of_id,
		.owner = THIS_MODULE,
	},
	.probe = ad82584f_i2c_probe,
	.remove = ad82584f_i2c_remove,
	.id_table = ad82584f_i2c_id,
};

module_i2c_driver(ad82584f_i2c_driver);

MODULE_DESCRIPTION("ASoC ad82584f driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");
