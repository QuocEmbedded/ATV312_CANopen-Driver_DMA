#include "gpio.h"
#include "stm32f407xx.h"
#include "stdbool.h"

#define AHB1_EN (1U<<0) // bật RCC cua GPIO lên

#define LED_BR6 (1U<<6) // bit reset cua GPIO_6
#define LED_BS6 (1U<<22) // bit set cua GPIO_6
#define LED_BR7 (1U<<7) // bit reset cua GPIO_6
#define LED_BS7 (1U<<23) // bit set cua GPIO_6

void led_init(void)
{
    // 1. Bật clock cho Port A (Giữ nguyên)
    RCC->AHB1ENR |= AHB1_EN;
    // 2. Cho chân GPIO_6 là output thì set the nay
    GPIOA->MODER |= (1U<< 12);
    GPIOA->MODER &= ~(1U<<13); 

    GPIOA->MODER |= (1U<< 14);
    GPIOA->MODER &= ~(1U<<15);

    led6_off();
    led7_off();
}

//led sink thi nen set nhu the nay
void led6_off(void)
{
    GPIOA->BSRR |= LED_BR6;
}

void led6_on(void) 
{
    GPIOA->BSRR |= LED_BS6;
}

void Toggle_led6(void)
{
    GPIOA->ODR ^= (1U<<6);
}

void Toggle2_led6(void)
{
    static volatile uint32_t ledcounter = 0;
    ledcounter = !ledcounter;
        if (ledcounter)
        {
            led6_on();
        }
        else
        {
            led6_off();
        }
}


void led7_off(void)
{
    GPIOA->BSRR |= LED_BR7;
}

void led7_on(void) 
{
    GPIOA->BSRR |= LED_BS7;
}

void Toggle_led7(void)
{
    GPIOA->ODR ^= (1U<<7);
}


void Button_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;

    GPIOA->MODER &= ~(3U << (0 * 2)); 
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |= (2U << (0 * 2)); 

    GPIOE->MODER &= ~(3U<<6);
    GPIOE->PUPDR &= ~(3U<<6);
    GPIOE->PUPDR |=  (1U<<6);

    GPIOE->MODER &= ~(3U<<8);
    GPIOE->PUPDR &= ~(3U<<8);
    GPIOE->PUPDR |=  (1U<<8);
}

bool ButtonA0_State(void) // nut nay cho K_up
{return (GPIOA->IDR & (1U<<0)) != 0;}

bool ButtonE3_State(void) // nut nay cho K1
{return (GPIOE->IDR & (1U<<3)) == 0;}

bool ButtonE4_State(void) // nut nay cho K0
{return (GPIOE->IDR & (1U<<4)) == 0;}
