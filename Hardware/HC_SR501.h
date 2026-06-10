#ifndef __HC_SR501_H
#define __HC_SR501_H

#include "stm32f10x.h"

// 引脚及 GPIO 时钟定义
#define PIR_RCC     RCC_APB2Periph_GPIOA
#define PIR_PORT    GPIOA
#define PIR_PIN     GPIO_Pin_3

// 函数声明
void PIR_Init(void);
uint8_t PIR_Get_Status(void);

#endif /* __HC_SR501_H */




