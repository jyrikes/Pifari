#define _POSIX_C_SOURCE 200809L
#include "mcp_gateway.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static McpServerEntry g_servers[MCP_MAX_SERVERS];
static size_t g_server_count = 0;
static pthread_mutex_t g_gateway_lock = PTHREAD_MUTEX_INITIALIZER;
static long long g_next_rpc_id = 1;
static int g_initialized = 0;

static long long mcp_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
}

static uint64_t fnv1a_hash_str(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    if (!s) return h;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        h ^= (uint64_t)(*p);
        h *= 1099511628211ULL;
    }
    return h;
}

const char *mcp_status_str(McpStatus st) {
    switch (st) {
        case MCP_STATUS_OK: return "OK";
        case MCP_STATUS_ERR_INVALID_ARGS: return "ERR_INVALID_ARGS";
        case MCP_STATUS_ERR_NOT_FOUND: return "ERR_NOT_FOUND";
        case MCP_STATUS_ERR_TRANSPORT: return "ERR_TRANSPORT";
        case MCP_STATUS_ERR_TIMEOUT: return "ERR_TIMEOUT";
        case MCP_STATUS_ERR_CANCELLED: return "ERR_CANCELLED";
        case MCP_STATUS_ERR_PROTOCOL: return "ERR_PROTOCOL";
        case MCP_STATUS_ERR_POLICY_DENIED: return "ERR_POLICY_DENIED";
        case MCP_STATUS_ERR_CONCURRENCY_LIMIT: return "ERR_CONCURRENCY_LIMIT";
        case MCP_STATUS_ERR_PAYLOAD_TOO_LARGE: return "ERR_PAYLOAD_TOO_LARGE";
        case MCP_STATUS_ERR_DUPLICATE_CALL: return "ERR_DUPLICATE_CALL";
        default: return "ERR_UNKNOWN";
    }
}

void mcp_gateway_init(void) {
    pthread_mutex_lock(&g_gateway_lock);
    if (!g_initialized) {
        memset(g_servers, 0, sizeof(g_servers));
        for (size_t i = 0; i < MCP_MAX_SERVERS; i++) {
            pthread_mutex_init(&g_servers[i].lock, NULL);
            g_servers[i].child_pid = -1;
            g_servers[i].stdin_fd = -1;
            g_servers[i].stdout_fd = -1;
        }
        g_server_count = 0;
        g_initialized = 1;
    }
    pthread_mutex_unlock(&g_gateway_lock);
}

static McpServerEntry *find_server_Mutable(const char *server_id) {
    if (!server_id) return NULL;
    for (size_t i = 0; i < g_server_count; i++) {
        if (strcmp(g_servers[i].server_id, server_id) == 0) {
            return &g_servers[i];
        }
    }
    return NULL;
}

const McpServerEntry *mcp_gateway_get_server(const char *server_id) {
    mcp_gateway_init();
    pthread_mutex_lock(&g_gateway_lock);
    const McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    return srv;
}

