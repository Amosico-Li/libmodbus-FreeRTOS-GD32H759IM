/*******************************************************************************
**** 文件路径         : \GD32_FreeRTOS_libmodbus\Bsp\inc\usart.h
**** 作者名称         : Amosico
**** 文件版本         : V1.0.0
**** 创建日期         : 2025-04-09 16:13:56
**** 简要说明         :
**** 版权信息         :
********************************************************************************/

#ifndef USART_H
#define USART_H

#include "gd32h7xx_gpio.h"
#include "gd32h7xx_usart.h"
#include "modbus-rtu-private.h"

void modbus_usart_init(modbus_rtu_t* ctx_rtu);
void modbus_usart_enable(void);
void modbus_usart_disable(void);
uint32_t modbus_usart_dma_transmit(const uint8_t* req, int req_length);

void debug_usart_init(int baud);
#endif