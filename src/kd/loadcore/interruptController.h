/* Copyright (C) 2011, 2012 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef INTERRUPTCONTROLLER_H
#define	INTERRUPTCONTROLLER_H

s32 loadCoreCpuSuspendIntr(void);
void loadCoreCpuResumeIntr(s32 intr);
void sub_00003D84(void);

#endif	/* INTERRUPTCONTROLLER_H */

