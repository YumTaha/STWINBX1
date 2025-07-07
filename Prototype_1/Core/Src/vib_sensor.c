/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "vib_sensor.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin
#include <string.h>

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */
/* Sensor configuration and context */
static stmdev_ctx_t dev_ctx;
static iis3dwb_fifo_status_t fifo_status;

/* Double buffer management */
static iis3dwb_fifo_out_raw_t buffer_A[VIB_HALF_SEC_SAMPLES];
static iis3dwb_fifo_out_raw_t buffer_B[VIB_HALF_SEC_SAMPLES];

static vib_double_buffer_t double_buffer = {
    .buffer_A = buffer_A,
    .buffer_B = buffer_B,
    .active_buffer = buffer_A,
    .dma_buffer = buffer_B,
    .active_index = 0,
    .buffer_size = VIB_HALF_SEC_SAMPLES,
    .dma_in_progress = 0
};

/* ============================================================================
 * STATIC FUNCTION PROTOTYPES
 * ============================================================================ */
static int32_t drv_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static int32_t drv_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);

/* ============================================================================
 * LOW-LEVEL HARDWARE INTERFACE (SPI/COMMUNICATION)
 * ============================================================================ */

/*  SPI write wrapper: assert CS, send reg & data, de-assert CS */
static int32_t drv_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_RESET);
  BSP_SPI2_Send(&reg, 1);
  BSP_SPI2_Send(bufp, len);
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
  return 0;
}

/*  SPI read wrapper: assert CS, send reg|0x80, read data, de-assert CS */
static int32_t drv_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_RESET);
  BSP_SPI2_Send(&reg, 1);
  BSP_SPI2_Recv(bufp, len);
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
  return 0;
}

/* ============================================================================
 * SENSOR INITIALIZATION & CONFIGURATION
 * ============================================================================ */

int32_t vib_sensor_init(void) {
  uint8_t whoamI, rst;

  /* 1) bring up the SPI bus */
  if (BSP_SPI2_Init() != BSP_ERROR_NONE)		return -1;

  /* 2) hook our wrappers into the ST driver */
  dev_ctx.write_reg = drv_write;
  dev_ctx.read_reg  = drv_read;
  dev_ctx.handle    = &hspi2;
  dev_ctx.mdelay    = HAL_Delay;

  /* 3) check device ID */
  iis3dwb_device_id_get(&dev_ctx, &whoamI);
  if (whoamI != IIS3DWB_ID) 					return -2;		// WHO_AM_I_EXPECTED 0x7B

  /* 4) reset sensor */
  iis3dwb_reset_set(&dev_ctx, PROPERTY_ENABLE);
  do {
    iis3dwb_reset_get(&dev_ctx, &rst);
  } while (rst);

  /* 5) basic configuration/fifo */
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE); /* Enable Block Data Update */ // Important if you are reading acceleration
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g); /* Set full scale */
  iis3dwb_i2c_interface_set(&dev_ctx, PROPERTY_ENABLE); /* Disable I2C */ // Not important
  iis3dwb_fifo_watermark_set(&dev_ctx, VIB_FIFO_WATERMARK); // 1. Set FIFO threshold (samples)
  iis3dwb_fifo_xl_batch_set(&dev_ctx, IIS3DWB_XL_BATCHED_AT_26k7Hz);
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // 2. Set FIFO mode to stop collecting data when FIFO is full

  /* 6) enable FIFO threshold interrupt on INT1 */
  iis3dwb_pin_int1_route_t int1_route = {0};
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);

  /* Set Output Data Rate */
  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);

  return 0;
}

/* ============================================================================
 * DATA ACQUISITION & FIFO MANAGEMENT
 * ============================================================================ */

void vib_sensor_get_fifo_status(void){
	iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);
}

const iis3dwb_fifo_out_raw_t* vib_sensor_read_fifo_data_double_buffer(uint32_t *num_samples) {
    uint16_t samples_to_read = 0;

    /* Initialize output parameters */
    if (num_samples) *num_samples = 0;

    // 1. Get current FIFO status from the sensor
    if (iis3dwb_fifo_status_get(&dev_ctx, &fifo_status) != 0) {
    	Error_Handler();
        return NULL;
    }

    // 2. Check if FIFO threshold interrupt flag is set
    if (fifo_status.fifo_th) {
        samples_to_read = fifo_status.fifo_level;
        
        // 2a. Ensure not to overflow the active buffer
        if ((double_buffer.active_index + samples_to_read) > double_buffer.buffer_size) {
            samples_to_read = double_buffer.buffer_size - double_buffer.active_index;
        }

        // 2b. Read samples into active buffer at current position
        if (iis3dwb_fifo_out_multi_raw_get(&dev_ctx, 
                                          &double_buffer.active_buffer[double_buffer.active_index], 
                                          samples_to_read) == 0) {
            // Update buffer state
            double_buffer.active_index += samples_to_read;
        } else {
        	Error_Handler();
            return NULL;
        }

        // 3. Check if active buffer is full and DMA is not busy
        if (double_buffer.active_index == double_buffer.buffer_size && !double_buffer.dma_in_progress) {
            // Swap buffers - active becomes DMA buffer, DMA buffer becomes active
            iis3dwb_fifo_out_raw_t *tmp = double_buffer.dma_buffer;
            double_buffer.dma_buffer = double_buffer.active_buffer;
            double_buffer.active_buffer = tmp;
            
            // Reset active buffer index
            double_buffer.active_index = 0;
            
            // Mark DMA as in progress
            double_buffer.dma_in_progress = 1;
            
            // Signal LED that buffer is ready
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            
            // Return full buffer for DMA transmission
            if (num_samples) *num_samples = double_buffer.buffer_size;
            return double_buffer.dma_buffer;
        }
    }

    return NULL; // No buffer ready for transmission
}

void vib_sensor_dma_transmission_complete(void) {
    double_buffer.dma_in_progress = 0; // Allow next buffer swap
}

uint32_t vib_sensor_get_active_buffer_level(void) {
    return double_buffer.active_index;
}
