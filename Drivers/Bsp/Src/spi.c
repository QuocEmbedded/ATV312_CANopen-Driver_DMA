#include "spi.h"
#define SR_RXE  (1U<<0)
#define SR_TXE  (1U<<1)
#define SR_BUSY (1U<<7)


void Gpio_Spi_Init()
{
    RCC->AHB1ENR  |=  (1U<<0); // bat clock cho GPIOA

    //moder chan alternative fucntion, chon chan PA4 lam chan output
    GPIOA->MODER  &= ~((3U<<10) | (3U<<12) | (3U<<14));
    GPIOA->MODER  |=  ((2U<<10) | (2U<<12) | (2U<<14));

    // cho chan PA4 output va o che do push pull
    GPIOA->MODER  &= ~(3U<<8);
    GPIOA->MODER  |=  (1U<<8);
    GPIOA->OTYPER &= ~(1U<<4); // push pull

    // cau hinh chan tuong ung SCK, MISO, MOSI - SCK SDO SDI
    GPIOA->AFR[0] &= ~((15U<<20)| (15U<<24)| (15U<<28));
    GPIOA->AFR[0] |=   ((5U<<20) | (5U<<24) | (5U<<28));

    // cau hinh mac dinh chan nay lam ss
    GPIOA->ODR |= (1U<<4);

}
void Spi_Config()
{
    // bat clock cho bus chua chuc nang cua SPI1 len
    RCC->APB2ENR |= (1U<<12); 

    // chon pcsk /4 = 16Mhz/4 = 4Mhz
    SPI1->CR1 &= ~(7U << 3);
    SPI1->CR1 |=  (4U << 3);    
    // Chon CPOL = 0 and CPHA = 0, tuong ung voi chip BME680
    SPI1->CR1 &=  ~(1U<<1);
    SPI1->CR1 &=  ~(1U<<0);

    // chon cach giao tiep: full duplec hay half duplex --> chon full duplex
    SPI1->CR1 &= ~(1U<<10);

    // chon kieu truyen du lieu: MSB first
    SPI1->CR1 &=  ~(1U<<7);

    // chon che do cho chip: Master/Slave --> Master
    SPI1->CR1 |=   (1U<<2);

    // Kich thuoc goi tin--> 8 bit
    SPI1->CR1 &= ~(1U<<11);

    // Set sofware management de co the quyet dinh SS
    SPI1->CR1 |=   (1U<<9);

    // Set master de dam bao luon truyen duoc, khong bi loi truyen
    SPI1->CR1 |=   (1U<<8);

    // enable SPI module len
    SPI1->CR1 |=   (1U<<6);
}

void SPI_Transmiter(uint8_t *data, uint32_t size)
{
    uint32_t i = 0;
    while(i < size){
        while (!(SPI1->SR & SR_TXE)) {}
        SPI1->DR = data[i]; 
        while (!(SPI1->SR & SR_RXE)) {}
        (void)SPI1->DR;
        i++;
    }
    while (!(SPI1->SR & SR_TXE)) {}
    while ((SPI1->SR & SR_BUSY)) {}
}

void SPI_Receiver(uint8_t *data, uint32_t size)
{
    while (size)
    {
        while (!(SPI1->SR & (SR_TXE))){}
        SPI1->DR = 0; // gui 1 gia tri dummy
        while (!(SPI1->SR & (SR_RXE))){}
        *data++ = SPI1->DR; // gia tri cua tung bit data lay tu DR
        size--;
    }
}

void cs_enable()
{
    GPIOA->ODR &= ~(1U<<4);
}

void cs_disable()
{
    GPIOA->ODR |= (1U<<4);
}