/* JSON helper: extract string value for a given key ("key":"value") */
static int json_extract_string(const char *json, const char *key, char *out, size_t out_cap) {
    if (!json || !key || !out || out_cap == 0) return 0;
    out[0] = '\0';
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    size_t pat_len = strlen(pat);
    const char *search = json;
    const char *p = NULL;
    while ((p = strstr(search, pat)) != NULL) {
        const char *q = p + pat_len;
        while (*q && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
        if (*q == ':') {
            p = q + 1;
            break;
        }
        search = p + pat_len;
        p = NULL;
    }
    if (!p) return 0;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (*p != '"') return 0;
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

static int json_extract_int(const char *json, const char *key, long long *out_val) {
    if (!json || !key || !out_val) return 0;
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (*p != ':') return 0;
    p++;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (*p != '-' && !isdigit((unsigned char)*p)) return 0;
    *out_val = strtoll(p, NULL, 10);
    return 1;
}

static int json_validate_object_or_empty(const char *args_json) {
    if (!args_json || args_json[0] == '\0') return 1;
    size_t len = strlen(args_json);
    if (len > MCP_MAX_PAYLOAD_BYTES) return -2;
    const char *s = args_json;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s != '{') return 0;
    const char *e = args_json + len - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    if (*e != '}') return 0;
    return 1;
}

static void json_escape_into(const char *src, char *dst, size_t dst_cap) {
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

/* Send raw line over stdio pipe */
static int stdio_write_line(McpServerEntry *srv, const char *line) {
    if (srv->stdin_fd < 0 || !line) return -1;
    size_t len = strlen(line);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(srv->stdin_fd, line + sent, len - sent);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    if (len == 0 || line[len - 1] != '\n') {
        if (write(srv->stdin_fd, "\n", 1) != 1) return -1;
    }
    return 0;
}

/* Read matching JSON-RPC response line by id with timeout and cancel_flag support */
static McpStatus stdio_read_rpc_response(
    McpServerEntry *srv,
    long long expected_id,
    int timeout_ms,
    volatile int *cancel_flag,
    char *out_buf,
    size_t out_cap
) {
    if (srv->stdout_fd < 0 || !out_buf || out_cap == 0) return MCP_STATUS_ERR_TRANSPORT;
    out_buf[0] = '\0';
    long long start_ms = mcp_now_ms();
    int eff_timeout = timeout_ms > 0 ? timeout_ms : srv->default_timeout_ms;
    if (eff_timeout <= 0) eff_timeout = 5000;

    char line_buf[65536];
    size_t line_pos = 0;

    for (;;) {
        if (cancel_flag && *cancel_flag) {
            char cancel_msg[256];
            snprintf(cancel_msg, sizeof(cancel_msg),
                     "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/cancelled\",\"params\":{\"requestId\":%lld,\"reason\":\"cancelled_by_task_engine\"}}\n",
                     expected_id);
            stdio_write_line(srv, cancel_msg);
            return MCP_STATUS_ERR_CANCELLED;
        }
        long long elapsed = mcp_now_ms() - start_ms;
        if (elapsed >= eff_timeout) {
            char cancel_msg[256];
            snprintf(cancel_msg, sizeof(cancel_msg),
                     "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/cancelled\",\"params\":{\"requestId\":%lld,\"reason\":\"timeout\"}}\n",
                     expected_id);
            stdio_write_line(srv, cancel_msg);
            return MCP_STATUS_ERR_TIMEOUT;
        }
        int wait_step = (int)(eff_timeout - elapsed);
        if (wait_step > 15) wait_step = 15;

        struct pollfd pfd;
        pfd.fd = srv->stdout_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = poll(&pfd, 1, wait_step);
        if (pr < 0) {
            if (errno == EINTR) continue;
            return MCP_STATUS_ERR_TRANSPORT;
        }
        if (pr == 0) continue;

        if (pfd.revents & (POLLIN | POLLHUP)) {
            char ch;
            ssize_t r = read(srv->stdout_fd, &ch, 1);
            if (r < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
                return MCP_STATUS_ERR_TRANSPORT;
            }
            if (r == 0) {
                return MCP_STATUS_ERR_TRANSPORT;
            }
            if (ch == '\n') {
                line_buf[line_pos] = '\0';
                line_pos = 0;
                if (line_buf[0] == '\0') continue;
                long long resp_id = -1;
                if (json_extract_int(line_buf, "id", &resp_id) && resp_id == expected_id) {
                    snprintf(out_buf, out_cap, "%s", line_buf);
                    return MCP_STATUS_OK;
                }
                /* Ignore notifications or stale responses from timed-out/cancelled requests */
            } else if (ch != '\r') {
                if (line_pos + 1 < sizeof(line_buf)) {
                    line_buf[line_pos++] = ch;
                } else {
                    return MCP_STATUS_ERR_PAYLOAD_TOO_LARGE;
                }
            }
        }
    }
}

/* HTTP JSON-RPC 2.0 call over POSIX sockets */
static McpStatus http_send_rpc_request(
    McpServerEntry *srv,
    const char *rpc_payload,
    int timeout_ms,
    volatile int *cancel_flag,
    char *out_buf,
    size_t out_cap
) {
    if (!srv || !rpc_payload || !out_buf || out_cap == 0) return MCP_STATUS_ERR_INVALID_ARGS;
    out_buf[0] = '\0';

    /* Parse endpoint http://host:port/path or host:port/path */
    const char *ep = srv->endpoint;
    if (strncmp(ep, "http://", 7) == 0) ep += 7;
    char host[128] = "127.0.0.1";
    char port_str[16] = "80";
    char path[256] = "/mcp";

    const char *slash = strchr(ep, '/');
    const char *colon = strchr(ep, ':');
    if (colon && (!slash || colon < slash)) {
        size_t hlen = (size_t)(colon - ep);
        if (hlen >= sizeof(host)) hlen = sizeof(host) - 1;
        memcpy(host, ep, hlen);
        host[hlen] = '\0';
        if (slash) {
            size_t plen = (size_t)(slash - (colon + 1));
            if (plen >= sizeof(port_str)) plen = sizeof(port_str) - 1;
            memcpy(port_str, colon + 1, plen);
            port_str[plen] = '\0';
            snprintf(path, sizeof(path), "%s", slash);
        } else {
            snprintf(port_str, sizeof(port_str), "%s", colon + 1);
        }
    } else if (slash) {
        size_t hlen = (size_t)(slash - ep);
        if (hlen >= sizeof(host)) hlen = sizeof(host) - 1;
        memcpy(host, ep, hlen);
        host[hlen] = '\0';
        snprintf(path, sizeof(path), "%s", slash);
    } else if (ep[0]) {
        snprintf(host, sizeof(host), "%s", ep);
    }

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return MCP_STATUS_ERR_TRANSPORT;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return MCP_STATUS_ERR_TRANSPORT;
    }
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int cr = connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (cr < 0 && errno != EINPROGRESS) {
        close(sock);
        return MCP_STATUS_ERR_TRANSPORT;
    }

    int eff_timeout = timeout_ms > 0 ? timeout_ms : srv->default_timeout_ms;
    if (eff_timeout <= 0) eff_timeout = 5000;
    long long start_ms = mcp_now_ms();

    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    if (poll(&pfd, 1, eff_timeout) <= 0) {
        close(sock);
        return MCP_STATUS_ERR_TIMEOUT;
    }

    /* Read optional token from environment variable without storing or logging it */
    const char *bearer = NULL;
    if (srv->auth_env_var[0] != '\0') {
        bearer = getenv(srv->auth_env_var);
    }

    char req_header[2048];
    size_t body_len = strlen(rpc_payload);
    int hlen = snprintf(
        req_header, sizeof(req_header),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%s\r\n"
        "Content-Type: application/json\r\n"
        "Accept: application/json, text/event-stream\r\n"
        "%s%s%s"
        "%s%s%s"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        path, host, port_str,
        srv->session_id[0] ? "Mcp-Session-Id: " : "",
        srv->session_id[0] ? srv->session_id : "",
        srv->session_id[0] ? "\r\n" : "",
        (bearer && bearer[0]) ? "Authorization: Bearer " : "",
        (bearer && bearer[0]) ? bearer : "",
        (bearer && bearer[0]) ? "\r\n" : "",
        body_len
    );

    if (hlen <= 0 || (size_t)hlen >= sizeof(req_header)) {
        close(sock);
        return MCP_STATUS_ERR_PAYLOAD_TOO_LARGE;
    }

    if (send(sock, req_header, (size_t)hlen, 0) < 0 ||
        send(sock, rpc_payload, body_len, 0) < 0) {
        close(sock);
        return MCP_STATUS_ERR_TRANSPORT;
    }

    char raw_resp[65536];
    size_t raw_len = 0;
    while (raw_len + 1 < sizeof(raw_resp)) {
        if (cancel_flag && *cancel_flag) {
            close(sock);
            return MCP_STATUS_ERR_CANCELLED;
        }
        long long elapsed = mcp_now_ms() - start_ms;
        if (elapsed >= eff_timeout) {
            close(sock);
            return MCP_STATUS_ERR_TIMEOUT;
        }
        pfd.fd = sock;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, 15);
        if (pr < 0) {
            if (errno == EINTR) continue;
            close(sock);
            return MCP_STATUS_ERR_TRANSPORT;
        }
        if (pr == 0) continue;
        ssize_t n = recv(sock, raw_resp + raw_len, sizeof(raw_resp) - 1 - raw_len, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
            close(sock);
            return MCP_STATUS_ERR_TRANSPORT;
        }
        if (n == 0) break;
        raw_len += (size_t)n;
    }
    raw_resp[raw_len] = '\0';
    close(sock);

    const char *body = strstr(raw_resp, "\r\n\r\n");
    if (!body) return MCP_STATUS_ERR_PROTOCOL;
    /* Check if Mcp-Session-Id is present in headers */
    const char *sid_hdr = strstr(raw_resp, "Mcp-Session-Id:");
    if (!sid_hdr) sid_hdr = strstr(raw_resp, "mcp-session-id:");
    if (sid_hdr && sid_hdr < body) {
        sid_hdr += 15;
        while (*sid_hdr == ' ' || *sid_hdr == '\t') sid_hdr++;
        size_t si = 0;
        while (sid_hdr[si] && sid_hdr[si] != '\r' && sid_hdr[si] != '\n' && si + 1 < sizeof(srv->session_id)) {
            srv->session_id[si] = sid_hdr[si];
            si++;
        }
        srv->session_id[si] = '\0';
    }
    body += 4;
    /* Support SSE "data: {...}" or direct JSON */
    const char *sse_data = strstr(body, "data:");
    if (sse_data) {
        sse_data += 5;
        while (*sse_data == ' ') sse_data++;
        snprintf(out_buf, out_cap, "%s", sse_data);
    } else {
        snprintf(out_buf, out_cap, "%s", body);
    }
    return MCP_STATUS_OK;
}

