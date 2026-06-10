#ifndef __UARTCOMM_H
#define __UARTCOMM_H

#include "stm32f10x.h"
#include <stdint.h>

/* ==================== 引脚定义 ==================== */
#define UARTCOMM_USART        USART1
#define UARTCOMM_BAUDRATE     115200
#define UARTCOMM_RCC_USART    RCC_APB2Periph_USART1
#define UARTCOMM_RCC_GPIO     RCC_APB2Periph_GPIOA
#define UARTCOMM_PORT         GPIOA
#define UARTCOMM_TX_PIN       GPIO_Pin_9
#define UARTCOMM_RX_PIN       GPIO_Pin_10

/* ==================== RX 环缓冲区 ==================== */
#define UART_RX_BUF_SIZE      64

/* ==================== 数据帧结构体 ==================== */
typedef struct
{
    float    temperature;
    float    humidity;
    uint16_t mq2_value;
    uint8_t  pir_status;
    uint8_t  flame_status;
    uint8_t  servo_status;
    uint8_t  fan_speed;
    uint16_t light_value;       /* 光敏 ADC 值 (0~4095) */
    uint8_t  led_mode;          /* 0=手动, 1=自动 */
    uint8_t  led_brightness;    /* LED 亮度 0~100 */
} SensorFrame;

/* ==================== 接收命令结构 ==================== */
typedef enum {
    CMD_NONE = 0,
    CMD_FAN_SPEED,            /* FAN:0 ~ FAN:100 */
    CMD_LED_AUTO,             /* LED:AUTO */
    CMD_LED_MANUAL,           /* LED:MAN */
    CMD_LED_BRIGHTNESS,       /* LED:0 ~ LED:100 */
    CMD_SERVO,               /* SRV:0 关门, SRV:1 开门 */
    CMD_AUTH_ADD,            /* AUTH:ADD:XXXXXXXX */
    CMD_AUTH_DEL,            /* AUTH:DEL:XXXXXXXX */
    CMD_AUTH_CLR,            /* AUTH:CLR */
} UartCmdType;

typedef struct
{
    UartCmdType type;
    uint8_t     value;
    uint8_t     auth_uid[4];  /* AUTH:ADD/DEL 的卡号 */
} UartCommand;

/* ==================== API ==================== */
void UartComm_Init(void);
void UartComm_SendFrame(const SensorFrame *frame);
void UartComm_SendByte(uint8_t byte);
void UartComm_SendString(const char *str);

void    UartComm_RX_IRQ(void);
uint8_t UartComm_GetCommand(UartCommand *cmd);

#endif /* __UARTCOMM_H */
