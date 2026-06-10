#ifndef __MQ2_H
#define __MQ2_H

#include "stm32f10x.h"

/* AO 模拟输出 (ADC) */
#define MQ2_RCC_GPIO   RCC_APB2Periph_GPIOA
#define MQ2_RCC_ADC    RCC_APB2Periph_ADC1
#define MQ2_PORT_AO    GPIOA
#define MQ2_PIN_AO     GPIO_Pin_1
#define MQ2_ADC_CH     ADC_Channel_1

/* DO 数字输出 (TTL 阈值开关) */
#define MQ2_RCC_DO     RCC_APB2Periph_GPIOB
#define MQ2_PORT_DO    GPIOB
#define MQ2_PIN_DO     GPIO_Pin_15

void MQ2_Init(void);
uint16_t MQ2_Get_Value(void);
uint16_t MQ2_Get_Average(uint8_t times);
uint8_t  MQ2_DO_GetStatus(void);    /* 0=安全, 1=超过阈值 */

#endif /* __MQ2_H */
