#include <stdio.h>
#include "reporter.h"

msglev_t level = 0;
char msghead[][8] = {
    "MESSAGE",
    "WARNING",
    "ERROR  ",
    "DEBUG  "
};

void setMsgLevel(msglev_t lev){
    level = lev;
}

void msg(msglev_t lev, char *msg, int line){
    if(lev<level)return;
    if(line>0){
        printf("%s : Line%6d: %s\n", msghead[lev], line, msg);
    }else{
        printf("%s : %s\n", msghead[lev], msg);
    }
}
