#include "symbol.h"

#ifndef _MYCC_LEXER_
#define _MYCC_LEXER_

extern void lexer_load(char *filename);
extern void lexer_free();

extern int getLine();
extern symbol_t readSym();

#endif