static McpStatus mcp_rpc_exchange(
    McpServerEntry *srv,
    const char *method,
    const char *params_json,
    int timeout_ms,
    volatile int *cancel_flag,
    char *out_resp,
    size_t out_cap
) {
    long long req_id = __atomic_fetch_add(&g_next_rpc_id, 1, __ATOMIC_SEQ_CST);
    char req_buf[8192];
    int n = snprintf(
        req_buf, sizeof(req_buf),
        "{\"jsonrpc\":\"2.0\",\"id\":%lld,\"method\":\"%s\",\"params\":%s}\n",
        req_id, method, params_json ? params_json : "{}"
    );
    if (n <= 0 || (size_t)n >= sizeof(req_buf)) return MCP_STATUS_ERR_PAYLOAD_TOO_LARGE;

    if (srv->transport == MCP_TRANSPORT_STDIO) {
        if (stdio_write_line(srv, req_buf) != 0) return MCP_STATUS_ERR_TRANSPORT;
        return stdio_read_rpc_response(srv, req_id, timeout_ms, cancel_flag, out_resp, out_cap);
    } else {
        return http_send_rpc_request(srv, req_buf, timeout_ms, cancel_flag, out_resp, out_cap);
    }
}

McpStatus mcp_gateway_register_server(
    const char *server_id,
    McpTransportType transport,
    const char *endpoint,
    const char *auth_env_var,
    int default_timeout_ms,
    int max_concurrent
) {
    mcp_gateway_init();
    if (!server_id || !server_id[0] || !endpoint || !endpoint[0]) {
        return MCP_STATUS_ERR_INVALID_ARGS;
    }

    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    if (!srv) {
        if (g_server_count >= MCP_MAX_SERVERS) {
            pthread_mutex_unlock(&g_gateway_lock);
            return MCP_STATUS_ERR_INVALID_ARGS;
        }
        srv = &g_servers[g_server_count++];
        pthread_mutex_init(&srv->lock, NULL);
        srv->child_pid = -1;
        srv->stdin_fd = -1;
        srv->stdout_fd = -1;
    }
    pthread_mutex_lock(&srv->lock);
    snprintf(srv->server_id, sizeof(srv->server_id), "%s", server_id);
    srv->transport = transport;
    snprintf(srv->endpoint, sizeof(srv->endpoint), "%s", endpoint);
    if (auth_env_var && auth_env_var[0]) {
        snprintf(srv->auth_env_var, sizeof(srv->auth_env_var), "%s", auth_env_var);
    } else {
        srv->auth_env_var[0] = '\0';
    }
    srv->default_timeout_ms = default_timeout_ms > 0 ? default_timeout_ms : 3000;
    srv->max_concurrent = max_concurrent > 0 ? max_concurrent : 4;
    pthread_mutex_unlock(&srv->lock);
    pthread_mutex_unlock(&g_gateway_lock);
    return MCP_STATUS_OK;
}

