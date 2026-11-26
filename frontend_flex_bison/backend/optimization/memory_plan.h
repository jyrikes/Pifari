#ifndef PIFARI_MEMORY_PLAN_H
#define PIFARI_MEMORY_PLAN_H
#include <stddef.h>
/* Aloca slots temporários reutilizáveis a partir dos intervalos de vida [first,last]. */
typedef struct {
    size_t first, last, slot;
} PifariLiveRange;
size_t pifari_plan_slots(PifariLiveRange *ranges, size_t count);
#endif
