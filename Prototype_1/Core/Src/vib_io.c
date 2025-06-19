#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin
#include <string.h>
#include "stdio.h"

extern UART_HandleTypeDef huart2;
#define FIFO_WATERMARK    500

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
  iis3dwb_pin_int1_route_t int1_route = {0};
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);

  /* Set Output Data Rate */
  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);

  return 0;
}



static iis3dwb_fifo_out_raw_t fifo_data[5000];
static iis3dwb_fifo_out_raw_t *current = fifo_data;
static uint32_t fifo_level = 0;
iis3dwb_fifo_status_t fifo_status;

void vib_read(void) {
    uint16_t num = 0;

    // Read FIFO status
    iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);

    if (fifo_status.fifo_th == 1) {
        num = fifo_status.fifo_level;

        // Make sure we don't write past the end of the buffer
        if (fifo_level + num > 5000) {
            // Reset buffer if overrun would occur
            current = fifo_data;
            fifo_level = 0;
        }

        // Read out FIFO entries in a single read
        iis3dwb_fifo_out_multi_raw_get(&dev_ctx, current, num);

        current += num;         // Advance pointer
        fifo_level += num;      // Track number of samples
    }

    // Optionally reset buffer after enough samples
    if (fifo_level >= 4000) {
    	// Disable and write the buffer to uart, then reset buffer variables re-enable and continue
        current = fifo_data;
        fifo_level = 0;
    }
}


void get_status(void){

	iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);
}
