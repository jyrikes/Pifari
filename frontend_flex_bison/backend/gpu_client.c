#define _GNU_SOURCE
#include "gpu_client.h"

#include "devices.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t worker_pid = -1;
static FILE *to_worker;
static FILE *from_worker;
static int worker_failed;
static char last_error[256];

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *b64_encode(const unsigned char *data, size_t len) {
 size_t out_len = ((len + 2) / 3) * 4;
 char *out = malloc(out_len + 1);
 if (!out) return NULL;
 size_t j = 0;
 for (size_t i = 0; i < len; i += 3) {
  unsigned a = data[i];
  unsigned b = (i + 1 < len) ? data[i + 1] : 0;
  unsigned c = (i + 2 < len) ? data[i + 2] : 0;
  out[j++] = B64[a >> 2];
  out[j++] = B64[((a & 3u) << 4) | (b >> 4)];
  out[j++] = (i + 1 < len) ? B64[((b & 15u) << 2) | (c >> 6)] : '=';
  out[j++] = (i + 2 < len) ? B64[c & 63u] : '=';
 }
 out[j] = 0;
 return out;
}

static int b64_value(char c) {
 if (c >= 'A' && c <= 'Z') return c - 'A';
 if (c >= 'a' && c <= 'z') return c - 'a' + 26;
 if (c >= '0' && c <= '9') return c - '0' + 52;
 if (c == '+') return 62;
 if (c == '/') return 63;
 return -1;
}

static unsigned char *b64_decode(const char *in, size_t *out_len) {
 size_t len = strlen(in);
 size_t alloc = (len / 4 + 1) * 3;
 unsigned char *out = malloc(alloc ? alloc : 1);
 if (!out) return NULL;
 size_t j = 0;
 int quad[4];
 size_t q = 0;
 for (size_t i = 0; i < len; i++) {
  if (in[i] == '=' || in[i] == '\n' || in[i] == '\r' || in[i] == ' ') {
   if (in[i] != '=') continue;
   quad[q++] = -2;
  } else {
   int v = b64_value(in[i]);
   if (v < 0) continue;
   quad[q++] = v;
  }
  if (q == 4) {
   if (quad[0] < 0 || quad[1] < 0) break;
   out[j++] = (unsigned char)((quad[0] << 2) | (quad[1] >> 4));
   if (quad[2] >= 0) out[j++] = (unsigned char)(((quad[1] & 15) << 4) | (quad[2] >> 2));
   if (quad[3] >= 0) out[j++] = (unsigned char)(((quad[2] & 3) << 6) | quad[3]);
   q = 0;
  }
 }
 *out_len = j;
 return out;
}

static void mark_failed(const char *why) {
 worker_failed = 1;
 snprintf(last_error, sizeof(last_error), "%s", why ? why : "falha no worker de GPU");
 if (to_worker) { fclose(to_worker); to_worker = NULL; }
 if (from_worker) { fclose(from_worker); from_worker = NULL; }
}

static int ensure_worker(void) {
 if (to_worker && from_worker) return 0;
 if (worker_failed) return -1;

 char script[600];
 pifari_worker_script(script, sizeof(script));
 if (!script[0]) { mark_failed("tools/pifari_worker.py nao encontrado"); return -1; }

 const char *python = getenv("PIFARI_PYTHON");
 if (!python || !*python) python = "python.exe";

 int in_pipe[2], out_pipe[2];
 if (pipe(in_pipe) != 0) { mark_failed("pipe"); return -1; }
 if (pipe(out_pipe) != 0) { mark_failed("pipe"); return -1; }

 fflush(NULL);
 pid_t pid = fork();
 if (pid < 0) { mark_failed("fork"); return -1; }
 if (pid == 0) {
  dup2(in_pipe[0], 0);
  dup2(out_pipe[1], 1);
  close(in_pipe[0]); close(in_pipe[1]);
  close(out_pipe[0]); close(out_pipe[1]);
  execlp(python, python, script, "--stdio", (char *)NULL);
  _exit(127);
 }
 close(in_pipe[0]);
 close(out_pipe[1]);
 to_worker = fdopen(in_pipe[1], "w");
 from_worker = fdopen(out_pipe[0], "r");
 if (!to_worker || !from_worker) { mark_failed("fdopen"); return -1; }
 setvbuf(to_worker, NULL, _IOLBF, 0);
 worker_pid = pid;
 return 0;
}

int pifari_gpu_available(void) { return ensure_worker() == 0 ? 1 : 0; }

int pifari_gpu_affine_relu(const double *x, double *y, size_t n, double scale, double bias) {
 if (ensure_worker() != 0) return -1;
 if (!n) return 0;

 char *payload = b64_encode((const unsigned char *)x, n * sizeof(double));
 if (!payload) { mark_failed("memoria para base64"); return -1; }
 int wrote = fprintf(to_worker, "AFFINE %zu %.17g %.17g %s\n", n, scale, bias, payload) > 0;
 free(payload);
 if (!wrote || fflush(to_worker) != 0) { mark_failed("escrita no worker"); return -1; }

 char *line = NULL;
 size_t cap = 0;
 ssize_t got = getline(&line, &cap, from_worker);
 if (got <= 0) { free(line); mark_failed("worker encerrou a conexao"); return -1; }

 int rc = -1;
 if (!strncmp(line, "OK ", 3)) {
  size_t out_len = 0;
  unsigned char *raw = b64_decode(line + 3, &out_len);
  if (raw && out_len == n * sizeof(double)) {
   memcpy(y, raw, out_len);
   rc = 0;
  } else {
   snprintf(last_error, sizeof(last_error), "resposta com %zu bytes, esperado %zu", out_len, n * sizeof(double));
  }
  free(raw);
 } else if (!strncmp(line, "ERR ", 4)) {
  char *nl = strchr(line, '\n');
  if (nl) *nl = 0;
  snprintf(last_error, sizeof(last_error), "%s", line + 4);
 }
 free(line);
 return rc;
}

void pifari_gpu_shutdown(void) {
 if (to_worker) {
  fputs("QUIT\n", to_worker);
  fflush(to_worker);
  fclose(to_worker);
  to_worker = NULL;
 }
 if (from_worker) { fclose(from_worker); from_worker = NULL; }
 if (worker_pid > 0) {
  int status = 0;
  while (waitpid(worker_pid, &status, 0) < 0 && errno == EINTR) {}
  worker_pid = -1;
 }
}

const char *pifari_gpu_last_error(void) {
 return last_error[0] ? last_error : "sem erro";
}
