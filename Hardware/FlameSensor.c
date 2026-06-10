#include "FlameSensor.h"

/**
 * @brief  初始化PA2为上拉输入模式，用于读取火焰传感器的数字信号
 */
void Flame_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 开启 GPIOA 时钟
    RCC_APB2PeriphClockCmd(FLAME_RCC, ENABLE);
    
    // 2. 配置 PA2 为上拉输入 (IPU)
    // 默认保持高电平(1)，当传感器检测到火光拉低电平时，能稳定捕获
    GPIO_InitStructure.GPIO_Pin = FLAME_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(FLAME_PORT, &GPIO_InitStructure);
}

/**
 * @brief  获取当前是否有火灾/火源
 * @return 1: 发现火源（危险）, 0: 安全（无火）
 */
uint8_t Flame_Get_Status(void)
{
    // 3线火焰传感器原理：检测到红外火光时，DO输出低电平
    if (GPIO_ReadInputDataBit(FLAME_PORT, FLAME_PIN) == Bit_RESET)
    {
        return 1; // 读到低电平，说明有火
    }
    else
    {
        return 0; // 读到高电平，说明正常
    }
}



