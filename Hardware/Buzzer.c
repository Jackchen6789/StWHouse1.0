#include "Buzzer.h"

/**
 * @brief  初始化PB1引脚为蜂鸣器驱动
 */
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 使能 GPIOB 时钟
    RCC_APB2PeriphClockCmd(BUZZER_RCC, ENABLE);
    
    // 2. 配置 PB1 为通用推挽输出
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
    
    // 3. 初始状态关闭蜂鸣器
    Buzzer_Off();
}

/**
 * @brief  开启蜂鸣器
 */
void Buzzer_On(void)
{
    // 如果你的模块是高电平触发，请改为 SetBits；如果是低电平触发，请改为 ResetBits
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN); 
}

/**
 * @brief  关闭蜂鸣器
 */
void Buzzer_Off(void)
{
    // 对应上面，高电平触发则改为 ResetBits；低电平触发则改为 SetBits
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN); 
}