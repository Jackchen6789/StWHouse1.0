// /**
//  * @file    main.c
//  * @brief   ESP32-S3-N16R8 仓瞳 Web 控制端 (极致科技感全响应式大屏)
//  * WiFi STA 连路由器 + 异步局部刷新接口 + /fan 控风扇
//  */

// #include "driver/uart.h"
// #include "esp_event.h"
// #include "esp_http_server.h"
// #include "esp_log.h"
// #include "esp_netif.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "hal/gpio_types.h"
// #include "nvs_flash.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #define TAG "StWHouse"

// /* ==================== 基础参数配置 ==================== */
// #define WIFI_SSID "M3307_2.4G"
// #define WIFI_PASS "m3307dsgcxz"

// #define UART_PORT_NUM UART_NUM_1
// #define UART_TX_PIN GPIO_NUM_17
// #define UART_RX_PIN GPIO_NUM_18
// #define UART_BAUD_RATE 115200
// #define UART_BUF_SIZE 256

// /* ==================== 全局数据共享区 ==================== */
// typedef struct
// {
//     float temperature;
//     float humidity;
//     uint16_t mq2;
//     uint8_t pir;
//     uint8_t flame;
//     uint8_t servo;
//     uint8_t fan;
// } SensorData;

// static volatile SensorData g_wms_data = {
//     .temperature = 0.0, .humidity = 0.0, .mq2 = 0, .pir = 0, .flame = 0, .servo = 0, .fan = 0, .light = 0, .led_mode = 1, .led_brightness = 0};

// /* ======================================================================== */
// /* UART 初始化                                                              */
// /* ======================================================================== */
// static void uart_init(void)
// {
//     uart_config_t uart_config = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_DEFAULT,
//     };
//     uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
//     uart_param_config(UART_PORT_NUM, &uart_config);
//     uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
//                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
// }

// /* ======================================================================== */
// /* 向 STM32 发送风扇命令                                                    */
// /* ======================================================================== */
// static void uart_send_fan_cmd(uint8_t speed)
// {
//     char cmd[16];
//     int len = snprintf(cmd, sizeof(cmd), "FAN:%u\n", speed);
//     uart_write_bytes(UART_PORT_NUM, cmd, len);
//     ESP_LOGI(TAG, "--> STM32: FAN:%u", speed);
// }

// static int parse_frame(const char *line, volatile SensorData *out)
// {
//     int n = sscanf(line,
//                    "T:%f;H:%f;MQ:%hu;PIR:%hhu;FLM:%hhu;SRV:%hhu;FAN:%hhu;LGHT:%hu;LEDM:%hhu;LEDB:%hhu",
//                    &out->temperature,
//                    &out->humidity,
//                    &out->mq2,
//                    &out->pir,
//                    &out->flame,
//                    &out->servo,
//                    &out->fan);
//     return (n == 7) ? 0 : -1;
// }

// /* ======================================================================== */
// /* WiFi 事件监听                                                            */
// /* ======================================================================== */
// static void wifi_event_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
//     {
//         esp_wifi_connect();
//     }
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
//     {
//         esp_wifi_connect();
//         ESP_LOGI(TAG, "连接断开，正在尝试重连...");
//     }
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
//     {
//         ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//         ESP_LOGI(TAG, "WiFi 连接成功! IP: " IPSTR, IP2STR(&event->ip_info.ip));
//     }
// }

// void wifi_init_sta(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_sta();
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     esp_event_handler_instance_t instance_any_id;
//     esp_event_handler_instance_t instance_got_ip;
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(
//         WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(
//         IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

//     wifi_config_t wifi_config = {
//         .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS},
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());
// }

// /* ======================================================================== */
// /* HTTP: /api/data ── 返回实时传感器 JSON                                     */
// /* ======================================================================== */
// static esp_err_t data_get_handler(httpd_req_t *req)
// {
//     char json_response[256];
//     snprintf(json_response, sizeof(json_response),
//              "{\"temp\":%.1f,\"humi\":%.1f,\"mq2\":%u,"
//              "\"pir\":%d,\"flame\":%d,\"servo\":%d,\"fan\":%u}",
//              g_wms_data.temperature,
//              g_wms_data.humidity,
//              g_wms_data.mq2,
//              g_wms_data.pir,
//              g_wms_data.flame,
//              g_wms_data.servo,
//              g_wms_data.fan);
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
//     httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
//     return ESP_OK;
// }

