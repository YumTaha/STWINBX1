#ifndef VIB_IO_H
#define VIB_IO_H

#include "vib_sensor.h"
#include "vib_comm.h"
#include "stm32u5xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * HIGH-LEVEL VIBRATION I/O INTERFACE
 * ============================================================================ */

/* Initialize the complete vibration I/O system (sensor + communication)
 * Returns: 0 on success, negative on error */
int32_t vib_io_init(void);

/* Main vibration data processing function
 * Should be called periodically from main loop or interrupt handler
 * Handles sensor data acquisition and automatic transmission */
void vib_io_process(void);

/* UART DMA transmission complete callback
 * Should be called from HAL_UART_TxCpltCallback in main.c */
void vib_io_uart_tx_complete_callback(UART_HandleTypeDef *huart);

/* ============================================================================
 * UTILITY/DEBUG FUNCTIONS
 * ============================================================================ */

/* Debug printf wrapper for ITM (if needed) */
int _write(int file, char *ptr, int len);

#ifdef __cplusplus
}
#endif

#endif /* VIB_IO_H */
