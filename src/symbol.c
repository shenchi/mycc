#include "symbol.h"
#include "memmgr.h"

symbol_t Symbol(symtype_t type, int value){
    symbol_t p = (symbol_t)mmalloc(sizeof(struct symbol_s));
    p->type = type;
    p->value = value;
    return p;
}

int isAddOperator(symbol_t sym){
	return (sym->type==PLUS || sym->type==MINUS);
}

int isAssignOperator(symbol_t sym){
	return (sym->type==ASN);
}

int isCharLiteral(symbol_t sym){
	return (sym->type==CHARLIT);
}

int isID(symbol_t sym){
	return (sym->type==ID);
}

int isIntLiteral(symbol_t sym){
	return (sym->type==INTEGER);
}

int isKeyword(symbol_t sym){
	return (sym->type>=CONST && sym->type<=RETURN);
}

int isLiteral(symbol_t sym){
	return isIntLiteral(sym)||isCharLiteral(sym)||isRealLiteral(sym);
}

int isMulOperator(symbol_t sym){
	return (sym->type==MULTI || sym->type==DIVIDE);
}

int isOperator(symbol_t sym){
	return isAddOperator(sym)||isMulOperator(sym)||isAssignOperator(sym)||isEqualOperator(sym);
}

int isEqualOperator(symbol_t sym){
	return (sym->type>=LESS && sym->type<=EQU);
}

int isRealLiteral(symbol_t sym){
	return (sym->type==REAL);
}

int isStringLiteral(symbol_t sym){
	return (sym->type==STRLIT);
}

