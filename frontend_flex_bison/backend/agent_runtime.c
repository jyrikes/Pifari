#define _POSIX_C_SOURCE 200809L
#include "agent_runtime.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static ArAgentEntry g_agents[AR_MAX_AGENTS];
static size_t g_agent_count = 0;

static ArTaskEntry g_tasks[AR_MAX_TASKS];
static size_t g_task_count = 0;

static ArPolicyRule g_policies[AR_MAX_POLICIES];
static size_t g_policy_count = 0;

static ArContextItem g_context[AR_MAX_CONTEXT_ITEMS];
static size_t g_context_count = 0;

static ArArtifactEntry g_artifacts[AR_MAX_ARTIFACTS];
static size_t g_artifact_count = 0;

static ArTelemetryEvent g_events[AR_MAX_TELEMETRY_EVENTS];
static size_t g_event_count = 0;

static pthread_mutex_t g_ar_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_ar_initialized = 0;
static long long g_ar_epoch_ms = 0;

static long long ar_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
}

static uint64_t ar_fnv1a64(const void *data, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static void ar_json_escape(const char *src, char *dst, size_t dst_cap) {
    if (!dst || dst_cap == 0) return;
    size_t j = 0;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    for (size_t i = 0; src[i] && j + 2 < dst_cap; i++) {
        char c = src[i];
        if (c == '"' || c == '\\') {
            dst[j++] = '\\';
            dst[j++] = c;
        } else if (c == '\n') {
            dst[j++] = '\\';
            dst[j++] = 'n';
        } else if (c == '\r') {
            dst[j++] = '\\';
            dst[j++] = 'r';
        } else if (c == '\t') {
            dst[j++] = '\\';
            dst[j++] = 't';
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

static void record_event_locked(
    const char *event_type,
    const char *agent_id,
    const char *task_id,
    const char *server_id,
    const char *tool_name,
    const char *status,
    long long duration_ms,
    const char *detail
) {
    if (g_event_count >= AR_MAX_TELEMETRY_EVENTS) return;
    ArTelemetryEvent *ev = &g_events[g_event_count];
    ev->seq = g_event_count + 1;
    ev->timestamp_ms = ar_now_ms() - g_ar_epoch_ms;
    snprintf(ev->event_type, sizeof(ev->event_type), "%s", event_type ? event_type : "EVENT");
    snprintf(ev->agent_id, sizeof(ev->agent_id), "%s", agent_id ? agent_id : "");
    snprintf(ev->task_id, sizeof(ev->task_id), "%s", task_id ? task_id : "");
    snprintf(ev->server_id, sizeof(ev->server_id), "%s", server_id ? server_id : "");
    snprintf(ev->tool_name, sizeof(ev->tool_name), "%s", tool_name ? tool_name : "");
    snprintf(ev->status, sizeof(ev->status), "%s", status ? status : "");
    ev->duration_ms = duration_ms;
    snprintf(ev->detail, sizeof(ev->detail), "%s", detail ? detail : "");
    g_event_count++;
}

void agent_runtime_init(void) {
    mcp_gateway_init();
    pthread_mutex_lock(&g_ar_lock);
    if (!g_ar_initialized) {
        memset(g_agents, 0, sizeof(g_agents));
        memset(g_tasks, 0, sizeof(g_tasks));
        memset(g_policies, 0, sizeof(g_policies));
        memset(g_context, 0, sizeof(g_context));
        memset(g_artifacts, 0, sizeof(g_artifacts));
        memset(g_events, 0, sizeof(g_events));
        g_agent_count = 0;
        g_task_count = 0;
        g_policy_count = 0;
        g_context_count = 0;
        g_artifact_count = 0;
        g_event_count = 0;
        g_ar_epoch_ms = ar_now_ms();
        g_ar_initialized = 1;
    }
    pthread_mutex_unlock(&g_ar_lock);
}

void agent_runtime_reset(void) {
    mcp_gateway_shutdown_all();
    pthread_mutex_lock(&g_ar_lock);
    memset(g_agents, 0, sizeof(g_agents));
    memset(g_tasks, 0, sizeof(g_tasks));
    memset(g_policies, 0, sizeof(g_policies));
    memset(g_context, 0, sizeof(g_context));
    memset(g_artifacts, 0, sizeof(g_artifacts));
    memset(g_events, 0, sizeof(g_events));
    g_agent_count = 0;
    g_task_count = 0;
    g_policy_count = 0;
    g_context_count = 0;
    g_artifact_count = 0;
    g_event_count = 0;
    g_ar_epoch_ms = ar_now_ms();
    g_ar_initialized = 1;
    pthread_mutex_unlock(&g_ar_lock);
}

static ArAgentEntry *find_agent_locked(const char *agent_id) {
    if (!agent_id) return NULL;
    for (size_t i = 0; i < g_agent_count; i++) {
        if (strcmp(g_agents[i].agent_id, agent_id) == 0) {
            return &g_agents[i];
        }
    }
    return NULL;
}

static ArTaskEntry *find_task_locked(const char *task_id) {
    if (!task_id) return NULL;
    for (size_t i = 0; i < g_task_count; i++) {
        if (strcmp(g_tasks[i].task_id, task_id) == 0) {
            return &g_tasks[i];
        }
    }
    return NULL;
}

const char *agent_runtime_task_state_str(ArTaskState st) {
    switch (st) {
        case TASK_STATE_PENDING: return "PENDING";
        case TASK_STATE_READY: return "READY";
        case TASK_STATE_RUNNING: return "RUNNING";
        case TASK_STATE_COMPLETED: return "COMPLETED";
        case TASK_STATE_FAILED: return "FAILED";
        case TASK_STATE_CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

int agent_runtime_register_agent(
    const char *agent_id,
    const char *role,
    const char *goal,
    const char *model_adapter,
    int max_concurrency,
    int default_timeout_ms
) {
    agent_runtime_init();
    if (!agent_id || !agent_id[0] || !role || !role[0]) return -1;

    pthread_mutex_lock(&g_ar_lock);
    ArAgentEntry *ag = find_agent_locked(agent_id);
    if (!ag) {
        if (g_agent_count >= AR_MAX_AGENTS) {
            pthread_mutex_unlock(&g_ar_lock);
            return -1;
        }
        ag = &g_agents[g_agent_count++];
        ag->version = 1;
    } else {
        ag->version++;
    }
    snprintf(ag->agent_id, sizeof(ag->agent_id), "%s", agent_id);
    snprintf(ag->role, sizeof(ag->role), "%s", role);
    snprintf(ag->goal, sizeof(ag->goal), "%s", goal ? goal : "");
    snprintf(ag->model_adapter, sizeof(ag->model_adapter), "%s",
             (model_adapter && model_adapter[0]) ? model_adapter : "local-deterministic");
    ag->max_concurrency = max_concurrency > 0 ? max_concurrency : 2;
    ag->default_timeout_ms = default_timeout_ms > 0 ? default_timeout_ms : 2000;

    record_event_locked("AGENT_REGISTERED", agent_id, "", "", "", "OK", 0, ag->role);
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

const ArAgentEntry *agent_runtime_get_agent(const char *agent_id) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    const ArAgentEntry *ag = find_agent_locked(agent_id);
    pthread_mutex_unlock(&g_ar_lock);
    return ag;
}

int agent_runtime_policy_allow(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    int allow_side_effects
) {
    agent_runtime_init();
    if (!agent_id || !server_id || !tool_name) return -1;

    pthread_mutex_lock(&g_ar_lock);
    for (size_t i = 0; i < g_policy_count; i++) {
        if (strcmp(g_policies[i].agent_id, agent_id) == 0 &&
            strcmp(g_policies[i].server_id, server_id) == 0 &&
            strcmp(g_policies[i].tool_name, tool_name) == 0) {
            g_policies[i].allow_side_effects = allow_side_effects ? 1 : 0;
            pthread_mutex_unlock(&g_ar_lock);
            return 0;
        }
    }
    if (g_policy_count >= AR_MAX_POLICIES) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    ArPolicyRule *r = &g_policies[g_policy_count++];
    snprintf(r->agent_id, sizeof(r->agent_id), "%s", agent_id);
    snprintf(r->server_id, sizeof(r->server_id), "%s", server_id);
    snprintf(r->tool_name, sizeof(r->tool_name), "%s", tool_name);
    r->allow_side_effects = allow_side_effects ? 1 : 0;
    record_event_locked("POLICY_GRANTED", agent_id, "", server_id, tool_name, "ALLOW", 0,
                        allow_side_effects ? "side_effects=allowed" : "read_only");
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

static int policy_check_locked(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    int is_side_effect
) {
    if (!agent_id || !server_id || !tool_name) return 0;
    for (size_t i = 0; i < g_policy_count; i++) {
        int agent_match = (strcmp(g_policies[i].agent_id, agent_id) == 0 ||
                           strcmp(g_policies[i].agent_id, "*") == 0);
        int srv_match = (strcmp(g_policies[i].server_id, server_id) == 0 ||
                         strcmp(g_policies[i].server_id, "*") == 0);
        int tool_match = (strcmp(g_policies[i].tool_name, tool_name) == 0 ||
                          strcmp(g_policies[i].tool_name, "*") == 0);
        if (agent_match && srv_match && tool_match) {
            if (is_side_effect && !g_policies[i].allow_side_effects) {
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

int agent_runtime_policy_check(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    int is_side_effect
) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    int allowed = policy_check_locked(agent_id, server_id, tool_name, is_side_effect);
    pthread_mutex_unlock(&g_ar_lock);
    return allowed;
}

int agent_runtime_ctx_put(
    const char *key,
    const char *value,
    const char *producer_agent_id,
    const char *producer_task_id
) {
    agent_runtime_init();
    if (!key || !key[0] || !value) return -1;

    pthread_mutex_lock(&g_ar_lock);
    for (size_t i = 0; i < g_context_count; i++) {
        if (strcmp(g_context[i].key, key) == 0) {
            if (strcmp(g_context[i].value, value) == 0) {
                /* No-op does not increment version or updatedAt */
                pthread_mutex_unlock(&g_ar_lock);
                return g_context[i].version;
            }
            snprintf(g_context[i].value, sizeof(g_context[i].value), "%s", value);
            snprintf(g_context[i].producer_agent_id, sizeof(g_context[i].producer_agent_id),
                     "%s", producer_agent_id ? producer_agent_id : "");
            snprintf(g_context[i].producer_task_id, sizeof(g_context[i].producer_task_id),
                     "%s", producer_task_id ? producer_task_id : "");
            g_context[i].version++;
            g_context[i].updated_at_ms = ar_now_ms() - g_ar_epoch_ms;
            record_event_locked("CTX_UPDATED", producer_agent_id, producer_task_id, "", key, "OK", 0, value);
            int ver = g_context[i].version;
            pthread_mutex_unlock(&g_ar_lock);
            return ver;
        }
    }
    if (g_context_count >= AR_MAX_CONTEXT_ITEMS) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    ArContextItem *it = &g_context[g_context_count++];
    snprintf(it->key, sizeof(it->key), "%s", key);
    snprintf(it->value, sizeof(it->value), "%s", value);
    snprintf(it->producer_agent_id, sizeof(it->producer_agent_id), "%s", producer_agent_id ? producer_agent_id : "");
    snprintf(it->producer_task_id, sizeof(it->producer_task_id), "%s", producer_task_id ? producer_task_id : "");
    it->version = 1;
    it->updated_at_ms = ar_now_ms() - g_ar_epoch_ms;
    record_event_locked("CTX_CREATED", producer_agent_id, producer_task_id, "", key, "OK", 0, value);
    pthread_mutex_unlock(&g_ar_lock);
    return 1;
}

static const char *ctx_get_locked(const char *key) {
    if (!key) return NULL;
    for (size_t i = 0; i < g_context_count; i++) {
        if (strcmp(g_context[i].key, key) == 0) {
            return g_context[i].value;
        }
    }
    return NULL;
}

const char *agent_runtime_ctx_get(const char *key) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    const char *v = ctx_get_locked(key);
    pthread_mutex_unlock(&g_ar_lock);
    return v ? v : "";
}

int agent_runtime_ctx_version(const char *key) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    int ver = 0;
    if (key) {
        for (size_t i = 0; i < g_context_count; i++) {
            if (strcmp(g_context[i].key, key) == 0) {
                ver = g_context[i].version;
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_ar_lock);
    return ver;
}

/* Expand {{ctx:key}} placeholders inside task args_json safely */
static void expand_context_templates_locked(const char *src, char *dst, size_t dst_cap) {
    if (!dst || dst_cap == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t out_i = 0;
    const char *p = src;
    while (*p && out_i + 1 < dst_cap) {
        if (strncmp(p, "{{ctx:", 6) == 0) {
            const char *end = strstr(p + 6, "}}");
            if (end) {
                char key[AR_ID_MAX];
                size_t klen = (size_t)(end - (p + 6));
                if (klen >= sizeof(key)) klen = sizeof(key) - 1;
                memcpy(key, p + 6, klen);
                key[klen] = '\0';
                const char *val = ctx_get_locked(key);
                if (val) {
                    char escaped[2048];
                    ar_json_escape(val, escaped, sizeof(escaped));
                    size_t elen = strlen(escaped);
                    if (out_i + elen >= dst_cap) elen = dst_cap - 1 - out_i;
                    memcpy(dst + out_i, escaped, elen);
                    out_i += elen;
                }
                p = end + 2;
                continue;
            }
        }
        dst[out_i++] = *p++;
    }
    dst[out_i] = '\0';
}

McpCallResult agent_runtime_invoke_mcp(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    const char *args_json,
    int timeout_ms
) {
    agent_runtime_init();
    McpCallResult res;
    memset(&res, 0, sizeof(res));

    int is_side_effect = mcp_gateway_tool_is_side_effect(server_id, tool_name);
    pthread_mutex_lock(&g_ar_lock);
    int allowed = policy_check_locked(agent_id, server_id, tool_name, is_side_effect);
    if (!allowed) {
        record_event_locked("POLICY_DENY", agent_id, "", server_id, tool_name,
                            "DENIED", 0, "ferramenta bloqueada pelo Policy Engine");
        pthread_mutex_unlock(&g_ar_lock);
        res.status = MCP_STATUS_ERR_POLICY_DENIED;
        snprintf(res.error_message, sizeof(res.error_message),
                 "Policy Engine negou acesso do agente '%s' a '%s.%s'",
                 agent_id ? agent_id : "", server_id ? server_id : "", tool_name ? tool_name : "");
        return res;
    }
    pthread_mutex_unlock(&g_ar_lock);

    res = mcp_gateway_call_tool(server_id, tool_name, args_json, timeout_ms, NULL, 0);

    pthread_mutex_lock(&g_ar_lock);
    record_event_locked(
        "MCP_CALL",
        agent_id,
        "",
        server_id,
        tool_name,
        mcp_status_str(res.status),
        res.latency_ms,
        res.status == MCP_STATUS_OK ? res.output_text : res.error_message
    );
    pthread_mutex_unlock(&g_ar_lock);
    return res;
}

int agent_runtime_create_task(
    const char *task_id,
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    const char *args_json,
    const char *output_ctx_key,
    int priority,
    int timeout_ms
) {
    agent_runtime_init();
    if (!task_id || !task_id[0] || !agent_id || !agent_id[0]) return -1;

    pthread_mutex_lock(&g_ar_lock);
    if (!find_agent_locked(agent_id)) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    ArTaskEntry *t = find_task_locked(task_id);
    if (!t) {
        if (g_task_count >= AR_MAX_TASKS) {
            pthread_mutex_unlock(&g_ar_lock);
            return -1;
        }
        t = &g_tasks[g_task_count++];
        memset(t, 0, sizeof(*t));
        t->version = 1;
    } else {
        t->version++;
    }
    snprintf(t->task_id, sizeof(t->task_id), "%s", task_id);
    snprintf(t->agent_id, sizeof(t->agent_id), "%s", agent_id);
    snprintf(t->server_id, sizeof(t->server_id), "%s", server_id ? server_id : "");
    snprintf(t->tool_name, sizeof(t->tool_name), "%s", tool_name ? tool_name : "");
    snprintf(t->args_json, sizeof(t->args_json), "%s", (args_json && args_json[0]) ? args_json : "{}");
    snprintf(t->output_ctx_key, sizeof(t->output_ctx_key), "%s", output_ctx_key ? output_ctx_key : "");
    t->priority = priority;
    t->timeout_ms = timeout_ms > 0 ? timeout_ms : 2000;
    t->state = TASK_STATE_PENDING;
    t->cancel_requested = 0;

    record_event_locked("TASK_CREATED", agent_id, task_id, server_id, tool_name, "PENDING", 0, t->args_json);
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

int agent_runtime_task_add_dep(const char *task_id, const char *dep_task_id) {
    agent_runtime_init();
    if (!task_id || !dep_task_id || strcmp(task_id, dep_task_id) == 0) return -1;

    pthread_mutex_lock(&g_ar_lock);
    ArTaskEntry *t = find_task_locked(task_id);
    ArTaskEntry *dep = find_task_locked(dep_task_id);
    if (!t || !dep || t->dep_count >= AR_MAX_DEPS) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    for (size_t i = 0; i < t->dep_count; i++) {
        if (strcmp(t->deps[i], dep_task_id) == 0) {
            pthread_mutex_unlock(&g_ar_lock);
            return 0;
        }
    }
    snprintf(t->deps[t->dep_count++], AR_ID_MAX, "%s", dep_task_id);
    t->version++;
    record_event_locked("TASK_DEP_ADDED", t->agent_id, task_id, "", dep_task_id, "PENDING", 0, "dependency linked");
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

int agent_runtime_task_cancel(const char *task_id, const char *reason) {
    agent_runtime_init();
    if (!task_id) return -1;

    pthread_mutex_lock(&g_ar_lock);
    ArTaskEntry *t = find_task_locked(task_id);
    if (!t) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    t->cancel_requested = 1;
    if (t->state == TASK_STATE_PENDING || t->state == TASK_STATE_READY) {
        t->state = TASK_STATE_CANCELLED;
        t->version++;
        snprintf(t->error_text, sizeof(t->error_text), "%s", reason ? reason : "cancelled_before_start");
        ArAgentEntry *ag = find_agent_locked(t->agent_id);
        if (ag) {
            ag->cancelled_tasks++;
            ag->version++;
        }
        record_event_locked("TASK_CANCELLED", t->agent_id, t->task_id, t->server_id, t->tool_name,
                            "CANCELLED", 0, t->error_text);
    }
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

const ArTaskEntry *agent_runtime_get_task(const char *task_id) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    const ArTaskEntry *t = find_task_locked(task_id);
    pthread_mutex_unlock(&g_ar_lock);
    return t;
}

static void *task_worker_thread(void *arg) {
    ArTaskEntry *t = (ArTaskEntry *)arg;
    char agent_id[AR_ID_MAX];
    char server_id[AR_ID_MAX];
    char tool_name[AR_ID_MAX];
    char expanded_args[4096];
    int timeout_ms = 2000;

    pthread_mutex_lock(&g_ar_lock);
    if (t->cancel_requested) {
        t->state = TASK_STATE_CANCELLED;
        t->version++;
        snprintf(t->error_text, sizeof(t->error_text), "cancelled_before_exec");
        ArAgentEntry *ag = find_agent_locked(t->agent_id);
        if (ag) {
            if (ag->active_tasks > 0) ag->active_tasks--;
            ag->cancelled_tasks++;
            ag->version++;
        }
        record_event_locked("TASK_CANCELLED", t->agent_id, t->task_id, t->server_id, t->tool_name,
                            "CANCELLED", 0, t->error_text);
        pthread_mutex_unlock(&g_ar_lock);
        return NULL;
    }

    snprintf(agent_id, sizeof(agent_id), "%s", t->agent_id);
    snprintf(server_id, sizeof(server_id), "%s", t->server_id);
    snprintf(tool_name, sizeof(tool_name), "%s", t->tool_name);
    expand_context_templates_locked(t->args_json, expanded_args, sizeof(expanded_args));
    timeout_ms = t->timeout_ms;
    t->attempts++;
    t->start_ms = ar_now_ms();
    record_event_locked("TASK_STARTED", t->agent_id, t->task_id, t->server_id, t->tool_name,
                        "RUNNING", 0, expanded_args);
    pthread_mutex_unlock(&g_ar_lock);

    /* Policy verification before calling MCP Gateway */
    int is_side_effect = mcp_gateway_tool_is_side_effect(server_id, tool_name);
    pthread_mutex_lock(&g_ar_lock);
    int allowed = policy_check_locked(agent_id, server_id, tool_name, is_side_effect);
    if (!allowed) {
        t->duration_ms = ar_now_ms() - t->start_ms;
        t->state = TASK_STATE_FAILED;
        t->version++;
        snprintf(t->error_text, sizeof(t->error_text), "Policy Engine denied %s.%s", server_id, tool_name);
        ArAgentEntry *ag = find_agent_locked(agent_id);
        if (ag) {
            if (ag->active_tasks > 0) ag->active_tasks--;
            ag->failed_tasks++;
            ag->version++;
            snprintf(ag->last_error, sizeof(ag->last_error), "%s", t->error_text);
        }
        record_event_locked("POLICY_DENY", agent_id, t->task_id, server_id, tool_name,
                            "FAILED", t->duration_ms, t->error_text);
        pthread_mutex_unlock(&g_ar_lock);
        return NULL;
    }
    pthread_mutex_unlock(&g_ar_lock);

    McpCallResult res = mcp_gateway_call_tool(
        server_id, tool_name, expanded_args, timeout_ms, &t->cancel_requested, 0
    );

    pthread_mutex_lock(&g_ar_lock);
    t->duration_ms = ar_now_ms() - t->start_ms;
    ArAgentEntry *ag = find_agent_locked(agent_id);
    if (ag && ag->active_tasks > 0) ag->active_tasks--;

    if (res.status == MCP_STATUS_OK) {
        t->state = TASK_STATE_COMPLETED;
        t->version++;
        snprintf(t->result_text, sizeof(t->result_text), "%s", res.output_text);
        if (ag) {
            ag->completed_tasks++;
            ag->version++;
            snprintf(ag->last_result, sizeof(ag->last_result), "%s", res.output_text);
        }
        if (t->output_ctx_key[0]) {
            pthread_mutex_unlock(&g_ar_lock);
            agent_runtime_ctx_put(t->output_ctx_key, res.output_text, agent_id, t->task_id);
            pthread_mutex_lock(&g_ar_lock);
        }
        record_event_locked("TASK_COMPLETED", agent_id, t->task_id, server_id, tool_name,
                            "COMPLETED", t->duration_ms, t->result_text);
    } else if (res.status == MCP_STATUS_ERR_CANCELLED || res.status == MCP_STATUS_ERR_TIMEOUT) {
        t->state = TASK_STATE_CANCELLED;
        t->version++;
        snprintf(t->error_text, sizeof(t->error_text), "%s",
                 res.status == MCP_STATUS_ERR_TIMEOUT ? "cancelled_by_deadline_timeout" : "cancelled_by_request");
        if (ag) {
            ag->cancelled_tasks++;
            ag->version++;
            snprintf(ag->last_error, sizeof(ag->last_error), "%s", t->error_text);
        }
        record_event_locked("TASK_CANCELLED", agent_id, t->task_id, server_id, tool_name,
                            "CANCELLED", t->duration_ms, t->error_text);
    } else {
        t->state = TASK_STATE_FAILED;
        t->version++;
        snprintf(t->error_text, sizeof(t->error_text), "%s", res.error_message);
        if (ag) {
            ag->failed_tasks++;
            ag->version++;
            snprintf(ag->last_error, sizeof(ag->last_error), "%s", t->error_text);
        }
        record_event_locked("TASK_FAILED", agent_id, t->task_id, server_id, tool_name,
                            "FAILED", t->duration_ms, t->error_text);
    }
    pthread_mutex_unlock(&g_ar_lock);
    return NULL;
}

int agent_runtime_task_run_single(const char *task_id) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    ArTaskEntry *t = find_task_locked(task_id);
    if (!t) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    if (t->state == TASK_STATE_CANCELLED) {
        pthread_mutex_unlock(&g_ar_lock);
        return 0;
    }
    t->state = TASK_STATE_RUNNING;
    t->version++;
    ArAgentEntry *ag = find_agent_locked(t->agent_id);
    if (ag) ag->active_tasks++;
    pthread_mutex_unlock(&g_ar_lock);

    task_worker_thread(t);
    return t->state == TASK_STATE_COMPLETED ? 1 : 0;
}

/* Execute all pending tasks in the DAG respecting dependencies and concurrency limits */
int agent_runtime_dag_run(int max_parallel) {
    agent_runtime_init();
    if (max_parallel <= 0) max_parallel = 2;
    if (max_parallel > 16) max_parallel = 16;

    for (;;) {
        ArTaskEntry *batch[16];
        size_t batch_count = 0;
        int any_pending_or_ready = 0;
        int progressed = 0;

        pthread_mutex_lock(&g_ar_lock);
        /* Pass 1: Evaluate dependencies for PENDING tasks */
        for (size_t i = 0; i < g_task_count; i++) {
            ArTaskEntry *t = &g_tasks[i];
            if (t->state != TASK_STATE_PENDING) continue;
            if (t->cancel_requested) {
                t->state = TASK_STATE_CANCELLED;
                t->version++;
                snprintf(t->error_text, sizeof(t->error_text), "cancelled_before_ready");
                record_event_locked("TASK_CANCELLED", t->agent_id, t->task_id, t->server_id, t->tool_name,
                                    "CANCELLED", 0, t->error_text);
                progressed = 1;
                continue;
            }

            int deps_ok = 1;
            int deps_aborted = 0;
            for (size_t d = 0; d < t->dep_count; d++) {
                ArTaskEntry *dt = find_task_locked(t->deps[d]);
                if (!dt || dt->state == TASK_STATE_FAILED || dt->state == TASK_STATE_CANCELLED) {
                    deps_aborted = 1;
                    break;
                }
                if (dt->state != TASK_STATE_COMPLETED) {
                    deps_ok = 0;
                }
            }
            if (deps_aborted) {
                t->state = TASK_STATE_CANCELLED;
                t->version++;
                snprintf(t->error_text, sizeof(t->error_text), "cancelled_due_to_failed_dependency");
                ArAgentEntry *ag = find_agent_locked(t->agent_id);
                if (ag) {
                    ag->cancelled_tasks++;
                    ag->version++;
                }
                record_event_locked("TASK_CANCELLED", t->agent_id, t->task_id, t->server_id, t->tool_name,
                                    "CANCELLED", 0, t->error_text);
                progressed = 1;
            } else if (deps_ok) {
                t->state = TASK_STATE_READY;
                t->version++;
                progressed = 1;
            } else {
                any_pending_or_ready = 1;
            }
        }

        /* Pass 2: Select up to max_parallel READY tasks ordered by priority (descending) */
        while ((int)batch_count < max_parallel) {
            ArTaskEntry *best = NULL;
            for (size_t i = 0; i < g_task_count; i++) {
                ArTaskEntry *t = &g_tasks[i];
                if (t->state != TASK_STATE_READY) continue;
                ArAgentEntry *ag = find_agent_locked(t->agent_id);
                if (ag && ag->active_tasks >= ag->max_concurrency) continue;
                if (!best || t->priority > best->priority) {
                    best = t;
                }
            }
            if (!best) break;
            best->state = TASK_STATE_RUNNING;
            best->version++;
            ArAgentEntry *ag = find_agent_locked(best->agent_id);
            if (ag) ag->active_tasks++;
            batch[batch_count++] = best;
        }

        for (size_t i = 0; i < g_task_count; i++) {
            if (g_tasks[i].state == TASK_STATE_PENDING || g_tasks[i].state == TASK_STATE_READY) {
                any_pending_or_ready = 1;
            }
        }
        pthread_mutex_unlock(&g_ar_lock);

        if (batch_count > 0) {
            pthread_t threads[16];
            for (size_t b = 0; b < batch_count; b++) {
                pthread_create(&threads[b], NULL, task_worker_thread, batch[b]);
            }
            for (size_t b = 0; b < batch_count; b++) {
                pthread_join(threads[b], NULL);
            }
            continue;
        }

        if (!any_pending_or_ready || !progressed) {
            break;
        }
    }

    int completed = 0;
    pthread_mutex_lock(&g_ar_lock);
    for (size_t i = 0; i < g_task_count; i++) {
        if (g_tasks[i].state == TASK_STATE_COMPLETED) completed++;
    }
    pthread_mutex_unlock(&g_ar_lock);
    return completed;
}

int agent_runtime_artifact_save(
    const char *artifact_id,
    const char *file_path,
    const char *content,
    const char *producer_agent_id,
    const char *producer_task_id
) {
    agent_runtime_init();
    if (!artifact_id || !artifact_id[0] || !file_path || !file_path[0] || !content) return -1;

    FILE *fp = fopen(file_path, "wb");
    if (!fp) return -1;
    size_t len = strlen(content);
    if (len > 0 && fwrite(content, 1, len, fp) != len) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    uint64_t hash = ar_fnv1a64(content, len);

    pthread_mutex_lock(&g_ar_lock);
    if (g_artifact_count >= AR_MAX_ARTIFACTS) {
        pthread_mutex_unlock(&g_ar_lock);
        return -1;
    }
    ArArtifactEntry *art = &g_artifacts[g_artifact_count++];
    snprintf(art->artifact_id, sizeof(art->artifact_id), "%s", artifact_id);
    snprintf(art->file_path, sizeof(art->file_path), "%s", file_path);
    snprintf(art->producer_agent_id, sizeof(art->producer_agent_id), "%s", producer_agent_id ? producer_agent_id : "");
    snprintf(art->producer_task_id, sizeof(art->producer_task_id), "%s", producer_task_id ? producer_task_id : "");
    art->byte_size = len;
    snprintf(art->checksum_fnv1a, sizeof(art->checksum_fnv1a), "%016llx", (unsigned long long)hash);
    art->created_at_ms = ar_now_ms() - g_ar_epoch_ms;

    record_event_locked("ARTIFACT_SAVED", producer_agent_id, producer_task_id, "", artifact_id,
                        "OK", 0, file_path);
    pthread_mutex_unlock(&g_ar_lock);
    return 0;
}

const ArArtifactEntry *agent_runtime_get_artifact(const char *artifact_id) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    const ArArtifactEntry *found = NULL;
    if (artifact_id) {
        for (size_t i = 0; i < g_artifact_count; i++) {
            if (strcmp(g_artifacts[i].artifact_id, artifact_id) == 0) {
                found = &g_artifacts[i];
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_ar_lock);
    return found;
}

int agent_runtime_progress_percent(void) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    if (g_task_count == 0) {
        pthread_mutex_unlock(&g_ar_lock);
        return 100;
    }
    size_t done = 0;
    for (size_t i = 0; i < g_task_count; i++) {
        ArTaskState st = g_tasks[i].state;
        if (st == TASK_STATE_COMPLETED || st == TASK_STATE_FAILED || st == TASK_STATE_CANCELLED) {
            done++;
        }
    }
    int pct = (int)((done * 100) / g_task_count);
    pthread_mutex_unlock(&g_ar_lock);
    return pct;
}

size_t agent_runtime_telemetry_count(void) {
    agent_runtime_init();
    pthread_mutex_lock(&g_ar_lock);
    size_t c = g_event_count;
    pthread_mutex_unlock(&g_ar_lock);
    return c;
}

int agent_runtime_export_report_json(const char *file_path) {
    agent_runtime_init();
    if (!file_path || !file_path[0]) return -1;

    FILE *fp = fopen(file_path, "wb");
    if (!fp) return -1;

    int progress = agent_runtime_progress_percent();

    pthread_mutex_lock(&g_ar_lock);
    fprintf(fp, "{\n");
    fprintf(fp, "  \"runtime\": \"Pifari Multi-Agent & MCP Orchestrator\",\n");
    fprintf(fp, "  \"mcp_protocol\": \"%s\",\n", MCP_PROTOCOL_VERSION);
    fprintf(fp, "  \"progress_percent\": %d,\n", progress);
    fprintf(fp, "  \"agent_count\": %zu,\n", g_agent_count);
    fprintf(fp, "  \"task_count\": %zu,\n", g_task_count);
    fprintf(fp, "  \"artifact_count\": %zu,\n", g_artifact_count);
    fprintf(fp, "  \"telemetry_event_count\": %zu,\n", g_event_count);

    /* Agents */
    fprintf(fp, "  \"agents\": [\n");
    for (size_t i = 0; i < g_agent_count; i++) {
        ArAgentEntry *a = &g_agents[i];
        char esc_role[256], esc_goal[512], esc_res[2048], esc_err[1024];
        ar_json_escape(a->role, esc_role, sizeof(esc_role));
        ar_json_escape(a->goal, esc_goal, sizeof(esc_goal));
        ar_json_escape(a->last_result, esc_res, sizeof(esc_res));
        ar_json_escape(a->last_error, esc_err, sizeof(esc_err));
        fprintf(fp,
                "    {\"agent_id\":\"%s\",\"role\":\"%s\",\"goal\":\"%s\",\"adapter\":\"%s\","
                "\"version\":%d,\"completed\":%d,\"failed\":%d,\"cancelled\":%d,"
                "\"last_result\":\"%s\",\"last_error\":\"%s\"}%s\n",
                a->agent_id, esc_role, esc_goal, a->model_adapter,
                a->version, a->completed_tasks, a->failed_tasks, a->cancelled_tasks,
                esc_res, esc_err, (i + 1 < g_agent_count) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Tasks */
    fprintf(fp, "  \"tasks\": [\n");
    for (size_t i = 0; i < g_task_count; i++) {
        ArTaskEntry *t = &g_tasks[i];
        char esc_res[2048], esc_err[1024];
        ar_json_escape(t->result_text, esc_res, sizeof(esc_res));
        ar_json_escape(t->error_text, esc_err, sizeof(esc_err));
        fprintf(fp,
                "    {\"task_id\":\"%s\",\"agent_id\":\"%s\",\"server_id\":\"%s\",\"tool_name\":\"%s\","
                "\"state\":\"%s\",\"version\":%d,\"priority\":%d,\"duration_ms\":%lld,"
                "\"attempts\":%d,\"result\":\"%s\",\"error\":\"%s\"}%s\n",
                t->task_id, t->agent_id, t->server_id, t->tool_name,
                agent_runtime_task_state_str(t->state), t->version, t->priority, t->duration_ms,
                t->attempts, esc_res, esc_err, (i + 1 < g_task_count) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Context Store */
    fprintf(fp, "  \"context_store\": [\n");
    for (size_t i = 0; i < g_context_count; i++) {
        ArContextItem *c = &g_context[i];
        char esc_val[2048];
        ar_json_escape(c->value, esc_val, sizeof(esc_val));
        fprintf(fp,
                "    {\"key\":\"%s\",\"value\":\"%s\",\"producer_agent\":\"%s\",\"producer_task\":\"%s\","
                "\"version\":%d,\"updated_at_ms\":%lld}%s\n",
                c->key, esc_val, c->producer_agent_id, c->producer_task_id,
                c->version, c->updated_at_ms, (i + 1 < g_context_count) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Artifacts */
    fprintf(fp, "  \"artifacts\": [\n");
    for (size_t i = 0; i < g_artifact_count; i++) {
        ArArtifactEntry *art = &g_artifacts[i];
        char esc_path[1024];
        ar_json_escape(art->file_path, esc_path, sizeof(esc_path));
        fprintf(fp,
                "    {\"artifact_id\":\"%s\",\"file_path\":\"%s\",\"producer_agent\":\"%s\","
                "\"producer_task\":\"%s\",\"byte_size\":%zu,\"checksum_fnv1a\":\"%s\",\"created_at_ms\":%lld}%s\n",
                art->artifact_id, esc_path, art->producer_agent_id, art->producer_task_id,
                art->byte_size, art->checksum_fnv1a, art->created_at_ms,
                (i + 1 < g_artifact_count) ? "," : "");
    }
    fprintf(fp, "  ],\n");

    /* Telemetry Events */
    fprintf(fp, "  \"telemetry\": [\n");
    for (size_t i = 0; i < g_event_count; i++) {
        ArTelemetryEvent *ev = &g_events[i];
        char esc_det[1024];
        ar_json_escape(ev->detail, esc_det, sizeof(esc_det));
        fprintf(fp,
                "    {\"seq\":%zu,\"timestamp_ms\":%lld,\"event_type\":\"%s\",\"agent_id\":\"%s\","
                "\"task_id\":\"%s\",\"server_id\":\"%s\",\"tool_name\":\"%s\",\"status\":\"%s\","
                "\"duration_ms\":%lld,\"detail\":\"%s\"}%s\n",
                ev->seq, ev->timestamp_ms, ev->event_type, ev->agent_id,
                ev->task_id, ev->server_id, ev->tool_name, ev->status,
                ev->duration_ms, esc_det, (i + 1 < g_event_count) ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    pthread_mutex_unlock(&g_ar_lock);
    fclose(fp);
    return 0;
}
