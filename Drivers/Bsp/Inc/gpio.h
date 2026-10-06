#ifndef GPIO_H
#define GPIO_H
#include <stdbool.h>

void led_init(void);
void led6_on(void);
void led6_off(void);
void Toggle_led6(void);
void Toggle2_led6(void);
void led7_on(void);
void led7_off(void);
void Toggle_led7(void);
void Button_Init(void);
bool ButtonA0_State(void);
bool ButtonE3_State(void);
bool ButtonE4_State(void);

#endif