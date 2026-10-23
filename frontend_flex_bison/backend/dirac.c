#include "dirac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PifariDiracExpr {
    PifariDiracKind kind;
    char *label;
    struct PifariDiracExpr *left;
    struct PifariDiracExpr *right;
};

static PifariDiracExpr *pifari_dirac_clone(const PifariDiracExpr *expr);

static char *copy_text(const char *text) {
    size_t length = strlen(text ? text : "") + 1;
    char *copy = malloc(length);
    if (copy) memcpy(copy, text ? text : "", length);
    return copy;
}

static PifariDiracExpr *make_node(PifariDiracKind kind,
                                  const PifariDiracExpr *left,
                                  const PifariDiracExpr *right,
                                  const char *label) {
    PifariDiracExpr *node = calloc(1, sizeof(*node));
    if (!node) return NULL;
    node->kind = kind;
    node->label = label ? copy_text(label) : NULL;
    if (label && !node->label) { free(node); return NULL; }
    if (left) {
        node->left = pifari_dirac_clone(left);
        if (!node->left) { free(node->label); free(node); return NULL; }
    }
    if (right) {
        node->right = pifari_dirac_clone(right);
        if (!node->right) {
            pifari_dirac_free(node->left);
            free(node->label); free(node); return NULL;
        }
    }
    return node;
}

static PifariDiracExpr *pifari_dirac_clone(const PifariDiracExpr *expr) {
    if (!expr) return NULL;
    return make_node(expr->kind, expr->left, expr->right, expr->label);
}

PifariDiracExpr *pifari_dirac_symbol(PifariDiracKind kind, const char *label) {
    if (kind != PIFARI_DIRAC_KET && kind != PIFARI_DIRAC_BRA &&
        kind != PIFARI_DIRAC_OPERATOR) return NULL;
    if (!label || !*label) return NULL;
    return make_node(kind, NULL, NULL, label);
}

PifariDiracExpr *pifari_dirac_sum(const PifariDiracExpr *left,
                                  const PifariDiracExpr *right) {
    if (!left || !right) return NULL;
    return make_node(PIFARI_DIRAC_SUM, left, right, NULL);
}

PifariDiracExpr *pifari_dirac_compose(const PifariDiracExpr *left,
                                      const PifariDiracExpr *right) {
    if (!left || !right) return NULL;
    PifariDiracKind kind = PIFARI_DIRAC_PRODUCT;
    if (left->kind == PIFARI_DIRAC_BRA && right->kind == PIFARI_DIRAC_KET)
        kind = PIFARI_DIRAC_INNER_PRODUCT;
    else if (left->kind == PIFARI_DIRAC_OPERATOR && right->kind == PIFARI_DIRAC_KET)
        kind = PIFARI_DIRAC_APPLY;
    return make_node(kind, left, right, NULL);
}

PifariDiracExpr *pifari_dirac_adjoint(const PifariDiracExpr *expr) {
    if (!expr) return NULL;
    if (expr->kind == PIFARI_DIRAC_KET)
        return pifari_dirac_symbol(PIFARI_DIRAC_BRA, expr->label);
    if (expr->kind == PIFARI_DIRAC_BRA)
        return pifari_dirac_symbol(PIFARI_DIRAC_KET, expr->label);
    if (expr->kind == PIFARI_DIRAC_PRODUCT) {
        PifariDiracExpr *right = pifari_dirac_adjoint(expr->right);
        PifariDiracExpr *left = pifari_dirac_adjoint(expr->left);
        if (!left || !right) {
            pifari_dirac_free(left); pifari_dirac_free(right); return NULL;
        }
        PifariDiracExpr *result = pifari_dirac_compose(right, left);
        pifari_dirac_free(left); pifari_dirac_free(right);
        return result;
    }
    return make_node(PIFARI_DIRAC_ADJOINT, expr, NULL, NULL);
}

static int format_into(const PifariDiracExpr *expr, char *out,
                       size_t capacity, size_t *used) {
    if (!expr) return -1;
    const char *open = "", *close = "", *symbol = "";
    switch (expr->kind) {
        case PIFARI_DIRAC_KET: open = "|"; close = ">"; symbol = expr->label; break;
        case PIFARI_DIRAC_BRA: open = "<"; close = "|"; symbol = expr->label; break;
        case PIFARI_DIRAC_OPERATOR: open = "{"; close = "}"; symbol = expr->label; break;
        case PIFARI_DIRAC_INNER_PRODUCT: {
            if (expr->left && expr->right &&
                expr->left->kind == PIFARI_DIRAC_BRA &&
                expr->right->kind == PIFARI_DIRAC_KET) {
                const char *bra = expr->left->label;
                const char *ket = expr->right->label;
                size_t a = strlen(bra), b = strlen(ket);
                if (*used + a + b + 4 >= capacity) return -1;
                out[(*used)++] = '<';
                memcpy(out + *used, bra, a); *used += a;
                out[(*used)++] = '|';
                memcpy(out + *used, ket, b); *used += b;
                out[(*used)++] = '>';
                out[*used] = '\0';
                return 0;
            }
            return -1;
        }
        case PIFARI_DIRAC_SUM:
        case PIFARI_DIRAC_PRODUCT:
        case PIFARI_DIRAC_APPLY: {
            if (*used + 1 >= capacity) return -1;
            out[(*used)++] = '(';
            if (format_into(expr->left, out, capacity, used) != 0) return -1;
            const char *op = expr->kind == PIFARI_DIRAC_SUM ? " + " :
                expr->kind == PIFARI_DIRAC_APPLY ? "" : " ";
            size_t length = strlen(op);
            if (*used + length >= capacity) return -1;
            memcpy(out + *used, op, length); *used += length;
            if (format_into(expr->right, out, capacity, used) != 0) return -1;
            if (*used + 2 >= capacity) return -1;
            out[(*used)++] = ')'; out[*used] = '\0';
            return 0;
        }
        case PIFARI_DIRAC_ADJOINT:
            if (*used + 1 >= capacity) return -1;
            out[(*used)++] = '(';
            if (format_into(expr->left, out, capacity, used) != 0) return -1;
            if (*used + 4 >= capacity) return -1;
            memcpy(out + *used, ")†", 4); *used += 4; out[*used] = '\0';
            return 0;
    }
    size_t a = strlen(open), b = strlen(symbol), c = strlen(close);
    if (*used + a + b + c >= capacity) return -1;
    memcpy(out + *used, open, a); *used += a;
    memcpy(out + *used, symbol, b); *used += b;
    memcpy(out + *used, close, c); *used += c;
    out[*used] = '\0';
    return 0;
}

char *pifari_dirac_format(const PifariDiracExpr *expr) {
    if (!expr) return NULL;
    size_t capacity = 4096, used = 0;
    char *text = calloc(capacity, 1);
    if (!text) return NULL;
    if (format_into(expr, text, capacity, &used) != 0) {
        free(text);
        return NULL;
    }
    return text;
}

void pifari_dirac_free(PifariDiracExpr *expr) {
    if (!expr) return;
    pifari_dirac_free(expr->left);
    pifari_dirac_free(expr->right);
    free(expr->label);
    free(expr);
}
