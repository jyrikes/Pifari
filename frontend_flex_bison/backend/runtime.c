#define _GNU_SOURCE
#include "runtime.h"
#include "operator_engine.h"
#include "ast_ir.h"
#include "numeric_buffer.h"
#include "native_kernel.h"
#include "agent_runtime.h"
#include "mcp_server.h"
#include "devices.h"
#include "gpu_client.h"
#include "types.h"
#include "effects.h"
#include <errno.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>

typedef enum { V_NULL, V_INT, V_FLOAT, V_BOOL, V_STRING, V_LIST, V_RANGE, V_F64_ARRAY } ValueType;
typedef struct Value Value;
typedef struct F64ArrayStorage { size_t refs, length; double *data; } F64ArrayStorage;
struct Value { ValueType type; union { long long i; double f; int b; char *s; struct { Value *items; size_t count; } list; long long range_end; F64ArrayStorage *array; } as; };
static int compare_numeric_values(const void *pa,const void *pb){
 const Value *a=(const Value*)pa,*b=(const Value*)pb;
 long double x=a->type==V_INT?(long double)a->as.i:(long double)a->as.f;
 long double y=b->type==V_INT?(long double)b->as.i:(long double)b->as.f;
 return (x>y)-(x<y);
}
typedef struct { size_t refs, capacity; Value items[]; } ListStorage;
static ListStorage *list_storage(Value v){return (ListStorage*)((char*)v.as.list.items - offsetof(ListStorage,items));}
static Value list_new(size_t count){
 Value v={.type=V_LIST};if(count>(SIZE_MAX-sizeof(ListStorage))/sizeof(Value)){v.type=V_NULL;return v;}
 ListStorage*s=calloc(1,sizeof(ListStorage)+(count?count:1)*sizeof(Value));
 if(!s){v.type=V_NULL;return v;}s->refs=1;s->capacity=count?count:1;v.as.list.items=s->items;v.as.list.count=count;return v;
}
static Value f64_array_new(size_t length){
 Value v={.type=V_F64_ARRAY};F64ArrayStorage*a=calloc(1,sizeof(*a));
 if(!a){v.type=V_NULL;return v;}a->refs=1;a->length=length;
 if(length){if(length>SIZE_MAX/sizeof(double)){free(a);v.type=V_NULL;return v;}void*p=NULL;if(posix_memalign(&p,64,length*sizeof(double))!=0){free(a);v.type=V_NULL;return v;}a->data=p;memset(a->data,0,length*sizeof(double));}
 v.as.array=a;return v;
}
typedef struct Binding { char *name; Value value; PifariType kind; struct Binding *next, *hash_next; } Binding;
typedef struct LocalFrame { Binding *slots; size_t count; Binding *buckets[256]; struct LocalFrame *parent; const Node *owner; } LocalFrame;
static _Thread_local LocalFrame *local_frame;
typedef struct FunctionDef {
 const char *name;
 const Node *node;
 struct FunctionDef *next;
 void *jit_handle;
 long long (*jit_fn)(long long);
 int jit_requires_range;
 PifariExprGraph ir;
 int ir_ready;
 unsigned effects; unsigned char effects_ready;
 PifariNativeKernel *native_kernel;
 PifariExprGraph indexed_ir;
 int indexed_ir_ready;
 PifariNativeKernel *indexed_kernel;
 char **slot_names;
 size_t slot_count;
 int slots_ready;
} FunctionDef;
static FunctionDef *functions;
typedef struct ChainKernelCache { char *key; PifariNativeKernel *kernel; struct ChainKernelCache *next; } ChainKernelCache;
static ChainKernelCache *chain_kernel_cache;
static pthread_mutex_t chain_kernel_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static _Thread_local Binding *globals;
#define BINDING_BUCKETS 1024u
static _Thread_local Binding *binding_buckets[BINDING_BUCKETS];
static _Thread_local int runtime_failed;
static _Thread_local int return_active;
static _Thread_local Value return_value;
/* Estado de concorrência por thread: identifica quem já é trabalhador do pool
   (para não recursar no pool e não mutar AST compartilhada) e carrega a última
   mensagem de erro desta thread para atribuição por item. */
static _Thread_local int in_worker;
static _Thread_local int cancel_requested;
static _Thread_local char error_message[256];
/* Modo de distribuição dos fluxos (runtime, não sintaxe — SPEC §7.4).
   0 = threads; 1 = processos. Só pode virar processos antes de o pool de
   threads existir, porque fork() com threads vivas herda locks em uso. */
static int execution_processes;
static size_t execution_workers;
static int thread_pool_active;
#define PIFARI_PROCESS_TIMEOUT_MS 60000
#define PIFARI_MAX_PROCESSES 64
#define PIFARI_MAX_SHARED 64
/* Optional counters: disabled by default so normal execution pays only one branch. */
static int profiling_enabled;
static unsigned long long profile_eval_nodes, profile_binding_lookups, profile_value_copies, profile_bytecode_ops;
static void profile_inc(unsigned long long *counter){if(profiling_enabled)__atomic_add_fetch(counter,1,__ATOMIC_RELAXED);}

static Value eval(const Node*n);
static void exec_node(const Node*n);
/* Tarefas: implementadas depois de clone_bindings, mas usadas por eval_call. */
static long long pifari_task_spawn(const Node *expr);
static int pifari_task_cancel(long long id);
static Value pifari_task_await(const Node *at, long long id);
static void pifari_tasks_shutdown(void);
static int pifari_task_cancelled(void);
static long long pifari_shared_new(Value v);
static int pifari_shared_write(long long id, Value v);
static Value pifari_shared_read(long long id, int *ok);
static Value pifari_shared_add(long long id, double delta, int *ok);
static void pifari_shared_shutdown(void);
#define PIFARI_TASK_CANCELLED() pifari_task_cancelled()
static Binding *clone_bindings(Binding *src);
static Binding *find_binding(const char *name);
static Binding *node_target_binding(const Node *n, const char *name);
static Value call_user_function(const Node *call, FunctionDef *fn_def);
static char *runtime_invoke_mcp_tool_cb(const char *fn_name, size_t argc, const char **argv, int *out_is_error);
static void jit_compile_function(FunctionDef *f);
static void free_value(Value v);
static void fail(const Node *n,const char *fmt,...);
static pthread_mutex_t pifari_output_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *dup_string(const char *s){size_t n=strlen(s?s:"")+1;char *p=malloc(n);if(!p){perror("malloc");exit(2);}memcpy(p,s?s:"",n);return p;}
static Value vnull(void){ Value v={.type=V_NULL}; return v; }
static Value vint(long long n){ Value v={.type=V_INT};v.as.i=n;return v; }
static Value vfloat(double n){ Value v={.type=V_FLOAT};v.as.f=n;return v; }
static Value vbool(int n){ Value v={.type=V_BOOL};v.as.b=!!n;return v; }
static Value vstr(const char*s){ Value v={.type=V_STRING};v.as.s=dup_string(s?s:"");return v; }
static long long monotonic_ms(void){struct timespec ts;if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0)return 0;return (long long)ts.tv_sec*1000LL+ts.tv_nsec/1000000LL;}
static long long monotonic_ns(void){struct timespec ts;if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0)return 0;return (long long)ts.tv_sec*1000000000LL+ts.tv_nsec;}
/* Conta threads vivas: fork() com mais de uma thread pode herdar mutex preso
   por outra thread, então o modo processos exige processo single-thread. */
