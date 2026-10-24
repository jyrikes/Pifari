#define _GNU_SOURCE
#include "typecheck.h"

#include "types.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Tabela de nomes visíveis. Uma para o escopo global e uma por função; blocos
   compartilham a tabela da função (aproximação conservadora: evita falso
   positivo de "usado antes de declarar" dentro de ramos condicionais). */
typedef struct {
 char **names;
 PifariType *kinds;
 unsigned char *typed;
 size_t count, cap;
} TcScope;

typedef struct TcFunction {
 const char *name;
 size_t params;
 int has_return_type;
 PifariType return_type;
 struct TcFunction *next;
} TcFunction;

typedef struct {
 TcScope globals;
 TcScope *current;
 TcFunction *functions;
 int errors;
 int loop_depth;
 int in_function;
} TcCtx;

static void tc_fail(TcCtx *ctx, const Node *n, const char *fmt, ...) {
 va_list ap;
 ctx->errors++;
 fprintf(stderr, "Pifari:%d:%d: erro semântico: ", n ? n->line : 1, n ? n->column : 1);
 va_start(ap, fmt);
 vfprintf(stderr, fmt, ap);
 va_end(ap);
 fputc('\n', stderr);
}

/* ---------- escopos ---------- */

static void scope_init(TcScope *s) { memset(s, 0, sizeof(*s)); }

static void scope_free(TcScope *s) {
 for (size_t i = 0; i < s->count; i++) free(s->names[i]);
 free(s->names);
 free(s->kinds);
 free(s->typed);
 memset(s, 0, sizeof(*s));
}

static long scope_find(TcScope *s, const char *name) {
 if (!name) return -1;
 for (size_t i = 0; i < s->count; i++)
  if (!strcmp(s->names[i], name)) return (long)i;
 return -1;
}

static void scope_add(TcScope *s, const char *name, PifariType kind, int typed) {
 if (!name || !*name) return;
 if (scope_find(s, name) >= 0) return;
 if (s->count == s->cap) {
  size_t cap = s->cap ? s->cap * 2 : 32;
  char **names = realloc(s->names, cap * sizeof(char *));
  if (!names) return;
  s->names = names;
  PifariType *kinds = realloc(s->kinds, cap * sizeof(PifariType));
  if (!kinds) return;
  s->kinds = kinds;
  unsigned char *typed = realloc(s->typed, cap);
  if (!typed) return;
  s->typed = typed;
  s->cap = cap;
 }
 s->names[s->count] = strdup(name);
 if (!s->names[s->count]) return;
 s->kinds[s->count] = kind;
 s->typed[s->count] = (unsigned char)(typed ? 1 : 0);
 s->count++;
}

static int scope_is_typed(TcScope *s, const char *name) {
 long i = scope_find(s, name);
 return i >= 0 && s->typed[i];
}

/* Declara em todos os escopos visíveis: usado para assinaturas e para
   atribuições, que em Pifari criam o nome no escopo onde ele não existe. */
static void declare_visible(TcCtx *ctx, const char *name, PifariType kind, int typed) {
 scope_add(ctx->current, name, kind, typed);
 if (ctx->current != &ctx->globals) scope_add(&ctx->globals, name, kind, typed);
}

static int is_declared(TcCtx *ctx, const char *name) {
 return scope_find(ctx->current, name) >= 0 || scope_find(&ctx->globals, name) >= 0;
}

/* ---------- tabela de funções ---------- */

static void collect_functions(TcCtx *ctx, const Node *n) {
 if (!n) return;
 if (!strcmp(n->kind, "Function") || !strcmp(n->kind, "FunctionExpr")) {
  TcFunction *f = calloc(1, sizeof(*f));
  if (f) {
   f->name = n->value;
   const Node *params = n->count ? n->children[0] : NULL;
   f->params = params ? params->count : 0;
   if (n->count > 2 && n->children[2] && !strcmp(n->children[2]->kind, "Type")) {
    f->has_return_type = 1;
    f->return_type = pifari_type_from_name(n->children[2]->value);
   }
   f->next = ctx->functions;
   ctx->functions = f;
  }
 }
 for (size_t i = 0; i < n->count; i++) collect_functions(ctx, n->children[i]);
}

static TcFunction *find_function(TcCtx *ctx, const char *name) {
 if (!name) return NULL;
 for (TcFunction *f = ctx->functions; f; f = f->next)
  if (!strcmp(f->name, name)) return f;
 return NULL;
}

/* ---------- expressões ---------- */

static PifariType literal_type(const Node *n) {
 if (!n) return PT_ANY;
 if (!strcmp(n->kind, "Integer")) return PT_INT;
 if (!strcmp(n->kind, "Float")) return PT_FLOAT;
 if (!strcmp(n->kind, "String")) return PT_STR;
 if (!strcmp(n->kind, "Boolean")) return PT_BOOL;
 if (!strcmp(n->kind, "Null")) return PT_NULL;
 return PT_ANY;
}

static void tc_expr(TcCtx *ctx, const Node *n);

