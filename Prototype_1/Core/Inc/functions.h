#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "stdlib.h"
#include "stm32u5xx.h"

void printWelcomeMessage(void);
uint8_t readUserInput(void);
uint8_t processUserInput(uint8_t opt);
void performCriticalTasks(void);

#endif
