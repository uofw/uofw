/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

int sceKernelRegisterPriorityExceptionHandler(int exno, int prio, void (*func)());
int sceKernelRegisterDefaultExceptionHandler(void *func);
int sceKernelReleaseExceptionHandler(int exno, void (*func)());
int sceKernelReleaseDefaultExceptionHandler(void (*func)());
int sceKernelRegisterExceptionHandler(int exno, void (*func)());

