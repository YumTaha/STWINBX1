#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin
//#include "cmsis_os.h"       // For FreeRTOS semaphore
#include <string.h>
#include "stdio.h"
//#include "app_freertos.h"

extern UART_HandleTypeDef huart2;
//#define IIS3DWB_FIFO_TAG_MASK   0xF8
#define FIFO_WATERMARK    256

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

int _write(int file, char *ptr, int len){
  int i=0;
  for(i=0 ; i<len ; i++)
	  ITM_SendChar((*ptr++));
  return len;
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

  /* 5) basic configuration/fifo */
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g);
  iis3dwb_fifo_watermark_set(&dev_ctx, FIFO_WATERMARK); // 1. Set FIFO threshold (samples)
  iis3dwb_fifo_xl_batch_set(&dev_ctx, IIS3DWB_XL_BATCHED_AT_26k7Hz);
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // 2. Set FIFO mode to stop collecting data when FIFO is full


  /* 6) enable FIFO threshold interrupt on INT1 */
  iis3dwb_pin_int1_route_t int1_route = {0};
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);

  /* Set data rate to 26,667 times per second */
  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);

  return 0;
}





static iis3dwb_fifo_out_raw_t fifo_data[FIFO_WATERMARK];
iis3dwb_fifo_status_t fifo_status;
static uint8_t tx_buffer[2000];

static int16_t *datax;
static int16_t *datay;
static int16_t *dataz;
static int32_t *ts;

void vib_read(void){
	uint16_t num = 0, k;
    uint16_t num_samples = 0;


	/* Read watermark flag */
	iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);

	if (fifo_status.fifo_th == 1) {
		num = fifo_status.fifo_level;
		iis3dwb_fifo_data_level_get(&dev_ctx, &num_samples);
		snprintf((char *)tx_buffer, sizeof(tx_buffer), "-- FIFO num %d : data level %d \r\n", num, num_samples);
		HAL_UART_Transmit(&huart2, tx_buffer, strlen((char const *)tx_buffer), 1000);

		/* read out all FIFO entries in a single read */
		iis3dwb_fifo_out_multi_raw_get(&dev_ctx, fifo_data, num);
		// Reset FIFO
		iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_BYPASS_MODE);
		iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE);

		for (k = 0; k < num; k++) {
			iis3dwb_fifo_out_raw_t *f_data;

			/* print out first two and last two FIFO entries only */
			if (k > 1 && k < num - 2)	continue;

			f_data = &fifo_data[k];

			/* Read FIFO sensor value */
			datax = (int16_t *)&f_data->data[0];
			datay = (int16_t *)&f_data->data[2];
			dataz = (int16_t *)&f_data->data[4];
			ts = (int32_t *)&f_data->data[0];

			switch (f_data->tag >> 3) {
			case IIS3DWB_XL_TAG:
				snprintf((char *)tx_buffer, sizeof(tx_buffer), "%d: ACC [mg]:\t%4.0f\t%4.0f\t%4.0f\r\n",
						k,
						iis3dwb_from_fs8g_to_mg(*datax),
						iis3dwb_from_fs8g_to_mg(*datay),
						iis3dwb_from_fs8g_to_mg(*dataz));
				HAL_UART_Transmit(&huart2, tx_buffer, strlen((char const *)tx_buffer), 1000);
				break;
			default:
				break;
			}
		}
		snprintf((char *)tx_buffer, sizeof(tx_buffer), "------ \r\n\r\n");
		HAL_UART_Transmit(&huart2, tx_buffer, strlen((char const *)tx_buffer), 1000);

	}
}


/*
void vib_fifo_read_all_simple(void)
{
    uint16_t num_samples = 0;
    iis3dwb_fifo_out_raw_t fifo_buf[256]; // adjust size as needed


    // 1. How many samples in FIFO?
    //if (iis3dwb_fifo_data_level_get(&dev_ctx, &num_samples) != 0)
    //    return;  // Error

    //if (num_samples == 0)	return;  // Nothing to read

    //if (num_samples > FIFO_THRESHOLD)	num_samples = FIFO_THRESHOLD;  // Safety


    // 1. Get the number of available FIFO samples
	iis3dwb_fifo_data_level_get(&dev_ctx, &num_samples);
	if (num_samples == 0 || num_samples > 256)
		return; // nothing to read, or error

    // 2. Read them all in one call
    iis3dwb_fifo_out_multi_raw_get(&dev_ctx, fifo_buf, num_samples);


    for (uint16_t i = 0; i < num_samples; i++) {
		uint8_t tag = fifo_buf[i].tag & IIS3DWB_FIFO_TAG_MASK;
		if (tag == IIS3DWB_XL_TAG) {
			// Accelerometer data
			int16_t x = (int16_t)((fifo_buf[i].data[1] << 8) | fifo_buf[i].data[0]);
			int16_t y = (int16_t)((fifo_buf[i].data[3] << 8) | fifo_buf[i].data[2]);
			int16_t z = (int16_t)((fifo_buf[i].data[5] << 8) | fifo_buf[i].data[4]);
			// Process (e.g., send to UART, buffer, etc.)
			char msg[32];
			int len = snprintf(msg, sizeof(msg), "%d,%d,%d\r\n", x, y, z);
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);

		}
    }

    // Reset FIFO
    iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_BYPASS_MODE);
    iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE);
}

*/

