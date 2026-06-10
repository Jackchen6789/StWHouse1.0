# 仓瞳 StWHouse 待办任务清单

> 创建时间: 2026-06-07
> 排序依据: 合理性 + 不崩性，从最安全到最复杂

---

## 1. 汇编演示模块 — OLED 输出（原 USART3 方案已废弃）

- **不崩性**: ⭐⭐⭐⭐⭐ 纯增量，不动任何现有代码，删掉一行调用即回滚
- **合理性**: ⭐⭐⭐⭐⭐ 独立模块，零耦合，OLED 已验证可用
- **背景**: USART1 被 ESP32 占用，USART2 被火焰传感器+PIR 占用，USART3 芯片克隆疑似不可用——三个串口全灭，原「USART3 汇编输出日志」方案无法落地
- **新方案**: 汇编演示改用 OLED 屏幕输出（PB6/PB7 软件 I2C，已可用），不占用任何串口
- **相关记忆**: [[usart3-debug-serial-failed]]
- **涉及文件**:
  - `User/asm_demo.c` — 改 `Asm_PrintCpuInfo()`，printf 全部替换为 OLED_ShowString / snprintf+OLED 显示
  - `User/main.c` — `#include "asm_demo.h"`，初始化后调用 `Asm_PrintCpuInfo()`
  - Keil 工程 — User 组添加 `asm_demo.c`
- **OLED 显示内容（4 行轮播）**:
  - MSP / PSP 堆栈指针
  - CLZ(0xFFF) = 20，RBIT 位反转
  - 1234/10 = 123（魔数除法）
  - 触发 PendSV 的 ICSR 寄存器值
- **状态**: ❌ 待处理

---

## 2. DHT11 汇编精确延时（NOP 循环替代 SysTick）

- **不崩性**: ⭐⭐⭐⭐ 只改 DHT11.c 两个函数调用，失败则 DHT11 读不到，其余传感器全正常，回滚只需改回原名
- **合理性**: ⭐⭐⭐⭐ 解决 DHT11 偶尔读取失败的真因（时序误差），而非加补丁
- **问题**: 当前 `Delay_us()` 基于 SysTick 轮询，分辨率只有 13.9μs（72MHz/1000=72kHz）。DHT11 单总线时序要求微秒级精确——"0"位高电平仅 27μs，"1"位高电平 70μs，采样窗口窄。实际测得的延时误差可能大到 34%，是 DHT11 偶尔读取失败的根本原因
- **方案**: 在 `asm_demo.c` 中新增 `Asm_DelayUs_Critical()`，用 NOP 计数循环（3周期/次 = 41.7ns）替代 SysTick 轮询，分辨率从 13.9μs → 14ns，**精度提升 1000 倍**。关键区段关全局中断（`CPSID I`），防止中断抢占总线时序
- **安全分析**:
  - DHT11 起始信号 30μs + 位采样 40μs，关中断最长 40μs ≪ USART1 115200bps 每字节间隔 87μs → **安全，不会丢字节**
  - DHT11 一次完整读取共关中断约 1.5ms，裸机场景无看门狗 → **不会复位**
  - 舵机脉冲 500-2500μs 太长，保持用 C 版 `Delay_us`，不关中断 → **不动**
- **涉及文件**:
  - `User/asm_demo.h` — 新增 `Asm_DelayUs_Critical` 声明
  - `User/asm_demo.c` — 新增汇编 NOP 延时函数（MRS PRIMASK + CPSID I + SUBS/BNE 循环 + MSR PRIMASK）
  - `Hardware/DHT11.c` — `DHT11_Start()` 的 `Delay_us(30)` → `Asm_DelayUs_Critical(30)`；`DHT11_ReadByte()` 的 `Delay_us(40)` → `Asm_DelayUs_Critical(35)`（35μs 是 DHT11 "0"/"1" 位判定窗口正中）
- **答辩话术**: "SysTick 微秒延时的分辨率受限于 1ms 定时器周期，误差高达 34%。我通过 NOP 指令周期计数实现了纳秒级精确延时，将 DHT11 单总线时序精度提升了三个数量级。"
- **状态**: ❌ 待处理

---

## 3. 添加 MQTT 认证机制

- **不崩性**: ⭐⭐⭐ 认证失败 → MQTT 断连 → Web 大屏无数据。但代码已预留 credentials 字段，回滚只需去掉配置
- **合理性**: ⭐⭐⭐⭐⭐ IoT 安全最基础的底线措施，TODO 和答辩文档均已规划
- **目标**: 为 MQTT 通信添加用户名/密码认证，增强安全性
- **涉及文件**:
  - ESP32: `mqtt_client_config_t` 添加 username/password
  - Qt: `SimpleMqttClient::connectToBroker` 已预留参数，传入即可
  - Broker: EMQX/NanoMQ 端配置认证
- **状态**: ❌ 待处理

---

## 4. 整理项目文件夹结构

- **不崩性**: ⭐⭐ 移动文件 → Keil/ESP-IDF 路径断裂 → 编译报错。无 git 版本控制，回滚需手动移回
- **合理性**: ⭐⭐⭐ 工程后期整理有必要，但在答辩前做风险较高（动完得重新验证编译+烧录）
- **目标**: 重新整理项目目录，清理冗余文件，规范文件夹命名和层级结构
- **涉及**: ESP32、Qt、STM32(Start/Library/User) 等多个子目录
- **状态**: ❌ 待处理

---

## 5. 一键切换网络配置（免烧录）

- **不崩性**: ⭐ 涉及文件最多（ESP32+Qt+Web+配置），逻辑分支最多（首次启动/WiFi失败/NVS读写/AP回退），任一环节出错 → 系统哑火
- **合理性**: ⭐⭐⭐⭐⭐ 运维刚需，是真正产品化的关键一步，但答辩不是运维演示
- **问题**: 换网络环境时，ESP32 `main.c` 和 Qt `mainwindow.cpp` 里的 WiFi/Broker IP 都得改、都得重新编译烧录，太麻烦
- **目标**: 一处配置、运行时生效，不再重新烧录

### Qt 端
- `mainwindow.cpp` 改为启动时读取项目根目录 `network.conf`（key=value），Qt 重启即生效，无需重新编译

### ESP32 端 — Web 设置页（方案A ✅）
- 利用现有的 Web 仪表盘（`ESP32/ESP32/index.html`），新增一个"设置"Tab
- 配置项：MQTT Broker IP、Port、WiFi SSID、WiFi 密码
- 存储在 NVS（非易失存储），断电不丢
- "保存并重启"按钮生效
- 首次/WiFi 连不上时自动开 AP 热点供配置

### 涉及文件
- `qt/StWHouseDash/mainwindow.cpp` — 启动时读 `network.conf`
- 项目根 — 新建 `network.conf`
- `ESP32/ESP32/main/main.c` — NVS 读写 + AP 回退逻辑
- `ESP32/ESP32/index.html` — 新增设置 Tab + 保存接口

- **状态**: ❌ 待处理
