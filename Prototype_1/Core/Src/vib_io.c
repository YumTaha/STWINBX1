#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
#include "main.h"      // for CS_DWB_GPIO_Port, CS_DWB_Pin

extern SPI_HandleTypeDef hspi2;
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
  iis3dwb_xl_data_rate_set  (&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_2g);
  iis3dwb_xl_filt_path_on_out_set(&dev_ctx, IIS3DWB_LP_6k3Hz);

  /* 6) enable data-ready interrupt on INT1 */
  iis3dwb_int1_ctrl_t int1_ctrl;
  iis3dwb_read_reg(&dev_ctx, IIS3DWB_INT1_CTRL, (uint8_t *)&int1_ctrl, 1);
  int1_ctrl.int1_drdy_xl = 1;
  iis3dwb_write_reg(&dev_ctx, IIS3DWB_INT1_CTRL, (uint8_t *)&int1_ctrl, 1);

  return 0;
}
