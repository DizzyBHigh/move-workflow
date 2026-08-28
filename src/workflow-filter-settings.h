#pragma once

#include <cstdint>

#include "workflow-model.h"

struct obs_source;

void workflow_filter_apply_node_settings(obs_source_t *filter,
                                         const workflow_node_t *node,
                                         uint64_t *workflow_duration_ms,
                                         uint64_t *restore_delay_ms);
