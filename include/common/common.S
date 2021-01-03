# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

#ifndef COMMON_INCLUDED
# error "Only include common_asm.h!"
#endif

#define COP0_CTRL_EPC           $0
#define COP0_CTRL_STATUS        $2
#define COP0_CTRL_CAUSE         $3
#define COP0_CTRL_V0            $4
#define COP0_CTRL_V1            $5
#define COP0_CTRL_EXC_TABLE     $8
#define COP0_CTRL_NMI_HANDLER   $9
#define COP0_CTRL_SC_TABLE      $12
#define COP0_CTRL_IS_INTERRUPT  $13
#define COP0_CTRL_SP_KERNEL     $14
#define COP0_CTRL_SP_USER       $15
#define COP0_CTRL_TCB           $16
#define COP0_CTRL_NMI_TABLE     $18
#define COP0_CTRL_23            $23
#define COP0_CTRL_PROFILER_BASE $25

#define COP0_STATE_BAD_VADDR    $8  # virtual address of last error/exception
#define COP0_STATE_COUNT        $9  # system counter
#define COP0_STATE_COMPARE      $11 # counter comparison value
#define COP0_STATE_STATUS       $12 # system status
#define COP0_STATE_CAUSE        $13 # exception cause
#define COP0_STATE_EPC          $14 # exception program counter
#define COP0_STATE_PRID         $15 # processor revision id
#define COP0_STATE_CONFIG       $16 # configuration
#define COP0_STATE_SCCODE       $21
#define COP0_STATE_CPUID        $22 # CPU ID (0 = Main, 1 = ME)
#define COP0_STATE_EBASE        $25 # virtual address of exception vector
#define COP0_STATE_TAG_LO       $28 # cache instruction register
#define COP0_STATE_TAG_HI       $29 # cache instruction register
#define COP0_STATE_ERROR_PC     $30 # error exception program counter

#define COP1_CTRL_FCCR          $25 # Floating Point Condition Codes Register
#define COP1_CTRL_FEXR          $26 # Floating Point Exceptions Register
#define COP1_CTRL_FENR          $28 # Floating Point Enables Register
#define COP1_CTRL_FCSR          $31 # Floating Point Control and Status Register

#define INTR_CTRL_STATE         $0 # Interrupt Controller State Register

#define VFPU_PFXS $128
#define VFPU_PFXT $129
#define VFPU_PFXD $130
#define VFPU_CC   $131
#define VFPU_INF4 $132
#define VFPU_REV  $135
#define VFPU_RCX0 $136
#define VFPU_RCX1 $137
#define VFPU_RCX2 $138
#define VFPU_RCX3 $139
#define VFPU_RCX4 $140
#define VFPU_RCX5 $141
#define VFPU_RCX6 $142
#define VFPU_RCX7 $143

#define zr 0

#define halt .word 0x70000000

.set noat
.set noreorder

