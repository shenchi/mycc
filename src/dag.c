#include "memmgr.h"
#include "string.h"
#include "symmgr.h"
#include "qcode.h"

#define DAG_LEAF 0
#define DAG_OP   1
#define DAG_INST 2
#define DAG_DEAD 3

typedef struct dnode_s * dnode;
struct dnode_s{
	opcode_t op;
	int nref, type;
	vartype_t vartype;
	ident_t oprand;
	dnode l, x, y;
	dnode next;
};

typedef struct nodelist_s *nodelist;
struct nodelist_s{
	ident_t id;
	dnode addr;
	int		flag;
	nodelist next;
};

nodelist dnodelist = 0;
dnode nodes = 0;

nodelist findlist(ident_t id){
	nodelist p = dnodelist;
	while(p){
		if(p->id == id)return p;
		p = p->next;
	}
	return 0;
}

dnode getNode(){
	dnode p = nodes;
	while(p){
		if(p->nref == 0 && p->type> DAG_LEAF && p->type < 3)break;
		p = p->next;
	}
	return p;
}

void deleteNodes(){
	dnode p = nodes, q;
	while(p){
		q = p;
		p = p->next;
		mfree(q);
	}
	nodes = 0;
}

void updatelist(ident_t id, dnode addr){
	nodelist p = findlist(id);
	if(p){
		p->addr = addr;
	}else{
		p = (nodelist)mmalloc(sizeof(struct nodelist_s));
		p->id = id;
		p->addr = addr;
		p->flag = 0;
		p->next = dnodelist;
		dnodelist = p;
	}
}

void deletelist(ident_t id){
	nodelist p = dnodelist, q = 0;
	if(dnodelist->id == id){
		q = dnodelist;
		dnodelist = dnodelist->next;
	}else{
		while(p->next){
			q = p->next;
			if(q->id == id){
				p->next = q->next;
				break;
			}
			p = p->next;
		}
	}
	if(q)mfree(q);
}

void clearnodelist(){
	nodelist p = dnodelist, q;
	while(p){
		q = p;
		p = p->next;
		if(q->id && q->id->type == TEMP)
			mfree(q->id);
		mfree(q);
	}
	dnodelist = 0;
}

dnode createLeaf(ident_t id){
	dnode ret = (dnode)mmalloc(sizeof(struct dnode_s));
	ret->nref = 0;
	ret->op = NOP;
	ret->type = DAG_LEAF;
	ret->oprand = id;
	ret->vartype = id->subtype;
	ret->l = ret->x = ret->y = 0;
	ret->next = nodes;
	nodes = ret;
	return ret;
}

dnode createNode(int type, opcode_t op, dnode l, dnode x, dnode y){
	dnode ret = (dnode)mmalloc(sizeof(struct dnode_s));
	ret->nref = 0;
	ret->type = type;
	ret->op = op;
	ret->oprand = 0;
	if(x && y)
		ret->vartype = (x->vartype > y->vartype) ? (x->vartype) : (y->vartype);
	else if(x)
		ret->vartype = x->vartype;
	else if(y)
		ret->vartype = y->vartype;
	else
		ret->vartype = TVOID;
	ret->l = l;
	if(l)l->nref++;
	ret->x = x;
	if(x)x->nref++;
	ret->y = y;
	if(y)y->nref++;
	ret->next = nodes;
	nodes = ret;
	return ret;
}

symtab_t stab;
int namenum;

void dag_start_new_fun(symtab_t s){
	stab = s;
	namenum = 0;
}

