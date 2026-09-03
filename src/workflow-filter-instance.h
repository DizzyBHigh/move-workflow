#pragma once

#include <obs.h>
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct workflow_filter_instance workflow_filter_instance;
typedef struct workflow_filter_instance_set workflow_filter_instance_set;

struct workflow_filter_instance {
	obs_source_t *original;
	obs_source_t *instance;
	obs_source_t *parent;
};

workflow_filter_instance *workflow_filter_instance_create(obs_source_t *original,
										obs_source_t *parent,
										const workflow_node_t *node);
bool workflow_filter_instance_execute(workflow_filter_instance *instance);
void workflow_filter_instance_destroy(workflow_filter_instance *instance);

workflow_filter_instance_set *workflow_filter_instance_set_create(workflow_t *workflow);
void workflow_filter_instance_set_destroy(workflow_filter_instance_set *set);
bool workflow_filter_instance_set_prepare_node(workflow_filter_instance_set *set,
										workflow_node_t *node);
workflow_filter_instance *workflow_filter_instance_set_get(
										workflow_filter_instance_set *set,
										const workflow_node_t *node);

#ifdef __cplusplus
}
#endif
