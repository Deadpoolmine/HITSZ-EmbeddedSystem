#include "uart.h"

void uart_init(void)
{

    USART_TypeDef *usart1 = (USART_TypeDef *)USART1_BASE;
    volatile unsigned int *pReg;
    /* ??GPIOA/USART1?? */
    /* RCC_APB2ENR */
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);

    /* ??????: PA9(USART1_TX), PA10(USART1_RX)
     * GPIOA_CRH = 0x40010800 + 0x04
     */

    pReg = (volatile unsigned int *)(0x40010800 + 0x04);

    /* PA9(USART1_TX) */
    *pReg &= ~((3 << 4) | (3 << 6));
    *pReg |= (1 << 4) | (2 << 6); /* Output mode, max speed 10 MHz; Alternate function output Push-pull */

    /* PA10(USART1_RX) */
    *pReg &= ~((3 << 8) | (3 << 10));
    *pReg |= (0 << 8) | (1 << 10); /* Input mode (reset state); Floating input (reset state) */

    /* ?????
     * 115200 = 8000000/16/USARTDIV
     * USARTDIV = 4.34
     * DIV_Mantissa = 4
     * DIV_Fraction / 16 = 0.34
     * DIV_Fraction = 16*0.34 = 5
     * ?????:
     * DIV_Fraction / 16 = 5/16=0.3125
     * USARTDIV = DIV_Mantissa + DIV_Fraction / 16 = 4.3125
     * baudrate = 8000000/16/4.3125 = 115942
     */
#define DIV_Mantissa 4
#define DIV_Fraction 5
    usart1->BRR = (DIV_Mantissa << 4) | (DIV_Fraction);

    /* ??????: 8n1 */
    usart1->CR1 = (1 << 13) | (0 << 12) | (0 << 10) | (1 << 3) | (1 << 2);
    usart1->CR2 &= ~(3 << 12);

    /* Enable USART1 */
}

int getchar(void)
{
    USART_TypeDef *usart1 = USART1;
    while ((usart1->SR & (1 << 5)) == 0)
        ;
    return usart1->DR;
}

int putchar(char c)
{
    USART_TypeDef *usart1 = USART1;
    while ((usart1->SR & (1 << 7)) == 0)
        ;
    usart1->DR = c;

    return c;
}

int putstring(const char *s)
{
    while (*s) {
        putchar(*s);
        s++;
    }
    return 0;
}
