/**
 * @file    main.c
 * @brief   STM32F103C8T6 仓瞳主控
 *          RFID 切换门锁 | 风扇/灯光 ESP32 远程控制 | 光敏自动调光 | UART 上报
 */

#include "stm32f10x.h"
#include <stdio.h>

#include "delay.h"
#include "OLED_I2C.h"
#include "DHT11.h"
#include "MQ2.h"
#include "HC_SR501.h"
#include "FlameSensor.h"
#include "RCC522.h"
#include "Fan.h"
#include "Buzzer.h"
#include "Servo.h"
#include "UartComm.h"
#include "LightSensor.h"
#include "LedLight.h"
#include "asm_demo.h"
/* ======================================================================== */
/*                             全局状态变量                                  */
/* ======================================================================== */
static DHT11_DataTypeDef g_dht11;
static SensorFrame      g_frame;

static uint8_t g_door_open = 0;
static uint8_t g_fan_speed = 0;

/* ── LED 控制 ── */
static uint8_t g_led_mode       = 1;    /* 0=手动, 1=自动 (默认自动) */
static uint8_t g_led_brightness = 0;    /* 0~100 */

/* ── RFID 防抖 ── */
static uint8_t  g_card_active = 0;
static uint8_t  g_card_prev   = 0;

/* ── 蜂鸣器告警 ── */
static uint8_t  g_alarm_tick  = 0;    /* 脉冲翻转计数器 */

static const uint8_t AUTH_CARD[4] = {0x81, 0xF4, 0x2C, 0x07};

/* ?????? (??10??) */
#define AUTH_MAX 10
static uint8_t g_auth_cards[AUTH_MAX][4];
static uint8_t g_auth_count = 0;

static uint8_t RFID_IsAuthCard(const uint8_t *uid)
{
    uint8_t i;
    /* ????? */
    if (uid[0] == AUTH_CARD[0] && uid[1] == AUTH_CARD[1] &&
        uid[2] == AUTH_CARD[2] && uid[3] == AUTH_CARD[3])
        return 1;
    /* ???????? */
    for (i = 0; i < g_auth_count; i++)
    {
        if (uid[0] == g_auth_cards[i][0] && uid[1] == g_auth_cards[i][1] &&
            uid[2] == g_auth_cards[i][2] && uid[3] == g_auth_cards[i][3])
            return 1;
    }
    return 0;
}

/* ======================================================================== */
/*                            内部函数声明                                   */
/* ======================================================================== */
static void System_InitAll(void);
static void Sensor_ReadAll(void);
static void LED_AutoControl(uint16_t light_adc);
static uint8_t RFID_TryRead(uint8_t *uid);
static void Door_Toggle(void);

/* ======================================================================== */
/*                         系统初始化                                       */
/* ======================================================================== */
static void System_InitAll(void)
{
    SystemInit();
    Delay_Init();
    UartComm_Init();
    OLED_Init();
    DHT11_Init();
    MQ2_Init();
    PIR_Init();
    Flame_Init();
    RC522_Init();
    Fan_Init();
    Buzzer_Init();
    Servo_Init();
    LightSensor_Init();
    LedLight_Init();
    // Asm_PrintCpuInfo();  // 答辩演示时取消注释即可

    // ═══════════════════════════════════════════════════════════════
    // 【答辩演示】取消下面注释, 编译烧录后系统会故意触发 HardFault
    // 串口助手将输出完整的崩溃诊断报告, 然后自动复位恢复正常运行
    //     *(volatile uint32_t *)0 = 0;  // 向地址 0 写数据 → BusFault
    // ═══════════════════════════════════════════════════════════════

    Servo_Lock();
    Fan_Off();
    Buzzer_Off();
    LedLight_Off();
    g_door_open       = 0;
    g_fan_speed       = 0;
    g_led_mode        = 1;    /* 默认自动模式 */
    g_led_brightness  = 0;
}

/* ======================================================================== */
/*                          传感器采集                                      */
/* ======================================================================== */
static void Sensor_ReadAll(void)
{
    DHT11_ReadData(&g_dht11);
    g_frame.temperature  = (float)g_dht11.temperature + (float)g_dht11.temperature_dec * 0.1f;
    g_frame.humidity     = (float)g_dht11.humidity     + (float)g_dht11.humidity_dec     * 0.1f;
    g_frame.mq2_value    = MQ2_Get_Average(10);
    g_frame.pir_status   = PIR_Get_Status();
    g_frame.flame_status = Flame_Get_Status();
    g_frame.light_value  = LightSensor_GetValue();
}

