#include <stdint.h>

#ifndef TIMER_H
#define TIMER_H

#define SR_UIF (1U<<0)
void Timer2_Init(void);
void Delay_us_TIM2(uint32_t us);
void Delay_ms_TIM2(uint32_t ms);
#endif