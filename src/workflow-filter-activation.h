#pragma once

#include <obs.h>

bool workflow_filter_restore(obs_source_t *runtime, obs_source_t *original);
bool workflow_filter_activate(obs_source_t *source, obs_source_t *parent);