/* ======================================================================== */
/*               自动调光: ADC 值越大(越暗) → LED 越亮                       */
/* ======================================================================== */
static void LED_AutoControl(uint16_t light_adc)
{
    uint8_t brightness;

    /**
     * 光敏电阻特性: 暗 → 阻值大 → ADC 读数大;  亮 → 阻值小 → ADC 读数小
     * 映射规则 (可实际标定后调整):
     *   ADC < 500   → 非常亮 → LED 0%   (关)
     *   ADC 500~1500 → 稍暗   → LED 10%~30%
     *   ADC 1500~2500 → 较暗  → LED 30%~60%
     *   ADC 2500~3500 → 很暗  → LED 60%~90%
     *   ADC > 3500  → 极暗   → LED 100%
     */
    if (light_adc < 500)
        brightness = 0;
    else if (light_adc < 1500)
        brightness = 10 + (uint8_t)((light_adc - 500) * 20 / 1000);   /* 10~30 */
    else if (light_adc < 2500)
        brightness = 30 + (uint8_t)((light_adc - 1500) * 30 / 1000);  /* 30~60 */
    else if (light_adc < 3500)
        brightness = 60 + (uint8_t)((light_adc - 2500) * 30 / 1000);  /* 60~90 */
    else
        brightness = 100;

    if (brightness > 100) brightness = 100;

    g_led_brightness = brightness;
    LedLight_SetBrightness(brightness);
}

/* ======================================================================== */
/*                     蜂鸣器告警检测                                       */
/* ======================================================================== */
static void Alarm_CheckAndBeep(void)
{
    uint8_t gas   = MQ2_DO_GetStatus();
    uint8_t flame = Flame_Get_Status();

    if (gas || flame)
    {
        g_alarm_tick = !g_alarm_tick;
        if (g_alarm_tick)
            Buzzer_On();
        else
            Buzzer_Off();
    }
    else
    {
        g_alarm_tick = 0;
        Buzzer_Off();
    }
}

/* ======================================================================== */
/*                         RFID 函数                                        */
/* ======================================================================== */
static uint8_t RFID_TryRead(uint8_t *uid)
{
    uint8_t tagType[2];
    if (RC522_Request(REQ_ALL, tagType) != MI_OK)
        return MI_NOTAGERR;
    return RC522_Anticoll(uid);
}

/* ======================================================================== */
/*                      门锁切换                                            */
/* ======================================================================== */
static void Door_Toggle(void)
{
    if (g_door_open)
    {
        Servo_Lock();
        g_door_open = 0;
    }
    else
    {
        Servo_Unlock();
        g_door_open = 1;
    }
    Buzzer_On();  Delay_ms(80);
    Buzzer_Off();
}

/* ======================================================================== */
/*                          OLED 显示                                       */
/* ======================================================================== */
static void OLED_ShowCardUI(const uint8_t *uid, uint8_t is_auth)
{
    char line[17];

    OLED_ShowString(1, 1, "Card Detected!  ");
    snprintf(line, sizeof(line), "ID:%02X%02X%02X%02X",
             uid[0], uid[1], uid[2], uid[3]);
    OLED_ShowString(2, 1, line);

    snprintf(line, sizeof(line), "LUX:%04d LED:%03d",
             g_frame.light_value, g_led_brightness);
    OLED_ShowString(3, 1, line);

    if (is_auth)
        OLED_ShowString(4, 1, g_door_open ? "DOOR:OPEN       " : "DOOR:LOCKED     ");
    else
        OLED_ShowString(4, 1, "UNAUTHORIZED!!! ");

    OLED_Refresh();
}

static void OLED_ShowMonitorUI(void)
{
    char line[17];
    uint8_t gas   = MQ2_DO_GetStatus();
    uint8_t flame = Flame_Get_Status();

    /* 告警优先显示 */
    if (gas && flame)
        OLED_ShowString(1, 1, "GAS+FIRE ALARM! ");
    else if (gas)
        OLED_ShowString(1, 1, "GAS ALARM!      ");
    else if (flame)
        OLED_ShowString(1, 1, "FIRE ALARM!     ");
    else if (g_frame.pir_status)
        OLED_ShowString(1, 1, "WARNING: Human! ");
    else
        OLED_ShowString(1, 1, "SYS MONITOR     ");

    snprintf(line, sizeof(line), "T:%.1f FAN:%03d  ",
             g_frame.temperature, g_fan_speed);
    OLED_ShowString(2, 1, line);

    snprintf(line, sizeof(line), "LUX:%04d LED:%03d",
             g_frame.light_value, g_led_brightness);
    OLED_ShowString(3, 1, line);

    if (g_frame.pir_status)
        OLED_ShowString(4, 1, "PIR STAT: ACTIVE");
    else
        OLED_ShowString(4, 1, g_led_mode ? "LED MODE: AUTO  " : "LED MODE: MANUAL");

    OLED_Refresh();
}

