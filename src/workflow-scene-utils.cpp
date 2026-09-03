#include "workflow-scene-utils.h"

#include <cstring>

namespace workflow_scene_utils {

void copy_text(char *destination, size_t capacity, const QString &value)
{
    if (!destination || capacity == 0)
        return;
    const QByteArray bytes = value.toUtf8();
    std::strncpy(destination, bytes.constData(), capacity - 1);
    destination[capacity - 1] = '\0';
}

bool add_node_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id)
{
    const QByteArray bytes = id.toUtf8();
    for (size_t i = 0; i < count; ++i)
        if (std::strcmp(ids[i], bytes.constData()) == 0)
            return false;
    if (count >= WORKFLOW_MAX_LINKS)
        return false;
    std::strncpy(ids[count], bytes.constData(), WORKFLOW_MAX_NAME - 1);
    ids[count][WORKFLOW_MAX_NAME - 1] = '\0';
    ++count;
    return true;
}

void remove_id(size_t &count, char ids[][WORKFLOW_MAX_NAME], const QString &id)
{
    const QByteArray wanted = id.toUtf8();
    size_t write = 0;
    for (size_t read = 0; read < count; ++read) {
        if (std::strcmp(ids[read], wanted.constData()) != 0) {
            if (write != read)
                std::memcpy(ids[write], ids[read], WORKFLOW_MAX_NAME);
            ++write;
        }
    }
    count = write;
}

} // namespace workflow_scene_utils