#include "optimizer.h"

#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    const PifariExprGraph *graph;
    const double *input;
    double *output;
    size_t begin;
    size_t end;
    int error;
} MapTask;

void pifari_graph_init(PifariExprGraph *graph) {
    if (!graph)
        return;
    *graph = (PifariExprGraph){0};
    graph->root = -1;
}

void pifari_graph_destroy(PifariExprGraph *graph) {
    if (!graph)
        return;
    free(graph->nodes);
    *graph = (PifariExprGraph){0};
    graph->root = -1;
}

int pifari_graph_add(PifariExprGraph *graph, PifariExprOp op, int left, int right,
                     double constant) {
    if (!graph || graph->count > (size_t)INT32_MAX)
        return -1;

    if (graph->count == graph->capacity) {
        size_t capacity = graph->capacity ? graph->capacity * 2 : 16;
        if (capacity < graph->capacity || capacity > SIZE_MAX / sizeof(*graph->nodes))
            return -1;
        void *memory = realloc(graph->nodes, capacity * sizeof(*graph->nodes));
        if (!memory)
            return -1;
        graph->nodes = memory;
        graph->capacity = capacity;
    }

    if (op < PEX_INPUT || op > PEX_MAX0)
        return -1;
    if ((op == PEX_ALIAS || op == PEX_NEG || op == PEX_MAX0) &&
        (left < 0 || (size_t)left >= graph->count))
        return -1;
    if (op >= PEX_ADD && op <= PEX_DIV &&
        (left < 0 || right < 0 || (size_t)left >= graph->count || (size_t)right >= graph->count))
        return -1;

    graph->nodes[graph->count] = (PifariExprNode){op, left, right, constant};
    return (int)graph->count++;
}

int pifari_graph_compose(const PifariExprGraph *first, const PifariExprGraph *second,
                         PifariExprGraph *out) {
    if (!first || !second || !out || !first->nodes || !second->nodes || first->root < 0 ||
        second->root < 0 || (size_t)first->root >= first->count ||
        (size_t)second->root >= second->count)
        return -1;
    pifari_graph_init(out);
    int *first_map = malloc(first->count * sizeof(*first_map));
    int *second_map = malloc(second->count * sizeof(*second_map));
    if (!first_map || !second_map) {
        free(first_map);
        free(second_map);
        return -1;
    }
    int ok = 1;
    for (size_t i = 0; i < first->count && ok; ++i) {
        PifariExprNode n = first->nodes[i];
        if (n.op == PEX_INPUT)
            first_map[i] = pifari_graph_add(out, PEX_INPUT, -1, -1, 0.0);
        else {
            int l = n.left >= 0 ? first_map[n.left] : -1;
            int r = n.right >= 0 ? first_map[n.right] : -1;
            first_map[i] = pifari_graph_add(out, n.op, l, r, n.constant);
        }
        if (first_map[i] < 0)
            ok = 0;
    }
    int composed_input = first_map[first->root];
    for (size_t i = 0; i < second->count && ok; ++i) {
        PifariExprNode n = second->nodes[i];
        if (n.op == PEX_INPUT)
            second_map[i] = pifari_graph_add(out, PEX_ALIAS, composed_input, -1, 0.0);
        else {
            int l = n.left >= 0 ? second_map[n.left] : -1;
            int r = n.right >= 0 ? second_map[n.right] : -1;
            second_map[i] = pifari_graph_add(out, n.op, l, r, n.constant);
        }
        if (second_map[i] < 0)
            ok = 0;
    }
    if (ok) {
        out->root = second_map[second->root];
        ok = pifari_graph_optimize(out) == 0;
    }
    free(first_map);
    free(second_map);
    if (!ok) {
        pifari_graph_destroy(out);
        return -1;
    }
    return 0;
}

static int constant_value(const PifariExprGraph *graph, int id, double *value) {
    if (id < 0 || (size_t)id >= graph->count || graph->nodes[id].op != PEX_CONST)
        return 0;
    *value = graph->nodes[id].constant;
    return 1;
}

