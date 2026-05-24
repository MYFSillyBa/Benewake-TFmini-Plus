#本文件使用gemini3.1pro生成并做了一些适应性修改
# Benewake TFmini Plus STM32 驱动代码

本项目提供了北醒 (Benewake) TFmini Plus 激光雷达在 STM32 平台上的驱动源码与应用示例。适用于底盘避障、云台测距辅助、机器人定高控制等场景。

## 🛠️ 硬件与开发环境

*   **微控制器**: STM32H723VGT6
*   **开发环境**: Keil MDK v5
*   **通信接口**: UART

## 🔌 引脚接线说明 (UART 模式)

TFmini Plus 传感器与 STM32 的硬件连接如下：

| TFmini Plus 线序颜色 | 引脚功能 | 连接到 STM32 | 备注 |
| :--- | :--- | :--- | :--- |
| **红线** | 5V | 5V | 必须保证供电充足 (峰值电流较大) |
| **黑线** | GND | GND | 共地 |
| **绿线** | TX | RX [例如：PA10 (USART1_RX)] | 传感器发送，单片机接收 |
| **白线** | RX | TX [例如：PA9 (USART1_TX)] | 单片机发送，传感器接收 (如需配参) |

> **注意**：TFmini Plus 默认波特率为 `115200`，在本项目中使用的是921600，请验证后使用，数据位 8，停止位 1，无奇偶校验。

## 🚀 核心代码说明

**数据帧解析**：严格按照北醒官方的 `59 59 Dist_L Dist_H Strength_L Strength_H...` 协议进行校验和解析。

### 快速调用示例

将本仓库的 `.c` 和 `.h` 文件加入到你的 Keil 工程中后，可以这样获取距离：

```c
#include "tfmini_plus.h"

// 假设在你的主循环或者状态机中调用，本项目写在了此处，可自行做修改
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart,uint16_t Size)
{
	if(huart->Instance == USART2)
    {
			USER_USART2_RxHandler(huart,Size);
    }
		   huart->ReceptionType = HAL_UART_RECEPTION_TOIDLE;
	
  /* Enalbe IDLE interrupt */
   __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
	
  /* Enable the DMA transfer for the receiver request */
   SET_BIT(huart->Instance->CR3, USART_CR3_DMAR);
	
  /* Enable DMA */
   __HAL_DMA_ENABLE(huart->hdmarx);
}
