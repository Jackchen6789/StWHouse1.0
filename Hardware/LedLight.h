#ifndef __LEDLIGHT_H
#define __LEDLIGHT_H

#include "stm32f10x.h"

/* LED 引脚定义: PB3 (TIM2_CH2 PWM) */
#define LED_RCC_GPIO   RCC_APB2Periph_GPIOB
#define LED_RCC_AFIO   RCC_APB2Periph_AFIO
#define LED_PORT       GPIOB
#define LED_PIN        GPIO_Pin_3
#define LED_TIM        TIM2
#define LED_TIM_RCC    RCC_APB1Periph_TIM2

void LedLight_Init(void);
void LedLight_SetBrightness(uint8_t percent);   /* 0~100 */
void LedLight_On(void);
void LedLight_Off(void);

#endif
