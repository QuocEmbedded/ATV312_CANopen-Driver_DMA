#include "i2c.h"
#include "uart.h"

#define CR1_START (1U<<8)
#define CR1_STOP  (1U<<9)
#define CR1_ACK   (1U<<10)

#define SR1_START (1U<<0)
#define SR1_ADDR  (1U<<1)
#define SR1_BTF   (1U<<2)
#define SR1_RXNE  (1U<<6)
#define SR1_TXE   (1U<<7)
#define SR1_AF    (1U<<10)

#define SR2_BUSY  (1U<<1)

#define I2C_TIMEOUT 50000U

void I2C1_Init()
{
    // I. Cau hinh chan PB6(SCL), PB7(SDA)
    RCC->AHB1ENR  |=  (1U<<1);

    GPIOB->MODER &= ~(3U<<12);
    GPIOB->MODER  |= (2U<<12);
    GPIOB->MODER &= ~(3U<<14);
    GPIOB->MODER  |= (2U<<14);

    GPIOB->OTYPER |=  (1U<<6);
    GPIOB->OTYPER |=  (1U<<7);

    GPIOB->PUPDR  |= (1U<<12);
    GPIOB->PUPDR  |= (1U<<14);
 
    GPIOB->AFR[0] |=  (4U<<24);
    GPIOB->AFR[0] |= (4U<<28);

    // II. Cau hinh I2C
    RCC->APB1ENR  |= (1U<<21);
    I2C1->CR1     |= (1U<<15); // bat reset I2C
    I2C1->CR1    &= ~(1U<<15); // xoa reset I2C
    
    I2C1->CR2      = (0x2A); // set 42Mhz
    I2C1->CCR      = (0xD2); // chay o 100kHZ
    I2C1->TRISE    = (0x2B); // chu ki nhip bus APB1

    I2C1->CR1     |= (1U<<0); // bat I2C
}

void I2C1_Read_Byte(uint8_t saddr, uint8_t maddr, uint8_t* data)
{
    uint32_t to = I2C_TIMEOUT;
    while ((I2C1->SR2 & (SR2_BUSY)) && --to);
    
    // 1. Start
    I2C1->CR1 |= (CR1_START);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & (SR1_START)) && --to);

    // 2. Gui dia chi de ghi thanh ghi
    I2C1->DR = (saddr<<1);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_ADDR) && --to);
    (void)I2C1->SR1; // Doc SR1 roi SR2 de xoa sach co ADDR
    (void)I2C1->SR2;

    // 3. Gui dia chi thanh ghi can doc
    I2C1->DR = maddr;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_TXE) && --to);
    
    // 4. Repeated Start de doc 1 byte
    I2C1->CR1 |= (CR1_START);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & (SR1_START)) && --to);

    I2C1->DR = (saddr<<1) | 1;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_ADDR) && --to);

    // Voi 1 byte duy nhat: Tat ACK va bat STOP TRUOC KHI xoa co ADDR
    I2C1->CR1 &= ~(CR1_ACK);
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    I2C1->CR1 |= CR1_STOP;

    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_RXNE) && --to);
    *data = (uint8_t)I2C1->DR;
}

void I2C1_Burst_Read(uint8_t saddr, uint8_t maddr, int n, uint8_t* data)
{
    uint32_t to = I2C_TIMEOUT;
    while ((I2C1->SR2 & SR2_BUSY) && --to);

    I2C1->CR1 |= (CR1_START);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_START) && --to);

    I2C1->DR = (saddr << 1);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_ADDR) && --to);
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    I2C1->DR = maddr;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_TXE) && --to);

    I2C1->CR1 |= (CR1_START);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_START) && --to);

    I2C1->DR = (saddr << 1) | 1;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_ADDR) && --to);

    // Xu ly rieng truong hop n == 1 de khong bao gio bi treo phan cung
    if (n == 1)
    {
        I2C1->CR1 &= ~(CR1_ACK);
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C1->CR1 |= CR1_STOP;

        to = I2C_TIMEOUT;
        while (!(I2C1->SR1 & SR1_RXNE) && --to);
        *data = (uint8_t)I2C1->DR;
    }
    else
    {
        I2C1->CR1 |= (CR1_ACK);
        (void)I2C1->SR1;
        (void)I2C1->SR2;

        while (n > 0U)
        {
            if (n == 1U)
            {
                I2C1->CR1 &= ~(CR1_ACK);
                I2C1->CR1 |= CR1_STOP;
                to = I2C_TIMEOUT;
                while (!(I2C1->SR1 & SR1_RXNE) && --to);
                *data++ = (uint8_t)I2C1->DR;
                break;
            }
            else
            {
                to = I2C_TIMEOUT;
                while (!(I2C1->SR1 & SR1_RXNE) && --to);
                *data++ = (uint8_t)I2C1->DR;
                n--;
            }
        }
    }
}

void I2C1_Write(uint8_t saddr, uint8_t maddr, uint8_t data)
{
    uint32_t to = I2C_TIMEOUT;
    while ((I2C1->SR2 & SR2_BUSY) && --to);

    I2C1->CR1 |= (CR1_START);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_START) && --to);
    
    I2C1->DR = (saddr << 1);
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_ADDR) && --to);
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    I2C1->DR = maddr;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_TXE) && --to);

    I2C1->DR = data;
    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_TXE) && --to);

    to = I2C_TIMEOUT;
    while (!(I2C1->SR1 & SR1_BTF) && --to);
    I2C1->CR1 |= CR1_STOP;
}