// static const httpd_uri_t data_uri = {
//     .uri = "/api/data",
//     .method = HTTP_GET,
//     .handler = data_get_handler,
//     .user_ctx = NULL};

// /* ======================================================================== */
// /* HTTP: /fan?speed=N ── 风扇控制                                            */
// /* ======================================================================== */
// static esp_err_t fan_handler(httpd_req_t *req)
// {
//     char buf[32];
//     if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK)
//     {
//         char param[8];
//         if (httpd_query_key_value(buf, "speed", param, sizeof(param)) == ESP_OK)
//         {
//             int speed = atoi(param);
//             if (speed < 0)
//                 speed = 0;
//             if (speed > 100)
//                 speed = 100;
//             uart_send_fan_cmd((uint8_t)speed);
//         }
//     }
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_send(req, "{\"ok\":1}", HTTPD_RESP_USE_STRLEN);
//     return ESP_OK;
// }

// static const httpd_uri_t fan_uri = {
//     .uri = "/fan",
//     .method = HTTP_GET,
//     .handler = fan_handler,
//     .user_ctx = NULL};

// /* ======================================================================== */
// /* HTTP: / ── 极客数智化看板大屏主页 (零外部链接依赖)                            */
// /* ======================================================================== */
// static const char *INDEX_HTML = R"html(
// <!DOCTYPE html>
// <html lang='zh'>
// <head>
//     <meta charset='UTF-8'>
//     <meta name='viewport' content='width=device-width,initial-scale=1.0'>
//     <title>仓瞳 StWHouse 控制系统</title>
//     <style>
//         * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', system-ui, sans-serif; }
//         body { background: #0f172a; color: #f8fafc; padding: 24px 16px; min-height: 100vh; display: flex; flex-direction: column; align-items: center; }

//         .container { width: 100%; max-width: 800px; }

//         header { text-align: center; margin-bottom: 24px; }
//         header h1 { font-size: 28px; font-weight: 700; letter-spacing: 2px; color: #f8fafc; text-shadow: 0 0 20px rgba(56,189,248,0.2); }
//         header h1 span { color: #38bdf8; position: relative; }
//         header .sub { font-size: 13px; color: #64748b; margin-top: 6px; letter-spacing: 1px; }
//         header .sub span { color: #38bdf8; font-weight: 600; }

//         .grid-layout { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 16px; margin-bottom: 16px; }

//         .card { background: #1e293b; border-radius: 16px; padding: 20px; border: 1px solid #334155; box-shadow: 0 10px 25px -5px rgba(0,0,0,0.3); transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1); }
//         .card:hover { border-color: #38bdf8; transform: translateY(-2px); box-shadow: 0 12px 30px -5px rgba(56,189,248,0.15); }

//         .card-header { display: flex; justify-content: space-between; align-items: center; font-size: 13px; color: #94a3b8; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 10px; }
//         .card-main { display: flex; align-items: baseline; justify-content: space-between; }
//         .val { font-size: 36px; font-weight: 700; color: #f8fafc; font-family: 'Courier New', Courier, monospace; }
//         .unit { font-size: 14px; color: #64748b; margin-left: 4px; font-weight: normal; }

//         /* 风扇控制专区 */
//         .fan-card { grid-column: span 1; display: flex; flex-direction: column; justify-content: space-between; }
//         .fan-row { display: flex; align-items: center; justify-content: space-between; margin-top: 4px; }

//         .switch { position: relative; display: inline-block; width: 50px; height: 26px; }
//         .switch input { opacity: 0; width: 0; height: 0; }
//         .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #334155; border-radius: 34px; transition: 0.3s; }
//         .slider:before { position: absolute; content: ""; height: 20px; width: 20px; left: 3px; bottom: 3px; background: #94a3b8; border-radius: 50%; transition: 0.3s; }
//         input:checked + .slider { background: #38bdf8; }
//         input:checked + .slider:before { transform: translateX(24px); background: #0f172a; }

