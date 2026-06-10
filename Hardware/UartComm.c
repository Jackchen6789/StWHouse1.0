#include "UartComm.h"
#include <stdio.h>
#include <string.h>

/* ======================================================================== */
/*                           RX 环缓冲区                                    */
/* ======================================================================== */
static volatile uint8_t  rx_buf[UART_RX_BUF_SIZE];
static volatile uint8_t  rx_head = 0;
static volatile uint8_t  rx_tail = 0;

static char    line_buf[32];
static uint8_t line_idx = 0;

static volatile UartCommand g_pending_cmd;
static volatile uint8_t     g_cmd_ready = 0;

/* ======================================================================== */
/*                            USART1 初始化                                  */
/* ======================================================================== */
void UartComm_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(UARTCOMM_RCC_GPIO | UARTCOMM_RCC_USART, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = UARTCOMM_TX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UARTCOMM_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = UARTCOMM_RX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(UARTCOMM_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = UARTCOMM_BAUDRATE;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(UARTCOMM_USART, &USART_InitStructure);

    USART_ITConfig(UARTCOMM_USART, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(UARTCOMM_USART, ENABLE);
}

/* ======================================================================== */
/*                            发送函数                                      */
/* ======================================================================== */
void UartComm_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(UARTCOMM_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(UARTCOMM_USART, byte);
}

void UartComm_SendString(const char *str)
{
    while (*str)
        UartComm_SendByte((uint8_t)*str++);
}

/**
 * 数据帧: T:25.5;H:62.0;MQ:0847;PIR:1;FLM:0;SRV:1;FAN:075;LGHT:2048;LEDM:1;LEDB:050\r\n
 */
void UartComm_SendFrame(const SensorFrame *frame)
{
    char buf[80];
    snprintf(buf, sizeof(buf),
             "T:%.1f;H:%.1f;MQ:%04u;PIR:%u;FLM:%u;SRV:%u;FAN:%03u;LGHT:%04u;LEDM:%u;LEDB:%03u\r\n",
             frame->temperature,
             frame->humidity,
             frame->mq2_value,
             frame->pir_status,
             frame->flame_status,
             frame->servo_status,
             frame->fan_speed,
             frame->light_value,
             frame->led_mode,
             frame->led_brightness);
    UartComm_SendString(buf);
}

/* ======================================================================== */
/*                         RX 中断服务                                      */
/* ======================================================================== */
void UartComm_RX_IRQ(void)
{
    uint8_t ch;

    if (USART_GetITStatus(UARTCOMM_USART, USART_IT_RXNE) != RESET)
    {
        ch = (uint8_t)USART_ReceiveData(UARTCOMM_USART);

        if (ch == '\n' || ch == '\r')
        {
            if (line_idx > 0)
            {
                line_buf[line_idx] = '\0';

                /* FAN:0 ~ FAN:100 */
                if (strncmp(line_buf, "FAN:", 4) == 0)
                {
                    int val = 0;
                    sscanf(line_buf + 4, "%d", &val);
                    if (val < 0)   val = 0;
                    if (val > 100) val = 100;
                    g_pending_cmd.type  = CMD_FAN_SPEED;
                    g_pending_cmd.value = (uint8_t)val;
                    g_cmd_ready = 1;
                }
                /* LED:AUTO */
                else if (strcmp(line_buf, "LED:AUTO") == 0)
                {
                    g_pending_cmd.type  = CMD_LED_AUTO;
                    g_pending_cmd.value = 0;
                    g_cmd_ready = 1;
                }
                /* LED:MAN */
                else if (strcmp(line_buf, "LED:MAN") == 0)
                {
                    g_pending_cmd.type  = CMD_LED_MANUAL;
                    g_pending_cmd.value = 0;
                    g_cmd_ready = 1;
                }
                /* LED:0 ~ LED:100 (必须在 LED:AUTO/MAN 之后判断, 避免误匹配) */
                else if (strncmp(line_buf, "LED:", 4) == 0)
                {
                    int val = 0;
                    sscanf(line_buf + 4, "%d", &val);
                    if (val < 0)   val = 0;
                    if (val > 100) val = 100;
                    g_pending_cmd.type  = CMD_LED_BRIGHTNESS;
                    g_pending_cmd.value = (uint8_t)val;
                    g_cmd_ready = 1;
                }
                /* SRV:0 ??, SRV:1 ?? */
                else if (strncmp(line_buf, "SRV:", 4) == 0)
                {
                    int val = 0;
                    sscanf(line_buf + 4, "%d", &val);
                    val = (val != 0) ? 1 : 0;
                    g_pending_cmd.type  = CMD_SERVO;
                    g_pending_cmd.value = (uint8_t)val;
                    g_cmd_ready = 1;
                }
                /* AUTH:ADD:XXXXXXXX ???? */
                else if (strncmp(line_buf, "AUTH:ADD:", 9) == 0)
                {
                    unsigned int u0, u1, u2, u3;
                    if (sscanf(line_buf + 9, "%02X%02X%02X%02X", &u0, &u1, &u2, &u3) == 4)
                    {
                        g_pending_cmd.type = CMD_AUTH_ADD;
                        g_pending_cmd.auth_uid[0] = (uint8_t)u0;
                        g_pending_cmd.auth_uid[1] = (uint8_t)u1;
                        g_pending_cmd.auth_uid[2] = (uint8_t)u2;
                        g_pending_cmd.auth_uid[3] = (uint8_t)u3;
                        g_cmd_ready = 1;
                    }
                }
                /* AUTH:DEL:XXXXXXXX ???? */
                else if (strncmp(line_buf, "AUTH:DEL:", 9) == 0)
                {
                    unsigned int u0, u1, u2, u3;
                    if (sscanf(line_buf + 9, "%02X%02X%02X%02X", &u0, &u1, &u2, &u3) == 4)
                    {
                        g_pending_cmd.type = CMD_AUTH_DEL;
                        g_pending_cmd.auth_uid[0] = (uint8_t)u0;
                        g_pending_cmd.auth_uid[1] = (uint8_t)u1;
                        g_pending_cmd.auth_uid[2] = (uint8_t)u2;
                        g_pending_cmd.auth_uid[3] = (uint8_t)u3;
                        g_cmd_ready = 1;
                    }
                }
                /* AUTH:CLR ?????? */
                else if (strcmp(line_buf, "AUTH:CLR") == 0)
                {
                    g_pending_cmd.type = CMD_AUTH_CLR;
                    g_cmd_ready = 1;
                }
                line_idx = 0;
            }
        }
        else if (line_idx < sizeof(line_buf) - 1)
        {
            line_buf[line_idx++] = ch;
        }
    }
}

/* ======================================================================== */
/*                    主循环取出命令                                        */
/* ======================================================================== */
uint8_t UartComm_GetCommand(UartCommand *cmd)
{
    if (g_cmd_ready)
    {
        *cmd = g_pending_cmd;
        g_cmd_ready = 0;
        return 1;
    }
    return 0;
}
