/*
 * Phoenix-RTOS
 *
 * IMX6ULL NOR Flash driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Maciej Purski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _IMX6ULL_FLASHNOR_H_
#define _IMX6ULL_FLASHNOR_H_

/* Micron N25Q256A commands */
#define FLASH_ENABLE				0x66
#define FLASH_MEMORY				0x99

#define FLASH_READ_ID				0x9F
#define FLASH_MULTIPLE_IO_READ_ID	0xAF
#define FLASH_READ_SERIAL_DISC_PAR	0x5A

#define FLASH_READ					0x03
#define FLASH_FAST_READ				0x0B
#define FLASH_4BYTE_READ			0x13

#define FLASH_READ_STATUS_REGISTER	0x5
#define FLASH_READ_NONVOLATILE_REG 	0xB5

#define FLASH_WRITE_ENABLE			0x06

#define FLASH_SECTOR_ERASE			0xD8

int flashnor_init();

#endif
