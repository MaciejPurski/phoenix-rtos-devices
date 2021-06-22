#include <errno.h>
#include <fcntl.h>
#include <sys/list.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/threads.h>
#include <posix/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <usb.h>
#include <usbdriver.h>
#include <cdc.h>

#include "../libtty/fifo.h"

#define RX_FIFO_SIZE (2 * _PAGE_SIZE)

typedef struct usbacm_dev {
	char stack[2][1024] __attribute__ ((aligned(8)));
	struct usbacm_dev *prev, *next;
	usb_insertion_t instance;
	int pipeCtrl;
	int pipeIntIN;
	int pipeBulkIN;
	int pipeBulkOUT;
	int id;
	unsigned port;
	handle_t readLock;
	fifo_t *fifo;
} usbacm_dev_t;


static struct {
	usbacm_dev_t *devices;
	unsigned drvport;
} usbacm_common;


static int usbacm_init(usbacm_dev_t *dev)
{
	usb_cdc_line_coding_t line_coding;
	usb_setup_packet_t setup;
	int type;

	type = REQUEST_DIR_HOST2DEV | REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE;

	line_coding.dwDTERate = 0xc1200;
	line_coding.bCharFormat = 0;
	line_coding.bParityType = 0;
	line_coding.bDataBits = 8;

	setup.bmRequestType = REQUEST_DIR_HOST2DEV | REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE;
	setup.bRequest = USB_CDC_REQ_SET_LINE_CODING;
	setup.wIndex = dev->instance.interface;
	setup.wValue = 0;
	setup.wLength = sizeof(line_coding);

	fprintf(stderr, "usbacm: Trying to set line coding\n");
	if (usb_transferControl(dev->pipeCtrl, &setup, &line_coding, sizeof(line_coding), usb_dir_out) < 0) {
		fprintf(stderr, "usbacm: Control transfer failed\n");
		return -EIO;
	}
	fprintf(stderr, "usbacm: Line Coding set success\n");

	setup.bRequest = USB_CDC_REQ_SET_CONTROL_LINE_STATE;
	setup.wValue = 3;
	setup.wLength = 0;
	fprintf(stderr, "usbacm: Trying to set line state\n");
	if (usb_transferControl(dev->pipeCtrl, &setup, NULL, 0, usb_dir_out) < 0) {
		fprintf(stderr, "usbacm: Control transfer failed\n");
		return -EIO;
	}
	fprintf(stderr, "usbacm: Line state set success\n");

	dev->fifo = malloc(sizeof(fifo_t) + RX_FIFO_SIZE * sizeof(dev->fifo->data[0]));
	if (dev->fifo == NULL) {
		fprintf(stderr, "usbacm: Fail to allocate pipe\n");
		return -ENOMEM;
	}

	fifo_init(dev->fifo, RX_FIFO_SIZE);

	return EOK;
}


static int usbacm_read(usbacm_dev_t *dev, char *buf, size_t len)
{
	int i;

	mutexLock(dev->readLock);
	for (i = 0; i < len && !fifo_is_empty(dev->fifo); i++)
		buf[i] = fifo_pop_back(dev->fifo);
	mutexUnlock(dev->readLock);

	return i;
}


static int usbacm_write(usbacm_dev_t *dev, char *buf, size_t len)
{
	int ret;

	if ((ret = usb_transferBulk(dev->pipeBulkOUT, buf, len, usb_dir_out)) < 0) {
		fprintf(stderr, "usbacm: write failed\n");
		return -EIO;
	}

	return ret;
}


static void usbacm_readthr(void *arg)
{
	char buf[512];
	usbacm_dev_t *dev = (usbacm_dev_t *)arg;
	int ret, i;

	for (;;) {
		if ((ret = usb_transferBulk(dev->pipeBulkIN, buf, sizeof(buf), usb_dir_in)) < 0) {
			fprintf(stderr, "usbacm%d: read failed\n", dev->id);
			break;
		}
		mutexLock(dev->readLock);
		for (i = 0; i < ret && !fifo_is_full(dev->fifo); i++)
			fifo_push(dev->fifo, buf[i]);
		mutexUnlock(dev->readLock);
	}

	endthread();
}


