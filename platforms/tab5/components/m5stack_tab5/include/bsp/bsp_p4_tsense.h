#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern esp_err_t bsp_tsense_init(void);
extern esp_err_t bsp_tsense_deinit(void);
extern esp_err_t bsp_tsense_read_x100(uint32_t *tempx100);

#ifdef __cplusplus
}   
#endif

