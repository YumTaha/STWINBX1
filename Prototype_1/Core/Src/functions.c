#include "functions.h"
#include "main.h"


#define WELCOME_MSG "Welcome to the STWINBX1 management console\r\n"
#define MAIN_MENU \
    "Select the option you are interested in:\r\n" \
    "  1. Toggle LED2 LED\r\n" \
    "  2. Read USER BUTTON status\r\n" \
    "  3. Clear screen and print this message\r\n"
#define PROMPT "\r\n> "

extern UART_HandleTypeDef huart2;
extern volatile FlagStatus UartReady;
extern uint8_t readBuf[2];


void printWelcomeMessage(void) {
	char *strings[] = {"\033[0;0H", "\033[2J", WELCOME_MSG, MAIN_MENU, PROMPT};

	for (uint8_t i = 0; i < 5; i++) {
		HAL_UART_Transmit_IT(&huart2, (uint8_t*)strings[i], strlen(strings[i]));
		while (HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX ||
				HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX_RX);
	}
}

uint8_t readUserInput(void) {
    int8_t retVal = -1;

    if(UartReady == SET) {
        UartReady = RESET;
        readBuf[1] = 0;
        retVal = atoi((char *)readBuf);
    }
    return retVal;
}

uint8_t processUserInput(uint8_t opt) {
	char msg[32];

	if(!opt || opt > 3)
		return 0;

	sprintf(msg, "%d", opt);
	HAL_UART_Transmit_IT(&huart2, (uint8_t*)msg, strlen(msg));
	    while (HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX ||
	           HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX_RX);

	switch(opt) {
		case 1:
			HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)"\r\nLED2 toggled\r\n", strlen("\r\nLED2 toggled\r\n"));
			while (HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX ||
				   HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX_RX);
			break;
		case 2:
			sprintf(msg, "\r\nUSER BUTTON status: %s\r\n",
					HAL_GPIO_ReadPin(USR_BUTTON_GPIO_Port, USR_BUTTON_Pin) == GPIO_PIN_SET ? "PRESSED" : "RELEASED");
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)msg, strlen(msg));
			while (HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX ||
				   HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX_RX);
			break;
		case 3:
			return 2;
	};

	return 1;
 }

void performCriticalTasks(void){
	HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); HAL_Delay(20);
}
