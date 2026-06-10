#ifndef __FLAMESENSOR_H
#define __FLAMESENSOR_H

#include "stm32f10x.h"

#define FLAME_RCC   RCC_APB2Periph_GPIOA
#define FLAME_PORT  GPIOA
#define FLAME_PIN   GPIO_Pin_2

void Flame_Init(void);
uint8_t Flame_Get_Status(void);

#endif /* __FLAME_H */




