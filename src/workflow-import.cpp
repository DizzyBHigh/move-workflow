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

static void remap_imported_node(workflow_t *workflow, const char *old_id, const char *new_id)
{
    for (size_t i = 0; i < workflow->node_count; ++i) {
        workflow_node_t *node = &workflow->nodes[i];
        char (*groups[])[WORKFLOW_MAX_NAME] = {node->end_node_ids,
            node->simultaneous_node_ids, node->next_node_ids, node->shortcut_node_ids};
        const size_t counts[] = {node->end_node_count, node->simultaneous_node_count,
                                 node->next_node_count, node->shortcut_node_count};
        for (size_t g = 0; g < 4; ++g)
            for (size_t j = 0; j < counts[g]; ++j)
                if (strcmp(groups[g][j], old_id) == 0)
                    snprintf(groups[g][j], WORKFLOW_MAX_NAME, "%s", new_id);
    }
    for (size_t i = 0; i < workflow->entry_node_count; ++i)
        if (strcmp(workflow->entry_node_ids[i], old_id) == 0)
            snprintf(workflow->entry_node_ids[i], WORKFLOW_MAX_NAME, "%s", new_id);
}

static bool normalize_imported_node_ids(workflow_manager_t *manager, workflow_t *workflow)
{
    for (size_t i = 0; i < workflow->node_count; ++i) {
        char old_id[WORKFLOW_MAX_NAME];
        snprintf(old_id, sizeof(old_id), "%s", workflow->nodes[i].id);
        if (!workflow_manager_generate_node_id(manager, workflow->nodes[i].id,
                                                sizeof(workflow->nodes[i].id)))
            return false;
        remap_imported_node(workflow, old_id, workflow->nodes[i].id);
    }
    return true;
}

bool workflow_import_file(workflow_manager_t *manager, const char *path)
{
    if (!manager || !path || !path[0] || manager->workflow_count >= WORKFLOW_MANAGER_MAX_WORKFLOWS) {
        blog(LOG_WARNING, "[Move Workflow] Import rejected: invalid manager/path or workflow capacity.");
        return false;
    }
    QFile input(QString::fromUtf8(path));
    if (!input.open(QIODevice::ReadOnly)) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: could not open '%s'.", path);
        return false;
    }
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: invalid JSON (%s).", error.errorString().toUtf8().constData());
        return false;
    }
    const QJsonObject root = doc.object();
    const int version = root["format_version"].toInt();
    if (root["format"].toString() != "obs-move-workflow" || (version != 1 && version != 2)) {
        blog(LOG_WARNING, "[Move Workflow] Import rejected: format='%s', version=%d.",
             root["format"].toString().toUtf8().constData(), version);
        return false;
    }

    const std::unique_ptr<workflow_manager_t> imported(new workflow_manager_t{});
    if (!workflow_manager_from_json(imported.get(), root) || imported->workflow_count != 1) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: JSON contained %zu workflows after parsing.",
             imported->workflow_count);
        return false;
    }
    workflow_t *workflow = &imported->workflows[0];
    blog(LOG_INFO, "[Move Workflow] Import parsed workflow '%s' with %zu nodes.",
         workflow->name, workflow->node_count);
    if (!normalize_imported_node_ids(imported.get(), workflow)) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: could not normalize node IDs.");
        return false;
    }
    make_copy_name(manager, workflow);
    if (!workflow->id[0] || !workflow->name[0] || !workflow_manager_node_ids_unique(imported.get())) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: invalid workflow identity after normalization.");
        return false;
    }

    manager->workflows[manager->workflow_count++] = *workflow;
    workflow_manager_repair_node_ids(manager);
    if (!workflow_manager_node_ids_unique(manager)) {
        blog(LOG_WARNING, "[Move Workflow] Import failed: duplicate node IDs after insertion.");
        return false;
    }
    workflow_manager_set_selected(manager, workflow->id);
    blog(LOG_INFO, "[Move Workflow] Imported workflow '%s' (manager now has %zu workflows).",
         workflow->name, manager->workflow_count);
    return true;
}
