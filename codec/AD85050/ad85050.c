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
#include <linux/of.h>
//#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/gpio.h>
#include <linux/amlogic/gpio-amlogic.h>
#include "../../../drivers/gpio/gpiolib.h"

#include "ad85050.h"
#define LINUX_VERSION_CODE 264516   //include/generated/uapi/linux/version.h
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c)) //(4, 0, 0) == 2^18 == 262144

#define TDM_A	0
#define TDM_B	1
#define TDM_C	2
#define LANE_MAX 4

static int g_mute_pin = 0;
struct gpio_desc *desc;

//#define	AD85050_REG_RAM_CHECK
#define	AD85050_REG_EQ_DRC

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void ad85050_early_suspend(struct early_suspend *h);
static void ad85050_late_resume(struct early_suspend *h);
#endif

#define AD85050_RATES (SNDRV_PCM_RATE_32000 | \
        SNDRV_PCM_RATE_44100 | \
        SNDRV_PCM_RATE_48000 | \
        SNDRV_PCM_RATE_64000 | \
        SNDRV_PCM_RATE_88200 | \
        SNDRV_PCM_RATE_96000 | \
        SNDRV_PCM_RATE_176400 | \
        SNDRV_PCM_RATE_192000)

#define AD85050_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
        SNDRV_PCM_FMTBIT_S24_LE | \
        SNDRV_PCM_FMTBIT_S32_LE)

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

static const struct snd_kcontrol_new ad85050_snd_controls[] = {
    SOC_SINGLE_TLV("ad850 Volume", MVOL, 0,
            0xff, 1, mvol_tlv),
    SOC_SINGLE_TLV("Ch1 Volume", C1VOL, 0,
            0xff, 1, chvol_tlv),
    SOC_SINGLE_TLV("Ch2 Volume", C2VOL, 0,
            0xff, 1, chvol_tlv),
};

static struct snd_soc_codec *ad85050_codec;

