
#ifndef _MYCC_SYMBOL_
#define _MYCC_SYMBOL_

typedef enum symtype_e{
	// identifier
	ID = 0,

	//	keywords
	CONST,
	INT,
	FLOAT,
	CHAR,
	VOID,
	IF,
	WHILE,
	SWITCH,
	CASE,
	DEFUALT,
	MAIN,
	SCANF,
	PRINTF,
	RETURN,

	// literal
	INTEGER,
	REAL,
	CHARLIT,
	STRLIT,

	// operator
	PLUS,
	MINUS,
	MULTI,
	DIVIDE,
	LESS,
	LEQU,
	GREATER,
	GEQU,
	UNEQU,
	EQU,
	ASN,

	// others
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,
	SEMIC,
	COLON,
	COMMA,
	SEOF
} symtype_t;

struct symbol_s{
    symtype_t   type;
    int         value;
};
typedef struct symbol_s * symbol_t;

extern symbol_t Symbol(symtype_t type, int value);

extern int isID(symbol_t sym);
extern int isKeyword(symbol_t sym);
extern int isLiteral(symbol_t sym);
extern int isIntLiteral(symbol_t sym);
extern int isRealLiteral(symbol_t sym);
extern int isCharLiteral(symbol_t sym);
extern int isStringLiteral(symbol_t sym);
extern int isOperator(symbol_t sym);
extern int isAddOperator(symbol_t sym);
extern int isMulOperator(symbol_t sym);
extern int isEqualOperator(symbol_t sym);
extern int isAssignOperator(symbol_t sym);

#endif