static int pifari_live_threads(void){
 FILE*f=fopen("/proc/self/status","r");
 if(!f)return 1;
 char line[256];int n=1;
 while(fgets(line,sizeof(line),f))if(!strncmp(line,"Threads:",8)){n=atoi(line+8);break;}
 fclose(f);
 return n;
}
static void fail(const Node*n,const char*fmt,...){
 if(runtime_failed)return;
 runtime_failed=1;
 va_list ap;va_start(ap,fmt);
 vsnprintf(error_message,sizeof(error_message),fmt,ap);
 va_end(ap);
 fprintf(stderr,"Pifari:%d:%d: erro de execução: ",n?n->line:1,n?n->column:1);
 fputs(error_message,stderr);fputc('\n',stderr);
}
static PifariType value_type_of(Value v){
 switch(v.type){
  case V_INT:return PT_INT;
  case V_FLOAT:return PT_FLOAT;
  case V_BOOL:return PT_BOOL;
  case V_STRING:return PT_STR;
  case V_LIST:return PT_LIST;
  case V_F64_ARRAY:return PT_F64ARRAY;
  case V_RANGE:return PT_RANGE;
  case V_NULL:return PT_NULL;
  default:return PT_ANY;
 }
}
static void free_value(Value v){
 if(v.type==V_STRING)free(v.as.s);
 if(v.type==V_F64_ARRAY&&v.as.array&&__atomic_sub_fetch(&v.as.array->refs,1,__ATOMIC_ACQ_REL)==0){free(v.as.array->data);free(v.as.array);}
 if(v.type==V_LIST&&v.as.list.items){
  ListStorage*s=list_storage(v);
  if(__atomic_sub_fetch(&s->refs,1,__ATOMIC_ACQ_REL)==0){
   for(size_t i=0;i<v.as.list.count;i++)free_value(v.as.list.items[i]);
   free(s);
  }
 }
}
static Value copy_value(Value v){
 profile_inc(&profile_value_copies);
 if(v.type==V_STRING)return vstr(v.as.s);
 if(v.type==V_F64_ARRAY&&v.as.array)__atomic_add_fetch(&v.as.array->refs,1,__ATOMIC_RELAXED);
 if(v.type==V_LIST&&v.as.list.items)__atomic_add_fetch(&list_storage(v)->refs,1,__ATOMIC_RELAXED);
 return v;
}
static size_t binding_hash(const char *s){
 uint64_t h=UINT64_C(1469598103934665603);
 for(;*s;s++){h^=(unsigned char)*s;h*=UINT64_C(1099511628211);}
 return (size_t)(h&(BINDING_BUCKETS-1));
}
static void rebuild_binding_index(void){
 memset(binding_buckets,0,sizeof(binding_buckets));
 for(Binding*b=globals;b;b=b->next){size_t h=binding_hash(b->name);b->hash_next=binding_buckets[h];binding_buckets[h]=b;}
}
static Binding *find_binding(const char*n){
 profile_inc(&profile_binding_lookups);
 size_t h=binding_hash(n);
 for(LocalFrame*f=local_frame;f;f=f->parent)
  for(Binding*b=f->buckets[h&255u];b;b=b->hash_next)if(!strcmp(b->name,n))return b;
 for(Binding*b=binding_buckets[h];b;b=b->hash_next)if(!strcmp(b->name,n))return b;
 return NULL;
}
static void set_binding(const char*n,Value v,const Node*at,int require_existing){
 Binding*b=find_binding(n);
 if(!b&&require_existing){fail(at,"variável '%s' não foi declarada",n);free_value(v);return;}
 if(!b){b=calloc(1,sizeof(*b));if(!b){fail(at,"memória insuficiente ao criar variável");free_value(v);return;}b->name=dup_string(n);b->next=globals;globals=b;size_t h=binding_hash(n);b->hash_next=binding_buckets[h];binding_buckets[h]=b;}else free_value(b->value);
 b->value=v;
}
static Binding *set_loop_binding(Binding *b,const char *name,Value v,const Node *at){
 if(b){free_value(b->value);b->value=v;return b;}
 set_binding(name,v,at,0);
 return find_binding(name);
}
static PifariNativeKernel *chain_kernel_get(Value names, const PifariExprGraph *graph) {
 size_t key_len=1;
 for(size_t i=0;i<names.as.list.count;i++)key_len+=strlen(names.as.list.items[i].as.s)+1;
 char *key=malloc(key_len); if(!key)return NULL; key[0]=0;
 size_t pos=0;for(size_t i=0;i<names.as.list.count;i++){if(i)key[pos++]=(char)31;size_t n=strlen(names.as.list.items[i].as.s);memcpy(key+pos,names.as.list.items[i].as.s,n);pos+=n;}key[pos]=0;
 pthread_mutex_lock(&chain_kernel_cache_mutex);
 for(ChainKernelCache *it=chain_kernel_cache;it;it=it->next)if(!strcmp(it->key,key)){PifariNativeKernel *k=it->kernel;pthread_mutex_unlock(&chain_kernel_cache_mutex);free(key);return k;}
 PifariNativeKernel *kernel=NULL;
 if(pifari_native_kernel_create(graph,&kernel)!=0){pthread_mutex_unlock(&chain_kernel_cache_mutex);free(key);return NULL;}
 ChainKernelCache *entry=calloc(1,sizeof(*entry));
 if(!entry){pifari_native_kernel_destroy(kernel);pthread_mutex_unlock(&chain_kernel_cache_mutex);free(key);return NULL;}
 entry->key=key;entry->kernel=kernel;entry->next=chain_kernel_cache;chain_kernel_cache=entry;
 pthread_mutex_unlock(&chain_kernel_cache_mutex);return kernel;
}
static void chain_kernel_cache_clear(void) {
 pthread_mutex_lock(&chain_kernel_cache_mutex);
 ChainKernelCache *it=chain_kernel_cache;chain_kernel_cache=NULL;
 pthread_mutex_unlock(&chain_kernel_cache_mutex);
 while(it){ChainKernelCache *next=it->next;pifari_native_kernel_destroy(it->kernel);free(it->key);free(it);it=next;}
}
static int truthy(Value v){switch(v.type){case V_NULL:return 0;case V_BOOL:return v.as.b;case V_INT:return v.as.i!=0;case V_FLOAT:return v.as.f!=0.0;case V_STRING:return v.as.s[0]!=0;case V_LIST:return v.as.list.count!=0;case V_RANGE:return v.as.range_end>0;case V_F64_ARRAY:return v.as.array&&v.as.array->length>0;}return 0;}
static double number(Value v,const Node*n){if(v.type==V_INT)return (double)v.as.i;if(v.type==V_FLOAT)return v.as.f;fail(n,"esperado número, recebido tipo %d",v.type);return 0;}
static char *stringify(Value v){
 char buf[128];
 switch(v.type){
 case V_NULL:return dup_string("Null");
 case V_BOOL:return dup_string(v.as.b?"true":"false");
 case V_INT:snprintf(buf,sizeof buf,"%lld",v.as.i);return dup_string(buf);
 case V_FLOAT:snprintf(buf,sizeof buf,"%.15g",v.as.f);return dup_string(buf);
 case V_STRING:return dup_string(v.as.s);
 case V_F64_ARRAY:{size_t cap=32,len=0;char*out=malloc(cap);if(!out)return dup_string("[]");out[len++]='[';for(size_t i=0;i<v.as.array->length;i++){char x[64];int z=snprintf(x,sizeof x,"%s%.15g",i?",":"",v.as.array->data[i]);if(len+(size_t)z+2>cap){cap=(len+(size_t)z+2)*2;char*q=realloc(out,cap);if(!q){free(out);return dup_string("[]");}out=q;}memcpy(out+len,x,(size_t)z);len+=(size_t)z;}out[len++]=']';out[len]=0;return out;}
 case V_LIST:{size_t cap=32,len=0;char*out=malloc(cap);out[len++]='[';for(size_t i=0;i<v.as.list.count;i++){char*x=stringify(v.as.list.items[i]);size_t z=strlen(x);if(len+z+4>cap){cap=(len+z+4)*2;out=realloc(out,cap);}if(i)out[len++]=',';memcpy(out+len,x,z);len+=z;free(x);}out[len++]=']';out[len]=0;return out;}
 case V_RANGE:{size_t cap=32,len=0;char*out=malloc(cap);out[len++]='[';for(long long i=0;i<v.as.range_end;i++){char x[32];int z=snprintf(x,sizeof x,"%s%lld",i?",":"",i);if(len+(size_t)z+2>cap){cap=(len+(size_t)z+2)*2;out=realloc(out,cap);}memcpy(out+len,x,(size_t)z);len+=(size_t)z;}out[len++]=']';out[len]=0;return out;}
 }return dup_string("?");
}
static int equal_values(Value a,Value b){
 if(a.type==b.type){switch(a.type){
  case V_NULL:return 1;
  case V_BOOL:return a.as.b==b.as.b;
  case V_INT:return a.as.i==b.as.i;
  case V_FLOAT:return a.as.f==b.as.f;
  case V_STRING:return !strcmp(a.as.s,b.as.s);
  case V_LIST:
   if(a.as.list.count!=b.as.list.count)return 0;
   for(size_t i=0;i<a.as.list.count;i++)if(!equal_values(a.as.list.items[i],b.as.list.items[i]))return 0;
   return 1;
  case V_RANGE:return a.as.range_end==b.as.range_end;
  case V_F64_ARRAY:if(!a.as.array||!b.as.array||a.as.array->length!=b.as.array->length)return 0;for(size_t i=0;i<a.as.array->length;i++)if(a.as.array->data[i]!=b.as.array->data[i])return 0;return 1;
 }}
 if((a.type==V_INT||a.type==V_FLOAT)&&(b.type==V_INT||b.type==V_FLOAT))return number(a,NULL)==number(b,NULL);
 return 0;
}
static LocalFrame *find_owner_frame(const Node*owner){
 for(LocalFrame*f=local_frame;f;f=f->parent)if(f->owner==owner)return f;
 return NULL;
}
typedef struct { ByteInstr *code; size_t count,cap; int supported; } ByteBuilder;
static int bc_emit(ByteBuilder *b, ByteInstr ins){
 if(b->count==b->cap){size_t nc=b->cap?b->cap*2:8;ByteInstr*q=realloc(b->code,nc*sizeof(*q));if(!q)return 0;b->code=q;b->cap=nc;}
 b->code[b->count++]=ins;return 1;
}
static int bc_compile_node(ByteBuilder*b,const Node*n){
 if(!n)return 0;
 ByteInstr ins={0};
 if(!strcmp(n->kind,"Integer")&&n->numeric_cached){ins.op=BC_INT;ins.i=n->integer_cache;return bc_emit(b,ins);}
 if(!strcmp(n->kind,"Float")&&n->numeric_cached){ins.op=BC_FLOAT;ins.f=n->float_cache;return bc_emit(b,ins);}
 if(!strcmp(n->kind,"Name")){
  if(n->local_slot>=0&&n->slot_owner){ins.op=BC_LOAD_SLOT;ins.i=n->local_slot;ins.slot_owner=n->slot_owner;}
  else {ins.op=BC_LOAD;ins.name=n->value;}
  return bc_emit(b,ins);
 }
 if(!strcmp(n->kind,"Unary")&&n->count==1&&!strcmp(n->value,"-")){if(!bc_compile_node(b,n->children[0]))return 0;ins.op=BC_NEG;return bc_emit(b,ins);}
 if(!strcmp(n->kind,"Binary")&&n->count==2){
  switch(n->op_code){case PIFARI_OP_ADD:ins.op=BC_ADD;break;case PIFARI_OP_SUB:ins.op=BC_SUB;break;case PIFARI_OP_MUL:ins.op=BC_MUL;break;case PIFARI_OP_DIV:ins.op=BC_DIV;break;case PIFARI_OP_FLOORDIV:ins.op=BC_IDIV;break;case PIFARI_OP_MOD:ins.op=BC_MOD;break;case PIFARI_OP_POW:ins.op=BC_POW;break;case PIFARI_OP_EQ:ins.op=BC_EQ;break;case PIFARI_OP_NE:ins.op=BC_NE;break;case PIFARI_OP_LT:ins.op=BC_LT;break;case PIFARI_OP_LE:ins.op=BC_LE;break;case PIFARI_OP_GT:ins.op=BC_GT;break;case PIFARI_OP_GE:ins.op=BC_GE;break;default:return 0;}
  if(!bc_compile_node(b,n->children[0])||!bc_compile_node(b,n->children[1]))return 0;
  return bc_emit(b,ins);
 }
 if(!strcmp(n->kind,"Index")&&n->count==2){
  if(!bc_compile_node(b,n->children[0])||!bc_compile_node(b,n->children[1]))return 0;
  ins.op=BC_INDEX;return bc_emit(b,ins);
 }
 if(!strcmp(n->kind,"Call")&&n->value&&!strcmp(n->value,"exp")&&n->count==1&&n->children[0]&&n->children[0]->count==1){
  if(!bc_compile_node(b,n->children[0]->children[0]))return 0;
  ins.op=BC_EXP;return bc_emit(b,ins);
 }
 return 0;
}
static pthread_mutex_t bytecode_compile_mutex = PTHREAD_MUTEX_INITIALIZER;
static int bc_ensure(const Node*n){
 Node*m=(Node*)n;
 /* Hot path: compiled bytecode is published once and then read without locking. */
 if(__atomic_load_n(&m->bytecode_ready,__ATOMIC_ACQUIRE))
  return m->bytecode_supported;
 pthread_mutex_lock(&bytecode_compile_mutex);
 if(__atomic_load_n(&m->bytecode_ready,__ATOMIC_RELAXED)){
  int ok=m->bytecode_supported;
  pthread_mutex_unlock(&bytecode_compile_mutex);
  return ok;
 }
 ByteBuilder b={0};
 if(bc_compile_node(&b,n)){
  m->bytecode=b.code;
  m->bytecode_count=b.count;
  m->bytecode_supported=1;
 }else{
  free(b.code);
  m->bytecode_supported=0;
 }
 __atomic_store_n(&m->bytecode_ready,1,__ATOMIC_RELEASE);
 int ok=m->bytecode_supported;
 pthread_mutex_unlock(&bytecode_compile_mutex);
 return ok;
}
static Value eval_bytecode(const Node*n){
 size_t cap=n->bytecode_count+1,sp=0;
 /* Small expression stacks stay on the C stack; large ones use the heap. */
 Value local_stack[32];
 Value*stack=cap<=32?local_stack:malloc(cap*sizeof(Value));
 if(!stack){fail(n,"memória insuficiente na VM");return vnull();}
 for(size_t pc=0;pc<n->bytecode_count&&!runtime_failed&&!PIFARI_TASK_CANCELLED();pc++){
  profile_inc(&profile_bytecode_ops);
  ByteInstr in=n->bytecode[pc];Value a,b,r=vnull();
  switch(in.op){
   case BC_INT:stack[sp++]=vint(in.i);break;
   case BC_FLOAT:stack[sp++]=vfloat(in.f);break;
   case BC_LOAD:{Binding*x=find_binding(in.name);if(!x){fail(n,"variável '%s' não foi declarada",in.name);break;}stack[sp++]=copy_value(x->value);break;}
   case BC_LOAD_SLOT:{LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count){fail(n,"slot local inválido na VM");break;}stack[sp++]=copy_value(f->slots[in.i].value);break;}
   case BC_NEG:if(!sp){fail(n,"bytecode inválido");break;}a=stack[--sp];if(a.type==V_INT)r=vint(-a.as.i);else r=vfloat(-number(a,n));free_value(a);stack[sp++]=r;break;
   default:
    if(sp<2){fail(n,"bytecode inválido");break;}
    b=stack[--sp];a=stack[--sp];
    if(in.op==BC_INDEX){
     if(b.type!=V_INT||(a.type!=V_LIST&&a.type!=V_RANGE&&a.type!=V_F64_ARRAY)){free_value(a);free_value(b);fail(n,"indexação na VM exige lista, array tipado/range e índice inteiro");break;}
     long long length=a.type==V_LIST?(long long)a.as.list.count:a.type==V_F64_ARRAY?(long long)a.as.array->length:a.as.range_end;
     long long index=b.as.i;if(index<0)index+=length;free_value(b);
     if(index<0||index>=length){free_value(a);fail(n,"índice fora dos limites");break;}
     r=a.type==V_RANGE?vint(index):a.type==V_F64_ARRAY?vfloat(a.as.array->data[index]):copy_value(a.as.list.items[index]);free_value(a);stack[sp++]=r;break;
    }
    if(in.op==BC_EQ||in.op==BC_NE){int eq=equal_values(a,b);r=vbool(in.op==BC_EQ?eq:!eq);free_value(a);free_value(b);stack[sp++]=r;break;}
    if(in.op==BC_LT||in.op==BC_LE||in.op==BC_GT||in.op==BC_GE){
     int cmp;if(a.type==V_STRING&&b.type==V_STRING)cmp=strcmp(a.as.s,b.as.s);else if((a.type==V_INT||a.type==V_FLOAT)&&(b.type==V_INT||b.type==V_FLOAT)){double x=number(a,n),y=number(b,n);cmp=(x>y)-(x<y);}else{free_value(a);free_value(b);fail(n,"comparação na VM exige operandos compatíveis");break;}
     r=vbool(in.op==BC_LT?cmp<0:in.op==BC_LE?cmp<=0:in.op==BC_GT?cmp>0:cmp>=0);free_value(a);free_value(b);stack[sp++]=r;break;
    }
    if((a.type!=V_INT&&a.type!=V_FLOAT)||(b.type!=V_INT&&b.type!=V_FLOAT)){free_value(a);free_value(b);fail(n,"a VM aritmética exige operandos numéricos");break;}
    if(in.op==BC_ADD&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_add_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na soma");else r=vint(z);}
    else if(in.op==BC_SUB&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_sub_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na subtracao");else r=vint(z);}
    else if(in.op==BC_MUL&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_mul_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na multiplicacao");else r=vint(z);}
    else {double x=number(a,n),y=number(b,n);if(in.op==BC_ADD)r=vfloat(x+y);else if(in.op==BC_SUB)r=vfloat(x-y);else if(in.op==BC_MUL)r=vfloat(x*y);else if(in.op==BC_DIV){if(y==0)fail(n,"divisão por zero");else r=vfloat(x/y);}else if(in.op==BC_IDIV){if(y==0)fail(n,"divisão por zero");else r=(a.type==V_INT&&b.type==V_INT)?vint(a.as.i/b.as.i):vint((long long)(x/y));}else if(in.op==BC_MOD){if(y==0)fail(n,"divisão por zero");else r=vint((long long)x%(long long)y);}else if(in.op==BC_POW)r=vfloat(pow(x,y));}
    free_value(a);free_value(b);if(!runtime_failed)stack[sp++]=r;else free_value(r);break;
  }
 }
 Value out=vnull();
 if(!runtime_failed&&sp){
  out=stack[sp-1];
  for(size_t i=0;i+1<sp;i++)free_value(stack[i]);
 }else{
  for(size_t i=0;i<sp;i++)free_value(stack[i]);
 }
 if(stack!=local_stack)free(stack);
 return out;
}
static Value eval_binary(const Node*n){
 Value a=eval(n->children[0]);
 if(runtime_failed){free_value(a);return vnull();}
 if(n->op_code==PIFARI_OP_AND){
  int result=truthy(a);free_value(a);
  if(!result)return vbool(0);
  Value b=eval(n->children[1]);result=truthy(b);free_value(b);return vbool(result);
 }
 if(n->op_code==PIFARI_OP_OR){
  int result=truthy(a);free_value(a);
  if(result)return vbool(1);
  Value b=eval(n->children[1]);result=truthy(b);free_value(b);return vbool(result);
 }
 Value b=eval(n->children[1]);
 if(runtime_failed){free_value(a);free_value(b);return vnull();}
 Value result=vnull();int valid=1;
 const char*op=n->value;
 if(n->op_code==PIFARI_OP_EQ)result=vbool(equal_values(a,b));
 else if(n->op_code==PIFARI_OP_NE)result=vbool(!equal_values(a,b));
 else if(n->op_code==PIFARI_OP_ADD&&a.type==V_LIST&&b.type==V_LIST){
  if(a.as.list.count>SIZE_MAX-b.as.list.count)fail(n,"lista concatenada grande demais");
  else {
   size_t total=a.as.list.count+b.as.list.count;
   result=list_new(total);
   if(result.type!=V_LIST){result=vnull();fail(n,"memória insuficiente na concatenação de listas");}
   else {
    for(size_t i=0;i<a.as.list.count;i++)result.as.list.items[i]=copy_value(a.as.list.items[i]);
    for(size_t i=0;i<b.as.list.count;i++)result.as.list.items[a.as.list.count+i]=copy_value(b.as.list.items[i]);
   }
  }
 } else if(n->op_code==PIFARI_OP_ADD&&(a.type==V_STRING||b.type==V_STRING)){
  char*x=stringify(a),*y=stringify(b);size_t z=strlen(x)+strlen(y)+1;char*s=malloc(z);
  if(!s){fail(n,"memória insuficiente na concatenação");}
  else {snprintf(s,z,"%s%s",x,y);result=vstr(s);free(s);}
  free(x);free(y);
 } else if(n->op_code==PIFARI_OP_LT||n->op_code==PIFARI_OP_LE||n->op_code==PIFARI_OP_GT||n->op_code==PIFARI_OP_GE){
  int cmp;
  if(a.type==V_STRING&&b.type==V_STRING)cmp=strcmp(a.as.s,b.as.s);
  else {double x=number(a,n),y=number(b,n);cmp=(x>y)-(x<y);}
  if(n->op_code==PIFARI_OP_LT)result=vbool(cmp<0);
  else if(n->op_code==PIFARI_OP_LE)result=vbool(cmp<=0);
  else if(n->op_code==PIFARI_OP_GT)result=vbool(cmp>0);
  else result=vbool(cmp>=0);
 } else if(a.type==V_INT&&b.type==V_INT&&n->op_code!=PIFARI_OP_DIV&&n->op_code!=PIFARI_OP_POW){
  long long x=a.as.i,y=b.as.i,z=0;
  if(n->op_code==PIFARI_OP_ADD){if(__builtin_add_overflow(x,y,&z))fail(n,"overflow de inteiro na soma");else result=vint(z);}
  else if(n->op_code==PIFARI_OP_SUB){if(__builtin_sub_overflow(x,y,&z))fail(n,"overflow de inteiro na subtracao");else result=vint(z);}
  else if(n->op_code==PIFARI_OP_MUL){if(__builtin_mul_overflow(x,y,&z))fail(n,"overflow de inteiro na multiplicacao");else result=vint(z);}
  else if(n->op_code==PIFARI_OP_FLOORDIV||n->op_code==PIFARI_OP_MOD){
   if(!y)fail(n,"divisão por zero");else result=vint(n->op_code==PIFARI_OP_FLOORDIV?x/y:x%y);
  } else valid=0;
 } else {
  double x=number(a,n),y=number(b,n);
  if(n->op_code==PIFARI_OP_ADD)result=vfloat(x+y);
  else if(n->op_code==PIFARI_OP_SUB)result=vfloat(x-y);
  else if(n->op_code==PIFARI_OP_MUL)result=vfloat(x*y);
  else if(n->op_code==PIFARI_OP_DIV){if(y==0)fail(n,"divisão por zero");else result=vfloat(x/y);}
  else if(n->op_code==PIFARI_OP_FLOORDIV||n->op_code==PIFARI_OP_MOD){
   if(y==0)fail(n,"divisão por zero");else result=vint(n->op_code==PIFARI_OP_FLOORDIV?(long long)floor(x/y):(long long)x%(long long)y);
  } else if(n->op_code==PIFARI_OP_POW)result=vfloat(pow(x,y));else valid=0;
 }
 free_value(a);free_value(b);
 if(!valid&&!runtime_failed)fail(n,"operador binário não suportado: %s",op);
 if(runtime_failed){free_value(result);return vnull();}
 return result;
}
static Value eval_call(const Node*n){
 const char*name=n->value?n->value:"";
 for(FunctionDef *f=functions;f;f=f->next)if(!strcmp(f->name,name))return call_user_function(n,f);
 const Node*args=n->count?n->children[0]:NULL;size_t count=args?args->count:0;
 if(!strcmp(name,"modo_processos")){
  if(count!=1){fail(n,"modo_processos espera a quantidade de processos");return vnull();}
  Value wv=eval(args->children[0]);if(runtime_failed){free_value(wv);return vnull();}
  if(wv.type!=V_INT||wv.as.i<1||wv.as.i>PIFARI_MAX_PROCESSES){
   free_value(wv);fail(n,"modo_processos espera inteiro entre 1 e %d",PIFARI_MAX_PROCESSES);return vnull();
  }
  long long nproc=wv.as.i;free_value(wv);
  int threads=pifari_live_threads();
  if(threads>1||thread_pool_active){
   fail(n,"modo_processos exige ser chamado antes de o runtime criar threads (há %d ativa(s)); fork com threads herdaria locks em uso",threads);
   return vnull();
  }
  execution_processes=1;execution_workers=(size_t)nproc;return vint(nproc);
 }
 if(!strcmp(name,"modo_threads")){
  if(count!=0){fail(n,"modo_threads não espera argumentos");return vnull();}
  execution_processes=0;execution_workers=0;return vnull();
 }
 if(!strcmp(name,"compartilhado")){
  if(count!=1){fail(n,"compartilhado espera o valor inicial");return vnull();}
  Value v=eval(args->children[0]);
  if(runtime_failed){free_value(v);return vnull();}
  long long id=pifari_shared_new(v);
  if(id<0){free_value(v);fail(n,"limite de %d células compartilhadas atingido",PIFARI_MAX_SHARED);return vnull();}
  return vint(id);
 }
 if(!strcmp(name,"ler")||!strcmp(name,"escrever")||!strcmp(name,"incrementar")){
  const int is_read=!strcmp(name,"ler"),is_write=!strcmp(name,"escrever");
  if(count<(is_read?1:2)){fail(n,"%s espera a célula%s",name,is_read?"":" e o valor");return vnull();}
  Value idv=eval(args->children[0]);
  if(runtime_failed){free_value(idv);return vnull();}
  if(idv.type!=V_INT){free_value(idv);fail(n,"%s espera o identificador devolvido por compartilhado",name);return vnull();}
  long long id=idv.as.i;free_value(idv);
  int ok=1;
  if(is_read){
   Value out=pifari_shared_read(id,&ok);
   if(!ok){fail(n,"célula compartilhada inválida");return vnull();}
   return out;
  }
  Value v=eval(args->children[1]);
  if(runtime_failed){free_value(v);return vnull();}
  if(is_write){
   if(pifari_shared_write(id,v)!=0){fail(n,"célula compartilhada inválida");return vnull();}
   return vnull();
  }
  if(v.type!=V_INT&&v.type!=V_FLOAT){free_value(v);fail(n,"incrementar espera um delta numérico");return vnull();}
  double delta=v.type==V_INT?(double)v.as.i:v.as.f;free_value(v);
  Value out=pifari_shared_add(id,delta,&ok);
  if(!ok){fail(n,"incrementar exige célula compartilhada com valor numérico");return vnull();}
  return out;
 }
 if(!strcmp(name,"cancelar")){
  if(count!=1){fail(n,"cancelar espera o identificador da tarefa");return vnull();}
  Value idv=eval(args->children[0]);
  if(idv.type!=V_INT){free_value(idv);fail(n,"cancelar espera o identificador devolvido por spawn");return vnull();}
  long long id=idv.as.i;free_value(idv);
  if(pifari_task_cancel(id)<0){fail(n,"tarefa inválida");return vnull();}
  return vnull();
 }
 if(!strcmp(name,"efeitos")){
  if(count!=1){fail(n,"efeitos espera o nome da função");return vnull();}
  Value nv=eval(args->children[0]);
  if(runtime_failed){free_value(nv);return vnull();}
  if(nv.type!=V_STRING){free_value(nv);fail(n,"efeitos espera texto com o nome da função");return vnull();}
  FunctionDef*fn=NULL;
  for(FunctionDef*f=functions;f;f=f->next)if(!strcmp(f->name,nv.as.s)){fn=f;break;}
  PifariEffect effect=fn?(fn->effects_ready?fn->effects:(PifariEffect)EFFECT_IO):pifari_builtin_effect(nv.as.s);
  free_value(nv);
  return vstr(pifari_effect_name(effect));
 }
 if(!strcmp(name,"dispositivos")){
  if(count!=0){fail(n,"dispositivos não espera argumentos");return vnull();}
  pifari_devices_probe();
  int total=pifari_devices_count();
  Value out=list_new((size_t)(total>0?total:0));
  if(out.type!=V_LIST){fail(n,"memória insuficiente ao listar dispositivos");return vnull();}
  for(int i=0;i<total;i++){
   const PifariDevice*d=pifari_devices_at(i);
   Value rec=list_new(5);
   if(rec.type!=V_LIST){free_value(out);fail(n,"memória insuficiente ao listar dispositivos");return vnull();}
   rec.as.list.items[0]=vstr(d->id);
   rec.as.list.items[1]=vstr(d->backend);
   rec.as.list.items[2]=vstr(d->name);
   rec.as.list.items[3]=vint(d->mem_mb);
   rec.as.list.items[4]=vint((long long)d->ops);
   out.as.list.items[i]=rec;
  }
  return out;
 }
 if(!strcmp(name,"usar")){
  if(count!=1){fail(n,"usar espera o id do dispositivo");return vnull();}
  Value idv=eval(args->children[0]);
  if(runtime_failed){free_value(idv);return vnull();}
  if(idv.type!=V_STRING){free_value(idv);fail(n,"usar espera texto, por exemplo usar(\"gpu:0\")");return vnull();}
  pifari_devices_probe();
  int index=pifari_devices_select(idv.as.s);
  if(index<0){
   free_value(idv);
   fail(n,"dispositivo não encontrado; use dispositivos() para listar (cpu:0 sempre existe)");
   return vnull();
  }
  free_value(idv);
  return vnull();
 }
 if(!strcmp(name,"array_set")){
  if(count!=3){fail(n,"array_set espera array, índice e valor");return vnull();}
  if(!strcmp(args->children[0]->kind,"Name")){
   Binding *b=node_target_binding(args->children[0],args->children[0]->value);
   if(b&&b->value.type==V_F64_ARRAY&&b->value.as.array){
    Value idx=eval(args->children[1]);if(runtime_failed){free_value(idx);return vnull();}
    Value val=eval(args->children[2]);if(runtime_failed){free_value(idx);free_value(val);return vnull();}
    if(idx.type!=V_INT||(val.type!=V_INT&&val.type!=V_FLOAT)){free_value(idx);free_value(val);fail(n,"array_set exige array f64, índice inteiro e valor numérico");return vnull();}
    long long i=idx.as.i;size_t length=b->value.as.array->length;if(i<0)i+=(long long)length;
    if(i<0||(size_t)i>=length){free_value(idx);free_value(val);fail(n,"índice fora dos limites");return vnull();}
    b->value.as.array->data[i]=number(val,n);free_value(idx);free_value(val);return vnull();
   }
  }
  Value arr=eval(args->children[0]);if(runtime_failed){free_value(arr);return vnull();}
  Value idx=eval(args->children[1]);if(runtime_failed){free_value(arr);free_value(idx);return vnull();}
  Value val=eval(args->children[2]);if(runtime_failed){free_value(arr);free_value(idx);free_value(val);return vnull();}
  if(arr.type!=V_F64_ARRAY||!arr.as.array||idx.type!=V_INT||(val.type!=V_INT&&val.type!=V_FLOAT)){free_value(arr);free_value(idx);free_value(val);fail(n,"array_set exige array f64, índice inteiro e valor numérico");return vnull();}
  long long i=idx.as.i;size_t length=arr.as.array->length;if(i<0)i+=(long long)length;
  if(i<0||(size_t)i>=length){free_value(arr);free_value(idx);free_value(val);fail(n,"índice fora dos limites");return vnull();}
  arr.as.array->data[i]=number(val,n);free_value(arr);free_value(idx);free_value(val);return vnull();
 }
 if(!strcmp(name,"exp")){if(count!=1){fail(n,"exp espera um argumento");return vnull();}Value v=eval(args->children[0]);if(runtime_failed)return vnull();if(v.type!=V_INT&&v.type!=V_FLOAT){free_value(v);fail(n,"exp exige argumento numérico");return vnull();}double x=number(v,n);free_value(v);return vfloat(exp(x));}
 if(!strcmp(name,"print")||!strcmp(name,"println")){char **parts=calloc(count?count:1,sizeof(char*));for(size_t i=0;i<count;i++){Value v=eval(args->children[i]);if(runtime_failed){for(size_t j=0;j<i;j++)free(parts[j]);free(parts);return vnull();}parts[i]=stringify(v);}pthread_mutex_lock(&pifari_output_mutex);for(size_t i=0;i<count;i++){fputs(parts[i],stdout);if(i+1<count)fputc(' ',stdout);free(parts[i]);}fputc('\n',stdout);fflush(stdout);pthread_mutex_unlock(&pifari_output_mutex);free(parts);return vnull();}
 if(!strcmp(name,"agora_ms")){if(count!=0){fail(n,"agora_ms não recebe argumentos");return vnull();}return vint(monotonic_ms());}
 if(!strcmp(name,"agora_ns")){if(count!=0){fail(n,"agora_ns não recebe argumentos");return vnull();}return vint(monotonic_ns());}
 if(!strcmp(name,"thread_id")){if(count!=0){fail(n,"thread_id não recebe argumentos");return vnull();}return vint((long long)(uintptr_t)pthread_self());}
 if(!strcmp(name,"sleep_ms")){if(count!=1){fail(n,"sleep_ms espera um argumento: milissegundos");return vnull();}Value v=eval(args->children[0]);if(runtime_failed)return vnull();if(v.type!=V_INT&&v.type!=V_FLOAT){fail(n,"sleep_ms exige milissegundos numéricos");return vnull();}double ms=number(v,n);if(ms<0||ms>86400000.0){fail(n,"sleep_ms aceita valores entre 0 e 86400000 ms");return vnull();}struct timespec req={(time_t)((long long)ms/1000),(long)((long long)ms%1000)*1000000L};while(nanosleep(&req,&req)!=0){if(errno!=EINTR){fail(n,"falha em sleep_ms: %s",strerror(errno));return vnull();}}return vnull();}
 if(!strcmp(name,"map_fused")){
  if(count<2||count>3||!args||strcmp(args->children[0]->kind,"Name")){
   fail(n,"map_fused espera nome_da_funcao, lista numerica e workers opcional");return vnull();
  }
  FunctionDef *target=NULL;
  for(FunctionDef *f=functions;f;f=f->next)
   if(!strcmp(f->name,args->children[0]->value)){target=f;break;}
  if(!target){fail(n,"map_fused: funcao '%s' nao encontrada",args->children[0]->value);return vnull();}
  Value seq=eval(args->children[1]);
  if(runtime_failed){free_value(seq);return vnull();}
  if(seq.type!=V_LIST){free_value(seq);fail(n,"map_fused exige uma lista numerica");return vnull();}
  unsigned workers=1;
  if(count==3){
   Value w=eval(args->children[2]);
   if(runtime_failed){free_value(seq);free_value(w);return vnull();}
   if(w.type!=V_INT||w.as.i<1||w.as.i>32){
    free_value(seq);free_value(w);fail(n,"workers deve ser inteiro entre 1 e 32");return vnull();
   }
   workers=(unsigned)w.as.i;free_value(w);
  }
  PifariExprGraph local_graph;
  const PifariExprGraph *graph=NULL;
  int owns_graph=0;
  char ir_error[160];
  if(target->ir_ready){
   graph=&target->ir;
  }else if(pifari_ast_function_to_ir(target->node,&local_graph,ir_error,sizeof(ir_error))==0){
   graph=&local_graph;owns_graph=1;
  }else{
   free_value(seq);fail(n,"map_fused: nao foi possivel compilar funcao para IR: %s",ir_error);return vnull();
  }
  size_t len=seq.as.list.count;
  PifariF64Buffer input={0},output={0};
  if(pifari_f64_buffer_init(&input,len)!=0 ||
     pifari_f64_buffer_init(&output,len)!=0){
   pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
   if(owns_graph) pifari_graph_destroy(&local_graph);
   free_value(seq);
   fail(n,"map_fused: memoria insuficiente para buffers F64");return vnull();
  }
  int valid=1;
  for(size_t i=0;i<len;i++){
   Value x=seq.as.list.items[i];
   if(x.type!=V_INT&&x.type!=V_FLOAT){valid=0;break;}
   input.data[i]=number(x,n);
  }
  int map_status=-1;
  if(target->ir_ready&&!target->native_kernel)
   (void)pifari_native_kernel_create(&target->ir,&target->native_kernel);
  if(valid){
   if(target->native_kernel)
    map_status=pifari_native_kernel_map(target->native_kernel,input.data,output.data,len,workers);
   else
    map_status=pifari_graph_map_parallel(graph,input.data,output.data,len,workers);
  }
  if(map_status!=0){
   pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
   if(owns_graph) pifari_graph_destroy(&local_graph);
   free_value(seq);
   fail(n,valid?"map_fused: falha ao executar kernel nativo/IR":"map_fused aceita somente elementos numericos");return vnull();
  }
  Value result=list_new(len);
  if(result.type==V_LIST)for(size_t i=0;i<len;i++)result.as.list.items[i]=vfloat(output.data[i]);
  pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
  if(owns_graph) pifari_graph_destroy(&local_graph);
   free_value(seq);
  if(result.type!=V_LIST){fail(n,"map_fused: memoria insuficiente no resultado");return vnull();}
  return result;
 }
 if(!strcmp(name,"map_indexed_native")){
  if(count<2||count>3||!args){fail(n,"map_indexed_native espera nome_da_funcao, array_f64 e workers opcional");return vnull();}
  Value fn_name=eval(args->children[0]);if(runtime_failed){free_value(fn_name);return vnull();}
  Value seq=eval(args->children[1]);if(runtime_failed){free_value(fn_name);free_value(seq);return vnull();}
  if(fn_name.type!=V_STRING||seq.type!=V_F64_ARRAY||!seq.as.array){free_value(fn_name);free_value(seq);fail(n,"map_indexed_native exige nome de função string e array F64");return vnull();}
  unsigned workers=1;
  if(count==3){Value w=eval(args->children[2]);if(runtime_failed){free_value(fn_name);free_value(seq);free_value(w);return vnull();}if(w.type!=V_INT||w.as.i<1||w.as.i>32){free_value(fn_name);free_value(seq);free_value(w);fail(n,"workers deve ser inteiro entre 1 e 32");return vnull();}workers=(unsigned)w.as.i;free_value(w);}
  FunctionDef *target=NULL;for(FunctionDef*f=functions;f;f=f->next)if(!strcmp(f->name,fn_name.as.s)){target=f;break;}
  if(!target){fail(n,"função indexada '%s' não encontrada",fn_name.as.s);free_value(fn_name);free_value(seq);return vnull();}
  char ir_error[160];
  if(!target->indexed_ir_ready){if(pifari_ast_indexed_function_to_ir(target->node,&target->indexed_ir,ir_error,sizeof(ir_error))!=0){free_value(fn_name);free_value(seq);fail(n,"map_indexed_native: %s",ir_error);return vnull();}target->indexed_ir_ready=1;}
  if(!target->indexed_kernel&&pifari_native_kernel_create(&target->indexed_ir,&target->indexed_kernel)!=0){free_value(fn_name);free_value(seq);fail(n,"map_indexed_native: compilação do kernel nativo falhou");return vnull();}
  size_t len=seq.as.array->length;Value result=f64_array_new(len);
  if(result.type!=V_F64_ARRAY){free_value(fn_name);free_value(seq);fail(n,"map_indexed_native: memória insuficiente no resultado");return vnull();}
  int status=pifari_native_kernel_map(target->indexed_kernel,seq.as.array->data,result.as.array->data,len,workers);
  free_value(fn_name);free_value(seq);
  if(status!=0){free_value(result);fail(n,status==-2?"map_indexed_native: divisão por zero":"map_indexed_native: falha no kernel nativo");return vnull();}
  return result;
 }
 if(!strcmp(name,"map_fused_chain")){
  if(count<2||count>3||!args){fail(n,"map_fused_chain espera lista_de_funcoes, dados e workers opcional");return vnull();}
  Value names=eval(args->children[0]); if(runtime_failed){free_value(names);return vnull();}
  Value seq=eval(args->children[1]); if(runtime_failed){free_value(names);free_value(seq);return vnull();}
  if(names.type!=V_LIST||names.as.list.count==0||(seq.type!=V_LIST&&seq.type!=V_RANGE&&seq.type!=V_F64_ARRAY)){free_value(names);free_value(seq);fail(n,"map_fused_chain exige lista de nomes de funções e lista/range de dados");return vnull();}
  unsigned workers=1;
  if(count==3){Value w=eval(args->children[2]);if(runtime_failed){free_value(names);free_value(seq);free_value(w);return vnull();}if(w.type!=V_INT||w.as.i<1||w.as.i>32){free_value(names);free_value(seq);free_value(w);fail(n,"workers deve ser inteiro entre 1 e 32");return vnull();}workers=(unsigned)w.as.i;free_value(w);}
  PifariExprGraph composed={0}; int have_graph=0,ok=1; char ir_error[160];
  for(size_t i=0;i<names.as.list.count&&ok;i++){
   Value fn=names.as.list.items[i]; if(fn.type!=V_STRING){ok=0;snprintf(ir_error,sizeof ir_error,"cada nome na cadeia deve ser string");break;}
   FunctionDef *target=NULL;for(FunctionDef*f=functions;f;f=f->next)if(!strcmp(f->name,fn.as.s)){target=f;break;}
   if(!target){ok=0;snprintf(ir_error,sizeof ir_error,"função '%s' não encontrada",fn.as.s);break;}
   PifariExprGraph next={0};
   if(pifari_ast_function_to_ir(target->node,&next,ir_error,sizeof ir_error)!=0){ok=0;break;}
   if(!have_graph){composed=next;have_graph=1;}
   else {PifariExprGraph merged={0};int rc=pifari_graph_compose(&composed,&next,&merged);pifari_graph_destroy(&composed);pifari_graph_destroy(&next);if(rc!=0){ok=0;snprintf(ir_error,sizeof ir_error,"falha ao compor grafos IR");break;}composed=merged;}
  }
  size_t len=seq.type==V_LIST?seq.as.list.count:seq.type==V_F64_ARRAY?seq.as.array->length:(size_t)seq.as.range_end;PifariF64Buffer input={0},output={0};PifariNativeKernel *kernel=NULL;
  if(ok&&(pifari_f64_buffer_init(&input,len)!=0||pifari_f64_buffer_init(&output,len)!=0)){ok=0;snprintf(ir_error,sizeof ir_error,"memória insuficiente nos buffers F64");}
  for(size_t i=0;i<len&&ok;i++){if(seq.type==V_RANGE){input.data[i]=(double)i;continue;}if(seq.type==V_F64_ARRAY){input.data[i]=seq.as.array->data[i];continue;}Value x=seq.as.list.items[i];if(x.type!=V_INT&&x.type!=V_FLOAT){ok=0;snprintf(ir_error,sizeof ir_error,"dados devem ser numéricos");break;}input.data[i]=number(x,n);}
  if(ok&&(kernel=chain_kernel_get(names,&composed))==NULL){ok=0;snprintf(ir_error,sizeof ir_error,"compilação nativa/cache da cadeia falhou");}
  int status=ok?pifari_native_kernel_map(kernel,input.data,output.data,len,workers):-1;
  if(status!=0){pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);if(have_graph)pifari_graph_destroy(&composed);free_value(names);free_value(seq);fail(n,ok&&status==-2?"divisão por zero na cadeia fundida":"map_fused_chain: %s",ok?"falha no kernel nativo":ir_error);return vnull();}
  Value result=seq.type==V_F64_ARRAY?f64_array_new(len):list_new(len);if(result.type==V_F64_ARRAY){for(size_t i=0;i<len;i++)result.as.array->data[i]=output.data[i];}else if(result.type==V_LIST)for(size_t i=0;i<len;i++)result.as.list.items[i]=vfloat(output.data[i]);
  pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);if(have_graph)pifari_graph_destroy(&composed);free_value(names);free_value(seq);
  if(result.type!=V_LIST&&result.type!=V_F64_ARRAY){fail(n,"map_fused_chain: memória insuficiente no resultado");return vnull();}return result;
 }
 if(!strcmp(name,"map_affine_relu")){
  if(count<3||count>4){fail(n,"map_affine_relu espera lista, escala, deslocamento e workers opcional");return vnull();}
  Value seq=eval(args->children[0]);if(runtime_failed){free_value(seq);return vnull();}
  Value sv=eval(args->children[1]),bv=eval(args->children[2]);
  if(runtime_failed){free_value(seq);free_value(sv);free_value(bv);return vnull();}
  if(seq.type!=V_LIST||(sv.type!=V_INT&&sv.type!=V_FLOAT)||(bv.type!=V_INT&&bv.type!=V_FLOAT)){
   free_value(seq);free_value(sv);free_value(bv);fail(n,"map_affine_relu exige lista numérica, escala e deslocamento numéricos");return vnull();
  }
  unsigned workers=1;
  if(count==4){Value w=eval(args->children[3]);if(runtime_failed){free_value(seq);free_value(sv);free_value(bv);free_value(w);return vnull();}if(w.type!=V_INT||w.as.i<1||w.as.i>32){free_value(seq);free_value(sv);free_value(bv);free_value(w);fail(n,"workers deve ser inteiro entre 1 e 32");return vnull();}workers=(unsigned)w.as.i;free_value(w);}
  size_t len=seq.as.list.count;double*in=malloc((len?len:1)*sizeof(double));double*outbuf=malloc((len?len:1)*sizeof(double));
  if(!in||!outbuf){free(in);free(outbuf);free_value(seq);free_value(sv);free_value(bv);fail(n,"memória insuficiente em map_affine_relu");return vnull();}
  int valid=1;for(size_t i=0;i<len;i++){Value x=seq.as.list.items[i];if(x.type!=V_INT&&x.type!=V_FLOAT){valid=0;break;}in[i]=number(x,n);}
  int computed=0;
  if(valid&&len){
   pifari_devices_probe();
   const PifariDevice*dev=pifari_devices_selected();
   size_t threshold=65536;
   const char*thr=getenv("PIFARI_GPU_THRESHOLD");
   if(thr&&*thr){long long t=atoll(thr);if(t>0)threshold=(size_t)t;}
   if(dev&&!strcmp(dev->backend,"opencl")&&len>=threshold)
    computed=(pifari_gpu_affine_relu(in,outbuf,len,number(sv,n),number(bv,n))==0);
  }
  if(!computed&&(!valid||pifari_kernel_affine_relu(in,outbuf,len,number(sv,n),number(bv,n),workers)!=0)){free(in);free(outbuf);free_value(seq);free_value(sv);free_value(bv);fail(n,"map_affine_relu falhou: elementos precisam ser numéricos");return vnull();}
  Value result=list_new(len);if(result.type!=V_LIST){free(in);free(outbuf);free_value(seq);free_value(sv);free_value(bv);fail(n,"memória insuficiente no resultado de map_affine_relu");return vnull();}
  for(size_t i=0;i<len;i++)result.as.list.items[i]=vfloat(outbuf[i]);
  free(in);free(outbuf);free_value(seq);free_value(sv);free_value(bv);return result;
 }
 if(!strcmp(name,"matmul")){
  if(count!=2){fail(n,"matmul espera duas matrizes");return vnull();}
  Value a=eval(args->children[0]);if(runtime_failed){free_value(a);return vnull();}
  Value b=eval(args->children[1]);if(runtime_failed){free_value(a);free_value(b);return vnull();}
  if(a.type!=V_LIST||b.type!=V_LIST){free_value(a);free_value(b);fail(n,"matmul exige duas listas de linhas");return vnull();}
  size_t rows=a.as.list.count,inner=0,brows=b.as.list.count,cols=0;int is_float=0,valid=1;
  if(rows){Value*r=&a.as.list.items[0];if(r->type!=V_LIST)valid=0;else inner=r->as.list.count;}
  if(brows){Value*r=&b.as.list.items[0];if(r->type!=V_LIST)valid=0;else cols=r->as.list.count;}
  if(inner!=brows)valid=0;
  for(size_t i=0;i<rows&&valid;i++){
   Value*r=&a.as.list.items[i];if(r->type!=V_LIST||r->as.list.count!=inner){valid=0;break;}
   for(size_t k=0;k<inner;k++){Value x=r->as.list.items[k];if(x.type!=V_INT&&x.type!=V_FLOAT){valid=0;break;}if(x.type==V_FLOAT)is_float=1;}
  }
  for(size_t k=0;k<brows&&valid;k++){
   Value*r=&b.as.list.items[k];if(r->type!=V_LIST||r->as.list.count!=cols){valid=0;break;}
   for(size_t j=0;j<cols;j++){Value x=r->as.list.items[j];if(x.type!=V_INT&&x.type!=V_FLOAT){valid=0;break;}if(x.type==V_FLOAT)is_float=1;}
  }
  if(!valid){free_value(a);free_value(b);fail(n,"matmul exige matrizes retangulares compatíveis com elementos numéricos");return vnull();}
  Value out=list_new(rows);if(out.type!=V_LIST){free_value(a);free_value(b);fail(n,"memória insuficiente em matmul");return vnull();}
  for(size_t i=0;i<rows;i++){
   out.as.list.items[i]=list_new(cols);
   if(out.as.list.items[i].type!=V_LIST){free_value(out);free_value(a);free_value(b);fail(n,"memória insuficiente em matmul");return vnull();}
   for(size_t j=0;j<cols;j++)out.as.list.items[i].as.list.items[j]=is_float?vfloat(0.0):vint(0);
  }
  if(is_float){
   for(size_t i=0;i<rows;i++)for(size_t k=0;k<inner;k++){
    Value av=a.as.list.items[i].as.list.items[k];double x=number(av,n);
    for(size_t j=0;j<cols;j++){double y=number(b.as.list.items[k].as.list.items[j],n);out.as.list.items[i].as.list.items[j].as.f+=x*y;}
   }
  }else{
   for(size_t i=0;i<rows;i++)for(size_t k=0;k<inner;k++){
    long long x=a.as.list.items[i].as.list.items[k].as.i;
    for(size_t j=0;j<cols;j++)out.as.list.items[i].as.list.items[j].as.i+=x*b.as.list.items[k].as.list.items[j].as.i;
   }
  }
  free_value(a);free_value(b);return out;
 }
 if(!strcmp(name,"sort")){
  if(count!=1){fail(n,"sort espera uma lista numérica");return vnull();}
  Value seq=eval(args->children[0]);if(runtime_failed){free_value(seq);return vnull();}
  if(seq.type!=V_LIST){free_value(seq);fail(n,"sort exige uma lista");return vnull();}
  size_t len=seq.as.list.count;Value out=list_new(len);if(out.type!=V_LIST){free_value(seq);fail(n,"memória insuficiente em sort");return vnull();}
  for(size_t i=0;i<len;i++){Value x=seq.as.list.items[i];if(x.type!=V_INT&&x.type!=V_FLOAT){free_value(seq);free_value(out);fail(n,"sort aceita somente listas numéricas");return vnull();}out.as.list.items[i]=copy_value(x);}
  qsort(out.as.list.items,len,sizeof(Value),compare_numeric_values);free_value(seq);return out;
 }
 if(!strcmp(name,"binary_search")){
  if(count!=2){fail(n,"binary_search espera lista ordenada e alvo numérico");return vnull();}
  Value seq=eval(args->children[0]),target=eval(args->children[1]);if(runtime_failed){free_value(seq);free_value(target);return vnull();}
  if(seq.type!=V_LIST||(target.type!=V_INT&&target.type!=V_FLOAT)){free_value(seq);free_value(target);fail(n,"binary_search exige lista e alvo numéricos");return vnull();}
  long double needle=target.type==V_INT?(long double)target.as.i:(long double)target.as.f;long long lo=0,hi=(long long)seq.as.list.count-1,found=-1;
  while(lo<=hi){long long mid=lo+(hi-lo)/2;Value x=seq.as.list.items[mid];if(x.type!=V_INT&&x.type!=V_FLOAT){free_value(seq);free_value(target);fail(n,"binary_search aceita somente listas numéricas ordenadas");return vnull();}long double v=x.type==V_INT?(long double)x.as.i:(long double)x.as.f;if(v<needle)lo=mid+1;else if(v>needle)hi=mid-1;else{found=mid;break;}}
  free_value(seq);free_value(target);return vint(found);
 }
 if(!strcmp(name,"solve_linear")){
  if(count!=2){fail(n,"solve_linear espera matriz quadrada A e vetor b");return vnull();}
  Value av=eval(args->children[0]),bv=eval(args->children[1]);if(runtime_failed){free_value(av);free_value(bv);return vnull();}
  if(av.type!=V_LIST||bv.type!=V_LIST||av.as.list.count!=bv.as.list.count){free_value(av);free_value(bv);fail(n,"solve_linear exige matriz quadrada e vetor de tamanho compatível");return vnull();}
  size_t dim=av.as.list.count;if(dim&&dim>SIZE_MAX/dim){free_value(av);free_value(bv);fail(n,"dimensão excessiva em solve_linear");return vnull();}
  if(dim&&dim>SIZE_MAX/sizeof(double)/dim){free_value(av);free_value(bv);fail(n,"matriz grande demais em solve_linear");return vnull();}
  size_t cells=dim?dim*dim:1;
  double*mat=malloc(cells*sizeof(double)),*rhs=malloc((dim?dim:1)*sizeof(double));
  if(!mat||!rhs){free(mat);free(rhs);free_value(av);free_value(bv);fail(n,"memória insuficiente em solve_linear");return vnull();}
  int valid=1;for(size_t i=0;i<dim&&valid;i++){
   Value row=av.as.list.items[i];if(row.type!=V_LIST||row.as.list.count!=dim){valid=0;break;}
   for(size_t j=0;j<dim;j++){Value x=row.as.list.items[j];if(x.type!=V_INT&&x.type!=V_FLOAT){valid=0;break;}mat[i*dim+j]=number(x,n);if(!isfinite(mat[i*dim+j])){valid=0;break;}}
   Value y=bv.as.list.items[i];if(y.type!=V_INT&&y.type!=V_FLOAT){valid=0;break;}rhs[i]=number(y,n);if(!isfinite(rhs[i])){valid=0;break;}
  }
  if(!valid){free(mat);free(rhs);free_value(av);free_value(bv);fail(n,"solve_linear exige elementos numéricos e matriz quadrada");return vnull();}
  for(size_t k=0;k<dim;k++){
   size_t pivot=k;double best=fabs(mat[k*dim+k]);for(size_t i=k+1;i<dim;i++){double v=fabs(mat[i*dim+k]);if(v>best){best=v;pivot=i;}}
   if(best==0.0||!isfinite(best)){free(mat);free(rhs);free_value(av);free_value(bv);fail(n,"solve_linear: matriz singular");return vnull();}
   if(pivot!=k){for(size_t j=k;j<dim;j++){double tmp=mat[k*dim+j];mat[k*dim+j]=mat[pivot*dim+j];mat[pivot*dim+j]=tmp;}double tmp=rhs[k];rhs[k]=rhs[pivot];rhs[pivot]=tmp;}
   for(size_t i=k+1;i<dim;i++){double factor=mat[i*dim+k]/mat[k*dim+k];mat[i*dim+k]=0.0;for(size_t j=k+1;j<dim;j++)mat[i*dim+j]-=factor*mat[k*dim+j];rhs[i]-=factor*rhs[k];}
  }
  for(size_t ii=dim;ii>0;ii--){size_t i=ii-1;for(size_t j=i+1;j<dim;j++)rhs[i]-=mat[i*dim+j]*rhs[j];rhs[i]/=mat[i*dim+i];}
  Value out=list_new(dim);if(out.type==V_LIST)for(size_t i=0;i<dim;i++)out.as.list.items[i]=vfloat(rhs[i]);
  free(mat);free(rhs);free_value(av);free_value(bv);if(out.type!=V_LIST)fail(n,"memória insuficiente no resultado de solve_linear");return out;
 }
 if(!strcmp(name,"array_set")){
  if(count!=3){fail(n,"array_set espera array, índice e valor");return vnull();}
  Value arr=eval(args->children[0]);if(runtime_failed){free_value(arr);return vnull();}
  Value idx=eval(args->children[1]);if(runtime_failed){free_value(arr);free_value(idx);return vnull();}
  Value val=eval(args->children[2]);if(runtime_failed){free_value(arr);free_value(idx);free_value(val);return vnull();}
  if(arr.type!=V_F64_ARRAY||!arr.as.array||idx.type!=V_INT||(val.type!=V_INT&&val.type!=V_FLOAT)){free_value(arr);free_value(idx);free_value(val);fail(n,"array_set exige array f64, índice inteiro e valor numérico");return vnull();}
  long long i=idx.as.i;size_t length=arr.as.array->length;if(i<0)i+=(long long)length;
  if(i<0||(size_t)i>=length){free_value(arr);free_value(idx);free_value(val);fail(n,"índice fora dos limites");return vnull();}
  arr.as.array->data[i]=number(val,n);free_value(arr);free_value(idx);free_value(val);return vnull();
 }
 if(!strcmp(name,"f64_array")||!strcmp(name,"array_f64")){
  if(count!=1){fail(n,"f64_array espera tamanho ou lista numérica");return vnull();}
  Value src=eval(args->children[0]);if(runtime_failed){free_value(src);return vnull();}
  size_t length=0;if(src.type==V_INT&&src.as.i>=0&&src.as.i<=10000000)length=(size_t)src.as.i;else if(src.type==V_LIST)length=src.as.list.count;else if(src.type==V_RANGE&&src.as.range_end>=0)length=(size_t)src.as.range_end;else{free_value(src);fail(n,"f64_array espera tamanho entre 0 e 10000000 ou lista/range numérico");return vnull();}
  Value out=f64_array_new(length);if(out.type!=V_F64_ARRAY){free_value(src);fail(n,"memória insuficiente em f64_array");return vnull();}
  for(size_t i=0;i<length;i++){Value x=src.type==V_LIST?src.as.list.items[i]:(src.type==V_RANGE?vint((long long)i):vint(0));if(x.type!=V_INT&&x.type!=V_FLOAT){free_value(src);free_value(out);fail(n,"f64_array só aceita elementos numéricos");return vnull();}out.as.array->data[i]=number(x,n);}
  free_value(src);return out;
 }
 if(!strcmp(name,"range")){if(count!=1){fail(n,"range espera um argumento: limite");return vnull();}Value v=eval(args->children[0]);if(runtime_failed)return vnull();if(v.type!=V_INT||v.as.i<0||v.as.i>1000000){fail(n,"range aceita um inteiro entre 0 e 1000000");return vnull();}Value out={.type=V_RANGE};out.as.range_end=v.as.i;return out;}
 if(!strcmp(name,"len")){if(count!=1){fail(n,"len espera um argumento");return vnull();}Value v=eval(args->children[0]);if(v.type==V_STRING)return vint((long long)strlen(v.as.s));if(v.type==V_LIST)return vint((long long)v.as.list.count);if(v.type==V_RANGE)return vint(v.as.range_end);if(v.type==V_F64_ARRAY)return vint((long long)v.as.array->length);fail(n,"len espera texto, lista, array tipado ou range");return vnull();}
 if(!strcmp(name,"int")||!strcmp(name,"long")||!strcmp(name,"short")||!strcmp(name,"float")||!strcmp(name,"double")||!strcmp(name,"str")||!strcmp(name,"txt")||!strcmp(name,"bool")){
  if(count!=1){fail(n,"cast %s espera um argumento",name);return vnull();}Value v=eval(args->children[0]);if(runtime_failed)return vnull();
  if(!strcmp(name,"str")||!strcmp(name,"txt")){char*s=stringify(v);Value r=vstr(s);free(s);return r;}
  if(!strcmp(name,"bool"))return vbool(truthy(v));
  if(!strcmp(name,"int")||!strcmp(name,"long")||!strcmp(name,"short")){if(v.type==V_STRING){char*end;errno=0;long long z=strtoll(v.as.s,&end,10);if(errno||*end){fail(n,"conversão para int inválida");return vnull();}return vint(z);}return vint((long long)number(v,n));}
  return vfloat(number(v,n));
 }
 if(!strcmp(name,"mcp_register")){
  if(count<3||count>5){fail(n,"mcp_register espera server_id, transport, endpoint, [timeout_ms], [max_concurrent]");return vnull();}
  Value sid=eval(args->children[0]),tr=eval(args->children[1]),ep=eval(args->children[2]);
  int timeout_ms=3000,max_conc=4;
  if(count>=4){Value tv=eval(args->children[3]);if(tv.type==V_INT)timeout_ms=(int)tv.as.i;free_value(tv);}
  if(count>=5){Value mv=eval(args->children[4]);if(mv.type==V_INT)max_conc=(int)mv.as.i;free_value(mv);}
  if(sid.type!=V_STRING||tr.type!=V_STRING||ep.type!=V_STRING){free_value(sid);free_value(tr);free_value(ep);fail(n,"mcp_register exige strings");return vnull();}
  McpTransportType ttype=!strcmp(tr.as.s,"http")?MCP_TRANSPORT_HTTP:MCP_TRANSPORT_STDIO;
  McpStatus st=mcp_gateway_register_server(sid.as.s,ttype,ep.as.s,"",timeout_ms,max_conc);
  free_value(sid);free_value(tr);free_value(ep);
  return vbool(st==MCP_STATUS_OK);
 }
 if(!strcmp(name,"mcp_discover_tools")){
  if(count!=1){fail(n,"mcp_discover_tools espera server_id");return vnull();}
  Value sid=eval(args->children[0]);if(sid.type!=V_STRING){free_value(sid);fail(n,"mcp_discover_tools exige string");return vnull();}
  size_t tc=0;McpStatus st=mcp_gateway_discover_tools(sid.as.s,&tc);free_value(sid);
  if(st!=MCP_STATUS_OK){fail(n,"falha em mcp_discover_tools: %s",mcp_status_str(st));return vnull();}
  return vint((long long)tc);
 }
 if(!strcmp(name,"mcp_discover_resources")){
  if(count!=1){fail(n,"mcp_discover_resources espera server_id");return vnull();}
  Value sid=eval(args->children[0]);if(sid.type!=V_STRING){free_value(sid);fail(n,"mcp_discover_resources exige string");return vnull();}
  size_t rc=0;McpStatus st=mcp_gateway_discover_resources(sid.as.s,&rc);free_value(sid);
  if(st!=MCP_STATUS_OK){fail(n,"falha em mcp_discover_resources: %s",mcp_status_str(st));return vnull();}
  return vint((long long)rc);
 }
 if(!strcmp(name,"mcp_read_resource")){
  if(count!=2){fail(n,"mcp_read_resource espera server_id e uri");return vnull();}
  Value sid=eval(args->children[0]),uri=eval(args->children[1]);
  if(sid.type!=V_STRING||uri.type!=V_STRING){free_value(sid);free_value(uri);fail(n,"mcp_read_resource exige strings");return vnull();}
  McpCallResult res=mcp_gateway_read_resource(sid.as.s,uri.as.s,2000);
  free_value(sid);free_value(uri);
  return res.status==MCP_STATUS_OK?vstr(res.output_text):vstr(res.error_message);
 }
 if(!strcmp(name,"mcp_call")){
  if(count<3||count>4){fail(n,"mcp_call espera agent_id, server_id, tool_name, [args_json]");return vnull();}
  Value aid=eval(args->children[0]),sid=eval(args->children[1]),tn=eval(args->children[2]);
  Value aj=count==4?eval(args->children[3]):vstr("{}");
  if(aid.type!=V_STRING||sid.type!=V_STRING||tn.type!=V_STRING||aj.type!=V_STRING){free_value(aid);free_value(sid);free_value(tn);free_value(aj);fail(n,"mcp_call exige strings");return vnull();}
  McpCallResult res=agent_runtime_invoke_mcp(aid.as.s,sid.as.s,tn.as.s,aj.as.s,2000);
  free_value(aid);free_value(sid);free_value(tn);free_value(aj);
  if(res.status==MCP_STATUS_OK)return vstr(res.output_text);
  char errbuf[640];snprintf(errbuf,sizeof(errbuf),"ERR:%s:%s",mcp_status_str(res.status),res.error_message);
  return vstr(errbuf);
 }
 if(!strcmp(name,"Agent")){
  if(count<3||count>6){fail(n,"Agent espera agent_id, role, goal, [adapter], [max_concurrency], [timeout_ms]");return vnull();}
  Value aid=eval(args->children[0]),role=eval(args->children[1]),goal=eval(args->children[2]);
  Value adp=count>=4?eval(args->children[3]):vstr("local-deterministic");
  int max_c=2,tm_ms=2000;
  if(count>=5){Value cv=eval(args->children[4]);if(cv.type==V_INT)max_c=(int)cv.as.i;free_value(cv);}
  if(count>=6){Value tv=eval(args->children[5]);if(tv.type==V_INT)tm_ms=(int)tv.as.i;free_value(tv);}
  if(aid.type!=V_STRING||role.type!=V_STRING||goal.type!=V_STRING||adp.type!=V_STRING){free_value(aid);free_value(role);free_value(goal);free_value(adp);fail(n,"Agent exige strings");return vnull();}
  int rc=agent_runtime_register_agent(aid.as.s,role.as.s,goal.as.s,adp.as.s,max_c,tm_ms);
  Value out=rc==0?copy_value(aid):vnull();
  free_value(aid);free_value(role);free_value(goal);free_value(adp);
  if(rc!=0)fail(n,"falha ao registrar Agent");
  return out;
 }
 if(!strcmp(name,"policy_allow")){
  if(count<3||count>4){fail(n,"policy_allow espera agent_id, server_id, tool_name, [allow_side_effects]");return vnull();}
  Value aid=eval(args->children[0]),sid=eval(args->children[1]),tn=eval(args->children[2]);
  int allow_se=0;if(count==4){Value sv=eval(args->children[3]);allow_se=truthy(sv);free_value(sv);}
  if(aid.type!=V_STRING||sid.type!=V_STRING||tn.type!=V_STRING){free_value(aid);free_value(sid);free_value(tn);fail(n,"policy_allow exige strings");return vnull();}
  int rc=agent_runtime_policy_allow(aid.as.s,sid.as.s,tn.as.s,allow_se);
  free_value(aid);free_value(sid);free_value(tn);
  return vbool(rc==0);
 }
 if(!strcmp(name,"Task")){
  if(count<5||count>8){fail(n,"Task espera task_id, agent_id, server_id, tool_name, args_json, [output_ctx_key], [priority], [timeout_ms]");return vnull();}
  Value tid=eval(args->children[0]),aid=eval(args->children[1]),sid=eval(args->children[2]),tn=eval(args->children[3]),aj=eval(args->children[4]);
  Value ok=count>=6?eval(args->children[5]):vstr("");
  int prio=1,tm_ms=2000;
  if(count>=7){Value pv=eval(args->children[6]);if(pv.type==V_INT)prio=(int)pv.as.i;free_value(pv);}
  if(count>=8){Value tv=eval(args->children[7]);if(tv.type==V_INT)tm_ms=(int)tv.as.i;free_value(tv);}
  if(tid.type!=V_STRING||aid.type!=V_STRING||sid.type!=V_STRING||tn.type!=V_STRING||aj.type!=V_STRING||ok.type!=V_STRING){
   free_value(tid);free_value(aid);free_value(sid);free_value(tn);free_value(aj);free_value(ok);fail(n,"Task exige strings");return vnull();
  }
  int rc=agent_runtime_create_task(tid.as.s,aid.as.s,sid.as.s,tn.as.s,aj.as.s,ok.as.s,prio,tm_ms);
  Value out=rc==0?copy_value(tid):vnull();
  free_value(tid);free_value(aid);free_value(sid);free_value(tn);free_value(aj);free_value(ok);
  if(rc!=0)fail(n,"falha ao criar Task");
  return out;
 }
 if(!strcmp(name,"task_add_dep")){
  if(count!=2){fail(n,"task_add_dep espera task_id e dep_task_id");return vnull();}
  Value tid=eval(args->children[0]),did=eval(args->children[1]);
  if(tid.type!=V_STRING||did.type!=V_STRING){free_value(tid);free_value(did);fail(n,"task_add_dep exige strings");return vnull();}
  int rc=agent_runtime_task_add_dep(tid.as.s,did.as.s);
  free_value(tid);free_value(did);return vbool(rc==0);
 }
 if(!strcmp(name,"task_cancel")){
  if(count<1||count>2){fail(n,"task_cancel espera task_id e [reason]");return vnull();}
  Value tid=eval(args->children[0]),rs=count==2?eval(args->children[1]):vstr("cancelled_by_user");
  if(tid.type!=V_STRING||rs.type!=V_STRING){free_value(tid);free_value(rs);fail(n,"task_cancel exige strings");return vnull();}
  int rc=agent_runtime_task_cancel(tid.as.s,rs.as.s);
  free_value(tid);free_value(rs);return vbool(rc==0);
 }
 if(!strcmp(name,"task_status")){
  if(count!=1){fail(n,"task_status espera task_id");return vnull();}
  Value tid=eval(args->children[0]);if(tid.type!=V_STRING){free_value(tid);fail(n,"task_status exige string");return vnull();}
  const ArTaskEntry *te=agent_runtime_get_task(tid.as.s);free_value(tid);
  return vstr(te?agent_runtime_task_state_str(te->state):"NOT_FOUND");
 }
 if(!strcmp(name,"task_result")){
  if(count!=1){fail(n,"task_result espera task_id");return vnull();}
  Value tid=eval(args->children[0]);if(tid.type!=V_STRING){free_value(tid);fail(n,"task_result exige string");return vnull();}
  const ArTaskEntry *te=agent_runtime_get_task(tid.as.s);free_value(tid);
  if(!te)return vstr("");
  return vstr(te->state==TASK_STATE_COMPLETED?te->result_text:te->error_text);
 }
 if(!strcmp(name,"dag_run")){
  int max_p=2;
  if(count==1){Value pv=eval(args->children[0]);if(pv.type==V_INT)max_p=(int)pv.as.i;free_value(pv);}
  int done=agent_runtime_dag_run(max_p);
  return vint((long long)done);
 }
 if(!strcmp(name,"ctx_put")){
  if(count<2||count>4){fail(n,"ctx_put espera key, value, [agent_id], [task_id]");return vnull();}
  Value k=eval(args->children[0]),v=eval(args->children[1]);
  Value aid=count>=3?eval(args->children[2]):vstr("runtime");
  Value tid=count>=4?eval(args->children[3]):vstr("");
  char *vs=v.type==V_STRING?strdup(v.as.s):stringify(v);
  int ver=0;
  if(k.type==V_STRING&&aid.type==V_STRING&&tid.type==V_STRING&&vs){
   ver=agent_runtime_ctx_put(k.as.s,vs,aid.as.s,tid.as.s);
  }
  free(vs);free_value(k);free_value(v);free_value(aid);free_value(tid);
  return vint((long long)ver);
 }
 if(!strcmp(name,"ctx_get")){
  if(count!=1){fail(n,"ctx_get espera key");return vnull();}
  Value k=eval(args->children[0]);if(k.type!=V_STRING){free_value(k);fail(n,"ctx_get exige string");return vnull();}
  const char *val=agent_runtime_ctx_get(k.as.s);
  Value out=vstr(val?val:"");free_value(k);return out;
 }
 if(!strcmp(name,"artifact_save")){
  if(count<3||count>5){fail(n,"artifact_save espera artifact_id, file_path, content, [agent_id], [task_id]");return vnull();}
  Value arid=eval(args->children[0]),fp=eval(args->children[1]),ct=eval(args->children[2]);
  Value aid=count>=4?eval(args->children[3]):vstr("runtime");
  Value tid=count>=5?eval(args->children[4]):vstr("");
  if(arid.type!=V_STRING||fp.type!=V_STRING||ct.type!=V_STRING||aid.type!=V_STRING||tid.type!=V_STRING){
   free_value(arid);free_value(fp);free_value(ct);free_value(aid);free_value(tid);fail(n,"artifact_save exige strings");return vnull();
  }
  int rc=agent_runtime_artifact_save(arid.as.s,fp.as.s,ct.as.s,aid.as.s,tid.as.s);
  const ArArtifactEntry *ae=rc==0?agent_runtime_get_artifact(arid.as.s):NULL;
  Value out=vstr(ae?ae->checksum_fnv1a:"");
  free_value(arid);free_value(fp);free_value(ct);free_value(aid);free_value(tid);
  return out;
 }
 if(!strcmp(name,"telemetry_progress")){
  return vint((long long)agent_runtime_progress_percent());
 }
 if(!strcmp(name,"telemetry_export")){
  if(count!=1){fail(n,"telemetry_export espera file_path");return vnull();}
  Value fp=eval(args->children[0]);if(fp.type!=V_STRING){free_value(fp);fail(n,"telemetry_export exige string");return vnull();}
  int rc=agent_runtime_export_report_json(fp.as.s);free_value(fp);
  return vbool(rc==0);
 }
 if(!strcmp(name,"mcp_server_init")){
  if(count!=2){fail(n,"mcp_server_init espera server_name e version");return vnull();}
  Value sn=eval(args->children[0]),vr=eval(args->children[1]);
  if(sn.type!=V_STRING||vr.type!=V_STRING){free_value(sn);free_value(vr);fail(n,"mcp_server_init exige strings");return vnull();}
  pifari_mcp_server_init(sn.as.s,vr.as.s);
  free_value(sn);free_value(vr);return vbool(1);
 }
 if(!strcmp(name,"mcp_server_register_tool")){
  if(count<4||count>5){fail(n,"mcp_server_register_tool espera tool_name, description, fn_name, param_list, [read_only]");return vnull();}
  Value tn=eval(args->children[0]),ds=eval(args->children[1]),fn=eval(args->children[2]),pl=eval(args->children[3]);
  int ro=1;if(count==5){Value rv=eval(args->children[4]);ro=truthy(rv);free_value(rv);}
  if(tn.type!=V_STRING||ds.type!=V_STRING||fn.type!=V_STRING||pl.type!=V_LIST){
   free_value(tn);free_value(ds);free_value(fn);free_value(pl);fail(n,"mcp_server_register_tool exige 3 strings e 1 lista de parametros");return vnull();
  }
  const char *pnames[PIFARI_MCP_SRV_MAX_PARAMS];
  size_t pc=pl.as.list.count>PIFARI_MCP_SRV_MAX_PARAMS?PIFARI_MCP_SRV_MAX_PARAMS:pl.as.list.count;
  for(size_t i=0;i<pc;i++){
   pnames[i]=pl.as.list.items[i].type==V_STRING?pl.as.list.items[i].as.s:"arg";
  }
  int rc=pifari_mcp_server_register_tool(tn.as.s,ds.as.s,fn.as.s,pc,pnames,ro);
  free_value(tn);free_value(ds);free_value(fn);free_value(pl);
  return vbool(rc==0);
 }
 if(!strcmp(name,"mcp_server_register_resource")){
  if(count!=4){fail(n,"mcp_server_register_resource espera uri, name, mime_type, content");return vnull();}
  Value u=eval(args->children[0]),nm=eval(args->children[1]),mt=eval(args->children[2]),ct=eval(args->children[3]);
  if(u.type!=V_STRING||nm.type!=V_STRING||mt.type!=V_STRING||ct.type!=V_STRING){
   free_value(u);free_value(nm);free_value(mt);free_value(ct);fail(n,"mcp_server_register_resource exige strings");return vnull();
  }
  int rc=pifari_mcp_server_register_resource(u.as.s,nm.as.s,mt.as.s,ct.as.s);
  free_value(u);free_value(nm);free_value(mt);free_value(ct);
  return vbool(rc==0);
 }
 if(!strcmp(name,"mcp_server_handle_rpc")){
  if(count!=1){fail(n,"mcp_server_handle_rpc espera json_line");return vnull();}
  Value jl=eval(args->children[0]);if(jl.type!=V_STRING){free_value(jl);fail(n,"mcp_server_handle_rpc exige string");return vnull();}
  char out[16384];
  int has=pifari_mcp_server_handle_line(jl.as.s,runtime_invoke_mcp_tool_cb,out,sizeof(out));
  free_value(jl);
  return vstr(has?out:"");
 }
 if(!strcmp(name,"mcp_server_run_stdio")){
  int rc=pifari_mcp_server_run_stdio(runtime_invoke_mcp_tool_cb);
  return vbool(rc==0);
 }
 if(!strcmp(name,"mcp_server_run_http")){
  if(count!=1){fail(n,"mcp_server_run_http espera porta");return vnull();}
  Value pv=eval(args->children[0]);int port=pv.type==V_INT?(int)pv.as.i:8765;free_value(pv);
  int rc=pifari_mcp_server_run_http(port,runtime_invoke_mcp_tool_cb);
  return vbool(rc==0);
 }
 Binding*b=find_binding(name);if(b)return copy_value(b->value);
 fail(n,"função '%s' não existe",name);return vnull();
}

