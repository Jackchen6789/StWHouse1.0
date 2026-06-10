#ifndef __DEBUGUART_H
#define __DEBUGUART_H

#include "stm32f10x.h"
#include <stdint.h>

/* ==================== 引脚定义 ====================
 * USART3 独立调试串口，与 USART1（接ESP32）完全隔离
 * PB10 = TX  → USB-TTL RX
 * PB11 = RX  → USB-TTL TX
 * 波特率 115200 8N1
 */
#define DEBUG_USART             USART3
#define DEBUG_BAUDRATE          115200
#define DEBUG_RCC_USART         RCC_APB1Periph_USART3
#define DEBUG_RCC_GPIO          RCC_APB2Periph_GPIOB
#define DEBUG_PORT              GPIOB
#define DEBUG_TX_PIN            GPIO_Pin_10
#define DEBUG_RX_PIN            GPIO_Pin_11

/* ==================== API ==================== */
void DebugUart_Init(void);
void DebugUart_SendByte(uint8_t byte);
void DebugUart_SendString(const char *str);

#endif /* __DEBUGUART_H */