static int m_reg_tab[AD85050_REGISTER_COUNT][2] = {
    {0x00, 0x00},//##State_Control_1
    {0x01, 0x91},//##State_Control_2
    {0x02, 0x00},//##State_Control_3
    {0x03, 0x18},//##Master_volume_control
    {0x04, 0x18},//##Channel_1_volume_control
    {0x05, 0x18},//##Channel_2_volume_control
    {0x06, 0x18},//##Channel_3_volume_control
    {0x07, 0x18},//##Channel_4_volume_control
    {0x08, 0x18},//##Channel_5_volume_control
    {0x09, 0x18},//##Channel_6_volume_control
    {0x0a, 0x10},//##Bass_Tone_Boost_and_Cut
    {0x0b, 0x10},//##Treble_Tone_Boost_and_Cut
    {0x0c, 0x90},//##State_Control_4
    {0x0d, 0x0c},//##Channel_1_configuration_registers
    {0x0e, 0x0c},//##Channel_2_configuration_registers
    {0x0f, 0x05},//##Channel_3_configuration_registers
    {0x10, 0x05},//##Channel_4_configuration_registers
    {0x11, 0x05},//##Channel_5_configuration_registers
    {0x12, 0x05},//##Channel_6_configuration_registers
    {0x13, 0x04},//##Channel_7_configuration_registers
    {0x14, 0x04},//##Channel_8_configuration_registers
    {0x15, 0x6a},//##DRC1_limiter_attack/release_rate
    {0x16, 0x6a},//##DRC2_limiter_attack/release_rate
    {0x17, 0x6a},//##DRC3_limiter_attack/release_rate
    {0x18, 0x6a},//##DRC4_limiter_attack/release_rate
    {0x19, 0x10},//##State_Control_6
    {0x1a, 0x70},//##State_Control_7
    {0x1b, 0x00},//##State_Control_8
    {0x1c, 0x00},//##State_Control_9
    {0x1d, 0x00},//##Coefficient_RAM_Base_Address
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
    {0x2a, 0x00},//##Top_8-bits_of_coefficients_A0
    {0x2b, 0x00},//##Middle_8-bits_of_coefficients_A0
    {0x2c, 0x00},//##Bottom_8-bits_of_coefficients_A0
    {0x2d, 0x00},//##Coefficient_RAM_R/W_control
    {0x2e, 0x00},//##Protection_Enable/Disable
    {0x2f, 0x00},//##Memory_BIST_status
    {0x30, 0x00},//##Memory_Test_Tor_Sensing_Time
    {0x31, 0x00},//##Scan_key
    {0x32, 0x00},//##Test_Mode_Control_Reg
    {0x33, 0x00},//####Test_Mode_2_Control_Reg
    {0x34, 0x00},//##Volume_Fine_tune
    {0x35, 0x00},//##Volume_Fine_tune
    {0x36, 0x40},//##HP_Control
    {0x37, 0x00},//##Device_ID_register
    {0x38, 0x00},//##RAM1_test_register_address
    {0x39, 0x00},//##Top_8-bits_of_RAM1_Data
    {0x3a, 0x00},//##Middle_8-bits_of_RAM1_Data
    {0x3b, 0x00},//##Bottom_8-bits_of_RAM1_Data
    {0x3c, 0x00},//##RAM1_test_r/w_control
    {0x3d, 0x00},//##RAM3_test_register_address
    {0x3e, 0x00},//##Top_8-bits_of_RAM3_Data
    {0x3f, 0x00},//##Middle_8-bits_of_RAM3_Data
    {0x40, 0x00},//##Bottom_8-bits_of_RAM3_Data
    {0x41, 0x00},//##RAM3_test_r/w_control
    {0x42, 0x00},//##Level_Meter_Clear
    {0x43, 0x00},//##Power_Meter_Clear
    {0x44, 0x00},//##TOP_of_C1_Level_Meter
    {0x45, 0x00},//##Middle_of_C1_Level_Meter
    {0x46, 0x00},//##Bottom_of_C1_Level_Meter
    {0x47, 0x00},//##TOP_of_C2_Level_Meter
    {0x48, 0x00},//##Middle_of_C2_Level_Meter
    {0x49, 0x00},//##Bottom_of_C2_Level_Meter
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
    {0x57, 0x00},//##Middle_of_C7_Level_Meter
    {0x58, 0x00},//##Bottom_of_C7_Level_Meter
    {0x59, 0x00},//##TOP_of_C8_Level_Meter
    {0x5a, 0x00},//##Middle_of_C8_Level_Meter
    {0x5b, 0x00},//##Bottom_of_C8_Level_Meter
    {0x5c, 0x00},//##I2S_Data_Output_Selection_Register
    {0x5d, 0x00},//##Check_Status
    {0x5e, 0x00},//##Top_8_bits_of_DRC_CHK_set_value
    {0x5f, 0x00},//##Middle_8_bits_of_DRC_CHK_set_value
    {0x60, 0x00},//##Bottom_8_bits_of_DRC_CHK_set_value
    {0x61, 0x00},//##Top_8_bits_of_BEQ_CHK_set_value
    {0x62, 0x00},//##Middle_8_bits_of_BEQ_CHK_set_value
    {0x63, 0x00},//##Bottom_8_bits_of_BEQ_CHK_set_value
    {0x64, 0x00},//##Top_8_bits_of_DRC_CHK_result
    {0x65, 0x00},//##Middle_8_bits_of_DRC_CHK_result
    {0x66, 0x00},//##Bottom_8_bits_of_DRC_CHK_result
    {0x67, 0x00},//##Top_8_bits_of_BEQ_CHK_result
    {0x68, 0x00},//##Middle_8_bits_of_BEQ_CHK_result
    {0x69, 0x00},//##Bottom_8_bits_of_BEQ_CHK_result
    {0x6a, 0x00},//##Reserve
    {0x6b, 0x00},//##Reserve
    {0x6c, 0x00},//##Reserve
    {0x6d, 0x00},//##Reserve
    {0x6e, 0x00},//##Reserve
    {0x6f, 0x00},//##Reserve
    {0x70, 0x78},//##Dither_Signal_Setting
    {0x71, 0x00},//##Reserve
    {0x72, 0x00},//##Reserve
    {0x73, 0x00},//##Reserve
    {0x74, 0x00},//##Mono_Key_High_Byte
    {0x75, 0x00},//##Mono_Key_Low_Byte
    {0x76, 0x00},//##Reserve
    {0x77, 0x07},//##Hi-res_Item
    {0x78, 0xfc},//##Test_Mode_register
    {0x79, 0x58},//##VOS_Control_and_IDAC_VN_LPF_Setting
    {0x7a, 0x00},//##Reserve
    {0x7b, 0x55},//##MBIST_User_Program_Top_Byte_Even
    {0x7c, 0x55},//##MBIST_User_Program_Middle_Byte_Even
    {0x7d, 0x55},//##MBIST_User_Program_Bottom_Byte_Even
    {0x7e, 0x55},//##MBIST_User_Program_Top_Byte_Odd
    {0x7f, 0x55},//##MBIST_User_Program_Middle_Byte_Odd
    {0x80, 0x55},//##MBIST_User_Program_Bottom_Byte_Odd
    {0x81, 0x15},//##GPIO0_Control
    {0x82, 0x0f},//##GPIO1_Control
    {0x83, 0x0f},//##GPIO2_Control
    {0x84, 0x00},//##ERROR_Register_Read_only
    {0x85, 0x00},//##ERROR_Latch_Register_Read_only
    {0x86, 0x00},//##ERROR_Clear_Register
    {0x87, 0xff},//##HP_Master_Volume
    {0x88, 0x18},//##HP_Channel_1_Volume
    {0x89, 0x18},//##HP_Channel_2_Volume
    {0x8a, 0x18},//##HP_Channel_3_Volume
    {0x8b, 0x18},//##HP_Channel_4_Volume
    {0x8c, 0x18},//##HP_Channel_5_Volume
    {0x8d, 0x18},//##HP_Channel_6_Volume
    {0x8e, 0x00},//##HP_Volume_Fine_Tune_1
    {0x8f, 0x00},//##HP_Volume_Fine_Tune_2
    {0x90, 0x6a},//##SMB_Left_DRC_A/R_rate
    {0x91, 0x6a},//##SMB_Right_DRC_A/R_rate
    {0x92, 0x00},//##RAM2_Test_egister_Address
    {0x93, 0x00},//##Top_8-bits_of_RAM2_Data
    {0x94, 0x00},//##Middle_8-bits_of_RAM2_Data
    {0x95, 0x00},//##Bottom_8-bits_of_RAM2_Data
    {0x96, 0x00},//##RAM2_test_r/w_control
    {0x97, 0x00},//##RAM4_Test_egister_Address
    {0x98, 0x00},//##Top_8-bits_of_RAM4_Data
    {0x99, 0x00},//##Middle_8-bits_of_RAM4_Data
    {0x9a, 0x00},//##Bottom_8-bits_of_RAM4_Data
    {0x9b, 0x00},//##RAM4_test_r/w_control
};
#ifdef	AD85050_REG_EQ_DRC

