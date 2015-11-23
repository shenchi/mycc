#include "symmgr.h"

#ifndef _MYCC_REG_ALLOCATOR_
#define _MYCC_REG_ALLOCATOR_

#define	R_EAX (1)
#define R_EBX (1<<1)
#define R_ECX (1<<2)
#define R_EDX (1<<3)
#define R_ESI (1<<4)
#define R_EDI (1<<5)
// #define R_EBP (1<<6)
#define R_END (1<<6)

// temp reg includs EAX, ECX, EDX
//#define TEMP_REG 0x0D
#define TEMP_REG 0x0F

// var reg includes EBX, ESI, EDI
#define GLOBAL_REG 0x32

#define rstate(p) ((p) & reg_state)
#define rlock(p) reg_state &= ~(p)
#define runlock(p) reg_state |= (p)


typedef struct store_s{
    int wanted_reg;
    int cur_reg;
    int addr;
} *store_t;

extern int reg_state;

extern void rfree(int reg);

extern int ralloc(ident_t id, int regmask);

extern void rprotect();

extern void rreturn();

extern void rclear();


#endif