McpStatus mcp_gateway_connect(const char *server_id) {
    mcp_gateway_init();
    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) return MCP_STATUS_ERR_NOT_FOUND;

    pthread_mutex_lock(&srv->lock);
    if (srv->connected) {
        pthread_mutex_unlock(&srv->lock);
        return MCP_STATUS_OK;
    }

    if (srv->transport == MCP_TRANSPORT_STDIO) {
        int in_pipe[2], out_pipe[2];
        if (pipe(in_pipe) != 0) {
            pthread_mutex_unlock(&srv->lock);
            return MCP_STATUS_ERR_TRANSPORT;
        }
        if (pipe(out_pipe) != 0) {
            close(in_pipe[0]);
            close(in_pipe[1]);
            pthread_mutex_unlock(&srv->lock);
            return MCP_STATUS_ERR_TRANSPORT;
        }
        pid_t pid = fork();
        if (pid < 0) {
            close(in_pipe[0]); close(in_pipe[1]);
            close(out_pipe[0]); close(out_pipe[1]);
            pthread_mutex_unlock(&srv->lock);
            return MCP_STATUS_ERR_TRANSPORT;
        }
        if (pid == 0) {
            dup2(in_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);
            close(in_pipe[0]); close(in_pipe[1]);
            close(out_pipe[0]); close(out_pipe[1]);
            execl("/bin/sh", "sh", "-c", srv->endpoint, (char *)NULL);
            _exit(127);
        }
        close(in_pipe[0]);
        close(out_pipe[1]);
        int fl = fcntl(out_pipe[0], F_GETFL, 0);
        fcntl(out_pipe[0], F_SETFL, fl | O_NONBLOCK);
        srv->child_pid = pid;
        srv->stdin_fd = in_pipe[1];
        srv->stdout_fd = out_pipe[0];
    }

    /* Perform MCP initialize handshake */
    char resp[16384];
    const char *init_params =
        "{\"protocolVersion\":\"" MCP_PROTOCOL_VERSION "\","
        "\"capabilities\":{\"roots\":{\"listChanged\":false}},"
        "\"clientInfo\":{\"name\":\"pifari-mcp-gateway\",\"version\":\"1.0.0\"}}";

    McpStatus st = mcp_rpc_exchange(srv, "initialize", init_params, srv->default_timeout_ms, NULL, resp, sizeof(resp));
    if (st != MCP_STATUS_OK) {
        pthread_mutex_unlock(&srv->lock);
        mcp_gateway_disconnect(server_id);
        return st;
    }

    if (!json_extract_string(resp, "protocolVersion", srv->negotiated_version, sizeof(srv->negotiated_version))) {
        pthread_mutex_unlock(&srv->lock);
        mcp_gateway_disconnect(server_id);
        return MCP_STATUS_ERR_PROTOCOL;
    }
    srv->capabilities_tools = strstr(resp, "\"tools\"") != NULL ? 1 : 0;
    srv->capabilities_resources = strstr(resp, "\"resources\"") != NULL ? 1 : 0;
    srv->capabilities_prompts = strstr(resp, "\"prompts\"") != NULL ? 1 : 0;

    /* Send notifications/initialized */
    const char *notif = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\",\"params\":{}}\n";
    if (srv->transport == MCP_TRANSPORT_STDIO) {
        stdio_write_line(srv, notif);
    } else {
        char dummy[1024];
        http_send_rpc_request(srv, notif, srv->default_timeout_ms, NULL, dummy, sizeof(dummy));
    }

    srv->connected = 1;
    pthread_mutex_unlock(&srv->lock);
    return MCP_STATUS_OK;
}

