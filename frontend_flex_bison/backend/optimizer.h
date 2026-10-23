#ifndef PIFARI_OPTIMIZER_H
#define PIFARI_OPTIMIZER_H
#include <stddef.h>
typedef enum { PEX_INPUT, PEX_CONST, PEX_ALIAS, PEX_ADD, PEX_SUB, PEX_MUL, PEX_DIV, PEX_NEG, PEX_MAX0 } PifariExprOp;
typedef struct { PifariExprOp op; int left,right; double constant; } PifariExprNode;
typedef struct { PifariExprNode *nodes; size_t count,capacity; int root; } PifariExprGraph;
void pifari_graph_init(PifariExprGraph *g);
void pifari_graph_destroy(PifariExprGraph *g);
int pifari_graph_add(PifariExprGraph *g,PifariExprOp op,int left,int right,double constant);
/* Nodes are appended after their dependencies; optimizer folds safe constants. */
int pifari_graph_optimize(PifariExprGraph *g);
int pifari_graph_eval(const PifariExprGraph *g,double input,double *out);
/* Compõe dois grafos unários: out(x) = second(first(x)). */
int pifari_graph_compose(const PifariExprGraph *first, const PifariExprGraph *second, PifariExprGraph *out);
/* Quantidade de slots temporarios exigidos apos planejamento de vida util. */
size_t pifari_graph_workspace_slots(const PifariExprGraph *g);
/* Fused graph evaluation: one reusable workspace, no recursion or per-node arrays. */
int pifari_graph_map(const PifariExprGraph *g,const double *input,double *output,size_t count);
/* Parallel map with disjoint output ranges; workers is clamped to [1,32]. */
int pifari_graph_map_parallel(const PifariExprGraph *g,const double *input,double *output,size_t count,unsigned workers);
#endif
