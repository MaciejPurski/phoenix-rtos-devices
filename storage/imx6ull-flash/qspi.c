/*
 * Phoenix-RTOS
 *
 * IMX6ULL Quad SPI driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Maciej Purski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <string.h>
#include <stdio.h>

#include <sys/mman.h>
#include <sys/threads.h>
#include <sys/platform.h>

#include <phoenix/arch/imx6ull.h>

#include "qspi.h"


/* QSPI Peripheral Bus registers */
enum {
	qspi_mcr = 0, qspi_ipcr = 2, qspi_flshcr, qspi_buf0cr, qspi_buf1cr, qspi_buf2cr,
	qspi_buf3cr, qspi_bfgencr, qspi_buf0ind = 12, qspi_buf1ind, qspi_buf2ind,
	qspi_sfar = 64, qspi_smpr = 66, qspi_rbsr, qspi_rbct,
	qspi_tbsr = 84, qspi_tbdr, qspi_sr = 87, qspi_fr, qspi_rser,
	qspi_spndst, qspi_sptrclr, qspi_sfa1ad = 96, qspi_sfa2ad, qspi_sfb1ad,
	qspi_sfb2ad, qspi_rbdr = 128, qspi_lutkey = 192, qspi_lckcr,
	qspi_lut = 196
};

struct {
	volatile uint32_t *base;
	volatile uint32_t *ahb;

} qspi_common;


static void qspi_enable(int enable)
{
	/* Enable / disable QuadSPI clocks */
	if (enable)
		*(qspi_common.base + qspi_mcr) &= ~QSPI_MCR_MDIS;
	else
		*(qspi_common.base + qspi_mcr) |= QSPI_MCR_MDIS;
}


static void qspi_reset(void)
{
	unsigned i;

	/* Software reset AHB and Serial Flash domain */
	*(qspi_common.base + qspi_mcr) |= QSPI_MCR_SWRSTHD | QSPI_MCR_SWRSTSD;

	for (i = 0; i < 100; i++) {}

	qspi_enable(0);
	*(qspi_common.base + qspi_mcr) &= ~(QSPI_MCR_SWRSTHD | QSPI_MCR_SWRSTSD);
	qspi_enable(1);
}

void qspi_lutSequenceWrite(uint32_t index, uint32_t *cmd)
{
	printf("LUT before %x\n", *(qspi_common.base + qspi_lut));
	/* Unlock LUT */
	*(qspi_common.base + qspi_lutkey) = QSPI_LUT_KEY;
	*(qspi_common.base + qspi_lckcr) = 2;

	memcpy(qspi_common.base + qspi_lut + 4 * index, cmd, 4 * sizeof(*cmd));

	/* Lock LUT */
	*(qspi_common.base + qspi_lutkey) = QSPI_LUT_KEY;
	*(qspi_common.base + qspi_lckcr) = 1;

	printf("LUT after %x\n", *(qspi_common.base + qspi_lut));
}



void qspi_issueIPCommand(unsigned seqId)
{
	printf("SR: %x FR: %x\n", *(qspi_common.base + qspi_sr), *(qspi_common.base + qspi_fr));
	/* Wait for last command to be finished */
	while (*(qspi_common.base + qspi_sr) & QSPI_SR_BUSY) { }

	/* Set flash device address only when no other is provided */
	*(qspi_common.base + qspi_sfar) = 0x60000000;

	/* Clear the RX pointer */
	*(qspi_common.base + qspi_mcr) |= QSPI_MCR_CLR_RXF;

	/* Clear sequence pointer before starting a new sequence */
	*(qspi_common.base + qspi_sptrclr) |= QSPI_SPTRCLR_IPPTRC;

	/* Set sequence ID and data size, starts the sequence */
	*(qspi_common.base + qspi_ipcr) = *(qspi_common.base + qspi_ipcr) & ~(QSPI_IPCR_SEQID | QSPI_IPCR_IDATSZ)
										| (seqId & 0xF << 24);
	printf("SR: %x FR: %x\n", *(qspi_common.base + qspi_sr), *(qspi_common.base + qspi_fr));
	while (*(qspi_common.base + qspi_sr) & QSPI_SR_BUSY) { printf("WAIT\n");}
	printf("SR: %x FR: %x\n", *(qspi_common.base + qspi_sr), *(qspi_common.base + qspi_fr));
}