McpStatus mcp_gateway_disconnect(const char *server_id) {
    mcp_gateway_init();
    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) return MCP_STATUS_ERR_NOT_FOUND;

    pthread_mutex_lock(&srv->lock);
    if (srv->stdin_fd >= 0) {
        close(srv->stdin_fd);
        srv->stdin_fd = -1;
    }
    if (srv->stdout_fd >= 0) {
        close(srv->stdout_fd);
        srv->stdout_fd = -1;
    }
    if (srv->child_pid > 0) {
        kill(srv->child_pid, SIGTERM);
        int status = 0;
        for (int i = 0; i < 20; i++) {
            pid_t w = waitpid(srv->child_pid, &status, WNOHANG);
            if (w == srv->child_pid || w < 0) {
                srv->child_pid = -1;
                break;
            }
            struct timespec ts = {0, 5000000};
            nanosleep(&ts, NULL);
        }
        if (srv->child_pid > 0) {
            kill(srv->child_pid, SIGKILL);
            waitpid(srv->child_pid, &status, 0);
            srv->child_pid = -1;
        }
    }
    srv->connected = 0;
    srv->session_id[0] = '\0';
    pthread_mutex_unlock(&srv->lock);
    return MCP_STATUS_OK;
}

void mcp_gateway_shutdown_all(void) {
    if (!g_initialized) return;
    for (size_t i = 0; i < g_server_count; i++) {
        if (g_servers[i].server_id[0]) {
            mcp_gateway_disconnect(g_servers[i].server_id);
        }
    }
}

