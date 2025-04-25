/*******************************************************************************
**** 文件路径         : \GD32_FreeRTOS_libmodbus\App\src\app_task.c
**** 作者名称         : Amosico
**** 文件版本         : V1.0.0
**** 创建日期         : 2025-04-09 16:10:50
**** 简要说明         :
**** 版权信息         :
********************************************************************************/

/* System Include */
#include <stdio.h>

/* GD32 Include */
#include "gd32h7xx_gpio.h"

/* FreeRTOS Include */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* libmodbus Include */
#include "modbus.h"

#include "usart.h"

/* FreeRTOS User Part*/
static TaskHandle_t Modbus_Master_Tsk_Handle = NULL;
static TaskHandle_t Modbus_Slave_Tsk_Handle = NULL;
static TaskHandle_t LED_Tsk_Handle = NULL;
QueueHandle_t Modbus_Recv_Buf = NULL;

static void Modbus_Master_Tsk(void* para) {
    uint8_t coil_reg[64] = {0};     // 0x01 Reg
    uint16_t tab_reg_r[64] = {0};   // 0x03 Reg
    // uint16_t tab_reg_w[64] = {10, 20, 30, 40, 50};   // 0x10 Reg
    modbus_t* ctx = modbus_new_rtu("COM1", 115200, 'N', 8, 1);
    modbus_set_debug(ctx, 1);
    modbus_set_slave(ctx, 1);   // slave addr = 1
    modbus_set_byte_timeout(ctx, 0, 500000);
    modbus_set_response_timeout(ctx, 0, 500000);
    modbus_connect(ctx);        // init serial here
    modbus_flush(ctx);

    for(;;) {
        /* 0x01 */ 
        // int rc = modbus_read_bits(ctx, 0, 10, coil_reg);
        // if(rc == -1) {
        //     printf("read error!\n");
        // }
        // for(int i = 0; i < rc; i++) {
        //     printf("reg[%d]=%d(0x%X)\n", i, coil_reg[i], coil_reg[i]);
        // }
        
        /* 0x03 */
        int rc = modbus_read_registers(ctx, 0, 3, tab_reg_r);
        if(rc == -1) {
            printf("read error!\n");
        }else{
            for(int i = 0; i < rc; i++) {
                printf("reg[%d]=%d(0x%X)\n", i, tab_reg_r[i], tab_reg_r[i]);
            }
        }

        /* 0x10 */
        // int rc = modbus_write_registers(ctx, 0, 10, tab_reg_w);
        // if(rc == -1) {
        //     printf("write error!\n");
        // }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void Modbus_Slave_Tsk(void* para) {
    modbus_t* ctx = NULL;
    uint8_t req[256] = {0};
    int rc;
    modbus_mapping_t* modbus_map;
    modbus_map = modbus_mapping_new(0, 0, 10, 0);
    modbus_map->tab_registers[0] = 10;
    modbus_map->tab_registers[1] = 20;

    ctx = modbus_new_rtu("COM1", 115200, 'N', 8, 1);
    modbus_set_slave(ctx, 1);   // slave addr = 1
    modbus_set_debug(ctx, 1);
    modbus_set_byte_timeout(ctx, 0, 100000);
    modbus_set_response_timeout(ctx, 0, 100000);
    modbus_connect(ctx);        // init serial here
    modbus_flush(ctx);
    for(;;) {
        rc = modbus_receive(ctx, req);
        if(rc > 0) {
            modbus_reply(ctx, req, rc, modbus_map);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void Led_Tsk(void* para) {
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_bit_reset(GPIOA, GPIO_PIN_6);
    for(;;) {
        gpio_bit_toggle(GPIOA, GPIO_PIN_6);
        //printf("LED TSK!\n");
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

uint8_t FreeRTOS_Task_Create_And_Start(void) {
    Modbus_Recv_Buf = xQueueCreate(256, 1);
    if(!Modbus_Recv_Buf) {
        return -1;
    }

    // uint8_t xReturn = xTaskCreate((TaskFunction_t)Modbus_Master_Tsk,
    //                               (const char*)"modbus_master",
    //                               (uint16_t)256,
    //                               (void*)NULL,
    //                               (UBaseType_t)1,
    //                               (TaskHandle_t*)&Modbus_Master_Tsk_Handle);

    // if(pdTRUE != xReturn)
    //     return -1;

    uint8_t xReturn = xTaskCreate((TaskFunction_t)Modbus_Slave_Tsk,
                                  (const char*)"modbus_slave",
                                  (uint16_t)256,
                                  (void*)NULL,
                                  (UBaseType_t)1,
                                  (TaskHandle_t*)&Modbus_Slave_Tsk_Handle);

    if(pdTRUE != xReturn)
        return -1;

    xReturn = xTaskCreate((TaskFunction_t)Led_Tsk,
                          (const char*)"Running",
                          (uint16_t)128,
                          (void*)NULL,
                          (UBaseType_t)1,
                          (TaskHandle_t*)&LED_Tsk_Handle);

    if(pdTRUE == xReturn)
        vTaskStartScheduler();
    return xReturn;
}