static size_t count_nodes(const Node*n){if(!n)return 0;size_t z=1;for(size_t i=0;i<n->count;i++)z+=count_nodes(n->children[i]);return z;}
static void collect_slot_names(const Node*n,Binding*slots,size_t*used,size_t cap){
 if(!n||!strcmp(n->kind,"Function")||!strcmp(n->kind,"FunctionExpr"))return;
 const char*name=NULL;
 if(!strcmp(n->kind,"Assign")||!strcmp(n->kind,"TypedDecl")||!strcmp(n->kind,"For"))name=n->value;
 if(!strcmp(n->kind,"Flow")&&n->value&&!strcmp(n->value,"|<")&&n->count>1&&n->children[1]&&!strcmp(n->children[1]->kind,"Name"))name=n->children[1]->value;
 if(name&&*name){
  int found=0;for(size_t i=0;i<*used;i++)if(!strcmp(slots[i].name,name)){found=1;break;}
  if(!found&&*used<cap)slots[(*used)++].name=(char*)name;
 }
 for(size_t i=0;i<n->count;i++)collect_slot_names(n->children[i],slots,used,cap);
}
static void annotate_slot_nodes(Node*n,Binding*slots,size_t count,const Node*owner){
 if(!n||!strcmp(n->kind,"Function")||!strcmp(n->kind,"FunctionExpr"))return;
 if(!strcmp(n->kind,"Name")){
  for(size_t i=0;i<count;i++)if(!strcmp(slots[i].name,n->value)){n->local_slot=(int)i;n->slot_owner=owner;break;}
 }
 const char*target=NULL;
 if(!strcmp(n->kind,"Assign")||!strcmp(n->kind,"TypedDecl")||!strcmp(n->kind,"For")||!strcmp(n->kind,"IndexAssign"))target=n->value;
 if(!strcmp(n->kind,"Flow")&&n->value&&!strcmp(n->value,"|<")&&n->count>1&&n->children[1]&&!strcmp(n->children[1]->kind,"Name"))target=n->children[1]->value;
 if(target)for(size_t i=0;i<count;i++)if(!strcmp(slots[i].name,target)){n->target_slot=(int)i;n->target_owner=owner;break;}
 for(size_t i=0;i<n->count;i++)annotate_slot_nodes(n->children[i],slots,count,owner);
}
static void prepare_function_slots(FunctionDef*f){
 if(!f||!f->node||f->slots_ready)return;
 const Node*def=f->node,*params=def->count?def->children[0]:NULL,*body=def->count>1?def->children[1]:NULL;
 size_t cap=count_nodes(body)+(params?params->count:0);
 Binding*temp=calloc(cap?cap:1,sizeof(Binding));if(!temp)return;
 size_t used=0;
 if(params)for(size_t i=0;i<params->count;i++){
  const char*name=params->children[i]->value;int found=0;
  for(size_t j=0;j<used;j++)if(!strcmp(temp[j].name,name)){found=1;break;}
  if(!found)temp[used++].name=(char*)name;
 }
 collect_slot_names(body,temp,&used,cap);
 annotate_slot_nodes((Node*)body,temp,used,def);
 f->slot_names=calloc(used?used:1,sizeof(char*));
 if(f->slot_names){
  for(size_t i=0;i<used;i++)f->slot_names[i]=temp[i].name;
  f->slot_count=used;f->slots_ready=1;
 }
 free(temp);
}
static Binding *node_target_binding(const Node*n,const char*name){
 if(n&&n->target_slot>=0&&n->target_owner){
  LocalFrame*f=find_owner_frame(n->target_owner);
  if(f&&(size_t)n->target_slot<f->count)return &f->slots[n->target_slot];
 }
 return find_binding(name);
}
static void set_node_binding(const Node*n,const char*name,Value v,const Node*at){
 Binding*b=node_target_binding(n,name);
 if(!b){set_binding(name,v,at,0);return;}
 free_value(b->value);b->value=v;
}

