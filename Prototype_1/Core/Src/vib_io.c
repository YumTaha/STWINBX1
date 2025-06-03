#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin
//#include "cmsis_os.h"       // For FreeRTOS semaphore
#include <string.h>
//#include "app_freertos.h"

//#define FIFO_DMA_BUF_SIZE  (FIFO_THRESHOLD * 7)
//uint8_t fifo_dma_buffer[FIFO_DMA_BUF_SIZE];

static stmdev_ctx_t dev_ctx;


/*  SPI write wrapper: assert CS, send reg & data, de-assert CS */
static int32_t  drv_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_RESET);
  BSP_SPI2_Send(&reg, 1);
  BSP_SPI2_Send(bufp, len);
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
  return 0;
}

/*  SPI read wrapper: assert CS, send reg|0x80, read data, de-assert CS */
static int32_t  drv_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_RESET);
  BSP_SPI2_Send(&reg, 1);
  BSP_SPI2_Recv(bufp, len);
  HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
  return 0;
}

/* Provide Access via a Getter Function */
stmdev_ctx_t* vib_io_get_ctx(void)		{return &dev_ctx;}

/**
 * @brief  Initialize the IIS3DWB vibration sensor and configure it for data acquisition.
 *
 * This function performs the following:
 *  - Initializes the SPI interface for communication with the IIS3DWB sensor.
 *  - Sets up the STM32 sensor driver context with board-specific read/write functions.
 *  - Verifies the sensor is connected by reading the WHO_AM_I register.
 *  - Resets the IIS3DWB sensor to default configuration.
 *  - Configures the sensor for 3-axis measurement at 26.7 kHz, 2g full scale, with block data update.
 *  - Sets the output data filter path to 6.3 kHz bandwidth.
 *  - Enables the data-ready interrupt on INT1.
 *
 * @retval 0   Initialization and configuration successful.
 * @retval -1  SPI interface initialization failed.
 * @retval -2  IIS3DWB sensor not found (WHO_AM_I mismatch).
 */
int32_t vib_io_init(void)
{
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

  /* 5) basic configuration */
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g);
  iis3dwb_xl_filt_path_on_out_set(&dev_ctx, IIS3DWB_LP_6k3Hz);

  /* 6) initialize fifo */
  iis3dwb_fifo_watermark_set(&dev_ctx, 256); // 1. Set FIFO threshold (samples)
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // 2. Set FIFO mode (stream FIFO, overwrites oldest)
  iis3dwb_fifo_xl_batch_set(&dev_ctx, IIS3DWB_XL_BATCHED_AT_26k7Hz);
  iis3dwb_fifo_stop_on_wtm_set(&dev_ctx, 1);

  /* 7) enable FIFO threshold interrupt on INT1 */
  iis3dwb_pin_int1_route_t int1_route = {0};
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);

  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);
  return 0;
}



void vib_fifo_read_all_simple(void)
{
    uint16_t num_samples = 0;
    iis3dwb_fifo_out_raw_t fifo_buf[FIFO_THRESHOLD];

    // 1. How many samples in FIFO?
    if (iis3dwb_fifo_data_level_get(&dev_ctx, &num_samples) != 0)
        return;  // Error

    if (num_samples == 0)
        return;  // Nothing to read

    if (num_samples > FIFO_THRESHOLD)
        num_samples = FIFO_THRESHOLD;  // Safety

    // 2. Read them all in one call
    if (iis3dwb_fifo_out_multi_raw_get(&dev_ctx, fifo_buf, num_samples) != 0)
        return;  // Error

    // 3. Process each sample
    /*
    for (uint16_t i = 0; i < num_samples; i++)
    {
        if (fifo_buf[i].tag == IIS3DWB_XL_TAG)  // Accelerometer sample
        {
            int16_t x = (fifo_buf[i].data[1] << 8) | fifo_buf[i].data[0];
            int16_t y = (fifo_buf[i].data[3] << 8) | fifo_buf[i].data[2];
            int16_t z = (fifo_buf[i].data[5] << 8) | fifo_buf[i].data[4];
            // Convert to mg if you want, or just use raw
            // float fx = iis3dwb_from_fs2g_to_mg(x); // if FS=2g
            // ...
            printf("FIFO X:%d Y:%d Z:%d\r\n", x, y, z);
        }
    }
    */
}


























/* FIFO DMA read function */
/*int vib_fifo_dma_read_all(void)
{
    memset(fifo_dma_buffer, 0, FIFO_THRESHOLD * 7);

    HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Receive_DMA(&hspi2, fifo_dma_buffer, FIFO_THRESHOLD * 7) != HAL_OK) {
        HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
        return -1;  // error
    }
    return 0; // success
}*/

/* DMA Complete Callback */
/*void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        HAL_GPIO_WritePin(CS_DWB_GPIO_Port, CS_DWB_Pin, GPIO_PIN_SET);
        osSemaphoreRelease(vib_dma_semHandle);  // Unblock the task!
    }
}
*/
