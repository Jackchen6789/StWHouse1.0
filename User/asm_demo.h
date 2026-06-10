/**
 * @file    asm_demo.h
 * @brief   ARM Cortex-M3 汇编演示模块——用于课设答辩展示底层能力
 * @note    本模块完全不修改任何现有业务代码，独立编译链接
 *          所有函数使用 ARMCC (Keil MDK) 内联汇编语法
 */

#ifndef __ASM_DEMO_H
#define __ASM_DEMO_H

#include "stm32f10x.h"

/* === 堆栈指针读取 === */
uint32_t Asm_GetMSP(void);   /* 读主堆栈指针 (MSP) */
uint32_t Asm_GetPSP(void);   /* 读进程堆栈指针 (PSP) */

/* === 原子操作 === */
uint32_t Asm_AtomicSwap(volatile uint32_t *ptr, uint32_t new_val);

/* === 临界区保护 === */
uint32_t Asm_EnterCritical(void);   /* 关中断, 返回旧 PRIMASK */
void     Asm_ExitCritical(uint32_t primask); /* 恢复中断状态 */

/* === 位操作指令 === */
uint32_t Asm_CLZ(uint32_t val);     /* 前导零计数 (CLZ 指令) */
uint32_t Asm_RBIT(uint32_t val);    /* 位反转 (RBIT 指令) */

/* === 快速数学 === */
uint32_t Asm_UnsignedDiv10(uint32_t n);  /* 除以 10 (魔数乘法) */

/* === 异常/中断演示 === */
void Asm_TriggerPendSV(void);       /* 软件触发 PendSV 异常 */

/* === 启动流程打印 (调试用) === */
void Asm_PrintCpuInfo(void);        /* 通过串口输出 CPU 寄存器信息 */

#endif /* __ASM_DEMO_H */
