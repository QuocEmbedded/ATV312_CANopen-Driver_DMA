#include "timer.h"
#include "stm32f407xx.h"

void Timer2_Init(void)
{
    /* 1. Bật clock ngoại vi TIM2 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* 2. Prescaler chia 84 (giả sử clock timer cấp tới TIM2 là 84 MHz) -> 1 MHz = 1 us/tick */
    TIM2->PSC = 84 - 1;

    /* 3. Đặt giá trị nạp lại tối đa cho timer 32-bit */
    TIM2->ARR = 0xFFFFFFFF;

    /* 4. Ép nạp PSC vào thanh ghi đệm ngay lập tức */
    TIM2->EGR |= TIM_EGR_UG;
    
    /* 5. Xóa cờ Update Flag do bước 4 sinh ra */
    TIM2->SR &= ~TIM_SR_UIF;

    /* 6. Bật Counter */
    TIM2->CR1 |= TIM_CR1_CEN;
}

void Delay_us_TIM2(uint32_t us)
{
    uint32_t start_tick = TIM2->CNT;
    while ((TIM2->CNT - start_tick) < us);
}

void Delay_ms_TIM2(uint32_t ms)
{
    uint32_t ticks_to_wait = ms * 1000UL;
    uint32_t start_tick = TIM2->CNT;
    while ((TIM2->CNT - start_tick) < ticks_to_wait);
}