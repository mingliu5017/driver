#ifndef _UART_LED_H
#define _UART_LED_H


#define UART_LED_VERSION  "1.0.1"

typedef enum  {
  LED_IDLE,
  LED_HOTWORDLISTENING,
  LED_THINKING,
  LED_RESPONDING,
  LED_MIC_MUTE,
  LED_CAST_READY_TO_SET_UP,
  LED_VERIFY_DEVICE,
  LED_CONNECTING_WIFI,
  LED_DOWNLOADING,
  LED_INSTALLING,
  LED_ALARM_RINGING,
  LED_REMINDER_NOTIFICATION,
  LED_VOLUME_MUTE,
  LED_UPDATE,
  LED_STATE_MAX
}LED_DISPLAY_STATE;

typedef enum  {
  FDR_START,
  FDR_PERSENT_33,
  FDR_PERSENT_66,
  FDR_COMPLETE,
  FDR_STATE_MAX
}LED_FDR_STATE;

int uart_open(int fd,const char *pathname);
int uart_set(int fd,int nSpeed,int nBits,char nEvent,int nStop);
int uart_close(int fd);
int display_led_state(int fd, LED_DISPLAY_STATE led_state);
int display_fdr_state(int fd, LED_FDR_STATE fdr_state);
int display_valume(int fd, int valume_level);

#endif