//         input[type=range] { -webkit-appearance: none; width: 100%; height: 6px; border-radius: 10px; background: #334155; outline: none; margin: 18px 0 8px 0; }
//         input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 22px; height: 22px; border-radius: 50%; background: #38bdf8; cursor: pointer; border: 2px solid #1e293b; transition: .15s; }
//         input[type=range]::-webkit-slider-thumb:hover { transform: scale(1.15); box-shadow: 0 0 12px #38bdf8; }

//         .fan-status { font-size: 11px; color: #64748b; text-align: right; font-style: italic; margin-top: 2px; }

//         /* 安全防灾底栏 */
//         .security-card { background: #1e293b; border-radius: 16px; padding: 18px 20px; border: 1px solid #334155; }
//         .sec-title { font-size: 14px; font-weight: 600; color: #94a3b8; letter-spacing: 0.5px; margin-bottom: 14px; text-transform: uppercase; border-bottom: 1px solid #334155; padding-bottom: 8px; }
//         .sec-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; }
//         .sec-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 14px; background: #0f172a; border-radius: 10px; border: 1px solid #1e293b; }
//         .sec-label { font-size: 13px; color: #cbd5e1; }

//         .badge { padding: 4px 12px; border-radius: 6px; font-size: 12px; font-weight: 700; letter-spacing: 0.5px; text-align: center; min-width: 72px; transition: all 0.3s; }
//         .badge-ok { background: rgba(16,185,129,0.12); color: #10b981; border: 1px solid rgba(16,185,129,0.3); }
//         .badge-warn { background: rgba(239,68,68,0.12); color: #ef4444; border: 1px solid rgba(239,68,68,0.4); animation: pulse 1.6s infinite; }
//         .badge-info { background: rgba(56,189,248,0.12); color: #38bdf8; border: 1px solid rgba(56,189,248,0.3); }

//         @keyframes pulse {
//             0% { opacity: 1; box-shadow: 0 0 0 0 rgba(239,68,68,0.3); }
//             70% { opacity: 0.7; box-shadow: 0 0 0 6px rgba(239,68,68,0); }
//             100% { opacity: 1; box-shadow: 0 0 0 0 rgba(239,68,68,0); }
//         }
//     </style>
// </head>
// <body>
//     <div class='container'>
//         <header>
//             <h1>仓<span>瞳</span> 智能仓储大屏</h1>
//             <div class='sub'>边缘分布式控制节点 | Host: <span id='ip'>--</span></div>
//         </header>

//         <div class='grid-layout'>
//             <div class='card'>
//                 <div class='card-header'><span>🌡️ 环境温度</span></div>
//                 <div class='card-main'><div class='val' id='t_val'>--.-</div><div class='unit'>℃</div></div>
//             </div>
//             <div class='card'>
//                 <div class='card-header'><span>💧 环境湿度</span></div>
//                 <div class='card-main'><div class='val' id='h_val'>--.-</div><div class='unit'>%</div></div>
//             </div>
//             <div class='card'>
//                 <div class='card-header'><span>💨 烟雾浓度</span></div>
//                 <div class='card-main'><div class='val' id='m_val'>----</div><div class='unit'>ADC</div></div>
//             </div>

//             <div class='card fan-card'>
//                 <div class='card-header'><span>🌀 对流风机控制</span><span id='speedDisp' style='color:#38bdf8;font-weight:700;'>0%</span></div>
//                 <input type='range' id='fanSlider' min='0' max='100' value='0' oninput='onSlider()' onchange='sendFan()'>
//                 <div class='fan-row'>
//                     <label class='switch'>
//                         <input type='checkbox' id='fanToggle' onchange='onToggle()'>
//                         <span class='slider'></span>
//                     </label>
//                     <div class='fan-status' id='fanStatus'>系统就绪</div>
//                 </div>
//             </div>
//         </div>

