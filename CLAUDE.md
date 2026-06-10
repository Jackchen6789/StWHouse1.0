# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 项目概述

仓瞳 (StWHouse) —— 智慧仓库环境安防系统，四层 IoT 架构：

```
STM32 (边缘采集) → ESP32 (MQTT 网桥) → EMQX Broker → Qt / Web 双端大屏
```

## 构建命令

### STM32（Keil MDK，ARMCC V5）

无 CLI 构建 —— 用 Keil MDK V5 打开 `Project.uvprojx`，`F7` 编译，ST-Link/DAP-Link 通过 SWD（PA13/PA14）烧录。

**预处理器宏：** `USE_STDPERIPH_DRIVER`、`STM32F10X_MD`

### ESP32（ESP-IDF v5.5.1）

```bash
cd ESP32/ESP32
idf.py build
idf.py -p COMx flash monitor
```

### Qt 桌面客户端（Qt 6.11.1 MinGW 64-bit）

```bash
cd qt/StWHouseDash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## 架构

### 数据流向

1. **STM32 `User/main.c`** — 200ms 主循环 5 步流水线：读全部传感器 → 逻辑决策（RFID门禁/灯光自动/红外告警）→ OLED 四行刷新 → USART1 TX 帧上报 → USART1 RX 指令派发
2. **ESP32 `main/main.c`**（有效代码从第 478 行开始；1-477 行是已废弃的旧 HTTP Server，以注释保留）—— UART1 桥接：解析 `T:...;H:...` 帧 → 转 JSON → MQTT 发布 `wms/warehouse1/data`；订阅 `wms/warehouse1/cmd` → 转发控制指令到 STM32
3. **EMQX Broker** `192.168.123.59:1883`（MQTT）/ `:8083`（WebSocket）—— 消息路由中枢
4. **Qt `mainwindow.cpp`** — 管理员桌面客户端：实时仪表盘 + 用户管理 + sensor_logs 历史回溯 + RFID 卡号管理；手写 MQTT 3.1.1 协议栈（`SimpleMqttClient`）
5. **Web `index.html`** — 纯静态仪表盘（ECharts 5.4 + Paho MQTT WebSocket），浏览器直接打开

### 关键目录

| 目录 | 用途 |
|------|------|
| `Hardware/` | STM32 外设驱动，13 个模块（DHT11、MQ2、PIR、RC522、OLED、Fan、Servo、LED 等） |
| `User/` | STM32 应用层：`main.c`、`stm32f10x_it.c`（USART1 中断服务）、`asm_demo.c`（ARM 内联汇编演示） |
| `Library/` | STM32 标准外设库 V3.5（厂商库，不要改） |
| `Start/` | 启动文件 + `system_stm32f10x` |
| `ESP32/ESP32/main/` | ESP32 MQTT 网桥固件 |
| `ESP32/ESP32/index.html` | Web 仪表盘 |
| `qt/StWHouseDash/` | Qt6 桌面客户端 |
| `System/` | 系统延时文件 |

### 通信协议

**STM32 → ESP32（USART1，115200 8N1，200ms 周期）：**
```
T:25.5;H:62.0;MQ:0847;PIR:1;FLM:0;SRV:1;FAN:075;LGHT:2048;LEDM:1;LEDB:050\r\n
```

**ESP32 → STM32（USART1）：**
```
FAN:75\n           → 风扇转速 0~100%
LED:AUTO\n         → LED 自动模式
LED:MAN\n          → LED 手动模式
LED:50\n           → LED 亮度（仅手动模式）
SRV:1\n / SRV:0\n  → 远程开关门
AUTH:ADD:XXXXXXXX\n → 添加授权卡号
AUTH:DEL:XXXXXXXX\n → 删除授权卡号
AUTH:CLR\n         → 清空全部授权卡
```

**MQTT Topic（QoS 0）：**
- 发布：`wms/warehouse1/data` — 传感器 JSON（11 字段，含 `card`）
- 订阅：`wms/warehouse1/cmd` — 控制 JSON（`speed`、`led_mode`、`led_brightness`、`servo`、`auth_add`、`auth_del`、`auth_clr`）

### 引脚分配要点（STM32F103C8T6）

关键重映射：
- **PB3**：LED PWM（TIM2_CH2），需 `GPIO_FullRemap_TIM2`，默认 JTAG 引脚须先 `GPIO_Remap_SWJ_JTAGDisable`
- **PB4**：风扇 PWM（TIM3_CH1），需 `GPIO_PartialRemap_TIM3`
- 芯片疑似克隆品，**无 USART3**，调试输出只能用 USART1（与 ESP32 共用）或 OLED

## 开发注意事项

- **MQTT QoS**：传感器数据使用 QoS 0，避免 NanoMQ 报 `packet id duplicates` 错误。不要改回 QoS 1。
- **PIR 消抖**：滑动窗口方案已尝试并还原，不要再加。红外检测直接用 GPIO 原始读数。
- **USART3 不可用**：芯片克隆品无 USART3，不要尝试用 USART3 做调试输出。
- **DHT11 时序**：当前 `Delay_us()` 基于 SysTick（分辨率约 14μs），偶尔读失败。NOP 汇编精确延时方案已规划但未实施，参见 TODO.md。
- **网络配置**：路由器 `M3307_2.4G`，Broker `192.168.123.59:1883`。WiFi 密码硬编码在 `ESP32/ESP32/main/main.c`（`WIFI_SSID`/`WIFI_PASS` 宏）。
- **MQTT 认证**：ESP32 和 Qt 代码已预留 username/password 字段，当前使用匿名模式。
- **`network_config.txt`**：含 WiFi 密码，已 git-ignore，不要提交。
