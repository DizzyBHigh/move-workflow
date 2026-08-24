#include "workflow-hotkeys.h"

#include <obs-module.h>

void workflow_hotkeys_register(void)
{
    /* Registration is completed when Shortcut runtime state is available. */
}

void workflow_hotkeys_unregister(void)
{
    /* No global hook is used; OBS owns all keyboard handling. */
}