static void usbacm_msgthr(void *arg)
{
	usbacm_dev_t *dev = (usbacm_dev_t *)arg;
	unsigned long rid;
	msg_t msg;
	int ret;

	for (;;) {
		if (msgRecv(dev->port, &msg, &rid) < 0)
			continue;
		switch (msg.type) {
		case mtOpen:
		case mtClose:
			msg.o.io.err = EOK;
			break;

		case mtRead:
			if ((msg.o.io.err = usbacm_read(dev, msg.o.data, msg.o.size)) == 0) {
				if (msg.i.io.mode & O_NONBLOCK) {
					msg.o.io.err = -EWOULDBLOCK;
				}
				else {
					// LIST_ADD(&dev->readers, req);
					// req = NULL;
				}
			}
			break;

		case mtWrite:
			msg.o.io.err = usbacm_write(dev, msg.i.data, msg.i.size);
			break;

		case mtGetAttr:
			break;

		default:
			msg.o.io.err = -EINVAL;
		}
		msgRespond(dev->port, &msg, rid);
	}

	endthread();
}


static int usbacm_handleInsertion(usb_insertion_t *insertion)
{
	char path[32];
	usbacm_dev_t *dev;
	oid_t oid;

	/* TODO: get rid of it */
	if (usbacm_common.devices != NULL)
		return 0;

	dev = malloc(sizeof(usbacm_dev_t));
	dev->instance = *insertion;

	if ((dev->pipeCtrl = usb_open(insertion, usb_transfer_control, usb_dir_bi)) < 0) {
		free(dev);
		return -EINVAL;
	}

	if (usb_setConfiguration(dev->pipeCtrl, 1) != 0) {
		free(dev);
		return -EINVAL;
	}

	fprintf(stderr, "usbacm: setConfiguration success\n");
	if ((dev->pipeBulkIN = usb_open(insertion, usb_transfer_bulk, usb_dir_in)) < 0) {
		free(dev);
		return -EINVAL;
	}

	if ((dev->pipeBulkOUT = usb_open(insertion, usb_transfer_bulk, usb_dir_out)) < 0) {
		free(dev);
		return -EINVAL;
	}

	if ((dev->pipeIntIN = usb_open(insertion, usb_transfer_interrupt, usb_dir_in)) < 0) {
		fprintf(stderr, "usbacm: NO interrupt endpoint present\n");
	} else {
		fprintf(stderr, "usbacm: Interrupt endpoint FOUND\n");
	}

	if (portCreate(&dev->port) != 0) {
		fprintf(stderr, "usbacm: Can't create port!\n");
		return -EINVAL;
	}

	/* Get next device number */
	if (usbacm_common.devices == NULL)
		dev->id = 0;
	else
		dev->id = usbacm_common.devices->prev->id + 1;

	if (usbacm_init(dev) != EOK) {
		fprintf(stderr, "usbacm: Init failed\n");
		free(dev);
		return -EINVAL;
	}

	snprintf(path, sizeof(path), "/dev/usbacm%d", dev->id);
	oid.port = dev->port;
	oid.id = dev->id;
	if (create_dev(&oid, path) != 0) {
		fprintf(stderr, "usbacm: Can't create dev!\n");
		return -EINVAL;
	}

	if (mutexCreate(&dev->readLock) != 0) {
		fprintf(stderr, "usbacm: Can't create mutex!\n");
		return -EINVAL;
	}

	LIST_ADD(&usbacm_common.devices, dev);

	beginthread(usbacm_msgthr, 4, dev->stack[0], sizeof(dev->stack[0]), dev);
	beginthread(usbacm_readthr, 4, dev->stack[1], sizeof(dev->stack[1]), dev);
	// beginthread(usbacm_msgthr, 4, dev->stack[2], sizeof(dev->stack[2]), dev);
	// beginthread(usbacm_msgthr, 4, dev->stack[3], sizeof(dev->stack[3]), dev);

	return 0;
}


int main(int argc, char *argv[])
{
	oid_t oid;
	int ret;
	msg_t msg;
	usb_msg_t *umsg = (usb_msg_t *)msg.i.raw;
	usb_device_id_t id = { 0x12d1,
	                       0x1001,
	                       USB_CONNECT_WILDCARD,
	                       USB_CONNECT_WILDCARD,
	                       USB_CONNECT_WILDCARD };
	/* TODO: this driver should handle multiple device types */

	if (portCreate(&usbacm_common.drvport) != 0) {
		fprintf(stderr, "usbacm: Can't create port!\n");
		return -EINVAL;
	}

	if ((usb_connect(&id, usbacm_common.drvport)) < 0) {
		fprintf(stderr, "usbacm: Fail to connect to usb host!\n");
		return -EINVAL;
	}

	for (;;) {
		ret = usb_eventsWait(usbacm_common.drvport, &msg);
		if (ret != 0)
			continue;
		switch (umsg->type) {
			case usb_msg_insertion:
				usbacm_handleInsertion(&umsg->insertion);
				break;
			case usb_msg_deletion:
				break;
			default:
				fprintf(stderr, "usbacm: Error when receiving event from host\n");
				break;
		}
	}

	return 0;
}