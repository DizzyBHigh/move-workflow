#include "workflow-import.h"
#include "workflow-persistence-json.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-module.h>
#include <cstdio>
#include <cstring>

static bool unique_id(const workflow_manager_t *m, const char *id)
{
    return !workflow_manager_find_const(m, id);
}

bool workflow_import_file(workflow_manager_t *manager, const char *path)
{
    if (!manager || !path || !path[0] || manager->workflow_count >= WORKFLOW_MANAGER_MAX_WORKFLOWS)
        return false;

    QFile input(QString::fromUtf8(path));
    if (!input.open(QIODevice::ReadOnly)) return false;
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) return false;

    const QJsonObject root = doc.object();
    if (root["format"].toString() != "obs-move-workflow" ||
        root["format_version"].toInt() != 1)
        return false;

    workflow_manager_t imported{};
    if (!workflow_manager_from_json(&imported, root) || imported.workflow_count != 1)
        return false;

    workflow_t workflow = imported.workflows[0];
    if (!unique_id(manager, workflow.id)) {
        char base[WORKFLOW_MAX_NAME];
        snprintf(base, sizeof(base), "%s", workflow.id);
        for (unsigned int n = 2; n < 1000; ++n) {
            snprintf(workflow.id, WORKFLOW_MAX_NAME, "%s-import-%u", base, n);
            if (unique_id(manager, workflow.id)) break;
        }
        if (!unique_id(manager, workflow.id)) return false;
    }

    manager->workflows[manager->workflow_count++] = workflow;
    workflow_manager_set_selected(manager, workflow.id);
    blog(LOG_INFO, "[Move Workflow] Imported workflow '%s'", workflow.name);
    return true;
}
