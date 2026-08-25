#include "workflow-debug.h"

#include <obs-module.h>
#include <cstdarg>
#include <cstdio>

static bool debug_enabled;

void workflow_debug_set_enabled(bool enabled)
{
    debug_enabled = enabled;
}

bool workflow_debug_is_enabled(void)
{
    return debug_enabled;
}

void workflow_debug_log(const char *format, ...)
{
    if (!debug_enabled || !format)
        return;

    char message[1024] = {};
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    blog(LOG_INFO, "[Move Workflow][Debug] %s", message);
}