uint32_t qspi_dumpRX()
{
	unsigned i;
	volatile uint32_t *buf = (qspi_common.base + qspi_rbdr);
	uint32_t fill = (*(qspi_common.base + qspi_rbsr) >> 8) & 0x3F;
	uint32_t rbsr = *(qspi_common.base + qspi_rbsr);

	printf("rbct: %x\n", *(qspi_common.base + qspi_rbct));
	printf("RX fill: %d status: %x\n", fill, rbsr);
	while ((*(qspi_common.base + qspi_sr) & QSPI_SR_RXWE) == 0) { }
	for (i = 0; i < 32; i++)
		printf("%x ", buf[i]);
	printf("\n-------\n");
	return fill;
}


int qspi_programFlash(unsigned seqId, uint32_t address, uint32_t *data, size_t datasz)
{
	size_t written = 0;

	/* At least 4 words of data must be written */
	if (datasz < 4 * sizeof(*data))
		return 0;

	/* If the buffer is not empty, clear it */
	if (*(qspi_common.base + qspi_sr) & QSPI_SR_TXEDA)
		*(qspi_common.base + qspi_mcr) |= QSPI_MCR_CLR_TXF;

	/* Program the address */
	*(qspi_common.base + qspi_sfar) = address;
	
	while (written < QSPI_TX_SIZE && written < datasz) {
		/* TODO: if zamiast while? */
		uint32_t tbsr = *(qspi_common.base + qspi_tbsr);
		printf("Transmit Counter: %d TX Buffer fill level: %d\n", (tbsr & 0xFFFF0000) >> 16, (tbsr & 0xF00) >> 8);
		while (*(qspi_common.base + qspi_sr) & QSPI_SR_TXFULL) {}

		*(qspi_common.base + qspi_tbdr) = data[written++];
	}

	qspi_issueIPCommand(seqId);

	return written;
}

int qspi_flashRead(size_t sz)
{
	size_t read = 0;

	while (read < sz) {
		/* Check if data is ready - ammount of data is equal to RX watermark */
		while ((*(qspi_common.base + qspi_sr) & QSPI_SR_RXWE) == 0) { }

		/* Read RX */
		printf("%x ", *(qspi_common.base + qspi_rbdr));
		read += 4;
		/* POP number of bytes equal to RX watermark */
		*(qspi_common.base + qspi_fr) |= QSPI_FR_RBDF;
	}
	printf("\n READ FINISH\n");
}

int qspi_init(void)
{
	int res;

	qspi_common.base = mmap(NULL, SIZE_PAGE, PROT_READ | PROT_WRITE, MAP_DEVICE, OID_PHYSMEM, QSPI_BASE_ADDR);
	if (qspi_common.base == MAP_FAILED) {
		printf("qspi: Could not map IP base address!\n");
		return -1;
	}

	qspi_reset();

	/* Clear Rx and Tx buffers */
	*(qspi_common.base + qspi_mcr) |= QSPI_MCR_CLR_RXF | QSPI_MCR_CLR_TXF;

	qspi_enable(0);

	/* Enable QSPI clock */
	platformctl_t pctl = { 0 };
	pctl.action = pctl_set;
	pctl.type = pctl_devclock;
	pctl.devclock.dev = pctl_clk_qspi;
	pctl.devclock.state = 1;
	if ((res = platformctl(&pctl)) != 0) {
		printf("qspi: Could not enable clock!\n");
		return -1;
	}

	/* Todo set AHB buffers */

	/* TODO: Set watermark for Rx and Tx dma */
	/* TODO: config */

	/* Set access to RX registers via IP, set watermark to 0, which is equivalent to 4 bytes */
	*(qspi_common.base + qspi_rbct) |= 1 << 8;

	/* Set Flash config register */
	*(qspi_common.base + qspi_flshcr) = 4 << 8 | 4;

	qspi_enable(1);

	return 0;
}