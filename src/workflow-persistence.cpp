#include "workflow-persistence.h"
#include "workflow-persistence-json.h"
#include <obs-module.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace {
workflow_manager_t manager{};
bool initialized = false;

QString path()
{
    char *p = obs_module_config_path("move-workflow.json");
    if (!p) {
        blog(LOG_WARNING, "[Move Workflow] Could not resolve config path");
        return {};
    }
    const QString result = QString::fromUtf8(p);
    bfree(p);
    blog(LOG_INFO, "[Move Workflow] Persistence path: %s", result.toUtf8().constData());
    return result;
}
}

void workflow_persistence_init(void)
{
    if (initialized) return;
    initialized = true;
    workflow_manager_init(&manager);
    const QString file = path();
    if (file.isEmpty()) return;
    QFile input(file);
    if (!input.open(QIODevice::ReadOnly)) {
        blog(LOG_INFO, "[Move Workflow] No saved workflow file: %s", file.toUtf8().constData());
        return;
    }
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(input.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        blog(LOG_WARNING, "[Move Workflow] Invalid workflow JSON: %s", error.errorString().toUtf8().constData());
        return;
    }
    if (workflow_manager_from_json(&manager, doc.object()))
        blog(LOG_INFO, "[Move Workflow] Loaded %zu workflows", manager.workflow_count);
    else
        blog(LOG_WARNING, "[Move Workflow] Workflow JSON contained no workflows");
}

workflow_manager_t *workflow_persistence_manager(void)
{
    workflow_persistence_init();
    return &manager;
}

bool workflow_persistence_save(void)
{
    workflow_persistence_init();
    const QString file = path();
    if (file.isEmpty()) return false;
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile output(file);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        blog(LOG_WARNING, "[Move Workflow] Failed to open save file: %s", file.toUtf8().constData());
        return false;
    }
    const QJsonDocument doc(workflow_manager_to_json(&manager));
    output.write(doc.toJson(QJsonDocument::Indented));
    const bool ok = output.flush();
    blog(ok ? LOG_INFO : LOG_WARNING, "[Move Workflow] %s %zu workflows",
         ok ? "Saved" : "Failed to save", manager.workflow_count);
    return ok;
}

bool workflow_persistence_sync(const workflow_manager_t *source)
{
    if (!source) return false;
    workflow_persistence_init();
    manager = *source;
    return workflow_persistence_save();
}