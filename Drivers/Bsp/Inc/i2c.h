#ifndef I2C_H
#define I2C_H
#include "stm32f407xx.h"

void I2C1_Init();
void I2C1_Scan(void);
void I2C1_Read_Byte(uint8_t saddr, uint8_t maddr, uint8_t* data);
void I2C1_Burst_Read(uint8_t saddr, uint8_t maddr, int n, uint8_t* data);
void I2C1_Write(uint8_t saddr, uint8_t maddr, uint8_t data);
void I2C1_Burst_Write(uint8_t saddr, uint8_t reg_start, const uint8_t *data, uint16_t length);

#endif