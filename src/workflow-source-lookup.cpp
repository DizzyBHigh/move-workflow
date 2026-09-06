#include "workflow-source-lookup.h"

#include "workflow-debug.h"

#include <cstring>

struct source_lookup_context {
    const char *uuid;
    obs_source_t *source;
};

static bool find_source_callback(void *data, obs_source_t *source)
{
    auto *context = (source_lookup_context *)data;
    if (!context || context->source || !source || !context->uuid)
        return true;

    const char *uuid = obs_source_get_uuid(source);
    if (uuid && !strcmp(uuid, context->uuid)) {
        context->source = obs_source_get_ref(source);
        return false;
    }
    return true;
}

obs_source_t *workflow_find_source_by_uuid(const char *uuid)
{
    if (!uuid || !*uuid)
        return nullptr;

    source_lookup_context context{uuid, nullptr};
    obs_enum_sources(find_source_callback, &context);
    return context.source;
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
    obs_enum_sources(diagnose_source_callback, &context);

    workflow_debug_log("Source UUID lookup: uuid='%s' name='%s' top_level_sources=%zu name_matches=%zu",
                       uuid ? uuid : "", name ? name : "", context.total,
                       context.name_matches);
}
