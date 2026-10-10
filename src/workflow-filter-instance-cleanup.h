#pragma once

#include "workflow-filter-instance.h"

#ifdef __cplusplus
extern "C" {
#endif

bool workflow_filter_instance_parent_cleanup_register(workflow_filter_instance *instance);
void workflow_filter_instance_parent_cleanup_unregister(workflow_filter_instance *instance);

#ifdef __cplusplus
}
#endif
