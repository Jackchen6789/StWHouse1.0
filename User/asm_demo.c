/**
 * @file    asm_demo.c
 * @brief   ARM Cortex-M3 汇编演示模块 (ARMCC V5 / Keil MDK)
 *
 *  全部使用 __asm 裸函数形式, 兼容 ARMCC V5 编译器。
 *  参数: R0, R1, R2, R3   返回值: R0
 *  可自由使用 R0-R3, R4-R11 需入栈保护。
 */

#include "asm_demo.h"
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  1. 堆栈指针读取 —— MRS 指令
 *  ═══════════════════════════════════════════════════════════════════════════
 *  访问 CPU 内部寄存器, 纯 C 语言无法实现。
 */

__asm uint32_t Asm_GetMSP(void)
{
    MRS R0, MSP
    BX  LR
}

__asm uint32_t Asm_GetPSP(void)
{
    MRS R0, PSP
    BX  LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  2. 原子交换 —— LDREX / STREX 独占访问
 *  ═══════════════════════════════════════════════════════════════════════════
 *  参数: R0 = ptr, R1 = new_val
 *  返回: R0 = old_val
 *  STREX 成功时 R3=0, 失败时 R3=1, 失败则循环重试。
 */

__asm uint32_t Asm_AtomicSwap(volatile uint32_t *ptr, uint32_t new_val)
{
    ; R0 = ptr, R1 = new_val
try_swap
    LDREX   R2, [R0]       ; 独占加载旧值
    STREX   R3, R1, [R0]   ; 尝试独占存储新值, R3=0 成功
    CMP     R3, #0
    BNE     try_swap       ; 失败则重试
    MOV     R0, R2         ; 返回旧值
    BX      LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  3. 临界区保护 —— CPSID / CPSIE
 *  ═══════════════════════════════════════════════════════════════════════════
 */

__asm uint32_t Asm_EnterCritical(void)
{
    MRS  R0, PRIMASK
    CPSID I
    BX   LR
}

__asm void Asm_ExitCritical(uint32_t primask)
{
    MSR  PRIMASK, R0
    BX   LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  4. 前导零计数 —— CLZ 指令 (单周期)
 *  ═══════════════════════════════════════════════════════════════════════════
 */

__asm uint32_t Asm_CLZ(uint32_t val)
{
    CLZ  R0, R0
    BX   LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  5. 位反转 —— RBIT 指令
 *  ═══════════════════════════════════════════════════════════════════════════
 */

__asm uint32_t Asm_RBIT(uint32_t val)
{
    RBIT R0, R0
    BX   LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  6. 快速除以10 —— 魔数乘法
 *  ═══════════════════════════════════════════════════════════════════════════
 *  n/10 = (n * 0xCCCCCCCD) >> 35
 *  UMULL 产生 64 位结果: [R1:R0] = R0 * R2
 */

__asm uint32_t Asm_UnsignedDiv10(uint32_t n)
{
    ; R0 = n
    LDR     R2, =0xCCCCCCCD
    UMULL   R0, R1, R2, R0  ; R1:R0 = n * 魔数
    MOV     R0, R1           ; 取高32位
    LSR     R0, R0, #3       ; 再右移3位, 相当于 >> 35
    BX      LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  7. 触发 PendSV —— 写 ICSR 寄存器
 *  ═══════════════════════════════════════════════════════════════════════════
 *  ICSR = 0xE000ED04, PENDSVSET = bit28 = 0x10000000
 */

__asm void Asm_TriggerPendSV(void)
{
    LDR     R0, =0xE000ED04
    LDR     R1, [R0]
    ORR     R1, R1, #0x10000000
    STR     R1, [R0]
    BX      LR
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  8. CPU 信息打印
 *  ═══════════════════════════════════════════════════════════════════════════
 */

void Asm_PrintCpuInfo(void)
{
    uint32_t msp = Asm_GetMSP();
    uint32_t psp = Asm_GetPSP();
    uint32_t clz_demo = Asm_CLZ(0x00000FFF);
    uint32_t rbit_demo = Asm_RBIT(0x80000000);
    uint32_t div_demo = Asm_UnsignedDiv10(1234);

    printf("\r\n=== ARM Cortex-M3 CPU Info (asm_demo) ===\r\n");
    printf("MSP        = 0x%08X\r\n", (unsigned int)msp);
    printf("PSP        = 0x%08X\r\n", (unsigned int)psp);
    printf("CLZ(FFF)   = %u  (expected:20)\r\n", (unsigned int)clz_demo);
    printf("RBIT(8...0)= 0x%08X (expected:0x00000001)\r\n", (unsigned int)rbit_demo);
    printf("1234/10    = %u  (expected:123)\r\n", (unsigned int)div_demo);
    printf("===========================================\r\n\r\n");
}
