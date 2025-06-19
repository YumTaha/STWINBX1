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
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE); /* Enable Block Data Update */ // Important if you are reading acceleration
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g); /* Set full scale */
  iis3dwb_i2c_interface_set(&dev_ctx, PROPERTY_ENABLE); /* Disable I2C */ // Not important
  iis3dwb_fifo_watermark_set(&dev_ctx, FIFO_WATERMARK); // 1. Set FIFO threshold (samples)
  iis3dwb_fifo_xl_batch_set(&dev_ctx, IIS3DWB_XL_BATCHED_AT_26k7Hz);
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // 2. Set FIFO mode to stop collecting data when FIFO is full


  /* 6) enable FIFO threshold interrupt on INT1 */
  iis3dwb_pin_int1_route_t int1_route;
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);

  /* Set Output Data Rate */
  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);

  return 0;
}





static iis3dwb_fifo_out_raw_t fifo_data[FIFO_WATERMARK];
iis3dwb_fifo_status_t fifo_status;
static uint8_t tx_buffer[1000];

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

		// Clear FIFO #########IMPORTANT#########
		iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_BYPASS_MODE); // Disable and Clears FIFO mode
		iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // Enable FIFO mode


		for (k = 0; k < num; k++) {
			iis3dwb_fifo_out_raw_t *f_data;

			/* print out first two and last two FIFO entries only */
			if (k > 0 && k < num - 1)	continue;

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
						iis3dwb_from_fs2g_to_mg(*datax),
						iis3dwb_from_fs2g_to_mg(*datay),
						iis3dwb_from_fs2g_to_mg(*dataz));
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

