#define _POSIX_C_SOURCE 200809L
#include "mcp_server.h"
#include "mcp_gateway.h"

#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static char g_srv_name[PIFARI_MCP_SRV_ID_MAX] = "pifari-mcp-server";
static char g_srv_version[32] = "1.0.0";
static PifariMcpServerTool g_srv_tools[PIFARI_MCP_SRV_MAX_TOOLS];
static size_t g_srv_tool_count = 0;
static PifariMcpServerResource g_srv_resources[PIFARI_MCP_SRV_MAX_RESOURCES];
static size_t g_srv_resource_count = 0;

static void srv_json_escape(const char *src, char *dst, size_t dst_cap) {
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

/* Locate `"key":` in a JSON fragment and return pointer right after `:` (skipping whitespace) */
static const char *srv_json_find_key_value(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    size_t pat_len = strlen(pat);
    const char *search = json;
    const char *p = NULL;
    while ((p = strstr(search, pat)) != NULL) {
        const char *q = p + pat_len;
        while (*q && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
        if (*q == ':') {
            q++;
            while (*q && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
            return q;
        }
        search = p + pat_len;
    }
    return NULL;
}

/* Extract scalar value (string, number, bool) for `"key":` as string into out */
static int srv_json_extract_scalar(const char *json, const char *key, char *out, size_t out_cap) {
    if (!out || out_cap == 0) return 0;
    out[0] = '\0';
    const char *p = srv_json_find_key_value(json, key);
    if (!p || !*p) return 0;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < out_cap) {
            if (*p == '\\' && p[1]) {
                p++;
                if (*p == 'n') out[i++] = '\n';
                else if (*p == 't') out[i++] = '\t';
                else if (*p == 'r') out[i++] = '\r';
                else out[i++] = *p;
                p++;
            } else {
                out[i++] = *p++;
            }
        }
        out[i] = '\0';
        return 1;
    }
    size_t i = 0;
    while (*p && *p != ',' && *p != '}' && *p != ']' && !isspace((unsigned char)*p) && i + 1 < out_cap) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i > 0 ? 1 : 0;
}

void pifari_mcp_server_init(const char *server_name, const char *version) {
    snprintf(g_srv_name, sizeof(g_srv_name), "%s",
             (server_name && server_name[0]) ? server_name : "pifari-mcp-server");
    snprintf(g_srv_version, sizeof(g_srv_version), "%s",
             (version && version[0]) ? version : "1.0.0");
    g_srv_tool_count = 0;
    g_srv_resource_count = 0;
}

int pifari_mcp_server_register_tool(
    const char *tool_name,
    const char *description,
    const char *function_name,
    size_t param_count,
    const char **param_names,
    int read_only
) {
    if (!tool_name || !tool_name[0] || !function_name || !function_name[0]) return -1;
    if (g_srv_tool_count >= PIFARI_MCP_SRV_MAX_TOOLS) return -1;
    if (param_count > PIFARI_MCP_SRV_MAX_PARAMS) param_count = PIFARI_MCP_SRV_MAX_PARAMS;

    PifariMcpServerTool *t = &g_srv_tools[g_srv_tool_count++];
    memset(t, 0, sizeof(*t));
    snprintf(t->tool_name, sizeof(t->tool_name), "%s", tool_name);
    snprintf(t->description, sizeof(t->description), "%s", description ? description : "");
    snprintf(t->function_name, sizeof(t->function_name), "%s", function_name);
    t->param_count = param_count;
    for (size_t i = 0; i < param_count; i++) {
        snprintf(t->param_names[i], sizeof(t->param_names[i]), "%s",
                 (param_names && param_names[i]) ? param_names[i] : "arg");
    }
    t->read_only = read_only ? 1 : 0;
    return 0;
}

int pifari_mcp_server_register_resource(
    const char *uri,
    const char *name,
    const char *mime_type,
    const char *content
) {
    if (!uri || !uri[0] || !content) return -1;
    if (g_srv_resource_count >= PIFARI_MCP_SRV_MAX_RESOURCES) return -1;

    PifariMcpServerResource *r = &g_srv_resources[g_srv_resource_count++];
    memset(r, 0, sizeof(*r));
    snprintf(r->uri, sizeof(r->uri), "%s", uri);
    snprintf(r->name, sizeof(r->name), "%s", (name && name[0]) ? name : uri);
    snprintf(r->mime_type, sizeof(r->mime_type), "%s",
             (mime_type && mime_type[0]) ? mime_type : "application/json");
    snprintf(r->content, sizeof(r->content), "%s", content);
    return 0;
}

int pifari_mcp_server_handle_line(
    const char *json_line,
    PifariToolInvokeFn invoke_cb,
    char *out_buf,
    size_t out_cap
) {
    if (!json_line || !out_buf || out_cap == 0) return 0;
    out_buf[0] = '\0';

    char method[128] = "";
    if (!srv_json_extract_scalar(json_line, "method", method, sizeof(method))) {
        return 0;
    }

    char id_raw[64] = "";
    int has_id = srv_json_extract_scalar(json_line, "id", id_raw, sizeof(id_raw));
    if (!has_id || strncmp(method, "notifications/", 14) == 0) {
        return 0;
    }

    if (strcmp(method, "initialize") == 0) {
        char esc_name[128], esc_ver[64];
        srv_json_escape(g_srv_name, esc_name, sizeof(esc_name));
        srv_json_escape(g_srv_version, esc_ver, sizeof(esc_ver));
        snprintf(
            out_buf, out_cap,
            "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{"
            "\"protocolVersion\":\"%s\","
            "\"capabilities\":{\"tools\":{\"listChanged\":false},\"resources\":{\"subscribe\":false,\"listChanged\":false}},"
            "\"serverInfo\":{\"name\":\"%s\",\"version\":\"%s\"}}}",
            id_raw, MCP_PROTOCOL_VERSION, esc_name, esc_ver
        );
        return 1;
    }

    if (strcmp(method, "ping") == 0) {
        snprintf(out_buf, out_cap, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{}}", id_raw);
        return 1;
    }

    if (strcmp(method, "tools/list") == 0) {
        size_t pos = 0;
        pos += (size_t)snprintf(out_buf + pos, out_cap - pos,
                                "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"tools\":[", id_raw);
        for (size_t i = 0; i < g_srv_tool_count && pos + 256 < out_cap; i++) {
            PifariMcpServerTool *t = &g_srv_tools[i];
            char esc_desc[512];
            srv_json_escape(t->description, esc_desc, sizeof(esc_desc));
            pos += (size_t)snprintf(
                out_buf + pos, out_cap - pos,
                "%s{\"name\":\"%s\",\"description\":\"%s\","
                "\"annotations\":{\"readOnlyHint\":%s,\"sideEffect\":%s},"
                "\"inputSchema\":{\"type\":\"object\",\"properties\":{",
                (i > 0) ? "," : "",
                t->tool_name,
                esc_desc,
                t->read_only ? "true" : "false",
                t->read_only ? "false" : "true"
            );
            for (size_t p = 0; p < t->param_count && pos + 128 < out_cap; p++) {
                pos += (size_t)snprintf(
                    out_buf + pos, out_cap - pos,
                    "%s\"%s\":{\"type\":\"string\"}",
                    (p > 0) ? "," : "",
                    t->param_names[p]
                );
            }
            pos += (size_t)snprintf(out_buf + pos, out_cap - pos, "}}}");
        }
        snprintf(out_buf + pos, out_cap - pos, "]}}");
        return 1;
    }

    if (strcmp(method, "resources/list") == 0) {
        size_t pos = 0;
        pos += (size_t)snprintf(out_buf + pos, out_cap - pos,
                                "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"resources\":[", id_raw);
        for (size_t i = 0; i < g_srv_resource_count && pos + 256 < out_cap; i++) {
            PifariMcpServerResource *r = &g_srv_resources[i];
            pos += (size_t)snprintf(
                out_buf + pos, out_cap - pos,
                "%s{\"uri\":\"%s\",\"name\":\"%s\",\"mimeType\":\"%s\"}",
                (i > 0) ? "," : "", r->uri, r->name, r->mime_type
            );
        }
        snprintf(out_buf + pos, out_cap - pos, "]}}");
        return 1;
    }

    if (strcmp(method, "resources/read") == 0) {
        const char *params = srv_json_find_key_value(json_line, "params");
        char uri[256] = "";
        srv_json_extract_scalar(params ? params : json_line, "uri", uri, sizeof(uri));
        for (size_t i = 0; i < g_srv_resource_count; i++) {
            if (strcmp(g_srv_resources[i].uri, uri) == 0) {
                char esc_text[4096];
                srv_json_escape(g_srv_resources[i].content, esc_text, sizeof(esc_text));
                snprintf(
                    out_buf, out_cap,
                    "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"contents\":[{\"uri\":\"%s\",\"mimeType\":\"%s\",\"text\":\"%s\"}]}}",
                    id_raw, g_srv_resources[i].uri, g_srv_resources[i].mime_type, esc_text
                );
                return 1;
            }
        }
        snprintf(out_buf, out_cap,
                 "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":-32602,\"message\":\"Resource not found\"}}",
                 id_raw);
        return 1;
    }

    if (strcmp(method, "tools/call") == 0) {
        const char *params = srv_json_find_key_value(json_line, "params");
        if (!params) params = json_line;
        char tname[PIFARI_MCP_SRV_ID_MAX] = "";
        srv_json_extract_scalar(params, "name", tname, sizeof(tname));

        PifariMcpServerTool *tool = NULL;
        for (size_t i = 0; i < g_srv_tool_count; i++) {
            if (strcmp(g_srv_tools[i].tool_name, tname) == 0) {
                tool = &g_srv_tools[i];
                break;
            }
        }
        if (!tool || !invoke_cb) {
            snprintf(out_buf, out_cap,
                     "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":-32601,\"message\":\"Unknown tool\"}}",
                     id_raw);
            return 1;
        }

        const char *args_obj = srv_json_find_key_value(params, "arguments");
        if (!args_obj) args_obj = params;

        char arg_storage[PIFARI_MCP_SRV_MAX_PARAMS][512];
        const char *argv[PIFARI_MCP_SRV_MAX_PARAMS];
        for (size_t p = 0; p < tool->param_count; p++) {
            arg_storage[p][0] = '\0';
            srv_json_extract_scalar(args_obj, tool->param_names[p], arg_storage[p], sizeof(arg_storage[p]));
            argv[p] = arg_storage[p];
        }

        int is_error = 0;
        char *res_str = invoke_cb(tool->function_name, tool->param_count, argv, &is_error);
        char esc_out[8192];
        srv_json_escape(res_str ? res_str : "", esc_out, sizeof(esc_out));
        free(res_str);

        snprintf(
            out_buf, out_cap,
            "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{\"content\":[{\"type\":\"text\",\"text\":\"%s\"}],\"isError\":%s}}",
            id_raw, esc_out, is_error ? "true" : "false"
        );
        return 1;
    }

    snprintf(out_buf, out_cap,
             "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}",
             id_raw);
    return 1;
}

int pifari_mcp_server_run_stdio(PifariToolInvokeFn invoke_cb) {
    char line[65536];
    char out[65536];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;
        if (pifari_mcp_server_handle_line(line, invoke_cb, out, sizeof(out))) {
            fputs(out, stdout);
            fputc('\n', stdout);
            fflush(stdout);
        }
    }
    return 0;
}

