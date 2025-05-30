/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
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

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "vib_io.h"
#include "iis3dwb_reg.h"
#include "steval_stwinbx1_bus.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for LED1_g_task */
osThreadId_t LED1_g_taskHandle;
const osThreadAttr_t LED1_g_task_attributes = {
  .name = "LED1_g_task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for LED2_o_task */
osThreadId_t LED2_o_taskHandle;
const osThreadAttr_t LED2_o_task_attributes = {
  .name = "LED2_o_task",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of LED1_g_task */
  LED1_g_taskHandle = osThreadNew(LED1_g_task_function, NULL, &LED1_g_task_attributes);

  /* creation of LED2_o_task */
  LED2_o_taskHandle = osThreadNew(LED2_o_task_function, NULL, &LED2_o_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_LED1_g_task_function */
/**
* @brief Function implementing the LED1_g_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED1_g_task_function */
void LED1_g_task_function(void *argument)
{
  /* USER CODE BEGIN LED1_g_task */
	/* Infinite loop */
	for(;;)
	{
	osDelay(1);
	}
  /* USER CODE END LED1_g_task */
}

/* USER CODE BEGIN Header_LED2_o_task_function */
/**
* @brief Function implementing the LED2_o_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED2_o_task_function */
void LED2_o_task_function(void *argument)
{
  /* USER CODE BEGIN LED2_o_task */
	/* Infinite loop */
	for(;;)
	{
	osDelay(1);
	}
  /* USER CODE END LED2_o_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

