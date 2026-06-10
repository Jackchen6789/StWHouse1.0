#ifndef __FAN_H
#define __FAN_H

#include "stm32f10x.h"

/* ======================================================================== */
/*                             L9110 引脚与时钟配置                          */
/* ======================================================================== */
/* 注意：PB4 默认是 JNTRST 引脚，代码内部已做 JTAG 禁用与 TIM3 部分重映射 */

#define FAN_RCC            RCC_APB2Periph_GPIOB
#define FAN_INA_PORT       GPIOB
#define FAN_INA_PIN        GPIO_Pin_4      // TIM3_CH1 (部分重映射后)
#define FAN_INB_PORT       GPIOB
#define FAN_INB_PIN        GPIO_Pin_5      // TIM3_CH2 (部分重映射后)

/* ======================================================================== */
/*                               风扇方向宏定义                             */
/* ======================================================================== */

#define FAN_FORWARD        0               // 正转 (INA=PWM, INB=0)
#define FAN_REVERSE        1               // 反转 (INA=0,   INB=PWM)

/* ======================================================================== */
/*                                 API 函数声明                             */
/* ======================================================================== */

/**
 * @brief  初始化L9110风扇驱动
 * @note   配置 TIM3 产生 20kHz 超声波 PWM（分频36，重装载100），
 *         自动释放 PB4 的 JTAG 复用，并配置 TIM3 部分重映射。
 */
void Fan_Init(void);

/**
 * @brief  设置风扇速度（自适应当前方向）
 * @param  speed: 速度占空比，范围 0 - 100
 */
void Fan_SetSpeed(uint8_t speed);

/**
 * @brief  开启风扇
 * @note   让风扇在当前设定方向下以 100% 全速运转
 */
void Fan_On(void);

/**
 * @brief  关闭风扇
 * @note   使通道1和通道2的比较值同时清零，实现电机刹车停止
 */
void Fan_Off(void);

/**
 * @brief  切换风扇旋转方向
 * @param  dir: 目标方向，可选 FAN_FORWARD 或 FAN_REVERSE
 * @note   切换时会自动读取当前通道的 PWM 速度，无缝继承到新方向。
 *         若当前风扇处于停止状态（速度为0），切换后默认以 100% 速度启动。
 */
void Fan_SetDirection(uint8_t dir);

#endif /* __FAN_H */