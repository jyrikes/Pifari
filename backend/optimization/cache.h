#ifndef PIFARI_CACHE_H
#define PIFARI_CACHE_H

#include <stddef.h>

/* Diretório de cache persistente (etapa F4).
   Ordem: PIFARI_CACHE_DIR > $XDG_CACHE_HOME/pifari > $HOME/.cache/pifari.
   Só devolve caminho; a criação de subdiretório é feita por quem chama, com 0700.
   PIFARI_WARMUP=off faz as funções devolverem -1 (cache desligado). */
int pifari_cache_enabled(void);

/* Copia o diretório raiz do cache em dst. 0 = ok, -1 = indisponível/desligado. */
int pifari_cache_root(char *dst, size_t capacity);

/* Copia <raiz>/<sub> em dst, criando o diretório com 0700. 0 = ok. */
int pifari_cache_subdir(const char *sub, char *dst, size_t capacity);

#endif
