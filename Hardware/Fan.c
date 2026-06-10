#include "Fan.h"

// �����ڲ�ȫ�ֱ�������¼��ǰ���ȵķ���Ĭ����ת��
static uint8_t g_fan_dir = FAN_FORWARD;

void Fan_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 1. ʹ��ʱ��
    RCC_APB2PeriphClockCmd(FAN_RCC, ENABLE);           
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 
    
    // 2. ���� JTAG���ͷ� PB4���ǳ���ȷ����
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    // 3. TIM3 ������ӳ��
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);
    
    // 4. ���� GPIO - PB4 �� PB5
    GPIO_InitStructure.GPIO_Pin = FAN_INA_PIN | FAN_INB_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(FAN_INA_PORT, &GPIO_InitStructure);
    
    // 5. ���� TIM3 ʱ�� (20kHz)
    TIM_TimeBaseStructure.TIM_Period = 100 - 1;        
    TIM_TimeBaseStructure.TIM_Prescaler = 36 - 1;      
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 6. ���� TIM3 CH1 (PB4)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    // 7. ���� TIM3 CH2 (PB5)
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    // ���Ż��������ñȽϼĴ������㣬ȷ���ϵ粻����
    TIM_SetCompare1(TIM3, 0);
    TIM_SetCompare2(TIM3, 0);
    
    // 8. ʹ�� TIM3
    TIM_Cmd(TIM3, ENABLE);
    
    // 9. ��ʼ����״̬
    g_fan_dir = FAN_FORWARD;
}

/**
 * @brief  ���÷����ٶȣ��Զ����䵱ǰ����
 * @param  speed: �ٶ�ֵ 0-100%
 */
void Fan_SetSpeed(uint8_t speed)
{
    if (speed > 100)
        speed = 100;
    
    if (g_fan_dir == FAN_FORWARD)
    {
        // ��ת��INA(CH1) ��� PWM��INB(CH2) ���ֵ͵�ƽ
        TIM_SetCompare1(TIM3, speed);
        TIM_SetCompare2(TIM3, 0);
    }
    else
    {
        // ��ת��INA(CH1) ���ֵ͵�ƽ��INB(CH2) ��� PWM
        TIM_SetCompare1(TIM3, 0);
        TIM_SetCompare2(TIM3, speed);
    }
}

void Fan_On(void)
{
    Fan_SetSpeed(100);  
}

void Fan_Off(void)
{
    TIM_SetCompare1(TIM3, 0);  
    TIM_SetCompare2(TIM3, 0);  
}

/**
 * @brief  ���÷��ȷ�������Ӧ��ǰ�����ٶȣ�
 * @param  dir: FAN_FORWARD(��ת) �� FAN_REVERSE(��ת)
 */
void Fan_SetDirection(uint8_t dir)
{
    uint16_t current_speed;
    
    // ��ȡ��ǰ����������Ǹ�ͨ�����ٶ�
    if (g_fan_dir == FAN_FORWARD)
    {
        current_speed = TIM_GetCapture1(TIM3);
    }
    else
    {
        current_speed = TIM_GetCapture2(TIM3);
    }
    
    // �����ǰ��ֹͣ״̬��Ĭ�ϸ��� 100% �ٶ�
    if (current_speed == 0)
    {
        current_speed = 100;  
    }
    
    // ����ȫ�ַ���
    g_fan_dir = dir;
    
    // ����Ӧ���ٶȺ��·���
    Fan_SetSpeed(current_speed);
}