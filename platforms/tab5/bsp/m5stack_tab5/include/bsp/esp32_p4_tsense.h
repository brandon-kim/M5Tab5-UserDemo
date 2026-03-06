#ifndef BSP_ESP32_P4_TSENSE_H
#define BSP_ESP32_P4_TSENSE_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern esp_err_t bsp_tsense_init(void);
extern esp_err_t bsp_tsense_deinit(void);
extern uint32_t  bsp_tsense_read_x100(void);

#ifdef __cplusplus
}   
#endif

#endif // BSP_ESP32_P4_TSENSE_H