McpStatus mcp_gateway_discover_tools(const char *server_id, size_t *out_count) {
    if (out_count) *out_count = 0;
    McpStatus st = mcp_gateway_connect(server_id);
    if (st != MCP_STATUS_OK) return st;

    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) return MCP_STATUS_ERR_NOT_FOUND;

    pthread_mutex_lock(&srv->lock);
    char resp[32768];
    st = mcp_rpc_exchange(srv, "tools/list", "{}", srv->default_timeout_ms, NULL, resp, sizeof(resp));
    if (st != MCP_STATUS_OK) {
        pthread_mutex_unlock(&srv->lock);
        return st;
    }

    srv->tool_count = 0;
    const char *tools_arr = strstr(resp, "\"tools\"");
    if (tools_arr) {
        const char *cursor = strchr(tools_arr, '[');
        while (cursor && srv->tool_count < MCP_MAX_TOOLS_PER_SERVER) {
            const char *obj_start = strchr(cursor, '{');
            if (!obj_start) break;
            /* Find matching brace for tool item (up to 2 levels of nesting for inputSchema/annotations) */
            int depth = 0;
            const char *p = obj_start;
            while (*p) {
                if (*p == '{') depth++;
                else if (*p == '}') {
                    depth--;
                    if (depth == 0) break;
                }
                p++;
            }
            if (*p != '}') break;
            size_t obj_len = (size_t)(p - obj_start + 1);
            char item_buf[2048];
            if (obj_len >= sizeof(item_buf)) obj_len = sizeof(item_buf) - 1;
            memcpy(item_buf, obj_start, obj_len);
            item_buf[obj_len] = '\0';

            McpToolInfo *ti = &srv->tools[srv->tool_count];
            if (json_extract_string(item_buf, "name", ti->name, sizeof(ti->name)) && ti->name[0]) {
                json_extract_string(item_buf, "description", ti->description, sizeof(ti->description));
                ti->is_side_effect = (strstr(item_buf, "\"readOnlyHint\":false") != NULL ||
                                      strstr(item_buf, "\"readOnlyHint\": false") != NULL ||
                                      strstr(item_buf, "\"sideEffect\":true") != NULL ||
                                      strstr(item_buf, "\"sideEffect\": true") != NULL ||
                                      strncmp(ti->name, "write_", 6) == 0 ||
                                      strncmp(ti->name, "mutate_", 7) == 0 ||
                                      strncmp(ti->name, "delete_", 7) == 0) ? 1 : 0;
                srv->tool_count++;
            }
            cursor = p + 1;
        }
    }
    if (out_count) *out_count = srv->tool_count;
    pthread_mutex_unlock(&srv->lock);
    return MCP_STATUS_OK;
}

