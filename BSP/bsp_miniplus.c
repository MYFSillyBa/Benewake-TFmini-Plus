#include "bsp_miniplus.h"

ALIGN_32BYTES(uint8_t tfmini_rx_buf[2][32]); 
TFmini_Data_t tfmini_data;  // 供上层调用的全局数据实例
static uint16_t dist_history[3] = {0};
static uint8_t filter_idx = 0;

void tfmini_to_data(const uint8_t *tfmini_buf, TFmini_Data_t *tfmini_ctrl)
{
    if (tfmini_buf == NULL || tfmini_ctrl == NULL)
    {
        return;
    }

    // H7 必须的 Cache 刷新，保证读取到 DMA 的最新数据
    SCB_InvalidateDCache_by_Addr((uint32_t *)tfmini_buf, 32);

    // 判断帧头 0x59 0x59
    if (tfmini_buf[0] == 0x59 && tfmini_buf[1] == 0x59) 
    {
        uint8_t sum = 0;
        // 计算前 8 个字节的累加和
        for (int i = 0; i < 8; i++) {
            sum += tfmini_buf[i];
        }
        
        // 校验和验证
        if (sum == tfmini_buf[8]) 
        {
            TFmini_Frame_t *pFrame = (TFmini_Frame_t *)tfmini_buf;
            
            // 拼接 16 位数据
            uint16_t dist = (uint16_t)(pFrame->dist_H << 8 | pFrame->dist_L);
            uint16_t strength = (uint16_t)(pFrame->strength_H << 8 | pFrame->strength_L);
            uint16_t temp = (uint16_t)(pFrame->temp_H << 8 | pFrame->temp_L);
            
            // 赋值信号强度和温度 (公式: Temp/8 - 256)
            tfmini_ctrl->strength = strength;
            tfmini_ctrl->temperature = temp / 8.0f - 256.0f;
            
            // 官方规定：当信号强度小于 100 或等于 65535(过曝) 时，数据不可信，输出 0 
            if (strength >= 100 && strength != 65535) {
                tfmini_ctrl->distance_m = dist / 100.0f; // 原始单位为cm，转换为米 
								tfmini_ctrl->distance_cm = dist;

            } else {
                tfmini_ctrl->distance_m = 0.0f;
							tfmini_ctrl->distance_cm = 0.0f;
            }
        }
    }

}

void USART_RxDMA_MultiBuffer_Init(UART_HandleTypeDef *huart, uint32_t *DstAddress, uint32_t *SecondMemAddress, uint32_t DataLength)
{
    huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
    huart->RxXferSize = DataLength * 2;
    SET_BIT(huart->Instance->CR3, USART_CR3_DMAR);
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    
    do {
        __HAL_DMA_DISABLE(huart->hdmarx);
    } while(((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->CR & DMA_SxCR_EN);
    
    ((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->PAR = (uint32_t)&huart->Instance->RDR;
    ((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->M0AR = (uint32_t)DstAddress;
    ((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->M1AR = (uint32_t)SecondMemAddress;
    ((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->NDTR = DataLength;
    SET_BIT(((DMA_Stream_TypeDef *)huart->hdmarx->Instance)->CR, DMA_SxCR_DBM);
    __HAL_DMA_ENABLE(huart->hdmarx);
}

void USER_USART2_RxHandler(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(((((DMA_Stream_TypeDef  *)huart->hdmarx->Instance)->CR) & DMA_SxCR_CT ) == RESET)//这一行代码判断的是DMA当前正在使用哪个内存地址
    {
        __HAL_DMA_DISABLE(huart->hdmarx);//失能dma准备下一步的寄存器操作
          
        /*CR是控制寄存器，这里对CR进行操作就是告诉单片机一些指令*/
        ((DMA_Stream_TypeDef  *)huart->hdmarx->Instance)->CR |= DMA_SxCR_CT;//告诉dma下一波数据搬到缓冲区2中
                    
        if(Size == TFMINI_RX_BUF_NUM)
        {
            //进行数据处理
            tfmini_to_data(tfmini_rx_buf[0], &tfmini_data);
        }
                  
        __HAL_DMA_SET_COUNTER(huart->hdmarx, TFMINI_RX_BUF_NUM * 2);//重新填满NDTR计数器
        __HAL_DMA_ENABLE(huart->hdmarx);
    }
    else
    { 
        /* Disable DMA */
        __HAL_DMA_DISABLE(huart->hdmarx);
                 
        /* Switch Memory 1 to Memory 0*/
        ((DMA_Stream_TypeDef  *)huart->hdmarx->Instance)->CR &= ~(DMA_SxCR_CT);//告诉dma下一波数据搬到缓冲区1中
                
        if(Size == TFMINI_RX_BUF_NUM)
        {
            tfmini_to_data(tfmini_rx_buf[1], &tfmini_data);
        }
        __HAL_DMA_SET_COUNTER(huart->hdmarx, TFMINI_RX_BUF_NUM * 2);
        __HAL_DMA_ENABLE(huart->hdmarx);
    }
}

void vofa( float data1,float data2,float data3,float data4)
{
    char data[16];
    memcpy((char*) data    ,&data1,4);
    memcpy((char*)(data+4 ),&data2,4);
    memcpy((char*)(data+8 ),&data3,4);
    memcpy((char*)(data+12),&data4,4);
    char tail[20]={data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],0x00,0x00,0x80,0x7f};
    HAL_UART_Transmit(&huart3,(uint8_t *)tail,20,0xffff);
}