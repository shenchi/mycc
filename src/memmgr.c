#include <stdlib.h>
#include "reporter.h"

typedef struct memrec_s * memrec_t;
struct memrec_s{
    int size;
    void* ptr;
    memrec_t next, prev;
};

memrec_t memused, memfree;

void mem_init(){
    memused = 0;
    memfree = 0;
}
void mem_free(){
    memrec_t p,q;
    p = memused;
    while(p){
        free(p->ptr);
        q = p;
        p = q->next;
		free(q);
    }
    p = memfree;
    while(p){
        free(p->ptr);
        q = p;
        p = q->next;
        free(q);
    }
}

void* mmalloc(int size){
    memrec_t p = memfree, q;
    while(p){
        if(p->size >= size){
            if(p==memfree)memfree = p->next;
            if(p->prev)p->prev->next=p->next;
            if(p->next)p->next->prev=p->prev;

            if(memused)memused->prev = p;
            p->next = memused;
            p->prev = 0;
            memused = p;

            return p->ptr;
        }
        p = p->next;
    }
    q = (memrec_t)malloc(sizeof(struct memrec_s));
    if(!q){
        msg(DEBUG, "failed to allocate a mem-node", 0);
        exit(0);
    }
    q->ptr = malloc(size);
    if(!(q->ptr)){
        msg(DEBUG, "failed to allocate memory", 0);
        exit(0);
    }
    if(memused)memused->prev = q;
    q->next = memused;
    q->prev = 0;
    memused = q;
    return q->ptr;
}

void mfree(void* ptr){
    memrec_t p = memused;
    while(p){
        if((int)(p->ptr) == (int)ptr){
            if(p==memused)memused = p->next;
            if(p->prev)p->prev->next=p->next;
            if(p->next)p->next->prev=p->prev;

            if(memfree)memfree->prev = p;
            p->next = memfree;
            p->prev = 0;
            memfree = p;

            return;
        }
        p = p->next;
    }
}
