/* $FreeBSD$ */
/*-
 * Copyright (c) 2008 Hans Petter Selasky. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <cyg/io/usb_sys/stdint.h>
#include <cyg/io/usb_sys/stddef.h>
#include <cyg/io/usb_sys/param.h>
#include <cyg/io/usb_sys/queue.h>
#include <cyg/io/usb_sys/types.h>
#include <cyg/io/usb_sys/systm.h>
#include <cyg/io/usb_sys/kernel.h>
#include <cyg/io/usb_sys/bus.h>
#include <cyg/io/usb_sys/module.h>
#include <cyg/io/usb_sys/lock.h>
#include <cyg/io/usb_sys/mutex.h>
#include <cyg/io/usb_sys/condvar.h>
#include <cyg/io/usb_sys/sysctl.h>
#include <cyg/io/usb_sys/sx.h>
#include <cyg/io/usb_sys/unistd.h>
#include <cyg/io/usb_sys/callout.h>
#include <cyg/io/usb_sys/malloc.h>
#include <cyg/io/usb_sys/priv.h>

#include <cyg/io/usb_dev/usb/usb.h>
#include <cyg/io/usb_dev/usb/usbdi.h>
#include <cyg/io/usb_dev/usb/usbdi_util.h>

#include <cyg/io/usb_dev/usb/usb_core.h>
#include <cyg/io/usb_dev/usb/usb_util.h>
#include <cyg/io/usb_dev/usb/usb_process.h>
#include <cyg/io/usb_dev/usb/usb_device.h>
#include <cyg/io/usb_dev/usb/usb_request.h>
#include <cyg/io/usb_dev/usb/usb_busdma.h>

#include <cyg/io/usb_dev/usb/usb_controller.h>
#include <cyg/io/usb_dev/usb/usb_bus.h>


/*------------------------------------------------------------------------*
 * device_delete_all_children - delete all children of a device
 *------------------------------------------------------------------------*/
#ifndef device_delete_all_children
int
device_delete_all_children(device_t dev)
{
	device_t *devlist;
	int devcount;
	int error;

	error = device_get_children(dev, &devlist, &devcount);
	if (error == 0) {
		while (devcount-- > 0) {
			error = device_delete_child(dev, devlist[devcount]);
			if (error) {
				break;
			}
		}
		free(devlist, M_TEMP);
	}
	return (error);
}
#endif

/*------------------------------------------------------------------------*
 *	device_set_usb_desc
 *
 * This function can be called at probe or attach to set the USB
 * device supplied textual description for the given device.
 *------------------------------------------------------------------------*/
void
device_set_usb_desc(device_t dev)
{
	struct usb_attach_arg *uaa;
	struct usb_device *udev;
	struct usb_interface *iface;
	char *temp_p;
	usb_error_t err;

	if (dev == NULL) {
		/* should not happen */
		return;
	}
	uaa = device_get_ivars(dev);
	if (uaa == NULL) {
		/* can happen if called at the wrong time */
		return;
	}
	udev = uaa->device;
	iface = uaa->iface;

	if ((iface == NULL) ||
	    (iface->idesc == NULL) ||
	    (iface->idesc->iInterface == 0)) {
		err = USB_ERR_INVAL;
	} else {
		err = 0;
	}

	temp_p = (char *)udev->bus->scratch[0].data;

	if (!err) {
		/* try to get the interface string ! */
		err = usbd_req_get_string_any
		    (udev, NULL, temp_p,
		    sizeof(udev->bus->scratch), iface->idesc->iInterface);
	}
	if (err) {
		/* use default description */
		usb_devinfo(udev, temp_p,
		    sizeof(udev->bus->scratch));
	}
	device_set_desc_copy(dev, temp_p);
	device_printf(dev, "<%s> on %s\n", temp_p,
	    device_get_nameunit(udev->bus->bdev));
}

/*------------------------------------------------------------------------*
 *	 usb_pause_mtx - factored out code
 *
 * This function will delay the code by the passed number of system
 * ticks. The passed mutex "mtx" will be dropped while waiting, if
 * "mtx" is not NULL.
 *------------------------------------------------------------------------*/
void
usb_pause_mtx(struct cyg_mutex_t *mtx, int _ticks)
{
	if (mtx != NULL)
		cyg_mutex_unlock(mtx);

	if (cold) {
		/* convert to milliseconds */
		_ticks = (_ticks * 1000) / hz;

		/* convert to microseconds, rounded up */
		_ticks = (_ticks + 1) * 1000;
		MDELAY(_ticks);

	}
	if (mtx != NULL)
		cyg_mutex_lock(mtx);
}

/*------------------------------------------------------------------------*
 *	usb_printbcd
 *
 * This function will print the version number "bcd" to the string
 * pointed to by "p" having a maximum length of "p_len" bytes
 * including the terminating zero.
 *------------------------------------------------------------------------*/
void
usb_printbcd(char *p, uint16_t p_len, uint16_t bcd)
{
	if (snprintf(p, p_len, "%x.%02x", bcd >> 8, bcd & 0xff)) {
		/* ignore any errors */
	}
}

/*------------------------------------------------------------------------*
 *	usb_trim_spaces
 *
 * This function removes spaces at the beginning and the end of the string
 * pointed to by the "p" argument.
 *------------------------------------------------------------------------*/
void
usb_trim_spaces(char *p)
{
	char *q;
	char *e;

	if (p == NULL)
		return;
	q = e = p;
	while (*q == ' ')		/* skip leading spaces */
		q++;
	while ((*p = *q++))		/* copy string */
		if (*p++ != ' ')	/* remember last non-space */
			e = p;
	*e = 0;				/* kill trailing spaces */
}

/*------------------------------------------------------------------------*
 *	usb_make_str_desc - convert an ASCII string into a UNICODE string
 *------------------------------------------------------------------------*/
uint8_t
usb_make_str_desc(void *ptr, uint16_t max_len, const char *s)
{
	struct usb_string_descriptor *p = ptr;
	uint8_t totlen;
	int j;

	if (max_len < 2) {
		/* invalid length */
		return (0);
	}
	max_len = ((max_len / 2) - 1);

	j = strlen(s);

	if (j < 0) {
		j = 0;
	}
	if (j > 126) {
		j = 126;
	}
	if (max_len > j) {
		max_len = j;
	}
	totlen = (max_len + 1) * 2;

	p->bLength = totlen;
	p->bDescriptorType = UDESC_STRING;

	while (max_len--) {
		USETW2(p->bString[max_len], 0, s[max_len]);
	}
	return (totlen);
}
