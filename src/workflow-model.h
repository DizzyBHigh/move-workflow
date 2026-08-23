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
 * references an action the OBS user has already built, then controls the
 * workflow around that action.
 *
 * The selected action supplies the primitive operation (move/source/swap/
 * value). Workflow orchestration is owned by the director and is NOT copied
 * from the selected Move filter: start/end actions, triggers, simultaneous
 * actions, next actions, next-move-on, and delays are director concerns.
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

/* The action that can trigger entry into a workflow. */
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

/* Duration is a director-controlled setting. */
typedef struct workflow_duration_override {
	workflow_value_mode_t mode;
	uint64_t duration_ms;
} workflow_duration_override_t;

/* Director-owned delay; meaningful for Move Source / Move Source Swap. */
typedef struct workflow_delay_override {
	workflow_value_mode_t mode;
	uint64_t delay_ms;
} workflow_delay_override_t;

/*
 * A node is the reusable director representation of one prebuilt action.
 * Relationships point to other nodes, so every chained action is configured
 * through the same director model.
 */
typedef struct workflow_node {
	char id[WORKFLOW_MAX_NAME];
	char name[WORKFLOW_MAX_NAME];

	/* Entry trigger. Normally configured only on the first node in a tree. */
	workflow_trigger_ref_t trigger;

	/* The existing action selected by this node. */
	workflow_action_ref_t action;

	/* Director-owned settings. */
	workflow_duration_override_t duration;
	workflow_delay_override_t start_delay;
	workflow_delay_override_t end_delay;
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
	 * own action selection and overrides.
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
