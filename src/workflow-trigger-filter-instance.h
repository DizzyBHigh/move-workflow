#pragma once

#include <obs.h>
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*workflow_trigger_filter_enum_cb)(obs_source_t *source, obs_source_t *filter, void *param);

bool workflow_trigger_filter_is_instance(obs_source_t *filter);
void workflow_trigger_filter_enum_instances(workflow_trigger_filter_enum_cb callback, void *param);
obs_source_t *workflow_trigger_filter_find(const char *source_uuid, const char *filter_uuid);
bool workflow_trigger_filter_get_target(obs_source_t *filter, char *workflow_id, char *trigger_id);
bool workflow_trigger_filter_set_target(obs_source_t *filter, const char *workflow_id, const char *trigger_id);
void workflow_trigger_filter_ref_set(workflow_trigger_filter_ref_t *ref, obs_source_t *source, obs_source_t *filter);
bool workflow_trigger_filter_ref_matches(const workflow_trigger_filter_ref_t *ref, obs_source_t *source, obs_source_t *filter);

#ifdef __cplusplus
}
#endif
