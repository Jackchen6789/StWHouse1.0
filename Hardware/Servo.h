#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

// 舵机引脚定义
#define SERVO_RCC    RCC_APB2Periph_GPIOA
#define SERVO_PORT   GPIOA
#define SERVO_PIN    GPIO_Pin_4

// 函数声明
void Servo_Init(void);
void Servo_Lock(void);   // 关锁 (0度)
void Servo_Unlock(void); // 开锁 (180度)

#endif /* __SERVO_H */


