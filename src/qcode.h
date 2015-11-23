#include "symmgr.h"
#ifndef _MYCC_QUADCODE_
#define _MYCC_QUADCODE_

typedef enum opcode_e{
	NOP = 0,
	MOV,
	ADD,
	SUB,
	MUL,
	DIV,
	CMP,
	NEG,
	FADD,
	FSUB,
	FMUL,
	FDIV,
	FCMP,
	FNEG,
	JGT,
	JGE,
	JLT,
	JLE,
	JEQ,
	JNE,
	JMP,
	PARAM,
	CALL,
	RET,
	IN,
	OUT
} opcode_t;

typedef struct qcode_s * qcode_t;
struct qcode_s{
	int			index;
    opcode_t    code;
    ident_t     op1, op2, opr;
    qcode_t     next;
};

typedef struct param_s * param_t;
struct param_s{
    ident_t id;
    param_t next;
};

typedef struct func_s{
    symtab_t    local;
    param_t     params;
    int         nparam;
    qcode_t     codes;
} *func_t;

extern qcode_t createQCode();
extern func_t createFunc();

extern int insertQCode(func_t f, qcode_t c);
extern int addparam(func_t f, ident_t id);
extern void printQCode(qcode_t c, int t);

#endif
