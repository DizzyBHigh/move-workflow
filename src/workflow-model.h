#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WORKFLOW_MAX_NAME 256
#define WORKFLOW_MAX_VALUE 1024
#define WORKFLOW_MAX_NODES 32
#define WORKFLOW_MAX_LINKS 8

/*
 * Workflow nodes are either entry triggers or actions that reference an
 * existing Move/Swap/Value filter in OBS. The workflow layer does not
 * recreate the primitive Move action; it only orchestrates it.
 */
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

/* The action that can trigger entry into a workflow branch. */
typedef struct workflow_trigger_ref {
	char action[WORKFLOW_MAX_NAME];
} workflow_trigger_ref_t;

/* A reference to an existing Move-family filter/action in OBS. */
typedef struct workflow_action_ref {
	char scene_name[WORKFLOW_MAX_NAME];
	char source_name[WORKFLOW_MAX_NAME];
	char filter_name[WORKFLOW_MAX_NAME];
	char filter_id[WORKFLOW_MAX_NAME];
	workflow_move_kind_t kind;
} workflow_action_ref_t;

/* Director-controlled duration. */
typedef struct workflow_duration_override {
	workflow_value_mode_t mode;
	uint64_t duration_ms;
} workflow_duration_override_t;

/* Director-owned delay. */
typedef struct workflow_delay_override {
	workflow_value_mode_t mode;
	uint64_t delay_ms;
} workflow_delay_override_t;

/*
 * A node represents either a trigger or one prebuilt Move-family action.
 * Trigger nodes may exist anywhere in the graph; they are not implicitly
 * restricted to the first node.
 *
 * Action nodes deliberately contain no duplicate Move/Swap/Value operation
 * settings. Source + Filter identify the existing action. The node only
 * owns orchestration settings: delays, duration, simultaneous actions and
 * end/next relationships.
 */
typedef struct workflow_node {
	char id[WORKFLOW_MAX_NAME];
	char name[WORKFLOW_MAX_NAME];
	workflow_node_type_t type;

	/* Used by trigger nodes. Empty/None for action nodes. */
	workflow_trigger_ref_t trigger;

	/* Used by action nodes. References an existing OBS filter. */
	workflow_action_ref_t action;

	/* Director-owned settings. */
	workflow_duration_override_t duration;
	workflow_delay_override_t start_delay;
	workflow_delay_override_t end_delay;
	workflow_value_mode_t simultaneous_actions_mode;
	workflow_value_mode_t end_actions_mode;
	workflow_value_mode_t next_actions_mode;

	/*
	 * These legacy fields remain in the model temporarily so the existing
	 * editor/director implementation continues to compile while the UI is
	 * migrated to the explicit Trigger/Action node model. They are no longer
	 * part of the intended Action Node settings surface.
	 */
	workflow_value_mode_t start_trigger_mode;
	workflow_value_mode_t stop_trigger_mode;
	workflow_value_mode_t next_move_on_mode;
	char start_trigger_value[WORKFLOW_MAX_VALUE];
	char stop_trigger_value[WORKFLOW_MAX_VALUE];
	char next_move_on_value[WORKFLOW_MAX_VALUE];

	/*
	 * Relationships refer to other node IDs. Simultaneous nodes start with
	 * this node; end/next nodes are downstream actions.
	 */
	size_t end_node_count;
	char end_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];

	size_t simultaneous_node_count;
	char simultaneous_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];

	size_t next_node_count;
	char next_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];
} workflow_node_t;

typedef struct workflow {
	char id[WORKFLOW_MAX_NAME];
	char name[WORKFLOW_MAX_NAME];
	bool enabled;

	/* Entry nodes are explicit; there may be more than one trigger branch. */
	size_t entry_node_count;
	char entry_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];

	size_t node_count;
	workflow_node_t nodes[WORKFLOW_MAX_NODES];
} workflow_t;

static inline const char *workflow_node_type_name(workflow_node_type_t type)
{
	switch (type) {
	case WORKFLOW_NODE_TRIGGER:
		return "Trigger";
	case WORKFLOW_NODE_ACTION:
		return "Action";
	default:
		return "Unknown";
	}
}

static inline const char *workflow_move_kind_name(workflow_move_kind_t kind)
{
	switch (kind) {
	case WORKFLOW_MOVE_ACTION:
		return "Move Action";
	case WORKFLOW_MOVE_SOURCE:
		return "Move Source";
	case WORKFLOW_MOVE_SWAP:
		return "Move Source Swap";
	case WORKFLOW_MOVE_VALUE:
		return "Move Value";
	default:
		return "Unknown";
	}
}

static inline const char *workflow_value_mode_name(workflow_value_mode_t mode)
{
	return mode == WORKFLOW_OVERRIDE ? "Override" : "Use existing";
}

static inline const char *workflow_expected_filter_id(workflow_move_kind_t kind)
{
	switch (kind) {
	case WORKFLOW_MOVE_ACTION:
		return "move_action_filter";
	case WORKFLOW_MOVE_SOURCE:
		return "move_source_filter";
	case WORKFLOW_MOVE_SWAP:
		return "move_source_swap_filter";
	case WORKFLOW_MOVE_VALUE:
		return "move_value_filter";
	default:
		return "";
	}
}
