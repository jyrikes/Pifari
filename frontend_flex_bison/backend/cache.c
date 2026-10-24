#define _GNU_SOURCE
#include "cache.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int pifari_cache_enabled(void) {
 const char *env = getenv("PIFARI_WARMUP");
 if (env && (!strcmp(env, "off") || !strcmp(env, "0") || !strcmp(env, "false"))) return 0;
 return 1;
}

int pifari_cache_root(char *dst, size_t capacity) {
 if (!dst || capacity < 8) return -1;
 if (!pifari_cache_enabled()) return -1;
 const char *explicit_dir = getenv("PIFARI_CACHE_DIR");
 if (explicit_dir && *explicit_dir) {
  snprintf(dst, capacity, "%s", explicit_dir);
  return 0;
 }
 const char *xdg = getenv("XDG_CACHE_HOME");
 if (xdg && *xdg) {
  snprintf(dst, capacity, "%s/pifari", xdg);
  return 0;
 }
 const char *home = getenv("HOME");
 if (!home || !*home) return -1;
 snprintf(dst, capacity, "%s/.cache/pifari", home);
 return 0;
}

int pifari_cache_subdir(const char *sub, char *dst, size_t capacity) {
 char root[512];
 if (pifari_cache_root(root, sizeof(root)) != 0) return -1;
 snprintf(dst, capacity, "%s/%s", root, sub ? sub : "");
 /* cria a raiz e o subdiretório; ambos 0700 */
 char partial[512];
 snprintf(partial, sizeof(partial), "%s", root);
 for (char *p = partial + 1; *p; p++) {
  if (*p != '/') continue;
  *p = 0;
  mkdir(partial, 0700);
  *p = '/';
 }
 mkdir(partial, 0700);
 if (sub && *sub) {
  if (mkdir(dst, 0700) != 0 && errno != EEXIST) return -1;
 }
 struct stat st;
 if (stat(dst, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid()) return -1;
 chmod(dst, 0700);
 return 0;
}
