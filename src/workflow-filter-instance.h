#pragma once

#include <obs.h>

struct workflow_node;

struct workflow_filter_instance {
	obs_source_t *original;
	obs_source_t *instance;
	obs_source_t *parent;
};

workflow_filter_instance *workflow_filter_instance_create(obs_source_t *original,
										obs_source_t *parent,
										const workflow_node *node);

bool workflow_filter_instance_execute(workflow_filter_instance *instance);

void workflow_filter_instance_destroy(workflow_filter_instance *instance);
