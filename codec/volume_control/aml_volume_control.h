/***************************************************************************
** CopyRight: Amlogic             
** Date     : 2018-09-13
** Description 
**  
***************************************************************************/
#ifndef _AML_VOLUME_H_
#define _AML_VOLUME_H_

#ifdef __cplusplus
extern "C"{ 
#endif


/************************************************************
** FunctionName : aml_mixer_setvolume
** Description  : 设置指定mixer的音量
** Input Param  : 
				  const char *mixer_controler：mixer名字 （可通过 amixer controls | grep "iface=MIXER" 查得）
				  int vol_percent：音量值。 0~100
				  int snd_chnl_id:  -1: All_channel other:  as follow

*** see alsa-lib-x.x.x\include\mixer.h
**** Mixer simple element channel identifier 
***typedef enum _snd_mixer_selem_channel_id {
***	// Unknown //
***	SND_MIXER_SCHN_UNKNOWN = -1,
***	// Front left //
***	SND_MIXER_SCHN_FRONT_LEFT = 0,
***	// Front right //
***	SND_MIXER_SCHN_FRONT_RIGHT,
***	// Rear left //
***	SND_MIXER_SCHN_REAR_LEFT,
***	// Rear right //
***	SND_MIXER_SCHN_REAR_RIGHT,
***	// Front center //
***	SND_MIXER_SCHN_FRONT_CENTER,
***	// Woofer //
***	SND_MIXER_SCHN_WOOFER,
***	// Side Left //
***	SND_MIXER_SCHN_SIDE_LEFT,
***	// Side Right //
***	SND_MIXER_SCHN_SIDE_RIGHT,
***	// Rear Center //
***	SND_MIXER_SCHN_REAR_CENTER,
***	SND_MIXER_SCHN_LAST = 31,
***	// Mono (Front left alias) //
***	SND_MIXER_SCHN_MONO = SND_MIXER_SCHN_FRONT_LEFT
***} snd_mixer_selem_channel_id_t;
***

** Output Param : 
** Return Value : success:0  other:fail
**************************************************************/
int aml_mixer_setvolume(const char *mixer_controler, int vol_percent, int snd_chnl_id);

/************************************************************
** FunctionName : aml_mixer_setvolume
** Description  : 设置指定mixer的音量
** Input Param  : 
				  const char *mixer_controler：mixer名字 （可通过 amixer controls | grep "iface=MIXER" 查得）
				  int snd_chnl_id

*** see alsa-lib-x.x.x\include\mixer.h
**** Mixer simple element channel identifier 
***typedef enum _snd_mixer_selem_channel_id {
***	// Unknown //
***	SND_MIXER_SCHN_UNKNOWN = -1,
***	// Front left //
***	SND_MIXER_SCHN_FRONT_LEFT = 0,
***	// Front right //
***	SND_MIXER_SCHN_FRONT_RIGHT,
***	// Rear left //
***	SND_MIXER_SCHN_REAR_LEFT,
***	// Rear right //
***	SND_MIXER_SCHN_REAR_RIGHT,
***	// Front center //
***	SND_MIXER_SCHN_FRONT_CENTER,
***	// Woofer //
***	SND_MIXER_SCHN_WOOFER,
***	// Side Left //
***	SND_MIXER_SCHN_SIDE_LEFT,
***	// Side Right //
***	SND_MIXER_SCHN_SIDE_RIGHT,
***	// Rear Center //
***	SND_MIXER_SCHN_REAR_CENTER,
***	SND_MIXER_SCHN_LAST = 31,
***	// Mono (Front left alias) //
***	SND_MIXER_SCHN_MONO = SND_MIXER_SCHN_FRONT_LEFT
***} snd_mixer_selem_channel_id_t;
***
	  	
** Output Param : 
** Return Value : success:0~100  other: -1
**************************************************************/
int aml_mixer_getvolume(const char *mixer_controler, int snd_chnl_id);



/************************************************************
** FunctionName : aml_mixer_set_control_value
** Description  : 
				  相当于调用 amixer cset xxxx value， 
			      例如 
			         amixer cset numid=1,iface=PCM,name='TDMOUT_B GAIN Enable' 1
			         amixer cset numid=123,iface=PCM,name='TDMOUT_B GAIN CH0' 150
					aml_mixer_set_control_value("numid=123,iface=PCM,name='TDMOUT_B GAIN CH0'", "150");
** Input Param  : 
				  const char *ctrl_identifier: 例如 numid=123,iface=PCM,name='TDMOUT_B GAIN CH0'
				  onst char *value：
** Output Param : 
** Return Value : success:0  other:fail
**************************************************************/
int aml_mixer_set_control_value(const char *ctrl_identifier, const char *value);


#ifdef __cplusplus
}
#endif
#endif