int pifari_graph_optimize(PifariExprGraph *graph) {
    if (!graph || graph->root < 0 || (size_t)graph->root >= graph->count)
        return -1;

    for (size_t i = 0; i < graph->count; ++i) {
        PifariExprNode *node = &graph->nodes[i];
        double left_value = 0.0, right_value = 0.0, result = 0.0;
        int left_constant = constant_value(graph, node->left, &left_value);
        int right_constant = constant_value(graph, node->right, &right_value);

        if (node->op == PEX_NEG && left_constant) {
            result = -left_value;
        } else if (node->op == PEX_MAX0 && left_constant) {
            result = left_value > 0.0 ? left_value : 0.0;
        } else if (node->op >= PEX_ADD && node->op <= PEX_DIV && left_constant && right_constant) {
            switch (node->op) {
                case PEX_ADD:
                    result = left_value + right_value;
                    break;
                case PEX_SUB:
                    result = left_value - right_value;
                    break;
                case PEX_MUL:
                    result = left_value * right_value;
                    break;
                case PEX_DIV:
                    if (right_value == 0.0)
                        continue;
                    result = left_value / right_value;
                    break;
                default:
                    continue;
            }
        } else {
            continue;
        }

        if (isfinite(result)) {
            node->op = PEX_CONST;
            node->constant = result;
            node->left = -1;
            node->right = -1;
        }
    }
    return 0;
}

/* Linear-scan liveness: temporaries are reused only after their last consumer.
 * A node's slot never aliases either operand during that node's evaluation. */
static int build_workspace_plan(const PifariExprGraph *graph, size_t **slots_out,
                                size_t *slot_count_out) {
    if (!graph || !graph->nodes || !graph->count || graph->root < 0 ||
        (size_t)graph->root >= graph->count || graph->count > SIZE_MAX / sizeof(size_t))
        return -1;

    size_t *last_use = malloc(graph->count * sizeof(*last_use));
    size_t *slots = malloc(graph->count * sizeof(*slots));
    size_t *slot_end = malloc(graph->count * sizeof(*slot_end));
    if (!last_use || !slots || !slot_end) {
        free(last_use);
        free(slots);
        free(slot_end);
        return -1;
    }

    for (size_t i = 0; i < graph->count; ++i)
        last_use[i] = i;

    for (size_t i = 0; i < graph->count; ++i) {
        const PifariExprNode *node = &graph->nodes[i];
        if (node->op == PEX_ALIAS || node->op == PEX_NEG || node->op == PEX_MAX0 ||
            (node->op >= PEX_ADD && node->op <= PEX_DIV)) {
            if (node->left < 0 || (size_t)node->left >= i)
                goto invalid_graph;
            if (last_use[node->left] < i)
                last_use[node->left] = i;
        }
        if (node->op >= PEX_ADD && node->op <= PEX_DIV) {
            if (node->right < 0 || (size_t)node->right >= i)
                goto invalid_graph;
            if (last_use[node->right] < i)
                last_use[node->right] = i;
        }
    }
    last_use[graph->root] = graph->count;

    size_t slot_count = 0;
    for (size_t i = 0; i < graph->count; ++i) {
        size_t chosen = slot_count;
        for (size_t candidate = 0; candidate < slot_count; ++candidate) {
            if (slot_end[candidate] < i) {
                chosen = candidate;
                break;
            }
        }
        if (chosen == slot_count)
            ++slot_count;
        slots[i] = chosen;
        slot_end[chosen] = last_use[i];
    }

    free(last_use);
    free(slot_end);
    *slots_out = slots;
    *slot_count_out = slot_count;
    return 0;

invalid_graph:
    free(last_use);
    free(slots);
    free(slot_end);
    return -1;
}

size_t pifari_graph_workspace_slots(const PifariExprGraph *graph) {
    size_t *slots = NULL, count = 0;
    if (build_workspace_plan(graph, &slots, &count) != 0)
        return 0;
    free(slots);
    return count;
}