//         <div class='security-card'>
//             <div class='sec-title'>🛡️ 仓储灾防安防矩阵</div>
//             <div class='sec-grid'>
//                 <div class='sec-row'><span class='sec-label'>红外防盗状态</span><span class='badge badge-ok' id='pir_badge'>正常</span></div>
//                 <div class='sec-row'><span class='sec-label'>火焰高危侦测</span><span class='badge badge-ok' id='flm_badge'>安全</span></div>
//                 <div class='sec-row'><span class='sec-label'>核心柜锁状态</span><span class='badge badge-info' id='srv_badge'>闭锁</span></div>
//                 <div class='sec-row'><span class='sec-label'>当前回传转速</span><span id='f_val' style='color:#38bdf8;font-weight:700;font-family:monospace;font-size:16px;'>0%</span></div>
//             </div>
//         </div>
//     </div>

//     <script>
//         var slider = document.getElementById('fanSlider');
//         var toggle = document.getElementById('fanToggle');
//         var disp = document.getElementById('speedDisp');
//         var fstat = document.getElementById('fanStatus');

//         function onSlider() {
//             var v = parseInt(slider.value);
//             disp.textContent = v + '%';
//             toggle.checked = (v > 0);
//         }

//         function onToggle() {
//             if (toggle.checked) {
//                 if (parseInt(slider.value) == 0) slider.value = 100;
//             } else {
//                 slider.value = 0;
//             }
//             disp.textContent = slider.value + '%';
//             sendFan(); // 开关切换直接发送
//         }

//         function sendFan() {
//             var v = slider.value;
//             fstat.style.color = '#64748b';
//             fstat.textContent = '同步中...';
//             fetch('/fan?speed=' + v)
//                 .then(r => r.json())
//                 .then(data => {
//                     if(data.ok) {
//                         fstat.style.color = '#10b981';
//                         fstat.textContent = '已应用: ' + v + '%';
//                     }
//                 })
//                 .catch(() => {
//                     fstat.style.color = '#ef4444';
//                     fstat.textContent = '通信失败';
//                 });
//         }

//         // 自动拉取最新的执行状态与传感器状态
//         setInterval(async () => {
//             try {
//                 var r = await fetch('/api/data');
//                 if (!r.ok) return;
//                 var d = await r.json();

//                 document.getElementById('t_val').innerText = d.temp.toFixed(1);
//                 document.getElementById('h_val').innerText = d.humi.toFixed(1);
//                 document.getElementById('m_val').innerText = d.mq2;
//                 document.getElementById('f_val').innerText = d.fan + '%';

//                 var p = document.getElementById('pir_badge');
//                 if (d.pir) { p.innerText = '⚠ 闯入'; p.className = 'badge badge-warn'; }
//                 else { p.innerText = '正常'; p.className = 'badge badge-ok'; }

//                 var l = document.getElementById('flm_badge');
//                 if (d.flame) { l.innerText = '🚨 火警'; l.className = 'badge badge-warn'; }
//                 else { l.innerText = '安全'; l.className = 'badge badge-ok'; }

//                 var s = document.getElementById('srv_badge');
//                 if (d.servo) { s.innerText = '已开锁'; s.className = 'badge badge-warn'; }
//                 else { s.innerText = '闭锁状态'; s.className = 'badge badge-info'; }

//                 document.getElementById('ip').innerText = window.location.host;
//             } catch (e) {}
//         }, 1500);
//     </script>
// </body>
// </html>
// )html";

// static esp_err_t index_get_handler(httpd_req_t *req)
// {
//     httpd_resp_set_type(req, "text/html; charset=utf-8");
//     httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
//     return ESP_OK;
// }

// static const httpd_uri_t index_uri = {
//     .uri = "/",
//     .method = HTTP_GET,
//     .handler = index_get_handler,
//     .user_ctx = NULL};

// /* ======================================================================== */
// /* HTTP 服务器启动                                                           */
// /* ======================================================================== */
// httpd_handle_t start_webserver(void)
// {
//     httpd_handle_t server = NULL;
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//     config.max_uri_handlers = 10;

//     if (httpd_start(&server, &config) == ESP_OK)
//     {
//         httpd_register_uri_handler(server, &index_uri);
//         httpd_register_uri_handler(server, &data_uri);
//         httpd_register_uri_handler(server, &fan_uri);
//         return server;
//     }
//     return NULL;
// }

