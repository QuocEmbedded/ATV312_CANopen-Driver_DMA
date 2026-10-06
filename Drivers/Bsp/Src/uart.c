#include "stm32f407xx.h"
#include "uart.h"
#include "gpio.h"

volatile char buffer[50]; // tao 
volatile char *rx_ptr = buffer;
volatile uint8_t rx_index = 0;
volatile uint8_t rx_ready = 0;

void UART1_Init(void)
{
    //1. cau hinh clock
    RCC->AHB1ENR |= (1U<<0); // bat clock cho gpio
    RCC->APB2ENR |= (1U<<4); // bat clock cho uart1.

    //2. cau hinh chan GPIO
    GPIOA->MODER &= ~(3U<<18); // reset moder cho PA9
    GPIOA->MODER |= (2U<<18); // bat chuc nang alternative cho chan PA9
    GPIOA->MODER &= ~(3U<<20); // reset moder cho PA10
    GPIOA->MODER |= (2U<<20); // bat chuc nang alternative cho chan PA10  
    
    GPIOA->OTYPER &= ~(1U<<9);  // bat chuc nang push pull
    GPIOA->OTYPER &= ~(1U<<10); // bat chuc nang push pull

    GPIOA->OSPEEDR &= ~(3U<<18); // reset speed
    GPIOA->OSPEEDR |= (2U<<18); // bat high speed
    GPIOA->OSPEEDR &= ~(3U<<20); // reset speed
    GPIOA->OSPEEDR |= (2U<<20); // bat high speed

    GPIOA->AFR[1] &= ~(0xFU<<4);
    GPIOA->AFR[1] |= (7U<<4); // bat AF7 chuc nang UART1
    GPIOA->AFR[1] &= ~(0xFU<<8);
    GPIOA->AFR[1] |= (7U<<8); // bat AF7 chuc nang UART1

    USART1->CR1 &= ~(1U<<13); // tat UE truoc khi chay
    USART1->BRR = (0x2D9); // baudrate 115200 @168Mhz
    USART1->CR1 &= ~(1U<<12); // 1 start bit, 8 data bits, n word length

    USART1->CR1 |= (1U<<5); // bat nhan ngat

    USART1->CR1 |= (3U<<2); // bat TE va RE
    USART1->CR1 |= (1U<<13); // bat UE

    // NVIC_EnableIRQ(USART1_IRQn);
    // NVIC_SetPriority(USART1_IRQn, 2);
}

// void USART1_IRQHandler()
// {
//     if (USART1->SR & (1U<<5)) // Neu bit so 5 co bat len
//     {
//         *rx_ptr = (char)(USART1->DR); // tien hanh chep du lieu trong DR vao tx_data
//         if (*rx_ptr == '\n' || *rx_ptr == '\r' ||rx_index >= 48)
//         {
//             //rx_ptr++;
//             *rx_ptr = '\0'; // khoa chuoi
//             rx_ready = 1; // da doc xong

//             rx_ptr = buffer;
//             rx_index = 0;
//         }
//         else
//         {
//             rx_ptr++;
//             rx_index++;
//         }
//     }
    
// }

void Send_String(char *tx_ptr)
{
    while (*tx_ptr != '\0')
    {
        while (!(USART1->SR & (1U<<7)));
        USART1->DR = *tx_ptr;
        tx_ptr++;
    }
}

void Send_Integer(int32_t num)
{
    char numbuffer[12]; //tao 1 mang (0-9), '-', '\0'
    uint32_t i = 0;
    uint32_t is_negative = 0;

    if (num == 0)
    {
        Send_String("0");
        return;
    }
    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }
    while (num > 0)
    {
        numbuffer[i] = num % 10 + '0';
        i++;
        num = num / 10;
    }
    if (is_negative)
    {
        numbuffer[i] = '-';
        i++;
    }
    numbuffer[i] = '\0';

    char *start_numbuffer = numbuffer;
    char *end_numbuffer = numbuffer + i - 1;
    while (start_numbuffer < end_numbuffer)
    {
        char temp = *start_numbuffer;
        *start_numbuffer = *end_numbuffer;
        *end_numbuffer = temp;
        start_numbuffer++;
        end_numbuffer--;
    }
    Send_String(numbuffer);
}







