#include <stdio.h>
#include "qcode.h"
#include "memmgr.h"

qcode_t createQCode(){
    qcode_t r = (qcode_t)mmalloc(sizeof(struct qcode_s));
    r->code = NOP;
    return r;
}

func_t createFunc(){
    func_t r = (func_t)mmalloc(sizeof(struct func_s));
    r->nparam = 0;
    r->params = 0;
    r->codes = 0;
    return r;
}

int insertQCode(func_t f, qcode_t c){
    qcode_t p = f->codes;
    if(!p){
        f->codes = c;
        c->next = 0;
		c->index = 0;
    }else{
        while(p->next){
            p=p->next;
        }
        p->next = c;
        c->next = 0;
		c->index = p->index + 1;
    }
	if(c->op1)c->op1->last = c->index;
	if(c->op2)c->op2->last = c->index;
    return 0;
}

int addparam(func_t f, ident_t id){
    param_t np = (param_t)mmalloc(sizeof(struct param_s));
    np->id = id;
    np->next = f->params;
    f->params = np;
    np->next = 0;
    f->nparam++;
    return 0;
}

char codename[][8] = {
	"NOP    ",
	"MOV    ",
	"ADD    ",
	"SUB    ",
	"MUL    ",
	"DIV    ",
	"CMP    ",
	"NEG    ",
	"FADD   ",
	"FSUB   ",
	"FMUL   ",
	"FDIV   ",
	"FCMP   ",
	"FNEG   ",
	"JGT    ",
	"JGE    ",
	"JLT    ",
	"JLE    ",
	"JEQ    ",
	"JNE    ",
	"JMP    ",
	"PARAM  ",
	"CALL   ",
	"RET    ",
	"IN     ",
	"OUT    "
};

void printQCode(qcode_t c, int t){
    qcode_t p = c;
    int i;
    while(p){
        for(i=0;i<t;i++)printf("\t");
        printf("%s", codename[p->code]);
        if(p->op1)
            printf("%s\t", p->op1->name);
        else
            printf("\t");
        if(p->op2)
            printf("%s\t", p->op2->name);
        else
            printf("\t");
        if(p->opr)
            printf("%s\t", p->opr->name);
        else
            printf("\t");
        printf("\n");
        p = p->next;
    }
}
