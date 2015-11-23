
#ifndef _MYCC_REPORTER_
#define _MYCC_REPORTER_

typedef enum msglev_e{
    MSG,
    WARN,
    ERR,
    DEBUG
} msglev_t;

extern void setMsgLevel(msglev_t lev);
extern void msg(msglev_t lev, char *msg, int line);

#endif
