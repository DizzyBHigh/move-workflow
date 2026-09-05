#include "workflow-node-identity.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
#include <QRandomGenerator>
#endif

void workflow_node_identity_generate(char *id, size_t size)
{
    if (!id || !size) return;
#ifdef __cplusplus
    const quint64 a = QRandomGenerator::global()->generate64();
    const quint64 b = QRandomGenerator::global()->generate64();
    snprintf(id, size, "node_%016llx%016llx", (unsigned long long)a,
             (unsigned long long)b);
#else
    static unsigned long counter;
    ++counter;
    snprintf(id, size, "node_%lu_%lu", (unsigned long)time(NULL), counter);
#endif
}

static bool id_used(const workflow_manager_t *manager, const char *id)
{
    for (size_t wi = 0; wi < manager->workflow_count; ++wi)
        for (size_t ni = 0; ni < manager->workflows[wi].node_count; ++ni)
            if (strcmp(manager->workflows[wi].nodes[ni].id, id) == 0)
                return true;
    return false;
}

bool workflow_manager_generate_node_id(const workflow_manager_t *manager,
                                       char *id, size_t size)
{
    if (!manager || !id || !size) return false;
    for (unsigned int attempt = 0; attempt < 32; ++attempt) {
        workflow_node_identity_generate(id, size);
        if (id[0] && !id_used(manager, id)) return true;
    }
    id[0] = '\0';
    return false;
}

bool workflow_manager_node_ids_unique(const workflow_manager_t *manager)
{
    if (!manager) return true;
    for (size_t wi = 0; wi < manager->workflow_count; ++wi) {
        const workflow_t *w = &manager->workflows[wi];
        for (size_t ni = 0; ni < w->node_count; ++ni) {
            if (!w->nodes[ni].id[0]) return false;
            for (size_t wj = 0; wj <= wi; ++wj) {
                const workflow_t *other = &manager->workflows[wj];
                const size_t limit = (wj == wi) ? ni : other->node_count;
                for (size_t nj = 0; nj < limit; ++nj)
                    if (strcmp(w->nodes[ni].id, other->nodes[nj].id) == 0)
                        return false;
            }
        }
    }
    return true;
}

bool workflow_node_belongs_to_workflow(const workflow_t *workflow, const char *node_id)
{
    if (!workflow || !node_id || !node_id[0]) return false;
    for (size_t i = 0; i < workflow->node_count; ++i)
        if (strcmp(workflow->nodes[i].id, node_id) == 0) return true;
    return false;
}

static void remap_id(char *id, const char *old_id, const char *new_id)
{
    if (old_id[0] && strcmp(id, old_id) == 0)
        snprintf(id, WORKFLOW_MAX_NAME, "%s", new_id);
}

static void remap_links(workflow_t *w, const char *old_id, const char *new_id)
{
    for (size_t i = 0; i < w->node_count; ++i) {
        workflow_node_t *node = &w->nodes[i];
        char (*groups[])[WORKFLOW_MAX_NAME] = {node->end_node_ids,
            node->simultaneous_node_ids, node->next_node_ids, node->shortcut_node_ids};
        const size_t counts[] = {node->end_node_count, node->simultaneous_node_count,
                                 node->next_node_count, node->shortcut_node_count};
        for (size_t g = 0; g < 4; ++g)
            for (size_t j = 0; j < counts[g]; ++j)
                remap_id(groups[g][j], old_id, new_id);
    }
    for (size_t i = 0; i < w->entry_node_count; ++i)
        remap_id(w->entry_node_ids[i], old_id, new_id);
}

static void remove_empty_links(workflow_t *w)
{
    for (size_t i = 0; i < w->node_count; ++i) {
        workflow_node_t *node = &w->nodes[i];
        char (*groups[])[WORKFLOW_MAX_NAME] = {node->end_node_ids,
            node->simultaneous_node_ids, node->next_node_ids, node->shortcut_node_ids};
        size_t *counts[] = {&node->end_node_count, &node->simultaneous_node_count,
                            &node->next_node_count, &node->shortcut_node_count};
        for (size_t g = 0; g < 4; ++g) {
            size_t write = 0;
            for (size_t j = 0; j < *counts[g]; ++j) {
                if (!groups[g][j][0]) continue;
                if (write != j) memcpy(groups[g][write], groups[g][j], WORKFLOW_MAX_NAME);
                ++write;
            }
            *counts[g] = write;
        }
    }
    size_t write = 0;
    for (size_t i = 0; i < w->entry_node_count; ++i)
        if (w->entry_node_ids[i][0]) {
            if (write != i) memcpy(w->entry_node_ids[write], w->entry_node_ids[i], WORKFLOW_MAX_NAME);
            ++write;
        }
    w->entry_node_count = write;
}

void workflow_manager_repair_node_ids(workflow_manager_t *manager)
{
    if (!manager) return;
    for (size_t wi = 0; wi < manager->workflow_count; ++wi) {
        workflow_t *w = &manager->workflows[wi];
        for (size_t ni = 0; ni < w->node_count; ++ni) {
            workflow_node_t *node = &w->nodes[ni];
            bool duplicate = !node->id[0];
            for (size_t wj = 0; !duplicate && wj <= wi; ++wj) {
                const workflow_t *other = &manager->workflows[wj];
                const size_t limit = (wj == wi) ? ni : other->node_count;
                for (size_t nj = 0; nj < limit; ++nj)
                    if (strcmp(node->id, other->nodes[nj].id) == 0) {
                        duplicate = true;
                        break;
                    }
            }
            if (!duplicate) continue;
            char old_id[WORKFLOW_MAX_NAME];
            snprintf(old_id, sizeof(old_id), "%s", node->id);
            if (workflow_manager_generate_node_id(manager, node->id, sizeof(node->id)))
                remap_links(w, old_id, node->id);
        }
        remove_empty_links(w);
    }
}
