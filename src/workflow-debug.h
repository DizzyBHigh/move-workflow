#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void workflow_debug_set_enabled(bool enabled);
bool workflow_debug_is_enabled(void);
void workflow_debug_log(const char *format, ...);

#ifdef __cplusplus
}
#endif
