#include "workflow-source-lookup.h"

#include "workflow-debug.h"

#include <cstring>

obs_source_t *workflow_find_source_by_uuid(const char *uuid)
{
    if (!uuid || !*uuid)
        return nullptr;

    return obs_get_source_by_uuid(uuid);
}

struct source_diagnostic_context {
    const char *uuid;
    const char *name;
    size_t total;
    size_t name_matches;
};

static bool diagnose_source_callback(void *data, obs_source_t *source)
{
    auto *context = (source_diagnostic_context *)data;
    if (!context || !source)
        return true;

    ++context->total;
    const char *source_name = obs_source_get_name(source);
    if (context->name && source_name && !strcmp(context->name, source_name))
        ++context->name_matches;
    return true;
}

void workflow_log_source_uuid_diagnostics(const char *uuid, const char *name)
{
    source_diagnostic_context context{uuid, name, 0, 0};
    obs_enum_all_sources(diagnose_source_callback, &context);

    workflow_debug_log("Source UUID lookup: uuid='%s' name='%s' all_sources=%zu name_matches=%zu",
                       uuid ? uuid : "", name ? name : "", context.total,
                       context.name_matches);
}
