# 仓瞳 (StWHouse) —— 基于 STM32 + ESP32 + MQTT 的智能仓库环境安防系统

> **St**orage **W**atch **House** · 守卫仓库的"眼睛"  
> STM32 边缘采集 → ESP32 MQTT 网桥 → EMQX 消息中间件 → Web 大屏消费

---

## 📖 项目概述

面向**中小型仓库**的环境监控与安防需求，构建从设备层到云端的完整物联网闭环：

```
传感器 ──▶ STM32F103C8T6 ──USART──▶ ESP32-S3 ──MQTT──▶ EMQX Broker ──▶ Web 大屏
   ▲                               ◀──USART──           ◀──MQTT──        控制指令
   │                                                                        │
   └────────────────── 执行器 (舵机·风扇·蜂鸣器·LED) ◀──────────────────────┘
```

**7 大核心能力：**

| # | 功能 | 实现 |
|:---|:---|:---|
| 1 | 环境感知 | DHT11 温湿度 + MQ2 烟雾 + 5516 光敏电阻 + 火焰传感器 |
| 2 | RFID 门禁 | RC522 刷卡，授权卡刷一次开门、再刷一次关门（Toggle） |
| 3 | 入侵检测 | HC-SR501 人体红外，无卡检测到人时 OLED 告警 + Web 红色标记 |
| 4 | 智能照明 | 自动模式：光敏 ADC → 五段亮度映射（越暗越亮）；手动模式：Web 远程调光 |
| 5 | 远程控风 | Web 滑块 → MQTT → ESP32 → USART → STM32 TIM3 PWM，0~100% 无级调速 |
| 6 | 本地显示 | 0.96" OLED 四行实时：温湿度 / 光照+LED / 门锁+风扇 / 红外+模式 |
| 7 | 数据上云 | 每 200ms 10 字段 JSON 经 MQTT 推 EMQX，Web 大屏 ECharts 实时渲染 |

---

## 🏗 技术栈

| 层级 | 方案 |
|:---|:---|
| 主控 MCU | STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash, LQFP48) |
| 外设库 | STM32 Standard Peripheral Library V3.5 |
| IDE | Keil MDK (ARMCC V5) |
| 无线网桥 | ESP32-S3-N16R8 (XTensa LX7, 16MB Flash) |
| SDK | ESP-IDF v5.5.1 |
| 消息中间件 | EMQX Broker (`192.168.123.59:1883` MQTT / `:8083` WebSocket) |
| Web 前端 | 纯 HTML+JS，ECharts 5.4 + Paho MQTT，无需构建工具 |
| 通信协议 | USART1 115200bps 8N1 双向 + MQTT QoS 1 |

---

## 🏗 系统架构

```mermaid
flowchart LR
    subgraph 传感层
        DHT11[DHT11<br/>温湿度]
        MQ2[MQ2<br/>烟雾]
        PIR[HC-SR501<br/>红外]
        FLAME[火焰传感器]
        LIGHT[5516 光敏]
        RFID[RC522<br/>门禁]
    end

    subgraph 主控
        STM32[STM32F103C8T6<br/>采集·决策·OLED]
    end

    subgraph 执行层
        SERVO[舵机 门锁]
        FAN[L9110 风扇]
        BUZZER[蜂鸣器]
        LED[LED 照明]
    end

    subgraph 网关
        ESP32[ESP32-S3<br/>MQTT 网桥]
    end

    subgraph 云端
        EMQX[EMQX Broker]
        WEB[Web 大屏<br/>ECharts]
    end

    传感层 --> STM32
    STM32 --> 执行层
    STM32 -->|I2C| OLED[OLED 0.96"]
    STM32 <-->|USART1| ESP32
    ESP32 <-->|MQTT| EMQX
    EMQX <-->|WebSocket| WEB
```

---

## 🔌 完整引脚分配表

### STM32F103C8T6 (LQFP48) — 18 个 GPIO 全部就位

