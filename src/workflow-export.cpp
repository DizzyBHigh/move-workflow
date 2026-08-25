#include "workflow-export.h"
#include "workflow-persistence-json.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-module.h>

bool workflow_export_selected(const workflow_manager_t *manager, const char *path)
{
    if (!manager || !path || !path[0]) return false;
    const workflow_t *selected = workflow_manager_selected_const(manager);
    if (!selected) return false;

    workflow_manager_t temporary{};
    workflow_manager_init(&temporary);
    temporary.workflows[0] = *selected;
    temporary.workflow_count = 1;
    snprintf(temporary.selected_workflow_id, WORKFLOW_MAX_NAME, "%s", selected->id);

    QJsonObject root = workflow_manager_to_json(&temporary);
    root["format"] = "obs-move-workflow";
    root["format_version"] = 1;
    root.remove("selected");

    QFile output(QString::fromUtf8(path));
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        blog(LOG_WARNING, "[Move Workflow] Export failed: %s", path);
        return false;
    }
    output.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    const bool ok = output.flush();
    blog(ok ? LOG_INFO : LOG_WARNING, "[Move Workflow] Export %s: %s",
         ok ? "completed" : "failed", path);
    return ok;
}
