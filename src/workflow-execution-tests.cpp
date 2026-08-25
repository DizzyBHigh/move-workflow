#include "workflow-action-runtime.h"
#include "workflow-debug.h"

#include <cstring>

namespace {

bool expect(const char *name, bool condition)
{
    workflow_debug_log("Execution test: %s -> %s", name,
                       condition ? "PASS" : "FAIL");
    return condition;
}

bool action_runtime_lifecycle_test()
{
    workflow_t workflow{};
    workflow_node_t node{};

    std::strncpy(workflow.id, "execution-test", WORKFLOW_MAX_NAME - 1);
    std::strncpy(workflow.name, "Execution Runtime Test", WORKFLOW_MAX_NAME - 1);
    std::strncpy(node.id, "action-test", WORKFLOW_MAX_NAME - 1);
    node.type = WORKFLOW_NODE_ACTION;

    node.start_delay.mode = WORKFLOW_OVERRIDE;
    node.start_delay.delay_ms = 100;
    node.duration.mode = WORKFLOW_OVERRIDE;
    node.duration.duration_ms = 500;
    node.end_delay.mode = WORKFLOW_OVERRIDE;
    node.end_delay.delay_ms = 200;

    workflow_action_runtime_t *runtime =
        workflow_action_runtime_create(&workflow, &node, 42);
    if (!runtime)
        return expect("runtime allocation", false);

    bool result = true;
    result &= expect("initial state is start delay",
                     workflow_action_runtime_state(runtime) ==
                         WORKFLOW_ACTION_RUNTIME_WAIT_START_DELAY);
    result &= expect("start delay captured",
                     workflow_action_runtime_start_delay_ms(runtime) == 100);
    result &= expect("duration captured",
                     workflow_action_runtime_duration_ms(runtime) == 500);
    result &= expect("end delay captured",
                     workflow_action_runtime_end_delay_ms(runtime) == 200);
    result &= expect("generation captured",
                     workflow_action_runtime_generation(runtime) == 42);

    workflow_action_runtime_begin_execution(runtime);
    result &= expect("execution state entered",
                     workflow_action_runtime_state(runtime) ==
                         WORKFLOW_ACTION_RUNTIME_EXECUTING);

    workflow_action_runtime_complete_duration(runtime);
    result &= expect("duration enters end delay",
                     workflow_action_runtime_state(runtime) ==
                         WORKFLOW_ACTION_RUNTIME_WAIT_END_DELAY);

    workflow_action_runtime_complete(runtime);
    result &= expect("end delay enters complete",
                     workflow_action_runtime_state(runtime) ==
                         WORKFLOW_ACTION_RUNTIME_COMPLETE);

    workflow_action_runtime_destroy(runtime);
    return result;
}

struct ExecutionTests {
    ExecutionTests()
    {
        const bool passed = action_runtime_lifecycle_test();
        workflow_debug_log("Execution tests: action runtime lifecycle -> %s",
                           passed ? "PASS" : "FAIL");
    }
};

ExecutionTests execution_tests;

} // namespace
