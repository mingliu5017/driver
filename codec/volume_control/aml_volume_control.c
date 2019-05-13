#define LOG_TAG "aml_volume"

#include <stdio.h>
#include <stdlib.h>

#include "alsa/asoundlib.h"

#include "aml_log.h"
#include "aml_volume_control.h"

int aml_mixer_setvolume(const char *mixer_controler, int vol_percent, int snd_chnl_id)
{
	int find = 0;
	long volMin = 0, volMax = 0, leftVal = 0, rightVal = 0;
	int err = -1;

	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;
	snd_mixer_elem_t *dst_elem = NULL;
	snd_mixer_selem_id_t *sid = NULL;

	snd_mixer_selem_id_alloca(&sid);

	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		LOG(LEVEL_FATAL, "snd_mixer_open Err\n");
		return -1;
	}

	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	elem = snd_mixer_first_elem(handle);
	while(elem) {
		const char *pstr_elem = snd_mixer_selem_get_name (elem);
		LOG(LEVEL_INFO, "elem_name: %s\n", pstr_elem);

		if ( strcmp( mixer_controler, pstr_elem) == 0) {
			find = 1;
			dst_elem = elem;
			//break;
		}
		elem = snd_mixer_elem_next(elem);
	}

	if (dst_elem == NULL) {
		LOG(LEVEL_ERROR, "snd_mixer_find_selem Err\n");
		snd_mixer_close(handle);
		handle = NULL;
		return -1;
	}

	LOG(LEVEL_INFO, "elem_name: %s\n", snd_mixer_selem_get_name (dst_elem) );

	snd_mixer_selem_get_capture_volume_range(dst_elem, &volMin, &volMax);
	LOG(LEVEL_INFO, "vol_range: %ld -- %ld\n", volMin, volMax);

	snd_mixer_handle_events(handle);
	snd_mixer_selem_get_playback_volume(dst_elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVal);
	snd_mixer_selem_get_playback_volume(dst_elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVal);
	LOG(LEVEL_INFO, "cur_vol: FRONT_LEFT=%ld, FRONT_RIGHT=%ld\n", leftVal, rightVal);

	long vol_set = volMin + vol_percent * (volMax - volMin) / 100.0f;
	if(snd_chnl_id == -1) {
		snd_mixer_selem_set_playback_volume_all(dst_elem, vol_set);
		LOG(LEVEL_INFO, "set all channel volume to :%ld\n", vol_set);
	} else {
		snd_mixer_selem_set_playback_volume(dst_elem, (snd_mixer_selem_channel_id_t)snd_chnl_id, vol_set);
		LOG(LEVEL_INFO, "set channel:%d volume to :%ld\n", snd_chnl_id, vol_set);
	}

	snd_mixer_close(handle);

	return 0;
}

int aml_mixer_getvolume(const char *mixer_controler, int snd_chnl_id)
{
	int find = 0;
	long volMin = 0, volMax = 0, value = 0;
	int err = -1;

	snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem = NULL;
	snd_mixer_elem_t *dst_elem = NULL;
	snd_mixer_selem_id_t *sid = NULL;

	snd_mixer_selem_id_alloca(&sid);

	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		LOG(LEVEL_FATAL, "snd_mixer_open Err\n");
		return -1;
	}

	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	elem = snd_mixer_first_elem(handle);
	while(elem) {
		const char *pstr_elem = snd_mixer_selem_get_name (elem);
		LOG(LEVEL_INFO, "elem_name: %s\n", pstr_elem);

		if ( strcmp( mixer_controler, pstr_elem) == 0) {
			find = 1;
			dst_elem = elem;
			//break;
		}
		elem = snd_mixer_elem_next(elem);
	}

	if (dst_elem == NULL) {
		LOG(LEVEL_ERROR, "snd_mixer_find_selem Err\n");
		snd_mixer_close(handle);
		handle = NULL;
		return -1;
	}

	LOG(LEVEL_INFO, "elem_name: %s\n", snd_mixer_selem_get_name(dst_elem) );

	snd_mixer_selem_get_capture_volume_range(dst_elem, &volMin, &volMax);
	LOG(LEVEL_INFO, "vol_range: %ld -- %ld\n", volMin, volMax);

	snd_mixer_handle_events(handle);
	snd_mixer_selem_get_playback_volume(dst_elem, (snd_mixer_selem_channel_id_t)snd_chnl_id, &value);

	int percent = (int)(100.0f * (value - volMin) / (volMax - volMin));

	//关闭混音器设备
	snd_mixer_close(handle);

	return percent;
}

static void show_control_id(snd_ctl_elem_id_t *id)
{
	char *str;

	str = snd_ctl_ascii_elem_id_get(id);
	if (str) {
		LOG(LEVEL_INFO, "%s", str);
	}
	free(str);
}


int aml_mixer_set_control_value(const char *ctrl_identifier, const char *value)
{
	int err = 0;
	char card[32] = "default";
	static snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	if (ctrl_identifier == NULL) {
		LOG(LEVEL_ERROR, "Specify a full control identifier: [[iface=<iface>,][name='name',][index=<index>,][device=<device>,][subdevice=<subdevice>]]|[numid=<numid>]\n");
		return -1;
	}

	if (snd_ctl_ascii_elem_id_parse(id, ctrl_identifier)) {
		LOG(LEVEL_ERROR, "Wrong control identifier: %s\n", ctrl_identifier);
		return -1;
	}

#if 0
	show_control_id(id);
	printf("\n");
#endif

	if (handle == NULL &&
	    (err = snd_ctl_open(&handle, card, 0)) < 0) {
		LOG(LEVEL_ERROR, "%s %d Control %s open error: %s\n", __func__, __LINE__, card, snd_strerror(err));
		return err;
	}

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		LOG(LEVEL_ERROR, "%s %d error:%d\n", __func__, __LINE__, snd_strerror(err));
		goto _exit_lable;
	}

	snd_ctl_elem_info_get_id(info, id);     /* FIXME: Remove it when hctl find works ok !!! */

	snd_ctl_elem_value_set_id(control, id);
	if ((err = snd_ctl_elem_read(handle, control)) < 0) {
		LOG(LEVEL_ERROR, "%s %d error:%d\n", __func__, __LINE__, snd_strerror(err));
		goto _exit_lable;
	}

	err = snd_ctl_ascii_value_parse(handle, control, info, value);
	if (err < 0) {
		LOG(LEVEL_ERROR, "%s %d error:%d\n", __func__, __LINE__, snd_strerror(err));
		goto _exit_lable;
	}

	if ((err = snd_ctl_elem_write(handle, control)) < 0) {
		LOG(LEVEL_ERROR, "%s %d error:%d\n", __func__, __LINE__, snd_strerror(err));
		goto _exit_lable;
	}

_exit_lable:
	snd_ctl_close(handle);
	handle = NULL;
	return err;
}

