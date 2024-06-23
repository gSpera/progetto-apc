#ifndef H_USART
#define H_USART

#include <stdint.h>

void usart_init(int baud);
void usart_putc(char ch);
void usart_puts(const char *str);
void usart_puthex(uint32_t word);
void usart_putdec(uint16_t halfword);

#endif