// /* ======================================================================== */
// /* UART 接收任务 (优化：修复高频空转可能引起的 WDT 看门狗复位)                */
// /* ======================================================================== */
// static void uart_rx_task(void *arg)
// {
//     uint8_t buf[UART_BUF_SIZE];
//     int buf_idx = 0;

//     while (1)
//     {
//         int len = uart_read_bytes(UART_PORT_NUM, buf + buf_idx, 1, pdMS_TO_TICKS(10));
//         if (len <= 0)
//         {
//             vTaskDelay(pdMS_TO_TICKS(5)); // 无数据时挂起，释放时间片给网络任务
//             continue;
//         }

//         if (buf[buf_idx] == '\n')
//         {
//             buf[buf_idx] = '\0';
//             if (buf_idx > 0 && buf[buf_idx - 1] == '\r')
//                 buf[--buf_idx] = '\0';

//             if (parse_frame((char *)buf, &g_wms_data) == 0)
//             {
//                 ESP_LOGI(TAG, "T:%.1f MQ:%u PIR:%d FLM:%d SRV:%d FAN:%u",
//                          g_wms_data.temperature, g_wms_data.mq2,
//                          g_wms_data.pir, g_wms_data.flame,
//                          g_wms_data.servo, g_wms_data.fan);
//             }
//             buf_idx = 0;
//             vTaskDelay(pdMS_TO_TICKS(2)); // 解析完一帧，主动出让调度
//         }
//         else if (++buf_idx >= UART_BUF_SIZE - 1)
//         {
//             buf_idx = 0;
//         }
//     }
// }

// /* ======================================================================== */
// /* 主函数                                                                   */
// /* ======================================================================== */
// void app_main(void)
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     ESP_LOGI(TAG, "=============================================");
//     ESP_LOGI(TAG, "  仓瞳 StWHouse ESP32-S3 数智化大屏系统就绪");
//     ESP_LOGI(TAG, "=============================================");

//     uart_init();
//     wifi_init_sta();
//     vTaskDelay(pdMS_TO_TICKS(4000));
//     start_webserver();

//     xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
// }

/**
 * @file     main.c
 * @brief    ESP32-S3-N16R8 仓瞳 MQTT 客户端 (对接 EMQX 边缘网关)
 * WiFi STA 连路由器 + 核心 MQTT 异步发布/订阅 + 优化级异步 UART 解析
 */

#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "mqtt_client.h" // 🌟 引入 ESP-IDF 官方 MQTT 库
#include "nvs_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "StWHouse_MQTT"

/* ==================== 基础参数配置 ==================== */
#define WIFI_SSID "M3307_2.4G"
#define WIFI_PASS "m3307dsgcxz"

// ⚠️ 记得改成你运行 EMQX Edge 的电脑的局域网实际 IP 地址！
#define EMQX_BROKER_URI "mqtt://192.168.123.59:1883"

// MQTT 通信主题配置
#define TOPIC_DATA "wms/warehouse1/data" // 上报传感器数据的 Topic
#define TOPIC_CMD "wms/warehouse1/cmd"   // 接收控制命令的 Topic

#define UART_PORT_NUM UART_NUM_1
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 256

/* ==================== 全局数据共享区 ==================== */
typedef struct
{
    float temperature;
    float humidity;
    uint16_t mq2;
    uint8_t pir;
    uint8_t flame;
    uint8_t servo;
    uint8_t fan;
    uint16_t light;
    uint8_t led_mode;
    uint8_t led_brightness;
    char last_card_uid[9];
} SensorData;

static volatile SensorData g_wms_data = {
    .temperature = 0.0, .humidity = 0.0, .mq2 = 0, .pir = 0, .flame = 0, .servo = 0, .fan = 0, .light = 0, .led_mode = 1, .led_brightness = 0};

// 全局 MQTT 客户端句柄
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool is_mqtt_connected = false;

/* ======================================================================== */
/* UART 初始化与发送命令                                                     */
/* ======================================================================== */
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void uart_send_fan_cmd(uint8_t speed)
{
    char cmd[16];
    int len = snprintf(cmd, sizeof(cmd), "FAN:%u\n", speed);
    uart_write_bytes(UART_PORT_NUM, cmd, len);
    ESP_LOGI(TAG, "--> STM32: FAN:%u", speed);
}

