#include "stm32f407xx.h"
#include "UART_DMA.h"
#include <string.h>
#include "timer.h"



char test[16] = "mai bao quoc\n\t";

int main(void)
{
    UART1_GPIO_Init();
    UART1_DMA_Init();
    Timer2_Init();
    while (1)
    {
        UART1_SendString_DMA(test, (uint16_t)strlen(test));
        Delay_ms_TIM2(100);
    }
}