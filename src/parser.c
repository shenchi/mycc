//#include <stdio.h>
#include "symbol.h"
#include "lexer.h"
#include "symmgr.h"
#include "qcode.h"
#include "reporter.h"
#include "string.h"
#include "memmgr.h"

symbol_t sym = 0;
func_t context = 0;

int ERROR_STATUS;

int line;
int labnum; // for generate label name

void parser_init(){
    global = createTable();
    line = 0;
    labnum = 0;
    ERROR_STATUS = 0;
}

void parser_free(){

}

void nextSym(){
    mfree(sym);
    sym = readSym();
    line = getLine();
}

symbol_t copySym(symbol_t s){
    return Symbol(s->type, s->value);
}

qcode_t gen(opcode_t code, ident_t op1, ident_t op2, ident_t opr){
    qcode_t c;
    if(context){
        c = createQCode();
        c->code = code;
        c->op1 = op1;
        c->op2 = op2;
        c->opr = opr;
        insertQCode(context, c);
        return c;
    }
    return 0;
}

int isType(symbol_t sym){
    return (sym->type == INT || sym->type == FLOAT || sym->type == CHAR);
}

int typecheck(vartype_t t, symtype_t v){
    if(t==TINT && v==INTEGER)
        return 1;
    if(t==TFLOAT && v==REAL)
        return 1;
    if(t==TCHAR && v==CHARLIT)
        return 1;
    return 0;
}

vartype_t translate(symtype_t t){
    if(t==INT || t==INTEGER)
        return TINT;
    if(t==FLOAT || t==REAL)
        return TFLOAT;
    if(t==CHAR || t==CHARLIT)
        return TCHAR;
    if(t==VOID)
        return TVOID;
    if(t==STRLIT)
        return TSTR;
    return 0;
}

ident_t createid(idtype_t type, vartype_t subtype, char *id, symbol_t val){
    symtab_t tab = global;
    ident_t nid;

    if(context){
        tab = context->local;
        if(findTable(tab, id)){
            msg(DEBUG, "identifier has been defined", line);
			exit(0);
        }
    }

    if(type==CONSTANT || type==LIT){
        if(!typecheck(subtype, val->type)){
            msg(ERR, "type mismatch", line);
			ERROR_STATUS = 1;
        }
    }

    nid = createIdent();
    nid->type = type;
    if(type!=LABEL)
        nid->subtype = subtype;
    nid->name = id;
    if(val)nid->data = val->value;
    nid->extra = 0;
    if(insertTable(tab, nid)){
        msg(DEBUG, "failed to insert identifier", line);
		exit(0);
    }
	if(tab == global) nid->loc = LOC_GLOBAL;
	else nid->loc = LOC_LOCAL;
    return nid;
}

ident_t createLabel(){
    int j, i = labnum++;
    char *labname = ccstring();
    do{
        j = i % 10;
        i = i / 10;
        labname = ccstrcat(labname, (char)(j+'0'));
    }while(i);
    labname = ccstrcat(labname, '@');
    ccstrinv(labname);
    return createid(LABEL, 0, labname, 0);
}

void putLabel(ident_t lab, qcode_t c){
    lab->data = (int)c;
}

int litnum = 0;
ident_t getLiteral(symtab_t tab, symbol_t s){
    symtab_t p = tab->next, q;
    ident_t id;
    while(p){
        id = p->entry;
        if(id->type == LIT && id->subtype == translate(s->type) && id->data == s->value){
            return id;
        }
        q = p;
        p = p->next;
    }
    id = createIdent();
    id->type=LIT;
    id->subtype = translate(s->type);
    id->data = s->value;
    id->name = ccgenname(litnum++, '_');
	id->loc = LOC_GLOBAL;
    insertTable(tab, id);
    return id;
}



//=======================================

void readLiteral(){
    if(isLiteral(sym))
        return;
    if(sym->type == PLUS){
        nextSym();
    }else if(sym->type == MINUS){
        nextSym();
        if(sym->type == REAL){
            ccstrinv((char*)(sym->value));
            sym->value = (int)ccstrcat((char*)(sym->value), '-');
            ccstrinv((char*)(sym->value));
        }else if(sym->type == INTEGER){
            sym->value = -(sym->value);
        }else{
            // error occur;
        }
    }
    return;
}

