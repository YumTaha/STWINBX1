#ifndef VIB_SENSOR_H
#define VIB_SENSOR_H

#include "iis3dwb_reg.h"
#include "stm32u5xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DEFINES & MACROS
 * ============================================================================ */
#define VIB_FIFO_WATERMARK    500
#define VIB_SAMPLES_PER_SEC 26667 // 1 seconds of data (26.7kHz)
#define VIB_HALF_SEC_SAMPLES (VIB_SAMPLES_PER_SEC / 2) // Half second buffer for double buffering
#define VIB_SAMPLE_BUFFER_SIZE  VIB_SAMPLES_PER_SEC // Total storage (for backward compatibility)

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */
typedef struct {
    iis3dwb_fifo_out_raw_t *buffer_A;
    iis3dwb_fifo_out_raw_t *buffer_B;
    iis3dwb_fifo_out_raw_t *active_buffer;    // Currently being filled
    iis3dwb_fifo_out_raw_t *dma_buffer;       // Currently being transmitted
    uint32_t active_index;                    // Current position in active buffer
    uint32_t buffer_size;                     // Size of each half-buffer
    volatile uint8_t dma_in_progress;         // Flag indicating DMA transmission active
} vib_double_buffer_t;

/* ============================================================================
 * SENSOR INITIALIZATION & CONFIGURATION
 * ============================================================================ */
/* Initialize vibration sensor and SPI communication
 * Returns: 0 on success, negative on error */
int32_t vib_sensor_init(void);

/* ============================================================================
 * DATA ACQUISITION & FIFO MANAGEMENT
 * ============================================================================ */
/* Get current FIFO status from vibration sensor */
void vib_sensor_get_fifo_status(void);

/* Read data from vibration sensor FIFO using double buffering
 * Returns: pointer to full buffer ready for transmission and number of samples,
 *          NULL if no buffer is ready for transmission */
const iis3dwb_fifo_out_raw_t* vib_sensor_read_fifo_data_double_buffer(uint32_t *num_samples);

/* Notify sensor module that DMA transmission is complete
 * Should be called from communication module when transmission finishes */
void vib_sensor_dma_transmission_complete(void);

/* Get current active buffer fill level (for debugging/monitoring) */
uint32_t vib_sensor_get_active_buffer_level(void);

#ifdef __cplusplus
}
#endif

#endif /* VIB_SENSOR_H */
