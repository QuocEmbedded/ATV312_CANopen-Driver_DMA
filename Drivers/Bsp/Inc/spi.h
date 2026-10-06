#include "stm32f407xx.h"
#ifndef SPI_H
#define SPI_H
void Gpio_Spi_Init();
void Spi_Config();
void SPI_Transmiter(uint8_t *data, uint32_t size);
void SPI_Receiver(uint8_t *data, uint32_t size);
void cs_enable();
void cs_disable();
#endif