| GPIO | 模式 | 外设 | 设备 | 功能 |
|:---:|:---|:---|:---|:---|
| **PA0** | GPIO 推挽 | — | DHT11 | 单总线温湿度 |
| **PA1** | 模拟输入 | ADC1_CH1 | MQ2 AO | 烟雾浓度 0~4095 |
| **PA2** | GPIO 浮空输入 | — | Flame Sensor | 火焰检测 DO |
| **PB15** | GPIO 浮空输入 | — | MQ2 DO | 烟雾数字阈值报警 |
| **PA3** | GPIO 浮空输入 | — | HC-SR501 | 人体红外 |
| **PA4** | GPIO 推挽 (软件PWM) | — | 舵机 SG90 | 门锁 0°关 / 180°开 |
| **PA5** | GPIO 复用推挽 | SPI1_SCK | RC522 | RFID 时钟 |
| **PA6** | GPIO 浮空输入 | SPI1_MISO | RC522 | RFID MISO |
| **PA7** | GPIO 复用推挽 | SPI1_MOSI | RC522 | RFID MOSI |
| **PA9** | GPIO 复用推挽 | USART1_TX | → ESP32 GPIO18 | 传感器帧上报 |
| **PA10** | GPIO 浮空输入 | USART1_RX | ← ESP32 GPIO17 | 接收控制指令 |
| **PB0** | 模拟输入 | ADC1_CH8 | 5516 光敏 AO | 环境光照 0~4095 |
| **PB1** | GPIO 推挽 | — | 蜂鸣器 | 刷卡反馈 |
| **PB3** ⚠️ | GPIO 复用推挽 | TIM2_CH2 (FullRemap) | LED 照明 | PWM 调光 1kHz |
| **PB4** ⚠️ | GPIO 复用推挽 | TIM3_CH1 (PartialRemap) | L9110 INA | 风扇 PWM 20kHz |
| **PB5** | GPIO 复用推挽 | TIM3_CH2 (PartialRemap) | L9110 INB | 风扇 PWM 20kHz |
| **PB6** | GPIO 推挽 (软件I2C) | — | OLED SCL | 显示屏时钟 |
| **PB7** | GPIO 推挽 (软件I2C) | — | OLED SDA | 显示屏数据 |
| **PB8** | GPIO 浮空输入 | — | 5516 光敏 DO | 数字阈值 (备用) |
| **PB9** | — | — | — | 空闲 (预留) |
| **PB10** | GPIO 复用推挽 | USART3_TX | 调试串口 TX | 独立调试日志输出 |
| **PB11** | GPIO 浮空输入 | USART3_RX | 调试串口 RX | 独立调试日志输入 |
| **PB12** | GPIO 推挽 | — | RC522 NSS/CS | RFID 片选 |

> ⚠️ **PB3/PB4** 默认 JTAG 引脚 (JTDO/JNTRST)。代码中 `GPIO_Remap_SWJ_JTAGDisable` 释放后可用 SWD (PA13/PA14) 烧录调试。
> ⚠️ **PB3 需要 TIM2_FullRemap** 才能输出 TIM2_CH2 PWM（默认在 PA1）。

### 5516 光敏模块接线

| 模块引脚 | STM32 |
|:---|:---|
| AO | PB0 (ADC1_CH8) |
| DO | PB8 |
| VCC | 3.3V |
| GND | GND |

### LED 接线

| LED | 接法 |
|:---|:---|
| 长脚 (阳极) | PB3 (串 220Ω 限流电阻) |
| 短脚 (阴极) | GND |

### STM32 ↔ ESP32 接线

| STM32 | 方向 | ESP32-S3 |
|:---|:---:|:---|
| PA9 (USART1_TX) | → | GPIO18 (UART1_RX) |
| PA10 (USART1_RX) | ← | GPIO17 (UART1_TX) |
| GND | ↔ | GND |

