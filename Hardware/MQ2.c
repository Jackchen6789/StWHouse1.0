#include "MQ2.h"

void MQ2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef  ADC_InitStructure;

    /* 1. 时钟: GPIOA(ADC) + GPIOB(DO) + ADC1 */
    RCC_APB2PeriphClockCmd(MQ2_RCC_GPIO | MQ2_RCC_DO | MQ2_RCC_ADC, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* 2. AO ── PA1 模拟输入 */
    GPIO_InitStructure.GPIO_Pin  = MQ2_PIN_AO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(MQ2_PORT_AO, &GPIO_InitStructure);

    /* 3. DO ── PB15 浮空输入 */
    GPIO_InitStructure.GPIO_Pin  = MQ2_PIN_DO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(MQ2_PORT_DO, &GPIO_InitStructure);

    /* 4. ADC1 配置 */
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

uint16_t MQ2_Get_Value(void)
{
    ADC_RegularChannelConfig(ADC1, MQ2_ADC_CH, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

uint16_t MQ2_Get_Average(uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;
    for (t = 0; t < times; t++)
        temp_val += MQ2_Get_Value();
    return (uint16_t)(temp_val / times);
}

/**
 * @brief  读取 MQ2 模块 DO 数字输出
 * @retval 0=安全(未超阈值), 1=报警(烟雾浓度超过模块设定阈值)
 */
uint8_t MQ2_DO_GetStatus(void)
{
    /* MQ2 模块 DO 低电平有效: 检测到气体 → DO=LOW, 正常 → DO=HIGH */
    return (GPIO_ReadInputDataBit(MQ2_PORT_DO, MQ2_PIN_DO) == Bit_RESET) ? 1 : 0;
}
