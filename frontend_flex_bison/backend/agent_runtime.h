#ifndef PIFARI_AGENT_RUNTIME_H
#define PIFARI_AGENT_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include "mcp_gateway.h"

#define AR_MAX_AGENTS 32
#define AR_MAX_TASKS 64
#define AR_MAX_DEPS 8
#define AR_MAX_POLICIES 128
#define AR_MAX_CONTEXT_ITEMS 128
#define AR_MAX_ARTIFACTS 64
#define AR_MAX_TELEMETRY_EVENTS 512
#define AR_ID_MAX 64

typedef enum {
    TASK_STATE_PENDING = 0,
    TASK_STATE_READY = 1,
    TASK_STATE_RUNNING = 2,
    TASK_STATE_COMPLETED = 3,
    TASK_STATE_FAILED = 4,
    TASK_STATE_CANCELLED = 5
} ArTaskState;

typedef struct {
    char agent_id[AR_ID_MAX];
    char role[128];
    char goal[256];
    char model_adapter[64];
    int max_concurrency;
    int default_timeout_ms;
    int active_tasks;
    int completed_tasks;
    int failed_tasks;
    int cancelled_tasks;
    int version;
    char last_result[1024];
    char last_error[512];
} ArAgentEntry;

typedef struct {
    char task_id[AR_ID_MAX];
    char agent_id[AR_ID_MAX];
    char server_id[AR_ID_MAX];
    char tool_name[AR_ID_MAX];
    char args_json[2048];
    char output_ctx_key[AR_ID_MAX];
    int priority;
    int timeout_ms;
    ArTaskState state;
    int version;
    char deps[AR_MAX_DEPS][AR_ID_MAX];
    size_t dep_count;
    volatile int cancel_requested;
    int attempts;
    long long start_ms;
    long long duration_ms;
    char result_text[4096];
    char error_text[512];
} ArTaskEntry;

typedef struct {
    char agent_id[AR_ID_MAX];
    char server_id[AR_ID_MAX];
    char tool_name[AR_ID_MAX];
    int allow_side_effects;
} ArPolicyRule;

typedef struct {
    char key[AR_ID_MAX];
    char value[4096];
    char producer_agent_id[AR_ID_MAX];
    char producer_task_id[AR_ID_MAX];
    int version;
    long long updated_at_ms;
} ArContextItem;

typedef struct {
    char artifact_id[AR_ID_MAX];
    char file_path[512];
    char producer_agent_id[AR_ID_MAX];
    char producer_task_id[AR_ID_MAX];
    size_t byte_size;
    char checksum_fnv1a[24];
    long long created_at_ms;
} ArArtifactEntry;

typedef struct {
    size_t seq;
    long long timestamp_ms;
    char event_type[48];
    char agent_id[AR_ID_MAX];
    char task_id[AR_ID_MAX];
    char server_id[AR_ID_MAX];
    char tool_name[AR_ID_MAX];
    char status[32];
    long long duration_ms;
    char detail[512];
} ArTelemetryEvent;

void agent_runtime_init(void);
void agent_runtime_reset(void);

/* 1. Agent Manager */
int agent_runtime_register_agent(
    const char *agent_id,
    const char *role,
    const char *goal,
    const char *model_adapter,
    int max_concurrency,
    int default_timeout_ms
);
const ArAgentEntry *agent_runtime_get_agent(const char *agent_id);

/* 2. Policy Engine */
int agent_runtime_policy_allow(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    int allow_side_effects
);
int agent_runtime_policy_check(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    int is_side_effect
);

/* 3. Context Store */
int agent_runtime_ctx_put(
    const char *key,
    const char *value,
    const char *producer_agent_id,
    const char *producer_task_id
);
const char *agent_runtime_ctx_get(const char *key);
int agent_runtime_ctx_version(const char *key);

/* 4. Task Engine & DAG Coordinator */
int agent_runtime_create_task(
    const char *task_id,
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    const char *args_json,
    const char *output_ctx_key,
    int priority,
    int timeout_ms
);
int agent_runtime_task_add_dep(const char *task_id, const char *dep_task_id);
int agent_runtime_task_cancel(const char *task_id, const char *reason);
int agent_runtime_task_run_single(const char *task_id);
int agent_runtime_dag_run(int max_parallel);
const ArTaskEntry *agent_runtime_get_task(const char *task_id);
const char *agent_runtime_task_state_str(ArTaskState st);

/* 5. Direct Policy-Checked MCP Invocation */
McpCallResult agent_runtime_invoke_mcp(
    const char *agent_id,
    const char *server_id,
    const char *tool_name,
    const char *args_json,
    int timeout_ms
);

/* 6. Artifact Manager */
int agent_runtime_artifact_save(
    const char *artifact_id,
    const char *file_path,
    const char *content,
    const char *producer_agent_id,
    const char *producer_task_id
);
const ArArtifactEntry *agent_runtime_get_artifact(const char *artifact_id);

/* 7. Telemetry Engine */
int agent_runtime_progress_percent(void);
size_t agent_runtime_telemetry_count(void);
int agent_runtime_export_report_json(const char *file_path);

#endif
