## 详细移植过程请见 CSDN：Amosico
## 移植主要包括如下操作：
# modbus-rtu.c文件更改
  1、modbus_new_rtu 更改：将函数中 malloc、free函数全部替换成 FreeRTOS 的 heap4.c 的内存申请和释放函数。并且增加设备初始化（串口）。
  2、_modbus_rtu_connect 更改，串口的中断使能和使能，这里需要把 ctx 的 s 设置成 1，因为在 _modbus_recv_msg 中会检测 s 是否 >= 0，如果不设置成 1，会导致接收发生异常，提前被return -1。
  3、_modbus_rtu_close 更改，串口中断失能和失能。
  4、_modbus_rtu_send 更改，串口 DMA 发送。
  5、_modbus_rtu_recv 更改，配合串口接受中断及 FreeRTOS 队列。
  6、_modbus_rtu_flush 更改，读空队列即可。
# modbus.c文件更改
  将所有 malloc、free 改成 FreeRTOS 的内存申请和释放。并且将有关 Win/Linux 部分全部删除。
  1、sleep_response_timeout 更改为 FreeRTOS 延时，延时时间采用 ctx 结构体内部设定响应超时时间，以 ms 为单位。
## 主从任务书写
// Master任务
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

// Slave任务
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
