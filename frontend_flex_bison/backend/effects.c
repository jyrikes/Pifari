#include "effects.h"

#include <string.h>

const char *pifari_effect_name(PifariEffect effect) {
 switch (effect) {
  case EFFECT_PURE: return "puro";
  case EFFECT_READ: return "leitura";
  case EFFECT_WRITE: return "escrita";
  case EFFECT_IO: default: return "io";
 }
}

static int in_list(const char *name, const char *const *list) {
 for (size_t i = 0; list[i]; i++)
  if (!strcmp(name, list[i])) return 1;
 return 0;
}

PifariEffect pifari_builtin_effect(const char *name) {
 if (!name) return EFFECT_IO;

 static const char *const pure[] = {
  "len", "int", "float", "double", "str", "txt", "bool", "exp", "range",
  "sort", "binary_search", "solve_linear", "matmul", "f64_array",
  "map_affine_relu", "map_fused", "map_fused_chain", "map_indexed_native",
  NULL
 };
 if (in_list(name, pure)) return EFFECT_PURE;

 static const char *const read[] = {
  "agora_ms", "agora_ns", "thread_id", "dispositivos", "ler",
  NULL
 };
 if (in_list(name, read)) return EFFECT_READ;

 static const char *const write[] = {
  "array_set", "escrever", "incrementar", "compartilhado",
  "modo_processos", "modo_threads", "usar",
  NULL
 };
 if (in_list(name, write)) return EFFECT_WRITE;

 static const char *const io[] = {
  "print", "println", "sleep_ms", "cancelar", "spawn", "await",
  "Agent", "Task", "task_add_dep", "task_cancel", "task_result", "task_status",
  "dag_run", "policy_allow", "ctx_get", "ctx_put",
  "artifact_save", "telemetry_export", "telemetry_progress",
  "mcp_register", "mcp_call", "mcp_discover_tools", "mcp_discover_resources",
  "mcp_read_resource", "mcp_server_init", "mcp_server_handle_rpc",
  "mcp_server_register_tool", "mcp_server_register_resource",
  "mcp_server_run_stdio", "mcp_server_run_http",
  NULL
 };
 if (in_list(name, io)) return EFFECT_IO;

 return EFFECT_IO;
}
