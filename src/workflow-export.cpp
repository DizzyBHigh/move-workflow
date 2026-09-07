#include "workflow-export.h"
#include "workflow-editor-node-order.hpp"
#include "workflow-persistence-json.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <obs-module.h>

static QJsonArray export_node_order(const workflow_t *workflow)
{
    QStringList stored = workflow_editor_node_order::stored_order(
        QString::fromUtf8(workflow->id));
    QStringList order;
    for (const QString &id : stored) {
        for (size_t i = 0; i < workflow->node_count; ++i) {
            if (id == QString::fromUtf8(workflow->nodes[i].id)) {
                if (!order.contains(id)) order.append(id);
                break;
            }
        }
    }
    for (size_t i = 0; i < workflow->node_count; ++i) {
        const QString id = QString::fromUtf8(workflow->nodes[i].id);
        if (!order.contains(id)) order.append(id);
    }

    QJsonArray result;
    for (const QString &id : order)
        result.append(id);
    return result;
}

bool workflow_export_selected(const workflow_manager_t *manager, const char *path)
{
    if (!manager || !path || !path[0]) return false;
    const workflow_t *selected = workflow_manager_selected_const(manager);
    if (!selected) return false;

    const QJsonObject manager_json = workflow_manager_to_json(manager);
    const QJsonArray workflows = manager_json["workflows"].toArray();
    QJsonObject selected_json;
    for (const auto &value : workflows) {
        if (!value.isObject()) continue;
        const QJsonObject workflow = value.toObject();
        if (workflow["id"].toString() == QString::fromUtf8(selected->id)) {
            selected_json = workflow;
            break;
        }
    }
    if (selected_json.isEmpty()) return false;
    selected_json["node_order"] = export_node_order(selected);

    QJsonObject root{{"version", manager_json["version"]},
                     {"format", "obs-move-workflow"},
                     {"format_version", 2},
                     {"workflows", QJsonArray{selected_json}}};

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
