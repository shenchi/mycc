#include "string.h"
#include "symmgr.h"
#include "memmgr.h"

symtab_t global = 0;

ident_t createIdent(){
    ident_t r = (ident_t)mmalloc(sizeof(struct ident_s));
    r->name = 0;
    r->extra = 0;
	r->data = 0;
	r->last = -1;
    return r;
}

symtab_t createTable(){
    symtab_t r = (symtab_t)mmalloc(sizeof(struct symtab_s));
    r->name = 0;
    r->next = r->prev = 0;
    return r;
}

int insertTable(symtab_t tab, ident_t id){
    symtab_t p, node = createTable();
    node->name = id->name;
    node->entry = id;
    if(!tab->next){
        tab->next = node;
        node->prev = tab;
    }else{
        p = tab->next;
        while(1){
            if(ccstrcmp(p->name, id->name)==0)
                return 1;
            if(!p->next)break;
            p=p->next;
        }
        p->next = node;
        node->prev=p;
    }
    return 0;
}

ident_t findTable(symtab_t tab, char* name){
    symtab_t node = tab->next;
    while(node){
        if(ccstrcmp(node->name, name)==0){
            return node->entry;
        }
        node = node->next;
    }
    return 0;
}

int removeTable(symtab_t tab, char* name){
    symtab_t node = tab->next;
    while(node){
        if(ccstrcmp(node->name, name)==0){
            if(node->next)
                node->next->prev = node->prev;
            if(node->prev)
                node->prev->next = node->next;
            // fix later
            return 0;
        }
        node = node->next;
    }
    return 1;
}
