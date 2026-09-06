#pragma once

#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

obs_source_t *workflow_find_source_by_uuid(const char *uuid);
void workflow_log_source_uuid_diagnostics(const char *uuid, const char *name);

#ifdef __cplusplus
}
#endif
