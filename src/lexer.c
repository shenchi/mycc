#include <stdio.h>
#include "memmgr.h"
#include "symbol.h"
#include "string.h"
#include "reporter.h"

FILE *fp;
char ch;
int lineno = 0;

// keep same order with symbol.h
char keywords[][20] = {
		"const",
		"int",
		"float",
		"char",
		"void",
		"if",
		"while",
		"switch",
		"case",
		"default",
		"main",
		"scanf",
		"printf",
		"return",
		""
};

void lexer_load(char *filename){
    fp = fopen(filename, "r");
    ch = 0;
    lineno = 0;
}

void lexer_free(){
    if(fp)fclose(fp);
}

void nextchar(){
	ch = fgetc(fp);
	if(ch=='\n'){
	    lineno++;
	}
}

int getLine(){
    return lineno+1;
}

int isBlank(){
	return (ch == 0 || ch==' ' || ch=='\n' || ch=='\t' || ch=='\r');
}

int isAlpha(){
	return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || ch=='_');
}

int isDigital(){
	return (ch>='0' && ch<='9');
}

symbol_t readNumLit(int sign){
    char *s;
    int i = 0;
		//float j = 0;
	s = ccstring();
	if(ch=='0'){
		i = 0;
		if(sign) s = ccstrcat(s, '-');
		s = ccstrcat(s, '0');
		nextchar();
	}else{
		while(isDigital()){
			i = i*10 + (ch-'0');
			s = ccstrcat(s, ch);
			nextchar();
		}
	}
	if(ch=='.'){
		nextchar();
		s = ccstrcat(s, '.');
		if(isDigital()){
			while(isDigital()){
				//j = (j + (float)(ch-'0')) / (float)10.0;
				s = ccstrcat(s, ch);
				nextchar();
			}
			//return Symbol(REAL, (int)((float)i + j));
			return Symbol(REAL, (int)s);
		}else{
               msg(ERR, "a wrong real literal",lineno+1);
		}
	}else{
	    mfree(s);
	    if(sign) i = -i;
		return Symbol(INTEGER, i);
	}
	return 0;
}

symbol_t readSym(){
    char *s, tch;
    symtype_t i;

	while(isBlank())nextchar();
	if(isAlpha()){
		s = ccstring();
		s = ccstrcat(s,ch);

		nextchar();
		while(isAlpha() || isDigital()){
			s = ccstrcat(s,ch);
			nextchar();
		}
		for(i = CONST; i<=RETURN; i ++)
			if(ccstrcmp(s,keywords[i-1])==0){
				return Symbol(i, 0);
			}
		return Symbol(ID, (int)s);

	}else if(isDigital()){
        return readNumLit(0);
	}else if(ch=='\''){
		nextchar();
		if(isAlpha() || isDigital() || ch=='+' || ch=='-' || ch=='*' || ch=='/'){
			tch = ch;
			nextchar();
			if(ch=='\''){
				nextchar();
				return Symbol(CHARLIT, (int)tch);
			}
		}
		msg(ERR, "a wrong char literal",lineno+1);
	}else if(ch=='\"'){
		nextchar();
		s = ccstring();
		while(ch!='\"' && ch!=EOF){
			s = ccstrcat(s, ch);
			nextchar();
		}
		if(ch!=EOF){
			nextchar();
			return Symbol(STRLIT, (int)s);
		}
		msg(ERR, "a wrong string literal",lineno+1);

	}else if(ch=='+'){
		nextchar();
		//if(isDigital())
        //    return readNumLit(0);
        //else
            return Symbol(PLUS, 0);
	}else if(ch=='-'){
		nextchar();
		//if(isDigital())
        //    return readNumLit(1);
        //else
            return Symbol(MINUS, 0);
	}else if(ch=='*'){
		nextchar();
		return Symbol(MULTI, 0);
	}else if(ch=='/'){
		nextchar();
		return Symbol(DIVIDE, 0);
	}else if(ch=='<'){
		nextchar();
		if(ch=='='){
			nextchar();
			return Symbol(LEQU, 0);
		}else
			return Symbol(LESS, 0);
	}else if(ch=='>'){
		nextchar();
		if(ch=='='){
			nextchar();
			return Symbol(GEQU, 0);
		}else
			return Symbol(GREATER, 0);
	}else if(ch=='!'){
		nextchar();
		if(ch=='='){
			nextchar();
			return Symbol(UNEQU, 0);
		}
		msg(ERR, "unexpected char \'!\'",lineno+1);
	}else if(ch=='='){
		nextchar();
		if(ch=='='){
			nextchar();
			return Symbol(EQU, 0);
		}else
			return Symbol(ASN, 0);
	}else if(ch=='('){
		nextchar();
		return Symbol(LPAREN, 0);
	}else if(ch==')'){
		nextchar();
		return Symbol(RPAREN, 0);
	}else if(ch=='{'){
		nextchar();
		return Symbol(LBRACE, 0);
	}else if(ch=='}'){
		nextchar();
		return Symbol(RBRACE, 0);
	}else if(ch==';'){
		nextchar();
		return Symbol(SEMIC, 0);
	}else if(ch==':'){
		nextchar();
		return Symbol(COLON, 0);
	}else if(ch==','){
		nextchar();
		return Symbol(COMMA, 0);
	}else if(ch==EOF){
		return Symbol(SEOF, 0);
	}

	msg(ERR, "unknown lexer error",lineno+1);
	return 0;
}
