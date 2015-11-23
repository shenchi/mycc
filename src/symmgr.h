
#ifndef _MYCC_SYMBOL_MANAGER_
#define _MYCC_SYMBOL_MANAGER_

typedef enum idtype_e{
    CONSTANT,
    VAR,
    LIT,
    FUNC,
    TEMP,
    LABEL
} idtype_t;

// when type is const, var, lit, temp
typedef enum vartype_e{
    TCHAR,
    TINT,
    TFLOAT,
    TVOID,
    TSTR
} vartype_t;

#define LOC_GLOBAL 0
#define LOC_LOCAL  1
#define LOC_PARAM  2

typedef struct ident_s{
    char        *name; // handled by this
    idtype_t    type;
    vartype_t   subtype;
    int         data;   // value for const and literal
                        // or register allocation for var
    void        *extra;
	int			loc;
	int			last;	// last use
} *ident_t;

typedef struct symtab_s * symtab_t;
// first node of link is a head
// store the name of the table
// and just next been used
struct symtab_s{
    char        *name;
    ident_t     entry;
    symtab_t    next, prev;
};

extern symtab_t global;

extern ident_t createIdent();
extern symtab_t createTable();

extern int insertTable(symtab_t tab, ident_t id);
extern ident_t findTable(symtab_t tab, char* name);
extern int removeTable(symtab_t tab, char* name);

#endif
