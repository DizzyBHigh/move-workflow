#pragma once

#include <stdint.h>

struct workflow_node_timing_defaults {
    uint64_t start_delay_ms;
    uint64_t duration_ms;
    uint64_t end_delay_ms;
    bool valid;
};

workflow_node_timing_defaults workflow_node_read_timing_defaults(
    const char *scene_name, const char *filter_name);
