#include "functions.h"
#include "main.h"


#define WELCOME_MSG "Welcome to the STWINBX1 management console\r\n"
#define MAIN_MENU \
    "Select the option you are interested in:\r\n" \
    "  1. Toggle LED2 LED\r\n" \
    "  2. Read USER BUTTON status\r\n" \
    "  3. Clear screen and print this message\r\n"
#define PROMPT "\r\nEnter your choice: "

extern UART_HandleTypeDef huart2;


void printWelcomeMessage(void) {
	HAL_UART_Transmit(&huart2, (uint8_t*)"\033[0;0H", strlen("\033[0;0H"), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)"\033[2J", strlen("\033[2J"), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)WELCOME_MSG, strlen(WELCOME_MSG), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)MAIN_MENU, strlen(MAIN_MENU), HAL_MAX_DELAY);
}

uint8_t readUserInput(void) {
	char readBuf[1];

	HAL_UART_Transmit(&huart2, (uint8_t*)PROMPT, strlen(PROMPT), HAL_MAX_DELAY);
	HAL_UART_Receive(&huart2, (uint8_t*)readBuf, 1, HAL_MAX_DELAY);

	return atoi(readBuf);
}

uint8_t processUserInput(uint8_t opt) {
	char msg[30];

	if(!opt || opt > 3)
		return 0;

	sprintf(msg, "%d", opt);
	HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

	switch(opt) {
		case 1:
			HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			break;
		case 2:
			sprintf(msg, "\r\nUSER BUTTON status: %s",
					HAL_GPIO_ReadPin(USR_BUTTON_GPIO_Port, USR_BUTTON_Pin) == GPIO_PIN_SET ? "PRESSED" : "RELEASED");
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			break;
		case 3:
			return 2;
	};

	return 1;
 }