/* Fluxos: o segundo >| / |> liga a variável de iteração; |< liga o agregado. */
static void tc_flow(TcCtx *ctx, const Node *n) {
 const char *op = n->value ? n->value : "";
 if (!strcmp(op, "|<")) {
  if (n->count > 0) tc_expr(ctx, n->children[0]);
  if (n->count > 1 && n->children[1] && !strcmp(n->children[1]->kind, "Name"))
   declare_visible(ctx, n->children[1]->value, PT_ANY, 0);
  return;
 }
 if (!strcmp(op, ">|") || !strcmp(op, "|>")) {
  const Node *source = n->count > 0 ? n->children[0] : NULL;
  const Node *target = n->count > 1 ? n->children[1] : NULL;
  if (source && !strcmp(source->kind, "Flow") && source->value && !strcmp(source->value, op) &&
      source->count >= 2 && source->children[1] && !strcmp(source->children[1]->kind, "Name")) {
   tc_expr(ctx, source->children[0]);
   declare_visible(ctx, source->children[1]->value, PT_ANY, 0);
   tc_expr(ctx, target);
   return;
  }
  tc_expr(ctx, source);
  tc_expr(ctx, target);
  return;
 }
 /* <| e desconhecidos */
 for (size_t i = 0; i < n->count; i++) tc_expr(ctx, n->children[i]);
}

static void tc_call(TcCtx *ctx, const Node *n) {
 const Node *args = n->count ? n->children[0] : NULL;
 size_t argc = args ? args->count : 0;
 TcFunction *fn = find_function(ctx, n->value);
 if (fn && fn->params != argc)
  tc_fail(ctx, n, "função '%s' espera %zu argumento(s), recebeu %zu", n->value, fn->params, argc);
 for (size_t i = 0; i < argc; i++) tc_expr(ctx, args->children[i]);
}

static void tc_expr(TcCtx *ctx, const Node *n) {
 if (!n) return;
 if (!strcmp(n->kind, "Name")) {
  if (!is_declared(ctx, n->value)) {
   if (ctx->in_function) {
    fprintf(stderr, "Pifari:%d:%d: aviso semântico: '%s' pode não estar declarada (ligação tardia de global)\n", n->line, n->column, n->value);
   } else tc_fail(ctx, n, "variável '%s' não foi declarada", n->value);
  }
  return;
 }
 if (!strcmp(n->kind, "Call")) { tc_call(ctx, n); return; }
 if (!strcmp(n->kind, "Flow")) { tc_flow(ctx, n); return; }
 if (!strcmp(n->kind, "Spawn")) {
  if (n->count < 1) { tc_fail(ctx, n, "spawn exige uma expressão"); return; }
  tc_expr(ctx, n->children[0]);
  return;
 }
 if (!strcmp(n->kind, "Function") || !strcmp(n->kind, "FunctionExpr")) return;
 for (size_t i = 0; i < n->count; i++) tc_expr(ctx, n->children[i]);
}

/* ---------- instruções ---------- */

static void tc_stmt(TcCtx *ctx, const Node *n);

static void tc_block(TcCtx *ctx, const Node *n) {
 if (!n) return;
 for (size_t i = 0; i < n->count; i++) tc_stmt(ctx, n->children[i]);
}

static void tc_function(TcCtx *ctx, const Node *n) {
 const Node *params = n->count ? n->children[0] : NULL;
 const Node *body = n->count > 1 ? n->children[1] : NULL;
 int has_ret = n->count > 2 && n->children[2] && !strcmp(n->children[2]->kind, "Type");
 PifariType ret = has_ret ? pifari_type_from_name(n->children[2]->value) : PT_ANY;
 if (has_ret && !pifari_type_is_known(n->children[2]->value))
  tc_fail(ctx, n->children[2], "tipo de retorno desconhecido: '%s'", n->children[2]->value);

 TcScope local;
 scope_init(&local);
 TcScope *saved = ctx->current;
 int saved_loop = ctx->loop_depth;
 ctx->current = &local;
 ctx->loop_depth = 0;
 ctx->in_function = 1;

 if (params) {
  for (size_t i = 0; i < params->count; i++) {
   const Node *p = params->children[i];
   PifariType t = PT_ANY;
   if (p && !strcmp(p->kind, "Param") && p->count && p->children[0] &&
       !strcmp(p->children[0]->kind, "Type")) {
    if (!pifari_type_is_known(p->children[0]->value))
     tc_fail(ctx, p, "tipo de parâmetro desconhecido: '%s'", p->children[0]->value);
    t = pifari_type_from_name(p->children[0]->value);
   }
   scope_add(&local, p ? p->value : NULL, t, 1);
  }
 }

 /* retorno literal incompatível com o tipo declarado */
 if (has_ret && body) {
  for (size_t i = 0; i < body->count; i++) {
   const Node *stmt = body->children[i];
   if (stmt && !strcmp(stmt->kind, "Return") && stmt->count) {
    PifariType lt = literal_type(stmt->children[0]);
    if (lt != PT_ANY && !pifari_type_assignable(ret, lt))
     tc_fail(ctx, stmt->children[0], "retorno incompatível: esperado %s, literal %s",
             pifari_type_name(ret), pifari_type_name(lt));
   }
  }
 }

 tc_block(ctx, body);

 ctx->current = saved;
 ctx->loop_depth = saved_loop;
 ctx->in_function = 0;
 scope_free(&local);
}

