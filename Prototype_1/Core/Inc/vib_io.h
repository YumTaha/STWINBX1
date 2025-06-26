#ifndef VIB_IO_H
#define VIB_IO_H

#include "iis3dwb_reg.h"
#include "stm32u5xx_hal.h"


typedef struct {
    uint8_t *data_ptr;
    uint32_t bytes_remaining;
    uint8_t busy;
} uart_dma_transfer_t;


/* call once at startup. returns 0 on success */
int32_t  vib_io_init(void);

void vib_read(void);

void get_fifo_status(void);

void send_buffer_UART_DMA_nb(const iis3dwb_fifo_out_raw_t *buffer, uint32_t num_samples);

void vib_uart_dma_tx_cplt_cb(UART_HandleTypeDef *huart);

#endif /* VIB_IO_H */
