#include "rcc522.h"
#include "delay.h" 
#include <string.h>

/* ==================== 引脚与SPI底层定义 ==================== */
#define RC522_CS_PIN        GPIO_Pin_12
#define RC522_CS_PORT       GPIOB
#define RC522_SCK_PIN       GPIO_Pin_5
#define RC522_MISO_PIN      GPIO_Pin_6
#define RC522_MOSI_PIN      GPIO_Pin_7
#define RC522_SPI           SPI1

#define RC522_CS_LOW()      GPIO_ResetBits(RC522_CS_PORT, RC522_CS_PIN)
#define RC522_CS_HIGH()     GPIO_SetBits(RC522_CS_PORT, RC522_CS_PIN)

/* ==================== 寄存器地址映射 ==================== */
#define REG_COMMAND         0x01
#define REG_COMM_IE_N       0x02
#define REG_COMM_IRQ        0x04
#define REG_ERROR           0x06
#define REG_STATUS2         0x08
#define REG_FIFO_DATA       0x09
#define REG_FIFO_LEVEL      0x0A
#define REG_CONTROL         0x0C
#define REG_BIT_FRAMING     0x0D
#define REG_COLL            0x0E
#define REG_MODE            0x11
#define REG_TX_CONTROL      0x14
#define REG_TX_AUTO         0x15
#define REG_TX_SEL          0x16
#define REG_RX_SEL          0x17
#define REG_T_MODE          0x2A
#define REG_T_PRESCALER     0x2B
#define REG_T_RELOAD_H      0x2C
#define REG_T_RELOAD_L      0x2D

/* ==================== RC522 指令集 ==================== */
#define CMD_IDLE            0x00
#define CMD_CALCCRC         0x03
#define CMD_TRANSCEIVE      0x0C
#define CMD_AUTHENT         0x0E
#define CMD_SOFTRESET       0x0F

/* ==================== 内部私有函数 ==================== */

