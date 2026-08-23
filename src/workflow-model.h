#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WORKFLOW_MAX_NAME 256
#define WORKFLOW_MAX_VALUE 1024
#define WORKFLOW_MAX_NODES 32
#define WORKFLOW_MAX_LINKS 8

/*
 * The workflow layer does not recreate Move/Swap/Value actions. A node
 * references an action the OBS user has already built, then optionally
 * overrides the director-level settings that are safe for the workflow to
 * control.
 */
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

/* A reference to an existing Move-family filter/action in OBS. */
typedef struct workflow_action_ref {
	char scene_name[WORKFLOW_MAX_NAME];
	char source_name[WORKFLOW_MAX_NAME];
	char filter_name[WORKFLOW_MAX_NAME];
	char filter_id[WORKFLOW_MAX_NAME];
	workflow_move_kind_t kind;
} workflow_action_ref_t;

/* Duration is one of the director-controlled settings. */
typedef struct workflow_duration_override {
	workflow_value_mode_t mode;
	uint64_t duration_ms;
} workflow_duration_override_t;

/*
 * A node is the reusable director representation of one prebuilt action.
 * Every node has the same shape, so end/next/simultaneous actions can each be
 * independently configured nodes rather than special one-off action types.
 */
typedef struct workflow_node {
	char id[WORKFLOW_MAX_NAME];
	char name[WORKFLOW_MAX_NAME];

	/* The existing action selected by this node. */
	workflow_action_ref_t action;

	/* Director-level overrides. USE_EXISTING leaves the selected action as-is. */
	workflow_duration_override_t duration;
	workflow_value_mode_t end_actions_mode;
	workflow_value_mode_t start_trigger_mode;
	workflow_value_mode_t stop_trigger_mode;
	workflow_value_mode_t simultaneous_actions_mode;
	workflow_value_mode_t next_actions_mode;
	workflow_value_mode_t next_move_on_mode;

	/* Values used only when the corresponding setting is overridden. */
	char start_trigger_value[WORKFLOW_MAX_VALUE];
	char stop_trigger_value[WORKFLOW_MAX_VALUE];
	char next_move_on_value[WORKFLOW_MAX_VALUE];

	/*
	 * Relationships refer to other node IDs. The referenced nodes carry their
	 * own action selection and overrides, so every configured action is treated
	 * consistently by the director.
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

	/* Entry node(s) for the workflow. */
	size_t entry_node_count;
	char entry_node_ids[WORKFLOW_MAX_LINKS][WORKFLOW_MAX_NAME];

	size_t node_count;
	workflow_node_t nodes[WORKFLOW_MAX_NODES];
} workflow_t;

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