static void tc_stmt(TcCtx *ctx, const Node *n) {
 if (!n) return;
 if (!strcmp(n->kind, "Program") || !strcmp(n->kind, "Statements") || !strcmp(n->kind, "Block")) {
  tc_block(ctx, n);
  return;
 }
 if (!strcmp(n->kind, "Assign")) {
  const Node *rhs = n->count ? n->children[0] : NULL;
  tc_expr(ctx, rhs);
  if (!is_declared(ctx, n->value)) declare_visible(ctx, n->value, PT_ANY, 0);
  return;
 }
 if (!strcmp(n->kind, "TypedDecl")) {
  const Node *type_node = n->count ? n->children[0] : NULL;
  const Node *init = n->count > 1 ? n->children[1] : NULL;
  if (type_node && !strcmp(type_node->kind, "Type") && !pifari_type_is_known(type_node->value))
   tc_fail(ctx, type_node, "tipo desconhecido: '%s'", type_node->value);
  if (scope_is_typed(ctx->current, n->value))
   tc_fail(ctx, n, "'%s' já foi declarado com tipo neste escopo", n->value);
  PifariType ann = type_node ? pifari_type_from_name(type_node->value) : PT_ANY;
  if (init) {
   tc_expr(ctx, init);
   PifariType lt = literal_type(init);
   if (lt != PT_ANY && !pifari_type_assignable(ann, lt))
    tc_fail(ctx, init, "atribuição incompatível: '%s' espera %s, literal %s",
            n->value, pifari_type_name(ann), pifari_type_name(lt));
  }
  declare_visible(ctx, n->value, ann, 1);
  if (ctx->current != &ctx->globals) scope_add(&ctx->globals, n->value, ann, 1);
  return;
 }
 if (!strcmp(n->kind, "For")) {
  const Node *iterable = n->count ? n->children[0] : NULL;
  const Node *body = n->count > 1 ? n->children[1] : NULL;
  tc_expr(ctx, iterable);
  declare_visible(ctx, n->value, PT_ANY, 0);
  ctx->loop_depth++;
  tc_stmt(ctx, body);
  ctx->loop_depth--;
  return;
 }
 if (!strcmp(n->kind, "Break") || !strcmp(n->kind, "Continue")) {
  if (ctx->loop_depth == 0)
   tc_fail(ctx, n, "'%s' fora de laço", !strcmp(n->kind, "Break") ? "break" : "continue");
  return;
 }
 if (!strcmp(n->kind, "Return")) {
  if (n->count) tc_expr(ctx, n->children[0]);
  return;
 }
 if (!strcmp(n->kind, "Function") || !strcmp(n->kind, "FunctionExpr")) {
  if (!is_declared(ctx, n->value)) declare_visible(ctx, n->value, PT_ANY, 0);
  tc_function(ctx, n);
  return;
 }
 if (!strcmp(n->kind, "ExpressionStatement")) {
  if (n->count) tc_expr(ctx, n->children[0]);
  return;
 }
 if (!strcmp(n->kind, "If") || !strcmp(n->kind, "IfElse") || !strcmp(n->kind, "IfElseIf")) {
  /* estrutura: condições e blocos intercalados; recursão genérica resolve */
  for (size_t i = 0; i < n->count; i++) tc_stmt(ctx, n->children[i]);
  return;
 }
 if (!strcmp(n->kind, "IndexAssign")) {
  for (size_t i = 0; i < n->count; i++) tc_expr(ctx, n->children[i]);
  if (n->value && !is_declared(ctx, n->value))
   tc_fail(ctx, n, "array/lista '%s' não foi declarada", n->value);
  return;
 }
 if (!strcmp(n->kind, "Import") || !strcmp(n->kind, "Include") || !strcmp(n->kind, "LinkTo")) {
  tc_fail(ctx, n, "diretiva '%s' não é suportada nesta versão", n->kind);
  return;
 }
 if (!strcmp(n->kind, "Class")) {
  tc_fail(ctx, n, "classes não são suportadas nesta versão");
  return;
 }
 /* qualquer outro nó é tratado como expressão */
 tc_expr(ctx, n);
}

int pifari_typecheck_program(const Node *program) {
 TcCtx ctx;
 memset(&ctx, 0, sizeof(ctx));
 scope_init(&ctx.globals);
 ctx.current = &ctx.globals;
 collect_functions(&ctx, program);
 tc_block(&ctx, program);
 int errors = ctx.errors;
 TcFunction *f = ctx.functions;
 while (f) { TcFunction *next = f->next; free(f); f = next; }
 scope_free(&ctx.globals);
 return errors;
}
