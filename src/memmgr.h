
#ifndef _MYCC_MEMORY_MANAGER_
#define _MYCC_MEMORY_MANAGER_

extern void mem_init();
extern void mem_free();

extern void* mmalloc(int size);
extern void mfree(void* ptr);

#endif
