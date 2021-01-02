/* Copyright (C) 2011 - 2015 The uOFW team
   See the file COPYING for copying permission.
   */

#ifndef MODULEMGR_OPTIONS_H
#define	MODULEMGR_OPTIONS_H

#include "common_imp.h"
#include "sysmem_user.h"

/* SceKernelLMOption.position */
#define SCE_KERNEL_LM_POS_LOW           (SCE_KERNEL_SMEM_Low)  /** Place module at lowest possible address. */
#define SCE_KERNEL_LM_POS_HIGH          (SCE_KERNEL_SMEM_High) /** Place module at highest possible address. */
#define SCE_KERNEL_LM_POS_ADDR          (SCE_KERNEL_SMEM_Addr)

/* SceKernelLMOption.access */
#define SCE_KERNEL_LM_ACCESS_NOSEEK     (1)

/** This structure specifies options for loading a module (via LoadModule()). */
typedef struct {
    /** The size of this structure. size = sizeof(SceKernelLMOption). */
    SceSize size;
    /** The memory partition where the program of the module will be stored. */
    SceUID  mpIdText;
    /** The memory partition where the data of the module will be stored. */
    SceUID  mpIdData;
    /** Unused for now. */
    u32     flags;
    /** Specify module placement policy in memory. */
    u8      position;
    /** Unused for now. */
    s8      access;
    /** Reserved. */
    s8      creserved[2];
} SceKernelLMOption;

/** This structure specifies options for starting/stopping a module. */
typedef struct {
    /** The size of the structure. size = sizeof(SceKernelSMOption). */
    SceSize size;
    /**
     * Partition of the stack of the running thread. If 0 is specified then the stack is allocated
     * in the same partition as the data segment of the module.
     */
    SceUID  mpIdStack;
    /** Stack size. If 0 is specified then the default system value is used. */
    SceSize stackSize;
    /** Priority of the running thread. */
    s32     priority;
    /** Attribute of the running thread. */
    u32     attribute;
} SceKernelSMOption;

#endif	/* MODULEMGR_OPTIONS_H */

