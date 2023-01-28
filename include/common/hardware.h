/* Copyright (C) 2011, 2012, 2013 The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef COMMON_INCLUDED
# error "Only include common_imp.h or common_header.h!"
#endif

#define HW(addr) (*(vs32 *)(addr))
#define HWPTR(addr) ((vs32 *)(addr))

#define RAM_TYPE_32_MB          (1)
#define RAM_TYPE_64_MB          (2)

#define HW_SYS_NMI_ENABLE_MASK  0xBC100000

#define HW_SYS_PLL_FREQ         0xBC100068

#define HW_SYS_IO_ENABLE        0xBC100078
#define HW_SYS_IO_ENABLE_AUDIOCLKOUT 0x800

#define HW_SYS_GPIO_ENABLE      0xBC10007C

// Pin used for Jigkick
#define HW_SYS_GPIO_ENABLE_PIN4 0x10

#define HW_RAM_SIZE             0xBC100040

#define HW_TIMER_0              0xBC500000
#define HW_TIMER_1              0xBC500010
#define HW_TIMER_2              0xBC500020
#define HW_TIMER_3              0xBC500030

#define HW_NAND_CONTROL 0xBD101000
#define HW_NAND_STATUS  0xBD101004
#define HW_NAND_COMMAND 0xBD101008
#define HW_NAND_COMMAND_RESET 0xFF
#define HW_NAND_ADDRESS 0xBD10100C
#define HW_NAND_RESET   0xBD101014
#define HW_NAND_DMA_ADDRESS 0xBD101020
#define HW_NAND_DMA_CONTROL 0xBD101024
#define HW_NAND_DMA_STATUS  0xBD101028

#define HW_NAND_DMA_BUFFER  0xBFF00000
#define HW_NAND_DMA_ECC     0xBFF00800
#define HW_NAND_DMA_SPARE0  0xBFF00900
#define HW_NAND_DMA_SPARE1  0xBFF00904
#define HW_NAND_DMA_SPARE2  0xBFF00908

#define HW_MEMORYSTICK_START_TPC 0xBD200030
#define HW_MEMORYSTICK_TPC       0xBD200034
#define HW_MEMORYSTICK_STATUS   0xBD200038
#define HW_MEMORYSTICK_STATUS_TIMEOUT   0x100
#define HW_MEMORYSTICK_STATUS_CRC_ERROR 0x200
#define HW_MEMORYSTICK_STATUS_READY     0x1000
#define HW_MEMORYSTICK_STATUS_UNK13     0x2000
#define HW_MEMORYSTICK_STATUS_FIFO_RW   0x4000
#define HW_MEMORYSTICK_SYS      0xBD20003C
#define HW_MEMORYSTICK_SYS_RESET 0x8000

#define HW_KIRK_SIGNATURE       0xBDE00000
#define HW_KIRK_VERSION         0xBDE00004
#define HW_KIRK_ERROR           0xBDE00008
#define HW_KIRK_START           0xBDE0000C
#define HW_KIRK_START_PHASE1             1
#define HW_KIRK_START_PHASE2             2
#define HW_KIRK_COMMAND         0xBDE00010
#define HW_KIRK_COMMAND_PRIV_DEC        0x01
#define HW_KIRK_COMMAND_ENC_2           0x02
#define HW_KIRK_COMMAND_DEC_2           0x03
#define HW_KIRK_COMMAND_ENC_3_IV_ZERO   0x04
#define HW_KIRK_COMMAND_ENC_3_IV_FUSEID 0x05
#define HW_KIRK_COMMAND_ENC_3_IV_USER   0x06
#define HW_KIRK_COMMAND_DEC_3_IV_ZERO   0x07
#define HW_KIRK_COMMAND_DEC_3_IV_FUSEID 0x08
#define HW_KIRK_COMMAND_DEC_3_IV_USER   0x09
#define HW_KIRK_COMMAND_PRIV_SIGNCHECK  0x0A
#define HW_KIRK_COMMAND_SHA1            0x0B
#define HW_KIRK_COMMAND_ECDSA_KEYGEN    0x0C
#define HW_KIRK_COMMAND_ECDSA_MULT      0x0D
#define HW_KIRK_COMMAND_PRNG            0x0E
#define HW_KIRK_COMMAND_PRNG_SEED       0x0F
#define HW_KIRK_COMMAND_ECDSA_SIGN      0x10
#define HW_KIRK_COMMAND_ECDSA_SIGNCHECK 0x11

#define HW_KIRK_COMMAND_RESULT  0xBDE00014
#define HW_KIRK_STATUS          0xBDE0001C
#define HW_KIRK_STATUS_PHASE1_FINISH 0x01
#define HW_KIRK_STATUS_PHASE2_FINISH 0x02
#define HW_KIRK_STATUS_NEEDPHASE2    0x10


#define HW_KIRK_COMMAND_END     0xBDE00028
#define HW_KIRK_SRC_BUF         0xBDE0002C
#define HW_KIRK_DST_BUF         0xBDE00030

#define HW_GPIO_READ            0xBE240004
#define HW_GPIO_READ_PIN4       0x00000010

#define HW_RESET_VECTOR         0xBFC00000
#define HW_RESET_VECTOR_SIZE    (0x1000)

/*
 * GE hardware registers
 */

