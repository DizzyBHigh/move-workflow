#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

void workflow_trigger_filter_methods_create(void **runtime, obs_source_t *filter, obs_data_t *settings);
void workflow_trigger_filter_methods_destroy(void **runtime);
void workflow_trigger_filter_methods_update(void **runtime, obs_source_t *filter, obs_data_t *settings);

#ifdef __cplusplus
}
#endif
