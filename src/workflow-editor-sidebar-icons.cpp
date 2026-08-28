#include "workflow-editor-sidebar-icons.h"

QIcon workflow_editor_sidebar_node_type_icon(workflow_node_type_t type)
{
    switch (type) {
    case WORKFLOW_NODE_TRIGGER:
        return QIcon::fromTheme("media-playback-start");
    case WORKFLOW_NODE_ACTION:
        return QIcon::fromTheme("system-run");
    case WORKFLOW_NODE_SCENE:
        return QIcon::fromTheme("view-list");
    default:
        return QIcon();
    }
}
