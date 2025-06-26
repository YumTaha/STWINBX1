#ifndef VIB_COMM_H
#define VIB_COMM_H

#include "iis3dwb_reg.h"
#include "stm32u5xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */
typedef struct {
    uint8_t *data_ptr;
    uint32_t bytes_remaining;
    uint8_t busy;
} vib_uart_dma_transfer_t;

typedef enum {
    VIB_COMM_OK = 0,
    VIB_COMM_BUSY = -1,
    VIB_COMM_ERROR = -2
} vib_comm_status_t;

/* ============================================================================
 * DATA TRANSMISSION (UART/DMA)
 * ============================================================================ */
/* Send vibration data buffer via UART DMA (non-blocking)
 * Parameters:
 *   buffer: pointer to vibration data samples
 *   num_samples: number of samples to transmit
 * Returns: VIB_COMM_OK on success, VIB_COMM_BUSY if already transmitting, VIB_COMM_ERROR on error */
vib_comm_status_t vib_comm_send_buffer_uart_dma(const iis3dwb_fifo_out_raw_t *buffer, uint32_t num_samples);

/* UART DMA transmission complete callback for vibration data
 * Should be called from HAL_UART_TxCpltCallback */
void vib_comm_uart_dma_tx_complete_callback(UART_HandleTypeDef *huart);

/* Check if UART DMA transmission is in progress
 * Returns: 1 if busy, 0 if idle */
uint8_t vib_comm_is_busy(void);

/* Get current transmission progress (for debugging/monitoring)
 * Returns: bytes remaining to transmit */
uint32_t vib_comm_get_bytes_remaining(void);

#ifdef __cplusplus
}
#endif

#endif /* VIB_COMM_H */