McpStatus mcp_gateway_discover_resources(const char *server_id, size_t *out_count) {
    if (out_count) *out_count = 0;
    McpStatus st = mcp_gateway_connect(server_id);
    if (st != MCP_STATUS_OK) return st;

    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) return MCP_STATUS_ERR_NOT_FOUND;

    pthread_mutex_lock(&srv->lock);
    char resp[32768];
    st = mcp_rpc_exchange(srv, "resources/list", "{}", srv->default_timeout_ms, NULL, resp, sizeof(resp));
    if (st != MCP_STATUS_OK) {
        pthread_mutex_unlock(&srv->lock);
        return st;
    }

    srv->resource_count = 0;
    const char *res_arr = strstr(resp, "\"resources\"");
    if (res_arr) {
        const char *cursor = strchr(res_arr, '[');
        while (cursor && srv->resource_count < MCP_MAX_RESOURCES_PER_SERVER) {
            const char *obj_start = strchr(cursor, '{');
            if (!obj_start) break;
            const char *obj_end = strchr(obj_start, '}');
            if (!obj_end) break;
            size_t obj_len = (size_t)(obj_end - obj_start + 1);
            char item_buf[1024];
            if (obj_len >= sizeof(item_buf)) obj_len = sizeof(item_buf) - 1;
            memcpy(item_buf, obj_start, obj_len);
            item_buf[obj_len] = '\0';

            McpResourceInfo *ri = &srv->resources[srv->resource_count];
            if (json_extract_string(item_buf, "uri", ri->uri, sizeof(ri->uri)) && ri->uri[0]) {
                json_extract_string(item_buf, "name", ri->name, sizeof(ri->name));
                json_extract_string(item_buf, "mimeType", ri->mime_type, sizeof(ri->mime_type));
                srv->resource_count++;
            }
            cursor = obj_end + 1;
        }
    }
    if (out_count) *out_count = srv->resource_count;
    pthread_mutex_unlock(&srv->lock);
    return MCP_STATUS_OK;
}

int mcp_gateway_tool_is_side_effect(const char *server_id, const char *tool_name) {
    if (!server_id || !tool_name) return 1;
    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) return 1;
    int side_effect = 0;
    pthread_mutex_lock(&srv->lock);
    for (size_t i = 0; i < srv->tool_count; i++) {
        if (strcmp(srv->tools[i].name, tool_name) == 0) {
            side_effect = srv->tools[i].is_side_effect;
            break;
        }
    }
    pthread_mutex_unlock(&srv->lock);
    return side_effect;
}

