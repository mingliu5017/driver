#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include <fcntl.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "./lcd.h"

typedef unsigned char BYTE;
#define LCD_TEST_VERSION	"1.0.0"

#define ENABLE_LEDCTRL_LOG  1

#if ENABLE_LEDCTRL_LOG
#define LOG(fmt, args...)  printf("[%s,#%d]" fmt "\n",  __FILE__, __LINE__, ##args)
#else
#define LOG(fmt, args...)
#endif

#define ARRAY_SIZE(array)  (sizeof(array)/sizeof(array[0]))

#define START_COL_OFFSET 4
#define START_ROW_OFFSET 0
/* 液晶128*64 */
#define LCD_WIDTH 128
#define LCD_HEIGHT 64
unsigned char number[32]="1328787801312345";

void lcd_set_column(int fd, unsigned char col)
{
	unsigned char lsb;
	unsigned int value=0;

	lsb = (col & 0xF);
	col = (col >> 4) & 0xF;

	// send  msb
	value = col | 0x10;
	ioctl(fd, GPIO_LCD_WRITE_CMD, &value);
	// send  lsb
	value = lsb;
	ioctl(fd, GPIO_LCD_WRITE_CMD, &value);
}

void lcd_set_page(int fd, unsigned char page)
{
	unsigned char cmd;
	unsigned int value=0;

	cmd = 0xB0 + (page & 0xF);
	value = cmd;
	ioctl(fd, GPIO_LCD_WRITE_CMD, &value);
}

void lcd_write_data(int fd, unsigned char value)
{
	unsigned int data=value;

	ioctl(fd, GPIO_LCD_WRITE_DATA, &data);
}

void lcd_write_CMD(int fd, unsigned char cmd)
{
	unsigned int data=cmd;

	ioctl(fd, GPIO_LCD_WRITE_CMD, &data);
}

void lcd_clean()
{
	int fd;
	unsigned int value=0;
	unsigned int n,m;

	fd = open("/dev/kelcd", 777);
	if (fd < 0) {
		perror("open");
		exit(-2);
	}
	
	for(n=0; n<9; n++)
	{
		lcd_set_page(fd,n);

		lcd_set_column(fd,0);

		for(m=0; m < 128+4; m++)
		{
			lcd_write_data(fd,value);
		}
	}
	close(fd);
}

void lcd_show_picture(const char *pLcdbuf)
{
	int fd;
	unsigned char line, col, data;

	fd = open("/dev/kelcd", 777);
	if (fd < 0) {
		perror("open");
		exit(-2);
	}

	for(line = 0; line < 8; line++)
	{
		// set display start line
		lcd_write_CMD(fd,0);
		// set page
		lcd_set_page(fd,line);

		// set start col
		lcd_set_column(fd, 0);

		for (col = 0; col < LCD_WIDTH; col++)
		{
		    data = *pLcdbuf;
		    lcd_write_data(fd,data);
			pLcdbuf++;
		}
	}
	close(fd);
}

void lcd_show_8_8_char(unsigned char line, unsigned char col, const char *pLcdbuf)
{
	int fd;
	unsigned char tmp=0,data=0;

	fd = open("/dev/kelcd", 777);
	if (fd < 0) {
		perror("open");
		exit(-2);
	}
	// set display start line
	lcd_write_CMD(fd,0);
	// set page
	lcd_set_page(fd,line);

	// set start col
	lcd_set_column(fd, col);

	for (tmp = 0; tmp < 8; tmp++)
	{
	    data = *pLcdbuf;
	    lcd_write_data(fd,data);
		pLcdbuf++;
	}

	close(fd);
}

void lcd_show_8_16_char(unsigned char line, unsigned char col, const char *pLcdbuf)
{
	int fd;
	unsigned char tmp=0,data=0;

	if(line > 3 || col > 15)
		return;
	
	fd = open("/dev/kelcd", 777);
	if (fd < 0) {
		perror("open");
		exit(-2);
	}
	lcd_write_CMD(fd,0XA0);
	lcd_write_CMD(fd,0XC0);
	line = 6 -(line*2);
	// set page
	lcd_set_page(fd,line+1);

	// set start col
	lcd_set_column(fd, col*8);

	for (tmp = 0; tmp < 8; tmp++)
	{
	    data = *pLcdbuf;
	    lcd_write_data(fd,data);
		pLcdbuf++;
	}

	// set page
	lcd_set_page(fd,line);
	
	// set start col
	lcd_set_column(fd, col*8);

	for (tmp = 0; tmp < 8; tmp++)
	{
		data = *pLcdbuf;
		lcd_write_data(fd,data);
		pLcdbuf++;
	}

	close(fd);
}

void lcd_print(unsigned char line, unsigned char col, const char *pchar, unsigned char len)
{
	unsigned char i=0,index=0,tmp_line=0,tmp_col=0;

	for (i = 0; i < len; i++)
	{
	    index = *pchar - 32;
		lcd_show_8_16_char(line,col,ASCII_CODE[index]);
	}
}


int main(int argc, char** argv) 
{
	unsigned char tmp=0;

	int opt;
	const char *opts = "hVWRGBCSK";
	const struct option longopts[] = {
			{ "help", no_argument, NULL, 'h' },
			{ "version", no_argument, NULL, 'V' },
			{ "version", no_argument, NULL, 'W' },
			{ 0, 0, 0, 0 },
	};

	/* parse options */
	optind = 0; opterr = 1;
	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1)
		switch (opt) {

		case 'h' /* --help */ :
			printf("Usage:\n"
					"  %s [OPTION]...\n"
					"\nOptions:\n"
					"  -h, --help\t\tprint this help and exit\n"
					"  -V, --version\t\tprint version and exit\n"
					"  -W, --led all white and exit\n"
					"  -R, --led all red and exit\n"
					"  -G, --led all green and exit\n"
					"  -B, --led all blue and exit\n"
					"  -C, --led all off and exit\n",
            		argv[0]);
			        return EXIT_SUCCESS;
		
		case 'V' /* --version */ :
				printf("%s\n", LCD_TEST_VERSION);
				return EXIT_SUCCESS;

		case 'W' :
				lcd_clean();
				printf("lcd_clean\n");
				break;

		case 'R' :
				printf("lcd_show_picture.\n");
				lcd_show_picture(RES_ICON_TIETONG_LOGO);
				break;

		case 'G' :
			    printf("lcd show 8 * 8 char.\n");
				break;

		case 'B' :
				printf("lcd show 8 * 16 char.\n");
				
#if 0
			    for (tmp = 0; tmp < 16; tmp++)
	            {
				    lcd_show_8_16_char(0,tmp,ASCII_CODE[tmp]);
				}
			    for (tmp = 16; tmp < 32; tmp++)
	            {
				    lcd_show_8_16_char(1,tmp-16,ASCII_CODE[tmp]);
				}
			    for (tmp = 32; tmp < 48; tmp++)
	            {
				    lcd_show_8_16_char(2,tmp-32,ASCII_CODE[tmp]);
				}
			    for (tmp = 48; tmp < 64; tmp++)
	            {
				    lcd_show_8_16_char(3,tmp-48,ASCII_CODE[tmp]);
				}
#endif
				break;

		default:
				fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
				return EXIT_FAILURE;
		}

	return EXIT_SUCCESS;
}