static int eval_topological(const PifariExprGraph *graph, double input, const size_t *slots,
                            double *workspace, double *output) {
    for (size_t i = 0; i < graph->count; ++i) {
        const PifariExprNode *node = &graph->nodes[i];
        double left = 0.0, right = 0.0, value = 0.0;

        if (node->op == PEX_INPUT) {
            value = input;
        } else if (node->op == PEX_CONST) {
            value = node->constant;
        } else {
            left = workspace[slots[node->left]];
            if (node->op == PEX_ALIAS)
                value = left;
            else if (node->op == PEX_NEG)
                value = -left;
            else if (node->op == PEX_MAX0)
                value = left > 0.0 ? left : 0.0;
            else {
                right = workspace[slots[node->right]];
                switch (node->op) {
                    case PEX_ADD:
                        value = left + right;
                        break;
                    case PEX_SUB:
                        value = left - right;
                        break;
                    case PEX_MUL:
                        value = left * right;
                        break;
                    case PEX_DIV:
                        if (right == 0.0)
                            return -2;
                        value = left / right;
                        break;
                    default:
                        return -1;
                }
            }
        }
        workspace[slots[i]] = value;
    }

    *output = workspace[slots[graph->root]];
    return 0;
}

int pifari_graph_eval(const PifariExprGraph *graph, double input, double *output) {
    if (!output)
        return -1;
    size_t *slots = NULL, slot_count = 0;
    if (build_workspace_plan(graph, &slots, &slot_count) != 0)
        return -1;
    double *workspace = malloc(slot_count * sizeof(*workspace));
    if (!workspace) {
        free(slots);
        return -1;
    }
    int status = eval_topological(graph, input, slots, workspace, output);
    free(workspace);
    free(slots);
    return status;
}

static int map_range(MapTask *task) {
    size_t *slots = NULL, slot_count = 0;
    if (build_workspace_plan(task->graph, &slots, &slot_count) != 0)
        return -1;
    double *workspace = malloc(slot_count * sizeof(*workspace));
    if (!workspace) {
        free(slots);
        return -1;
    }

    int status = 0;
    for (size_t i = task->begin; i < task->end; ++i) {
        status = eval_topological(task->graph, task->input[i], slots, workspace, &task->output[i]);
        if (status != 0)
            break;
    }
    free(workspace);
    free(slots);
    return status;
}

int pifari_graph_map(const PifariExprGraph *graph, const double *input, double *output,
                     size_t count) {
    if (!graph || graph->root < 0 || (size_t)graph->root >= graph->count ||
        (count && (!input || !output)))
        return -1;
    if (!count)
        return 0;
    MapTask task = {graph, input, output, 0, count, 0};
    return map_range(&task);
}

static void *map_worker(void *opaque) {
    MapTask *task = opaque;
    task->error = map_range(task);
    return NULL;
}

int pifari_graph_map_parallel(const PifariExprGraph *graph, const double *input, double *output,
                              size_t count, unsigned workers) {
    if (!graph || graph->root < 0 || (size_t)graph->root >= graph->count ||
        (count && (!input || !output)))
        return -1;
    if (!count)
        return 0;
    if (workers < 2 || count < 16384)
        return pifari_graph_map(graph, input, output, count);
    if (workers > 32)
        workers = 32;
    if ((size_t)workers > count)
        workers = (unsigned)count;

    pthread_t threads[32];
    MapTask tasks[32];
    unsigned created = 0;
    size_t base = count / workers, extra = count % workers, position = 0;

    for (unsigned i = 0; i < workers; ++i) {
        size_t length = base + (i < extra);
        tasks[i] = (MapTask){graph, input, output, position, position + length, 0};
        position += length;
        if (pthread_create(&threads[i], NULL, map_worker, &tasks[i]) != 0)
            break;
        ++created;
    }

    if (created != workers) {
        for (unsigned i = 0; i < created; ++i)
            pthread_join(threads[i], NULL);
        return pifari_graph_map(graph, input, output, count);
    }

    int status = 0;
    for (unsigned i = 0; i < workers; ++i) {
        pthread_join(threads[i], NULL);
        if (tasks[i].error && !status)
            status = tasks[i].error;
    }
    return status;
}
