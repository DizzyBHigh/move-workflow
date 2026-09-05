#include "workflow-scene-relationship.h"

#include <cstring>

namespace workflow_scene_relationship {

static bool contains(size_t count, const char ids[][WORKFLOW_MAX_NAME], const char *id)
{
    if (!id || !*id)
        return false;
    for (size_t i = 0; i < count; ++i)
        if (std::strcmp(ids[i], id) == 0)
            return true;
    return false;
}

static bool append(size_t &count, char ids[][WORKFLOW_MAX_NAME], const char *id)
{
    if (!id || !*id || count >= WORKFLOW_MAX_LINKS || contains(count, ids, id))
        return false;
    std::snprintf(ids[count], WORKFLOW_MAX_NAME, "%s", id);
    ++count;
    return true;
}

static void erase(size_t &count, char ids[][WORKFLOW_MAX_NAME], const char *id)
{
    if (!id)
        return;
    for (size_t i = 0; i < count; ++i) {
        if (std::strcmp(ids[i], id) != 0)
            continue;
        for (size_t j = i + 1; j < count; ++j)
            std::memcpy(ids[j - 1], ids[j], WORKFLOW_MAX_NAME);
        --count;
        return;
    }
}

static bool *unused(void) { return nullptr; }

bool add(workflow_node_t *source, const char *target_id, relationship_type type)
{
    if (!source || !target_id || !*target_id)
        return false;
    switch (type) {
    case relationship_type::shortcut:
        return append(source->shortcut_node_count, source->shortcut_node_ids, target_id);
    case relationship_type::simultaneous:
        return append(source->simultaneous_node_count, source->simultaneous_node_ids, target_id);
    case relationship_type::next:
        return append(source->next_node_count, source->next_node_ids, target_id);
    }
    return false;
}

void remove(workflow_node_t *source, const char *target_id, relationship_type type)
{
    if (!source)
        return;
    switch (type) {
    case relationship_type::shortcut:
        erase(source->shortcut_node_count, source->shortcut_node_ids, target_id);
        break;
    case relationship_type::simultaneous:
        erase(source->simultaneous_node_count, source->simultaneous_node_ids, target_id);
        break;
    case relationship_type::next:
        erase(source->next_node_count, source->next_node_ids, target_id);
        break;
    }
}

bool has(const workflow_node_t *source, const char *target_id, relationship_type type)
{
    if (!source)
        return false;
    switch (type) {
    case relationship_type::shortcut:
        return contains(source->shortcut_node_count, source->shortcut_node_ids, target_id);
    case relationship_type::simultaneous:
        return contains(source->simultaneous_node_count, source->simultaneous_node_ids, target_id);
    case relationship_type::next:
        return contains(source->next_node_count, source->next_node_ids, target_id);
    }
    return false;
}

} // namespace workflow_scene_relationship