qcode_t undag(){
	qcode_t ret = 0, nc;
	dnode cur = nodes;
	nodelist p = dnodelist; 
	ident_t nid;

	while(p){
		if(!(p->flag) && (p->addr->type != DAG_LEAF) && p->id->type == VAR){
			p->flag = 1;
			p->addr->oprand = p->id;
		}
		p = p->next;
	}

	while(cur){
		if(cur->type == DAG_OP && cur->oprand == 0){
			nid = createIdent();
			nid->name = ccgenname(namenum++, '$');
			nid->type = TEMP;
			nid->subtype = cur->vartype;
			nid->loc = LOC_LOCAL;
			insertTable(stab, nid);
			cur->oprand = nid;
		}
		cur = cur->next;
	}
	
	p = dnodelist;
	while(p){
		if(p->id->type == VAR && !(p->flag) && p->addr->oprand != p->id){
			nc = createQCode();
			nc->code = MOV;
			nc->op1 = p->addr->oprand;
			nc->op2 = 0;
			nc->opr = p->id;
			nc->next = ret;
			ret = nc;
		}
		p = p->next;
	}

	cur = getNode();

	while(cur){
		nc = createQCode();
		nc->code = cur->op;
		nc->op1 = nc->op2 = nc->opr = 0;
		if( cur->x ){
			if(cur->type <= DAG_INST)
				nc->op1 = cur->x->oprand;
			cur->x->nref--;
		}

		if( cur->y ){
			if(cur->type <= DAG_INST)
				nc->op2 = cur->y->oprand;
			cur->y->nref--;
		}
		
		if(cur->oprand){
			nc->opr = cur->oprand;
		}

		nc->next = ret;
		ret = nc;

		cur->type = DAG_DEAD;
		if(cur->l){
			cur = cur->l;
			cur->nref--;
		}else
			cur = cur->x;
		
		if(!cur || cur->type == DAG_LEAF || cur->nref)cur = getNode(); 
	}

	return ret;
}

void dag(qcode_t *first, qcode_t *last){
	qcode_t p = *first, ret = 0, q;
	nodelist nr;
	dnode node, x, y, z, cur_inst = 0, new_inst;
	ident_t nid;
	if(p->code == NOP){
		q = p;
		ret = q;
	}

	while(p  && p->code != RET && (p->code < JGT || p->code > JMP)){
		if(p->code == MOV){
			nr = findlist(p->op1);
			if(!nr){
				node = createLeaf(p->op1);
				updatelist(p->op1, node);
			}else{
				node = nr->addr;
			}
			updatelist(p->opr, node);

		} else if(p->code!= CMP && p->code!=FCMP && p->code >= ADD && p->code <=FNEG){
			nr = findlist(p->op1);
			if(!nr){
				node = createLeaf(p->op1);
				updatelist(p->op1, node);
			}else{
				node = nr->addr;
			}
			x = node;
			y = 0;
			if(p->op2){
				nr = findlist(p->op2);
				if(!nr){
					node = createLeaf(p->op2);
					updatelist(p->op2, node);
				}else{
					node = nr->addr;
				}
				y = node;
			}
			nr = dnodelist;
			while(nr){
				z = nr->addr;
				if(z->op == p->code && z->x == x && z->y == y)
					break;
				nr = nr->next;
			}

			if(!nr){
				z = createNode(DAG_OP, p->code, 0, x, y);
			}else{
				z = nr->addr;
			}
			updatelist(p->opr, z);

		} else {
			if(p->op1){
				nr = findlist(p->op1);
				if(!nr){
					x = createLeaf(p->op1);
					updatelist(p->op1, x);
				}else{
					x = nr->addr;
				}
			}else{
				x = 0;
			}

			if(p->op2){
				nr = findlist(p->op2);
				if(!nr){
					y = createLeaf(p->op2);
					updatelist(p->op2, y);
				}else{
					y = nr->addr;
				}
			}else{
				y = 0;
			}

			new_inst = createNode(DAG_INST, p->code, cur_inst, x, y);
			cur_inst = new_inst;

			if(p->opr){
				nid = createIdent();
				nid->name = ccgenname(namenum++, '$');
				nid->type = TEMP;
				nid->subtype = p->opr->subtype;
				nid->loc = LOC_LOCAL;
				insertTable(stab, nid);
				cur_inst->oprand = nid;
				cur_inst->vartype = nid->subtype;
				updatelist(p->opr, cur_inst);
			}else{
				cur_inst->vartype = TVOID;
			}
		}

		p = p->next;
	}

	if(ret)
		ret->next = undag();
	else
		ret = undag();

	*first = ret;

	q = ret;
	while(q->next){
		q = q->next;
	}
	if(p){
		if(p->code == RET && p->opr){
			nr = findlist(p->opr);
			p->opr = nr->addr->oprand;
		}
		*last = p;	
		q->next = p;
		p->next = 0;
	}else{
		*last = q;
	}

	deleteNodes();
	clearnodelist();

}
