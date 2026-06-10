#include "HC_SR501.h"

/**
 * @brief  初始化PA3引脚为输入模式，用于接收人体红外传感器信号
 */
void PIR_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 开启 GPIOA 时钟
    RCC_APB2PeriphClockCmd(PIR_RCC, ENABLE);
    
    // 2. 配置 PA3 为下拉输入 (IPD)
    // 没人时默认被内部电阻拉到低电平，当接收到模块的高电平信号时能精准捕获
    GPIO_InitStructure.GPIO_Pin = PIR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PIR_PORT, &GPIO_InitStructure);
}

/**
 * @brief  获取当前是否有人类活动
 * @return 1: 有人在感应区, 0: 无人活动
 */
uint8_t PIR_Get_Status(void)
{
    // 读取 PA3 的输入电平
    if (GPIO_ReadInputDataBit(PIR_PORT, PIR_PIN) == Bit_SET)
    {
        return 1; // 读到高电平，说明有人
    }
    else
    {
        return 0; // 读到低电平，说明无人
    }
}



