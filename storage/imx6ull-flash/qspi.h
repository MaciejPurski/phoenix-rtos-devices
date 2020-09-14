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
#ifndef _IMX6ULL_QSPI_H_
#define _IMX6ULL_QSPI_H_

#include <stdint.h>

#define QSPI_BASE_ADDR	0x21E0000
#define QSPI_LUT_SIZE	64
#define QSPI_RBDR_SIZE	32 

/* QSPI Module Configuration Register fields */
#define QSPI_MCR_SWRSTSD			(1 << 0)
#define QSPI_MCR_SWRSTHD			(1 << 1)
#define QSPI_MCR_END_CFG			(3 << 2)
#define QSPI_MCR_DQS_EN				(1 << 6)
#define QSPI_MCR_DDR_EN				(1 << 7)
#define QSPI_MCR_CLR_RXF			(1 << 10)
#define QSPI_MCR_CLR_TXF			(1 << 11)
#define QSPI_MCR_MDIS				(1 << 14)
#define QSPI_MCR_DQS_LOOPBACK_EN	(1 << 24)
#define QSPI_MCR_DQS_PHASE_EN		(1 << 30)

/* QSPI IP Configuration Register fields */
#define QSPI_IPCR_IDATSZ			0xFF
#define QSPI_IPCR_PAR_EN			(1 << 16)
#define QSPI_IPCR_SEQID				(0xF << 24)



/* QSPI Status Register fields */
#define QSPI_SR_BUSY				(1 << 0)
#define QSPI_SR_IP_ACC				(1 << 1)
#define QSPI_SR_AHB_ACC				(1 << 2)
#define QSPI_SR_AHBGNT				(1 << 5)
#define QSPI_SR_AHBTRN				(1 << 6)
#define QSPI_SR_AHB0NE				(1 << 7)
#define QSPI_SR_AHB1NE				(1 << 8)
#define QSPI_SR_AHB2NE				(1 << 9)
#define QSPI_SR_AHB3NE				(1 << 10)
#define QSPI_SR_AHB0FUL				(1 << 11)
#define QSPI_SR_AHB1FUL				(1 << 12)
#define QSPI_SR_AHB2FUL				(1 << 13)
#define QSPI_SR_AHB3FUL				(1 << 14)
#define QSPI_SR_RXWE				(1 << 16)
#define QSPI_SR_RXFULL				(1 << 19)
#define QSPI_SR_RXDMA				(1 << 23)
#define QSPI_SR_TXEDA				(1 << 24)
#define QSPI_SR_TXFULL				(1 << 27)

/* QSPI Flag Register fields */
#define QSPI_FR_TFF					(1 << 0)
#define QSPI_FR_IPGEF				(1 << 4)
#define QSPI_FR_IPIEF				(1 << 6)
#define QSPI_FR_IPAEF				(1 << 7)
#define QSPI_FR_IUEF				(1 << 11)
#define QSPI_FR_ABOF				(1 << 12)
#define QSPI_FR_ABSEF				(1 << 15)
#define QSPI_FR_RBDF				(1 << 16)
#define QSPI_FR_RBOF				(1 << 17)
#define QSPI_FR_ILLINE				(1 << 23)
#define QSPI_FR_TBUF				(1 << 26)
#define QSPI_FR_TBFF				(1 << 27)
#define QSPI_FR_DLPFF				(1 << 31)


/* QSPI Sequence Pointer Clear Register */
#define QSPI_SPTRCLR_BFPTRC			(1 << 0)
#define QSPI_SPTRCLR_IPPTRC			(1 << 8)

#define QSPI_TX_SIZE				128


/* LUT copied from imxrt */
/* TODO: make it common */ 

#define LUT_INSTR(opcode0, pads0, operand0, opcode1, pads1, operand1) \
				 (((opcode1 & 0x3f) << 26) | ((pads1 & 0x3) << 24) | (operand1 & 0xff) << 16) \
				 | (((opcode0 & 0x3f) << 10) | ((pads0 & 0x3) << 8) | (operand0 & 0xff))



#define QSPI_LUT_KEY 0x5AF05AF0

/* Instruction opcodes in SDR mode, based on IMXRT1064RM Rev. 0.1, 12/2018 chapter 26.7.8  */
#define QSPI_CMD       (0x1U)
#define QSPI_ADDR      (0x2U)
#define QSPI_DUMMY     (0x3U)
#define QSPI_MODE      (0x4U)
#define QSPI_MODE2     (0x5U)
#define QSPI_MODE4     (0x6U)
#define QSPI_READ      (0x7U)
#define QSPI_WRITE     (0x8U)
#define QSPI_JMP_ON_CS (0x9U)
#define QSPI_ADDR_DDR  (0xAU)
#define QSPI_MODE_DDR  (0xBU)
#define QSPI_MODE2_DDR (0xCU)
#define QSPI_MODE4_DDR (0xDU)
#define QSPI_READ_DDR  (0xEU)
#define QSPI_WRITE_DDR (0xFU)
#define QSPI_DATA_LEARN (0x10U)
#define QSPI_CMD_DDR    (0x11U)
#define QSPI_CADDR      (0x12U)
#define QSPI_CADDR_DDR  (0x13U)
#define QSPI_STOP       (0x0U)


#define LUT_PAD1  0
#define LUT_PAD2  1
#define LUT_PAD4  2
#define LUT_PAD8  3


#define LUT_3B_ADDR  0x18    /* 4 byte address */
#define LUT_4B_ADDR  0x20    /* 3 byte address */

int qspi_init(void);
void qspi_lutSequenceWrite(uint32_t index, uint32_t *cmd);
void qspi_issueIPCommand(unsigned seqId);
int qspi_flashRead(size_t sz);

#endif