McpCallResult mcp_gateway_call_tool(
    const char *server_id,
    const char *tool_name,
    const char *args_json,
    int timeout_ms,
    volatile int *cancel_flag,
    uint64_t idempotency_key
) {
    McpCallResult res;
    memset(&res, 0, sizeof(res));
    long long t0 = mcp_now_ms();

    if (!server_id || !server_id[0] || !tool_name || !tool_name[0]) {
        res.status = MCP_STATUS_ERR_INVALID_ARGS;
        snprintf(res.error_message, sizeof(res.error_message), "server_id e tool_name obrigatorios");
        return res;
    }

    int v = json_validate_object_or_empty(args_json);
    if (v == -2) {
        res.status = MCP_STATUS_ERR_PAYLOAD_TOO_LARGE;
        snprintf(res.error_message, sizeof(res.error_message), "args_json excede limite de payload");
        return res;
    }
    if (v <= 0) {
        res.status = MCP_STATUS_ERR_INVALID_ARGS;
        snprintf(res.error_message, sizeof(res.error_message), "args_json deve ser objeto JSON valido");
        return res;
    }

    McpStatus st = mcp_gateway_connect(server_id);
    if (st != MCP_STATUS_OK) {
        res.status = st;
        snprintf(res.error_message, sizeof(res.error_message), "falha ao conectar ao servidor MCP '%s'", server_id);
        return res;
    }

    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) {
        res.status = MCP_STATUS_ERR_NOT_FOUND;
        return res;
    }

    int is_side_effect = mcp_gateway_tool_is_side_effect(server_id, tool_name);
    uint64_t call_hash = idempotency_key;
    if (call_hash == 0 && is_side_effect) {
        char sig[1024];
        snprintf(sig, sizeof(sig), "%s:%s:%s", server_id, tool_name, args_json ? args_json : "{}");
        call_hash = fnv1a_hash_str(sig);
    }

    pthread_mutex_lock(&srv->lock);
    if (srv->active_calls >= srv->max_concurrent) {
        pthread_mutex_unlock(&srv->lock);
        res.status = MCP_STATUS_ERR_CONCURRENCY_LIMIT;
        snprintf(res.error_message, sizeof(res.error_message), "limite de concorrencia do servidor MCP atingido");
        return res;
    }
    /* Duplicate side-effect call guard */
    if (is_side_effect && call_hash != 0) {
        for (size_t i = 0; i < srv->recent_hash_count; i++) {
            if (srv->recent_side_effect_hashes[i] == call_hash) {
                pthread_mutex_unlock(&srv->lock);
                res.status = MCP_STATUS_ERR_DUPLICATE_CALL;
                snprintf(res.error_message, sizeof(res.error_message),
                         "chamada duplicada bloqueada para ferramenta com efeito colateral '%s'", tool_name);
                return res;
            }
        }
    }
    srv->active_calls++;

    char escaped_tool[128];
    json_escape_into(tool_name, escaped_tool, sizeof(escaped_tool));
    char params[4096];
    snprintf(params, sizeof(params), "{\"name\":\"%s\",\"arguments\":%s}",
             escaped_tool, (args_json && args_json[0]) ? args_json : "{}");

    char resp[32768];
    st = mcp_rpc_exchange(srv, "tools/call", params, timeout_ms, cancel_flag, resp, sizeof(resp));
    if (st == MCP_STATUS_OK && is_side_effect && call_hash != 0) {
        size_t idx = srv->recent_hash_count < 32 ? srv->recent_hash_count++ : (size_t)(call_hash % 32);
        srv->recent_side_effect_hashes[idx] = call_hash;
    }
    srv->active_calls--;
    pthread_mutex_unlock(&srv->lock);

    res.latency_ms = mcp_now_ms() - t0;
    res.status = st;
    if (st != MCP_STATUS_OK) {
        snprintf(res.error_message, sizeof(res.error_message), "MCP transport/timeout state: %s", mcp_status_str(st));
        return res;
    }

    /* Check JSON-RPC error or tool isError:true */
    const char *err_obj = strstr(resp, "\"error\"");
    if (err_obj && strstr(resp, "\"result\"") == NULL) {
        long long code = -32000;
        json_extract_int(err_obj, "code", &code);
        res.rpc_error_code = (int)code;
        json_extract_string(err_obj, "message", res.error_message, sizeof(res.error_message));
        res.status = MCP_STATUS_ERR_PROTOCOL;
        return res;
    }
    if (strstr(resp, "\"isError\":true") != NULL || strstr(resp, "\"isError\": true") != NULL) {
        json_extract_string(resp, "text", res.error_message, sizeof(res.error_message));
        res.status = MCP_STATUS_ERR_PROTOCOL;
        return res;
    }

    if (!json_extract_string(resp, "text", res.output_text, sizeof(res.output_text))) {
        snprintf(res.output_text, sizeof(res.output_text), "%s", resp);
    }
    return res;
}

McpCallResult mcp_gateway_read_resource(
    const char *server_id,
    const char *uri,
    int timeout_ms
) {
    McpCallResult res;
    memset(&res, 0, sizeof(res));
    long long t0 = mcp_now_ms();

    if (!server_id || !uri || !uri[0]) {
        res.status = MCP_STATUS_ERR_INVALID_ARGS;
        return res;
    }
    McpStatus st = mcp_gateway_connect(server_id);
    if (st != MCP_STATUS_OK) {
        res.status = st;
        return res;
    }
    pthread_mutex_lock(&g_gateway_lock);
    McpServerEntry *srv = find_server_Mutable(server_id);
    pthread_mutex_unlock(&g_gateway_lock);
    if (!srv) {
        res.status = MCP_STATUS_ERR_NOT_FOUND;
        return res;
    }

    pthread_mutex_lock(&srv->lock);
    char escaped_uri[256];
    json_escape_into(uri, escaped_uri, sizeof(escaped_uri));
    char params[512];
    snprintf(params, sizeof(params), "{\"uri\":\"%s\"}", escaped_uri);

    char resp[32768];
    st = mcp_rpc_exchange(srv, "resources/read", params, timeout_ms, NULL, resp, sizeof(resp));
    pthread_mutex_unlock(&srv->lock);

    res.latency_ms = mcp_now_ms() - t0;
    res.status = st;
    if (st == MCP_STATUS_OK) {
        if (!json_extract_string(resp, "text", res.output_text, sizeof(res.output_text))) {
            snprintf(res.output_text, sizeof(res.output_text), "%s", resp);
        }
    }
    return res;
}
