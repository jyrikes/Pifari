#include "../backend/dirac.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    PifariDiracExpr *ket = pifari_dirac_symbol(PIFARI_DIRAC_KET, "psi");
    PifariDiracExpr *bra = pifari_dirac_adjoint(ket);
    PifariDiracExpr *inner = pifari_dirac_compose(bra, ket);
    char *text = pifari_dirac_format(inner);
    assert(text && strcmp(text, "<psi|psi>") == 0);
    free(text);

    PifariDiracExpr *a = pifari_dirac_symbol(PIFARI_DIRAC_OPERATOR, "A");
    PifariDiracExpr *b = pifari_dirac_symbol(PIFARI_DIRAC_OPERATOR, "B");
    PifariDiracExpr *product = pifari_dirac_compose(a, b);
    PifariDiracExpr *adjoint = pifari_dirac_adjoint(product);
    text = pifari_dirac_format(adjoint);
    assert(text && strstr(text, "†") != NULL);
    free(text);

    pifari_dirac_free(ket);
    pifari_dirac_free(bra);
    pifari_dirac_free(inner);
    pifari_dirac_free(a);
    pifari_dirac_free(b);
    pifari_dirac_free(product);
    pifari_dirac_free(adjoint);
    puts("Dirac symbolic algebra: OK");
    return 0;
}
