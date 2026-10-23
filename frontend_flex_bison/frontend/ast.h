#ifndef PIFARI_AST_H
#define PIFARI_AST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef enum {
 PIFARI_OP_NONE, PIFARI_OP_ADD, PIFARI_OP_SUB, PIFARI_OP_MUL, PIFARI_OP_DIV,
 PIFARI_OP_FLOORDIV, PIFARI_OP_MOD, PIFARI_OP_POW, PIFARI_OP_EQ, PIFARI_OP_NE,
 PIFARI_OP_LT, PIFARI_OP_LE, PIFARI_OP_GT, PIFARI_OP_GE, PIFARI_OP_AND, PIFARI_OP_OR
} PifariOp;
typedef enum { BC_INT, BC_FLOAT, BC_LOAD, BC_LOAD_SLOT, BC_NEG, BC_ADD, BC_SUB, BC_MUL, BC_DIV, BC_IDIV, BC_MOD, BC_POW, BC_STORE_SLOT, BC_RANGE_BEGIN, BC_RANGE_TEST, BC_INC_SLOT, BC_JUMP, BC_RETURN, BC_POP, BC_MADD_SLOTS, BC_EQ, BC_NE, BC_LT, BC_LE, BC_GT, BC_GE, BC_INDEX } ByteOp;
typedef struct { ByteOp op; long long i; double f; const char *name; const struct Node *slot_owner; size_t target, alternate; } ByteInstr;
typedef struct Node {
 char *kind,*value; struct Node **children; size_t count,capacity; int line,column;
 PifariOp op_code; long long integer_cache; double float_cache; unsigned char numeric_cached;
 unsigned char constant_folded, constant_is_float; long long constant_integer; double constant_float;
 int local_slot, target_slot; const struct Node *slot_owner, *target_owner;
 ByteInstr *bytecode; size_t bytecode_count; unsigned char bytecode_ready, bytecode_supported;
 ByteInstr *function_code; size_t function_code_count; unsigned char function_code_ready, function_code_supported;
} Node;
static inline char *pifari_strdup(const char*s){if(!s)return NULL;size_t n=strlen(s)+1;char*p=malloc(n);if(p)memcpy(p,s,n);return p;}
static inline Node *node_new(const char*k,const char*v,int l,int c){
 Node*n=calloc(1,sizeof(Node));if(!n){perror("malloc");exit(2);}
 n->local_slot=-1;n->target_slot=-1;
 n->kind=pifari_strdup(k);n->value=pifari_strdup(v);n->line=l;n->column=c;
 if(k&&v&&!strcmp(k,"Binary")){
  if(!strcmp(v,"+"))n->op_code=PIFARI_OP_ADD;else if(!strcmp(v,"-"))n->op_code=PIFARI_OP_SUB;
  else if(!strcmp(v,"*"))n->op_code=PIFARI_OP_MUL;else if(!strcmp(v,"/"))n->op_code=PIFARI_OP_DIV;
  else if(!strcmp(v,"//"))n->op_code=PIFARI_OP_FLOORDIV;else if(!strcmp(v,"%"))n->op_code=PIFARI_OP_MOD;
  else if(!strcmp(v,"**"))n->op_code=PIFARI_OP_POW;else if(!strcmp(v,"=="))n->op_code=PIFARI_OP_EQ;
  else if(!strcmp(v,"!="))n->op_code=PIFARI_OP_NE;else if(!strcmp(v,"<"))n->op_code=PIFARI_OP_LT;
  else if(!strcmp(v,"<="))n->op_code=PIFARI_OP_LE;else if(!strcmp(v,">"))n->op_code=PIFARI_OP_GT;
  else if(!strcmp(v,">="))n->op_code=PIFARI_OP_GE;else if(!strcmp(v,"and"))n->op_code=PIFARI_OP_AND;
  else if(!strcmp(v,"or"))n->op_code=PIFARI_OP_OR;
 }
 if(k&&v&&!strcmp(k,"Integer")){char*e;long long x=strtoll(v,&e,10);if(*e=='\0'){n->integer_cache=x;n->numeric_cached=1;}}
 if(k&&v&&!strcmp(k,"Float")){char*e;double x=strtod(v,&e);if(*e=='\0'){n->float_cache=x;n->numeric_cached=1;}}
 return n;
}
static inline void pifari_try_fold(Node*p){
 if(!p)return;
 if(!strcmp(p->kind,"Unary")&&p->count==1&&p->value&&!strcmp(p->value,"-")){
  Node*a=p->children[0];if(!a->numeric_cached&&!a->constant_folded)return;
  int ai=a->constant_folded?!a->constant_is_float:!strcmp(a->kind,"Integer");
  long long x=a->constant_folded?a->constant_integer:a->integer_cache;
  double d=a->constant_folded?a->constant_float:a->float_cache;
  if(ai&&x!=LLONG_MIN){p->constant_folded=1;p->constant_integer=-x;}
  else if(!ai){p->constant_folded=1;p->constant_is_float=1;p->constant_float=-d;}
  return;
 }
 if(strcmp(p->kind,"Binary")||p->count!=2)return;
 Node*a=p->children[0],*b=p->children[1];
 if((!a->numeric_cached&&!a->constant_folded)||(!b->numeric_cached&&!b->constant_folded))return;
 int ai=a->constant_folded?!a->constant_is_float:!strcmp(a->kind,"Integer");
 int bi=b->constant_folded?!b->constant_is_float:!strcmp(b->kind,"Integer");
 long long x=a->constant_folded?a->constant_integer:a->integer_cache;
 long long y=b->constant_folded?b->constant_integer:b->integer_cache,z=0;
 if(ai&&bi){
  int ok=1;
  switch(p->op_code){
   case PIFARI_OP_ADD:ok=!__builtin_add_overflow(x,y,&z);break;
   case PIFARI_OP_SUB:ok=!__builtin_sub_overflow(x,y,&z);break;
   case PIFARI_OP_MUL:ok=!__builtin_mul_overflow(x,y,&z);break;
   case PIFARI_OP_FLOORDIV:if(!y||(x==LLONG_MIN&&y==-1))ok=0;else z=x/y;break;
   case PIFARI_OP_MOD:if(!y||(x==LLONG_MIN&&y==-1))ok=0;else z=x%y;break;
   case PIFARI_OP_DIV:
    if(!y){ok=0;break;}
    p->constant_folded=1;p->constant_is_float=1;p->constant_float=(double)x/(double)y;return;
   default:ok=0;break;
  }
  if(ok){p->constant_folded=1;p->constant_integer=z;}
  return;
 }
 double dx=ai?(double)x:(a->constant_folded?a->constant_float:a->float_cache);
 double dy=bi?(double)y:(b->constant_folded?b->constant_float:b->float_cache);
 if(p->op_code==PIFARI_OP_DIV&&dy!=0.0){p->constant_folded=1;p->constant_is_float=1;p->constant_float=dx/dy;}
 else if(p->op_code==PIFARI_OP_ADD){p->constant_folded=1;p->constant_is_float=1;p->constant_float=dx+dy;}
 else if(p->op_code==PIFARI_OP_SUB){p->constant_folded=1;p->constant_is_float=1;p->constant_float=dx-dy;}
 else if(p->op_code==PIFARI_OP_MUL){p->constant_folded=1;p->constant_is_float=1;p->constant_float=dx*dy;}
}
static inline Node *node_add(Node*p,Node*c){
 if(!p||!c)return p;
 if(p->count==p->capacity){size_t z=p->capacity?p->capacity*2:4;Node**q=realloc(p->children,z*sizeof(Node*));if(!q){perror("realloc");exit(2);}p->children=q;p->capacity=z;}
 p->children[p->count++]=c;pifari_try_fold(p);return p;
}
static inline Node *node_pair(const char*k,const char*v,Node*a,Node*b,int l,int c){Node*n=node_new(k,v,l,c);node_add(n,a);node_add(n,b);return n;}
static inline void node_print(const Node*n,int d){if(!n)return;for(int i=0;i<d;i++)fputs("  ",stdout);printf("%s",n->kind?n->kind:"?");if(n->value)printf("(%s)",n->value);printf(" @%d:%d\n",n->line,n->column);for(size_t i=0;i<n->count;i++)node_print(n->children[i],d+1);}
static inline void node_free(Node*n){if(!n)return;for(size_t i=0;i<n->count;i++)node_free(n->children[i]);free(n->bytecode);free(n->function_code);free(n->children);free(n->kind);free(n->value);free(n);}
#endif
