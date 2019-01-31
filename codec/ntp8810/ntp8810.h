#ifndef _NTP8810_H
#define _NTP8810_H

#define MVOL                             0x04

#define NTP8810_REGISTER_COUNT		 			29
#define NTP8810_RAM_TABLE_COUNT        			50

#define NTP8810_SOUNDON_REG_COUNT		 		4
#define NTP8810_SOUNDOFF_REG_COUNT		 		4
struct ntp8810_platform_data {
	int reset_pin;
	//int power_pin;
	//int audio_in_pin1;
	//int audio_in_pin2;
	//int audio_in_pin3;
};

#endif
