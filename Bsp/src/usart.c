/*******************************************************************************
**** 文件路径         : \GD32_FreeRTOS_libmodbus\Bsp\src\usart.c
**** 作者名称         : Amosico
**** 文件版本         : V1.0.0
**** 创建日期         : 2025-04-09 16:13:49
**** 简要说明         :
**** 版权信息         :
********************************************************************************/
#include "usart.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* MODBUS USART */
#define MODBUS_USART_GPIO_CLK RCU_GPIOA
#define MODBUS_USART_CLK      RCU_USART0
#define MODBUS_USART          USART0
#define MODBUS_USART_GPIO_AF  GPIO_AF_7
// Tx
#define MODBUS_USART_TX_PORT        GPIOA
#define MODBUS_USART_TX_PIN         GPIO_PIN_9
#define MODBUS_USART_TDATA_REG_ADDR (uint32_t)(&USART_TDATA(MODBUS_USART))
// RX
#define MODBUS_USART_RX_PORT        GPIOA
#define MODBUS_USART_RX_PIN         GPIO_PIN_10
#define MODBUS_USART_RDATA_REG_ADDR (uint32_t)(&USART_RDATA(MODBUS_USART))
/* DMA */
// Tx
#define MODBUS_USART_TX_DMA_CLK     RCU_DMA0
#define MODBUS_USART_TX_DMA         DMA0
#define MODBUS_USART_TX_DMA_CHN     DMA_CH0
#define MODBUS_USART_TX_DMA_REQUEST DMA_REQUEST_USART0_TX
// Rx
#define MODBUS_USART_RX_DMA_CLK     RCU_DMA0
#define MODBUS_USART_RX_DMA         DMA0
#define MODBUS_USART_RX_DMA_CHN     DMA_CH1
#define MODBUS_USART_RX_DMA_REQUEST DMA_REQUEST_USART0_RX

#define MODBUS_USART_START
#define MODBUS_USART_END

/* DEBUG USART */
#define DEBUG_USART_GPIO_CLK RCU_GPIOA
#define DEBUG_USART_CLK      RCU_USART1
#define DEBUG_USART          USART1
#define DEBUG_USART_GPIO_AF  GPIO_AF_7
// Tx
#define DEBUG_USART_TX_PORT GPIOA
#define DEBUG_USART_TX_PIN  GPIO_PIN_2
// RX
#define DEBUG_USART_RX_PORT GPIOA
#define DEBUG_USART_RX_PIN  GPIO_PIN_3

#define DEBUG_USART_START
#define DEBUG_USART_END


MODBUS_USART_START
static void modbus_usart_tx_dma_init(void) {
    rcu_periph_clock_enable(MODBUS_USART_TX_DMA_CLK);
    rcu_periph_clock_enable(RCU_DMAMUX);
    dma_single_data_parameter_struct dma_s;
    dma_deinit(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN);
    dma_s.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_s.direction = DMA_MEMORY_TO_PERIPH;
    dma_s.memory0_addr = 0;
    dma_s.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_s.number = 0;
    dma_s.periph_addr = MODBUS_USART_TDATA_REG_ADDR;
    dma_s.periph_inc = DMA_MEMORY_INCREASE_DISABLE;
    dma_s.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_s.priority = DMA_PRIORITY_HIGH;
    dma_s.request = MODBUS_USART_TX_DMA_REQUEST;
    dma_single_data_mode_init(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN, &dma_s);
}

static void modbus_usart_rx_dma_init(void) {
    dma_single_data_parameter_struct dma_s;
    dma_deinit(MODBUS_USART_RX_DMA, MODBUS_USART_RX_DMA_CHN);
    dma_s.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_s.direction = DMA_PERIPH_TO_MEMORY;
    dma_s.memory0_addr = 0;
    dma_s.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_s.number = 0;
    dma_s.periph_addr = MODBUS_USART_RDATA_REG_ADDR;
    dma_s.periph_inc = DMA_MEMORY_INCREASE_DISABLE;
    dma_s.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_s.priority = DMA_PRIORITY_HIGH;
    dma_s.request = MODBUS_USART_RX_DMA_REQUEST;
    dma_single_data_mode_init(MODBUS_USART_RX_DMA, MODBUS_USART_RX_DMA_CHN, &dma_s);
}