int tn = 0;
ident_t funccall(symbol_t id);
ident_t expression();

ident_t factor(){
    ident_t ret = 0;
    symbol_t id;
    if(sym->type==ID){
        id = copySym(sym);
        nextSym();
        if(sym->type!=LPAREN){
            if(context)
                ret = findTable(context->local, (char*)(id->value));
            if(!ret)
                ret = findTable(global, (char*)(id->value));
            if(!ret){
                msg(ERR, "undefined identifier", line);
				ERROR_STATUS = 1;
            }
            return ret;
        }
        nextSym();
        if((ret = funccall(id))==0){
            msg(ERR, "no value return from function", line);
			ERROR_STATUS = 1;
        }
        mfree(id);
    }else if(sym->type==LPAREN){
        nextSym();
        ret = expression(tn);
        if(sym->type!=RPAREN){
            msg(ERR, "missing \')\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else if(isLiteral(sym) || isAddOperator(sym)){
        readLiteral();
        ret = getLiteral(global, sym);
        nextSym();
    }
    return ret;
}

ident_t term(){
    ident_t tmp, tmp2, ntmp;
    symtype_t op;
    vartype_t type = TCHAR;
    char *name;
    tmp = factor();
    if(tmp->subtype>type)type=tmp->subtype;
    while(isMulOperator(sym)){
        op = sym->type;
        nextSym();
        tmp2 = factor();
        if(tmp2->subtype>type)type=tmp2->subtype;
        name = ccgenname(tn++, '$');
        if((ntmp=findTable(context->local, name))==0)
            ntmp = createid(TEMP, type, name, 0);
        ntmp->subtype = type;
        if(type==TFLOAT){
            if(op==MULTI)
                gen(FMUL, tmp, tmp2, ntmp);
            else
                gen(FDIV, tmp, tmp2, ntmp);
        }else{
            if(op==MULTI)
                gen(MUL, tmp, tmp2, ntmp);
            else
                gen(DIV, tmp, tmp2, ntmp);
        }
        tmp = ntmp;
    }
    return tmp;
}

ident_t expression(){
    ident_t tmp, tmp2, ntmp;
    symtype_t op;
    vartype_t type = TCHAR;
    char *name;
    int sign = 0;

    if(sym->type==PLUS){
        sign = 0;
        nextSym();
    }else if(sym->type==MINUS){
        sign = 1;
        nextSym();
    }
    tmp = term();
    if(tmp->subtype>type)type=tmp->subtype;

    if(sign){
        name = ccgenname(tn++, '$');
        if((ntmp=findTable(context->local, name))==0)
            ntmp = createid(TEMP, type, name, 0);
        ntmp->subtype = type;
        if(type==TFLOAT)
            gen(FNEG, tmp, 0, ntmp);
        else
            gen(NEG, tmp, 0, ntmp);
		tmp = ntmp;
    }

    while(isAddOperator(sym)){
        op = sym->type;
        nextSym();
        tmp2 = term();
        if(tmp2->subtype>type)type=tmp2->subtype;
        name = ccgenname(tn++, '$');
        if((ntmp=findTable(context->local, name))==0)
            ntmp = createid(TEMP, type, name, 0);
        ntmp->subtype = type;
        if(type==TFLOAT){
            if(op==PLUS)
                gen(FADD, tmp, tmp2, ntmp);
            else
                gen(FSUB, tmp, tmp2, ntmp);
        }else{
            if(op==PLUS)
                gen(ADD, tmp, tmp2, ntmp);
            else
                gen(SUB, tmp, tmp2, ntmp);
        }
        tmp = ntmp;
    }

    return tmp;
}


void statementlist();
int statement();

void returns(){
    ident_t tmp;
    if(sym->type!=LPAREN){
        gen(RET, 0, 0, 0);
        return;
    }
    nextSym();
    tmp = expression(0);
    gen(RET, tmp, 0, 0);
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
}

void printfcall(){
    ident_t tmp, id;
    if(sym->type!=LPAREN){
        msg(ERR, "missing \'(\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    if(sym->type!=STRLIT){
        tmp = expression(0);
        gen(OUT, tmp, 0, 0);
    }else{
        readLiteral();
        id = getLiteral(global, sym);
        gen(OUT, id, 0, 0);
        nextSym();
        if(sym->type==COMMA){
            nextSym();
            tmp = expression(0);
            gen(OUT, tmp, 0, 0);
        }
    }
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
}

void readvariable(symbol_t s){
    ident_t id;
    id = findTable(context->local, (char*)(s->value));
    if(!id) id = findTable(global, (char*)(s->value));
    if(!id){
        msg(ERR, "undefined identifier", line);
		ERROR_STATUS = 1;
    }
    if(id->type!=VAR){
        msg(ERR, "param must be variable", line);
		ERROR_STATUS = 1;
    }
    gen(IN, 0, 0, id);
}

void scanfcall(){
    if(sym->type!=LPAREN){
        msg(ERR, "missing \'(\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    if(sym->type!=ID){
        msg(ERR, "missing params", line);
		ERROR_STATUS = 1;
    }
    readvariable(sym);
    nextSym();
    while(sym->type==COMMA){
        nextSym();
        if(sym->type!=ID){
            msg(ERR, "unexpected \',\' or missing a param", line);
			ERROR_STATUS = 1;
        }
        readvariable(sym);
        nextSym();
    }
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
}

int valueparams(){
    ident_t tmp;
	int ret = 1;
    tmp = expression(0);
    gen(PARAM, tmp, 0, 0);
    while(sym->type==COMMA){
        nextSym();
        tmp = expression(0);
        gen(PARAM, tmp, 0, 0);
		ret++;
    }
	return ret;
}

ident_t funccall(symbol_t s){
	int nparam = 0;
    ident_t id, tmp;
    if(sym->type!=RPAREN){
        nparam = valueparams();
    }
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    id = findTable(global, (char*)(s->value));
    if(!id || id->type!=FUNC){
        msg(ERR, "no this function", line);
		ERROR_STATUS = 1;
    }
	if(nparam != ((func_t)(id->extra))->nparam){
		msg(ERR, "too few or too much params for this function", line);
		ERROR_STATUS = 1;
	}
	if(id->subtype==TVOID)
		tmp = 0;
	else{
		tmp = createid(TEMP, id->subtype, ccgenname(tn++, '$'), 0);
		if(!tmp){
			msg(DEBUG, "failed create temp", line);
			exit(0);
		}
	}
    gen(CALL, id, 0, tmp);
    return tmp;
}

void defaultcase(){
    if(sym->type!=COLON){
        // error occur;
    }
    nextSym();
    statement();
}

void jumpentry(ident_t expr, ident_t nextlab){
    ident_t lit;
    readLiteral();
    if(!isLiteral(sym)){
        // error occur;
    }
    if(sym->type!=INTEGER && sym->type!=CHARLIT){
        // error occur;
    }
    lit = getLiteral(global, sym);
    gen(CMP, expr, lit, 0);
    gen(JNE, nextlab, 0, 0);
    nextSym();
    if(sym->type!=COLON){
        // error occur;
    }
    nextSym();
    statement();
}

void jumptable(ident_t expr, ident_t exitlab){
    ident_t nextlab = 0;
    qcode_t nop;
    while(sym->type==CASE){
        nextSym();
        nextlab = createLabel();
        jumpentry(expr, nextlab);
        gen(JMP, exitlab, 0, 0);
        nop = gen(NOP, 0, 0, 0);
        putLabel(nextlab, nop);
    }
}

void switcher(){
    ident_t expr, exitlab;
    qcode_t nop;
    if(sym->type!=LPAREN){
        // error occur;
    }
    nextSym();
    expr = expression();
    if(sym->type!=RPAREN){
        // error occur;
    }
    nextSym();
    if(sym->type!=LBRACE){
        // error occur;
    }
    nextSym();
    exitlab = createLabel();
    jumptable(expr, exitlab);
    if(sym->type==DEFUALT){
        nextSym();
        defaultcase();
    }
    if(sym->type!=RBRACE){
        // error occur;
    }
    nextSym();

    nop = gen(NOP, 0, 0, 0);
    putLabel(exitlab, nop);
}

ident_t condition(){
    ident_t tmp1, tmp2 = 0, lab;
    symtype_t op = UNEQU;
    opcode_t opcode = NOP;
    vartype_t type = TCHAR;
    tmp1 = expression();
    if(tmp1->subtype>type)type=tmp1->subtype;
    if(isEqualOperator(sym)){
        op = sym->type;
        nextSym();
        tmp2 = expression();
        if(tmp2->subtype>type)type=tmp2->subtype;
    }
    if(type==TFLOAT){
        gen(FCMP, tmp1, tmp2, 0);
    }else{
        gen(CMP, tmp1, tmp2, 0);
    }
    lab = createLabel();
    if(op==LESS){
        opcode = JGE;
    }else if(op==LEQU){
        opcode = JGT;
    }else if(op==GREATER){
        opcode = JLE;
    }else if(op==GEQU){
        opcode = JLT;
    }else if(op==UNEQU){
        opcode = JEQ;
    }else if(op==EQU){
        opcode = JNE;
    }
    gen(opcode, lab, 0, 0);
    return lab;
}

void looper(){
    ident_t lab1, lab2;
    qcode_t nop;
    if(sym->type!=LPAREN){
        msg(ERR, "missing \'(\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    lab1 = createLabel();
    nop = gen(NOP, 0, 0, 0);
    putLabel(lab1, nop);
    lab2 = condition();
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    if(statement()==0){
        msg(ERR, "missing a statement", line);
		ERROR_STATUS = 1;
    }
    gen(JMP, lab1, 0, 0);
    nop = gen(NOP, 0, 0, 0);
    putLabel(lab2, nop);
}

void brancher(){
    ident_t lab;
    qcode_t nop;
    if(sym->type!=LPAREN){
        msg(ERR, "missing \'(\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    lab = condition();
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
    if(statement()==0){
        msg(ERR, "missing a statement", line);
		ERROR_STATUS = 1;
    }
    nop = gen(NOP, 0, 0, 0);
    putLabel(lab, nop);
}

void assign(symbol_t id){
    ident_t tmp, opr = 0;
    tmp = expression(0);
    if(context)opr=findTable(context->local, (char*)(id->value));
    if(!opr)opr = findTable(global, (char*)(id->value));
    if(!opr){
        msg(ERR, "undefined variable", line);
		ERROR_STATUS = 1;
    }
    gen(MOV, tmp, 0, opr);
}

int statement(){
    symbol_t id;
    if(sym->type==IF){
        nextSym();
        brancher();
    }else if(sym->type==WHILE){
        nextSym();
        looper();
    }else if(sym->type==LBRACE){
        nextSym();
        statementlist();
        if(sym->type!=RBRACE){
            msg(ERR, "missing \'}\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else if(sym->type==ID){
        id = copySym(sym);
        nextSym();
        if(sym->type==LPAREN){
            nextSym();
            funccall(id);
        }else if(sym->type==ASN){
            nextSym();
            assign(id);
        }else{
            msg(ERR, "expect a \'=\' or \'(\'", line);
			ERROR_STATUS = 1;
        }
        mfree(id);
        if(sym->type!=SEMIC){
            msg(ERR, "missing \';\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else if(sym->type==PRINTF){
        nextSym();
        printfcall();
        if(sym->type!=SEMIC){
            msg(ERR, "missing \';\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else if(sym->type==SCANF){
        nextSym();
        scanfcall();
        if(sym->type!=SEMIC){
            msg(ERR, "missing \';\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else if(sym->type==SWITCH){
        nextSym();
        switcher();
    }else if(sym->type==RETURN){
        nextSym();
        returns();
        if(sym->type!=SEMIC){
            msg(ERR, "missing \';\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
    }else return 0;// a 0 statement

    return 1;
}

void statementlist(){
    while(statement());
}

ident_t createvar(symbol_t t, symbol_t id){
    return createid(VAR, translate(t->type), (char*)(id->value), 0);
}

void vardef(symbol_t t, symbol_t id){
    while(1){
        createvar(t, id);
        if(sym->type!=COMMA)break;
        nextSym();
        if(sym->type!=ID){
            msg(ERR, "unexpected \',\' or missing a identifier", line);
			ERROR_STATUS = 1;
        }
        mfree(id);
        id = copySym(sym);
        nextSym();
    }
    mfree(id);
}

void variable(symbol_t t, symbol_t id){
    vardef(t, id);
    if(sym->type!=SEMIC){
        msg(ERR, "missing \';\'", line);
		ERROR_STATUS = 1;
    }
    nextSym();
}

void createconst(symbol_t t, symbol_t id, symbol_t val){
    createid(CONSTANT, translate(t->type), (char*)(id->value), val);
}

void constdef(){
    symbol_t t, id;
    if(!isType(sym)){
        msg(ERR, "missing a type name", line);
		ERROR_STATUS = 1;
    }
    t = copySym(sym);
    do{
        nextSym();
        if(sym->type!=ID){
            msg(ERR, "missing a identifier after a type name", line);
			ERROR_STATUS = 1;
        }
        id = copySym(sym);
        nextSym();
        if(sym->type!=ASN){
            msg(ERR, "constant need a value", line);
			ERROR_STATUS = 1;
        }
        nextSym();
        readLiteral();
        if(!isLiteral(sym)){
            msg(ERR, "missing a literal in constant definition", line);
			ERROR_STATUS = 1;
        }
        createconst(t, id, sym);
        mfree(t); mfree(id);
        nextSym();
    }while(sym->type==COMMA);
}

void constant(){
    while(1){
        constdef();
        if(sym->type!=SEMIC){
            msg(ERR, "missing \';\'", line);
			ERROR_STATUS = 1;
        }
        nextSym();
        if(sym->type!=CONST)
            break;
        nextSym();
    }
}

void body(){
    symbol_t t, id;
    if(sym->type==CONST){
        nextSym();
        constant();
    }
    while(isType(sym)){
        t = copySym(sym);
        nextSym();
        if(sym->type!=ID){
            msg(ERR, "missing a identifier after a type name", line);
			ERROR_STATUS = 1;
        }
        id = copySym(sym);
        nextSym();
        variable(t, id);
        mfree(t);
    }
    statementlist();
}

void funcdef(symbol_t t, symbol_t id){
    symbol_t pt, pid;
    ident_t np;
    func_t f;

    if(findTable(global, (char*)(id->value))){
        msg(ERR, "identifier redefinition", line);
		ERROR_STATUS = 1;
		do
			id->value = (int)ccstrcat((char*)(id->value), '@');
		while(findTable(global, (char*)(id->value)));
    }

    f = createFunc();
    f->local = createTable();
    np = createid(FUNC, translate(t->type), (char*)(id->value), 0);
    if(!np){
        msg(ERR, "identifier redefinition", line);
		ERROR_STATUS = 1;
		exit(0);
    }
    np->extra = (void*)f;
    context = f;

    if(sym->type!=RPAREN){
        while(1){
            if(!isType(sym)){
                msg(ERR, "missing a type name for param", line);
            }
            pt = copySym(sym);
            nextSym();
            if(sym->type!=ID){
                msg(ERR, "missing a identifier after a type name", line);
            }
            pid = copySym(sym);
            if((np=createvar(pt, pid))==0){
                msg(ERR, "identifier redefinition", line);
            }
            addparam(f, np);
			np->loc = LOC_PARAM;
            mfree(pt);mfree(pid);
            nextSym();
            if(sym->type!=COMMA)break;
            nextSym();
        }
    }
    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
    }
    nextSym();
    if(sym->type!=LBRACE){
        msg(ERR, "missing \'{\'", line);
    }
    nextSym();
    body();
	if(t->type==VOID)
		gen(RET, 0, 0, 0);
    if(sym->type!=RBRACE){
        msg(ERR, "missing \'}\'", line);
    }
    context = 0;
    nextSym();
}

void mainfunc(symbol_t t){
    func_t f;
    ident_t id;
    if(t->type!=VOID){
        msg(ERR, "wrong type for main()", line);
		t->type = VOID;
    }
    if(sym->type!=LPAREN){
        msg(ERR, "missing \'(\'", line);
    }else
		nextSym();

    if(sym->type!=RPAREN){
        msg(ERR, "missing \')\'", line);
		ERROR_STATUS = 1;
		do
			nextSym();
		while(sym->type != RPAREN && sym->type != LBRACE && sym->type != SEOF);
		if(sym->type == SEOF)return;
    }
    f = createFunc();
    f->local = createTable();
    id = createid(FUNC, TVOID, "main", 0);
    if(!id){
        msg(DEBUG, "failed defining main()", line);
		exit(0);
    }
    id->extra = (void*)f;
    context = f;
    if(sym->type == RPAREN)nextSym();
    if(sym->type!=LBRACE){
        msg(ERR, "missing \'{\'", line);
		ERROR_STATUS = 1;
    }else
		nextSym();

    body();

    gen(RET, 0, 0, 0);

    if(sym->type!=RBRACE){
        msg(ERR, "missing \'}\'", line);
		ERROR_STATUS = 1;
    }else
		nextSym();

    context = 0;
    
}

void program(){
    symbol_t t = 0, id = 0;
    nextSym();
    if(sym->type == CONST){
        nextSym();
        constant();
    }
    if(isType(sym)){
        do{
            t = copySym(sym);
            nextSym();
            if(sym->type==MAIN){
                nextSym();
                mainfunc(t);
                mfree(t);
                break;
            }
            if(sym->type != ID){
                msg(ERR, "missing a identifier after a type name", line);
				ERROR_STATUS = 1;

				do
					nextSym();
				while(sym->type != ID && sym->type != SEMIC && sym->type != SEOF);

				if(sym->type == SEOF) break;
				if(sym->type == SEMIC){
					nextSym();
					continue;
				}
            }
            id = copySym(sym);
            nextSym();
            if(sym->type == LPAREN)break;
            variable(t, id);
            mfree(t);
        }while(isType(sym) || sym->type == VOID);

    }else if(sym->type==VOID){
        t = copySym(sym);
        nextSym();
        if(sym->type==MAIN){
            nextSym();
            mainfunc(t);
            mfree(t);
        }else{
            if(sym->type != ID){
                msg(ERR, "missing a identifier after a type name", line);
				ERROR_STATUS = 1;
				do
					nextSym();
				while(sym->type != ID && sym->type !=SEOF);
				
			}else{
				id = copySym(sym);
				nextSym();
				if(sym->type!=LPAREN){
					msg(ERR, "missing \'(\'", line);
					ERROR_STATUS = 1;
					// something
				}
			}
        }
    }
    while(sym->type==LPAREN){
        nextSym();
        funcdef(t, id);
        mfree(t);mfree(id);
        if(!(isType(sym)||sym->type==VOID))break;
        t = copySym(sym);
        nextSym();
        if(sym->type==MAIN){
			nextSym();
            mainfunc(t);
            mfree(t);
            break;
        }
        if(sym->type!=ID){
            msg(ERR, "missing a identifier after a type name", line);
			ERROR_STATUS = 1;
			do
				nextSym();
			while(sym->type != ID && sym->type !=SEOF);
			if(sym->type == SEOF) break;
        }
        id = copySym(sym);
        nextSym();
    }

	if(sym->type!=SEOF){
		ERROR_STATUS = 1;
        msg(MSG, "Error occured!", 0);
	}
}

//void debuginfo();

void parser_start(){
    program();
    /*printf("debug:\nsymbol table:\n");
    debuginfo(global, 0);
    printf("debug:done\n");*/
}

/*
void debuginfo(symtab_t tab, int t){
    int i;
    symtab_t p = tab->next;
    ident_t id;
    while(p){
        for(i=0;i<t;i++)printf("\t");
        printf("%s\t%d\n", p->name, p->entry->last);
        id = p->entry;
        if(id->type==FUNC){
            printf("\tfunc %s:\n", id->name);
            debuginfo(((func_t)(id->extra))->local, t+1);
            printf("\tcodes:\n");
            printQCode(((func_t)(id->extra))->codes, t+1);
        }
        p = p->next;
    }
}
*/
