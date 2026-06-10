// dht11.h
#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"
#include "delay.h"

// 使用 PA0 作为数据引脚 (可根据需要修改)
#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_Pin_0
#define DHT11_RCC  RCC_APB2Periph_GPIOA

typedef struct
{
    uint8_t humidity;        // 整数部分
    uint8_t humidity_dec;    // 小数部分 (通常为0)
    uint8_t temperature;     // 整数部分
    uint8_t temperature_dec; // 小数部分
    uint8_t checksum;
} DHT11_DataTypeDef;

void DHT11_Init(void);
uint8_t DHT11_ReadData(DHT11_DataTypeDef *data);

#endif


