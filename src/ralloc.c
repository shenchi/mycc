#include "ralloc.h"
#include "memmgr.h"
#include "symmgr.h"

// extern from asmgen module
extern void emit(char *inst, int f, ident_t op1, ident_t op2);
extern void salloc(ident_t);

typedef struct list_s* list_t;
struct list_s{
    ident_t id;
    list_t  next;
};

// recent go first
list_t record = 0;

int reg_state = ~0;

void listadd(ident_t id){
    list_t n = (list_t)mmalloc(sizeof(struct list_s));
    n->id = id;
    n->next = record;
    record = n;
}

int listoldest(int regmask){
    int ret = 0;
    list_t p = record;
    while(p){
        if(((store_t)(p->id->extra))->cur_reg & regmask)
            ret = ((store_t)(p->id->extra))->cur_reg;
        p = p->next;
    }
    return ret;
}

void rstore(ident_t id){
    if(((store_t)(id->extra))->addr < 0)
        salloc(id);
    emit("MOV", 0x21, id, id);
    ((store_t)(id->extra))->cur_reg = 0;
}

extern int current_index;

void rfree(int reg){
	list_t p = record, q = 0;
	if(!reg)return;
    if(rstate(reg))return;
    runlock(reg);

    while(p){
        if(((store_t)(p->id->extra))->cur_reg == reg){
            if(q)
                q->next = p->next;
            else
                record = p->next;
			if(p->id->loc == LOC_GLOBAL || p->id->type == VAR || (p->id->last >= 0 && current_index <= p->id->last))
				rstore(p->id);
            ((store_t)(p->id->extra))->cur_reg = 0;
            mfree(p);
            break;
        }
        q = p;
        p = p->next;
    }
}

int ralloc(ident_t id, int regmask){
    int reg = 0, i;
	if(((store_t)(id->extra))->cur_reg) return ((store_t)(id->extra))->cur_reg;
    if(((store_t)(id->extra))->wanted_reg){
        reg = ((store_t)(id->extra))->wanted_reg;
    }else{
        for(i = R_EAX; i < R_END; i<<=1)
			if((i & regmask) && rstate(i)){
                reg = i;
				break;
			}
        if(!reg)reg = listoldest(regmask);
    }

    if(!rstate(reg)){
        rfree(reg);
    }

    rlock(reg);
    ((store_t)(id->extra))->cur_reg=reg;
    listadd(id);
    return reg;
}

void rprotect(){
    int i;
    for(i = R_EAX; i < R_END; i<<=1)
        rfree(i);
}

void rreturn(){
    list_t p = record;
    while(p){
        if(p->id->type == VAR && p->id->loc == LOC_GLOBAL)
            rstore(p->id);
		p=p->next;
    }
}

void rclear(){
    list_t p = record, q;
    reg_state = ~0;
    while(p){
        q = p;
        p=p->next;
        ((store_t)(q->id->extra))->cur_reg = 0;
        mfree(q);
    }
    record = 0;
}

