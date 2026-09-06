#pragma once

#include "workflow-model.h"
#include <obs.h>

bool workflow_filter_prepare_for_execution(obs_source_t *runtime, obs_source_t *original,
                                           const workflow_node_t *node);
bool workflow_filter_activate(obs_source_t *source, obs_source_t *parent);
