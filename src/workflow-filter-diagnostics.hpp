#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

void workflow_filter_diagnostics_log_runtime(obs_source_t *source,
                                             const char *stage);
void workflow_filter_diagnostics_log_target(obs_source_t *filter,
                                            const char *stage);
void workflow_filter_diagnostics_begin(obs_source_t *filter,
                                       uint32_t duration_ms);

#ifdef __cplusplus
}
#endif