/*
 * Main GE stuff
 */
// RW bit 1: set to 1 to reset, wait until bit is 0 to know the GE has been reset
#define HW_GE_RESET              HW(0xBD400000)
// Unknown (possibly read-only?); accessible through sceGeSet/GetReg() (SCE_GE_REG_UNK004),
// saved on suspend and passed in the interrupt handling functions but never used
#define HW_GE_UNK004             HW(0xBD400004)
// RO bits 0x0000FFFF: shifted left by 10, gives the EDRAM hardware size
// (sceGeEdramGetHwSize()) (only used for tachyon < 0x00500000)
#define HW_GE_EDRAM_HW_SIZE      HW(0xBD400008)

/*
 * GE execution/display list handling
 */
// RW bit 0x001: 0 = stopped, 1 = running
// R  bit 0x002: 0 = branching condition true, 1 = false
// R  bit 0x100: 1 = is at depth 1 (or 2) of calls
// R  bit 0x200: 1 = is at depth 2 of calls
#define HW_GE_EXEC               HW(0xBD400100)
#define HW_GE_EXEC_RUNNING       0x001
#define HW_GE_EXEC_BRANCHING     0x002
#define HW_GE_EXEC_DEPTH1        0x100
#define HW_GE_EXEC_DEPTH2        0x200
// Never used, accessible through sceGeSet/GetReg() (SCE_GE_REG_UNK104)
#define HW_GE_UNK104             HW(0xBD400104)
// RW: address of the display list currently being run (not sure if it's the current point of
// execution or the starting address)
#define HW_GE_LISTADDR           HW(0xBD400108)
// RW: stall address of the display list (0 = no stall address)
#define HW_GE_STALLADDR          HW(0xBD40010C)
// RW: first return address (after the first CALL command)
#define HW_GE_RADR1              HW(0xBD400110)
// RW: second return address (after the second CALL command)
#define HW_GE_RADR2              HW(0xBD400114)
// RW: address of vertices (for bezier etc)
#define HW_GE_VADR               HW(0xBD400118)
// RW: address of indices (for bezier etc)
#define HW_GE_IADR               HW(0xBD40011C)
// RW: address of the origin (set by ORIGIN, destination address for JUMP/BJUMP/CALL after adding BASE and the address specified in the command)
#define HW_GE_OADR               HW(0xBD400120)
// RW: same, for the first call
#define HW_GE_OADR1              HW(0xBD400124)
// RW: same, for the second call
#define HW_GE_OADR2              HW(0xBD400128)

/*
 * GE geometry clock
 */
// RW, bit 1 set by sceGeSetGeometryClock(), exact usage unknown
#define HW_GE_GEOMETRY_CLOCK     HW(0xBD400200)

// Never used, accessible through sceGeSet/GetReg() (SCE_GE_REG_UNK300)
#define HW_GE_UNK300             HW(0xBD400300)

