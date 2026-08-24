#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WORKFLOW_MAX_NAME 256
#define WORKFLOW_MAX_VALUE 1024
#define WORKFLOW_MAX_NODES 32
#define WORKFLOW_MAX_LINKS 8

typedef enum workflow_node_type {
    WORKFLOW_NODE_TRIGGER = 0,
    WORKFLOW_NODE_ACTION,
} workflow_node_type_t;

typedef enum workflow_move_kind {
    WORKFLOW_MOVE_ACTION = 0,
    WORKFLOW_MOVE_SOURCE,
    WORKFLOW_MOVE_SWAP,
    WORKFLOW_MOVE_VALUE,
} workflow_move_kind_t;

typedef enum workflow_value_mode {
    WORKFLOW_USE_EXISTING = 0,
    WORKFLOW_OVERRIDE,
} workflow_value_mode_t;

typedef enum workflow_trigger_type {
    WORKFLOW_TRIGGER_NONE = 0,
    WORKFLOW_TRIGGER_FRONTEND_ACTION,
    WORKFLOW_TRIGGER_SOURCE_VISIBILITY,
    WORKFLOW_TRIGGER_SOURCE_MUTE,
    WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK,
    WORKFLOW_TRIGGER_SOURCE_HOTKEY,
    WORKFLOW_TRIGGER_FILTER_ENABLE,
    WORKFLOW_TRIGGER_FRONTEND_HOTKEY,
    WORKFLOW_TRIGGER_SETTING,
    WORKFLOW_TRIGGER_UDP_PACKET,
    WORKFLOW_TRIGGER_WEBSOCKET_REQUEST,
    WORKFLOW_TRIGGER_WEBSOCKET_EVENT,
} workflow_trigger_type_t;

typedef enum workflow_trigger_state {
    WORKFLOW_TRIGGER_STATE_DISABLED = 0,
    WORKFLOW_TRIGGER_STATE_ENABLED = 1,
} workflow_trigger_state_t;

typedef struct workflow_trigger_ref {
    workflow_trigger_type_t type;
    workflow_trigger_state_t state;
    int audio_track;
    uint16_t udp_port;
    char action[WORKFLOW_MAX_NAME];
    char scene_name[WORKFLOW_MAX_NAME];
    char filter_name[WORKFLOW_MAX_NAME];
    char filter_id[WORKFLOW_MAX_NAME];
    char hotkey[WORKFLOW_MAX_NAME];
    char setting_name[WORKFLOW_MAX_NAME];
    char value[WORKFLOW_MAX_VALUE];
    char match[WORKFLOW_MAX_VALUE];
} workflow_trigger_ref_t;

typedef struct workflow_action_ref {
    char scene_name[WORKFLOW_MAX_NAME];
    char source_name[WORKFLOW_MAX_NAME];
    char filter_name[WORKFLOW_MAX_NAME];
    char filter_id[WORKFLOW_MAX_NAME];
    workflow_move_kind_t kind;
} workflow_action_ref_t;

typedef struct workflow_duration_override {
    workflow_value_mode_t mode;
    uint64_t duration_ms;
} workflow_duration_override_t;

typedef struct workflow_delay_override {
    workflow_value_mode_t mode;
    uint64_t delay_ms;
} workflow_delay_override_t;

typedef struct workflow_node {
    char id[WORKFLOW_MAX_NAME];
    char name[WORKFLOW_MAX_NAME];
    workflow_node_type_t type;
    workflow_trigger_ref_t trigger;
    workflow_action_ref_t action;
    workflow_duration_override_t duration;
    workflow_delay_override_t start_delay;
    workflow_delay_override_t end_delay;
    workflow_value_mode_t simultaneous_actions_mode;
    workflow_value_mode_t end_actions_mode;
    workflow_value_mode_t next_actions_mode;

    workflow_value_mode_t start_trigger_mode;
    workflow_value_mode_t stop_trigger_mode;
    workflow_value_mode_t next_move_on_mode;
    char start_trigger_value[WORKFLOW_MAX_VALUE];
    char stop_trigger_value[WORKFLOW_MAX_VALUE];
    char next_move_on_value[WORKFLOW_MAX_VALUE];

    size_t end_node_count;
    char end_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t simultaneous_node_count;
    char simultaneous_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t next_node_count;
    char next_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t shortcut_node_count;
    char shortcut_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
} workflow_node_t;

typedef struct workflow {
    char id[WORKFLOW_MAX_NAME];
    char name[WORKFLOW_MAX_NAME];
    bool enabled;
    size_t entry_node_count;
    char entry_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
    size_t node_count;
    workflow_node_t nodes[WORKFLOW_MAX_NODES];
} workflow_t;

static inline const char *workflow_node_type_name(workflow_node_type_t type)
{
    switch (type) {
    case WORKFLOW_NODE_TRIGGER: return "Trigger";
    case WORKFLOW_NODE_ACTION: return "Action";
    default: return "Unknown";
    }
}

static inline const char *workflow_move_kind_name(workflow_move_kind_t kind)
{
    switch (kind) {
    case WORKFLOW_MOVE_ACTION: return "Move Action";
    case WORKFLOW_MOVE_SOURCE: return "Move Source";
    case WORKFLOW_MOVE_SWAP: return "Move Source Swap";
    case WORKFLOW_MOVE_VALUE: return "Move Value";
    default: return "Unknown";
    }
}

static inline const char *workflow_trigger_type_name(workflow_trigger_type_t type)
{
    switch (type) {
    case WORKFLOW_TRIGGER_FRONTEND_ACTION: return "Frontend Action";
    case WORKFLOW_TRIGGER_SOURCE_VISIBILITY: return "Source Visibility";
    case WORKFLOW_TRIGGER_SOURCE_MUTE: return "Source Mute";
    case WORKFLOW_TRIGGER_SOURCE_AUDIO_TRACK: return "Source Audio Track";
    case WORKFLOW_TRIGGER_SOURCE_HOTKEY: return "Source Hotkey";
    case WORKFLOW_TRIGGER_FILTER_ENABLE: return "Filter Enable";
    case WORKFLOW_TRIGGER_FRONTEND_HOTKEY: return "Frontend Hotkey";
    case WORKFLOW_TRIGGER_SETTING: return "Setting";
    case WORKFLOW_TRIGGER_UDP_PACKET: return "UDP Packet";
    case WORKFLOW_TRIGGER_WEBSOCKET_REQUEST: return "WebSocket Request";
    case WORKFLOW_TRIGGER_WEBSOCKET_EVENT: return "WebSocket Event";
    default: return "None";
    }
}

static inline const char *workflow_value_mode_name(workflow_value_mode_t mode)
{
    return mode == WORKFLOW_OVERRIDE ? "Override" : "Use existing";
}

static inline const char *workflow_expected_filter_id(workflow_move_kind_t kind)
{
    switch (kind) {
    case WORKFLOW_MOVE_ACTION: return "move_action_filter";
    case WORKFLOW_MOVE_SOURCE: return "move_source_filter";
    case WORKFLOW_MOVE_SWAP: return "move_source_swap_filter";
    case WORKFLOW_MOVE_VALUE: return "move_value_filter";
    default: return "";
    }
}
