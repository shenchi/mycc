#include "memmgr.h"
#include "symmgr.h"
#include "qcode.h"

typedef struct block_s * block_t;
struct block_s{
	func_t pfunc;
	qcode_t first, last;
	block_t next1,next2;
};

block_t blocklist = 0;

void clearnop(func_t f){
	qcode_t q, p = f->codes;
	symtab_t t;
	while(p){
		if(p->code == NOP){
			while(p->next && p->next->code == NOP){
				t = f->local->next;
				q = p->next;
				while(t){
					if(t->entry->type == LABEL && t->entry->data == (int)q){
						t->entry->data = (int)p;
					}
					t = t->next;
				}
				p->next = q->next;
				p->index = q->index;
				mfree(q);
			}
		}
		p = p->next;
	}
}

block_t cutit(ident_t func){
	block_t p = 0, q, root = (block_t)mmalloc(sizeof(struct block_s));
	qcode_t c, cl;
	root->pfunc = (func_t)(func->extra);
	c = root->pfunc->codes;
	root->first = 0;

	clearnop(root->pfunc);	

	do{
		if(!p){
			p = (block_t)mmalloc(sizeof(struct block_s));
			root->next1 = p;
			p->first = c;
			p->next1 = 0;
			p->next2 = 0;
		}else{
			p->next1 = (block_t)mmalloc(sizeof(struct block_s));
			p = p->next1;
			p->first = c;
			p->next1 = 0;
			p->next2 = 0;
		}

		while(c){
			if(!(c->next) || c->code == RET)break;
			if(c->code >= JGT && c->code <= JMP)break;
			if(c->next && c->next->code == NOP)break;
			c = c->next;
		}
		
		p->last = c;
		cl = c;
		if(c)c = c->next;
		if(c)cl->next = 0;
	}while(c);

	p = root->next1;
	while(p){
		c = p->last;
		q = root->next1;
		while(q){
			if(c->code>=JGT && c->code<=JMP && c->op1->data == (int)(q->first)){
				p->next2 = q;
				break;
			}
			q = q->next1;
		}
		p = p->next1;
	}

	return root;
}

void debuginfo(){
	block_t g = blocklist, l;
	while(g){
		printf("\nNew Func\n");
		l = g->next1;
		while(l){
			printf("Block %x : Next %x, Branch %x\n", l, l->next1, l->next2);
			printQCode(l->first, 1);
			l = l->next1;
		}
		g = g->next2;
	}
}

void optimizer_init(){
	symtab_t p = global->next;
	block_t q = 0;

	while(p){
		if(p->entry->type == FUNC){
			if(!q){
				q = cutit(p->entry);
				blocklist = q;
				q->next2 = 0;
			}else{
				q->next2 = cutit(p->entry);
				q=q->next2;
				q->next2 = 0;
			}
		}
		p = p->next;
	}

	//debuginfo();
}

void linkit(block_t root){
	block_t p = root->next1;
	while(p){
		if(p->next1){
			p->last->next = p->next1->first;
		}
		p = p->next1;
	}
}

void optimizer_finish(){
	block_t p = blocklist;
	while(p){
		linkit(p);
		p->pfunc->codes = p->next1->first;
		printf("New Func:\n");
		printQCode(p->next1->first , 1);
		p = p->next2;
	}
}

extern void dag(qcode_t *first, qcode_t *last);
extern void dag_start_new_fun(symtab_t s);

void optimizer_dag(){
	block_t p = blocklist, q;
	symtab_t newtab, iter;
	while(p){
		q = p->next1;
		newtab = createTable();
		for(iter = p->pfunc->local->next; iter; iter = iter->next){
			if(iter->entry->type!=TEMP)
				insertTable(newtab, iter->entry);
		}
		dag_start_new_fun(newtab);
		while(q){
			//printf("Block %x\n", q);
			if(q->first != q->last)
				dag(&(q->first), &(q->last));
			//printQCode(q->first, 1);
			q = q->next1;
		}
		p->pfunc->local = newtab;
		p = p->next2;
	}
}