static int parse_frame(const char *line, volatile SensorData *out)
{
    int n = sscanf(line,
                   "T:%f;H:%f;MQ:%hu;PIR:%hhu;FLM:%hhu;SRV:%hhu;FAN:%hhu;LGHT:%hu;LEDM:%hhu;LEDB:%hhu",
                   &out->temperature,
                   &out->humidity,
                   &out->mq2,
                   &out->pir,
                   &out->flame,
                   &out->servo,
                   &out->fan,
                   &out->light,
                   &out->led_mode,
                   &out->led_brightness);
    return (n == 10) ? 0 : -1;
}

/* ======================================================================== */
/* MQTT 事件监听与解析控制指令                                                */
/* ======================================================================== */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "成功连上本地 EMQX Edge 服务器！");
        is_mqtt_connected = true;
        // 连上后，立刻订阅下发控制命令的主题
        esp_mqtt_client_subscribe(mqtt_client, TOPIC_CMD, 0);
        ESP_LOGI(TAG, "已订阅控制主题: %s", TOPIC_CMD);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "与 EMQX 服务器断开连接，正在自动重连...");
        is_mqtt_connected = false;
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "收到 EMQX 控制指令");
        if (strncmp(event->topic, TOPIC_CMD, event->topic_len) == 0)
        {
            char raw_data[64] = {0};
            int data_len = event->data_len > 63 ? 63 : event->data_len;
            memcpy(raw_data, event->data, data_len);

            int val = -1;

            /* LED 自动: {"led_mode":"auto"} */
            if (strstr(raw_data, "\"led_mode\":\"auto\"") != NULL)
            {
                ESP_LOGI(TAG, "LED -> 自动模式");
                uart_write_bytes(UART_PORT_NUM, "LED:AUTO\n", 9);
            }
            /* LED 手动: {"led_mode":"manual"} */
            else if (strstr(raw_data, "\"led_mode\":\"manual\"") != NULL)
            {
                ESP_LOGI(TAG, "LED -> 手动模式");
                uart_write_bytes(UART_PORT_NUM, "LED:MAN\n", 8);
            }
            /* LED 亮度: {"led_brightness":75} */
            else if (sscanf(raw_data, "{\"led_brightness\":%d}", &val) == 1)
            {
                if (val < 0)
                    val = 0;
                if (val > 100)
                    val = 100;
                ESP_LOGI(TAG, "LED 亮度: %d%%", val);
                char cmd[16];
                int len = snprintf(cmd, sizeof(cmd), "LED:%d\n", val);
                uart_write_bytes(UART_PORT_NUM, cmd, len);
            }
            /* 风扇: {"speed":75} */
            else if (sscanf(raw_data, "{\"speed\":%d}", &val) == 1)
            {
                if (val < 0)
                    val = 0;
                if (val > 100)
                    val = 100;
                ESP_LOGI(TAG, "风扇转速: %d%%", val);
                uart_send_fan_cmd((uint8_t)val);
            }
            /* 舵机: {"servo":1} 开门, {"servo":0} 关门 */
            else if (sscanf(raw_data, "{\"servo\":%d}", &val) == 1)
            {
                val = (val != 0) ? 1 : 0;
                ESP_LOGI(TAG, "舵机 -> %s", val ? "开门" : "关门");
                char cmd[16];
                int len = snprintf(cmd, sizeof(cmd), "SRV:%d\n", val);
                uart_write_bytes(UART_PORT_NUM, cmd, len);
            }
            /* AUTH:ADD:XXXXXXXX */
            else if (strncmp(raw_data, "{\"auth_add\":\"", 13) == 0)
            {
                char uid[9] = {0};
                memcpy(uid, raw_data + 13, 8);
                ESP_LOGI(TAG, "AUTH ADD: %s", uid);
                char abuf[32];
                int alen = snprintf(abuf, sizeof(abuf), "AUTH:ADD:%s\n", uid);
                uart_write_bytes(UART_PORT_NUM, abuf, alen);
            }
            /* AUTH:DEL:XXXXXXXX */
            else if (strncmp(raw_data, "{\"auth_del\":\"", 13) == 0)
            {
                char uid[9] = {0};
                memcpy(uid, raw_data + 13, 8);
                ESP_LOGI(TAG, "AUTH DEL: %s", uid);
                char abuf[32];
                int alen = snprintf(abuf, sizeof(abuf), "AUTH:DEL:%s\n", uid);
                uart_write_bytes(UART_PORT_NUM, abuf, alen);
            }
            /* AUTH:CLR */
            else if (strstr(raw_data, "\"auth_clr\":1") != NULL)
            {
                ESP_LOGI(TAG, "AUTH CLR");
                uart_write_bytes(UART_PORT_NUM, "AUTH:CLR\n", 9);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT 触发错误事件");
        break;
    default:
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = EMQX_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* ======================================================================== */
/* WiFi 事件监听                                                            */
/* ======================================================================== */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi 连接断开，正在自动重连...");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi 连上路由器啦! 分配到的 IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // 🌟 WiFi 获取到 IP 之后，再启动 MQTT 客户端连接服务器
        mqtt_app_start();
    }
}

