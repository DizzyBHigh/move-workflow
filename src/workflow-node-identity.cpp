#include "workflow-node-identity.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
#include <QRandomGenerator>
#endif

void workflow_node_identity_generate(char *id, size_t size)
{
    if (!id || !size)
        return;
#ifdef __cplusplus
    const quint64 a = QRandomGenerator::global()->generate64();
    const quint64 b = QRandomGenerator::global()->generate64();
    snprintf(id, size, "node_%016llx%016llx",
             (unsigned long long)a, (unsigned long long)b);
#else
    static unsigned long counter;
    ++counter;
    snprintf(id, size, "node_%lu_%lu", (unsigned long)time(NULL), counter);
#endif
}

bool workflow_manager_node_ids_unique(const workflow_manager_t *manager)
{
    if (!manager)
        return true;
    for (size_t wi = 0; wi < manager->workflow_count; ++wi) {
        const workflow_t *w = &manager->workflows[wi];
        for (size_t ni = 0; ni < w->node_count; ++ni) {
            if (!w->nodes[ni].id[0])
                return false;
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
    if (!workflow || !node_id || !node_id[0])
        return false;
    for (size_t i = 0; i < workflow->node_count; ++i)
        if (strcmp(workflow->nodes[i].id, node_id) == 0)
            return true;
    return false;
}

void workflow_manager_repair_node_ids(workflow_manager_t *manager)
{
    if (!manager)
        return;
    for (size_t wi = 0; wi < manager->workflow_count; ++wi) {
        workflow_t *w = &manager->workflows[wi];
        for (size_t ni = 0; ni < w->node_count; ++ni) {
            workflow_node_t *node = &w->nodes[ni];
            bool duplicate = !node->id[0];
            for (size_t wj = 0; !duplicate && wj <= wi; ++wj) {
                workflow_t *other = &manager->workflows[wj];
                size_t limit = (wj == wi) ? ni : other->node_count;
                for (size_t nj = 0; nj < limit; ++nj)
                    if (strcmp(node->id, other->nodes[nj].id) == 0) {
                        duplicate = true;
                        break;
                    }
            }
            if (duplicate)
                workflow_node_identity_generate(node->id, sizeof(node->id));
        }
    }
}
