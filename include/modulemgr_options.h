/* Copyright (C) 2011, 2012, 2013 The uOFW team
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

typedef struct {
    /** The size of the structure. size = sizeof(SceKernelLMOption). */
	SceSize	size;
	SceUID	mpIdText;  /* partition text */
	SceUID	mpIdData;  /* partition data */
	u32     flags;
	s8      position;  /* module allocation type */
	s8      access;    /* file access type */
	s8      creserved[2];
} SceKernelLMOption;

typedef struct {
	SceSize size;
	SceUID  mpidstack;	/* partition stack */
	SceSize stacksize;
	s32		priority;
	u32     attribute;	/* thread attribute */
} SceKernelSMOption;

#endif	/* MODULEMGR_OPTIONS_H */

