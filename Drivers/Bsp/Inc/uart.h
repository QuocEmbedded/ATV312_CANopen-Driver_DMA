#ifndef UART_H
#define UART_H
#include "stm32f407xx.h"

extern volatile char buffer[50]; // tao 
extern volatile char *rx_ptr;
extern volatile uint8_t rx_index;
extern volatile uint8_t rx_ready;

void UART1_Init(void);
void Send_String(char *str);
void Send_Integer(int32_t num);
#endif