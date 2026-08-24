#pragma once

#include "workflow-model.h"

#include <QComboBox>
#include <QLineEdit>

#include <cstring>

inline void settings_copy_text(char *destination, size_t capacity,
                                const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

inline QString settings_read_text(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

inline void settings_searchable(QComboBox *combo)
{
    if (!combo || combo->isEditable())
        return;
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);
}

inline bool settings_supported_filter(const char *id)
{
    return id && (!std::strcmp(id, "move_action_filter") ||
                  !std::strcmp(id, "move_source_filter") ||
                  !std::strcmp(id, "move_source_swap_filter") ||
                  !std::strcmp(id, "move_value_filter"));
}

inline workflow_move_kind_t settings_kind(const char *id)
{
    if (id && !std::strcmp(id, "move_source_filter"))
        return WORKFLOW_MOVE_SOURCE;
    if (id && !std::strcmp(id, "move_source_swap_filter"))
        return WORKFLOW_MOVE_SWAP;
    if (id && !std::strcmp(id, "move_value_filter"))
        return WORKFLOW_MOVE_VALUE;
    return WORKFLOW_MOVE_ACTION;
}