typedef struct { char *s; size_t len,cap; int failed; } JitBuffer;
static int jit_append(JitBuffer*b,const char*fmt,...){
 if(b->failed)return 0;
 va_list ap;va_start(ap,fmt);va_list cp;va_copy(cp,ap);
 int n=vsnprintf(NULL,0,fmt,cp);va_end(cp);
 if(n<0){va_end(ap);b->failed=1;return 0;}
 size_t need=b->len+(size_t)n+1;
 if(need>b->cap){size_t cap=b->cap?b->cap:256;while(cap<need)cap*=2;char*q=realloc(b->s,cap);if(!q){va_end(ap);b->failed=1;return 0;}b->s=q;b->cap=cap;}
 vsnprintf(b->s+b->len,b->cap-b->len,fmt,ap);va_end(ap);b->len+=(size_t)n;return 1;
}
static int jit_expr(JitBuffer*b,const Node*n,const Node*owner){
 if(!n)return 0;
 if(!strcmp(n->kind,"Integer")&&n->numeric_cached)return jit_append(b,"%lldLL",n->integer_cache);
 if(!strcmp(n->kind,"Name")&&n->local_slot>=0&&n->slot_owner==owner)return jit_append(b,"v[%d]",n->local_slot);
 if(!strcmp(n->kind,"Unary")&&n->count==1&&n->value&&!strcmp(n->value,"-"))return jit_append(b,"(-(")&&jit_expr(b,n->children[0],owner)&&jit_append(b,"))");
 if(!strcmp(n->kind,"Binary")&&n->count==2){
  const char*op=NULL;
  switch(n->op_code){case PIFARI_OP_ADD:op="+";break;case PIFARI_OP_SUB:op="-";break;case PIFARI_OP_MUL:op="*";break;case PIFARI_OP_FLOORDIV:op="/";break;case PIFARI_OP_MOD:op="%";break;default:return 0;}
  return jit_append(b,"(")&&jit_expr(b,n->children[0],owner)&&jit_append(b," %s ",op)&&jit_expr(b,n->children[1],owner)&&jit_append(b,")");
 }
 return 0;
}
static size_t jit_name_refs(const Node*n,const char*name){
 if(!n)return 0;
 size_t count=(!strcmp(n->kind,"Name")&&n->value&&!strcmp(n->value,name))?1:0;
 for(size_t i=0;i<n->count;i++)count+=jit_name_refs(n->children[i],name);
 return count;
}
static int jit_writes_name(const Node*n,const char*name){
 if(!n)return 0;
 if((!strcmp(n->kind,"Assign")||!strcmp(n->kind,"TypedDecl"))&&n->value&&!strcmp(n->value,name))return 1;
 for(size_t i=0;i<n->count;i++)if(jit_writes_name(n->children[i],name))return 1;
 return 0;
}
static int jit_stmt(JitBuffer*b,const Node*n,const Node*owner,int*has_return,int*requires_range){
 if(!n)return 0;
 if(!strcmp(n->kind,"Block")||!strcmp(n->kind,"Statements")){
  for(size_t i=0;i<n->count;i++)if(!jit_stmt(b,n->children[i],owner,has_return,requires_range))return 0;
  return 1;
 }
 if(!strcmp(n->kind,"Assign")){
  if(n->target_slot<0||n->target_owner!=owner||!n->count)return 0;
  return jit_append(b,"v[%d] = ",n->target_slot)&&jit_expr(b,n->children[0],owner)&&jit_append(b,";\n");
 }
 if(!strcmp(n->kind,"For")){
  if(n->target_slot<0||n->target_owner!=owner||n->count<2)return 0;
  const Node*r=n->children[0];
  const Node*args=r&&r->count?r->children[0]:NULL;
  if(!r||strcmp(r->kind,"Call")||!r->value||strcmp(r->value,"range")||!args||args->count!=1)return 0;
  const Node*bound=args->children[0];
  if(!bound||strcmp(bound->kind,"Name")||bound->local_slot<0||bound->slot_owner!=owner)return 0;
  /* Restrict JIT range bounds to parameters so range validation remains exact. */
  int is_parameter=0;const Node*params=owner->count?owner->children[0]:NULL;
  if(params)for(size_t i=0;i<params->count;i++)if(!strcmp(params->children[i]->value,bound->value))is_parameter=1;
  if(!is_parameter)return 0;
  if(jit_name_refs(owner->count>1?owner->children[1]:NULL,n->value)!=jit_name_refs(n->children[1],n->value)||jit_writes_name(owner->count>1?owner->children[1]:NULL,n->value))return 0;
  *requires_range=1;
  if(!jit_append(b,"{ long long bound_%d = v[%d]; for(v[%d]=0; v[%d]<bound_%d; ++v[%d]) {\n",n->target_slot,bound->local_slot,n->target_slot,n->target_slot,n->target_slot,n->target_slot))return 0;
  if(!jit_stmt(b,n->children[1],owner,has_return,requires_range))return 0;
  return jit_append(b,"} }\n");
 }
 if(!strcmp(n->kind,"Return")){
  if(n->count!=1)return 0;
  if(!jit_append(b,"return "))return 0;
  if(!jit_expr(b,n->children[0],owner)||!jit_append(b,";\n"))return 0;
  *has_return=1;return 1;
 }
 return 0;
}
/* Atribuição definida para o JIT.
   O C gerado inicializa todos os slots com 0. Se a função lê um slot antes de
   atribuí-lo, o interpretador leria o valor externo capturado no frame e o JIT
   leria 0 — resultado diferente. Enquanto a inicialização de slots não for
   passada ao JIT, só compilamos funções em que toda leitura é de parâmetro ou de
   nome atribuído antes, no fluxo linear. */
