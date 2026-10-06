#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_RX_BUFF_SIZE    64

void UART1_Init(void);
void Send_String(const char *tx_ptr);
uint16_t UART1_ReadPacket(uint8_t *dest, uint16_t max_len);
void Int_To_String(int32_t num, char *str);
void UART_Print_Hex16(uint16_t val);
void Float_To_String_1Dec(float val, char *str);
#endif