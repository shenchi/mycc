
#ifndef _MYCC_STRING_
#define _MYCC_STRING_

extern char* ccstring();

extern int ccstrlen(char *str);

extern char* ccstrcat(char *str, char a);

extern char *ccstrcpy(char *src);

extern int ccstrcmp(char *a, char *b);

extern void ccstrinv(char *str);

extern char* ccgenname(int n, char header);
#endif
