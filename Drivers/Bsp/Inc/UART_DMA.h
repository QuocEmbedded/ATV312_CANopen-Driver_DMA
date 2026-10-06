#ifndef UART_DMA_H_
#define UART_DMA_H_

#include <stdint.h>
#include <stdbool.h>

#define UART_DATA_BUFF_SIZE    64

void UART1_GPIO_Init(void);
void UART1_DMA_Init(void);
void UART1_SendString_DMA(const char *str, uint16_t len);
uint16_t UART1_ReadPacket(uint8_t *dest, uint16_t max_len);

#endif