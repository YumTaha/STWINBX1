/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "vib_io.h"
#include "vib_sensor.h"
#include "vib_comm.h"
#include "stdio.h"

/* ============================================================================
 * UTILITY/DEBUG FUNCTIONS
 * ============================================================================ */

/* Debug printf wrapper for ITM */
int _write(int file, char *ptr, int len){
  int i=0;
  for(i=0 ; i<len ; i++)
	  ITM_SendChar((*ptr++));
  return len;
}

/* ============================================================================
 * HIGH-LEVEL VIBRATION I/O INTERFACE
 * ============================================================================ */

int32_t vib_io_init(void) {
    return vib_sensor_init();
}

void vib_io_process(void) {
    uint32_t num_samples = 0;
    const iis3dwb_fifo_out_raw_t *buffer = vib_sensor_read_fifo_data(&num_samples);
    
    if (buffer != NULL && num_samples > 0) {
        // Buffer is ready for transmission
        if (!vib_comm_is_busy()) {
            vib_comm_status_t status = vib_comm_send_buffer_uart_dma(buffer, num_samples);
            if (status == VIB_COMM_OK) {
                // Transmission started successfully, reset sensor buffer
                vib_sensor_reset_buffer();
            }
            // Note: If transmission fails or is busy, we'll try again next time
        }
    }
}

void vib_io_uart_tx_complete_callback(UART_HandleTypeDef *huart) {
    vib_comm_uart_dma_tx_complete_callback(huart);
}
