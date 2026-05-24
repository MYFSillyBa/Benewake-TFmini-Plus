#include "main.h"
#include "usart.h"
#include "string.h"

/* ------------------- 1. TFmini 输出的数据结构体 ------------------- */
typedef struct {
    float distance_m;    // 距离 (米)
		float distance_cm;
    uint16_t strength;   // 信号强度
    float temperature;   // 内部温度 (摄氏度)
} TFmini_Data_t;

/* ------------------- 2. TFmini 协议解包结构体 ------------------- */
#pragma pack(push, 1)
typedef struct {
    uint8_t head1;       // 帧头1: 0x59
    uint8_t head2;       // 帧头2: 0x59
    uint8_t dist_L;      // 距离低8位
    uint8_t dist_H;      // 距离高8位
    uint8_t strength_L;  // 信号强度低8位
    uint8_t strength_H;  // 信号强度高8位
    uint8_t temp_L;      // 温度低8位
    uint8_t temp_H;      // 温度高8位
    uint8_t checksum;    // 前8字节累加和
} TFmini_Frame_t;
#pragma pack(pop)

/* ------------------- 3. 硬件双缓冲与全局变量 ------------------- */
#define TFMINI_RX_BUF_NUM 9  // TFmini 一帧数据固定为 9 个字节

// 哪怕只有 9 个字节，在 H7 上也必须分配 32 字节并进行 32 字节对齐！
extern ALIGN_32BYTES(uint8_t tfmini_rx_buf[2][32]); 
extern TFmini_Data_t tfmini_data;  // 供上层调用的全局数据实例

void tfmini_to_data(const uint8_t *tfmini_buf, TFmini_Data_t *tfmini_ctrl);
void USART_RxDMA_MultiBuffer_Init(UART_HandleTypeDef *huart, uint32_t *DstAddress, uint32_t *SecondMemAddress, uint32_t DataLength);
void USER_USART2_RxHandler(UART_HandleTypeDef *huart, uint16_t Size);
void vofa( float data1,float data2,float data3,float data4);
