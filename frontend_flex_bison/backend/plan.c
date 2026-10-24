#include "plan.h"

#include <stdio.h>
#include <stdlib.h>

static int cached_level = -1;

int pifari_opt_level(void) {
 if (cached_level >= 0) return cached_level;
 const char *env = getenv("PIFARI_OPT");
 int level = 1;
 if (env && *env) {
  int parsed = atoi(env);
  if (parsed >= 0 && parsed <= 2) level = parsed;
 }
 cached_level = level;
 return cached_level;
}

int pifari_opt_debug(void) {
 const char *env = getenv("PIFARI_OPT_DEBUG");
 return env && *env && env[0] != '0';
}

const char *pifari_plan_name(PifariPlan plan) {
 switch (plan) {
  case PLAN_WALK: return "walk";
  case PLAN_BYTECODE: return "bytecode";
  case PLAN_JIT: return "jit";
  case PLAN_NATIVE: return "native";
  case PLAN_GPU: return "gpu";
  default: return "?";
 }
}

void pifari_plan_log(const char *what, const char *name, PifariPlan plan, const char *reason) {
 if (!pifari_opt_debug()) return;
 fprintf(stderr, "Pifari: plano: %s %s -> %s (%s)\n", what ? what : "?",
         name ? name : "?", pifari_plan_name(plan), reason ? reason : "");
}
