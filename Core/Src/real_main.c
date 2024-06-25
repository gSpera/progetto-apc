/*
 * real_main.c
 *
 *  Created on: Jun 22, 2024
 *      Author: gs
 */

#include "real_main.h"
#include "main.h"

#include "usart.h"

#include "stm32f3xx_hal.h"

#define EP_CTRL 1
#define EP_BULK 2
uint8_t cmdBuffer[16];
uint8_t bulkBuffer[256];
#define FLASH_BLOCK 0x1

int memfind(char *buff, char what) {
	for (int i=0;i<2048;i++) {
		if (buff[i] == what) return i;
	}
	return 2043;
}

uint8_t class_init(USBD_HandleTypeDef *usb, uint8_t cfgid) {
	usart_puts("Password manager class init\n");

    /* Open EP OUT Control */
    USBD_LL_OpenEP(usb, EP_CTRL, USBD_EP_TYPE_CTRL, 16);

    /* Open EP OUT Bulk */
    USBD_LL_OpenEP(usb, EP_BULK, USBD_EP_TYPE_BULK, sizeof(bulkBuffer));
    USBD_LL_OpenEP(usb, EP_BULK | 0x80, USBD_EP_TYPE_BULK, sizeof(bulkBuffer));

    usb->ep_out[1].is_used = 1U;
    usb->ep_out[2].is_used = 1U;
    usb->ep_in[2].is_used = 1U;

    /* Prepare Out endpoint to receive next packet */
    // USBD_LL_PrepareReceive(usb, EP_CTRL, cmdBuffer, sizeof(cmdBuffer));
    USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
    // USBD_LL_StallEP(usb, 2);
	return 0;
}

uint8_t data_sent(USBD_HandleTypeDef *usb, uint8_t ep) {
	usart_puts("Sent data on EP");
	usart_putdec(ep);
	usart_putc('\n');
    USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
	return USBD_OK;
}

