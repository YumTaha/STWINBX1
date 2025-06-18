#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin
//#include "cmsis_os.h"       // For FreeRTOS semaphore
#include <string.h>
#include "stdio.h"

extern UART_HandleTypeDef huart2;
#define FIFO_WATERMARK    300

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
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g);
  iis3dwb_fifo_watermark_set(&dev_ctx, FIFO_WATERMARK); // 1. Set FIFO threshold (samples)
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE); // 2. Set FIFO mode to stop collecting data when FIFO is full
  /* Set data rate to 26,667 times per second */
    iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);

  /* 6) enable FIFO threshold interrupt on INT1 */
  iis3dwb_pin_int1_route_t int1_route = {0};
  int1_route.fifo_th = 1; // Enable FIFO threshold interrupt
  iis3dwb_pin_int1_route_set(&dev_ctx, &int1_route);



  return 0;
}

static iis3dwb_fifo_out_raw_t fifo_data[FIFO_WATERMARK];
iis3dwb_fifo_status_t fifo_status;

/* Read incoming raw data and print them through UART2 */
void vib_read_binary(void)
{
    char msg[32];

    uint16_t num = 0;

    // Read FIFO status
    iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);
    snprintf(msg, sizeof(msg), "FIFO level: %u\r\n", fifo_status.fifo_level);
	HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    if (fifo_status.fifo_th)
    {
        num = fifo_status.fifo_level;

        // Print FIFO level before reading
        snprintf(msg, sizeof(msg), "FIFO before: %u\r\n", num);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

        iis3dwb_fifo_out_multi_raw_get(&dev_ctx, fifo_data, num);

        // Read FIFO status again after draining
        iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);
        snprintf(msg, sizeof(msg), "FIFO after: %u\r\n", fifo_status.fifo_level);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    }
}

