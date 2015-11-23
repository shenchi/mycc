#include "memmgr.h"

char* ccstring(){
    char *newstr = (char*)mmalloc(1);
    *newstr = 0;
    return newstr;
}

int ccstrlen(char *str){
    int size = 0;
    while(*(str++)!=0)size++;
    return size;
}

char* ccstrcat(char *str, char a){
    int size = ccstrlen(str), i;
    char *newstr = (char*)mmalloc(size+2);
    for(i=0; i<size; i++)
        newstr[i] = str[i];
    newstr[i++]=a;
    newstr[i]=0;
    mfree(str);
    return newstr;
}

char *ccstrcpy(char *src){
    int size = ccstrlen(src), i;
    char *newstr = (char*)mmalloc(size+1);
    for(i = 0; i < size; i++)
        newstr[i] = src[i];
    newstr[i] = 0;
    return newstr;
}

int ccstrcmp(char *a, char *b){
    int r;
    do{
        r = *a - *b;
        if(r!=0)break;
    }while(*(a++)!=0 && *(b++)!=0);
    return r;
}

void ccstrinv(char *str){
    int i, len = ccstrlen(str);
    char t;
    for(i=0; i<len/2; i++){
        t = str[i];
        str[i] = str[len-1-i];
        str[len-1-i] = t;
    }
}

char* ccgenname(int n, char header){
    char *ret = ccstring();
    int i;
    do{
        i = n % 10;
        n = n / 10;
        ret = ccstrcat(ret, (char)(i+'0'));
    }while(n);
    ret = ccstrcat(ret, header);
    ccstrinv(ret);
    return ret;
}
