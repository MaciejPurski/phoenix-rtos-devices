/*
 * Phoenix-RTOS
 *
 * STM32L1 multidriver
 *
 * Copyright 2018 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _STM32_MULTI_H_
#define _STM32_MULTI_H_

enum { adc_get = 0, rtc_get, rtc_set, lcd_get, lcd_set,
	i2c_def, i2c_get, i2c_set, gpio_def, gpio_get, gpio_set, gpio_seq, uart_def, uart_get, uart_set,
	flash_def, flash_get, flash_set };

/* RTC */


typedef struct {
	unsigned char hours;
	unsigned char minutes;
	unsigned char seconds;

	unsigned char day;
	unsigned char month;
	unsigned char wday;
	unsigned int year;
} __attribute__((packed)) rtctimestamp_t;


/* LCD */


#define LCDDRV_CHAR_DEGREE 0xf8


typedef enum {
	LCDSYM_SMALL_ONE    = 1 << 0,
	LCDSYM_CLOCK        = 1 << 1,
	LCDSYM_DATE         = 1 << 2,
	LCDSYM_WARN         = 1 << 3,
	LCDSYM_BATT         = 1 << 4,
	LCDSYM_LOWBATT      = 1 << 5,
	LCDSYM_NOBATT       = 1 << 6,
	LCDSYM_LOCK         = 1 << 7,
	LCDYSM_FACTORY      = 1 << 8,
	LCDSYM_FIRE         = 1 << 9,
	LCDSYM_COM          = 1 << 10,
	LCDSYM_POUND        = 1 << 11,
	LCDSYM_EUR          = 1 << 12,
	LCDSYM_M3           = 1 << 13,
	LCDSYM_SLASH_TOP    = 1 << 14,
	LCDSYM_H_TOP        = 1 << 15,
	LCDSYM_H_BOT        = 1 << 16,
	LCDSYM_K            = 1 << 17,
	LCDSYM_W            = 1 << 18,
	LCDSYM_FRAME        = 1 << 19,
	LCDSYM_FRAME_SMALL  = 1 << 20,
	LCDSYM_BAR0         = 1 << 21,
	LCDSYM_BAR1         = 1 << 22,
	LCDSYM_BAR2         = 1 << 23,
	LCDSYM_DOT1_BOT     = 1 << 24,
	LCDSYM_DOT2_TOP     = 1 << 25,
	LCDSYM_DOT2_BOT     = 1 << 26,
	LCDSYM_DOT3_BOT     = 1 << 27,
	LCDSYM_DOT4_TOP     = 1 << 28,
	LCDSYM_DOT4_BOT     = 1 << 29,
	LCDSYM_TOTAL        = 30
} lcd_symbols_t;


typedef struct {
	char str[10];
	char str_small[2];
	unsigned int sym_mask;
	unsigned char state;
	int backlight;
	int on;
} __attribute__((packed)) lcdmsg_t;


/* I2C */


typedef struct {
	char addr;
	char reg;
} __attribute__((packed)) i2cmsg_t;


/* GPIO */


enum { gpioa = 0, gpiob, gpioc, gpiod, gpioe, gpiof, gpiog, gpioh };


typedef struct {
	int port;
} __attribute__((packed)) gpioget_t;


typedef struct {
	int port;
	int mask;
	int state;
} __attribute__((packed)) gpioset_t;


typedef struct {
	int port;
	char pin;
	char mode;
	char af;
	char otype;
	char ospeed;
	char pupd;
} __attribute__((packed)) gpiodef_t;


typedef struct {
	int type;

	union {
		gpioset_t set;
		gpioget_t get;
		gpiodef_t def;
	};
} __attribute__((packed)) gpioseq_t;


/* UART */


enum { usart1 = 0, usart2, usart3, uart4, uart5 };


enum { uart_mnormal = 0, uart_mnblock };


enum { uart_parnone = 0, uart_pareven, uart_parodd };


typedef struct {
	int uart;
	int mode;
} __attribute__((packed)) uartget_t;


typedef struct {
	int uart;
} __attribute__((packed)) uartset_t;


typedef struct {
	int uart;
	unsigned int baud;
	char enable;
	char bits;
	char parity;
} __attribute__((packed)) uartdef_t;


/* MULTI */


typedef struct {
	int type;

	union {
		int adc_channel;
		rtctimestamp_t rtc_timestamp;
		lcdmsg_t lcd_msg;
		i2cmsg_t i2c_msg;
		gpiodef_t gpio_def;
		gpioget_t gpio_get;
		gpioset_t gpio_set;
		uartget_t uart_get;
		uartset_t uart_set;
		uartdef_t uart_def;
	};
} __attribute__((packed)) multi_i_t;


typedef struct {
	int err;

	union {
		unsigned short adc_val;
		rtctimestamp_t rtc_timestamp;
		lcdmsg_t lcd_msg;
		unsigned int gpio_get;
	};
} __attribute__((packed)) multi_o_t;


#endif