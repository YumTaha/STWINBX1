/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : steval_stwinbx1_bus.h
  * @brief          : header file for the BSP BUS IO driver
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
*/
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STEVAL_STWINBX1_BUS_H
#define STEVAL_STWINBX1_BUS_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "steval_stwinbx1_conf.h"
#include "steval_stwinbx1_errno.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STEVAL_STWINBX1
  * @{
  */

/** @defgroup STEVAL_STWINBX1_BUS STEVAL_STWINBX1 BUS
  * @{
  */

/** @defgroup STEVAL_STWINBX1_BUS_Exported_Constants STEVAL_STWINBX1 BUS Exported Constants
  * @{
  */

#define BUS_SPI2_INSTANCE SPI2
#define BUS_SPI2_SCK_GPIO_PORT GPIOI
#define BUS_SPI2_SCK_GPIO_PIN GPIO_PIN_1
#define BUS_SPI2_SCK_GPIO_AF GPIO_AF5_SPI2
#define BUS_SPI2_SCK_GPIO_CLK_DISABLE() __HAL_RCC_GPIOI_CLK_DISABLE()
#define BUS_SPI2_SCK_GPIO_CLK_ENABLE() __HAL_RCC_GPIOI_CLK_ENABLE()
#define BUS_SPI2_MISO_GPIO_CLK_DISABLE() __HAL_RCC_GPIOD_CLK_DISABLE()
#define BUS_SPI2_MISO_GPIO_PORT GPIOD
#define BUS_SPI2_MISO_GPIO_AF GPIO_AF5_SPI2
#define BUS_SPI2_MISO_GPIO_CLK_ENABLE() __HAL_RCC_GPIOD_CLK_ENABLE()
#define BUS_SPI2_MISO_GPIO_PIN GPIO_PIN_3
#define BUS_SPI2_MOSI_GPIO_CLK_ENABLE() __HAL_RCC_GPIOI_CLK_ENABLE()
#define BUS_SPI2_MOSI_GPIO_CLK_DISABLE() __HAL_RCC_GPIOI_CLK_DISABLE()
#define BUS_SPI2_MOSI_GPIO_AF GPIO_AF5_SPI2
#define BUS_SPI2_MOSI_GPIO_PORT GPIOI
#define BUS_SPI2_MOSI_GPIO_PIN GPIO_PIN_3

#ifndef BUS_SPI2_POLL_TIMEOUT
  #define BUS_SPI2_POLL_TIMEOUT                   0x1000U
#endif
/* SPI2 Baud rate in bps  */
#ifndef BUS_SPI2_BAUDRATE
   #define BUS_SPI2_BAUDRATE   10000000U /* baud rate of SPIn = 10 Mbps*/
#endif

/**
  * @}
  */

/** @defgroup STEVAL_STWINBX1_BUS_Private_Types STEVAL_STWINBX1 BUS Private types
  * @{
  */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
typedef struct
{
  pSPI_CallbackTypeDef  pMspInitCb;
  pSPI_CallbackTypeDef  pMspDeInitCb;
}BSP_SPI_Cb_t;
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1U) */
/**
  * @}
  */

/** @defgroup STEVAL_STWINBX1_LOW_LEVEL_Exported_Variables LOW LEVEL Exported Constants
  * @{
  */

extern SPI_HandleTypeDef hspi2;

/**
  * @}
  */

/** @addtogroup STEVAL_STWINBX1_BUS_Exported_Functions
  * @{
  */

/* BUS IO driver over SPI Peripheral */
HAL_StatusTypeDef MX_SPI2_Init(SPI_HandleTypeDef* hspi);
int32_t BSP_SPI2_Init(void);
int32_t BSP_SPI2_DeInit(void);
int32_t BSP_SPI2_Send(uint8_t *pData, uint16_t Length);
int32_t BSP_SPI2_Recv(uint8_t *pData, uint16_t Length);
int32_t BSP_SPI2_SendRecv(uint8_t *pTxData, uint8_t *pRxData, uint16_t Length);
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
int32_t BSP_SPI2_RegisterDefaultMspCallbacks (void);
int32_t BSP_SPI2_RegisterMspCallbacks (BSP_SPI_Cb_t *Callbacks);
#endif /* (USE_HAL_SPI_REGISTER_CALLBACKS == 1U) */

int32_t BSP_GetTick(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* STEVAL_STWINBX1_BUS_H */

