/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#include "common.h"

// interruption
#define EXCEP_INT 1
// load or instruction fetch exception
#define EXCEP_ADEL 4
// address store exception
#define EXCEP_ADES 5
// instruction fetch bus error
#define EXCEP_IBE 6
// load/store bus error
#define EXCEP_DBE 7
// syscall
#define EXCEP_SYS 8
// breakpoint
#define EXCEP_BP 9
// reserved instruction
#define EXCEP_RI 10
// coprocessor unusable
#define EXCEP_CPU 11
// arithmetic overflow
#define EXCEP_OV 12
// floating-point exception
#define EXCEP_FPE 15
// watch (reference to WatchHi/WatchLo)
#define EXCEP_WATCH 23
// "Virtual Coherency Exception data" (used for NMI handling apparently)
#define EXCEP_VCED 31

int sceKernelRegisterPriorityExceptionHandler(int exno, int prio, void (*func)());
int sceKernelRegisterDefaultExceptionHandler(void *func);
int sceKernelReleaseExceptionHandler(int exno, void (*func)());
int sceKernelReleaseDefaultExceptionHandler(void (*func)());
int sceKernelRegisterExceptionHandler(int exno, void (*func)());

