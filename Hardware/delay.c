// delay.c
#include "delay.h"

static volatile uint32_t usTicks = 0; // 1微秒所需的时钟周期数

void Delay_Init(void)
{
    // 系统时钟频率 (Hz)
    uint32_t sysclk = SystemCoreClock;
    // 计算每微秒的时钟周期数
    usTicks = sysclk / 1000000;
    
    // 配置并启动 SysTick 定时器
    // SysTick 使用系统时钟，向下计数
    SysTick->LOAD = sysclk / 1000 - 1;  // 1ms 重装载值
    SysTick->VAL = 0;                    // 清空当前计数值
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  // 使用处理器时钟
                    SysTick_CTRL_ENABLE_Msk;      // 使能 SysTick
}

void Delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;

    ticks = us * usTicks; // 需要达到的计数
    told = SysTick->VAL;  // 当前计数值
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            // 计算经过的计数 (考虑向下计数)
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break;
        }
    }
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
        Delay_us(1000);
}


