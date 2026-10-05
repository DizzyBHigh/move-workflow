#include "workflow-filter-instance-cleanup.h"

#include <obs.h>

static void workflow_filter_instance_parent_removed(void *data, calldata_t *call_data)
{
    workflow_filter_instance *instance = (workflow_filter_instance *)data;
    if (!instance || !instance->parent)
        return;

    workflow_filter_instance_destroy(instance);
    (void)call_data;
}

bool workflow_filter_instance_parent_cleanup_register(workflow_filter_instance *instance)
{
    if (!instance || !instance->parent)
        return false;

    signal_handler_t *handler = obs_source_get_signal_handler(instance->parent);
    if (!handler)
        return false;

    signal_handler_connect(handler, "remove",
                           workflow_filter_instance_parent_removed, instance);
    signal_handler_connect(handler, "destroy",
                           workflow_filter_instance_parent_removed, instance);
    return true;
}

void workflow_filter_instance_parent_cleanup_unregister(workflow_filter_instance *instance)
{
    if (!instance || !instance->parent)
        return;

    signal_handler_t *handler = obs_source_get_signal_handler(instance->parent);
    if (!handler)
        return;

    signal_handler_disconnect(handler, "remove",
                              workflow_filter_instance_parent_removed, instance);
    signal_handler_disconnect(handler, "destroy",
                              workflow_filter_instance_parent_removed, instance);
}