typedef struct { const char **names; unsigned char *assigned; size_t count; } JitDefEnv;

static int jit_de_lookup(JitDefEnv *env,const char *name){
 if(!name)return -1;
 for(size_t i=0;i<env->count;i++)if(env->names[i]&&!strcmp(env->names[i],name))return (int)i;
 return -1;
}
static int jit_de_expr_ok(const Node *n,JitDefEnv *env){
 if(!n)return 1;
 if(!strcmp(n->kind,"Name")){
  int slot=jit_de_lookup(env,n->value);
  return slot>=0&&env->assigned[slot];
 }
 if(!strcmp(n->kind,"Function")||!strcmp(n->kind,"FunctionExpr"))return 0;
 for(size_t i=0;i<n->count;i++)if(!jit_de_expr_ok(n->children[i],env))return 0;
 return 1;
}
static void jit_de_assign(JitDefEnv *env,const char *name){
 int slot=jit_de_lookup(env,name);
 if(slot>=0)env->assigned[slot]=1;
}
static int jit_de_stmt_ok(const Node *n,JitDefEnv *env){
 if(!n)return 1;
 if(!strcmp(n->kind,"Block")||!strcmp(n->kind,"Statements")){
  for(size_t i=0;i<n->count;i++)if(!jit_de_stmt_ok(n->children[i],env))return 0;
  return 1;
 }
 if(!strcmp(n->kind,"Assign")||!strcmp(n->kind,"TypedDecl")){
  for(size_t i=0;i<n->count;i++)if(!jit_de_expr_ok(n->children[i],env))return 0;
  jit_de_assign(env,n->value);
  return 1;
 }
 if(!strcmp(n->kind,"IndexAssign")){
  for(size_t i=0;i<n->count;i++)if(!jit_de_expr_ok(n->children[i],env))return 0;
  return 1;
 }
 if(!strcmp(n->kind,"ExpressionStatement"))return n->count?jit_de_expr_ok(n->children[0],env):1;
 if(!strcmp(n->kind,"Return"))return n->count?jit_de_expr_ok(n->children[0],env):1;
 if(!strcmp(n->kind,"Break")||!strcmp(n->kind,"Continue"))return 1;
 if(!strcmp(n->kind,"For")){
  if(n->count<2||!jit_de_expr_ok(n->children[0],env))return 0;
  JitDefEnv inner=*env;
  inner.assigned=calloc(env->count?env->count:1,1);
  if(!inner.assigned)return 0;
  memcpy(inner.assigned,env->assigned,env->count);
  jit_de_assign(&inner,n->value);
  int ok=jit_de_stmt_ok(n->children[1],&inner);
  free(inner.assigned);
  return ok;
 }
 if(!strcmp(n->kind,"If")||!strcmp(n->kind,"IfElse")){
  if(n->count<2||!jit_de_expr_ok(n->children[0],env))return 0;
  for(size_t branch=1;branch<n->count;branch++){
   JitDefEnv inner=*env;
   inner.assigned=calloc(env->count?env->count:1,1);
   if(!inner.assigned)return 0;
   memcpy(inner.assigned,env->assigned,env->count);
   int ok=jit_de_stmt_ok(n->children[branch],&inner);
   free(inner.assigned);
   if(!ok)return 0;
  }
  return 1;
 }
 return 0;
}
static int jit_definitely_assigned(const Node *body,Binding *slots,size_t used){
 size_t cap=used?used:1;
 const char **names=calloc(cap,sizeof(char*));
 unsigned char *assigned=calloc(cap,1);
 if(!names||!assigned){free(names);free(assigned);return 0;}
 JitDefEnv env={.names=names,.assigned=assigned,.count=used};
 for(size_t i=0;i<used;i++)names[i]=slots[i].name;
 /* os slots de parâmetros chegam atribuídos (vindos dos argumentos) */
 if(used)assigned[0]=1;
 int ok=jit_de_stmt_ok(body,&env);
 free(names);free(assigned);
 return ok;
}static void jit_compile_function(FunctionDef*f){
 if(!f||!f->node||strcmp(f->node->kind,"Function"))return;
 const Node*def=f->node,*params=def->count?def->children[0]:NULL,*body=def->count>1?def->children[1]:NULL;
 if(!params||params->count!=1||!params->children[0]->value||!body)return;
 if(def->count>2)return;
 for(size_t pi=0;pi<params->count;pi++)if(!strcmp(params->children[pi]->kind,"Param"))return;
 size_t cap=count_nodes(body)+params->count;
 Binding*slots=calloc(cap?cap:1,sizeof(Binding));if(!slots)return;
 size_t used=0;slots[used++].name=params->children[0]->value;
 collect_slot_names(body,slots,&used,cap);
 annotate_slot_nodes((Node*)body,slots,used,def);
 if(!jit_definitely_assigned(body,slots,used)){free(slots);return;}
 int has_return=0,requires_range=0;JitBuffer b={0};
 jit_append(&b,"#include <limits.h>\nlong long pifari_jit(long long arg0) { long long v[%zu]={0}; v[0]=arg0;\n",used?used:1);
 if(!jit_stmt(&b,body,def,&has_return,&requires_range)||!has_return||b.failed){free(b.s);free(slots);return;}
 jit_append(&b,"return 0; }\n");
 if(b.failed){free(b.s);free(slots);return;}
 char cache_dir[128],cachepath[256],cmd[768],dir_template[128],cpath[192],sopath[192];
 snprintf(cache_dir,sizeof(cache_dir),"/tmp/pifari_jit_cache_%lu",(unsigned long)getuid());
 if(mkdir(cache_dir,0700)!=0&&errno!=EEXIST){free(b.s);free(slots);return;}
 struct stat cache_stat;
 if(stat(cache_dir,&cache_stat)!=0||!S_ISDIR(cache_stat.st_mode)||cache_stat.st_uid!=getuid()){free(b.s);free(slots);return;}
 (void)chmod(cache_dir,0700);
 uint64_t hash=UINT64_C(1469598103934665603);for(size_t i=0;i<b.len;i++){hash^=(unsigned char)b.s[i];hash*=UINT64_C(1099511628211);}
 snprintf(cachepath,sizeof(cachepath),"%s/%016llx.so",cache_dir,(unsigned long long)hash);
 void*h=NULL;
 if(access(cachepath,R_OK)==0)h=dlopen(cachepath,RTLD_NOW|RTLD_LOCAL);
 if(!h){
  snprintf(dir_template,sizeof(dir_template),"/tmp/pifari_jit_%ld_XXXXXX",(long)getpid());
  char*temp_dir=mkdtemp(dir_template);if(!temp_dir){free(b.s);free(slots);return;}
  snprintf(cpath,sizeof(cpath),"%s/source.c",temp_dir);
  snprintf(sopath,sizeof(sopath),"%s/module.so",temp_dir);
  FILE*fp=fopen(cpath,"wb");if(!fp){rmdir(temp_dir);free(b.s);free(slots);return;}
  size_t written=fwrite(b.s,1,b.len,fp);int close_rc=fclose(fp);
  if(written==0||close_rc!=0){unlink(cpath);rmdir(temp_dir);free(b.s);free(slots);return;}
  snprintf(cmd,sizeof(cmd),"gcc -std=c11 -O3 -march=native -shared -fPIC -o %s %s </dev/null >/dev/null 2>/dev/null",sopath,cpath);
  int rc=system(cmd);unlink(cpath);
  if(rc!=0){unlink(sopath);rmdir(temp_dir);free(b.s);free(slots);return;}
  if(rename(sopath,cachepath)!=0){unlink(sopath);rmdir(temp_dir);free(b.s);free(slots);return;}
  rmdir(temp_dir);h=dlopen(cachepath,RTLD_NOW|RTLD_LOCAL);
 }
 free(b.s);free(slots);if(!h)return;
 void*sym=dlsym(h,"pifari_jit");long long(*fn)(long long)=NULL;
 if(!sym||sizeof(fn)!=sizeof(sym)){dlclose(h);return;}
 memcpy(&fn,&sym,sizeof(fn));f->jit_handle=h;f->jit_fn=fn;f->jit_requires_range=requires_range;
}
static void free_function_registry(void){
 while(functions){
  FunctionDef*next=functions->next;
  if(functions->native_kernel)pifari_native_kernel_destroy(functions->native_kernel);
  if(functions->ir_ready)pifari_graph_destroy(&functions->ir);
  if(functions->indexed_kernel)pifari_native_kernel_destroy(functions->indexed_kernel);
  if(functions->indexed_ir_ready)pifari_graph_destroy(&functions->indexed_ir);
  if(functions->jit_handle)dlclose(functions->jit_handle);
  free(functions->slot_names);
  free(functions);functions=next;
 }
}
static void build_slot_index(LocalFrame*f){
 memset(f->buckets,0,sizeof(f->buckets));
 for(size_t i=0;i<f->count;i++){size_t h=binding_hash(f->slots[i].name)&255u;f->slots[i].hash_next=f->buckets[h];f->buckets[h]=&f->slots[i];}
}

