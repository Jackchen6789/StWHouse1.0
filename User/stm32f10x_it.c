/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  ******************************************************************************
  */

#include "stm32f10x_it.h"
#include "UartComm.h"

void NMI_Handler(void) {}

void HardFault_Handler(void) { while(1); }

void MemManage_Handler(void) { while(1); }
void BusFault_Handler(void)  { while(1); }
void UsageFault_Handler(void){ while(1); }
void SVC_Handler(void)       {}
void DebugMon_Handler(void)  {}
void PendSV_Handler(void)    {}
void SysTick_Handler(void)   {}

/**
  * @brief  USART1 接收中断: 委托给 UartComm 模块处理
  */
void USART1_IRQHandler(void)
{
    UartComm_RX_IRQ();
}
