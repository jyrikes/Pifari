#include "../backend/optimizer.h"
#include "../backend/operator_engine.h"
#include "../backend/memory_plan.h"
#include "../backend/reversible.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
 PifariExprGraph g;pifari_graph_init(&g);
 int x=pifari_graph_add(&g,PEX_INPUT,-1,-1,0),two=pifari_graph_add(&g,PEX_CONST,-1,-1,2);
 int three=pifari_graph_add(&g,PEX_CONST,-1,-1,3),five=pifari_graph_add(&g,PEX_ADD,two,three,0);
 int mul=pifari_graph_add(&g,PEX_MUL,x,five,0),ten=pifari_graph_add(&g,PEX_CONST,-1,-1,10);
 int add=pifari_graph_add(&g,PEX_ADD,mul,ten,0),relu=pifari_graph_add(&g,PEX_MAX0,add,-1,0);g.root=relu;
 assert(pifari_graph_optimize(&g)==0);assert(g.nodes[five].op==PEX_CONST&&g.nodes[five].constant==5);
 double in[]={-10,-2,0,3},out[4];assert(pifari_graph_map(&g,in,out,4)==0);
 assert(out[0]==0&&out[1]==0&&out[2]==10&&out[3]==25);
 size_t n=20000;double *large_in=malloc(n*sizeof(double)),*large_seq=malloc(n*sizeof(double)),*large_par=malloc(n*sizeof(double));assert(large_in&&large_seq&&large_par);
 for(size_t i=0;i<n;i++)large_in[i]=(double)((int)(i%101)-50);
 assert(pifari_graph_map(&g,large_in,large_seq,n)==0);assert(pifari_graph_map_parallel(&g,large_in,large_par,n,4)==0);
 for(size_t i=0;i<n;i++)assert(large_seq[i]==large_par[i]);
 free(large_in);free(large_seq);free(large_par);pifari_graph_destroy(&g);
 pifari_graph_init(&g);int dx=pifari_graph_add(&g,PEX_INPUT,-1,-1,0),dz=pifari_graph_add(&g,PEX_CONST,-1,-1,0);
 g.root=pifari_graph_add(&g,PEX_DIV,dx,dz,0);double bad=0;assert(pifari_graph_optimize(&g)==0);assert(pifari_graph_eval(&g,1,&bad)==-2);pifari_graph_destroy(&g);
 double a[]={-2,0,3},b[3];assert(pifari_kernel_affine_relu(a,b,3,2,-1,4)==0);assert(b[0]==0&&b[1]==0&&b[2]==5);
 size_t big_n=20000;double *big_in=malloc(big_n*sizeof(double)),*big_out=malloc(big_n*sizeof(double));assert(big_in&&big_out);
 for(size_t i=0;i<big_n;i++)big_in[i]=(double)(i%101)-50.0;
 assert(pifari_kernel_affine_relu(big_in,big_out,big_n,2.0,1.0,4)==0);
 for(size_t i=0;i<big_n;i++){double expected=2.0*big_in[i]+1.0;if(expected<0)expected=0;assert(big_out[i]==expected);}free(big_in);free(big_out);
 PifariLiveRange lr[]={{0,2,0},{3,4,0},{1,3,0}};assert(pifari_plan_slots(lr,3)==2);
 uint64_t bits[]={0,1,UINT64_C(0x123456789abcdef0)},original[3];for(int i=0;i<3;i++)original[i]=bits[i];
 pifari_xor_transform(bits,3,UINT64_C(0xa5a5a5a5a5a5a5a5));pifari_xor_transform(bits,3,UINT64_C(0xa5a5a5a5a5a5a5a5));
 for(int i=0;i<3;i++)assert(bits[i]==original[i]);
 PifariReversibleProgram program;pifari_reversible_init(&program);
 assert(pifari_reversible_add_xor(&program,0,UINT64_C(0x55))==0);
 assert(pifari_reversible_add_not(&program,1)==0);
 assert(pifari_reversible_add_swap(&program,0,2)==0);
 uint64_t state[]={11,22,33},saved[]={11,22,33};
 assert(pifari_reversible_apply(&program,state,3)==0);
 assert(state[0]!=saved[0]||state[1]!=saved[1]||state[2]!=saved[2]);
 assert(pifari_reversible_uncompute(&program,state,3)==0);
 for(size_t i=0;i<3;i++)assert(state[i]==saved[i]);
 pifari_reversible_destroy(&program);
 puts("optimizer/operator-engine/memory-plan/reversible: OK");return 0;
}
