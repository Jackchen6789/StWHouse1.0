#include "LedLight.h"

/**
 * @brief  初始化 LED 照明灯
 *         PB3 → TIM2_CH2 (需 TIM2 完全重映射), PWM 1kHz, 占空比 0~100%
 *         注意: PB3 默认 JTDO + TIM2_CH2 不在默认引脚上，需全重映射
 */
void LedLight_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    /* 1. 时钟 */
    RCC_APB2PeriphClockCmd(LED_RCC_GPIO | LED_RCC_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(LED_TIM_RCC, ENABLE);

    /* 2. 禁用 JTAG (释放 PB3/PB4/PA15) + TIM2 完全重映射 */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_TIM2, ENABLE);       /* ★ 关键: CH2→PB3 */

    /* 3. PB3 复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = LED_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);

    /* 4. TIM2 时基: 72MHz / 72 = 1MHz, ARR=999 → 1kHz PWM */
    TIM_TimeBaseStructure.TIM_Period        = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler     = 72 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(LED_TIM, &TIM_TimeBaseStructure);

    /* 5. TIM2_CH2 PWM1 模式 */
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC2Init(LED_TIM, &TIM_OCInitStructure);

    TIM_OC2PreloadConfig(LED_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(LED_TIM, ENABLE);
    TIM_Cmd(LED_TIM, ENABLE);
}

void LedLight_SetBrightness(uint8_t percent)
{
    uint16_t pulse;

    if (percent > 100) percent = 100;

    pulse = (uint16_t)((uint32_t)percent * 1000 / 100);
    if (pulse > 999) pulse = 999;

    TIM_SetCompare2(LED_TIM, pulse);
}

void LedLight_On(void)
{
    LedLight_SetBrightness(100);
}

void LedLight_Off(void)
{
    LedLight_SetBrightness(0);
}