### 调试串口 (独立于 ESP32 通信链路)

| STM32 | 方向 | USB-TTL |
|:---|:---:|:---|
| PB10 (USART3_TX) | → | RX |
| PB11 (USART3_RX) | ← | TX |
| GND | ↔ | GND |

> 波特率 115200 8N1。USART3 与 USART1（连接 ESP32）**完全独立**，可同时工作互不干扰，方便调试时观察 STM32 的运行日志。

---

## 📡 通信协议

### STM32 → ESP32 上报帧 (USART, 200ms 周期)

```
T:25.5;H:62.0;MQ:0847;PIR:1;FLM:0;SRV:1;FAN:075;LGHT:2048;LEDM:1;LEDB:050\r\n
```

| 字段 | 含义 | 范围 |
|:---|:---|:---|
| `T` | 温度 | °C 浮点 |
| `H` | 相对湿度 | % 浮点 |
| `MQ` | MQ2 烟雾 ADC | 0~4095 |
| `PIR` | 人体红外 | 0=无人 1=有人 |
| `FLM` | 火焰检测 | 0=安全 1=火警 |
| `SRV` | 门锁状态 | 0=关锁 1=开锁 |
| `FAN` | 风扇转速 | 0~100% |
| `LGHT` | 光照 ADC | 0~4095 (越大越暗) |
| `LEDM` | LED 模式 | 0=手动 1=自动 |
| `LEDB` | LED 亮度 | 0~100% |

### ESP32 → STM32 控制指令 (USART)

```
FAN:75\n        → 风扇转速 75%
LED:AUTO\n      → LED 切换自动模式
LED:MAN\n       → LED 切换手动模式
LED:50\n        → LED 亮度 50% (仅手动模式)
```

### ESP32 ↔ EMQX (MQTT)

**上报** — Topic: `wms/warehouse1/data` (QoS 1)
```json
{"temp":25.5,"humi":62.0,"mq2":847,"pir":1,"flame":0,"servo":1,"fan":75,"light":2048,"led_mode":1,"led_brightness":50}
```

**控制** — Topic: `wms/warehouse1/cmd` (QoS 0)
```json
{"speed":75}                  → 风扇调速
{"led_mode":"auto"}           → LED 自动模式
{"led_mode":"manual"}         → LED 手动模式
{"led_brightness":75}         → LED 亮度 0~100
```

---

## 🔐 RFID 门禁逻辑

授权卡号：`81 F4 2C 07`

| 场景 | 门锁 | 蜂鸣器 | OLED |
|:---|:---|:---|:---|
| 授权卡 (当前关锁) | ✅ 开锁 | 一声短鸣 | DOOR:OPEN |
| 授权卡 (当前开锁) | ❌ 关锁 | 一声短鸣 | DOOR:LOCKED |
| 陌生卡 | 不变 | 急促两声 | UNAUTHORIZED!!! |

### 无卡监控模式

| 红外 | OLED L1 | 说明 |
|:---|:---|:---|
| 无人 | `SYS MONITOR` | 正常展示环境数据 |
| 有人 | `WARNING: Human!` | 侵入告警 |

---

## 💡 LED 智能照明

### 自动模式 (上电默认)

光敏 ADC 五段映射，**环境越暗 → LED 越亮**：

| ADC 范围 | 环境 | LED 亮度 |
|:---|:---|:---|
| 0 ~ 500 | 很亮 | 0% (关) |
| 500 ~ 1500 | 稍暗 | 10% ~ 30% |
| 1500 ~ 2500 | 较暗 | 30% ~ 60% |
| 2500 ~ 3500 | 很暗 | 60% ~ 90% |
| 3500 ~ 4095 | 极暗 | 100% |

### 手动模式

Web 大屏点击"🖐 手动控制"→ 解锁滑块 → 拖动 0~100% → MQTT → ESP32 → STM32 → TIM2_CH2 PWM 调光。

---