uint8_t data_received(USBD_HandleTypeDef *usb, uint8_t ep) {
	uint32_t len = USBD_LL_GetRxDataSize(usb, ep);
	usart_puts("Received data on EP");
	usart_putdec(ep);
	usart_puts(", received: ");
	usart_putdec(len);
	usart_puts(" bytes \n");
	if (ep != 2) {
		usart_puts("Unkown endpoint\n");
	}

	for (uint32_t i = 0; i < len; i++) {
		usart_puthex(bulkBuffer[i]);
		usart_putc(' ');
	}
	usart_putc('\n');

	// Best protocol specification
	// First byte is the command,
	//  then : and arguments
	// U: Unlock, argument: Master password
	// W: Check if locked
	// C: Count avaible passwords
	// N: Retrieve password name, argument: index
	// R: Retrieve password, argument: index
	// S: Save password, argument: index, password

	int index;
	char password[256];
	switch (bulkBuffer[0]) {
	case 'U':
		usart_puts("Unlocking vault\n");
		vault_unlock();
		// USBD_LL_ClearStallEP(usb, EP_BULK);
		USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
		break;
	case 'W':
		usart_puts("Checking if vault is locked\n");
		int v = vault_is_locked();
		if (v == 0) {
			// Unlocked
			strcpy((char *) bulkBuffer, "U");
		} else {
			strcpy((char *) bulkBuffer, "L");
		}
		USBD_LL_Transmit(usb, EP_BULK | 0x80, bulkBuffer, 1);
		break;
	case 'C':
		usart_puts("Counting passwords\n");
		int c = password_count();
		uint16_t n = (uint16_t) sprintf((char *) bulkBuffer, "C:%d", c);
		usart_puts("Sending ");
		usart_putdec(n);
		usart_puts(" bytes\n");
		USBD_StatusTypeDef res = USBD_LL_Transmit(usb, EP_BULK | 0x80, bulkBuffer, n);
		if (res != USBD_OK) {
			usart_puts("ERROR while sending data: ");
			usart_puthex((uint32_t) res);
			usart_putc('\n');
		} else {
			usart_puts("DONE\n");
		}
		break;
	case 'N':
		sscanf((char *) bulkBuffer, "N:%d", &index);
		usart_puts("Reading name: ");
		usart_putdec(index);
		usart_putc('\n');
		char *password = password_name(index);
		strcpy((char *) bulkBuffer, password);
		USBD_LL_Transmit(usb, EP_BULK | 0x80, bulkBuffer, strlen(password));
		break;
	case 'R':
		sscanf((char *) bulkBuffer, "R:%d", &index);
		usart_puts("Reading password: ");
		usart_putdec(index);
		usart_putc('\n');
		password = password_retrieve(index);
		strcpy((char *) bulkBuffer, password);
		USBD_LL_Transmit(usb, EP_BULK | 0x80, bulkBuffer, strlen(password));
		break;
	case 'S':
		sscanf((char *) bulkBuffer, "S:%d:%50s:", &index, password);
		usart_puts("Writing password: ");
		usart_putdec(index);
		usart_puts(" : ");
		usart_putc('\n');
		password_save(index, password);
		USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
		break;
	case 'X':
		// int ret = sscanf((char *) bulkBuffer, "X:%50s:%50s:", name, password);
		char *name = &bulkBuffer[2];
		int index = memfind(name, ':');
		name[index] = 0;
		password = &name[index+1];
		index = memfind(password, ':');
		password[index] = 0;
		usart_puts("New password: ");
		usart_puts(name);
		usart_puts(" ");
		usart_puts(password);
		usart_putc('\n');

		password_new(name, password);
		USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
		break;
	default:
		usart_puts("Unkown command\n");
		USBD_LL_PrepareReceive(usb, EP_BULK, bulkBuffer, sizeof(bulkBuffer));
		break;
	}

	usart_puts("Done handling command\n");
	return USBD_OK;
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
	return NULL;
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

int vault_unlock() {
	usart_puts("Unlocking vault\n");
	return 0;
}
void  vault_lock() {
	usart_puts("Locking vault\n");
}


__attribute__((section(".vault"))) char names[1024];
__attribute__((section(".vault"))) char passwords[2048];
char namesBuff[1024] = {0xFF};
char passwordsBuff[2048] = {0xFF};

void real_main() {
	if (names[0] == 0) {
		usart_puts("Setting up vault\n");
		HAL_FLASH_Unlock();
		FLASH_EraseInitTypeDef erase;
		erase.TypeErase = FLASH_TYPEERASE_PAGES;
		erase.PageAddress = (uint32_t) &names;
		erase.NbPages = 2;
		uint32_t *err;
		HAL_FLASHEx_Erase(&erase, &err);
		HAL_FLASH_Lock();
	}
}


#define PAGE_SIZE 2048

char *get_index(char *buff, int index) {
	int count = 0;
	for (int i=0;i<2048;i++) {
		if (count == index) return &buff[i];
		if (buff[i] == '\0') count++;
	}

	return &buff[2048];
}

void update_flash() {
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef erase;
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.PageAddress = (uint32_t) &names;
	erase.NbPages = 2;
	uint32_t *err;
	usart_puts("Erasing flash\n");
	HAL_FLASHEx_Erase(&erase, &err);

	usart_puts("Programming flash\n");
	uint32_t *buff = (uint32_t *) namesBuff;
	uint32_t *src = (uint32_t *) names;
	for (int i = 0; i< 1024 / 4; i++) {
		usart_puts("Programming address: ");
		usart_puthex((uint32_t) &buff[i]);
		usart_putc('\n');
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&src[i], buff[i]) != HAL_OK) {
			usart_puts("Cannot program flash names\n");
		}
	}

	buff = (uint32_t *) passwordsBuff;
	src = (uint32_t *) passwords;
	for (int i=0;i<2048 / 4;i++) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&src[i], buff[i]) != HAL_OK) {
			usart_puts("Cannot program flash passwords\n");
		}
	}

	usart_puts("Programmed flash\n");
	HAL_FLASH_Lock();
}

int password_count() {
	int count = 0;
	if (names[0] == '\0') return 0;
	for (int i=0;i<1024;i++) {
		if (names[i] == '\xff') {
			break;
		}
		if (names[i] == '\0') {
			count++;
		}
	}

	return count;
}

char *password_name(int index) {
	return get_index(names, index);
}
char *password_retrieve(int index) {
	return get_index(passwords, index);
}

void password_new(char *name, char *password) {
	memcpy(namesBuff, names, sizeof(names));
	memcpy(passwordsBuff, passwords, sizeof(passwords));
	int namesIndex = memfind(namesBuff, '\xff');
	strcpy(&namesBuff[namesIndex], name);
	int passwordIndex = memfind(passwordsBuff, '\xff');
	strcpy(&passwordsBuff[passwordIndex], password);

	update_flash();
}

void password_save(int index, char *password) {
}

int vault_is_locked() {
	return 0;
}
