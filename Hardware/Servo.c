#include "Servo.h"
#include "Delay.h"

/**
 * @brief  ��ʼ��PA4����Ϊ�������
 */
void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. ʹ�� GPIOA ʱ��
    RCC_APB2PeriphClockCmd(SERVO_RCC, ENABLE);
    
    // 2. ���� PA4 Ϊͨ���������
    GPIO_InitStructure.GPIO_Pin = SERVO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_PORT, &GPIO_InitStructure);
    
    // 3. ��ʼĬ�ϱ��ֹ���״̬
    Servo_Lock();
}

/**
 * @brief  ���ת�� 0 �� ���� ����
 */
void Servo_Lock(void)
{
    uint8_t i;
    for(i = 0; i < 20; i++)
    {
        GPIO_SetBits(SERVO_PORT, SERVO_PIN);
        Delay_us(500);
        GPIO_ResetBits(SERVO_PORT, SERVO_PIN);
        Delay_us(19500);
    }
}

/**
 * @brief  ���ת�� 180 �� ���� ����
 */
void Servo_Unlock(void)
{
    uint8_t i;
    for(i = 0; i < 20; i++)
    {
        GPIO_SetBits(SERVO_PORT, SERVO_PIN);
        Delay_us(2500);
        GPIO_ResetBits(SERVO_PORT, SERVO_PIN);
        Delay_us(17500);
    }
}


