// delay.h
#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void Delay_Init(void);      // ≥ı ºªØSysTick
void Delay_us(uint32_t us); // Œ¢√Î—” ±
void Delay_ms(uint32_t ms); // ∫¡√Î—” ±

#endif