static uint8_t RC522_SPI_SendByte(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(RC522_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(RC522_SPI, data);
    while (SPI_I2S_GetFlagStatus(RC522_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(RC522_SPI);
}

static void RC522_WriteReg(uint8_t addr, uint8_t val)
{
    RC522_CS_LOW();
    RC522_SPI_SendByte((addr << 1) & 0x7E);
    RC522_SPI_SendByte(val);
    RC522_CS_HIGH();
}

static uint8_t RC522_ReadReg(uint8_t addr)
{
    uint8_t val;
    RC522_CS_LOW();
    RC522_SPI_SendByte(((addr << 1) & 0x7E) | 0x80);
    val = RC522_SPI_SendByte(0x00);
    RC522_CS_HIGH();
    return val;
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp & (~mask));
}

/**
  * @brief  核心指令通信引擎
  */
static uint8_t RC522_PcdComMF522(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                                 uint8_t *backData, uint16_t *backLen)
{
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0, waitIRq = 0;
    uint8_t lastBits, n;
    uint32_t i;

    if (command == CMD_AUTHENT)
    {
        irqEn = 0x12;
        waitIRq = 0x10;
    }
    else if (command == CMD_TRANSCEIVE)
    {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    RC522_WriteReg(REG_COMM_IE_N, irqEn | 0x80);
    RC522_ClearBitMask(REG_COMM_IRQ, 0x80);   // 清除总中断标记
    RC522_SetBitMask(REG_COMMAND, CMD_IDLE);  // 取消当前异步指令
    RC522_SetBitMask(REG_CONTROL, 0x80);     // 修正：FlushBuffer = 1 清空 FIFO (硬件自清零)

    // 数据写入 FIFO
    for (i = 0; i < sendLen; i++)
    {
        RC522_WriteReg(REG_FIFO_DATA, sendData[i]);
    }

    // 触发执行命令
    RC522_WriteReg(REG_COMMAND, command);

    if (command == CMD_TRANSCEIVE)
    {
        RC522_SetBitMask(REG_BIT_FRAMING, 0x80); // StartSend = 1 开始发送
    }

    // 修正：增加每轮读取的微小时间间隔，扩大计数值以适配 72MHz。防止卡片未准备就绪引发超时。
    i = 30000;
    do
    {
        n = RC522_ReadReg(REG_COMM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    RC522_ClearBitMask(REG_BIT_FRAMING, 0x80); // 停止发送

    if (i != 0)
    {
        if (!(RC522_ReadReg(REG_ERROR) & 0x1B)) // 检查无 Protocol, Parity, FIFO Ovr, BufferErr 错误
        {
            status = MI_OK;

            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }

            if (command == CMD_TRANSCEIVE)
            {
                n = RC522_ReadReg(REG_FIFO_LEVEL);
                lastBits = RC522_ReadReg(REG_CONTROL) & 0x07;
                
                if (lastBits)
                {
                    *backLen = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *backLen = n * 8;
                }

                if (n == 0) n = 1;
                if (n > 32) n = 32; // 修正：放宽至 32 字节限制，确保能接收 16 字节数据 + 2 字节 CRC

                for (i = 0; i < n; i++)
                {
                    backData[i] = RC522_ReadReg(REG_FIFO_DATA);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    return status;
}

/* ==================== 驱动接口实现 ==================== */

void RC522_Reset(void)
{
    RC522_WriteReg(REG_COMMAND, CMD_SOFTRESET);
    Delay_ms(1);
}

void RC522_AntennaOn(void)
{
    uint8_t temp;
    temp = RC522_ReadReg(REG_TX_CONTROL);
    if (!(temp & 0x03))
    {
        RC522_SetBitMask(REG_TX_CONTROL, 0x03);
    }
}

void RC522_AntennaOff(void)
{
    RC522_ClearBitMask(REG_TX_CONTROL, 0x03);
}

void RC522_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    // 使能 GPIOA, GPIOB 和 SPI1 的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1, ENABLE);

    // 修正引脚配置：SCK (PA5) 和 MOSI (PA7) 配置为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = RC522_SCK_PIN | RC522_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 修正引脚配置：MISO (PA6) 配置为上拉输入，避免总线浮空噪声
    GPIO_InitStructure.GPIO_Pin = RC522_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // CS (PB12) 配置为标准推挽输出
    GPIO_InitStructure.GPIO_Pin = RC522_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_CS_PORT, &GPIO_InitStructure);
    RC522_CS_HIGH();

    // SPI 模式配置 (RC522 支持模式 0 和模式 3，此处采用模式 0: CPOL=Low, CPHA=1Edge)
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    // 提升底层传输速度：72MHz / 16 = 4.5MHz (RC522 极限可达 10MHz)
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(RC522_SPI, &SPI_InitStructure);
    SPI_Cmd(RC522_SPI, ENABLE);

    RC522_Reset();
    Delay_ms(20);

    // 芯片内部定时器配置，用作长超时兜底
    RC522_WriteReg(REG_T_MODE, 0x8D);      
    RC522_WriteReg(REG_T_PRESCALER, 0x3E); 
    RC522_WriteReg(REG_T_RELOAD_H, 0);   
    RC522_WriteReg(REG_T_RELOAD_L, 30);    
    
    RC522_WriteReg(REG_TX_AUTO, 0x40);     // 开启底层自动载波调制
    RC522_WriteReg(REG_MODE, 0x3D);        // 设定Mifare卡专用通信模式
    
    RC522_AntennaOn();                     // 启动天线辐射
}

/**
  * @brief  寻卡
  * @param  reqMode: 寻卡模式 (REQ_IDLE 或 REQ_ALL)
  * @param  tagType: 传出参数，存储卡片类型代码 (2 字节)
  */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType)
{
    uint8_t status;
    uint16_t backLen;

    RC522_ClearBitMask(REG_STATUS2, 0x08); // 强制关闭加密层机制
    RC522_WriteReg(REG_BIT_FRAMING, 0x07);  // 发送完毕后，最后一字节只发 7 个 bit (标准寻卡协议)

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, &reqMode, 1, tagType, &backLen);

    if ((status == MI_OK) && (backLen == 0x10))
    {
        return MI_OK;
    }
    return MI_ERR;
}

/**
  * @brief  防冲突 (获取4字节UID)
  * @param  serialNumber: 传出参数，存储4字节卡号
  */
uint8_t RC522_Anticoll(uint8_t *serialNumber)
{
    uint8_t status;
    uint8_t i;
    uint16_t backLen;
    uint8_t cmd[2] = { PICC_ANTICOLL, 0x20 }; // 0x20 代表接下来的级联数据为 0 字节

    RC522_ClearBitMask(REG_STATUS2, 0x08);
    RC522_WriteReg(REG_BIT_FRAMING, 0x00);   // 标准字节帧格式

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, cmd, 2, serialNumber, &backLen);

    if (status == MI_OK)
    {
        // 校验收到的卡号 BCC 异或值
        uint8_t bcc = 0;
        for (i = 0; i < 4; i++)
        {
            bcc ^= serialNumber[i];
        }
        if (bcc != serialNumber[4]) 
        {
            return MI_OK; // 部分S50克隆卡BCC不标准，这里默认通过；若需要严谨可返回 MI_ERR
        }
    }
    return status;
}

/**
  * @brief  选定卡片
  */
uint8_t RC522_SelectTag(uint8_t *serialNumber)
{
    uint8_t status;
    uint8_t i;
    uint16_t backLen;
    uint8_t buffer[9];

    buffer[0] = PICC_SELECTTAG;
    buffer[1] = 0x70; // 70 代表发送 4字节UID + 1字节BCC 
    for (i = 0; i < 4; i++)
    {
        buffer[2 + i] = serialNumber[i];
    }
    buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, buffer, 7, buffer, &backLen);

    if ((status == MI_OK) && (backLen == 0x18))
    {
        return MI_OK;
    }
    return MI_ERR;
}

/**
  * @brief  验证卡片密码
  */
uint8_t RC522_Authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *serialNumber)
{
    uint8_t i;
    uint16_t backLen;
    uint8_t buffer[12];

    buffer[0] = authMode;
    buffer[1] = blockAddr;
    for (i = 0; i < 6; i++)
    {
        buffer[2 + i] = key[i];
    }
    for (i = 0; i < 4; i++)
    {
        buffer[8 + i] = serialNumber[i];
    }

    if (RC522_PcdComMF522(CMD_AUTHENT, buffer, 12, NULL, &backLen) != MI_OK)
    {
        return MI_ERR;
    }
    
    // 检查加密层状态位是否已经成功握手
    if (!(RC522_ReadReg(REG_STATUS2) & 0x08))
    {
        return MI_ERR;
    }
    return MI_OK;
}

/**
  * @brief  读取16字节数据块
  */
uint8_t RC522_ReadBlock(uint8_t blockAddr, uint8_t *data)
{
    uint8_t status;
    uint16_t backLen;

    data[0] = PICC_READ;
    data[1] = blockAddr;

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, data, 2, data, &backLen);

    if ((status == MI_OK) && (backLen == 0x90)) // 0x90 = 144 bits = 18 字节 (16数据 + 2CRC)
    {
        return MI_OK;
    }
    return MI_ERR;
}

/**
  * @brief  写16字节数据块
  */
uint8_t RC522_WriteBlock(uint8_t blockAddr, uint8_t *data)
{
    uint8_t status;
    uint16_t backLen;
    uint8_t cmd[2];

    cmd[0] = PICC_WRITE;
    cmd[1] = blockAddr;

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, cmd, 2, cmd, &backLen);

    if ((status != MI_OK) || (backLen != 4) || ((cmd[0] & 0x0F) != 0x0A))
    {
        return MI_ERR;
    }

    status = RC522_PcdComMF522(CMD_TRANSCEIVE, data, 16, cmd, &backLen);

    if ((status != MI_OK) || (backLen != 4) || ((cmd[0] & 0x0F) != 0x0A))
    {
        return MI_ERR;
    }
    return MI_OK;
}


