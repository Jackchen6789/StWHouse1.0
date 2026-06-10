#ifndef __RCC522_H
#define __RCC522_H

#include "stm32f10x.h"

/* ==================== 状态返回值 ==================== */
#define MI_OK                          0
#define MI_NOTAGERR                    1
#define MI_ERR                         2

/* ==================== RC522 / PICC 命令字 ==================== */
#define REQ_IDLE                       0x26    // 寻天线区内未进入休眠状态的卡
#define REQ_ALL                        0x52    // 寻天线区内全部卡
#define PICC_ANTICOLL                  0x93    // 防冲突
#define PICC_SELECTTAG                 0x93    // 选卡
#define PICC_AUTHENT1A                 0x60    // 验证A密钥
#define PICC_AUTHENT1B                 0x61    // 验证B密钥
#define PICC_READ                      0x30    // 读块
#define PICC_WRITE                     0xA0    // 写块

/* ==================== 函数声明 ==================== */
void RC522_Init(void);
void RC522_Reset(void);
void RC522_AntennaOn(void);
void RC522_AntennaOff(void);

uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType);
uint8_t RC522_Anticoll(uint8_t *serialNumber);
uint8_t RC522_SelectTag(uint8_t *serialNumber);
uint8_t RC522_Authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *serialNumber);
uint8_t RC522_ReadBlock(uint8_t blockAddr, uint8_t *data);
uint8_t RC522_WriteBlock(uint8_t blockAddr, uint8_t *data);

#endif