// RO memory for the commands
// Each type a command is executed by the GE, its value, including arguments,
// is saved there.
#define HW_GE_CMD(i)    HW(0xBD400800 + i * 4)
// RO registers for the different matrices
#define HW_GE_BONES     ((vs32*)HWPTR(0xBD400C00))
#define HW_GE_BONE(i)   ((vs32*)HWPTR(0xBD400C00 + i * 48))
#define HW_GE_WORLDS    ((vs32*)HWPTR(0xBD400D80))
#define HW_GE_VIEWS     ((vs32*)HWPTR(0xBD400DB0))
#define HW_GE_PROJS     ((vs32*)HWPTR(0xBD400DE0))
#define HW_GE_TGENS     ((vs32*)HWPTR(0xBD400E20))


// Interrupt statuses
// Triggered by the SIGNAL command
#define HW_GE_INTSIG 1
// Triggered by the END command
#define HW_GE_INTEND 2
// Triggered by the FINISH command
#define HW_GE_INTFIN 4
// Triggered by an error (?)
#define HW_GE_INTERR 8
// RO: current interrupt status?
#define HW_GE_INTERRUPT_TYPE1    HW(0xBD400304)
// RW: currently accepted interrupts?
// Set to HW_GE_INTSIG | HW_GE_INTEND | HW_GE_INTFIN on init & reset
#define HW_GE_INTERRUPT_TYPE2    HW(0xBD400308)
// WO: set to HW_GE_INTERRUPT_TYPE2 on init & reset
#define HW_GE_INTERRUPT_TYPE3    HW(0xBD40030C)
// WO: set current interrupt status? set to HW_GE_INTERRUPT_TYPE1 on init & reset
#define HW_GE_INTERRUPT_TYPE4    HW(0xBD400310)

// RW: set to 4 when the used edram size is 0x00200000 and 2 when it's 0x00400000 (!)
#define HW_GE_EDRAM_ENABLED_SIZE HW(0xBD400400)
// RW: unknown, bits 0x00F00000 set by sceGeEdramSetRefreshParam's 4th argument
#define HW_GE_EDRAM_REFRESH_UNK1 HW(0xBD500000)
// RW: set to 2 before reset and 0 after reset is done, bit 1 seems to be initialization
// (used by sceGeEdramInit())
#define HW_GE_EDRAM_UNK10        HW(0xBD500010)
// RW, set to 0x6C4 by default and bits 0x007FFFFF set by sceGeEdramSetRefreshParam's second argument
#define HW_GE_EDRAM_REFRESH_UNK2 HW(0xBD500020)
// RW, bits 0x000003FF set by sceGeEdramSetRefreshParam's third argument
#define HW_GE_EDRAM_REFRESH_UNK3 HW(0xBD500030)
// RW, set to 1 in sceGeEdramInit(), and to 3 if sceGeEdramSetRefreshParam's first argument (mode) is 1
// and the bit 2 isn't set
#define HW_GE_EDRAM_UNK40        HW(0xBD500040)
// Unknown, accessible through sceGeSetReg/GetReg() (SCE_GE_REG_EDRAM_UNK50)
#define HW_GE_EDRAM_UNK50        HW(0xBD500050)
// Unknown, accessible through sceGeSetReg/GetReg() (SCE_GE_REG_EDRAM_UNK60)
#define HW_GE_EDRAM_UNK60        HW(0xBD500060)
// RW bit 1: disable address translation
#define HW_GE_EDRAM_ADDR_TRANS_DISABLE HW(0xBD500070)
// RW: the address translation value
#define HW_GE_EDRAM_ADDR_TRANS_VALUE   HW(0xBD500080)
// Unknown, set to 3 in sceGeEdramInit(), accessible through sceGeSetReg/GetReg() (SCE_GE_REG_EDRAM_UNK90)
#define HW_GE_EDRAM_UNK90        HW(0xBD500090)
// Unknown, accessible through sceGeSetReg/GetReg() (SCE_GE_REG_EDRAM_UNKA0)
#define HW_GE_EDRAM_UNKA0        HW(0xBD5000A0)
