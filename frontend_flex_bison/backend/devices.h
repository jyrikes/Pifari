#ifndef PIFARI_DEVICES_H
#define PIFARI_DEVICES_H

#include <stddef.h>

/* Registro de dispositivos de execução (SPEC_CONCORRENCIA §7).
   cpu:0 sempre existe; GPU/remoto entram quando o worker responde. */

#define PIFARI_DEVICE_MAX 8

#define PIFARI_DEV_OPS_MAP    1
#define PIFARI_DEV_OPS_MATMUL 2
#define PIFARI_DEV_OPS_RELU   4
#define PIFARI_DEV_OPS_FP64   8

typedef struct {
 char id[32];
 char backend[16];
 char name[128];
 long long mem_mb;
 int ops;
 int workgroup;
 int units;
} PifariDevice;

/* Descobre os dispositivos. Idempotente e preguiçoso: só paga o custo do worker
   na primeira chamada. */
void pifari_devices_probe(void);
void pifari_devices_reset(void);

int pifari_devices_count(void);
const PifariDevice *pifari_devices_at(int index);

/* Retorna o índice do id ("cpu:0", "gpu:0", "auto") ou -1. */
int pifari_devices_find(const char *id);
int pifari_devices_select(const char *id);
int pifari_devices_selected_index(void);
const PifariDevice *pifari_devices_selected(void);

/* Converte /mnt/c/... em C:\... ; usado também por gpu_client.c. */
void pifari_to_windows_path(const char *src, char *dst, size_t capacity);

/* Caminho do tools/pifari_worker.py em formato Windows. */
void pifari_worker_script(char *dst, size_t capacity);

#endif