void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* ======================================================================== */
/* UART 接收任务 (在此处加入高频的 MQTT 数据实时上报)                         */
/* ======================================================================== */
static void uart_rx_task(void *arg)
{
    uint8_t buf[UART_BUF_SIZE];
    int buf_idx = 0;

    while (1)
    {
        int len = uart_read_bytes(UART_PORT_NUM, buf + buf_idx, 1, pdMS_TO_TICKS(10));
        if (len <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(5)); // 释放时间片
            continue;
        }

        if (buf[buf_idx] == '\n')
        {
            buf[buf_idx] = '\0';
            if (buf_idx > 0 && buf[buf_idx - 1] == '\r')
                buf[--buf_idx] = '\0';

            // 成功解析出一帧 STM32 数据

            /* CARD:XXXXXXXX 卡号上报 MQTT */
            if (strncmp((char *)buf, "CARD:", 5) == 0)
            {
                ESP_LOGI(TAG, "RFID 卡号: %s", (char *)buf + 5);
                strncpy(g_wms_data.last_card_uid, (char *)buf + 5, 8);
                g_wms_data.last_card_uid[8] = 0;
            }
            else if (parse_frame((char *)buf, &g_wms_data) == 0)
            {
                ESP_LOGI(TAG, "解析成功 -> T:%.1f H:%.1f MQ:%u", g_wms_data.temperature, g_wms_data.humidity, g_wms_data.mq2);

                // 🌟 将数据转换为标准的 JSON 格式
                char json_payload[320];
                snprintf(json_payload, sizeof(json_payload),
                         "{\"temp\":%.1f,\"humi\":%.1f,\"mq2\":%u,"
                         "\"pir\":%d,\"flame\":%d,\"servo\":%d,\"fan\":%u,"
                         "\"light\":%u,\"led_mode\":%d,\"led_brightness\":%d,\"card\":\"%s\"}",
                         g_wms_data.temperature, g_wms_data.humidity, g_wms_data.mq2,
                         g_wms_data.pir, g_wms_data.flame, g_wms_data.servo, g_wms_data.fan,
                         g_wms_data.light, g_wms_data.led_mode, g_wms_data.led_brightness, g_wms_data.last_card_uid);
                // 🌟 通过 MQTT 一行命令打包推送上云
                if (is_mqtt_connected && mqtt_client != NULL)
                {
                    int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_DATA, json_payload, 0, 0, 0);
                    ESP_LOGI(TAG, "数据成功发布到 EMQX! MsgID: %d", msg_id);
                }
            }
            buf_idx = 0;
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        else if (++buf_idx >= UART_BUF_SIZE - 1)
        {
            buf_idx = 0;
        }
    }
}

/* ======================================================================== */
/* 主函数                                                                   */
/* ======================================================================== */
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, " 仓瞳 StWHouse ESP32-S3 MQTT 网关端节点就绪");
    ESP_LOGI(TAG, "=============================================");

    uart_init();
    wifi_init_sta(); // 内部会在连上 WiFi 后自动触发初始化 MQTT

    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
}
