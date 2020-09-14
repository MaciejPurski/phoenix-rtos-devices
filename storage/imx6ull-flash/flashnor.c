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

#include <stdint.h>
#include <stdio.h>

#include "qspi.h"
#include "flashnor.h"


int flashnor_init(void)
{
	uint32_t cmd[4];
	unsigned i;

	if (qspi_init() != 0) {
		printf("flashnor: Can't init QSPI!\n");
		return -1;
	}
	/* We begin in Extended SPI mode, so use only one data line */
	cmd[0] = LUT_INSTR(QSPI_CMD, LUT_PAD1, FLASH_READ_ID, QSPI_READ, LUT_PAD1, 0x20);
	cmd[1] = 0;
	cmd[2] = 0;
	cmd[3] = 0;
	qspi_lutSequenceWrite(0, cmd);
	qspi_issueIPCommand(0);
	qspi_flashRead(0x20);

	return 0;
}