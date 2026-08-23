#pragma once

#include <obs-module.h>
#include <stdbool.h>
#include <stddef.h>

#define WORKFLOW_MAX_NAME 256
#define WORKFLOW_MAX_VALUE 1024

/* The four Move filter families our workflow layer can target. */
typedef enum workflow_move_kind {
	WORKFLOW_MOVE_ACTION = 0,
	WORKFLOW_MOVE_SOURCE,
	WORKFLOW_MOVE_SWAP,
	WORKFLOW_MOVE_VALUE,
} workflow_move_kind_t;

/* A field can inherit the settings already configured on the Move filter,
 * or be supplied by the workflow as an override. */
typedef enum workflow_value_mode {
	WORKFLOW_USE_EXISTING = 0,
	WORKFLOW_OVERRIDE,
} workflow_value_mode_t;

typedef struct workflow_move_action {
	char scene_name[WORKFLOW_MAX_NAME];
	char filter_name[WORKFLOW_MAX_NAME];
	char filter_id[WORKFLOW_MAX_NAME];

	workflow_move_kind_t kind;

	workflow_value_mode_t source_mode;
	char source_value[WORKFLOW_MAX_VALUE];

	workflow_value_mode_t swap_mode;
	char swap_value[WORKFLOW_MAX_VALUE];

	workflow_value_mode_t value_mode;
	char value_value[WORKFLOW_MAX_VALUE];
} workflow_move_action_t;

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