static size_t jit_name_refs(const Node*n,const char*name);
static int jit_writes_name(const Node*n,const char*name);
static int bc_compile_statement(ByteBuilder*b,const Node*n,const Node*owner){
 if(!n)return 0;
 if(!strcmp(n->kind,"Block")||!strcmp(n->kind,"Statements")||!strcmp(n->kind,"Program")){
  for(size_t i=0;i<n->count;i++)if(!bc_compile_statement(b,n->children[i],owner))return 0;
  return 1;
 }
 if(!strcmp(n->kind,"Assign")||!strcmp(n->kind,"TypedDecl")){
  if(n->target_slot<0||n->target_owner!=owner||!n->count)return 0;
  const Node*expr=n->children[0];
  if(!strcmp(n->kind,"Assign")&&expr&&!strcmp(expr->kind,"Binary")&&expr->op_code==PIFARI_OP_ADD&&expr->count==2){
   const Node*left=expr->children[0],*right=expr->children[1];
   if(left&&!strcmp(left->kind,"Name")&&left->local_slot==n->target_slot&&left->slot_owner==owner&&right&&!strcmp(right->kind,"Binary")&&right->op_code==PIFARI_OP_MUL&&right->count==2){
    const Node*x=right->children[0],*y=right->children[1];
    if(x&&!strcmp(x->kind,"Name")&&y&&!strcmp(y->kind,"Name")&&x->local_slot>=0&&y->local_slot>=0&&x->slot_owner==owner&&y->slot_owner==owner){
     ByteInstr fused={.op=BC_MADD_SLOTS,.i=n->target_slot,.target=(size_t)x->local_slot,.alternate=(size_t)y->local_slot,.slot_owner=owner};
     return bc_emit(b,fused);
    }
   }
  }
  if(!bc_compile_node(b,expr))return 0;
  ByteInstr in={.op=BC_STORE_SLOT,.i=n->target_slot,.slot_owner=owner};return bc_emit(b,in);
 }
 if(!strcmp(n->kind,"For")){
  if(n->target_slot<0||n->target_owner!=owner||n->count<2)return 0;
  const Node*range=n->children[0];
  const Node*range_args=range&&range->count?range->children[0]:NULL;
  if(!range||strcmp(range->kind,"Call")||!range->value||strcmp(range->value,"range")||!range_args||range_args->count!=1)return 0;
  if(jit_name_refs(owner->count>1?owner->children[1]:NULL,n->value)!=jit_name_refs(n->children[1],n->value)||jit_writes_name(owner->count>1?owner->children[1]:NULL,n->value))return 0;
  if(!bc_compile_node(b,range_args->children[0]))return 0;
  size_t begin=b->count;
  ByteInstr init={.op=BC_RANGE_BEGIN,.i=n->target_slot,.slot_owner=owner};
  if(!bc_emit(b,init))return 0;
  size_t test=b->count;
  ByteInstr check={.op=BC_RANGE_TEST,.i=n->target_slot,.slot_owner=owner,.target=begin,.alternate=0};
  if(!bc_emit(b,check))return 0;
  if(!bc_compile_statement(b,n->children[1],owner))return 0;
  ByteInstr inc={.op=BC_INC_SLOT,.i=n->target_slot,.slot_owner=owner};
  if(!bc_emit(b,inc))return 0;
  ByteInstr jump={.op=BC_JUMP,.target=test};
  if(!bc_emit(b,jump))return 0;
  b->code[test].alternate=b->count;
  return 1;
 }
 if(!strcmp(n->kind,"Return")){
  if(n->count&&!bc_compile_node(b,n->children[0]))return 0;
  ByteInstr in={.op=BC_RETURN,.i=n->count?1:0};return bc_emit(b,in);
 }
 if(!strcmp(n->kind,"ExpressionStatement")){
  if(n->count&&!bc_compile_node(b,n->children[0]))return 0;
  ByteInstr in={.op=BC_POP};return bc_emit(b,in);
 }
 return 0;
}
static int bc_ensure_function(const Node*definition,const Node*body){
 Node*m=(Node*)definition;
 if(__atomic_load_n(&m->function_code_ready,__ATOMIC_ACQUIRE))return m->function_code_supported;
 pthread_mutex_lock(&bytecode_compile_mutex);
 if(__atomic_load_n(&m->function_code_ready,__ATOMIC_RELAXED)){
  int ok=m->function_code_supported;pthread_mutex_unlock(&bytecode_compile_mutex);return ok;
 }
 ByteBuilder b={0};
 if(bc_compile_statement(&b,body,definition)){
  ByteInstr end={.op=BC_RETURN,.i=0};
  if(bc_emit(&b,end)){m->function_code=b.code;m->function_code_count=b.count;m->function_code_supported=1;}
  else free(b.code);
 }else free(b.code);
 __atomic_store_n(&m->function_code_ready,1,__ATOMIC_RELEASE);
 int ok=m->function_code_supported;pthread_mutex_unlock(&bytecode_compile_mutex);return ok;
}
static Value execute_function_bytecode(const Node*definition,const Node*call){
 const Node*n=call;
 size_t code_count=definition->function_code_count,cap=code_count+1,sp=0,pc=0;
 Value local_stack[32];Value*stack=cap<=32?local_stack:calloc(cap,sizeof(Value));
 long long local_bounds[32]={0};
 long long*bounds=code_count<=32?local_bounds:(code_count<=1048576?calloc(code_count,sizeof(long long)):NULL);
 if(!stack||!bounds){if(stack&&stack!=local_stack)free(stack);if(bounds&&bounds!=local_bounds)free(bounds);fail(call,"memória insuficiente na VM de função");return vnull();}
 while(pc<definition->function_code_count&&!runtime_failed&&!return_active&&!PIFARI_TASK_CANCELLED()){
  ByteInstr in=definition->function_code[pc];Value a,b,r=vnull();
  switch(in.op){
   case BC_INT:stack[sp++]=vint(in.i);pc++;break;
   case BC_FLOAT:stack[sp++]=vfloat(in.f);pc++;break;
   case BC_LOAD:{
    Binding*x=find_binding(in.name);if(!x){fail(call,"variável '%s' não foi declarada",in.name);break;}
    if(x->value.type==V_F64_ARRAY&&x->value.as.array&&pc+2<definition->function_code_count&&definition->function_code[pc+2].op==BC_INDEX){
     ByteInstr idx_ins=definition->function_code[pc+1];long long idx=-1;int has_idx=0;
     if(idx_ins.op==BC_INT){idx=idx_ins.i;has_idx=1;}
     else if(idx_ins.op==BC_LOAD){Binding*ib=find_binding(idx_ins.name);if(ib&&ib->value.type==V_INT){idx=ib->value.as.i;has_idx=1;}}
     else if(idx_ins.op==BC_LOAD_SLOT){LocalFrame*f=find_owner_frame(idx_ins.slot_owner);if(f&&idx_ins.i>=0&&(size_t)idx_ins.i<f->count&&f->slots[idx_ins.i].value.type==V_INT){idx=f->slots[idx_ins.i].value.as.i;has_idx=1;}}
     if(has_idx){
      long long length=(long long)x->value.as.array->length;if(idx<0)idx+=length;
      if(idx<0||idx>=length){fail(call,"índice fora dos limites");break;}
      stack[sp++]=vfloat(x->value.as.array->data[idx]);pc+=3;break;
     }
    }
    stack[sp++]=copy_value(x->value);pc++;break;
   }
   case BC_LOAD_SLOT:{LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count){fail(call,"slot local inválido na VM de função");break;}stack[sp++]=copy_value(f->slots[in.i].value);pc++;break;}
   case BC_NEG:
    if(!sp){fail(call,"bytecode inválido");break;}a=stack[--sp];r=a.type==V_INT?vint(-a.as.i):vfloat(-number(a,call));free_value(a);stack[sp++]=r;pc++;break;
   case BC_EXP:
    if(!sp){fail(call,"bytecode inválido");break;}a=stack[--sp];if(a.type!=V_INT&&a.type!=V_FLOAT){free_value(a);fail(call,"exp exige argumento numérico");break;}r=vfloat(exp(number(a,call)));free_value(a);stack[sp++]=r;pc++;break;
   case BC_ADD:case BC_SUB:case BC_MUL:case BC_DIV:case BC_IDIV:case BC_MOD:case BC_POW:case BC_EQ:case BC_NE:case BC_LT:case BC_LE:case BC_GT:case BC_GE:case BC_INDEX:
    if(sp<2){fail(call,"bytecode inválido");break;}
    b=stack[--sp];a=stack[--sp];
    if(in.op==BC_INDEX){
     if(b.type!=V_INT||(a.type!=V_LIST&&a.type!=V_RANGE&&a.type!=V_F64_ARRAY)){free_value(a);free_value(b);fail(call,"indexação na VM de função exige lista, array tipado/range e índice inteiro");break;}
     long long length=a.type==V_LIST?(long long)a.as.list.count:a.type==V_F64_ARRAY?(long long)a.as.array->length:a.as.range_end;long long index=b.as.i;if(index<0)index+=length;free_value(b);
     if(index<0||index>=length){free_value(a);fail(call,"índice fora dos limites");break;}
     r=a.type==V_RANGE?vint(index):a.type==V_F64_ARRAY?vfloat(a.as.array->data[index]):copy_value(a.as.list.items[index]);free_value(a);stack[sp++]=r;pc++;break;
    }
    if(in.op==BC_EQ||in.op==BC_NE){int eq=equal_values(a,b);r=vbool(in.op==BC_EQ?eq:!eq);free_value(a);free_value(b);stack[sp++]=r;pc++;break;}
    if(in.op==BC_LT||in.op==BC_LE||in.op==BC_GT||in.op==BC_GE){
     int cmp;if(a.type==V_STRING&&b.type==V_STRING)cmp=strcmp(a.as.s,b.as.s);else if((a.type==V_INT||a.type==V_FLOAT)&&(b.type==V_INT||b.type==V_FLOAT)){double x=number(a,call),y=number(b,call);cmp=(x>y)-(x<y);}else{free_value(a);free_value(b);fail(call,"comparação na VM exige operandos compatíveis");break;}
     r=vbool(in.op==BC_LT?cmp<0:in.op==BC_LE?cmp<=0:in.op==BC_GT?cmp>0:cmp>=0);free_value(a);free_value(b);stack[sp++]=r;pc++;break;
    }
    if((a.type!=V_INT&&a.type!=V_FLOAT)||(b.type!=V_INT&&b.type!=V_FLOAT)){free_value(a);free_value(b);fail(call,"a VM de função exige operandos numéricos");break;}
    if(in.op==BC_ADD&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_add_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na soma");else r=vint(z);}
    else if(in.op==BC_SUB&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_sub_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na subtracao");else r=vint(z);}
    else if(in.op==BC_MUL&&a.type==V_INT&&b.type==V_INT){long long z;if(__builtin_mul_overflow(a.as.i,b.as.i,&z))fail(n,"overflow de inteiro na multiplicacao");else r=vint(z);}
    else {double x=number(a,call),y=number(b,call);if(in.op==BC_ADD)r=vfloat(x+y);else if(in.op==BC_SUB)r=vfloat(x-y);else if(in.op==BC_MUL)r=vfloat(x*y);else if(in.op==BC_DIV){if(y==0)fail(call,"divisão por zero");else r=vfloat(x/y);}else if(in.op==BC_IDIV){if(y==0)fail(call,"divisão por zero");else r=(a.type==V_INT&&b.type==V_INT)?vint(a.as.i/b.as.i):vint((long long)(x/y));}else if(in.op==BC_MOD){if(y==0)fail(call,"divisão por zero");else r=vint((long long)x%(long long)y);}else r=vfloat(pow(x,y));}
    free_value(a);free_value(b);if(!runtime_failed)stack[sp++]=r;else free_value(r);pc++;break;
   case BC_STORE_SLOT:{
    LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count||!sp){fail(call,"slot de escrita inválido");break;}
    free_value(f->slots[in.i].value);f->slots[in.i].value=stack[--sp];pc++;break;
   }
   case BC_RANGE_BEGIN:{
    LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count||!sp){fail(call,"inicialização de range inválida");break;}
    a=stack[--sp];if(a.type!=V_INT||a.as.i<0||a.as.i>1000000){free_value(a);fail(call,"range aceita inteiro entre 0 e 1000000");break;}
    bounds[pc]=a.as.i;free_value(f->slots[in.i].value);f->slots[in.i].value=vint(0);pc++;break;
   }
   case BC_RANGE_TEST:{
    LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count||in.target>=definition->function_code_count){fail(call,"teste de range inválido");break;}
    Value cur=f->slots[in.i].value;
    if(cur.type!=V_INT){fail(call,"variável de range deve ser inteira");break;}
    if(cur.as.i>=bounds[in.target])pc=in.alternate;else pc++;break;
   }
   case BC_INC_SLOT:{
    LocalFrame*f=find_owner_frame(in.slot_owner);if(!f||in.i<0||(size_t)in.i>=f->count||f->slots[in.i].value.type!=V_INT){fail(call,"incremento de slot inválido");break;}
    f->slots[in.i].value.as.i++;pc++;break;
   }
   case BC_JUMP:if(in.target>=definition->function_code_count){fail(call,"salto inválido na VM");break;}pc=in.target;break;
   case BC_RETURN:
    if(in.i){if(!sp){fail(call,"return sem valor na VM");break;}return_value=stack[--sp];}
    else return_value=vnull();
    return_active=1;pc=definition->function_code_count;break;
   case BC_POP:if(sp)free_value(stack[--sp]);pc++;break;
   case BC_MADD_SLOTS:{
    LocalFrame*f=find_owner_frame(in.slot_owner);
    if(!f||in.i<0||(size_t)in.i>=f->count||in.target>=f->count||in.alternate>=f->count){fail(call,"slots inválidos na operação fundida");break;}
    Value*acc=&f->slots[in.i].value;Value*x=&f->slots[in.target].value;Value*y=&f->slots[in.alternate].value;
    if((acc->type!=V_INT&&acc->type!=V_FLOAT)||(x->type!=V_INT&&x->type!=V_FLOAT)||(y->type!=V_INT&&y->type!=V_FLOAT)){fail(call,"operação fundida exige números");break;}
    if(acc->type==V_INT&&x->type==V_INT&&y->type==V_INT)acc->as.i+=x->as.i*y->as.i;
    else {double total=number(*acc,call)+number(*x,call)*number(*y,call);acc->type=V_FLOAT;acc->as.f=total;}
    pc++;break;
   }
   default:fail(call,"opcode inválido na VM de função");break;
  }
 }
 for(size_t i=0;i<sp;i++)free_value(stack[i]);
 if(stack!=local_stack)free(stack);
 if(bounds!=local_bounds)free(bounds);
 return vnull();
}
static Value call_user_function(const Node *call,FunctionDef *fn_def){
 const Node *definition=fn_def->node;
 const Node *params=definition->count?definition->children[0]:NULL;
 const Node *body=definition->count>1?definition->children[1]:NULL;
 const Node *args=call->count?call->children[0]:NULL;
 size_t pc=params?params->count:0,ac=args?args->count:0;
 if(pc!=ac){fail(call,"função '%s' espera %zu argumento(s), recebeu %zu",definition->value,pc,ac);return vnull();}
 Value stack_values[8];
 Value *values=ac<=8?stack_values:calloc(ac?ac:1,sizeof(Value));
 if(!values){fail(call,"memória insuficiente nos argumentos");return vnull();}
 for(size_t i=0;i<ac;i++){values[i]=eval(args->children[i]);if(runtime_failed){for(size_t j=0;j<=i;j++)free_value(values[j]);if(values!=stack_values)free(values);return vnull();}}
 for(size_t i=0;i<pc&&i<ac&&params;i++){
  const Node*param=params->children[i];
  if(!param||strcmp(param->kind,"Param")||!param->count)continue;
  const Node*tn=param->children[0];
  if(!tn||strcmp(tn->kind,"Type"))continue;
  if(!pifari_type_is_known(tn->value)){fail(call,"tipo de parâmetro desconhecido: '%s'",tn->value?tn->value:"?");break;}
  PifariType want=pifari_type_from_name(tn->value);
  PifariType got=value_type_of(values[i]);
  int compat=pifari_type_assignable(want,got);
  if(!compat){fail(call,"parâmetro '%s' de '%s' espera %s, recebeu %s",param->value,definition->value,pifari_type_name(want),pifari_type_name(got));break;}
  if(compat==2&&values[i].type==V_INT)values[i]=vfloat((double)values[i].as.i);
 }
 if(runtime_failed){for(size_t j=0;j<ac;j++)free_value(values[j]);if(values!=stack_values)free(values);return vnull();}
 if(fn_def->jit_fn&&ac==1&&values[0].type==V_INT){
  long long arg=values[0].as.i;
  if(fn_def->jit_requires_range&&(arg<0||arg>1000000)){free_value(values[0]);if(values!=stack_values)free(values);fail(call,"range aceita inteiro entre 0 e 1000000");return vnull();}
  long long result=fn_def->jit_fn(arg);free_value(values[0]);if(values!=stack_values)free(values);
  return vint(result);
 }
 if(!fn_def->slots_ready&&!in_worker)prepare_function_slots(fn_def);
 size_t used=fn_def->slot_count;
 LocalFrame stack_frame;
 Binding stack_slots[16];
 LocalFrame *frame=&stack_frame;
 frame->slots=used<=16?stack_slots:calloc(used?used:1,sizeof(Binding));
 if(!frame->slots){for(size_t i=0;i<ac;i++)free_value(values[i]);if(values!=stack_values)free(values);fail(call,"memória insuficiente para slots locais");return vnull();}
 frame->count=used;frame->owner=definition;
 for(size_t i=0;i<pc&&i<used;i++){
  frame->slots[i].name=fn_def->slot_names[i];
  frame->slots[i].value=values[i];
  frame->slots[i].next=NULL;frame->slots[i].hash_next=NULL;
 }
 for(size_t i=pc;i<used;i++){
  frame->slots[i].name=fn_def->slot_names[i];
  Binding*old=find_binding(frame->slots[i].name);
  frame->slots[i].value=old?copy_value(old->value):vnull();
  frame->slots[i].next=NULL;frame->slots[i].hash_next=NULL;
 }
 if(values!=stack_values)free(values);
 int use_bc=(strcmp(definition->kind,"FunctionExpr")&&bc_ensure_function(definition,body));
 if(!use_bc)build_slot_index(frame);
 frame->parent=local_frame;LocalFrame*saved_frame=local_frame;local_frame=frame;
 int saved_return_active=return_active;Value saved_return_value=return_value;
 return_active=0;return_value=vnull();
 Value implicit_result=vnull();
 if(use_bc){
  (void)execute_function_bytecode(definition,call);
 }else if(!strcmp(definition->kind,"FunctionExpr")&&body&&!strcmp(body->kind,"Block")&&body->count){
  for(size_t i=0;i+1<body->count&&!runtime_failed&&!return_active;i++)exec_node(body->children[i]);
  if(!runtime_failed&&!return_active){const Node *last=body->children[body->count-1];if(!strcmp(last->kind,"ExpressionStatement")&&last->count)implicit_result=eval(last->children[0]);else exec_node(last);}
 }else exec_node(body);
 Value result=return_active?return_value:implicit_result;
 if(!return_active)free_value(return_value);
 if(definition->count>2&&definition->children[2]&&!strcmp(definition->children[2]->kind,"Type")&&!runtime_failed){
  const Node*ret_node=definition->children[2];
  if(!pifari_type_is_known(ret_node->value)){
   fail(call,"tipo de retorno desconhecido: '%s'",ret_node->value?ret_node->value:"?");
  }else{
   PifariType want=pifari_type_from_name(ret_node->value);
   PifariType got=value_type_of(result);
   int compat=pifari_type_assignable(want,got);
   if(!compat)fail(call,"função '%s' declara retorno %s, devolveu %s",definition->value,pifari_type_name(want),pifari_type_name(got));
   else if(compat==2&&result.type==V_INT)result=vfloat((double)result.as.i);
  }
 }
 local_frame=saved_frame;
 for(size_t i=0;i<used;i++)free_value(frame->slots[i].value);
 if(frame->slots!=stack_slots)free(frame->slots);
 return_active=saved_return_active;return_value=saved_return_value;
 return result;
}
static char *runtime_invoke_mcp_tool_cb(const char *fn_name,size_t argc,const char **argv,int *out_is_error){
 if(out_is_error)*out_is_error=0;
 FunctionDef *fn_def=NULL;
 for(FunctionDef *f=functions;f;f=f->next)if(!strcmp(f->name,fn_name?fn_name:"")){fn_def=f;break;}
 if(!fn_def){if(out_is_error)*out_is_error=1;return dup_string("funcao Pifari nao encontrada");}
 Node arg_nodes[PIFARI_MCP_SRV_MAX_PARAMS];
 Node *arg_ptrs[PIFARI_MCP_SRV_MAX_PARAMS];
 memset(arg_nodes,0,sizeof(arg_nodes));
 if(argc>PIFARI_MCP_SRV_MAX_PARAMS)argc=PIFARI_MCP_SRV_MAX_PARAMS;
 for(size_t i=0;i<argc;i++){
  const char *raw=(argv&&argv[i])?argv[i]:"";
  arg_nodes[i].local_slot=-1;arg_nodes[i].target_slot=-1;arg_nodes[i].line=1;arg_nodes[i].column=1;
  char *end_i=NULL,*end_f=NULL;
  errno=0;long long iv=strtoll(raw,&end_i,10);
  if(raw[0]&&errno==0&&end_i&&*end_i=='\0'){
   arg_nodes[i].kind="Integer";arg_nodes[i].value=(char*)raw;arg_nodes[i].integer_cache=iv;arg_nodes[i].numeric_cached=1;
  }else{
   errno=0;double fv=strtod(raw,&end_f);
   if(raw[0]&&errno==0&&end_f&&*end_f=='\0'){
    arg_nodes[i].kind="Float";arg_nodes[i].value=(char*)raw;arg_nodes[i].float_cache=fv;arg_nodes[i].numeric_cached=1;
   }else if(!strcmp(raw,"true")||!strcmp(raw,"false")){
    arg_nodes[i].kind="Boolean";arg_nodes[i].value=(char*)raw;
   }else{
    arg_nodes[i].kind="String";arg_nodes[i].value=(char*)raw;
   }
  }
  arg_ptrs[i]=&arg_nodes[i];
 }
 Node args_list={0};args_list.kind="Arguments";args_list.children=arg_ptrs;args_list.count=argc;args_list.local_slot=-1;args_list.target_slot=-1;
 Node *call_children[1]={&args_list};
 Node call_node={0};call_node.kind="Call";call_node.value=(char*)fn_name;call_node.children=call_children;call_node.count=1;call_node.line=1;call_node.column=1;call_node.local_slot=-1;call_node.target_slot=-1;
 int saved_failed=runtime_failed;runtime_failed=0;
 Value out_val=call_user_function(&call_node,fn_def);
 if(runtime_failed){
  if(out_is_error)*out_is_error=1;
  free_value(out_val);
  runtime_failed=saved_failed;
  return dup_string("erro de execucao na ferramenta Pifari");
 }
 runtime_failed=saved_failed;
 char *str=stringify(out_val);
 free_value(out_val);
 return str;
}
typedef struct {
 size_t next, count;
 Value *items, *results;
 char **errors;
 const Node *target, *variable, *at;
 Binding *base;
} ParallelWork;
/* Falha fica associada ao item que falhou (spec §240): guarda a mensagem no
   índice correspondente em vez de abortar o fluxo inteiro. */
static void record_parallel_error(ParallelWork*w,size_t index){
 if(!w->errors||index>=w->count)return;
 w->errors[index]=dup_string(error_message[0]?error_message:"erro não especificado");
}
/* Worker environments use shallow binding nodes: the base values are immutable
   during a flow, while loop variables and function locals live in slot frames. */
static Binding *clone_bindings(Binding *src){
 Binding *copy=NULL;
 for(Binding *b=src;b;b=b->next){Binding *n=calloc(1,sizeof(*n));if(!n)exit(2);n->name=b->name;n->value=b->value;n->next=copy;copy=n;}
 return copy;
}
static void free_binding_nodes(Binding*b){while(b){Binding*next=b->next;free(b);b=next;}}
#define PIFARI_POOL_SIZE 4
typedef struct {
 pthread_mutex_t mutex;
 pthread_cond_t work_ready, work_done;
 pthread_t threads[PIFARI_POOL_SIZE];
 size_t started, pending, active_workers;
 unsigned long generation;
 int shutdown, initialized;
 ParallelWork *work;
} PersistentPool;
static PersistentPool worker_pool={.mutex=PTHREAD_MUTEX_INITIALIZER,.work_ready=PTHREAD_COND_INITIALIZER,.work_done=PTHREAD_COND_INITIALIZER};
static void parallel_process_work(ParallelWork*w){
 globals=clone_bindings(w->base);rebuild_binding_index();runtime_failed=0;error_message[0]=0;
 Binding iteration_slot={.name=w->variable->value,.value={.type=V_NULL}};
 LocalFrame iteration_frame={.slots=&iteration_slot,.count=1};
 size_t iteration_hash=binding_hash(iteration_slot.name)&255u;
 iteration_frame.buckets[iteration_hash]=&iteration_slot;iteration_frame.parent=NULL;local_frame=&iteration_frame;
 in_worker=1;
 for(;;){
  size_t i=__atomic_fetch_add(&w->next,1,__ATOMIC_RELAXED);
  if(i>=w->count)break;
  if(cancel_requested)break;
  set_binding(w->variable->value,copy_value(w->items[i]),w->at,0);
  w->results[i]=eval(w->target);
  if(runtime_failed){free_value(w->results[i]);w->results[i]=vnull();record_parallel_error(w,i);runtime_failed=0;}
 }
 in_worker=0;
 free_value(iteration_slot.value);local_frame=NULL;
 free_binding_nodes(globals);globals=NULL;memset(binding_buckets,0,sizeof(binding_buckets));
}
/* Fluxo aninhado: chamado de dentro de um trabalhador, roda serialmente no próprio
   trabalhador (não recursa no pool, o que travava) e reusa o ambiente já isolado. */
static void run_work_serial(ParallelWork*w){
 Binding iteration_slot={.name=w->variable->value,.value={.type=V_NULL}};
 LocalFrame frame={.slots=&iteration_slot,.count=1};
 frame.buckets[binding_hash(iteration_slot.name)&255u]=&iteration_slot;
 frame.parent=local_frame;local_frame=&frame;
 int saved_failed=runtime_failed;runtime_failed=0;error_message[0]=0;
 for(size_t i=0;i<w->count;i++){
  if(runtime_failed||cancel_requested)break;
  free_value(iteration_slot.value);iteration_slot.value=copy_value(w->items[i]);
  w->results[i]=eval(w->target);
  if(runtime_failed){free_value(w->results[i]);w->results[i]=vnull();record_parallel_error(w,i);runtime_failed=0;}
 }
 free_value(iteration_slot.value);local_frame=frame.parent;
 runtime_failed=saved_failed;
}
static void *persistent_worker(void*opaque){
 size_t worker_index=(size_t)(uintptr_t)opaque;unsigned long seen=0;
 for(;;){
  pthread_mutex_lock(&worker_pool.mutex);
  while(!worker_pool.shutdown&&worker_pool.generation==seen)pthread_cond_wait(&worker_pool.work_ready,&worker_pool.mutex);
  if(worker_pool.shutdown){pthread_mutex_unlock(&worker_pool.mutex);break;}
  seen=worker_pool.generation;ParallelWork*w=worker_pool.work;
  int active=worker_index<worker_pool.active_workers;
  pthread_mutex_unlock(&worker_pool.mutex);
  if(active)parallel_process_work(w);
  pthread_mutex_lock(&worker_pool.mutex);
  if(worker_pool.pending&&--worker_pool.pending==0)pthread_cond_signal(&worker_pool.work_done);
  pthread_mutex_unlock(&worker_pool.mutex);
 }
 return NULL;
}
/* ===== Células compartilhadas =====
   Único caminho pelo qual trabalhadores compartilham estado (SPEC §3.5 e §6).
   Cada célula tem mutex próprio; leitura/escrita/incremento são atômicos. */
#define PIFARI_MAX_SHARED 64
typedef struct { pthread_mutex_t lock; Value value; int used; } PifariShared;
static PifariShared pifari_shared_cells[PIFARI_MAX_SHARED];
static pthread_mutex_t shared_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

static long long pifari_shared_new(Value v){
 long long slot=-1;
 pthread_mutex_lock(&shared_registry_mutex);
 for(int i=0;i<PIFARI_MAX_SHARED;i++)if(!pifari_shared_cells[i].used){slot=i;break;}
 if(slot>=0){
  PifariShared*s=&pifari_shared_cells[slot];
  s->used=1;pthread_mutex_init(&s->lock,NULL);s->value=v;
 }
 pthread_mutex_unlock(&shared_registry_mutex);
 return slot;
}
static PifariShared *pifari_shared_get(long long id){
 if(id<0||id>=PIFARI_MAX_SHARED||!pifari_shared_cells[id].used)return NULL;
 return &pifari_shared_cells[id];
}
static int pifari_shared_write(long long id,Value v){
 PifariShared*s=pifari_shared_get(id);
 if(!s){free_value(v);return -1;}
 pthread_mutex_lock(&s->lock);
 free_value(s->value);s->value=v;
 pthread_mutex_unlock(&s->lock);
 return 0;
}
static Value pifari_shared_read(long long id,int*ok){
 PifariShared*s=pifari_shared_get(id);
 if(!s){*ok=0;return vnull();}
 pthread_mutex_lock(&s->lock);
 Value out=copy_value(s->value);
 pthread_mutex_unlock(&s->lock);
 *ok=1;
 return out;
}
static Value pifari_shared_add(long long id,double delta,int*ok){
 PifariShared*s=pifari_shared_get(id);
 if(!s){*ok=0;return vnull();}
 pthread_mutex_lock(&s->lock);
 Value current=s->value;
 if(current.type!=V_INT&&current.type!=V_FLOAT){
  pthread_mutex_unlock(&s->lock);*ok=0;return vnull();
 }
 double base=current.type==V_INT?(double)current.as.i:current.as.f;
 double next=base+delta;
 Value out;
 if(current.type==V_INT&&delta==(double)(long long)delta)out=vint((long long)next);
 else out=vfloat(next);
 s->value=copy_value(out);
 pthread_mutex_unlock(&s->lock);
 *ok=1;
 return out;
}
static void pifari_shared_shutdown(void){
 for(int i=0;i<PIFARI_MAX_SHARED;i++){
  if(!pifari_shared_cells[i].used)continue;
  pthread_mutex_lock(&pifari_shared_cells[i].lock);
  free_value(pifari_shared_cells[i].value);
  pifari_shared_cells[i].value=vnull();
  pthread_mutex_unlock(&pifari_shared_cells[i].lock);
  pthread_mutex_destroy(&pifari_shared_cells[i].lock);
  pifari_shared_cells[i].used=0;
 }
}
/* ===== Tarefas: spawn / await / cancelar =====
   `spawn expr` cria e inicia imediatamente; o id inteiro devolvido indexa o
   registro. A tarefa roda com ambiente clonado (mesmo isolamento do worker) e o
   cancelamento é cooperativo, checado nos laços de execução. */
#define PIFARI_MAX_TASKS 64
typedef struct PifariTask {
 pthread_t thread;
 _Atomic int status;
 _Atomic int cancel;
 int used;
 Value result;
 char error[256];
 const Node *expr;
 Binding *env;
} PifariTask;
static PifariTask pifari_tasks[PIFARI_MAX_TASKS];
static pthread_mutex_t tasks_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local PifariTask *current_task;
static int pifari_task_cancelled(void){
 return current_task && __atomic_load_n(&current_task->cancel,__ATOMIC_RELAXED);
}

