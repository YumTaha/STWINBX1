/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "vib_comm.h"
#include "main.h"      // for LED GPIO definitions

/* ============================================================================
 * EXTERNAL VARIABLES
 * ============================================================================ */
extern UART_HandleTypeDef huart2;

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */
/* UART transmission state */
static vib_uart_dma_transfer_t uart_dma_tx = {0};

/* ============================================================================
 * DATA TRANSMISSION (UART/DMA)
 * ============================================================================ */

vib_comm_status_t vib_comm_send_buffer_uart_dma(const iis3dwb_fifo_out_raw_t *buffer, uint32_t num_samples)
{
    if (uart_dma_tx.busy) return VIB_COMM_BUSY; // Already in progress

    uart_dma_tx.data_ptr = (uint8_t*)buffer;
    uart_dma_tx.bytes_remaining = num_samples * sizeof(iis3dwb_fifo_out_raw_t);
    uart_dma_tx.busy = 1;

    uint16_t chunk = (uart_dma_tx.bytes_remaining > 0xFFFF) ? 0xFFFF : uart_dma_tx.bytes_remaining;

    if (HAL_UART_Transmit_DMA(&huart2, uart_dma_tx.data_ptr, chunk) != HAL_OK) {
        uart_dma_tx.busy = 0;
        return VIB_COMM_ERROR;
    }
    uart_dma_tx.bytes_remaining -= chunk;
    uart_dma_tx.data_ptr += chunk;
    
    return VIB_COMM_OK;
}

void vib_comm_uart_dma_tx_complete_callback(UART_HandleTypeDef *huart)
{
    if (huart != &huart2) return;
    
    if (uart_dma_tx.bytes_remaining > 0) {
        uint16_t chunk = (uart_dma_tx.bytes_remaining > 0xFFFF) ? 0xFFFF : uart_dma_tx.bytes_remaining;
        if (HAL_UART_Transmit_DMA(&huart2, uart_dma_tx.data_ptr, chunk) != HAL_OK) {
            uart_dma_tx.busy = 0;
            Error_Handler();
        }
        uart_dma_tx.bytes_remaining -= chunk;
        uart_dma_tx.data_ptr += chunk;
    } else {
        uart_dma_tx.busy = 0; // Transfer finished
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }
}

uint8_t vib_comm_is_busy(void) {
    return uart_dma_tx.busy;
}

uint32_t vib_comm_get_bytes_remaining(void) {
    return uart_dma_tx.bytes_remaining;
}
