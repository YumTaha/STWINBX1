#ifndef VIB_IO_H
#define VIB_IO_H

#include <stdint.h>
#include "iis3dwb_reg.h"
#include "stm32u5xx_hal.h"

#define FIFO_THRESHOLD 256

/* call once at startup. returns 0 on success */
int32_t  vib_io_init(void);

/* Provide Access via a Getter Function */
stmdev_ctx_t* vib_io_get_ctx(void);

void vib_fifo_read_all_simple(void);

int vib_fifo_dma_read_all(void);

// void vib_fifo_read_all(void);

void vib_read(void);
#endif /* VIB_IO_H */
