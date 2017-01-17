#ifndef _BOOT_H_
#define _BOOT_H_

#include "defs.h"
#include <intrin.h>

//
// exported functions from __init.asm
//
void __cli(void);
void __sti(void);
void __magic(void);         // MAGIC breakpoint into BOCHS (XCHG BX,BX)

#endif // _BOOT_H_