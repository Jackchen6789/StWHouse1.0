// dht11.c
#include "dht11.h"

/**
 * @brief  初始化DHT11引脚
 * @param  无
 * @retval 无
 */
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
    
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    // 注意：移除这里的 Delay_ms(1000)，在主循环中单独处理延时
}

/**
 * @brief  发送起始信号
 * @param  无
 * @retval 无
 */
static void DHT11_Start(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 主机拉低至少18ms
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
    
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_ms(20);
    
    // 主机拉高20-40us
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30);
    
    // 切换为输入模式，等待DHT11响应
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

/**
 * @brief  检查DHT11响应
 * @param  无
 * @retval 0: 成功, 1: 失败
 */
static uint8_t DHT11_CheckResponse(void)
{
    uint32_t timeout;
    
    // 等待DHT11拉低（响应低电平80us）
    timeout = 100000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET && timeout--)
    {
        if (timeout == 0)
            return 1; // 超时
    }
    
    // 等待低电平结束
    timeout = 100000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == RESET && timeout--)
    {
        if (timeout == 0)
            return 1; // 超时
    }
    
    // 等待DHT11拉高（准备发送数据）
    timeout = 100000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET && timeout--)
    {
        if (timeout == 0)
            return 1; // 超时
    }
    
    return 0; // 成功
}

/**
 * @brief  读取一个字节
 * @param  无
 * @retval 读取到的字节数据
 */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i;
    uint8_t byte = 0;
    uint32_t timeout;
    
    for (i = 0; i < 8; i++)
    {
        byte <<= 1;
        
        // 等待每个bit的起始低电平结束（50us低电平）
        timeout = 100000;
        while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == RESET && timeout--)
        {
            if (timeout == 0)
                return 0xFF;
        }
        
        // 延时约40us后采样
        Delay_us(40);
        
        // 此时采样，如果是高电平则为1，否则为0
        if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET)
        {
            byte |= 1; // 将最低位置为1
        }
        
        // 等待这个bit的高电平结束
        timeout = 100000;
        while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET && timeout--)
        {
            if (timeout == 0)
                return 0xFF;
        }
    }
    
    return byte;
}

/**
 * @brief  读取DHT11数据
 * @param  data: DHT11数据结构体指针
 * @retval 0: 成功, 1: 失败
 */
uint8_t DHT11_ReadData(DHT11_DataTypeDef *data)
{
    uint8_t buffer[5];
    uint8_t i;
    
    // 发送起始信号
    DHT11_Start();
    
    // 检查响应
    if (DHT11_CheckResponse() != 0)
    {
        return 1; // 响应失败
    }
    
    // 读取40位数据（5个字节）
    for (i = 0; i < 5; i++)
    {
        buffer[i] = DHT11_ReadByte();
        if (buffer[i] == 0xFF)
            return 1; // 读取失败
    }
    
    // 校验数据
    if ((buffer[0] + buffer[1] + buffer[2] + buffer[3]) != buffer[4])
    {
        return 1; // 校验失败
    }
    
    // 保存数据
    data->humidity = buffer[0];
    data->humidity_dec = buffer[1];
    data->temperature = buffer[2];
    data->temperature_dec = buffer[3];
    data->checksum = buffer[4];
    
    return 0; // 成功
}