## 🌐 Web 大屏 (index.html)

纯静态 HTML，浏览器直接打开，EMQX WebSocket 连接。

**仪表盘区域：**
- 6 张指标卡片：温度/湿度/烟雾/风扇/光照/LED 亮度
- ECharts 温湿度历史趋势折线图 (保留最近 30 个点)
- 安防状态阵列：红外/火焰/门锁，异常时红色高亮
- 风扇控制面板：滑块 + 一键关闭按钮
- 灯光控制面板：自动/手动切换 + 亮度滑块 (自动模式禁用)

**连接方式：** WebSocket → `192.168.123.59:8083/mqtt`

---

## 📂 工程目录

```
StWHouse1.0/
├── Hardware/                  # STM32 外设驱动层 (13 个模块)
│   ├── DHT11.c/.h             #   温湿度 (单总线)
│   ├── MQ2.c/.h               #   烟雾 (ADC1_CH1)
│   ├── HC_SR501.c/.h          #   人体红外
│   ├── FlameSensor.c/.h       #   火焰传感器
│   ├── LightSensor.c/.h       #   光敏电阻 5516 (ADC1_CH8)
│   ├── RCC522.c/.h            #   RFID (SPI1)
│   ├── OLED_I2C.c/.h          #   OLED 0.96" (软件I2C)
│   ├── OLED_Font.h            #   字库 8x16
│   ├── Fan.c/.h               #   风扇 L9110 (TIM3 PWM 20kHz)
│   ├── Servo.c/.h             #   舵机 (软件PWM 50Hz)
│   ├── LedLight.c/.h          #   LED (TIM2_CH2 PWM 1kHz)
│   ├── Buzzer.c/.h            #   蜂鸣器
│   ├── UartComm.c/.h          #   USART1 双向 (TX帧 + RX中断)
│   └── delay.c/.h             #   SysTick 延时
├── User/
│   ├── main.c                 #   主程序 (模块化, 5 步主循环)
│   ├── stm32f10x_it.c/.h      #   中断服务 (USART1_IRQHandler)
│   └── stm32f10x_conf.h       #   标准库裁剪
├── Library/                   # STM32 StdPeriph V3.5
├── Start/                     # 启动文件 + system_stm32f10x
├── ESP32/ESP32/               # ESP32-S3 MQTT 网桥
│   ├── CMakeLists.txt
│   ├── sdkconfig
│   ├── index.html             #   Web 大屏仪表盘
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c             #   WiFi STA + MQTT + UART 桥接
└── README.md
```

---

## 🔨 编译与烧录

### STM32 (Keil MDK)

1. 打开 `Project.uvprojx`
2. 确保以下文件已加入工程：`LightSensor.c`、`LedLight.c`、`UartComm.c`、`FlameSensor.c`、`main.c`、`stm32f10x_it.c`
3. `F7` 编译，ST-Link/DAP-Link SWD 烧录 (PA13/PA14)

### ESP32-S3 (ESP-IDF v5.5)

```bash
cd ESP32/ESP32
idf.py build
idf.py -p COMx flash monitor
```

---

## 📝 版本记录

| 版本 | 日期 | 内容 |
|:---|:---|:---|
| v1.0 | 2026-05-24 | 初始：模块化 main.c、UartComm TX 单向上报、火焰传感器接入 |
| v1.1 | 2026-05-25 | RFID Toggle 门锁；风扇与门锁解耦；UartComm 加入 RX 中断接收 |
| v1.2 | 2026-05-26 | ESP32 升级 MQTT 网桥 (EMQX)；USART 帧→JSON→MQTT Topic 全链路 |
| v1.3 | 2026-05-27 | **新增** 5516 光敏 (PB0 ADC) + LED 照明 (PB3 TIM2_FullRemap PWM)；自动/手动双模式调光（五段映射）；USART 帧扩展至 10 字段；Web 大屏新增灯光控制面板 |
