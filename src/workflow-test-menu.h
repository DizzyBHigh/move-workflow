#pragma once

#include "workflow-engine.h"
#include "workflow-model.h"

#ifdef __cplusplus
extern "C" {
#endif

void workflow_test_menu_register(workflow_t *workflow,
                                 workflow_engine_t *engine);

#ifdef __cplusplus
}
#endif
