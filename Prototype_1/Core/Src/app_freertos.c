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
/* Definitions for LED1_task */
osThreadId_t LED1_taskHandle;
const osThreadAttr_t LED1_task_attributes = {
  .name = "LED1_task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for LED2_task */
osThreadId_t LED2_taskHandle;
const osThreadAttr_t LED2_task_attributes = {
  .name = "LED2_task",
  .priority = (osPriority_t) osPriorityBelowNormal,
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
  /* creation of LED1_task */
  LED1_taskHandle = osThreadNew(LED1_task_function, NULL, &LED1_task_attributes);

  /* creation of LED2_task */
  LED2_taskHandle = osThreadNew(LED2_task_function, NULL, &LED2_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_LED1_task_function */
/**
* @brief Function implementing the LED1_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED1_task_function */
void LED1_task_function(void *argument)
{
  /* USER CODE BEGIN LED1_task */
  /* Infinite loop */
  for(;;)
  {
	  /*
	   * THIS TASK TOGGLES GREEN LED EVERY HALF A SECOND
	   */
	  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
	  osDelay(500);
  }
  /* USER CODE END LED1_task */
}

/* USER CODE BEGIN Header_LED2_task_function */
/**
* @brief Function implementing the LED2_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LED2_task_function */
void LED2_task_function(void *argument)
{
  /* USER CODE BEGIN LED2_task */
  /* Infinite loop */
  for(;;)
  {
	  /*
	   * THIS TASK TOGGLES ORANGE LED EVERY QUARTER A SECOND
	   */
	  HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
	  osDelay(250);
  }
  /* USER CODE END LED2_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

