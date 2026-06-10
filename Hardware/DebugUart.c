#include "DebugUart.h"

/* ======================================================================== */
/*                USART3 纯寄存器直写（不调标准库，排除库bug）                 */
/* ======================================================================== */
void DebugUart_Init(void)
{
    /* 1. 开时钟：直接写 RCC 寄存器 */
    RCC->APB2ENR |= (1UL << 3);    /* GPIOB */
    RCC->APB1ENR |= (1UL << 18);   /* USART3 */

    /* 2. PB10 = AF推挽 50MHz
     *    GPIOB_CRH bit[11:8]: CNF=10(AF) MODE=11(50MHz) = 0xB */
    GPIOB->CRH &= ~(0xFUL << 8);
    GPIOB->CRH |=  (0xBUL << 8);

    /* 3. USART3 115200-8N1 (PCLK1=36MHz, DIV=19.53, BRR=0x138) */
    USART3->BRR = 0x138;
    USART3->CR1 = 0x000C;   /* TE + RE */
    USART3->CR2 = 0;
    USART3->CR3 = 0;

    /* 4. 使能 USART3 */
    USART3->CR1 |= (1UL << 13);   /* UE */
}

void DebugUart_SendByte(uint8_t byte)
{
    USART3->DR = byte;
    for (volatile uint32_t i = 0; i < 72000; i++) { __NOP(); }
}

void DebugUart_SendString(const char *str)
{
    while (*str) {
        DebugUart_SendByte((uint8_t)*str++);
    }
}
