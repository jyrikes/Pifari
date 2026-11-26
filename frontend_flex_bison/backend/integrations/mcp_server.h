#ifndef PIFARI_MCP_SERVER_H
#define PIFARI_MCP_SERVER_H

#include <stddef.h>

#define PIFARI_MCP_SRV_MAX_TOOLS 32
#define PIFARI_MCP_SRV_MAX_PARAMS 8
#define PIFARI_MCP_SRV_MAX_RESOURCES 32
#define PIFARI_MCP_SRV_ID_MAX 64

typedef char *(*PifariToolInvokeFn)(const char *fn_name, size_t argc, const char **argv,
                                    int *out_is_error);

typedef struct {
    char tool_name[PIFARI_MCP_SRV_ID_MAX];
    char description[256];
    char function_name[PIFARI_MCP_SRV_ID_MAX];
    char param_names[PIFARI_MCP_SRV_MAX_PARAMS][PIFARI_MCP_SRV_ID_MAX];
    size_t param_count;
    int read_only;
} PifariMcpServerTool;

typedef struct {
    char uri[256];
    char name[PIFARI_MCP_SRV_ID_MAX];
    char mime_type[64];
    char content[4096];
} PifariMcpServerResource;

void pifari_mcp_server_init(const char *server_name, const char *version);

int pifari_mcp_server_register_tool(const char *tool_name, const char *description,
                                    const char *function_name, size_t param_count,
                                    const char **param_names, int read_only);

int pifari_mcp_server_register_resource(const char *uri, const char *name, const char *mime_type,
                                        const char *content);

/* Process a single JSON-RPC 2.0 line. Returns 1 if a response was written to out_buf, 0 for
 * notifications. */
int pifari_mcp_server_handle_line(const char *json_line, PifariToolInvokeFn invoke_cb,
                                  char *out_buf, size_t out_cap);

int pifari_mcp_server_run_stdio(PifariToolInvokeFn invoke_cb);
int pifari_mcp_server_run_http(int port, PifariToolInvokeFn invoke_cb);

#endif
