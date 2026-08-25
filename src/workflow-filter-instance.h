#pragma once

#include <obs.h>
#include "workflow-model.h"

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

bool workflow_filter_instance_execute_node(workflow_t *workflow,
										workflow_node_t *node);
