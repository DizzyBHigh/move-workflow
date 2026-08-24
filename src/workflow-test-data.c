#include "workflow-runtime.h"

#define TEST_SCENE_NAME "obs-move-workflow test scene"

static workflow_t workflow = {
    .id = "phase14-test-workflow",
    .name = "Test Move Source Workflow",
    .enabled = true,
    .entry_node_count = 6,
    .entry_node_ids = {"move-left", "move-center", "move-right", "move-bottom-left", "move-bottom-center", "move-bottom-right"},
    .node_count = 6,
    .nodes = {
        {.id = "move-left", .name = "Move Source - Top - Left",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Top - Left", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_trigger_mode = WORKFLOW_OVERRIDE,
         .start_trigger_value = "Enable", .stop_trigger_mode = WORKFLOW_USE_EXISTING,
         .simultaneous_actions_mode = WORKFLOW_OVERRIDE, .next_actions_mode = WORKFLOW_OVERRIDE,
         .next_move_on_mode = WORKFLOW_OVERRIDE},
        {.id = "move-center", .name = "Move Source - Top - Center",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Top - Center", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_trigger_mode = WORKFLOW_USE_EXISTING,
         .stop_trigger_mode = WORKFLOW_USE_EXISTING, .simultaneous_actions_mode = WORKFLOW_OVERRIDE,
         .next_actions_mode = WORKFLOW_OVERRIDE, .next_move_on_mode = WORKFLOW_USE_EXISTING},
        {.id = "move-right", .name = "Move Source - Top - Right",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Top - Right", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_trigger_mode = WORKFLOW_USE_EXISTING,
         .stop_trigger_mode = WORKFLOW_USE_EXISTING, .simultaneous_actions_mode = WORKFLOW_OVERRIDE,
         .next_actions_mode = WORKFLOW_OVERRIDE, .next_move_on_mode = WORKFLOW_USE_EXISTING},
        {.id = "move-bottom-left", .name = "Move Source - Bottom - Left",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Bottom - Left", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_trigger_mode = WORKFLOW_USE_EXISTING,
         .stop_trigger_mode = WORKFLOW_USE_EXISTING, .simultaneous_actions_mode = WORKFLOW_OVERRIDE,
         .next_actions_mode = WORKFLOW_OVERRIDE},
        {.id = "move-bottom-center", .name = "Move Source - Bottom - Center",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Bottom - Center", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_delay = {.mode = WORKFLOW_USE_EXISTING, .delay_ms = 0},
         .start_trigger_mode = WORKFLOW_USE_EXISTING, .stop_trigger_mode = WORKFLOW_USE_EXISTING,
         .simultaneous_actions_mode = WORKFLOW_OVERRIDE, .next_actions_mode = WORKFLOW_OVERRIDE},
        {.id = "move-bottom-right", .name = "Move Source - Bottom - Right",
         .action = {.scene_name = TEST_SCENE_NAME, .filter_name = "Move Source - Bottom - Right", .filter_id = "move_source_filter", .kind = WORKFLOW_MOVE_SOURCE},
         .duration = {.mode = WORKFLOW_USE_EXISTING}, .start_delay = {.mode = WORKFLOW_USE_EXISTING, .delay_ms = 0},
         .start_trigger_mode = WORKFLOW_USE_EXISTING, .stop_trigger_mode = WORKFLOW_USE_EXISTING,
         .simultaneous_actions_mode = WORKFLOW_OVERRIDE, .next_actions_mode = WORKFLOW_OVERRIDE},
    },
};

workflow_t *workflow_runtime_test_workflow(void)
{
    return &workflow;
}
