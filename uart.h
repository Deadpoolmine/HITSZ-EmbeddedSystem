#ifndef _UART_H
#define _UART_H

#include "stm32f1xx.h"

void uart_init(void);
int getchar(void);
int putchar(char c);
int putstring(const char *s);

#endif