int pifari_mcp_server_run_http(int port, PifariToolInvokeFn invoke_cb) {
    int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_fd < 0) return -1;
    int opt = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);

    if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
        listen(srv_fd, 16) != 0) {
        close(srv_fd);
        return -1;
    }

    for (;;) {
        int cli = accept(srv_fd, NULL, NULL);
        if (cli < 0) {
            if (errno == EINTR) continue;
            break;
        }
        char req[65536];
        ssize_t n = recv(cli, req, sizeof(req) - 1, 0);
        if (n > 0) {
            req[n] = '\0';
            const char *body = strstr(req, "\r\n\r\n");
            if (body) {
                body += 4;
                char out[65536];
                int has_resp = pifari_mcp_server_handle_line(body, invoke_cb, out, sizeof(out));
                char http_resp[65536];
                if (has_resp) {
                    int hlen = snprintf(
                        http_resp, sizeof(http_resp),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Mcp-Session-Id: pifari-native-http-01\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(out), out
                    );
                    if (hlen > 0) send(cli, http_resp, (size_t)hlen, 0);
                } else {
                    const char *accepted =
                        "HTTP/1.1 202 Accepted\r\n"
                        "Mcp-Session-Id: pifari-native-http-01\r\n"
                        "Content-Length: 2\r\n"
                        "Connection: close\r\n\r\n{}";
                    send(cli, accepted, strlen(accepted), 0);
                }
            }
        }
        close(cli);
    }
    close(srv_fd);
    return 0;
}
