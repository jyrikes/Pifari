#include "memory_plan.h"
#include <stdlib.h>
/* Ordenação estável simples: os grafos de expressões normalmente têm poucos temporários. */
size_t pifari_plan_slots(PifariLiveRange*r,size_t n){
 if(!r&&n)return 0;
 for(size_t i=1;i<n;i++){PifariLiveRange x=r[i];size_t j=i;while(j&&r[j-1].first>x.first){r[j]=r[j-1];j--;}r[j]=x;}
 size_t *end=calloc(n?n:1,sizeof(*end));if(!end)return 0;size_t slots=0;
 for(size_t i=0;i<n;i++){size_t s=0;while(s<slots&&end[s]>=r[i].first)s++;if(s==slots)slots++;end[s]=r[i].last;r[i].slot=s;}
 free(end);return slots;
}
