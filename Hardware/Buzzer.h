#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

// 蜂鸣器引脚定义
#define BUZZER_RCC    RCC_APB2Periph_GPIOB
#define BUZZER_PORT   GPIOB
#define BUZZER_PIN    GPIO_Pin_1

// 函数声明
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);

#endif /* __BUZZER_H */