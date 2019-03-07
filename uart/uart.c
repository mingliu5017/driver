#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>

#include "./uart.h"

/*
 * 安全读写函数
 */

ssize_t safe_write(int fd, const void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;

    while(nleft > 0)
    {
    if((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if(nwritten < 0&&errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}

ssize_t safe_read(int fd,void *vptr,size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr=vptr;
    nleft=n;

    while(nleft > 0)
    {
        if((nread = read(fd,ptr,nleft)) < 0)
        {
            if(errno == EINTR)//被信号中断
                nread = 0;
            else
                return -1;
        }
        else
        if(nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
    return (n-nleft);
}

int uart_open(int fd,const char *pathname)
{
    assert(pathname);

    /*打开串口*/
    fd = open(pathname,O_RDWR|O_NOCTTY|O_NDELAY);
    if(fd == -1)
    {
        perror("Open UART failed!");
        return -1;
    }

    /*清除串口非阻塞标志*/
    if(fcntl(fd,F_SETFL,0) < 0)
    {
        fprintf(stderr,"fcntl failed!\n");
        return -1;
    }

    return fd;
}

int uart_set(int fd,int nSpeed,int nBits,char nEvent,int nStop) 
{ 
	struct termios newttys1,oldttys1; 
	
    //printf("UART Speed:%d,bits:%d,Event:%s,stop:%d\n",nSpeed,nBits,nEvent,nStop);
	/*保存原有串口配置*/ 
	if(tcgetattr(fd,&oldttys1)!=0) 
	{ 
		perror("Setupserial 1"); 
		return -1; 
	} 
	bzero(&newttys1,sizeof(newttys1));
	newttys1.c_cflag|=(CLOCAL|CREAD ); 
	/*CREAD 开启串行数据接收，CLOCAL并打开本地连接模式*/ 
	newttys1.c_cflag &=~CSIZE;/*设置数据位*/ 
		
	/*数据位选择*/
	switch(nBits) 
	{ 
		case 7: 
			newttys1.c_cflag |=CS7; 
			break; 
		case 8:
			newttys1.c_cflag |=CS8;
			break; 
	} 
	/*设置奇偶校验位*/ 
	switch( nEvent ) 
	{ 
		case '0': /*奇校验*/ 
			newttys1.c_cflag |= PARENB;/*开启奇偶校验*/ 
			newttys1.c_iflag |= (INPCK | ISTRIP);/*INPCK打开输入奇偶校验；ISTRIP去除字符的第八个比特  */ 
			newttys1.c_cflag |= PARODD;/*启用奇校验(默认为偶校验)*/ 
		    break;
		case 'E':/*偶校验*/ 
			newttys1.c_cflag |= PARENB; /*开启奇偶校验  */ 
			newttys1.c_iflag |= ( INPCK | ISTRIP);/*打开输入奇偶校验并去除字符第八个比特*/
			newttys1.c_cflag &= ~PARODD;/*启用偶校验*/ 
			break; 
		case 'N': /*无奇偶校验*/ 
			newttys1.c_cflag &= ~PARENB; 
			break; 
	} 
	/*设置波特率*/ 
	switch( nSpeed ) 
	{ 
		case 2400: 
			cfsetispeed(&newttys1, B2400); 
			cfsetospeed(&newttys1, B2400); 
			break;
		case 4800: 
			cfsetispeed(&newttys1, B4800); 
			cfsetospeed(&newttys1, B4800); 
			break; 
		case 9600: 
			cfsetispeed(&newttys1, B9600);
			cfsetospeed(&newttys1, B9600); 
			break;
		case 115200: 
			cfsetispeed(&newttys1, B115200); 
			cfsetospeed(&newttys1, B115200); 
			break; 
		default: 
			cfsetispeed(&newttys1, B115200); 
			cfsetospeed(&newttys1, B115200); 
			break; 
	} 
	/*设置停止位*/ 
	if( nStop == 1)/*设置停止位；若停止位为1，则清除CSTOPB，若停止位为2，则激活CSTOPB*/ 
	{ 
		newttys1.c_cflag &= ~CSTOPB;/*默认为一位停止位； */ 
	} 
	else if( nStop == 2) 
	{ 
		newttys1.c_cflag |= CSTOPB;/*CSTOPB表示送两位停止位*/ 
	} 
	/*设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时*/
	newttys1.c_cc[VTIME] = 0;/*非规范模式读取时的超时时间；*/ 
	newttys1.c_cc[VMIN] = 0; /*非规范模式读取时的最小字符数*/ 
	tcflush(fd ,TCIFLUSH);/*tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 */ 
	/*激活配置使其生效*/ 
	if((tcsetattr( fd, TCSANOW,&newttys1))!=0) 
	{ 
		perror("com set error"); 
		return -1; 
	} 
	return 0; 
}


int uart_read(int fd,char *r_buf,size_t len)
{
    ssize_t cnt = 0;
    fd_set rfds;
    struct timeval time;
	int ret;

    /*将文件描述符加入读描述符集合*/
    FD_ZERO(&rfds);
    FD_SET(fd,&rfds);

    /*设置超时为15s*/
    time.tv_sec = 15;
    time.tv_usec = 0;

    /*实现串口的多路I/O*/
    ret = select(fd+1,&rfds,NULL,NULL,&time);
    switch(ret)
    {
        case -1:
            fprintf(stderr,"select error!\n");
            return -1;
        case 0:
            fprintf(stderr,"time over!\n");
            return -1;
        default:
            cnt = safe_read(fd,r_buf,len);
            if(cnt == -1)
            {
                fprintf(stderr,"read error!\n");
                return -1;
            }
            return cnt;
    }
}

int uart_write(int fd,const char *w_buf,size_t len)
{
    ssize_t cnt = 0;

    cnt = safe_write(fd,w_buf,len);
    if(cnt == -1)
    {
        fprintf(stderr,"write error!\n");
        return -1;
    }

    return cnt;
}

int uart_close(int fd)
{
    assert(fd);
    close(fd);

    /*可以在这里做些清理工作*/

    return 0;
}

int display_led_state(int fd, LED_DISPLAY_STATE led_state)
{
    char w_buf[128] ={0};
    size_t w_len = 0;
	int ret;

	switch(led_state)
	{
		case LED_IDLE :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+IDLE");
			break;

		case LED_HOTWORDLISTENING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+HOTWORDLISTENING");
			break;

		case LED_THINKING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+THINKING");
			break;

		case LED_RESPONDING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+RESPONDING");
			break;

		case LED_MIC_MUTE :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+MIC MUTE");
			break;

		case LED_CAST_READY_TO_SET_UP :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+CAST READY TO SET UP");
			break;
		
		case LED_VERIFY_DEVICE :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+VERIFY DEVICE");
			break;

		case LED_CONNECTING_WIFI :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+CONNECTING WIFI");
			break;
		
		case LED_DOWNLOADING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+DOWNLOADING");
			break;
		
		case LED_INSTALLING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+INSTALLING");
			break;
		
		case LED_ALARM_RINGING :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+ALARM RINGING");
			break;
		
		case LED_REMINDER_NOTIFICATION :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+REMINDER NOTIFICATION");
			break;

		case LED_VOLUME_MUTE :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+VOLUME MUTE");
			break;

		case LED_UPDATE :
			snprintf(w_buf, sizeof(w_buf), "%s", "AT+UPDATE");
			break;

		default: 
			break;
	}

	w_len = strlen(w_buf)+1;
	printf("common:%s,len:%d\n", w_buf,w_len);
    ret = uart_write(fd,w_buf,w_len);
    if(ret == -1)
    {
        fprintf(stderr,"uart write failed!\n");
        return -1;
    }

    return ret;
}

int display_fdr_state(int fd, LED_FDR_STATE fdr_state)
{
    char w_buf[128] ={0};
    size_t w_len = 0;
	int ret;

	switch(fdr_state)
	{
		case FDR_START :
			snprintf(w_buf, sizeof(w_buf), "AT+FDR %d", FDR_START+1);
			break;

		case FDR_PERSENT_33 :
			snprintf(w_buf, sizeof(w_buf), "AT+FDR %d", FDR_PERSENT_33+1);
			break;

		case FDR_PERSENT_66 :
			snprintf(w_buf, sizeof(w_buf), "AT+FDR %d", FDR_PERSENT_66+1);
			break;

		case FDR_COMPLETE :
			snprintf(w_buf, sizeof(w_buf), "AT+FDR %d", FDR_COMPLETE+1);
			break;

		default: 
			break;
	}

	w_len = strlen(w_buf)+1;
	printf("common:%s,len:%d\n", w_buf,w_len);
    ret = uart_write(fd,w_buf,w_len);
    if(ret == -1)
    {
        fprintf(stderr,"uart write failed!\n");
        return -1;
    }

    return ret;
}

int display_valume(int fd, int valume_level)
{
    char w_buf[128] ={0};
    size_t w_len = 0;
	int ret;

	if(valume_level > 10)
	{
       return -1;
	}

	if(0 == valume_level)
	{
	    display_led_state(fd, LED_VOLUME_MUTE);
		return 0;
	}
	else
	{
	    snprintf(w_buf, sizeof(w_buf), "AT+VOLUME %d", valume_level);
	}

	w_len = strlen(w_buf)+1;
	printf("common:%s,len:%d\n", w_buf,w_len);
    ret = uart_write(fd,w_buf,w_len);
    if(ret == -1)
    {
        fprintf(stderr,"uart write failed!\n");
        return -1;
    }

    return ret;
}

int main(int argc, char** argv) 
{
	int fd = 0;
	int ret = 0;
	int opt = 0;
	const char *opts = "HVabcdefghijklmnops";
	const struct option longopts[] = {
			{ "help", no_argument, NULL, 'h' },
			{ "version", no_argument, NULL, 'V' },
			{ "fdr", 1, NULL, 'o' },
			{ "valume", 1, NULL, 'p' },
			{ "state", 1, NULL, 's' },
			{ 0, 0, 0, 0 },
	};
			
	fd = uart_open(fd,"/dev/ttyS2");/*串口号/dev/ttySn*/
	if(fd == -1)
	{
	      fprintf(stderr,"uart_open error\n");
	      exit(EXIT_FAILURE);
	}

	if(uart_set(fd,9600,8,'N',1) == -1)
	{
	  fprintf(stderr,"uart set failed!\n");
	  exit(EXIT_FAILURE);
	}

	/* parse options */
	optind = 0; opterr = 1;
	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1)
		switch (opt) {

		case 'H' /* --help */ :
			printf("Usage:\n"
					"  %s [OPTION]...\n"
					"\nOptions:\n"
					"  -h, --help\t\tprint this help and exit\n"
					"  -V, --version\t\tprint version and exit\n",
	        		argv[0]);
			        return EXIT_SUCCESS;
		
		case 'V' /* --version */ :
				printf("%s\n", UART_LED_VERSION);
				return EXIT_SUCCESS;

		case 'a' :
			    display_led_state(fd, LED_IDLE);
				break;
		
		case 'b' :
			    display_led_state(fd, LED_HOTWORDLISTENING);
				break;
		
		case 'c' :
			    display_led_state(fd, LED_THINKING);
				break;

		case 'd' :
			    display_led_state(fd, LED_RESPONDING);
				break;

		case 'e' :
			    display_led_state(fd, LED_MIC_MUTE);
				break;

		case 'f' :
			    display_led_state(fd, LED_CAST_READY_TO_SET_UP);
				break;

		case 'g' :
			    display_led_state(fd, LED_VERIFY_DEVICE);
			    break;

		case 'h' :
				display_led_state(fd, LED_CONNECTING_WIFI);
				break;

		case 'i' :
				display_led_state(fd, LED_DOWNLOADING);
				break;
		case 'j' :
				display_led_state(fd, LED_INSTALLING);
				break;
		case 'k' :
				display_led_state(fd, LED_ALARM_RINGING);
				break;
		case 'l' :
				display_led_state(fd, LED_REMINDER_NOTIFICATION);
				break;
		case 'm' :
				display_led_state(fd, LED_VOLUME_MUTE);
				break;
		case 'n' :
				display_led_state(fd, LED_UPDATE);
				break;

		case 'o' :
				display_fdr_state(fd, atoi(optarg));
				break;
		case 'p' :
				display_valume(fd, atoi(optarg));
				break;

		case 's' :
				display_led_state(fd, atoi(optarg));
				break;

		default:
				fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
				return EXIT_FAILURE;
	}

	ret = uart_close(fd);
    if(ret == -1)
    {
        fprintf(stderr,"uart close error\n");
        exit(EXIT_FAILURE);
    }
	return EXIT_SUCCESS;
}

