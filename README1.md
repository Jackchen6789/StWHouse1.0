# 🏭 仓瞳 (StWHouse) —— 智慧仓储环境安防数字孪生平台

> **St**orage **W**atch **House** · 守卫仓库的"眼睛"
>
> STM32 边缘采集 → ESP32 MQTT 网桥 → EMQX 消息中间件 → Qt / Web 双端大屏消费

---

## 📋 目录

- [项目背景](#-项目背景)
- [系统概览](#-系统概览)
- [硬件架构](#-硬件架构)
- [软件架构](#-软件架构)
- [通信协议](#-通信协议)
- [功能模块详解](#-功能模块详解)
- [安全机制](#-安全机制)
- [工程目录结构](#-工程目录结构)
- [快速开始](#-快速开始)
- [技术栈](#-技术栈)
- [版本记录](#-版本记录)

---

## 🎯 项目背景

面向**中小型仓库**的环境监控与安防需求，设计并实现了一套从感知层到应用层的完整物联网闭环系统。系统以 STM32F103C8T6 为主控芯片，通过多种传感器实时采集仓库内的温湿度、烟雾浓度、光照强度、火焰状态和人体红外信号，结合 RC522 RFID 模块实现门禁管理，并通过 ESP32-S3 将数据经 MQTT 协议实时推送至 EMQX 消息中间件，最终在 Qt 桌面端和 Web 浏览器端以仪表盘形式可视化呈现，同时支持远程风扇调速与 LED 调光控制。

**核心设计目标：**
- ✅ 多传感器融合：7 类传感器协同工作，无盲区覆盖
- ✅ 实时响应：200ms 采集周期，秒级云端同步
- ✅ 双向控制：Web/Qt 下发指令，经 MQTT → ESP32 → USART → STM32 全链路送达
- ✅ 安全可靠：密码哈希存储 + 门禁鉴权 + MQTT 认证预留
- ✅ 双端可视化：Qt 桌面大屏（含用户管理 + 历史回溯）+ Web 企业级仪表盘
- ✅ 历史数据：sensor_logs 10 秒聚合写入，7 天自动清理，支持 1h/6h/24h 回溯

---

## 🏗 系统概览

```
                        ┌─────────────────────────────────┐
                        │          📱 应用层               │
                        │  ┌──────────┐  ┌─────────────┐  │
                        │  │ Qt 大屏   │  │ Web 仪表盘   │  │
                        │  │ (C++/Qt6) │  │ (HTML+ECharts)│  │
                        │  └─────┬────┘  └──────┬──────┘  │
                        │        │ MQTT/TCP     │ WebSocket│
                        └────────┼───────────────┼─────────┘
                                 │               │
                        ┌────────▼───────────────▼─────────┐
                        │       ☁️ 消息中间件层              │
                        │         EMQX Broker               │
                        │    MQTT :1883 / WS :8083          │
                        └────────────────┬─────────────────┘
                                         │ MQTT
                        ┌────────────────▼─────────────────┐
                        │       📡 网关层                   │
                        │      ESP32-S3 (MQTT 网桥)         │
                        │   WiFi STA + UART 透传 + 匿名模式  │
                        └────────┬──────────────┬──────────┘
                                 │ USART1       │ 可直接运行
                                 │ 115200 8N1   │ Web Server
                                 │              │ (index.html)
                        ┌────────▼──────────────▼──────────┐
                        │       🔌 边缘主控层               │
                        │    STM32F103C8T6 (Cortex-M3)     │
                        │  采集 · 决策 · OLED · PWM · 门禁   │
                        └──┬──┬──┬──┬──┬──┬──┬────────────┘
                           │  │  │  │  │  │  │
              ┌────────────┼──┼──┼──┼──┼──┼──┼────────────┐
              │            ▼  ▼  ▼  ▼  ▼  ▼  ▼            │
              │  DHT11  MQ2  火焰 光敏 红外 RC522         │
              │  (温湿度)(烟雾)(火警)(光照)(入侵)(门禁)     │
              │                  📡 传感器矩阵               │
              └────────────────────────────────────────────┘

              ┌────────────────────────────────────────────┐
              │  🎮 执行器：舵机(门锁) · L9110(风扇)        │
              │           蜂鸣器(告警) · LED(照明)          │
              └────────────────────────────────────────────┘
```

---

## 🔌 硬件架构

### 主控芯片：STM32F103C8T6

| 参数 | 规格 |
|:---|:---|
| 内核 | ARM Cortex-M3 @ 72MHz |
| Flash / SRAM | 64KB / 20KB |
| 封装 | LQFP48 |
| 外设库 | STM32 Standard Peripheral Library V3.5 |
| IDE / 编译器 | Keil MDK / ARMCC V5 |

### 完整引脚分配

| GPIO | 模式 | 外设 | 连接设备 | 功能说明 |
|:---:|:---|:---|:---|:---|
| **PA0** | GPIO 推挽 | — | DHT11 | 单总线温湿度传感器 |
| **PA1** | 模拟输入 | ADC1_CH1 | MQ2 AO | 烟雾浓度模拟量 (0~4095) |
| **PA2** | GPIO 浮空输入 | — | 火焰传感器 DO | 火焰检测数字输出 |
| **PB15** | GPIO 浮空输入 | — | MQ2 DO | 烟雾浓度数字阈值报警 |
| **PA3** | GPIO 浮空输入 | — | HC-SR501 | 人体红外感应 |
| **PA4** | GPIO 推挽 (软件PWM) | — | SG90 舵机 | 门锁控制 (0°关 / 180°开) |
| **PA5** | GPIO 复用推挽 | SPI1_SCK | RC522 | RFID SPI 时钟 |
| **PA6** | GPIO 浮空输入 | SPI1_MISO | RC522 | RFID SPI 数据输入 |
| **PA7** | GPIO 复用推挽 | SPI1_MOSI | RC522 | RFID SPI 数据输出 |
| **PA9** | GPIO 复用推挽 | USART1_TX | → ESP32 GPIO18 | 传感器帧上报 |
| **PA10** | GPIO 浮空输入 | USART1_RX | ← ESP32 GPIO17 | 接收控制指令 |
| **PB0** | 模拟输入 | ADC1_CH8 | 5516 光敏 AO | 环境光照采集 (0~4095) |
| **PB1** | GPIO 推挽 | — | 蜂鸣器 | RFID 刷卡反馈 |
| **PB3** ⚠️ | GPIO 复用推挽 | TIM2_CH2 (FullRemap) | LED 照明 | PWM 调光 1kHz |
| **PB4** ⚠️ | GPIO 复用推挽 | TIM3_CH1 (PartialRemap) | L9110 INA | 风扇 PWM 20kHz |
| **PB5** | GPIO 复用推挽 | TIM3_CH2 (PartialRemap) | L9110 INB | 风扇 PWM 20kHz |
| **PB6** | GPIO 推挽 (软件I2C) | — | OLED SCL | 0.96" OLED 时钟 |
| **PB7** | GPIO 推挽 (软件I2C) | — | OLED SDA | 0.96" OLED 数据 |
| **PB11** | GPIO 浮空输入 | — | 5516 光敏 DO | 光照数字阈值 (备用) |
| **PB12** | GPIO 推挽 | — | RC522 NSS/CS | RFID SPI 片选 |

> ⚠️ **PB3/PB4** 默认复用 JTAG 功能 (JTDO/JNTRST)，代码中通过 `GPIO_Remap_SWJ_JTAGDisable` 释放，仅保留 SWD (PA13/PA14) 用于烧录调试。
>
> ⚠️ **PB3** 需配置 `GPIO_FullRemap_TIM2` 才能将 TIM2_CH2 从默认的 PA1 重映射到 PB3。

### STM32 ↔ ESP32 串口接线

| STM32 | 方向 | ESP32-S3 |
|:---|:---:|:---|
| PA9 (USART1_TX) | → | GPIO18 (UART1_RX) |
| PA10 (USART1_RX) | ← | GPIO17 (UART1_TX) |
| GND | ↔ | GND |

### 传感器与执行器清单

| 类别 | 设备 | 型号 | 接口 | 用途 |
|:---|:---|:---|:---|:---|
| 环境 | 温湿度传感器 | DHT11 | 单总线 GPIO | 仓库温湿度监测 |
| 环境 | 烟雾传感器 | MQ2 | ADC + DO | 火灾预警 |
| 环境 | 光敏电阻模块 | 5516 | ADC + DO | 环境光照感知 |
| 安防 | 人体红外 | HC-SR501 | GPIO | 入侵检测 |
| 安防 | 火焰传感器 | — | GPIO DO | 明火检测 |
| 安防 | RFID 读卡器 | RC522 | SPI | 门禁身份识别 |
| 显示 | OLED 屏幕 | 0.96" 128x64 | 软件 I2C | 本地四行状态显示 |
| 执行 | 舵机 | SG90 | 软件 PWM | 门锁驱动 |
| 执行 | 风扇驱动 | L9110 | TIM3 PWM | 仓库通风散热 |
| 执行 | LED 照明 | 5mm 高亮 | TIM2 PWM | 智能调光照明 |
| 执行 | 蜂鸣器 | 有源 | GPIO | 刷卡/告警提示音 |

---

## 🧠 软件架构

### STM32 固件 (C — Keil MDK)

采用**模块化分层设计**，主循环 5 步流水线：

```
┌──────────────────────────────────────────────┐
│              main() 主循环 200ms               │
│                                                │
│  Step 1: Sensor_ReadAll()                      │
│  │  DHT11 · MQ2 · 光敏 · 红外 · 火焰 · RFID    │
│  │                                              │
│  Step 2: 逻辑决策                               │
│  │  红外+无卡 → 入侵告警                        │
│  │  自动模式 → LED_AutoControl(light_adc)       │
│  │                                              │
│  Step 3: OLED 四行刷新                          │
│  │  L1: 温湿度 / 警告                           │
│  │  L2: 烟雾 · 火焰                              │
│  │  L3: 光照 · LED 模式/亮度                    │
│  │  L4: 门锁 · 风扇 · 红外 · 模式               │
│  │                                              │
│  Step 4: USART 帧上报                           │
│  │  10 字段 → ESP32                             │
│  │                                              │
│  Step 5: USART RX 指令解析                      │
│  │  FAN:xx / LED:AUTO/MAN/xx → 更新执行器       │
│  └──────────────────────────────────────────────┘
```

**13 个外设驱动模块：**

| 模块 | 文件 | 功能 |
|:---|:---|:---|
| DHT11 | `dht11.c/h` | 单总线温湿度读取 |
| MQ2 | `mq2.c/h` | 烟雾 ADC + DO 阈值 |
| LightSensor | `lightsensor.c/h` | 5516 光敏 ADC 采集 |
| HC_SR501 | `hc_sr501.c/h` | 人体红外检测 |
| FlameSensor | `flamesensor.c/h` | 火焰传感器 DO |
| RCC522 | `rcc522.c/h` | RFID SPI 读写 + 卡号比对 |
| OLED_I2C | `oled_i2c.c/h` | 软件 I2C 驱动 128x64 OLED |
| Fan | `fan.c/h` | TIM3 双路 PWM 20kHz 风扇 |
| Servo | `servo.c/h` | 软件 PWM 50Hz 舵机 |
| LedLight | `ledlight.c/h` | TIM2_CH2 PWM 1kHz 调光 |
| Buzzer | `buzzer.c/h` | GPIO 蜂鸣器控制 |
| UartComm | `uartcomm.c/h` | USART1 帧收发 + RX 中断 |
| delay | `delay.c/h` | SysTick 微秒/毫秒延时 |

### ESP32-S3 固件 (C — ESP-IDF v5.5.1)

**角色：MQTT 网桥 + Web 静态资源托管**

```
┌─────────────────────────────────────┐
│          ESP32-S3 固件               │
│                                      │
│  ┌──────────┐   ┌────────────────┐  │
│  │ WiFi STA │   │ SPIFFS / HTTP   │  │
│  │ (连接路由器)│   │ (托管 index.html)│  │
│  └─────┬────┘   └────────────────┘  │
│        │                             │
│  ┌─────▼─────────────────────────┐  │
│  │     MQTT Client (匿名模式)     │  │
│  │  Pub: wms/warehouse1/data     │  │
│  │  Sub: wms/warehouse1/cmd      │  │
│  └─────┬─────────────────────────┘  │
│        │                             │
│  ┌─────▼─────────────────────────┐  │
│  │   UART1 桥接 (115200 8N1)     │  │
│  │  STM32 10字段帧 → JSON 上报    │  │
│  │  MQTT JSON 指令 → STM32 帧转发 │  │
│  │  RFID 卡号 CARD:XXXXXXXX 透传  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

> **架构演进**: v1.2→v1.5，ESP32 固件从「HTTP Server + 简易 MQTT」重构为「MQTT Client 网桥」。旧 HTTP 服务代码以注释形式保留在 `main.c` 顶部（行 1-477），当前激活代码为 MQTT 网桥版本（行 478-837）。`index.html` 为独立文件，通过浏览器直接访问 ESP32 IP 加载。

### Qt 桌面大屏 (C++ — Qt 6.11.1)

**角色：管理员专用桌面客户端**

| 模块 | 文件 | 功能 |
|:---|:---|:---|
| 主窗口 | `mainwindow.cpp/h` | 仪表盘 UI + 事件处理 + 用户管理 + 历史回溯 |
| MQTT 客户端 | `simplemqttclient.cpp/h` | 手写 MQTT 3.1.1 协议栈 (QTcpSocket)，预留认证字段 |
| 数据库 | SQLite (WAL 模式) | users 表 + cards 表 + sensor_logs 历史表 |

**功能矩阵：**

| 功能 | 实现 |
|:---|:---|
| 实时仪表盘 | 温度/湿度/烟雾/光照/LED 亮度 5 项指标卡片（2x2 网格） |
| 实时趋势图 | QtCharts 三线折线图，5 点滑动平均滤波，滚动 30 点 |
| 历史回溯 | **1h/6h/24h** 三档查询，摘要卡片（均温/最高/最低/湿度）+ 数据表格，每 10 秒自动刷新 |
| 风扇控制 | iOS 风格拨动开关 + 滑块 0~100% 无级调速 + 一键关闭 |
| 灯光控制 | 自动/手动模式切换 + 亮度滑块 |
| 门锁控制 | 远程开门/关门按钮（非对称消抖：开立即、关需两帧确认） |
| 用户登录 | 用户名 + 密码 → SHA-256 哈希验证 |
| 角色权限 | admin (全权限：用户管理 + 卡号管理) / operator (仅监控+控制) |
| 用户管理 | admin 可增删用户、修改密码，SHA-256 哈希存储 |
| **卡号管理** | 🆕 查看/授权/取消授权/删除/清空 RFID 卡号列表，**操作实时同步 STM32** |
| **历史数据** | 🆕 sensor_logs 表 10 秒聚合均值写入，7 天自动清理，支持一键清空 |
| MQTT 状态 | 实时连接状态指示灯 |

### Web 仪表盘 (HTML + ECharts 5.4 + Paho MQTT WebSocket)

**角色：轻量级监控页面，ESP32 直接托管，浏览器通过 WebSocket 直连 EMQX**

| 组件 | 实现 |
|:---|:---|
| 指标卡片 | 6 张：温度/湿度/烟雾/风扇转速/光照强度/LED 照明亮度 |
| 趋势图 | ECharts 双轴折线图，温湿度 30 点历史滚动 |
| 安防状态 | 红外/火焰/柜锁 三色阵列，异常红色高亮 + 脉冲动画 |
| 风扇面板 | 滑块 0~100% + 一键强行关闭按钮 |
| 灯光面板 | 自动/手动双按钮切换 + 亮度滑块（手动模式解锁）；**按钮状态自闭环**，不依赖下位机回传 |
| 连接方式 | WebSocket → `192.168.123.59:8083/mqtt` (Paho MQTT) |

---

## 📡 通信协议

### 三层通信链路

```
STM32 ◀──USART 115200──▶ ESP32-S3 ◀──MQTT──▶ EMQX ◀──TCP/WS──▶ Qt/Web
```

### STM32 → ESP32 上报帧 (200ms 周期)

```
T:25.5;H:62.0;MQ:0847;PIR:1;FLM:0;SRV:1;FAN:075;LGHT:2048;LEDM:1;LEDB:050\r\n
```

| 字段 | 含义 | 取值范围 |
|:---|:---|:---|
| `T` | 温度 (°C) | 浮点数 |
| `H` | 相对湿度 (%) | 浮点数 |
| `MQ` | 烟雾 ADC 值 | 0 ~ 4095 |
| `PIR` | 人体红外 | 0=无人, 1=有人 |
| `FLM` | 火焰检测 | 0=安全, 1=火警 |
| `SRV` | 门锁状态 | 0=关锁, 1=开锁 |
| `FAN` | 风扇转速 | 0 ~ 100 (%) |
| `LGHT` | 光照 ADC | 0 ~ 4095 (值越大越暗) |
| `LEDM` | LED 模式 | 0=手动, 1=自动 |
| `LEDB` | LED 亮度 | 0 ~ 100 (%) |

### ESP32 → STM32 控制指令

```
FAN:75\n         → 设置风扇转速 75%
LED:AUTO\n       → LED 切换为自动模式
LED:MAN\n        → LED 切换为手动模式
LED:50\n         → LED 亮度 50% (仅手动模式有效)
SRV:1\n          → 舵机开门 (远程开锁)
SRV:0\n          → 舵机关门 (远程关锁)
AUTH:ADD:XXXXXXXX\n  → 添加授权卡号 (8 位十六进制)
AUTH:DEL:XXXXXXXX\n  → 删除授权卡号
AUTH:CLR\n       → 清空全部授权卡号
```

### MQTT Topic 设计

**上行数据** — Topic: `wms/warehouse1/data` (QoS 0)

```json
{
  "temp": 25.5,
  "humi": 62.0,
  "mq2": 847,
  "pir": 1,
  "flame": 0,
  "servo": 1,
  "fan": 75,
  "light": 2048,
  "led_mode": 1,
  "led_brightness": 50,
  "card": "81F42C07"
}
```

> **QoS 0 说明**：传感器数据 200ms 一条高频上报，QoS 0 足以应对偶发性丢包，同时规避 NanoMQ 的 packet ID 重复警告（`nmq_pipe_send_start_v4: packet id duplicates in nano_qos_db`）。

**下行控制** — Topic: `wms/warehouse1/cmd` (QoS 0)

```json
{"speed": 75}              → 风扇调速
{"led_mode": "auto"}       → LED 自动模式
{"led_mode": "manual"}     → LED 手动模式
{"led_brightness": 75}     → LED 亮度 0~100
{"servo": 1}               → 远程开门
{"servo": 0}               → 远程关门
{"auth_add": "XXXXXXXX"}   → 添加授权卡号
{"auth_del": "XXXXXXXX"}   → 删除授权卡号
{"auth_clr": 1}            → 清空全部授权卡
```

---

## 📋 功能模块详解

### 🔐 RFID 门禁系统

**授权机制：**
- 硬编码主卡：`81 F4 2C 07`
- 动态授权列表：支持最多 10 张授权卡
- **Qt 端远程管理**：查看所有已检测卡号、授权/取消授权/删除/一键清空，操作经 MQTT → ESP32 → USART 实时同步 STM32
- STM32 通过 `AUTH:ADD/DEL/CLR` 命令接收远程授权变更

**刷卡逻辑 (Toggle 模式)：**

| 场景 | 动作 | 蜂鸣器 | OLED 显示 | MQTT 上报 |
|:---|:---|:---|:---|:---|
| 授权卡 + 当前关锁 | 开锁 | 一声短鸣 | `DOOR:OPEN` | `CARD:XXXXXXXX` |
| 授权卡 + 当前开锁 | 关锁 | 一声短鸣 | `DOOR:LOCKED` | `CARD:XXXXXXXX` |
| 非授权卡 | 无动作 | 急促两声 | `UNAUTHORIZED!!!` | `CARD:XXXXXXXX` |

> 每次刷卡（无论是否授权），STM32 都会发送 `CARD:XXXXXXXX\n` 帧给 ESP32，ESP32 将其填充到 MQTT JSON 的 `card` 字段，Qt 端收到后自动写入 `cards` 数据库表。首次出现的卡号默认未授权，需 admin 在卡号管理面板手动授权。

### 🚪 远程门锁控制

- Web 端暂不提供门锁按钮（安全考量）
- Qt 端提供远程开门/关门按钮，单击 toggle 当前状态
- **非对称消抖**：开门立即响应（`servo==1` 单帧确认），关门需连续两帧确认（防偶发错帧导致误锁）
- 指令路径：Qt → MQTT `{"servo":0/1}` → ESP32 → USART `SRV:0/1\n` → STM32 舵机

### 🚨 入侵检测

| 红外状态 | OLED 第1行 | Web/Qt 显示 | 说明 |
|:---|:---|:---|:---|
| 无人 | `SYS MONITOR` | 正常绿色 | 正常监控模式 |
| 有人 | `WARNING: Human!` | 红色高亮 | 侵入告警 |

### 💡 智能照明 (双模式)

**自动模式 (上电默认)** — 光敏 ADC 五段非线性映射：

| ADC 范围 | 环境亮度 | LED 输出 | 映射关系 |
|:---|:---|:---|:---|
| 0 ~ 500 | ☀️ 很亮 | 0% (关闭) | y = 0 |
| 500 ~ 1500 | 🌤 稍暗 | 10% ~ 30% | y = (x-500) × 0.02 + 10 |
| 1500 ~ 2500 | ⛅ 较暗 | 30% ~ 60% | y = (x-1500) × 0.03 + 30 |
| 2500 ~ 3500 | 🌙 很暗 | 60% ~ 90% | y = (x-2500) × 0.03 + 60 |
| 3500 ~ 4095 | 🌑 极暗 | 100% (全亮) | y = 100 |

**手动模式** — Web 大屏点击"🖐 手动控制"解锁滑块，拖动 0~100% 实时生效。

### 🌬 远程风扇控制

- Web/Qt 滑块范围 0~100%
- 经 MQTT → ESP32 → USART → STM32，TIM3 双路 PWM 20kHz 驱动 L9110
- 0% = 风扇停转，100% = 全速运转
- 风扇与门锁完全独立，无耦合

### 📟 OLED 本地显示

4 行 × 16 列实时刷新：

```
L1: T:25.5C H:62%     (或 WARNING: Human!)
L2: Smoke:0847 Fire:N
L3: Light:2048 LED:A75
L4: DOOR:L FAN:75 P:1
```

---

## 🔒 安全机制

| 层级 | 措施 | 实现方式 |
|:---|:---|:---|
| 用户密码 | SHA-256 哈希存储 | `QCryptographicHash::Sha256`，数据库无明文 |
| 旧密码迁移 | 启动时自动升级 | 检测明文密码 → 自动替换为 SHA-256 哈希 |
| 门禁鉴权 | RFID 白名单 | 主卡 (81F42C07) + 动态授权列表（最多 10 张），支持远程增删 |
| MQTT 通信 | 匿名模式（当前） | 局域网封闭环境；代码已完整预留 EMQX 认证升级路径 |
| MQTT QoS | 传感器数据 QoS 0 | 200ms 高频数据丢弃不重传，避免 NanoMQ packet ID 重复错误 |
| Web 灯光控制 | 按钮自闭环 | 网页 LED 模式按钮不依赖下位机回传，防模式错乱 |

> **认证升级路径**：需在 EMQX Dashboard 创建用户，然后在 ESP32 `mqtt_cfg` 中添加 `.credentials` 字段，Qt 端 `connectToBroker()` 传入 username/password 即可，无需修改协议代码。

---

## 📂 工程目录结构

```
StWHouse1.0/
│
├── Hardware/                      # STM32 外设驱动层 (13 个模块)
│   ├── DHT11.c/.h                 #   温湿度传感器 (单总线)
│   ├── MQ2.c/.h                   #   烟雾传感器 (ADC1_CH1)
│   ├── HC_SR501.c/.h              #   人体红外传感器
│   ├── FlameSensor.c/.h           #   火焰传感器
│   ├── LightSensor.c/.h           #   光敏电阻 5516 (ADC1_CH8)
│   ├── RCC522.c/.h                #   RFID 读卡器 (SPI1)
│   ├── OLED_I2C.c/.h              #   OLED 0.96" 128x64 (软件 I2C)
│   ├── OLED_Font.h                #   8x16 ASCII 字库
│   ├── Fan.c/.h                   #   风扇 L9110 (TIM3 双路 PWM 20kHz)
│   ├── Servo.c/.h                 #   舵机 SG90 (软件 PWM 50Hz)
│   ├── LedLight.c/.h              #   LED 照明 (TIM2_CH2 PWM 1kHz + 五段映射)
│   ├── Buzzer.c/.h                #   蜂鸣器
│   ├── UartComm.c/.h              #   USART1 双向通信 (帧发送 + RX 中断接收)
│   └── delay.c/.h                 #   SysTick 微秒/毫秒延时
│
├── User/                          # 应用层
│   ├── main.c                     #   主程序 (模块化, 5 步主循环 200ms)
│   ├── stm32f10x_it.c/.h          #   中断服务函数 (USART1_IRQHandler 等)
│   ├── stm32f10x_conf.h           #   标准外设库配置文件
│   ├── asm_demo.c                 #   ARM Cortex-M3 内联汇编演示 (8 个函数)
│   └── asm_demo.h                 #   汇编演示模块头文件
│
├── Library/                       # STM32 Standard Peripheral Library V3.5
├── Start/                         # 启动文件 + system_stm32f10x
├── System/                        # 系统文件 (delay 等)
├── Listings/                      # 编译产物 (Listing 文件)
├── Objects/                       # 编译产物 (目标文件)
├── DebugConfig/                   # Keil 调试配置
│
├── ESP32/ESP32/                   # ESP32-S3 MQTT 网桥 + Web 服务器
│   ├── CMakeLists.txt             #   顶层 CMake
│   ├── sdkconfig                  #   ESP-IDF 配置
│   ├── index.html                 #   Web 仪表盘 (ECharts 5.4 + Paho MQTT WebSocket)
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c                 #   WiFi STA + MQTT Client + UART 桥接 (行 1-477 旧 HTTP 代码已注释)
│
├── qt/StWHouseDash/               # Qt6 桌面大屏客户端
│   ├── CMakeLists.txt             #   CMake 构建文件
│   ├── main.cpp                   #   程序入口
│   ├── mainwindow.h/.cpp/.ui      #   主窗口 (仪表盘 + 用户管理)
│   └── simplemqttclient.h/.cpp    #   手写 MQTT 3.1.1 协议客户端
│
├── Project.uvprojx                # Keil MDK 工程文件
├── Project.code-workspace         # VS Code 多根工作区
├── README.md                      # 原始项目文档
├── README1.md                     # 本文档 (重构版)
└── 答辩文档_网络安全与汇编增补.md   # 安全与汇编增补说明
```

---

## 🚀 快速开始

### 前置环境

| 组件 | 版本要求 | 说明 |
|:---|:---|:---|
| Keil MDK | V5.x + ARMCC V5 | STM32 固件编译 |
| STM32 StdPeriph | V3.5 | 已内置在 `Library/` |
| ESP-IDF | v5.5.1 | ESP32 固件编译 |
| Qt | 6.11.1 (MinGW 64-bit) | Qt 桌面客户端编译 |
| EMQX | 5.x | MQTT 消息中间件 |
| 烧录工具 | ST-Link / DAP-Link | STM32 SWD 烧录 |

### STM32 编译与烧录

1. 用 Keil MDK 打开 `Project.uvprojx`
2. 确认 `Hardware/` 和 `User/` 下所有 `.c` 文件已加入工程
3. `F7` 编译 → ST-Link/DAP-Link 通过 SWD (PA13/PA14) 烧录

### ESP32-S3 编译与烧录

```bash
cd ESP32/ESP32
idf.py build
idf.py -p COMx flash monitor
```

### EMQX Broker 配置

1. 启动 EMQX，访问 Dashboard
2. 确认 MQTT 端口 1883、WebSocket 端口 8083 已开放
3. （可选）如需启用 MQTT 认证：在 访问控制 → 认证 中创建用户，然后在 ESP32 和 Qt 代码中传入凭证

### Qt 桌面端编译

```bash
cd qt/StWHouseDash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### Web 仪表盘

ESP32 启动后自动托管 `index.html`，浏览器访问 `http://<ESP32_IP>` 即可。

---

## 💻 技术栈

| 层级 | 技术选型 | 版本 |
|:---|:---|:---|
| **主控 MCU** | STM32F103C8T6 (ARM Cortex-M3) | — |
| **MCU 外设库** | STM32 Standard Peripheral Library | V3.5 |
| **MCU IDE** | Keil MDK (ARMCC V5) | V5.06 |
| **无线 SoC** | ESP32-S3-N16R8 (Xtensa LX7) | — |
| **ESP SDK** | ESP-IDF | v5.5.1 |
| **消息中间件** | EMQX Broker | 5.x |
| **桌面客户端** | Qt (Widgets + Charts + Sql) | 6.11.1 |
| **Web 前端** | 原生 HTML5 + JS + ECharts | 5.4 |
| **MQTT 协议** | MQTT 3.1.1 | — |
| **串口通信** | USART 115200bps 8N1 | — |
| **密码哈希** | SHA-256 (Qt QCryptographicHash) | — |
| **构建系统** | CMake (ESP32 + Qt) + Keil Build (STM32) | — |

---

## 📝 版本记录

| 版本 | 日期 | 里程碑 |
|:---|:---|:---|
| **v1.0** | 2026-05-24 | 初始版本：模块化 main.c、UartComm TX 单向帧上报、火焰传感器接入、DHT11/MQ2/HC-SR501/RC522/舵机/风扇/蜂鸣器/OLED 全驱动就位 |
| **v1.1** | 2026-05-25 | RFID Toggle 门锁逻辑 (刷一次开、再刷关)；风扇与门锁解耦；UartComm 加入 RX 中断接收控制指令 |
| **v1.2** | 2026-05-26 | ESP32-S3 升级为 MQTT 网桥：USART 帧 → JSON → EMQX → Topic 全链路贯通；Web 仪表盘 ECharts 实时渲染 |
| **v1.3** | 2026-05-27 | 新增 5516 光敏传感器 (PB0 ADC) + LED 照明系统 (PB3 TIM2_FullRemap PWM)；自动/手动双模式五段调光；USART 帧扩展至 10 字段 |
| **v1.4** | 2026-05-31 | Qt6 桌面大屏客户端上线 (仪表盘 + 用户管理)；MQTT 认证预留；SHA-256 密码哈希存储；ARM Cortex-M3 内联汇编演示模块；EMQX WebSocket 双端接入 |
| **v1.5** | 2026-06-04 | ESP32 重构为完整 MQTT 网桥 (ESP-IDF mqtt_client 库)；RFID 卡号经 MQTT 上报 Qt；远程门锁控制 (SRV 指令)；AUTH:ADD/DEL/CLR 远程授权卡管理；传感器数据 QoS 1→0 修复 NanoMQ packet ID 重复错误；Web 仪表盘升级 ECharts + Paho MQTT + 灯光按钮自闭环 |
| **v1.6** | 2026-06-07 | Qt 新增 sensor_logs 传感器历史数据表 (10 秒聚合均值写入，WAL 模式，7 天自动清理)；历史回溯面板 (1h/6h/24h 摘要卡片 + 数据表格 + 一键清空)；卡号管理面板 (授权/取消/删除/清空实时同步 STM32)；门锁非对称消抖 (开立即/关两帧确认)；实时图表 5 点滑动平均滤波 |

---

> *仓瞳 StWHouse 1.0 — 从晶体管到云端，守护仓库的每一度温度、每一缕光线和每一次开门。*
