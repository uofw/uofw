/* Copyright (C) 2011, 2012, 2013, 2014 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef MODULEMGR_OPTIONS_H
#define	MODULEMGR_OPTIONS_H

#include "common_imp.h"

/* SceKernelLMOption.position */
#define SCE_KERNEL_LM_POS_LOW           (SCE_KERNEL_SMEM_Low)
#define SCE_KERNEL_LM_POS_HIGH          (SCE_KERNEL_SMEM_High)
#define SCE_KERNEL_LM_POS_ADDR          (SCE_KERNEL_SMEM_Addr)

/* SceKernelLMOption.access */
#define SCE_KERNEL_LM_ACCESS_NOSEEK     (1)

/** This structure specifies options for loading a module (via LoadModule()). */
typedef struct {
    /** The size of this structure. size = sizeof(SceKernelLMOption). */
	SceSize	size;
    /** The memory partition where the program of the module will be stored. */
	SceUID	mpIdText; 
    /** The memory partition where the data of the module will be stored. */
	SceUID	mpIdData;
    /** Unused for now. */
	u32     flags;
    /** Specify the position of the module in memory. */
	s8      position; 
    /** Unused for now. */
	s8      access;
    /** Reserved. */
	s8      creserved[2];
} SceKernelLMOption;

typedef struct {
    /** The size of the structure. size = sizeof(SceKernelLMOption). */
	SceSize size;
	SceUID  mpidstack;	/* partition stack */
	SceSize stacksize;
	s32		priority;
	u32     attribute;	/* thread attribute */
} SceKernelSMOption;

#endif	/* MODULEMGR_OPTIONS_H */

