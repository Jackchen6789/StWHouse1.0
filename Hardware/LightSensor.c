#include "LightSensor.h"

/**
 * @brief  初始化光敏电阻模块 (不重复初始化 ADC1，由 MQ2_Init 完成)
 *         AO → PB0 (ADC1_CH8), DO → PB11 (GPIO 浮空输入)
 */
void LightSensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 时钟 (ADC1 时钟已在 MQ2_Init 中开启，这里确保 GPIOB 时钟) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 2. AO ── PB0 模拟输入 */
    GPIO_InitStructure.GPIO_Pin  = LIGHT_PIN_AO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(LIGHT_PORT_AO, &GPIO_InitStructure);

    /* 3. DO ── PB11 浮空输入 */
    GPIO_InitStructure.GPIO_Pin  = LIGHT_PIN_DO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(LIGHT_PORT_DO, &GPIO_InitStructure);
}

/**
 * @brief  读取光敏电阻模拟值 (0~4095)
 *         值越大表示环境越暗
 */
uint16_t LightSensor_GetValue(void)
{
    ADC_RegularChannelConfig(ADC1, LIGHT_ADC_CH, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

/**
 * @brief  读取 DO 数字输出
 * @retval 0=环境亮, 1=环境暗
 */
uint8_t LightSensor_IsDark(void)
{
    return (GPIO_ReadInputDataBit(LIGHT_PORT_DO, LIGHT_PIN_DO) == Bit_SET) ? 1 : 0;
}
