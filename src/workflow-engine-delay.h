#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*workflow_engine_delay_callback_t)(void *data);

bool workflow_engine_delay_start(uint64_t delay_ms,
                                 workflow_engine_delay_callback_t callback,
                                 void *data);
void workflow_engine_delay_shutdown(void);

#ifdef __cplusplus
}
#endif
