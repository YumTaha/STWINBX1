#ifndef VIB_IO_H
#define VIB_IO_H

#include <stdint.h>
#include "iis3dwb_reg.h"
#include "stm32u5xx_hal.h"

/* call once at startup. returns 0 on success */
int32_t  vib_io_init(void);

void vib_read(void);

void get_status(void);
#endif /* VIB_IO_H */
