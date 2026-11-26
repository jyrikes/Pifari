#ifndef PIFARI_MCP_GATEWAY_H
#define PIFARI_MCP_GATEWAY_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

#define MCP_PROTOCOL_VERSION "2025-03-26"
#define MCP_MAX_SERVERS 16
#define MCP_MAX_TOOLS_PER_SERVER 32
#define MCP_MAX_RESOURCES_PER_SERVER 32
#define MCP_MAX_PAYLOAD_BYTES 262144
#define MCP_ID_MAX 64
#define MCP_ENDPOINT_MAX 512

typedef enum { MCP_TRANSPORT_STDIO = 0, MCP_TRANSPORT_HTTP = 1 } McpTransportType;

typedef enum {
    MCP_STATUS_OK = 0,
    MCP_STATUS_ERR_INVALID_ARGS = -1,
    MCP_STATUS_ERR_NOT_FOUND = -2,
    MCP_STATUS_ERR_TRANSPORT = -3,
    MCP_STATUS_ERR_TIMEOUT = -4,
    MCP_STATUS_ERR_CANCELLED = -5,
    MCP_STATUS_ERR_PROTOCOL = -6,
    MCP_STATUS_ERR_POLICY_DENIED = -7,
    MCP_STATUS_ERR_CONCURRENCY_LIMIT = -8,
    MCP_STATUS_ERR_PAYLOAD_TOO_LARGE = -9,
    MCP_STATUS_ERR_DUPLICATE_CALL = -10
} McpStatus;

typedef struct {
    char name[MCP_ID_MAX];
    char description[256];
    int is_side_effect; /* 1 if mutating/non-idempotent, 0 if read-only */
} McpToolInfo;

typedef struct {
    char uri[256];
    char name[MCP_ID_MAX];
    char mime_type[64];
} McpResourceInfo;

typedef struct {
    char server_id[MCP_ID_MAX];
    McpTransportType transport;
    char endpoint[MCP_ENDPOINT_MAX]; /* command for stdio, host:port/path for http */
    char auth_env_var[MCP_ID_MAX]; /* Name of env var only; secret is NEVER stored in struct or logs
                                    */
    char session_id[128];          /* HTTP Mcp-Session-Id if provided by server */
    char negotiated_version[32];
    int connected;
    int capabilities_tools;
    int capabilities_resources;
    int capabilities_prompts;
    int default_timeout_ms;
    int max_concurrent;
    int active_calls;

    /* stdio process state */
    pid_t child_pid;
    int stdin_fd;
    int stdout_fd;

    /* discovered catalog */
    McpToolInfo tools[MCP_MAX_TOOLS_PER_SERVER];
    size_t tool_count;
    McpResourceInfo resources[MCP_MAX_RESOURCES_PER_SERVER];
    size_t resource_count;

    /* idempotency / duplicate call guard for side-effecting calls */
    uint64_t recent_side_effect_hashes[32];
    size_t recent_hash_count;

    pthread_mutex_t lock;
} McpServerEntry;

typedef struct {
    McpStatus status;
    int rpc_error_code;
    long long latency_ms;
    char output_text[4096];
    char error_message[512];
} McpCallResult;

void mcp_gateway_init(void);
void mcp_gateway_shutdown_all(void);

McpStatus mcp_gateway_register_server(const char *server_id, McpTransportType transport,
                                      const char *endpoint, const char *auth_env_var,
                                      int default_timeout_ms, int max_concurrent);

McpStatus mcp_gateway_connect(const char *server_id);
McpStatus mcp_gateway_disconnect(const char *server_id);

McpStatus mcp_gateway_discover_tools(const char *server_id, size_t *out_count);
McpStatus mcp_gateway_discover_resources(const char *server_id, size_t *out_count);

int mcp_gateway_tool_is_side_effect(const char *server_id, const char *tool_name);

McpCallResult mcp_gateway_call_tool(const char *server_id, const char *tool_name,
                                    const char *args_json, int timeout_ms,
                                    volatile int *cancel_flag, uint64_t idempotency_key);

McpCallResult mcp_gateway_read_resource(const char *server_id, const char *uri, int timeout_ms);

const McpServerEntry *mcp_gateway_get_server(const char *server_id);
const char *mcp_status_str(McpStatus st);

#endif
