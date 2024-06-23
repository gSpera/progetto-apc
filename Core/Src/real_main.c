/*
 * real_main.c
 *
 *  Created on: Jun 22, 2024
 *      Author: gs
 */

#include "real_main.h"
#include "main.h"

#include "usart.h"

#define EP_CTRL 1
#define EP_BULK 2
uint8_t cmdBuffer[16];
uint8_t bulkBuffer[512];

uint8_t class_init(USBD_HandleTypeDef *usb, uint8_t cfgid) {
	usart_puts("Password manager class init\n");

    /* Open EP OUT Control */
    USBD_LL_OpenEP(usb, EP_CTRL, USBD_EP_TYPE_CTRL, 16);

    /* Open EP OUT Bulk */
    USBD_LL_OpenEP(usb, EP_BULK, USBD_EP_TYPE_BULK, sizeof(bulkBuffer));

    usb->ep_out[1].is_used = 1U;
    usb->ep_out[2].is_used = 1U;

    /* Prepare Out endpoint to receive next packet */
    USBD_LL_PrepareReceive(usb, EP_CTRL, cmdBuffer, sizeof(cmdBuffer));
    USBD_LL_StallEP(usb, 2);
	return 0;
}

uint8_t data_sent(USBD_HandleTypeDef *usb, uint8_t ep) {
	usart_puts("Sent data on EP");
	usart_putdec(ep);
	usart_putc('\n');
	return 0;
}

uint8_t data_received(USBD_HandleTypeDef *usb, uint8_t ep) {
	usart_puts("Received data on EP");
	usart_putdec(ep);
	usart_putc('\n');
	return 0;
}

uint8_t class_descriptor[] = {
		// Configuration Descriptor
		9, // Len
		USB_DESC_TYPE_CONFIGURATION,
		9+9+7+7+7 + 24, 0,
		1, // Number of Interface
		1, // Configuration Value
		0, // Configuration String Index
		0xC0, // Attribute: Self-Powered
		0x32, // Max Power

		// Interface Descriptor
		9, // Len
		USB_DESC_TYPE_INTERFACE,
		0, // Interface Number
		0, // Alternate
		3, // Number of Endpoint
		0xFF, // Class Code
		'G', // Interface sub-class
		'S', // Interface protocol
		0x02, // Interface String Index

		// Endpoint Descriptor
		7, // Len
		USB_DESC_TYPE_ENDPOINT,
		1, // EP Number
		0x0, // Type, Control
		16, 0, // Packet Size
		0, // Interval, Ignored for Control

		// Endpoint descriptor
		7, // Len
		USB_DESC_TYPE_ENDPOINT,
		2, // EP Number
		0x2, // Type, Bulk
		0, 2, // Packet Size
		0, // Interval, Ignored for Bulk

		// Endpoint descriptor
		7, // Len
		USB_DESC_TYPE_ENDPOINT,
		0x82, // EP Number
		0x2, // Type, Bulk
		0, 2, // Packet Size
		0, // Interval, Ignored for Bulk

		// WebUSB support https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
		24, // Len
		0, // Descriptor Type
		0, // Capability Type
		0, // RESERVED
		0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47, 0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65, // UUID
		0x00, 0x01, // BCD Version
		0xEE, // Vendor Code
		3, // Landing page String Index
};

uint8_t *descriptor(uint16_t *len) {
	*len = sizeof(class_descriptor);
	return class_descriptor;
}

uint8_t *handle_setup(USBD_HandleTypeDef *usb, USBD_SetupReqTypedef *req) {
	usart_puts("Handle setup");
	return 1;
}

USBD_ClassTypeDef passwordManagerClass = {
		.Init = class_init,
		.DeInit = NULL,
		.Setup = handle_setup,
		.EP0_TxSent = NULL,
		.EP0_RxReady = NULL,

		.DataIn = data_sent,
		.DataOut = data_received,
		.SOF = NULL,
		.IsoINIncomplete = NULL,
		.IsoOUTIncomplete = NULL,
		.GetHSConfigDescriptor = descriptor,
		.GetFSConfigDescriptor = descriptor,
		.GetOtherSpeedConfigDescriptor = descriptor,
		.GetDeviceQualifierDescriptor = NULL,
};

void real_main() {
}
