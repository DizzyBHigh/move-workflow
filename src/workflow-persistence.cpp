#include "workflow-persistence.h"
#include "workflow-persistence-json.h"
#include <obs-module.h>
#include <QFile>
#include <QJsonDocument>
#include <cstring>

namespace {
workflow_manager_t manager{};
bool initialized = false;
QString path()
{
    char *p = obs_module_config_path("move-workflow.json");
    if (!p) return {};
    const QString result = QString::fromUtf8(p);
    bfree(p);
    return result;
}
}

void workflow_persistence_init(void)
{
    if (initialized) return;
    initialized = true;
    workflow_manager_init(&manager);
    const QString file = path();
    QFile input(file);
    if (!file.isEmpty() && input.open(QIODevice::ReadOnly)) {
        QJsonParseError error{};
        const QJsonDocument doc = QJsonDocument::fromJson(input.readAll(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject())
            workflow_manager_from_json(&manager, doc.object());
    }
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
    QFile output(file);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QJsonDocument doc(workflow_manager_to_json(&manager));
    output.write(doc.toJson(QJsonDocument::Indented));
    return output.flush();
}

bool workflow_persistence_sync(const workflow_manager_t *source)
{
    if (!source) return false;
    workflow_persistence_init();
    manager = *source;
    return workflow_persistence_save();
}
