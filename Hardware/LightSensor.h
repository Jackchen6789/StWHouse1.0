#ifndef __LIGHTSENSOR_H
#define __LIGHTSENSOR_H

#include "stm32f10x.h"

/* 光敏电阻 5516 模块引脚定义 */
#define LIGHT_RCC_GPIO   RCC_APB2Periph_GPIOB
#define LIGHT_RCC_ADC    RCC_APB2Periph_ADC1
#define LIGHT_PORT_AO    GPIOB
#define LIGHT_PIN_AO     GPIO_Pin_0
#define LIGHT_ADC_CH     ADC_Channel_8

#define LIGHT_PORT_DO    GPIOB
#define LIGHT_PIN_DO     GPIO_Pin_8

void LightSensor_Init(void);
uint16_t LightSensor_GetValue(void);      /* 读取 AO 模拟值 (0~4095) */
uint8_t  LightSensor_IsDark(void);        /* 读取 DO 数字值 (0=亮, 1=暗) */

#endif
