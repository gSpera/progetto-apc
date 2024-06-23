#include "usart.h"

#include <stm32f303xc.h>

void usart_init(int baud) {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;  // Used by USART1
    // GPIOC 4, 5 Alternate function
    GPIOC->MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOC->MODER |= GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;
    // GPIOC 4,5 Alternate function 7 (USART 1 TX/RX)
    // STM32F3Datasheet.pdf Page 47
    GPIOC->AFR[0] &= ~(GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5);
    GPIOC->AFR[0] |= (7 << 16) | (7 << 20);

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART1->BRR = SystemCoreClock / baud;
    USART1->CR1 |= USART_CR1_UE;
}

void usart_putc(char ch) {
    // Wait for USART1->ISR->TXE to be set
    while ((USART1->ISR & USART_ISR_TXE) == 0)
        ;
    USART1->TDR = ch;
}

void usart_puts(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        usart_putc(str[i]);
    }
}

void usart_puthex(uint32_t word) {
    // Mask top 4 bit
    uint32_t mask = 0xF << (32 - 4);

    for (int i = 0; i < 8; i++) {
        // Extract next 4 bit
        char v = (word & mask) >> (32 - 4);
        if (v < 10) {
            usart_putc(v + 0x30);
        } else {
            usart_putc(v + 0x41 - 10);
        }

        word = word << 4;
    }
}

void usart_putdec(uint16_t halfword) {
    char buff[5];
    int n;

    for(n=0;n<5;n++) {
        uint8_t v = halfword %  10 + '0';
        buff[n] = v;
        halfword /= 10;
        if (halfword == 0) break;
    }

    for (int i=n;i>=0;i--) {
        usart_putc(buff[i]);
    }
}