/* ======================================================================== */
/*                  UART 命令处理                                           */
/* ======================================================================== */
static void UartCmd_Process(void)
{
    UartCommand cmd;
    while (UartComm_GetCommand(&cmd))
    {
        switch (cmd.type)
        {
        case CMD_FAN_SPEED:
            g_fan_speed = cmd.value;
            if (g_fan_speed == 0)
                Fan_Off();
            else
                Fan_SetSpeed(g_fan_speed);
            break;

        case CMD_LED_AUTO:
            g_led_mode = 1;
            break;

        case CMD_LED_MANUAL:
            g_led_mode = 0;
            break;

        case CMD_LED_BRIGHTNESS:
            /* 手动模式下才响应亮度指令 */
            if (g_led_mode == 0)
            {
                g_led_brightness = cmd.value;
                LedLight_SetBrightness(cmd.value);
            }
            break;

        case CMD_SERVO:
            if (cmd.value == 1)
            {
                Servo_Unlock();
                g_door_open = 1;
            }
            else
            {
                Servo_Lock();
                g_door_open = 0;
            }
            break;

        case CMD_AUTH_ADD:
            if (g_auth_count < AUTH_MAX)
            {
                g_auth_cards[g_auth_count][0] = cmd.auth_uid[0];
                g_auth_cards[g_auth_count][1] = cmd.auth_uid[1];
                g_auth_cards[g_auth_count][2] = cmd.auth_uid[2];
                g_auth_cards[g_auth_count][3] = cmd.auth_uid[3];
                g_auth_count++;
            }
            break;

        case CMD_AUTH_DEL:
        {
            uint8_t i, j;
            for (i = 0; i < g_auth_count; i++)
            {
                if (g_auth_cards[i][0] == cmd.auth_uid[0] &&
                    g_auth_cards[i][1] == cmd.auth_uid[1] &&
                    g_auth_cards[i][2] == cmd.auth_uid[2] &&
                    g_auth_cards[i][3] == cmd.auth_uid[3])
                {
                    for (j = i; j < g_auth_count - 1; j++)
                    {
                        g_auth_cards[j][0] = g_auth_cards[j+1][0];
                        g_auth_cards[j][1] = g_auth_cards[j+1][1];
                        g_auth_cards[j][2] = g_auth_cards[j+1][2];
                        g_auth_cards[j][3] = g_auth_cards[j+1][3];
                    }
                    g_auth_count--;
                    break;
                }
            }
            break;
        }

        case CMD_AUTH_CLR:
            g_auth_count = 0;
            break;

        default:
            break;
        }
    }
}

/* ======================================================================== */
/*                               主函数                                      */
/* ======================================================================== */
int main(void)
{
    uint8_t uid[4];
    uint8_t is_auth;

    System_InitAll();

    while (1)
    {
 
        /* ---- 第1步：采集所有传感器 ---- */
        Sensor_ReadAll();

        /* ---- 第1.5步：气体/火焰告警检测 ---- */
        Alarm_CheckAndBeep();

        /* ---- 第2步：自动调光 ---- */
        if (g_led_mode == 1)
            LED_AutoControl(g_frame.light_value);

        /* ---- 第3步：RFID 刷卡检测 ---- */
        g_card_prev = g_card_active;
        if (RFID_TryRead(uid) == MI_OK)
        {
            g_card_active = 1;
            is_auth = RFID_IsAuthCard(uid);
            if (!g_card_prev)
            {
                if (is_auth)
                    Door_Toggle();
                else
                {
                    Buzzer_On();  Delay_ms(100);
                    Buzzer_Off(); Delay_ms(100);
                    Buzzer_On();  Delay_ms(100);
                    Buzzer_Off();
                }
            }
            OLED_ShowCardUI(uid, is_auth);

            /* 发送卡号给 ESP32 */
            char card_msg[16];
            snprintf(card_msg, sizeof(card_msg), "CARD:%02X%02X%02X%02X\n", uid[0], uid[1], uid[2], uid[3]);
            UartComm_SendString(card_msg);
        }
        else
        {
            g_card_active = 0;
            OLED_ShowMonitorUI();
        }

        /* ---- 第4步：处理 ESP32 发来的 UART 命令 ---- */
        UartCmd_Process();

        /* ---- 第5步：发送数据帧 ---- */
        g_frame.servo_status   = g_door_open;
        g_frame.fan_speed      = g_fan_speed;
        g_frame.led_mode       = g_led_mode;
        g_frame.led_brightness = g_led_brightness;

        UartComm_SendFrame(&g_frame);
        Delay_ms(200);
    }
}
