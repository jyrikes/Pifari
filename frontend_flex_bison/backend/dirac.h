#ifndef PIFARI_DIRAC_H
#define PIFARI_DIRAC_H

#include <stddef.h>

typedef enum {
    PIFARI_DIRAC_KET,
    PIFARI_DIRAC_BRA,
    PIFARI_DIRAC_OPERATOR,
    PIFARI_DIRAC_SUM,
    PIFARI_DIRAC_PRODUCT,
    PIFARI_DIRAC_ADJOINT,
    PIFARI_DIRAC_INNER_PRODUCT,
    PIFARI_DIRAC_APPLY
} PifariDiracKind;

typedef struct PifariDiracExpr PifariDiracExpr;

PifariDiracExpr *pifari_dirac_symbol(PifariDiracKind kind, const char *label);
PifariDiracExpr *pifari_dirac_sum(const PifariDiracExpr *left,
                                  const PifariDiracExpr *right);
PifariDiracExpr *pifari_dirac_compose(const PifariDiracExpr *left,
                                      const PifariDiracExpr *right);
PifariDiracExpr *pifari_dirac_adjoint(const PifariDiracExpr *expr);
char *pifari_dirac_format(const PifariDiracExpr *expr);
void pifari_dirac_free(PifariDiracExpr *expr);

#endif