static int m_ram1_tab[][4] = {
    {0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ1_A1
    {0x01, 0x00, 0x00, 0x00},//##Channel_1_EQ1_A2
    {0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ1_B1
    {0x03, 0x00, 0x00, 0x00},//##Channel_1_EQ1_B2
    {0x04, 0x20, 0x00, 0x00},//##Channel_1_EQ1_A0
    {0x05, 0x00, 0x00, 0x00},//##Channel_1_EQ2_A1
    {0x06, 0x00, 0x00, 0x00},//##Channel_1_EQ2_A2
    {0x07, 0x00, 0x00, 0x00},//##Channel_1_EQ2_B1
    {0x08, 0x00, 0x00, 0x00},//##Channel_1_EQ2_B2
    {0x09, 0x20, 0x00, 0x00},//##Channel_1_EQ2_A0
    {0x0a, 0x00, 0x00, 0x00},//##Channel_1_EQ3_A1
    {0x0b, 0x00, 0x00, 0x00},//##Channel_1_EQ3_A2
    {0x0c, 0x00, 0x00, 0x00},//##Channel_1_EQ3_B1
    {0x0d, 0x00, 0x00, 0x00},//##Channel_1_EQ3_B2
    {0x0e, 0x20, 0x00, 0x00},//##Channel_1_EQ3_A0
    {0x0f, 0x00, 0x00, 0x00},//##Channel_1_EQ4_A1
    {0x10, 0x00, 0x00, 0x00},//##Channel_1_EQ4_A2
    {0x11, 0x00, 0x00, 0x00},//##Channel_1_EQ4_B1
    {0x12, 0x00, 0x00, 0x00},//##Channel_1_EQ4_B2
    {0x13, 0x20, 0x00, 0x00},//##Channel_1_EQ4_A0
    {0x14, 0x00, 0x00, 0x00},//##Channel_1_EQ5_A1
    {0x15, 0x00, 0x00, 0x00},//##Channel_1_EQ5_A2
    {0x16, 0x00, 0x00, 0x00},//##Channel_1_EQ5_B1
    {0x17, 0x00, 0x00, 0x00},//##Channel_1_EQ5_B2
    {0x18, 0x20, 0x00, 0x00},//##Channel_1_EQ5_A0
    {0x19, 0x00, 0x00, 0x00},//##Channel_1_EQ6_A1
    {0x1a, 0x00, 0x00, 0x00},//##Channel_1_EQ6_A2
    {0x1b, 0x00, 0x00, 0x00},//##Channel_1_EQ6_B1
    {0x1c, 0x00, 0x00, 0x00},//##Channel_1_EQ6_B2
    {0x1d, 0x20, 0x00, 0x00},//##Channel_1_EQ6_A0
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
    {0x41, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A1 
    {0x42, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A2 
    {0x43, 0x00, 0x00, 0x00},//##Channel_1_EQ14_B1 
    {0x44, 0x00, 0x00, 0x00},//##Channel_1_EQ14_B2 
    {0x45, 0x20, 0x00, 0x00},//##Channel_1_EQ14_A0 
    {0x46, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A1 
    {0x47, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A2 
    {0x48, 0x00, 0x00, 0x00},//##Channel_1_EQ15_B1 
    {0x49, 0x00, 0x00, 0x00},//##Channel_1_EQ15_B2 
    {0x4a, 0x20, 0x00, 0x00},//##Channel_1_EQ15_A0 
    {0x4b, 0x7f, 0xff, 0xff},//##Channel_1_Mixer1 
    {0x4c, 0x00, 0x00, 0x00},//##Channel_1_Mixer2 
    {0x4d, 0x08, 0x00, 0x00},//##Channel_1_Prescale 
    {0x4e, 0x20, 0x00, 0x00},//##Channel_1_Postscale 
    {0x4f, 0xc7, 0xb6, 0x91},//##A0_of_L_channel_SRS_HPF 
    {0x50, 0x38, 0x49, 0x6e},//##A1_of_L_channel_SRS_HPF 
    {0x51, 0x0c, 0x46, 0xf8},//##B1_of_L_channel_SRS_HPF 
    {0x52, 0x0e, 0x81, 0xb9},//##A0_of_L_channel_SRS_LPF 
    {0x53, 0xf2, 0x2c, 0x12},//##A1_of_L_channel_SRS_LPF 
    {0x54, 0x0f, 0xca, 0xbb},//##B1_of_L_channel_SRS_LPF 
    {0x55, 0x20, 0x00, 0x00},//##CH1.2_Power_Clipping 
    {0x56, 0x20, 0x00, 0x00},//##CCH1.2_DRC1_Attack_threshold 
    {0x57, 0x20, 0x00, 0x00},//##CH1.2_DRC1_Release_threshold 
    {0x58, 0x20, 0x00, 0x00},//##CH3.4_DRC2_Attack_threshold 
    {0x59, 0x08, 0x00, 0x00},//##CH3.4_DRC2_Release_threshold 
    {0x5a, 0x20, 0x00, 0x00},//##CH5.6_DRC3_Attack_threshold 
    {0x5b, 0x20, 0x00, 0x00},//##CH5.6_DRC3_Release_threshold 
    {0x5c, 0x20, 0x00, 0x00},//##CH7.8_DRC4_Attack_threshold 
    {0x5d, 0x08, 0x00, 0x00},//##CH7.8_DRC4_Release_threshold 
    {0x5e, 0x00, 0x00, 0x1a},//##Noise_Gate_Attack_Level 
    {0x5f, 0x00, 0x00, 0x53},//##Noise_Gate_Release_Level 
    {0x60, 0x00, 0x80, 0x00},//##DRC1_Energy_Coefficient 
    {0x61, 0x00, 0x20, 0x00},//##DRC2_Energy_Coefficient 
    {0x62, 0x00, 0x80, 0x00},//##DRC3_Energy_Coefficient 
    {0x63, 0x00, 0x20, 0x00},//##DRC4_Energy_Coefficient 
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
    {0x80, 0x00, 0x00, 0x2c},//##Channel_1_MF_LPF1_A1
    {0x81, 0x00, 0x00, 0x16},//##Channel_1_MF_LPF1_A2
    {0x82, 0x3f, 0xb3, 0x69},//##Channel_1_MF_LPF1_B1
    {0x83, 0xe0, 0x4c, 0x3d},//##Channel_1_MF_LPF1_B2
    {0x84, 0x00, 0x00, 0x16},//##Channel_1_MF_LPF1_A0
    {0x85, 0x00, 0x00, 0x2c},//##Channel_1_MF_LPF2_A1
    {0x86, 0x00, 0x00, 0x16},//##Channel_1_MF_LPF2_A2
    {0x87, 0x3f, 0xb3, 0x69},//##Channel_1_MF_LPF2_B1
    {0x88, 0xe0, 0x4c, 0x3d},//##Channel_1_MF_LPF2_B2
    {0x89, 0x00, 0x00, 0x16},//##Channel_1_MF_LPF2_A0
    {0x8a, 0x00, 0x00, 0x00},//##Channel_1_MF_BPF1_A1
    {0x8b, 0xff, 0xe5, 0x52},//##Channel_1_MF_BPF1_A2
    {0x8c, 0x3f, 0xb3, 0x69},//##Channel_1_MF_BPF1_B1
    {0x8d, 0xe0, 0x4c, 0x3d},//##Channel_1_MF_BPF1_B2
    {0x8e, 0x00, 0x1a, 0xae},//##Channel_1_MF_BPF1_A0
    {0x8f, 0x00, 0x00, 0x00},//##Channel_1_MF_BPF2_A1
    {0x90, 0xff, 0xe5, 0x52},//##Channel_1_MF_BPF2_A2
    {0x91, 0x3f, 0xb3, 0x69},//##Channel_1_MF_BPF2_B1
    {0x92, 0xe0, 0x4c, 0x3d},//##Channel_1_MF_BPF2_B2
    {0x93, 0x00, 0x1a, 0xae},//##Channel_1_MF_BPF2_A0
    {0x94, 0x08, 0x00, 0x00},//##Channel_1_MF_CLIP
    {0x95, 0x01, 0x9a, 0xfd},//##Channel_1_MF_Gain1
    {0x96, 0x08, 0x00, 0x00},//##Channel_1_MF_Gain2
    {0x97, 0x0b, 0x4c, 0xe0},//##Channel_1_MF_Gain3
    {0x98, 0x08, 0x00, 0x00},//##Reserve
    {0x99, 0x08, 0x00, 0x00},//##Reserve
    {0x9a, 0x00, 0x00, 0x00},//##Reserve
    {0x9b, 0x00, 0x00, 0x00},//##Reserve
    {0x9c, 0x00, 0x00, 0x00},//##Reserve
    {0x9d, 0x00, 0x00, 0x00},//##Reserve
    {0x9e, 0x00, 0x00, 0x00},//##Reserve
    {0x9f, 0x00, 0x00, 0x00},//##Reserve
    {0xa0, 0x00, 0x00, 0x00},//##Channel_1_EQ16_A1
    {0xa1, 0x00, 0x00, 0x00},//##Channel_1_EQ16_A2
    {0xa2, 0x00, 0x00, 0x00},//##Channel_1_EQ16_B1
    {0xa3, 0x00, 0x00, 0x00},//##Channel_1_EQ16_B2
    {0xa4, 0x20, 0x00, 0x00},//##Channel_1_EQ16_A0
    {0xa5, 0x00, 0x00, 0x00},//##Channel_1_EQ17_A1
    {0xa6, 0x00, 0x00, 0x00},//##Channel_1_EQ17_A2
    {0xa7, 0x00, 0x00, 0x00},//##Channel_1_EQ17_B1
    {0xa8, 0x00, 0x00, 0x00},//##Channel_1_EQ17_B2
    {0xa9, 0x20, 0x00, 0x00},//##Channel_1_EQ17_A0
    {0xaa, 0x00, 0x00, 0x00},//##Channel_1_EQ18_A1
    {0xab, 0x00, 0x00, 0x00},//##Channel_1_EQ18_A2
    {0xac, 0x00, 0x00, 0x00},//##Channel_1_EQ18_B1
    {0xad, 0x00, 0x00, 0x00},//##Channel_1_EQ18_B2
    {0xae, 0x20, 0x00, 0x00},//##Channel_1_EQ18_A0
    {0xaf, 0x20, 0x00, 0x00},//##Channel_1_SMB_ATH
    {0xb0, 0x08, 0x00, 0x00},//##Channel_1_SMB_RTH
    {0xb1, 0x02, 0x00, 0x00},//##Channel_1_Boost_CTRL1
    {0xb2, 0x00, 0x80, 0x00},//##Channel_1_Boost_CTRL2
    {0xb3, 0x00, 0x20, 0x00},//##Channel_1_Boost_CTRL3
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
    {0x41, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A1 
    {0x42, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A2 
    {0x43, 0x00, 0x00, 0x00},//##Channel_2_EQ14_B1 
    {0x44, 0x00, 0x00, 0x00},//##Channel_2_EQ14_B2 
    {0x45, 0x20, 0x00, 0x00},//##Channel_2_EQ14_A0 
    {0x46, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A1 
    {0x47, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A2 
    {0x48, 0x00, 0x00, 0x00},//##Channel_2_EQ15_B1 
    {0x49, 0x00, 0x00, 0x00},//##Channel_2_EQ15_B2 
    {0x4a, 0x20, 0x00, 0x00},//##Channel_2_EQ15_A0 
    {0x4b, 0x00, 0x00, 0x00},//##Channel_2_Mixer1 
    {0x4c, 0x7f, 0xff, 0xff},//##Channel_2_Mixer2 
    {0x4d, 0x08, 0x00, 0x00},//##Channel_2_Prescale 
    {0x4e, 0x20, 0x00, 0x00},//##Channel_2_Postscale 
    {0x4f, 0xc7, 0xb6, 0x91},//##A0_of_R_channel_SRS_HPF 
    {0x50, 0x38, 0x49, 0x6e},//##A1_of_R_channel_SRS_HPF 
    {0x51, 0x0c, 0x46, 0xf8},//##B1_of_R_channel_SRS_HPF 
    {0x52, 0x0e, 0x81, 0xb9},//##A0_of_R_channel_SRS_LPF 
    {0x53, 0xf2, 0x2c, 0x12},//##A1_of_R_channel_SRS_LPF 
    {0x54, 0x0f, 0xca, 0xbb},//##B1_of_R_channel_SRS_LPF 
    {0x55, 0x20, 0x00, 0x00},//##Reserve
    {0x56, 0x20, 0x00, 0x00},//##Reserve
    {0x57, 0x08, 0x00, 0x00},//##Reserve
    {0x58, 0x20, 0x00, 0x00},//##Reserve
    {0x59, 0x08, 0x00, 0x00},//##Reserve
    {0x5a, 0x20, 0x00, 0x00},//##Reserve
    {0x5b, 0x08, 0x00, 0x00},//##Reserve
    {0x5c, 0x20, 0x00, 0x00},//##Reserve
    {0x5d, 0x08, 0x00, 0x00},//##Reserve
    {0x5e, 0x00, 0x00, 0x1a},//##Reserve
    {0x5f, 0x00, 0x00, 0x53},//##Reserve
    {0x60, 0x00, 0x80, 0x00},//##Reserve
    {0x61, 0x00, 0x20, 0x00},//##Reserve
    {0x62, 0x00, 0x80, 0x00},//##Reserve
    {0x63, 0x00, 0x20, 0x00},//##Reserve
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
    {0x80, 0x00, 0x00, 0x2c},//##Channel_2_MF_LPF1_A1
    {0x81, 0x00, 0x00, 0x16},//##Channel_2_MF_LPF1_A2
    {0x82, 0x3f, 0xb3, 0x69},//##Channel_2_MF_LPF1_B1
    {0x83, 0xe0, 0x4c, 0x3d},//##Channel_2_MF_LPF1_B2
    {0x84, 0x00, 0x00, 0x16},//##Channel_2_MF_LPF1_A0
    {0x85, 0x00, 0x00, 0x2c},//##Channel_2_MF_LPF2_A1
    {0x86, 0x00, 0x00, 0x16},//##Channel_2_MF_LPF2_A2
    {0x87, 0x3f, 0xb3, 0x69},//##Channel_2_MF_LPF2_B1
    {0x88, 0xe0, 0x4c, 0x3d},//##Channel_2_MF_LPF2_B2
    {0x89, 0x00, 0x00, 0x16},//##Channel_2_MF_LPF2_A0
    {0x8a, 0x00, 0x00, 0x00},//##Channel_2_MF_BPF1_A1
    {0x8b, 0xff, 0xe5, 0x52},//##Channel_2_MF_BPF1_A2
    {0x8c, 0x3f, 0xb3, 0x69},//##Channel_2_MF_BPF1_B1
    {0x8d, 0xe0, 0x4c, 0x3d},//##Channel_2_MF_BPF1_B2
    {0x8e, 0x00, 0x1a, 0xae},//##Channel_2_MF_BPF1_A0
    {0x8f, 0x00, 0x00, 0x00},//##Channel_2_MF_BPF2_A1
    {0x90, 0xff, 0xe5, 0x52},//##Channel_2_MF_BPF2_A2
    {0x91, 0x3f, 0xb3, 0x69},//##Channel_2_MF_BPF2_B1
    {0x92, 0xe0, 0x4c, 0x3d},//##Channel_2_MF_BPF2_B2
    {0x93, 0x00, 0x1a, 0xae},//##Channel_2_MF_BPF2_A0
    {0x94, 0x08, 0x00, 0x00},//##Channel_2_MF_CLIP
    {0x95, 0x01, 0x9a, 0xfd},//##Channel_2_MF_Gain1
    {0x96, 0x08, 0x00, 0x00},//##Channel_2_MF_Gain2
    {0x97, 0x0b, 0x4c, 0xe0},//##Channel_2_MF_Gain3
    {0x98, 0x08, 0x00, 0x00},//##Reserve
    {0x99, 0x08, 0x00, 0x00},//##Reserve
    {0x9a, 0x00, 0x00, 0x00},//##Reserve
    {0x9b, 0x00, 0x00, 0x00},//##Reserve
    {0x9c, 0x00, 0x00, 0x00},//##Reserve
    {0x9d, 0x00, 0x00, 0x00},//##Reserve
    {0x9e, 0x00, 0x00, 0x00},//##Reserve
    {0x9f, 0x00, 0x00, 0x00},//##Reserve
    {0xa0, 0x00, 0x00, 0x00},//##Channel_2_EQ16_A1
    {0xa1, 0x00, 0x00, 0x00},//##Channel_2_EQ16_A2
    {0xa2, 0x00, 0x00, 0x00},//##Channel_2_EQ16_B1
    {0xa3, 0x00, 0x00, 0x00},//##Channel_2_EQ16_B2
    {0xa4, 0x20, 0x00, 0x00},//##Channel_2_EQ16_A0
    {0xa5, 0x00, 0x00, 0x00},//##Channel_2_EQ17_A1
    {0xa6, 0x00, 0x00, 0x00},//##Channel_2_EQ17_A2
    {0xa7, 0x00, 0x00, 0x00},//##Channel_2_EQ17_B1
    {0xa8, 0x00, 0x00, 0x00},//##Channel_2_EQ17_B2
    {0xa9, 0x20, 0x00, 0x00},//##Channel_2_EQ17_A0
    {0xaa, 0x00, 0x00, 0x00},//##Channel_2_EQ18_A1
    {0xab, 0x00, 0x00, 0x00},//##Channel_2_EQ18_A2
    {0xac, 0x00, 0x00, 0x00},//##Channel_2_EQ18_B1
    {0xad, 0x00, 0x00, 0x00},//##Channel_2_EQ18_B2
    {0xae, 0x20, 0x00, 0x00},//##Channel_2_EQ18_A0
    {0xaf, 0x20, 0x00, 0x00},//##Channel_2_SMB_ATH
    {0xb0, 0x08, 0x00, 0x00},//##Channel_2_SMB_RTH
    {0xb1, 0x01, 0x00, 0x00},//##Reserve
    {0xb2, 0x00, 0x40, 0x00},//##Reserve
    {0xb3, 0x00, 0x10, 0x00},//##Reserve
};
#endif
/* codec private data */
struct ad85050_priv {
    struct regmap *regmap;
    struct snd_soc_codec *codec;
    struct ad85050_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif

    struct i2c_client *control_data;
};

static int ad85050_set_dai_sysclk(struct snd_soc_dai *codec_dai,
        int clk_id, unsigned int freq, int dir)
{
    return 0;
}

static int ad85050_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
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

static int ad85050_hw_params(struct snd_pcm_substream *substream,
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

static int ad85050_set_bias_level(struct snd_soc_codec *codec,
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

static int ad85050_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    int ret = 0;
    struct snd_soc_codec *codec = dai->codec;
    printk("%s---------------%d\n", __func__,__LINE__);

    ret = gpio_direction_output(g_mute_pin, 1);		// pull high AMP_SDB pin
    printk(" gpio mute pin ret=%d\n", ret);
    mdelay(20);
    //ad85050_init(codec);							// init amp again
    snd_soc_write(codec, 0x02, 0x00);   //--unmute amp

    return ret;
}

static void ad85050_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    struct snd_soc_codec *codec = dai->codec;
    printk("%s---------------%d\n", __func__,__LINE__);

    snd_soc_write(codec, 0x02, 0x7f);					//--mute amp //GPIOZ_5,AMP_SBD

    gpio_direction_output(g_mute_pin, 0);						// pull low AMP_SDB pin
    return;
}

static const struct snd_soc_dai_ops ad85050_dai_ops = {
    .startup = ad85050_startup,
    .shutdown = ad85050_shutdown,
    .hw_params = ad85050_hw_params,
    .set_sysclk = ad85050_set_dai_sysclk,
    .set_fmt = ad85050_set_dai_fmt,
};

static struct snd_soc_dai_driver ad85050_dai = {
    .name = "ad85050",   
    .playback = {
        .stream_name = "HIFI Playback",
        .channels_min = 1,
        .channels_max = 8,
        .rates = AD85050_RATES,
        .formats = AD85050_FORMATS,
    },
    .capture = {
        .stream_name ="HIFI Capture",
        .channels_min = 1, 
        .channels_max = 8,
        .rates = AD85050_RATES,
        .formats = AD85050_FORMATS,
    },
    .ops = &ad85050_dai_ops,
};
#ifdef	AD85050_REG_EQ_DRC

static int ad85050_set_eq_drc(struct snd_soc_codec *codec)
{
    u8 i;
    // ch1 ram
    for (i = 0; i < AD85050_RAM_TABLE_COUNT; i++) {
        snd_soc_write(codec, CFADDR, m_ram1_tab[i][0]);
        snd_soc_write(codec, A1CF1, m_ram1_tab[i][1]);
        snd_soc_write(codec, A1CF2, m_ram1_tab[i][2]);
        snd_soc_write(codec, A1CF3, m_ram1_tab[i][3]);
        snd_soc_write(codec, CFUD, 0x01);
    }
    // ch2 ram
    for (i = 0; i < AD85050_RAM_TABLE_COUNT; i++) {
        snd_soc_write(codec, CFADDR, m_ram2_tab[i][0]);
        snd_soc_write(codec, A1CF1, m_ram2_tab[i][1]);
        snd_soc_write(codec, A1CF2, m_ram2_tab[i][2]);
        snd_soc_write(codec, A1CF3, m_ram2_tab[i][3]);
        snd_soc_write(codec, CFUD, 0x41);
    }
    return 0;
}
#endif
static int ad85050_reg_init(struct snd_soc_codec *codec)
{
    int i = 0;
    for (i = 0; i < AD85050_REGISTER_COUNT; i++) {
        if(m_reg_tab[i][0] == 0x02)
            continue;    	
        snd_soc_write(codec, m_reg_tab[i][0], m_reg_tab[i][1]);
    };

    return 0;

}
#ifdef	AD85050_REG_RAM_CHECK
static int ad85050_reg_check(struct snd_soc_codec *codec)
{
    int i = 0;
    int reg_data = 0;

    for (i = 0; i < AD85050_REGISTER_COUNT; i++) {
        reg_data = snd_soc_read(codec, m_reg_tab[i][0]);
        printk("ad85050_reg_init  write 0x%x = 0x%x\n", m_reg_tab[i][0], reg_data);
    };
    return 0;
}

static int ad85050_eqram1_check(struct snd_soc_codec *codec)
{
    int i = 0;
    int H8_data = 0, M8_data = 0, L8_data = 0;

    for (i = 0; i < AD85050_RAM_TABLE_COUNT; i++) {
        snd_soc_write(codec, CFADDR, m_ram1_tab[i][0]);			// write ram addr
        snd_soc_write(codec, CFUD, 0x04);										// write read ram cmd

        H8_data = snd_soc_read(codec, A1CF1);
        M8_data = snd_soc_read(codec, A1CF2);
        L8_data = snd_soc_read(codec, A1CF3);
        printk("ad85050_set_eq_drc ram1  write 0x%x = 0x%x , 0x%x , 0x%x\n", m_ram1_tab[i][0], H8_data,M8_data,L8_data);
    };
    return 0;
}
#endif

static int ad85050_init(struct snd_soc_codec *codec)
{

    int ret;

    printk("%s\n", __func__);

    //ret=gpio_request(g_mute_pin, NULL);				// request AMP_SDB pin control GPIO
    ret = gpio_direction_output(g_mute_pin, 1);		// pull high AMP_SDB pin
    printk(" gpio mute pin ret=%d\n", ret);

    // wait 20ms
    mdelay(20);

    printk("%s---------------%d\n", __func__,__LINE__);

    // software reset amp 
    snd_soc_write(codec, 0x1a, 0x50);//--reset amp
    mdelay(1);
    snd_soc_write(codec, 0x1a, 0x70);//--Normal operation
    mdelay(20);

    // init AMP 
    dev_info(codec->dev, "ad85050_init!\n");

    snd_soc_write(codec, 0x02, 0x7f);//--mute amp

    // write amp register
    ad85050_reg_init(codec);

    snd_soc_write(codec, 0x02, 0x7f);//--mute amp
    udelay(100);

#ifdef	AD85050_REG_EQ_DRC

    // write amp ram (eq and drc ... ) 
    ad85050_set_eq_drc(codec);
    udelay(100);

    printk("%s\n", __func__);
#endif
#ifdef	AD85050_REG_RAM_CHECK

    // Please note that the register from 0x1d to 0x2d and from 0x44 to 0x5b, the value what you read(as below)may be different from what you wrote.
    ad85050_reg_check(codec);
    // Please note that ram1 from 0x64 to 0x67,the value what you read(as below)may be different from what you wrote.
    ad85050_eqram1_check(codec);
#endif	

    snd_soc_write(codec, 0x02, 0x00);//--unmute amp

    return 0; 
}

static int ad85050_probe(struct snd_soc_codec *codec)
{
    //int ret = 0;

    struct ad85050_priv *ad85050 = snd_soc_codec_get_drvdata(codec);
    printk("%s\n", __func__);

    if (codec == NULL) {
        dev_err(codec->dev, "Codec device not registered\n");
        return -ENODEV;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    //	struct ad85050_priv *ad85050 = snd_soc_codec_get_drvdata(codec);

    ad85050->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    ad85050->early_suspend.suspend = ad85050_early_suspend;
    ad85050->early_suspend.resume = ad85050_late_resume;
    //ad85050->early_suspend.param = codec;
    register_early_suspend(&(ad85050->early_suspend));
#endif

    ad85050_codec = codec;

    //ad85050->codec = codec;
    codec->control_data = ad85050->control_data;
    ad85050_init(codec);

    return 0;
}

static int ad85050_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct ad85050_priv *ad85050 = snd_soc_codec_get_drvdata(codec);

    unregister_early_suspend(&(ad85050->early_suspend));
#endif
    return 0;
}

static int ad85050_suspend(struct snd_soc_codec *codec)
{
    dev_info(codec->dev, "ad85050_suspend!\n");

    snd_soc_write(codec, 0x02, 0x7f);		// mute

    return 0;
}

static int ad85050_resume(struct snd_soc_codec *codec)
{
    dev_info(codec->dev, "ad85050_resume!\n");

    // init ad85050 again
    //ad85050_init(codec);

    snd_soc_write(codec, 0x02, 0x00);			// unmute

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ad85050_early_suspend(struct early_suspend *h)
{
}

static void ad85050_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget ad85050_dapm_widgets[] = {
    SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};



/*
 * I2C Read/Write Functions
 */
static int ad85050_i2c_read(struct i2c_client *ad85050_client,
        u8 reg, u8 *value, int len)
{
    int err;
    int tries = 0;

    struct i2c_msg msgs[] = {
        {
            .addr = ad85050_client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = ad85050_client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = value,
        },
    };

    do {
        err = i2c_transfer(ad85050_client->adapter, msgs,
                ARRAY_SIZE(msgs));
        if (err != ARRAY_SIZE(msgs))
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

    if (err != ARRAY_SIZE(msgs)) {
        dev_err(&ad85050_client->dev, "read transfer error %d\n"
                , err);
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}
static unsigned int ad85050_read(struct snd_soc_codec *codec, unsigned int reg)
{
    // struct ad85050_priv *ad85050  = snd_soc_codec_get_drvdata(codec);
    u8 data;
    int len = 1;
    int val = -EIO;

    if (ad85050_i2c_read(codec->control_data, reg & 0xff, &data, len) == 0) {
        val = data;		//(buf[0] << 8 | buf[1]);
    }

    return val;
}
static int ad85050_i2c_write(struct i2c_client *ad85050_client,
        u8 *value, u8 len)
{
    int err;
    int tries = 0;
    struct i2c_msg msgs[] = {
        {
            .addr = ad85050_client->addr,
            .flags = 0,
            .len = len,
            .buf = value,
        },
    };

    do {
        err = i2c_transfer(ad85050_client->adapter, msgs,
                ARRAY_SIZE(msgs));
        if (err != ARRAY_SIZE(msgs))
            msleep_interruptible(I2C_RETRY_DELAY);
    } while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

    if (err != ARRAY_SIZE(msgs)) {
        dev_err(&ad85050_client->dev, "write transfer error\n");
        err = -EIO;
    } else {
        err = 0;
    }

    return err;
}
static int ad85050_write(struct snd_soc_codec *codec, unsigned int reg,
        unsigned int val)
{
    u8 buf[2] = {0, 0};
    int ret;
    buf[0] = (reg & 0xff);
    buf[1] = (val & 0xff);

    ret = ad85050_i2c_write(codec->control_data, buf, ARRAY_SIZE(buf));

    return ret;
}



static const struct snd_soc_codec_driver soc_codec_dev_ad85050 = {
    .probe = ad85050_probe,
    .remove = ad85050_remove,
    .suspend = ad85050_suspend,
    .resume = ad85050_resume,
    .read =    ad85050_read,
    .write =   ad85050_write,
    .set_bias_level = ad85050_set_bias_level,
    .reg_cache_size = ARRAY_SIZE(m_reg_tab),
    .reg_word_size = sizeof(u16),
    .reg_cache_default = m_reg_tab,
#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
    .component_driver = {
#endif
        .controls = ad85050_snd_controls,
        .num_controls = ARRAY_SIZE(ad85050_snd_controls),
        .dapm_widgets = ad85050_dapm_widgets,
        .num_dapm_widgets = ARRAY_SIZE(ad85050_dapm_widgets),
#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
    }
#endif
};

static int ad85050_i2c_probe(struct i2c_client *i2c,
        const struct i2c_device_id *id)
{
    struct ad85050_priv *ad85050;
    struct ad85050_platform_data *pdata;
    //enum of_gpio_flags flags;
    int ret;
    int pin;
    printk("%s---------------%d\n", __func__,__LINE__);
    ad85050 = devm_kzalloc(&i2c->dev, sizeof(struct ad85050_priv),
            GFP_KERNEL);
    if (!ad85050)
        return -ENOMEM;
    /*       
             ad85050->regmap = devm_regmap_init_i2c(i2c, &ad85050_regmap);
             if (IS_ERR(ad85050->regmap)) {
             ret = PTR_ERR(ad85050->regmap);
             dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
             ret);
             return ret;
             }*/
    desc = of_get_named_gpiod_flags(i2c->dev.of_node, "ad85050_amp", 0, NULL);
    pin = desc_to_gpio(desc);
    if (!gpio_is_valid(pin)) {
        pr_err("ad85050 gpio %d is not valid\n", pin);
        return -EINVAL;
    }
    g_mute_pin = pin;
    gpio_request(pin, "ad85050_amp");
    gpio_direction_output(pin, 1);
    printk("g_mute_pin = %d\n", g_mute_pin);

    i2c_set_clientdata(i2c, ad85050);

    pdata = devm_kzalloc(&i2c->dev, sizeof(struct ad85050_platform_data), GFP_KERNEL);
    if (!pdata) {
        pr_err("%s failed to kzalloc for ad85050 pdata\n", __func__);
        return -ENOMEM;
    }
    ad85050->pdata = pdata;

    ad85050->control_data = i2c;

    //ad85050_parse_dt(ad85050, i2c->dev.of_node);

    ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_ad85050, &ad85050_dai, 1);
    if (ret != 0)
    {
        printk("Failed to register codec (%d)\n", ret);
    }

    printk("%s---------------%d probe end\n", __func__,__LINE__);

    return 0;
}

static int ad85050_i2c_remove(struct i2c_client *client)
{
    snd_soc_unregister_codec(&client->dev);

    return 0;
}
/*
   void ad85050_i2c_shutdown(struct i2c_client *client)
   {
//struct snd_soc_codec *codec;
//codec = ad85050_codec;
struct ad85050_priv *ad85050 = snd_soc_codec_get_drvdata(ad85050_codec);
struct ad85050_platform_data *pdata = ad85050->pdata;

gpio_direction_output(pdata->reset_pin, 0);

//return 0;
}*/

static const struct i2c_device_id ad85050_i2c_id[] = {
    { "ad85050", 0 },   //name should be same as BOARD_INFO<arch/arm/mach-xxx struct i2c_board_info ..>
    {},   //{name, addr} //addr slave dev's addr, slave addr = 011 0000
};

MODULE_DEVICE_TABLE(i2c, ad85050_i2c_id);

static const struct of_device_id ad85050_of_id[] = {
    { .compatible = "ESMT, ad85050"},
    { }, //dts
};
//MODULE_DEVICE_TABLE(of, ad85050_of_id);

static struct i2c_driver ad85050_i2c_driver = {
    .driver = {
        .name = "ad85050",
        .of_match_table = ad85050_of_id,
        .owner = THIS_MODULE,
    },
    //.shutdown = ad85050_i2c_shutdown,
    .probe = ad85050_i2c_probe,
    .remove = ad85050_i2c_remove,
    .id_table = ad85050_i2c_id,
};

//module_i2c_driver(ad85050_i2c_driver);

static int __init ad85050_i2c_init(void)
{
    printk("%s\n", __func__);
    return i2c_add_driver(&ad85050_i2c_driver);
}

static void __exit ad85050_i2c_exit(void)
{
    i2c_del_driver(&ad85050_i2c_driver);
}

module_init(ad85050_i2c_init);
module_exit(ad85050_i2c_exit);

MODULE_DESCRIPTION("ASoC ad85050 driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");