/*******************************************************************************
**** 函数功能: Modbus串口初始化
**** 入口参数: ctx_rtu: libmodbus控制结构体
**** 返回值: NULL
**** 函数备注:
********************************************************************************/
void modbus_usart_init(modbus_rtu_t* ctx_rtu) {
    /* USART Clk */
    rcu_periph_clock_enable(MODBUS_USART_GPIO_CLK);
    rcu_periph_clock_enable(MODBUS_USART_CLK);
    /* USART GPIO */
    gpio_af_set(MODBUS_USART_TX_PORT, MODBUS_USART_GPIO_AF, MODBUS_USART_TX_PIN);
    gpio_af_set(MODBUS_USART_RX_PORT, MODBUS_USART_GPIO_AF, MODBUS_USART_RX_PIN);
    gpio_mode_set(MODBUS_USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, MODBUS_USART_TX_PIN);
    gpio_output_options_set(
        MODBUS_USART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, MODBUS_USART_TX_PIN);
    gpio_mode_set(MODBUS_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, MODBUS_USART_RX_PIN);
    gpio_output_options_set(
        MODBUS_USART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, MODBUS_USART_RX_PIN);
    /* USART Setting */
    usart_deinit(MODBUS_USART);
    usart_baudrate_set(MODBUS_USART, ctx_rtu->baud);
    /* 停止位暂时只用1bit or 2bit */
    switch(ctx_rtu->stop_bit) {
        case 1: {
            usart_stop_bit_set(MODBUS_USART, USART_STB_1BIT);
            break;
        }
        case 2: {
            usart_stop_bit_set(MODBUS_USART, USART_STB_2BIT);
            break;
        }
        default: {
            usart_stop_bit_set(MODBUS_USART, USART_STB_1BIT);
            break;
        }
    }

    switch(ctx_rtu->data_bit) {
        case 7: {
            usart_word_length_set(MODBUS_USART, USART_WL_7BIT);
            break;
        }
        case 8: {
            usart_word_length_set(MODBUS_USART, USART_WL_8BIT);
            break;
        }
        case 9: {
            usart_word_length_set(MODBUS_USART, USART_WL_9BIT);
            break;
        }
        case 10: {
            usart_word_length_set(MODBUS_USART, USART_WL_10BIT);
            break;
        }
        default: {
            usart_word_length_set(MODBUS_USART, USART_WL_8BIT);
            break;
        }
    }

    switch(ctx_rtu->parity) {
        case 'N': {
            usart_parity_config(MODBUS_USART, USART_PM_NONE);
            break;
        }
        case 'O': {
            usart_parity_config(MODBUS_USART, USART_PM_ODD);
            break;
        }
        case 'E': {
            usart_parity_config(MODBUS_USART, USART_PM_EVEN);
            break;
        }
        default: {
            usart_parity_config(MODBUS_USART, USART_PM_NONE);
            break;
        }
    }
    /* 开启USART-DMA */
    modbus_usart_tx_dma_init();
    usart_dma_transmit_config(MODBUS_USART, USART_TRANSMIT_DMA_ENABLE);
    // modbus_usart_rx_dma_init();
    // usart_dma_receive_config(MODBUS_USART, USART_RECEIVE_DMA_ENABLE);
    /* 串口发送器,接收器使能 */
    usart_transmit_config(MODBUS_USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(MODBUS_USART, USART_RECEIVE_ENABLE);
}

void modbus_usart_enable(void) {
    usart_interrupt_enable(MODBUS_USART, USART_INT_RBNE);
    usart_enable(MODBUS_USART);
}

void modbus_usart_disable(void) {
    usart_interrupt_disable(MODBUS_USART, USART_INT_RBNE);
    usart_disable(MODBUS_USART);
}


/*******************************************************************************
**** 函数功能: 串口DMA发送
**** 入口参数: req: 发送缓冲区
**** 入口参数: req_length: 发送长度
**** 返回值: 发送成功长度
**** 函数备注:
********************************************************************************/
uint32_t modbus_usart_dma_transmit(const uint8_t* req, int req_length) {

    dma_memory_address_config(
        MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN, DMA_MEMORY_0, (uint32_t)req);
    dma_transfer_number_config(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN, req_length);
    uint8_t timeout = 3;
    dma_channel_enable(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN);
    while(dma_flag_get(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN, DMA_FLAG_FTF) == RESET) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout--;
        if(timeout == 0) {
            return -1;
        }
    }
    dma_channel_disable(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN);
    dma_flag_clear(MODBUS_USART_TX_DMA, MODBUS_USART_TX_DMA_CHN, DMA_FLAG_FTF);
    return req_length;
}

// USART IRQN
extern QueueHandle_t Modbus_Recv_Buf;
uint8_t data_t = 0;
void USART0_IRQHandler(void) {
    uint8_t data = 0;
    if(usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        data = usart_data_receive(MODBUS_USART);
        xQueueSendFromISR(Modbus_Recv_Buf, &data, NULL);
    }
}

MODBUS_USART_END


DEBUG_USART_START

void debug_usart_init(int baud) {
    /* USART Clk */
    rcu_periph_clock_enable(DEBUG_USART_GPIO_CLK);
    rcu_periph_clock_enable(DEBUG_USART_CLK);
    /* USART GPIO */
    gpio_af_set(DEBUG_USART_TX_PORT, DEBUG_USART_GPIO_AF, DEBUG_USART_TX_PIN);

    gpio_mode_set(DEBUG_USART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_USART_TX_PIN);
    gpio_output_options_set(
        DEBUG_USART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, DEBUG_USART_TX_PIN);
    gpio_af_set(DEBUG_USART_RX_PORT, DEBUG_USART_GPIO_AF, DEBUG_USART_RX_PIN);
    gpio_mode_set(DEBUG_USART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_USART_RX_PIN);
    gpio_output_options_set(
        DEBUG_USART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, DEBUG_USART_RX_PIN);
    /* USART Setting */
    usart_deinit(DEBUG_USART);
    usart_baudrate_set(DEBUG_USART, baud);
    usart_stop_bit_set(DEBUG_USART, USART_STB_1BIT);
    usart_word_length_set(DEBUG_USART, USART_WL_8BIT);
    usart_parity_config(DEBUG_USART, USART_PM_NONE);
    usart_transmit_config(DEBUG_USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(DEBUG_USART, USART_RECEIVE_ENABLE);
    usart_enable(DEBUG_USART);
}


DEBUG_USART_END