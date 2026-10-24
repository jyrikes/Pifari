#define _GNU_SOURCE
#include "devices.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static PifariDevice devices[PIFARI_DEVICE_MAX];
static int device_count;
static int probed;
static int selected_index;

/* /mnt/c/Users/... -> C:\Users\... */
static void to_windows_path(const char *src, char *dst, size_t cap) {
 if (!src || !*src) {
  if (cap) dst[0] = 0;
  return;
 }
 if (!strncmp(src, "/mnt/", 5) && isalpha((unsigned char)src[5]) && src[6] == '/') {
  size_t j = 0;
  if (j + 3 < cap) {
   dst[j++] = (char)toupper((unsigned char)src[5]);
   dst[j++] = ':';
   dst[j++] = '\\';
  }
  for (size_t i = 7; src[i] && j + 1 < cap; i++) dst[j++] = (src[i] == '/') ? '\\' : src[i];
  dst[j] = 0;
  return;
 }
 snprintf(dst, cap, "%s", src);
}

static void add_cpu(void) {
 PifariDevice *d = &devices[device_count++];
 memset(d, 0, sizeof(*d));
 snprintf(d->id, sizeof(d->id), "cpu:0");
 snprintf(d->backend, sizeof(d->backend), "cpu");
 long cpus = sysconf(_SC_NPROCESSORS_ONLN);
 if (cpus < 1) cpus = 1;
 snprintf(d->name, sizeof(d->name), "CPU local (%ld threads)", cpus);
 d->mem_mb = 0;
 d->ops = PIFARI_DEV_OPS_MAP | PIFARI_DEV_OPS_MATMUL | PIFARI_DEV_OPS_RELU | PIFARI_DEV_OPS_FP64;
 d->workgroup = 0;
 d->units = (int)cpus;
}

static void parse_dev_line(char *line) {
 if (device_count >= PIFARI_DEVICE_MAX) return;
 char *save = NULL;
 char *tok = strtok_r(line, "\t\r\n", &save);
 if (!tok || strcmp(tok, "DEV")) return;

 PifariDevice *d = &devices[device_count];
 memset(d, 0, sizeof(*d));
 int field = 0;
 while ((tok = strtok_r(NULL, "\t\r\n", &save))) {
  field++;
  if (field == 1) snprintf(d->id, sizeof(d->id), "%s", tok);
  else if (field == 2) snprintf(d->backend, sizeof(d->backend), "%s", tok);
  else if (field == 3) snprintf(d->name, sizeof(d->name), "%s", tok);
  else if (field == 4) d->mem_mb = atoll(tok);
  else if (field == 5) {
   if (strstr(tok, "map")) d->ops |= PIFARI_DEV_OPS_MAP;
   if (strstr(tok, "matmul")) d->ops |= PIFARI_DEV_OPS_MATMUL;
   if (strstr(tok, "relu")) d->ops |= PIFARI_DEV_OPS_RELU;
  } else if (field == 6 && !strncmp(tok, "fp64=", 5)) {
   if (atoi(tok + 5)) d->ops |= PIFARI_DEV_OPS_FP64;
  } else if (field == 7 && !strncmp(tok, "wg=", 3)) d->workgroup = atoi(tok + 3);
  else if (field == 8 && !strncmp(tok, "cu=", 3)) d->units = atoi(tok + 3);
 }
 if (!d->id[0] || !d->backend[0]) return;
 device_count++;
}

static void probe_worker(void) {
 const char *python = getenv("PIFARI_PYTHON");
 if (!python || !*python) python = "python.exe";

 char script[600];
 const char *env_script = getenv("PIFARI_WORKER");
 if (env_script && *env_script) {
  snprintf(script, sizeof(script), "%s", env_script);
 } else {
  char cwd[512], win[512];
  if (!getcwd(cwd, sizeof(cwd))) return;
  to_windows_path(cwd, win, sizeof(win));
  snprintf(script, sizeof(script), "%s\\tools\\pifari_worker.py", win);
 }

 char cmd[1400];
 snprintf(cmd, sizeof(cmd), "%s \"%s\" --list 2>/dev/null", python, script);
 FILE *pipe = popen(cmd, "r");
 if (!pipe) return;
 char line[512];
 while (fgets(line, sizeof(line), pipe)) {
  if (!strncmp(line, "END", 3)) break;
  parse_dev_line(line);
 }
 pclose(pipe);
}

void pifari_devices_probe(void) {
 if (probed) return;
 probed = 1;
 device_count = 0;
 selected_index = 0;
 add_cpu();
 probe_worker();
}

void pifari_devices_reset(void) {
 probed = 0;
 device_count = 0;
 selected_index = 0;
}

int pifari_devices_count(void) { return device_count; }

const PifariDevice *pifari_devices_at(int index) {
 if (index < 0 || index >= device_count) return NULL;
 return &devices[index];
}

int pifari_devices_find(const char *id) {
 if (!id) return -1;
 if (!strcmp(id, "auto")) return 0;
 for (int i = 0; i < device_count; i++)
  if (!strcmp(devices[i].id, id)) return i;
 return -1;
}

int pifari_devices_select(const char *id) {
 int index = pifari_devices_find(id);
 if (index < 0) return -1;
 selected_index = index;
 return index;
}

int pifari_devices_selected_index(void) { return selected_index; }

const PifariDevice *pifari_devices_selected(void) {
 if (selected_index < 0 || selected_index >= device_count) return NULL;
 return &devices[selected_index];
}
