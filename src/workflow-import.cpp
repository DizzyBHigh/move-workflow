#include "workflow-import.h"
#include "workflow-node-identity.hpp"
#include "workflow-persistence-json.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-module.h>
#include <cstdio>
#include <cstring>
#include <memory>

static bool unique_id(const workflow_manager_t *m, const char *id)
{
    return !workflow_manager_find_const(m, id);
}

static void make_copy_name(workflow_manager_t *manager, workflow_t *workflow)
{
    if (unique_id(manager, workflow->id)) return;
    char base[WORKFLOW_MAX_NAME];
    snprintf(base, sizeof(base), "%s - Copy", workflow->name);
    snprintf(workflow->name, WORKFLOW_MAX_NAME, "%s", base);
    snprintf(workflow->id, WORKFLOW_MAX_NAME, "%s", base);
    if (unique_id(manager, workflow->id)) return;
    for (unsigned int n = 2; n < 1000; ++n) {
        snprintf(workflow->name, WORKFLOW_MAX_NAME, "%s %u", base, n);
        snprintf(workflow->id, WORKFLOW_MAX_NAME, "%s %u", base, n);
        if (unique_id(manager, workflow->id)) return;
    }
    workflow->name[0] = '\0';
    workflow->id[0] = '\0';
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
        root["format_version"].toInt() != 1) return false;

    const std::unique_ptr<workflow_manager_t> imported(new workflow_manager_t{});
    if (!workflow_manager_from_json(imported.get(), root) || imported->workflow_count != 1)
        return false;
    workflow_t *workflow = &imported->workflows[0];
    workflow_manager_repair_node_ids(imported.get());
    make_copy_name(manager, workflow);
    if (!workflow->id[0] || !workflow->name[0] || !workflow_manager_node_ids_unique(imported.get()))
        return false;
    manager->workflows[manager->workflow_count++] = *workflow;
    workflow_manager_set_selected(manager, workflow->id);
    blog(LOG_INFO, "[Move Workflow] Imported workflow '%s'", workflow->name);
    return true;
}