static void *pifari_task_thread(void *arg){
 PifariTask *t=(PifariTask*)arg;
 globals=t->env;rebuild_binding_index();
 runtime_failed=0;error_message[0]=0;current_task=t;
 Value v=eval(t->expr);
 current_task=NULL;
 if(__atomic_load_n(&t->cancel,__ATOMIC_ACQUIRE)){
  __atomic_store_n(&t->status,5,__ATOMIC_RELEASE);
  free_value(v);
 }else if(runtime_failed){
  __atomic_store_n(&t->status,4,__ATOMIC_RELEASE);
  snprintf(t->error,sizeof(t->error),"%s",error_message[0]?error_message:"erro não especificado");
  free_value(v);
 }else{
  t->result=v;
  __atomic_store_n(&t->status,3,__ATOMIC_RELEASE);
 }
 free_binding_nodes(globals);globals=NULL;memset(binding_buckets,0,sizeof(binding_buckets));
 t->env=NULL;
 return NULL;
}
static long long pifari_task_spawn(const Node *expr){
 if(!expr){fail(NULL,"spawn exige uma expressão");return -1;}
 long long slot=-1;
 pthread_mutex_lock(&tasks_mutex);
 for(int i=0;i<PIFARI_MAX_TASKS;i++)if(!pifari_tasks[i].used){slot=i;break;}
 if(slot>=0){
  PifariTask*t=&pifari_tasks[slot];
  t->used=1;__atomic_store_n(&t->status,1,__ATOMIC_RELEASE);__atomic_store_n(&t->cancel,0,__ATOMIC_RELEASE);
  t->result=vnull();t->error[0]=0;t->expr=expr;t->env=clone_bindings(globals);
 }
 pthread_mutex_unlock(&tasks_mutex);
 if(slot<0){fail(expr,"limite de %d tarefas simultâneas atingido",PIFARI_MAX_TASKS);return -1;}
 PifariTask*t=&pifari_tasks[slot];
 if(pthread_create(&t->thread,NULL,pifari_task_thread,t)!=0){
  pthread_mutex_lock(&tasks_mutex);t->used=0;pthread_mutex_unlock(&tasks_mutex);
  fail(expr,"não foi possível criar a tarefa");return -1;
 }
 return slot;
}
static int pifari_task_cancel(long long id){
 if(id<0||id>=PIFARI_MAX_TASKS||!pifari_tasks[id].used)return -1;
 __atomic_store_n(&pifari_tasks[id].cancel,1,__ATOMIC_RELEASE);
 return (int)id;
}
static Value pifari_task_await(const Node *at,long long id){
 if(id<0||id>=PIFARI_MAX_TASKS||!pifari_tasks[id].used){fail(at,"tarefa inválida");return vnull();}
 PifariTask*t=&pifari_tasks[id];
 pthread_join(t->thread,NULL);
 int status=__atomic_load_n(&t->status,__ATOMIC_ACQUIRE);
 Value out=vnull();
 if(status==3)out=copy_value(t->result);
 else if(status==4)fail(at,"tarefa falhou: %s",t->error[0]?t->error:"erro não especificado");
 else if(status==5)fail(at,"tarefa cancelada");
 else fail(at,"estado de tarefa inesperado (%d)",status);
 free_value(t->result);t->result=vnull();
 pthread_mutex_lock(&tasks_mutex);t->used=0;pthread_mutex_unlock(&tasks_mutex);
 return out;
}
static void pifari_tasks_shutdown(void){
 for(int i=0;i<PIFARI_MAX_TASKS;i++){
  if(!pifari_tasks[i].used)continue;
  __atomic_store_n(&pifari_tasks[i].cancel,1,__ATOMIC_RELEASE);
  pthread_join(pifari_tasks[i].thread,NULL);
  free_value(pifari_tasks[i].result);
  pifari_tasks[i].result=vnull();
  pifari_tasks[i].used=0;
 }
}
static void pool_initialize(void){
 pthread_mutex_lock(&worker_pool.mutex);
 if(!worker_pool.initialized){
  worker_pool.initialized=1;
  for(size_t i=0;i<PIFARI_POOL_SIZE;i++)if(pthread_create(&worker_pool.threads[worker_pool.started],NULL,persistent_worker,(void*)(uintptr_t)worker_pool.started)==0)worker_pool.started++;
  thread_pool_active=1;
 }
 pthread_mutex_unlock(&worker_pool.mutex);
}
static void pool_run(ParallelWork*w){
 if(in_worker){run_work_serial(w);return;}
 pool_initialize();
 if(!worker_pool.started){parallel_process_work(w);return;}
 pthread_mutex_lock(&worker_pool.mutex);
 worker_pool.work=w;worker_pool.active_workers=w->count<worker_pool.started?w->count:worker_pool.started;worker_pool.pending=worker_pool.started;worker_pool.generation++;
 pthread_cond_broadcast(&worker_pool.work_ready);
 while(worker_pool.pending)pthread_cond_wait(&worker_pool.work_done,&worker_pool.mutex);
 worker_pool.work=NULL;
 pthread_mutex_unlock(&worker_pool.mutex);
}
static void pool_shutdown(void){
 pthread_mutex_lock(&worker_pool.mutex);
 if(!worker_pool.initialized){pthread_mutex_unlock(&worker_pool.mutex);return;}
 worker_pool.shutdown=1;pthread_cond_broadcast(&worker_pool.work_ready);
 pthread_mutex_unlock(&worker_pool.mutex);
 for(size_t i=0;i<worker_pool.started;i++)pthread_join(worker_pool.threads[i],NULL);
 pthread_mutex_lock(&worker_pool.mutex);worker_pool.started=0;worker_pool.initialized=0;worker_pool.shutdown=0;worker_pool.generation=0;pthread_mutex_unlock(&worker_pool.mutex);
}
/* Fast path: the AST itself proves that this parallel flow is a pure,
   single-argument numeric map. In that case skip per-item Value/function calls. */
static int try_native_map_flow(const Node *target, const Node *variable,
                               Value sequence, Value *result) {
 if(!target||strcmp(target->kind,"Call")||target->count!=1||
    !target->children[0]||target->children[0]->count!=1) return 0;
 const Node *argument=target->children[0]->children[0];
 if(!argument||strcmp(argument->kind,"Name")||
    strcmp(argument->value,variable->value)) return 0;

 FunctionDef *function=NULL;
 for(FunctionDef *f=functions;f;f=f->next)
  if(!strcmp(f->name,target->value?target->value:"")){function=f;break;}
 if(!function||!function->ir_ready||!function->effects_ready||function->effects!=EFFECT_PURE||
    (sequence.type!=V_LIST&&sequence.type!=V_RANGE&&sequence.type!=V_F64_ARRAY)) return 0;

 size_t length=sequence.type==V_LIST?sequence.as.list.count:
               sequence.type==V_F64_ARRAY?sequence.as.array->length:
               (size_t)sequence.as.range_end;
 if(sequence.type==V_LIST){
  for(size_t i=0;i<length;i++)
   if(sequence.as.list.items[i].type!=V_INT&&
      sequence.as.list.items[i].type!=V_FLOAT) return 0;
 }

 if(!function->native_kernel&&
    pifari_native_kernel_create(&function->ir,&function->native_kernel)!=0)
  return 0;

 PifariF64Buffer input={0},output={0};
 const double *in_ptr=NULL;
 if(sequence.type==V_F64_ARRAY){
  in_ptr=sequence.as.array->data;
 }else{
  if(pifari_f64_buffer_init(&input,length)!=0){
   fail(target,"memoria insuficiente ao preparar kernel fundido");
   return -1;
  }
  if(sequence.type==V_RANGE){
   for(size_t i=0;i<length;i++)input.data[i]=(double)i;
  }else{
   for(size_t i=0;i<length;i++)input.data[i]=number(sequence.as.list.items[i],target);
  }
  in_ptr=input.data;
 }
 if(pifari_f64_buffer_init(&output,length)!=0){
  pifari_f64_buffer_destroy(&input);
  fail(target,"memoria insuficiente ao preparar kernel fundido");
  return -1;
 }
 int status=pifari_native_kernel_map(function->native_kernel,in_ptr,
                                     output.data,length,PIFARI_POOL_SIZE);
 if(status!=0){
  pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
  fail(target,status==-2?"divisao por zero no kernel fundido":
                          "falha ao executar kernel nativo fundido");
  return -1;
 }
 Value mapped=list_new(length);
 if(mapped.type!=V_LIST){
  pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
  fail(target,"memoria insuficiente no resultado do kernel fundido");
  return -1;
 }
 for(size_t i=0;i<length;i++)mapped.as.list.items[i]=vfloat(output.data[i]);
 pifari_f64_buffer_destroy(&input);pifari_f64_buffer_destroy(&output);
 *result=mapped;
 return 1;
}

/* ===== Serialização de Value para resultados entre processos =====
   Só o resultado atravessa o pipe: os filhos são fork() do pai, então os itens
   já estão na memória deles por copy-on-write. */
typedef struct { unsigned char *data; size_t len, cap; int failed; } ByteBuf;
static void bb_reserve(ByteBuf*b,size_t extra){
 if(b->failed)return;
 if(b->len+extra<=b->cap)return;
 size_t cap=b->cap?b->cap:256;
 while(cap<b->len+extra)cap*=2;
 unsigned char*p=realloc(b->data,cap);
 if(!p){b->failed=1;return;}
 b->data=p;b->cap=cap;
}
static void bb_bytes(ByteBuf*b,const void*p,size_t n){if(b->failed)return;bb_reserve(b,n);if(b->failed)return;if(n)memcpy(b->data+b->len,p,n);b->len+=n;}
static void bb_u8(ByteBuf*b,unsigned v){unsigned char c=(unsigned char)v;bb_bytes(b,&c,1);}
static void bb_u32(ByteBuf*b,uint32_t v){unsigned char c[4]={(unsigned char)(v&255),(unsigned char)((v>>8)&255),(unsigned char)((v>>16)&255),(unsigned char)((v>>24)&255)};bb_bytes(b,c,4);}
static void bb_i64(ByteBuf*b,long long v){unsigned long long u=(unsigned long long)v;unsigned char c[8];for(int i=0;i<8;i++)c[i]=(unsigned char)(u>>(8*i));bb_bytes(b,c,8);}
static void bb_f64(ByteBuf*b,double d){unsigned long long u;memcpy(&u,&d,8);unsigned char c[8];for(int i=0;i<8;i++)c[i]=(unsigned char)(u>>(8*i));bb_bytes(b,c,8);}
static void encode_value(ByteBuf*b,Value v,int depth){
 if(b->failed)return;
 if(depth>32){b->failed=1;return;}
 switch(v.type){
  case V_NULL: bb_u8(b,0); break;
  case V_INT: bb_u8(b,1); bb_i64(b,v.as.i); break;
  case V_FLOAT: bb_u8(b,2); bb_f64(b,v.as.f); break;
  case V_BOOL: bb_u8(b,3); bb_u8(b,(unsigned)(v.as.b?1:0)); break;
  case V_STRING: {
   size_t n=strlen(v.as.s?v.as.s:"");
   if(n>1048576){b->failed=1;return;}
   bb_u8(b,4);bb_u32(b,(uint32_t)n);bb_bytes(b,v.as.s,n);break;
  }
  case V_LIST: {
   if(v.as.list.count>1000000){b->failed=1;return;}
   bb_u8(b,5);bb_u32(b,(uint32_t)v.as.list.count);
   for(size_t i=0;i<v.as.list.count;i++)encode_value(b,v.as.list.items[i],depth+1);
   break;
  }
  case V_F64_ARRAY: {
   size_t n=v.as.array?v.as.array->length:0;
   if(n>10000000){b->failed=1;return;}
   bb_u8(b,6);bb_u32(b,(uint32_t)n);
   for(size_t i=0;i<n;i++)bb_f64(b,v.as.array->data[i]);
   break;
  }
  case V_RANGE: bb_u8(b,7); bb_i64(b,v.as.range_end); break;
  default: b->failed=1; break;
 }
}
typedef struct { const unsigned char *data; size_t len, pos; int failed; } ByteCursor;
static unsigned char rd_u8(ByteCursor*c){if(c->failed||c->pos>=c->len){c->failed=1;return 0;}return c->data[c->pos++];}
static uint32_t rd_u32(ByteCursor*c){
 if(c->failed||c->pos+4>c->len){c->failed=1;return 0;}
 uint32_t v=0;for(int i=0;i<4;i++)v|=(uint32_t)c->data[c->pos+i]<<(8*i);
 c->pos+=4;return v;
}
static unsigned long long rd_u64(ByteCursor*c){
 if(c->failed||c->pos+8>c->len){c->failed=1;return 0;}
 unsigned long long v=0;for(int i=0;i<8;i++)v|=(unsigned long long)c->data[c->pos+i]<<(8*i);
 c->pos+=8;return v;
}
static Value decode_value(ByteCursor*c,int depth){
 if(c->failed||depth>32){c->failed=1;return vnull();}
 unsigned char tag=rd_u8(c);
 switch(tag){
  case 0: return vnull();
  case 1: return vint((long long)rd_u64(c));
  case 2: {unsigned long long u=rd_u64(c);double d;memcpy(&d,&u,8);return vfloat(d);}
  case 3: return vbool(rd_u8(c));
  case 4: {
   uint32_t n=rd_u32(c);
   if(c->failed||n>1048576||c->pos+n>c->len){c->failed=1;return vnull();}
   char*s=malloc((size_t)n+1);
   if(!s){c->failed=1;return vnull();}
   memcpy(s,c->data+c->pos,n);s[n]=0;c->pos+=n;
   Value out={.type=V_STRING};out.as.s=s;return out;
  }
  case 5: {
   uint32_t n=rd_u32(c);
   if(c->failed||n>1000000){c->failed=1;return vnull();}
   Value lst=list_new(n);
   if(lst.type!=V_LIST){c->failed=1;return vnull();}
   for(uint32_t i=0;i<n;i++){lst.as.list.items[i]=decode_value(c,depth+1);if(c->failed){free_value(lst);return vnull();}}
   return lst;
  }
  case 6: {
   uint32_t n=rd_u32(c);
   if(c->failed||n>10000000||c->pos+(size_t)n*8>c->len){c->failed=1;return vnull();}
   Value arr=f64_array_new(n);
   if(arr.type!=V_F64_ARRAY){c->failed=1;return vnull();}
   for(uint32_t i=0;i<n;i++){unsigned long long u=rd_u64(c);memcpy(&arr.as.array->data[i],&u,8);}
   return arr;
  }
  case 7: return vint((long long)rd_u64(c));
  default: c->failed=1;return vnull();
 }
}
static int pifari_write_all(int fd,const void*buf,size_t len){
 const unsigned char*p=(const unsigned char*)buf;
 while(len){
  ssize_t w=write(fd,p,len);
  if(w<0){if(errno==EINTR)continue;return -1;}
  if(w==0)return -1;
  p+=w;len-=(size_t)w;
 }
 return 0;
}
/* ===== Distribuição em processos =====
   Um contador atômico em memória compartilhada distribui os índices; cada filho
   devolve [índice][tamanho][status+payload] pelo seu pipe. O pai nunca escreve
   dados: zero risco de deadlock por pipe cheio no sentido de ida. */
