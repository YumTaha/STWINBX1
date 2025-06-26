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
#define VIB_SAMPLE_BUFFER_SIZE  40000 // 1.5 seconds of storage
#define VIB_SAMPLES_PER_SEC 26667 // 1 seconds of data (26.7kHz)

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */
typedef struct {
    iis3dwb_fifo_out_raw_t *buffer;
    iis3dwb_fifo_out_raw_t *current_ptr;
    uint32_t level;
    uint32_t max_size;
} vib_buffer_t;

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

/* Read data from vibration sensor FIFO and manage buffer
 * Returns: pointer to buffer and number of samples if buffer is ready for processing,
 *          NULL if buffer is not ready */
const iis3dwb_fifo_out_raw_t* vib_sensor_read_fifo_data(uint32_t *num_samples);

/* Reset the sensor data buffer to empty state */
void vib_sensor_reset_buffer(void);

/* Get current buffer level */
uint32_t vib_sensor_get_buffer_level(void);

#ifdef __cplusplus
}
#endif

#endif /* VIB_SENSOR_H */
