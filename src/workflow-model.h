#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WORKFLOW_MAX_NAME 256
#define WORKFLOW_MAX_VALUE 1024
#define WORKFLOW_MAX_NODES 32
#define WORKFLOW_MAX_LINKS 8
#define WORKFLOW_MAX_TRIGGERS 8

typedef enum workflow_node_type { WORKFLOW_NODE_TRIGGER = 0, WORKFLOW_NODE_ACTION } workflow_node_type_t;
typedef enum workflow_move_kind { WORKFLOW_MOVE_ACTION = 0, WORKFLOW_MOVE_SOURCE, WORKFLOW_MOVE_SWAP, WORKFLOW_MOVE_VALUE, WORKFLOW_CHANGE_SCENE } workflow_move_kind_t;
typedef enum workflow_value_mode { WORKFLOW_USE_EXISTING = 0, WORKFLOW_OVERRIDE } workflow_value_mode_t;
typedef enum workflow_scene_completion { WORKFLOW_SCENE_COMPLETE_IMMEDIATE = 0, WORKFLOW_SCENE_COMPLETE_TRANSITION = 1 } workflow_scene_completion_t;
typedef enum workflow_easing { WORKFLOW_EASE_NONE = 0, WORKFLOW_EASE_IN = 1, WORKFLOW_EASE_OUT = 2, WORKFLOW_EASE_IN_OUT = 3 } workflow_easing_t;
typedef enum workflow_easing_function {
    WORKFLOW_EASING_QUADRATIC = 1,
    WORKFLOW_EASING_CUBIC = 2,
    WORKFLOW_EASING_QUARTIC = 3,
    WORKFLOW_EASING_QUINTIC = 4,
    WORKFLOW_EASING_SINE = 5,
    WORKFLOW_EASING_CIRCULAR = 6,
    WORKFLOW_EASING_EXPONENTIAL = 7,
    WORKFLOW_EASING_ELASTIC = 8,
    WORKFLOW_EASING_BOUNCE = 9,
    WORKFLOW_EASING_BACK = 10
} workflow_easing_function_t;

typedef struct workflow_trigger_filter_ref { char source_uuid[WORKFLOW_MAX_NAME]; char filter_uuid[WORKFLOW_MAX_NAME]; } workflow_trigger_filter_ref_t;
typedef struct workflow_action_ref { char scene_name[WORKFLOW_MAX_NAME]; char source_name[WORKFLOW_MAX_NAME]; char filter_name[WORKFLOW_MAX_NAME]; char filter_id[WORKFLOW_MAX_NAME]; workflow_move_kind_t kind; workflow_scene_completion_t scene_completion; } workflow_action_ref_t;
typedef struct workflow_duration_override { workflow_value_mode_t mode; uint64_t duration_ms; } workflow_duration_override_t;
typedef struct workflow_delay_override { workflow_value_mode_t mode; uint64_t delay_ms; } workflow_delay_override_t;
typedef struct workflow_easing_override { workflow_value_mode_t mode; workflow_easing_t easing; workflow_easing_function_t function; } workflow_easing_override_t;

typedef struct workflow_node {
    char id[WORKFLOW_MAX_NAME]; char name[WORKFLOW_MAX_NAME]; workflow_node_type_t type; int position_x; int position_y;
    size_t trigger_count; workflow_trigger_filter_ref_t triggers[WORKFLOW_MAX_TRIGGERS]; workflow_action_ref_t action;
    workflow_duration_override_t duration; workflow_delay_override_t start_delay; workflow_delay_override_t end_delay;
    workflow_easing_override_t easing;
    workflow_value_mode_t simultaneous_actions_mode; workflow_value_mode_t end_actions_mode; workflow_value_mode_t next_actions_mode;
    workflow_value_mode_t start_trigger_mode; workflow_value_mode_t stop_trigger_mode; workflow_value_mode_t next_move_on_mode;
    char start_trigger_value[WORKFLOW_MAX_VALUE]; char stop_trigger_value[WORKFLOW_MAX_VALUE]; char next_move_on_value[WORKFLOW_MAX_VALUE];
    size_t end_node_count; char end_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t simultaneous_node_count; char simultaneous_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t next_node_count; char next_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t shortcut_node_count; char shortcut_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    uint32_t shortcut_key[WORKFLOW_MAX_LINKS]; uint32_t shortcut_modifiers[WORKFLOW_MAX_LINKS];
} workflow_node_t;

typedef struct workflow { char id[WORKFLOW_MAX_NAME]; char name[WORKFLOW_MAX_NAME]; bool enabled; size_t entry_node_count; char entry_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME]; size_t node_count; workflow_node_t nodes[WORKFLOW_MAX_NODES]; } workflow_t;

static inline const char *workflow_node_type_name(workflow_node_type_t type){switch(type){case WORKFLOW_NODE_TRIGGER:return "Trigger";case WORKFLOW_NODE_ACTION:return "Action";default:return "Unknown";}}
static inline const char *workflow_move_kind_name(workflow_move_kind_t kind){switch(kind){case WORKFLOW_MOVE_ACTION:return "Move Action";case WORKFLOW_MOVE_SOURCE:return "Move Source";case WORKFLOW_MOVE_SWAP:return "Move Source Swap";case WORKFLOW_MOVE_VALUE:return "Move Value";case WORKFLOW_CHANGE_SCENE:return "Change Scene";default:return "Unknown";}}
static inline const char *workflow_value_mode_name(workflow_value_mode_t mode){return mode==WORKFLOW_OVERRIDE?"Override":"Use existing";}
static inline const char *workflow_expected_filter_id(workflow_move_kind_t kind){switch(kind){case WORKFLOW_MOVE_ACTION:return "move_action_filter";case WORKFLOW_MOVE_SOURCE:return "move_source_filter";case WORKFLOW_MOVE_SWAP:return "move_source_swap_filter";case WORKFLOW_MOVE_VALUE:return "move_value_filter";default:return "";}}