static void process_map(ParallelWork*w,size_t workers){
 if(in_worker){run_work_serial(w);return;}
 if(workers<1)workers=1;
 if(workers>w->count)workers=w->count;
 if(!workers)return;
 size_t *shared=mmap(NULL,sizeof(size_t),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
 if(shared==MAP_FAILED){parallel_process_work(w);return;}
 *shared=0;
 int (*fds)[2]=calloc(workers,sizeof(*fds));
 pid_t *pids=calloc(workers,sizeof(pid_t));
 unsigned char *delivered=calloc(w->count?w->count:1,1);
 if(!fds||!pids||!delivered){
  free(fds);free(pids);free(delivered);munmap(shared,sizeof(size_t));parallel_process_work(w);return;
 }
 signal(SIGPIPE,SIG_IGN);
 size_t started=0;
 for(size_t i=0;i<workers;i++){
  if(pipe(fds[i])!=0)break;
  pid_t pid=fork();
  if(pid<0){close(fds[i][0]);close(fds[i][1]);break;}
  if(pid==0){
   close(fds[i][0]);
   for(size_t k=0;k<i;k++){close(fds[k][0]);close(fds[k][1]);}
   int outfd=fds[i][1];
   Binding iteration_slot={.name=w->variable->value,.value={.type=V_NULL}};
   LocalFrame frame={.slots=&iteration_slot,.count=1};
   frame.buckets[binding_hash(iteration_slot.name)&255u]=&iteration_slot;
   frame.parent=local_frame;local_frame=&frame;
   for(;;){
    size_t idx=__atomic_fetch_add(shared,1,__ATOMIC_SEQ_CST);
    if(idx>=w->count)break;
    free_value(iteration_slot.value);iteration_slot.value=copy_value(w->items[idx]);
    runtime_failed=0;error_message[0]=0;
    Value res=eval(w->target);
    ByteBuf b={0};
    if(runtime_failed){
     size_t m=strlen(error_message);
     bb_u8(&b,1);bb_u32(&b,(uint32_t)m);bb_bytes(&b,error_message,m);
    }else{
     bb_u8(&b,0);encode_value(&b,res,0);
     if(b.failed){
      free(b.data);b.data=NULL;b.len=0;b.cap=0;b.failed=0;
      const char*msg="resultado não serializável entre processos";
      bb_u8(&b,1);bb_u32(&b,(uint32_t)strlen(msg));bb_bytes(&b,msg,strlen(msg));
     }
    }
    free_value(res);
    uint32_t head[2]={(uint32_t)idx,(uint32_t)b.len};
    if(pifari_write_all(outfd,head,8)!=0||pifari_write_all(outfd,b.data,b.len)!=0){free(b.data);_exit(3);}
    free(b.data);
   }
   close(outfd);_exit(0);
  }
  pids[started++]=pid;
 }
 for(size_t k=0;k<started;k++)close(fds[k][1]);
 if(!started){
  free(fds);free(pids);free(delivered);munmap(shared,sizeof(size_t));parallel_process_work(w);return;
 }
 struct pollfd *pfds=calloc(started,sizeof(*pfds));
 unsigned char **rbuf=calloc(started,sizeof(*rbuf));
 size_t *rlen=calloc(started,sizeof(size_t)),*rcap=calloc(started,sizeof(size_t));
 for(size_t k=0;k<started;k++){pfds[k].fd=fds[k][0];pfds[k].events=POLLIN;}
 long long deadline=monotonic_ms()+PIFARI_PROCESS_TIMEOUT_MS;
 size_t alive=started;
 while(alive>0){
  long long remaining=deadline-monotonic_ms();
  if(remaining<=0){for(size_t k=0;k<started;k++)if(pfds[k].fd>=0)kill(pids[k],SIGKILL);break;}
  int ready=poll(pfds,started,(int)(remaining>1000?1000:remaining));
  if(ready<0){if(errno==EINTR)continue;break;}
  if(ready==0)continue;
  for(size_t k=0;k<started;k++){
   if(pfds[k].fd<0||!(pfds[k].revents&(POLLIN|POLLHUP|POLLERR)))continue;
   if(rlen[k]+4096>rcap[k]){
    size_t cap=rcap[k]?rcap[k]*2:8192;
    while(cap<rlen[k]+4096)cap*=2;
    unsigned char*p=realloc(rbuf[k],cap);
    if(!p){pfds[k].fd=-1;alive--;continue;}
    rbuf[k]=p;rcap[k]=cap;
   }
   ssize_t got=read(pfds[k].fd,rbuf[k]+rlen[k],rcap[k]-rlen[k]);
   if(got<=0){if(got<0&&errno==EINTR)continue;pfds[k].fd=-1;alive--;continue;}
   rlen[k]+=(size_t)got;
   size_t pos=0;
   while(rlen[k]-pos>=8){
    uint32_t idx=0,len=0;
    for(int i=0;i<4;i++)idx|=(uint32_t)rbuf[k][pos+i]<<(8*i);
    for(int i=0;i<4;i++)len|=(uint32_t)rbuf[k][pos+4+i]<<(8*i);
    if(rlen[k]-pos-8<len)break;
    const unsigned char*payload=rbuf[k]+pos+8;
    if(idx<w->count&&!delivered[idx]){
     delivered[idx]=1;
     if(len>=1&&payload[0]==0){
      ByteCursor c={payload+1,(size_t)len-1,0,0};
      Value v=decode_value(&c,0);
      if(c.failed){free_value(v);w->errors[idx]=dup_string("resultado inválido devolvido pelo processo");}
      else w->results[idx]=v;
     }else{
      size_t m=(len>=5)?(size_t)len-5:0;
      if(m>255)m=255;
      char msg[256];memcpy(msg,payload+5,m);msg[m]=0;
      w->errors[idx]=dup_string(m?msg:"falha no processo filho");
     }
    }
    pos+=8+len;
   }
   if(pos){memmove(rbuf[k],rbuf[k]+pos,rlen[k]-pos);rlen[k]-=pos;}
  }
 }
 for(size_t k=0;k<started;k++){
  if(pfds[k].fd>=0){close(pfds[k].fd);pfds[k].fd=-1;}
  int status=0;
  while(waitpid(pids[k],&status,0)<0&&errno==EINTR){}
  free(rbuf[k]);
 }
 for(size_t i=0;i<w->count;i++)if(!delivered[i]&&!w->errors[i])
  w->errors[i]=dup_string("processo não entregou resultado");
 free(pfds);free(rbuf);free(rlen);free(rcap);free(delivered);free(fds);free(pids);
 munmap(shared,sizeof(size_t));
}
static Value eval_parallel_flow(const Node *n){
 const Node *source=n->count>0?n->children[0]:NULL,*target=n->count>1?n->children[1]:NULL;
 if(!source||strcmp(source->kind,"Flow")||strcmp(source->value,">|")||source->count<2||!target){
  fail(n,"use o fluxo paralelo como iterável >| variável >| expressão");return vnull();
 }
 const Node *iterable=source->children[0],*variable=source->children[1];
 if(strcmp(variable->kind,"Name")){fail(n,"o nome após >| deve ser um identificador");return vnull();}
 Value seq=eval(iterable);if(runtime_failed){free_value(seq);return vnull();}
 Value fused_result=vnull();
 /* O caminho nativo fundido usa threads; em modo processos ele é ignorado para
    não misturar os dois modelos de execução no mesmo fluxo. */
 int fused_status=execution_processes?0:try_native_map_flow(target,variable,seq,&fused_result);
 if(fused_status!=0){free_value(seq);return fused_status>0?fused_result:vnull();}
 size_t count=seq.type==V_LIST?seq.as.list.count:seq.type==V_F64_ARRAY?seq.as.array->length:((seq.type==V_STRING)?strlen(seq.as.s):((seq.type==V_RANGE)?(size_t)seq.as.range_end:0));
 if(seq.type!=V_LIST&&seq.type!=V_F64_ARRAY&&seq.type!=V_STRING&&seq.type!=V_RANGE){fail(n,"o lado esquerdo de >| deve ser uma lista, array tipado, texto ou range");free_value(seq);return vnull();}
 Value *items=NULL;
 if(seq.type==V_LIST){
  /* Transfer ownership of the evaluated list buffer instead of deep-copying every
     element into a second array; workers copy only the current item into their slot. */
  items=seq.as.list.items;
 }else{
  items=calloc(count?count:1,sizeof(Value));
  if(!items){free_value(seq);fail(n,"memória insuficiente ao preparar tarefas paralelas");return vnull();}
  for(size_t i=0;i<count;i++){if(seq.type==V_RANGE)items[i]=vint((long long)i);else if(seq.type==V_F64_ARRAY)items[i]=vfloat(seq.as.array->data[i]);else{char c[2]={seq.as.s[i],0};items[i]=vstr(c);}}
 }
 Value out=list_new(count);
 if(out.type!=V_LIST){if(seq.type!=V_LIST){for(size_t i=0;i<count;i++)free_value(items[i]);free(items);}free_value(seq);fail(n,"memória insuficiente para resultados paralelos");return vnull();}
 char **errors=calloc(count?count:1,sizeof(char*));
 ParallelWork w={.next=0,.count=count,.items=items,.results=out.as.list.items,.errors=errors,.target=target,.variable=variable,.at=n,.base=globals};
 if(count){if(execution_processes)process_map(&w,execution_workers);else pool_run(&w);}
 if(seq.type!=V_LIST){for(size_t i=0;i<count;i++)free_value(items[i]);free(items);}
 free_value(seq);
 size_t first_bad=0;char *first_msg=NULL;
 for(size_t i=0;i<count;i++){
  if(!errors[i])continue;
  if(!first_msg){first_msg=errors[i];first_bad=i;}
  else free(errors[i]);
 }
 free(errors);
 if(first_msg){
  if(!runtime_failed)fail(n,"fluxo paralelo falhou no índice %zu: %s",first_bad,first_msg);
  free(first_msg);
 }
 return out;
}
/* Resolve nested list indexing without deep-copying each intermediate matrix row.
   The returned pointer is borrowed from the current environment and must not escape. */
static Value *resolve_list_reference(const Node*n){
 if(!n)return NULL;
 if(!strcmp(n->kind,"Name")){
  Binding*b=find_binding(n->value);
  return b?&b->value:NULL;
 }
 if(!strcmp(n->kind,"Index")&&n->count==2){
  Value*base=resolve_list_reference(n->children[0]);
  if(!base||base->type!=V_LIST)return NULL;
  Value idx=eval(n->children[1]);
  if(runtime_failed){free_value(idx);return NULL;}
  if(idx.type!=V_INT){free_value(idx);fail(n,"índice deve ser inteiro");return NULL;}
  long long i=idx.as.i;if(i<0)i+=(long long)base->as.list.count;
  free_value(idx);
  if(i<0||(size_t)i>=base->as.list.count){fail(n,"índice fora dos limites");return NULL;}
  return &base->as.list.items[i];
 }
 return NULL;
}
static Value eval(const Node*n){
 if(!n||runtime_failed)return vnull();
 profile_inc(&profile_eval_nodes);
 if(n->constant_folded)return n->constant_is_float?vfloat(n->constant_float):vint(n->constant_integer);
 if(!strcmp(n->kind,"Integer")){if(n->numeric_cached)return vint(n->integer_cache);char*e;long long x=strtoll(n->value,&e,10);if(*e){fail(n,"inteiro inválido");return vnull();}return vint(x);}
 if(!strcmp(n->kind,"Float")){if(n->numeric_cached)return vfloat(n->float_cache);char*e;double x=strtod(n->value,&e);if(*e){fail(n,"real inválido");return vnull();}return vfloat(x);}
 if(!strcmp(n->kind,"String"))return vstr(n->value);
 if(!strcmp(n->kind,"Boolean"))return vbool(!strcmp(n->value,"true"));
 if(!strcmp(n->kind,"Null"))return vnull();
 if(!strcmp(n->kind,"Name")){
  if(n->local_slot>=0&&n->slot_owner){LocalFrame*f=find_owner_frame(n->slot_owner);if(f&&(size_t)n->local_slot<f->count)return copy_value(f->slots[n->local_slot].value);}
  Binding*b=find_binding(n->value);if(!b){fail(n,"variável '%s' não foi declarada",n->value);return vnull();}return copy_value(b->value);
 }
 if(!strcmp(n->kind,"Binary")){if(bc_ensure(n))return eval_bytecode(n);return eval_binary(n);}
 if(!strcmp(n->kind,"Unary")){
  if(bc_ensure(n))return eval_bytecode(n);
  Value v=eval(n->children[0]),result=vnull();
  if(!strcmp(n->value,"-")){if(v.type==V_INT)result=vint(-v.as.i);else result=vfloat(-number(v,n));}
  else if(!strcmp(n->value,"not"))result=vbool(!truthy(v));
  else fail(n,"operador unário não suportado: %s",n->value);
  free_value(v);return runtime_failed?vnull():result;
 }
 if(!strcmp(n->kind,"Call"))return eval_call(n);
 if(!strcmp(n->kind,"List")){const Node*a=n->count?n->children[0]:NULL;size_t count=a?a->count:0;Value v=list_new(count);if(v.type!=V_LIST){fail(n,"memória insuficiente ao criar lista");return vnull();}for(size_t i=0;i<count;i++){v.as.list.items[i]=eval(a->children[i]);if(runtime_failed){free_value(v);return vnull();}}return v;}
 if(!strcmp(n->kind,"Index")){
  if(bc_ensure(n))return eval_bytecode(n);
  /* Fast path for list/matrix indexing: borrow the base list and copy only
     the selected value, rather than recursively cloning the entire list. */
  if(n->count==2&&(!strcmp(n->children[0]->kind,"Name")||!strcmp(n->children[0]->kind,"Index"))){
   Value*base=resolve_list_reference(n->children[0]);
   if(base&&(base->type==V_LIST||base->type==V_F64_ARRAY)){
    Value idx=eval(n->children[1]);
    if(runtime_failed){free_value(idx);return vnull();}
    if(idx.type!=V_INT){free_value(idx);fail(n,"indexação exige índice inteiro");return vnull();}
    long long length=base->type==V_LIST?(long long)base->as.list.count:(long long)base->as.array->length;
    long long i=idx.as.i;if(i<0)i+=length;free_value(idx);
    if(i<0||i>=length){fail(n,"índice fora dos limites");return vnull();}
    return base->type==V_F64_ARRAY?vfloat(base->as.array->data[i]):copy_value(base->as.list.items[i]);
   }
   if(runtime_failed)return vnull();
  }
  Value v=eval(n->children[0]),idx=eval(n->children[1]);
  if((v.type!=V_LIST&&v.type!=V_RANGE&&v.type!=V_F64_ARRAY)||idx.type!=V_INT){free_value(v);free_value(idx);fail(n,"indexação exige lista, array tipado/range e índice inteiro");return vnull();}
  long long count=v.type==V_LIST?(long long)v.as.list.count:v.type==V_F64_ARRAY?(long long)v.as.array->length:v.as.range_end;
  long long i=idx.as.i;if(i<0)i+=count;
  if(i<0||i>=count){free_value(v);free_value(idx);fail(n,"índice fora dos limites");return vnull();}
  Value result=v.type==V_RANGE?vint(i):v.type==V_F64_ARRAY?vfloat(v.as.array->data[i]):copy_value(v.as.list.items[i]);
  free_value(v);free_value(idx);return result;
 }
 if(!strcmp(n->kind,"Flow")&&!strcmp(n->value,">|"))return eval_parallel_flow(n);
 if(!strcmp(n->kind,"Flow")&&!strcmp(n->value,"<|")){
  if(n->count<2){fail(n,"operador <| exige uma continuação à direita");return vnull();}
  Value completed=eval(n->children[0]);free_value(completed);if(runtime_failed)return vnull();
  return eval(n->children[1]);
 }
 if(!strcmp(n->kind,"Flow")&&!strcmp(n->value,"|<")){
  Value collected=eval(n->children[0]);if(runtime_failed){free_value(collected);return vnull();}
  if(collected.type!=V_LIST){free_value(collected);fail(n,"|< só pode coletar os resultados de um fluxo que retorna uma lista");return vnull();}
  /* Agregação com ligação (SPEC_CONCORRENCIA §2.1): a barreira produz a lista e a
     liga ao nome à direita. A forma puramente expressiva fica adiada porque exigiria
     um delimitador sintático; ver docs/EXECUCAO.md. */
  if(n->count<2||strcmp(n->children[1]->kind,"Name")){free_value(collected);fail(n,"use |< nome_da_colecao para guardar os resultados");return vnull();}
  set_node_binding(n,n->children[1]->value,collected,n);return vnull();
 }
 if(!strcmp(n->kind,"Flow")&&!strcmp(n->value,"|>")){
  const Node *source=n->count>0?n->children[0]:NULL,*target=n->count>1?n->children[1]:NULL;
  if(!source||strcmp(source->kind,"Flow")||strcmp(source->value,"|>")||source->count<2||!target){
   fail(n,"use o fluxo como iterável |> variável |> expressão");return vnull();
  }
  const Node *iterable_node=source->children[0],*variable_node=source->children[1];
  if(strcmp(variable_node->kind,"Name")){fail(n,"o nome após |> deve ser um identificador");return vnull();}
  Value seq=eval(iterable_node);
  if(runtime_failed){free_value(seq);return vnull();}
  size_t seq_count=seq.type==V_LIST?seq.as.list.count:seq.type==V_F64_ARRAY?seq.as.array->length:
                   seq.type==V_STRING?strlen(seq.as.s):seq.type==V_RANGE?(size_t)seq.as.range_end:0;
  if(seq.type!=V_LIST&&seq.type!=V_F64_ARRAY&&seq.type!=V_STRING&&seq.type!=V_RANGE){
   fail(n,"o lado esquerdo do fluxo deve ser uma lista, array tipado, texto ou range");free_value(seq);return vnull();
  }
  /* Mesma regra de escopo do fluxo paralelo: a variável de iteração vive em um
     frame próprio e não vaza para o chamador (SPEC_CONCORRENCIA §3). */
  Binding iteration_slot={.name=variable_node->value,.value={.type=V_NULL}};
  LocalFrame frame={.slots=&iteration_slot,.count=1};
  frame.buckets[binding_hash(iteration_slot.name)&255u]=&iteration_slot;
  frame.parent=local_frame;local_frame=&frame;
  for(size_t i=0;i<seq_count&&!runtime_failed&&!cancel_requested;i++){
   free_value(iteration_slot.value);iteration_slot.value=vnull();
   if(seq.type==V_LIST)iteration_slot.value=copy_value(seq.as.list.items[i]);
   else if(seq.type==V_F64_ARRAY)iteration_slot.value=vfloat(seq.as.array->data[i]);
   else if(seq.type==V_RANGE)iteration_slot.value=vint((long long)i);
   else {char c[2]={seq.as.s[i],0};iteration_slot.value=vstr(c);}
   (void)eval(target);
  }
  free_value(iteration_slot.value);local_frame=frame.parent;free_value(seq);
  return vnull();
 }
  if(!strcmp(n->kind,"Spawn")){
  if(n->count<1){fail(n,"spawn exige uma expressão");return vnull();}
  long long id=pifari_task_spawn(n->children[0]);
  return id<0?vnull():vint(id);
 }
 if(!strcmp(n->kind,"Await")){
  Value idv=eval(n->children[0]);
  if(runtime_failed){free_value(idv);return vnull();}
  if(idv.type!=V_INT){free_value(idv);fail(n,"await espera o identificador devolvido por spawn");return vnull();}
  long long id=idv.as.i;free_value(idv);
  return pifari_task_await(n,id);
 }
 fail(n,"expressão não suportada: %s",n->kind);return vnull();
}
static void exec_block(const Node*n){if(!n)return;for(size_t i=0;i<n->count&&!runtime_failed&&!return_active&&!PIFARI_TASK_CANCELLED();i++)exec_node(n->children[i]);}
static int try_assign_list_append_in_place(const Node *n){
 if(!n->value||n->count<1)return 0;
 const Node *rhs=n->children[0];
 if(!rhs||strcmp(rhs->kind,"Binary")||!rhs->value||strcmp(rhs->value,"+")||rhs->count!=2)return 0;
 const Node *lhs_var=rhs->children[0],*rhs_add=rhs->children[1];
 if(!lhs_var||strcmp(lhs_var->kind,"Name")||!lhs_var->value||strcmp(lhs_var->value,n->value))return 0;
 if(!rhs_add||strcmp(rhs_add->kind,"List")||rhs_add->count!=1)return 0;
 const Node *list_args=rhs_add->children[0];
 if(!list_args||list_args->count!=1)return 0;
 Binding *b=node_target_binding(n,n->value);
 if(!b||b->value.type!=V_LIST||!b->value.as.list.items)return 0;
 Value item=eval(list_args->children[0]);
 if(runtime_failed){free_value(item);return 1;}
 b=node_target_binding(n,n->value);
 if(!b||b->value.type!=V_LIST||!b->value.as.list.items){free_value(item);return 0;}
 ListStorage *s=list_storage(b->value);
 size_t old_count=b->value.as.list.count,need=old_count+1;
 if(__atomic_load_n(&s->refs,__ATOMIC_ACQUIRE)==1){
  if(need>s->capacity){
   size_t new_cap=s->capacity<16?16:s->capacity*2;
   if(new_cap<need)new_cap=need;
   ListStorage *ns=realloc(s,sizeof(ListStorage)+new_cap*sizeof(Value));
   if(!ns){free_value(item);fail(n,"out of memory");return 1;}
   ns->capacity=new_cap;s=ns;b->value.as.list.items=s->items;
  }
  s->items[old_count]=item;
  b->value.as.list.count=need;
  return 1;
 }
 Value out=list_new(need);
 if(out.type==V_NULL){free_value(item);fail(n,"out of memory");return 1;}
 for(size_t i=0;i<old_count;i++)out.as.list.items[i]=copy_value(b->value.as.list.items[i]);
 out.as.list.items[old_count]=item;
 set_node_binding(n,n->value,out,n);
 return 1;
}
/* Efeitos de uma expressão/corpo (spec §384). Usado para decidir se um plano
   rápido é seguro: só função pura pode virar kernel nativo. */
static unsigned compute_effects(const Node *n, int depth) {
 if (!n || depth > 32) return EFFECT_IO;
 unsigned total = 0;
 if (!strcmp(n->kind, "Call")) {
  const char *name = n->value ? n->value : "";
  FunctionDef *fn = NULL;
  for (FunctionDef *f = functions; f; f = f->next)
   if (!strcmp(f->name, name)) { fn = f; break; }
  if (fn) total |= fn->effects_ready ? fn->effects : (unsigned)EFFECT_IO;
  else total |= (unsigned)pifari_builtin_effect(name);
 }
 if (!strcmp(n->kind, "Spawn") || !strcmp(n->kind, "Await")) total |= EFFECT_IO;
 if (!strcmp(n->kind, "Function") || !strcmp(n->kind, "FunctionExpr")) return total;
 for (size_t i = 0; i < n->count; i++) total |= compute_effects(n->children[i], depth + 1);
 return total;
}
static void exec_node(const Node*n){
 if(!n||runtime_failed)return;
 if(!strcmp(n->kind,"Program")||!strcmp(n->kind,"Block")||!strcmp(n->kind,"Statements")){exec_block(n);return;}
 if(!strcmp(n->kind,"Assign")&&try_assign_list_append_in_place(n))return;
 if(!strcmp(n->kind,"TypedDecl")||!strcmp(n->kind,"Attribute")){
  if(!n->value||!n->count){fail(n,"declaração tipada inválida");return;}
  const Node*type_node=n->children[0];
  PifariType ann=PT_ANY;
  if(type_node&&!strcmp(type_node->kind,"Type")){
   if(!pifari_type_is_known(type_node->value)){
    fail(n,"tipo desconhecido: '%s'",type_node->value?type_node->value:"?");
    return;
   }
   ann=pifari_type_from_name(type_node->value);
  }
  const Node*init=NULL;
  if(!strcmp(n->kind,"TypedDecl")){if(n->count>1)init=n->children[1];else if(n->count==1&&strcmp(n->children[0]->kind,"Type"))init=n->children[0];}
  Value v=init?eval(init):vnull();if(runtime_failed)return;
  if(init){
   PifariType actual=value_type_of(v);
   int compat=pifari_type_assignable(ann,actual);
   if(!compat){
    fail(n,"'%s' espera %s, recebeu %s",n->value,pifari_type_name(ann),pifari_type_name(actual));
    free_value(v);return;
   }
   if(compat==2&&v.type==V_INT)v=vfloat((double)v.as.i);
  }
  set_node_binding(n,n->value,v,n);
  Binding*bound=find_binding(n->value);
  if(bound)bound->kind=ann;
  return;
 }
 if(!strcmp(n->kind,"IndexAssign")){
  Binding *binding=n->value?node_target_binding(n,n->value):NULL;if(!binding){fail(n,"array/lista '%s' não foi declarado",n->value?n->value:"?");return;}
  Value idx=eval(n->children[0]);if(runtime_failed){free_value(idx);return;}Value val=eval(n->children[1]);if(runtime_failed){free_value(idx);free_value(val);return;}
  if(idx.type!=V_INT){free_value(idx);free_value(val);fail(n,"índice deve ser inteiro");return;}
  if(binding->value.type==V_F64_ARRAY&&binding->value.as.array){
   if(val.type!=V_INT&&val.type!=V_FLOAT){free_value(idx);free_value(val);fail(n,"atribuição em f64_array exige valor numérico");return;}
   long long i=idx.as.i;size_t length=binding->value.as.array->length;if(i<0)i+=(long long)length;
   if(i<0||(size_t)i>=length){free_value(idx);free_value(val);fail(n,"índice fora dos limites");return;}
   binding->value.as.array->data[i]=number(val,n);free_value(idx);free_value(val);return;
  }
  if(binding->value.type==V_LIST){
   long long i=idx.as.i;size_t length=binding->value.as.list.count;if(i<0)i+=(long long)length;
   if(i<0||(size_t)i>=length){free_value(idx);free_value(val);fail(n,"índice fora dos limites");return;}
   free_value(binding->value.as.list.items[i]);binding->value.as.list.items[i]=val;free_value(idx);return;
  }
  free_value(idx);free_value(val);fail(n,"atribuição indexada exige f64_array ou lista");return;
 }
 if(!strcmp(n->kind,"IndexAssignNested")){
  if(n->count<3){fail(n,"atribuição indexada aninhada inválida");return;}
  Value *base=resolve_list_reference(n->children[0]);
  if(!base){if(!runtime_failed)fail(n,"base de atribuição indexada inválida");return;}
  Value idx=eval(n->children[1]);if(runtime_failed){free_value(idx);return;}
  Value val=eval(n->children[2]);if(runtime_failed){free_value(idx);free_value(val);return;}
  if(idx.type!=V_INT){free_value(idx);free_value(val);fail(n,"índice deve ser inteiro");return;}
  if(base->type==V_F64_ARRAY&&base->as.array){
   if(val.type!=V_INT&&val.type!=V_FLOAT){free_value(idx);free_value(val);fail(n,"atribuição em f64_array exige valor numérico");return;}
   long long i=idx.as.i;size_t length=base->as.array->length;if(i<0)i+=(long long)length;
   if(i<0||(size_t)i>=length){free_value(idx);free_value(val);fail(n,"índice fora dos limites");return;}
   base->as.array->data[i]=number(val,n);free_value(idx);free_value(val);return;
  }
  if(base->type==V_LIST){
   long long i=idx.as.i;size_t length=base->as.list.count;if(i<0)i+=(long long)length;
   if(i<0||(size_t)i>=length){free_value(idx);free_value(val);fail(n,"índice fora dos limites");return;}
   free_value(base->as.list.items[i]);base->as.list.items[i]=val;free_value(idx);return;
  }
  free_value(idx);free_value(val);fail(n,"atribuição indexada exige f64_array ou lista");return;
 }
 if(!strcmp(n->kind,"Assign")){if(!n->value||!n->count){fail(n,"atribuição inválida");return;}Value v=eval(n->children[0]);if(!runtime_failed)set_node_binding(n,n->value,v,n);return;}
 if(!strcmp(n->kind,"ExpressionStatement")){if(n->count)(void)eval(n->children[0]);return;}
 if(!strcmp(n->kind,"If")){if(n->count>=2){Value c=eval(n->children[0]);if(truthy(c))exec_node(n->children[1]);free_value(c);}return;}
 if(!strcmp(n->kind,"IfElse")){if(n->count>=2){const Node*condition=n->children[0];Value c=condition->count?eval(condition->children[0]):vnull();if(truthy(c)){if(condition->count>1)exec_node(condition->children[1]);}else exec_node(n->children[1]);free_value(c);}return;}
 if(!strcmp(n->kind,"IfElseIf")){if(n->count>=2){const Node*c=n->children[0],*e=n->children[1];Value v=c->count?eval(c->children[0]):vnull();if(truthy(v)){if(c->count>1)exec_node(c->children[1]);}else if(e->count>1){Value ev=eval(e->children[0]);if(truthy(ev))exec_node(e->children[1]);free_value(ev);}free_value(v);}return;}
 if(!strcmp(n->kind,"For")){if(n->count<2){fail(n,"laço for inválido");return;}Value seq=eval(n->children[0]);if(seq.type==V_LIST){Binding *loop_binding=find_binding(n->value);for(size_t i=0;i<seq.as.list.count&&!runtime_failed&&!return_active&&!PIFARI_TASK_CANCELLED();i++){loop_binding=set_loop_binding(loop_binding,n->value,copy_value(seq.as.list.items[i]),n);exec_node(n->children[1]);}}else if(seq.type==V_F64_ARRAY){Binding *loop_binding=find_binding(n->value);for(size_t i=0;i<seq.as.array->length&&!runtime_failed&&!return_active&&!PIFARI_TASK_CANCELLED();i++){loop_binding=set_loop_binding(loop_binding,n->value,vfloat(seq.as.array->data[i]),n);exec_node(n->children[1]);}}else if(seq.type==V_RANGE){Binding *loop_binding=find_binding(n->value);for(long long i=0;i<seq.as.range_end&&!runtime_failed&&!return_active&&!PIFARI_TASK_CANCELLED();i++){loop_binding=set_loop_binding(loop_binding,n->value,vint(i),n);exec_node(n->children[1]);}}else if(seq.type==V_STRING){Binding *loop_binding=find_binding(n->value);for(size_t i=0;seq.as.s[i]&&!runtime_failed&&!return_active;i++){char c[2]={seq.as.s[i],0};loop_binding=set_loop_binding(loop_binding,n->value,vstr(c),n);exec_node(n->children[1]);}}else fail(n,"for exige uma lista, texto ou range iterável");free_value(seq);return;}
  if(!strcmp(n->kind,"Return")){if(return_active){fail(n,"return aninhado inválido");return;}Value v=n->count?eval(n->children[0]):vnull();if(!runtime_failed){return_value=v;return_active=1;}return;}
 if(!strcmp(n->kind,"Function")||!strcmp(n->kind,"FunctionExpr")){
  for(FunctionDef*f=functions;f;f=f->next)if(!strcmp(f->name,n->value)){
   fail(n,"função '%s' já foi definida",n->value);return;
  }
  FunctionDef*f=calloc(1,sizeof(*f));
  if(!f){fail(n,"memória insuficiente ao registrar função");return;}
  f->name=n->value;f->node=n;f->next=functions;functions=f;
  prepare_function_slots(f);
  f->effects=compute_effects(n->count>1?n->children[1]:NULL,0);f->effects_ready=1;
  char ir_error[160];
  if(pifari_ast_function_to_ir(n,&f->ir,ir_error,sizeof(ir_error))==0)
   f->ir_ready=1;
  jit_compile_function(f);
  return;
 }
 if(!strcmp(n->kind,"Break")||!strcmp(n->kind,"Continue")){fail(n,"%s ainda não é suportado pelo runtime",n->kind);return;}
 if(!strcmp(n->kind,"Import")||!strcmp(n->kind,"Include")||!strcmp(n->kind,"LinkTo")){fail(n,"diretiva '%s' não é suportada nesta versão; remova-a ou aguarde implementação",n->kind);return;}
 if(!strcmp(n->kind,"Class")){fail(n,"classes são reconhecidas pelo parser, mas objetos e métodos ainda não são executáveis");return;}
 (void)eval(n);
}
int runtime_execute(const Node*program){
 profiling_enabled=getenv("PIFARI_PROFILE")!=NULL;
 profile_eval_nodes=profile_binding_lookups=profile_value_copies=profile_bytecode_ops=0;
 globals=NULL;local_frame=NULL;memset(binding_buckets,0,sizeof(binding_buckets));runtime_failed=0;return_active=0;return_value=vnull();
 free_function_registry();
 exec_node(program);
 pifari_shared_shutdown();
 pifari_tasks_shutdown();
 pool_shutdown();
 pifari_gpu_shutdown();
 agent_runtime_reset();
 Binding*b=globals;while(b){Binding*next=b->next;free(b->name);free_value(b->value);free(b);b=next;}globals=NULL;memset(binding_buckets,0,sizeof(binding_buckets));
 free_function_registry();
 chain_kernel_cache_clear();
 free_value(return_value);return_value=vnull();return_active=0;
 if(profiling_enabled)fprintf(stderr,"PIFARI_PROFILE eval_nodes=%llu binding_lookups=%llu value_copies=%llu bytecode_ops=%llu\n",profile_eval_nodes,profile_binding_lookups,profile_value_copies,profile_bytecode_ops);
 return runtime_failed?1:0;
}
