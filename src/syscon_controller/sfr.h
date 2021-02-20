/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SFR_H
#define SFR_H

/* Special Function Registers (SFR) */

/* Port registers (P0 - P7 and P12 - P14) */
#define GET_PORT_VAL(i)			*(u8 *)(0xFF00 + (i))
#define SET_PORT_VAL(i, v)		*(u8 *)(0xFF00 + (i)) = (v)

#define GET_ADCR_VAL			*(u8 *)0xFF08 /* 10-bit A/D conversion result register */
#define GET_ADCRH_VAL			*(u8 *)0xFF09 /* 8-bit A/D conversion result register */

#define GET_RXB6_VAL			*(u8 *)0xFF0A /* Receive buffer register 6 */
#define GET_TXB6_VAL			*(u8 *)0xFF0B /* Transmit buffer register 6 */

#define GET_SIO10_VAL			*(u8 *)0xFF0F /* Serial I/O shift register 10 */

#define GET_TM00_VAL			*(u16 *)0xFF10 /* 16-bit timer counter 00 */

/* 16-bit timer capture/compare register 000 */
#define GET_CR000_VAL			*(u16 *)0xFF12
#define SET_CR000_VAL(v)		*(u16 *)0xFF12 = (v)

/* 16-bit timer capture/compare register 010 */
#define GET_CR010_VAL			*(u16 *)0xFF14
#define SET_CR010_VAL(v)		*(u16 *)0xFF14 = (v)

#define GET_TM50_VAL			*(u8 *)0xFF16 /* 8-bit timer counter 50 */

/* 8-bit timer compare register 50 */
#define GET_CR50_VAL			*(u8 *)0xFF17
#define SET_CR50_VAL(v)			*(u8 *)0xFF17 = (v)

/* 8-bit timer H compare register 00 */
#define GET_CMP00_VAL			*(u8 *)0xFF18
#define SET_CMP00_VAL(v)		*(u8 *)0xFF18 = (v)

/* 8-bit timer H compare register 10 */
#define GET_CMP10_VAL			*(u8 *)0xFF19
#define SET_CMP10_VAL(v)		*(u8 *)0xFF19 = (v)

/* 8-bit timer H compare register 01 */
#define GET_CMP01_VAL			*(u8 *)0xFF1A
#define SET_CMP01_VAL(v)		*(u8 *)0xFF1A = (v)

/* 8-bit timer H compare register 11 */
#define GET_CMP11_VAL			*(u8 *)0xFF1B
#define SET_CMP11_VAL(v)		*(u8 *)0xFF1B = (v)

#define GET_TM51_VAL			*(u8 *)0xFF1F /* 8-bit timer counter 51 */

/* Port mode registers (PM0 - PM7, PM12 and PM14) */
#define GET_PORT_MODE_VAL(i)	*(u8 *)(0xFF20 + (i))
#define SET_PORT_MODE_VAL(i, v)	*(u8 *)(0xFF20 + (i)) = (v)

/* A/D converter mode register */
#define GET_ADM_VAL				*(u8 *)0xFF28
#define SET_ADM_VAL(v)			*(u8 *)0xFF28 = (v)				

/* Analog input channel specification register */
#define GET_ADS_VAL				*(u8 *)0xFF29
#define SET_ADS_VAL(v)			*(u8 *)0xFF29 = (v)		

/* A/D port configuration register */
#define GET_ADPC_VAL			*(u8 *)0xFF2F
#define SET_ADPC_VAL(v)			*(u8 *)0xFF2F = (v)		

/* Pull-up resistor option registers (PU0 - PU7, PU12 and PU14) */
#define GET_PU_VAL(i)			*(u8 *)(0xFF30 + (i))
#define SET_PU_VAL(i, v)		*(u8 *)(0xFF30 + (i)) = (v)

/* Clock output selection register */
#define GET_CKS_VAL				*(u8 *)0xFF40
#define SET_CKS_VAL(v)			*(u8 *)0xFF40 = (v)

/* 8-bit timer compare register 51 */
#define GET_CR51_VAL			*(u8 *)0xFF41
#define SET_CR51_VAL(v)			*(u8 *)0xFF41 = (v)

/* 8-bit timer mode control register 51 */
#define GET_TMC51_VAL			*(u8 *)0xFF43
#define SET_TNC51_VAL(v)		*(u8 *)0xFF43 = (v)

/* External interrupt rising edge enable register */
#define GET_EGP_VAL				*(u8 *)0xFF48
#define SET_EGP_VAL(v)			*(u8 *)0xFF48 = (v)

/* External interrupt falling edge enable register */
#define GET_EGN_VAL				*(u8 *)0xFF49
#define SET_EGN_VAL(v)			*(u8 *)0xFF49 = (v)

#define GET_SIO11_VAL			*(u8 *)0xFF4A /* Serial I/O shift register 11 */

/* Transmit buffer register 11 */
#define GET_SOTB11_VAL			*(u8 *)0xFF4C
#define SET_SOTB11_VAL(v)		*(u8 *)0xFF4C = (v)

/* Input switch control register */
#define GET_ISC_VAL				*(u8 *)0xFF4F
#define SET_ISC_VAL(v)			*(u8 *)0xFF4F = (v)

/* Asynchronous serial interface operation mode register 6 */
#define GET_ASIM6_VAL			*(u8 *)0xFF50
#define SET_ASIM6_VAL(v)		*(u8 *)0xFF50 = (v)

/* Asynchronous serial interface reception error status register 6 */
#define GET_ASIS6_VAL			*(u8 *)0xFF53

/* Asynchronous serial interface transmission status register 6 */
#define GET_ASIF6_VAL			*(u8 *)0xFF55

/* Clock selection register 6 */
#define GET_CKSR6_VAL			*(u8 *)0xFF56
#define SET_CKSR6_VAL(v)		*(u8 *)0xFF56 = (v)

/* Baud rate generator control register 6 */
#define GET_BRGC6_VAL			*(u8 *)0xFF57
#define SET_BRGC6_VAL(v)		*(u8 *)0xFF57 = (v)

/* Asynchronous serial interface control register 6 */
#define GET_ASICL6_VAL			*(u8 *)0xFF58
#define SET_ASICL6_VAL(v)		*(u8 *)0xFF58 = (v)

/* Remainder data register 0 */
#define GET_SDR0_VAL			*(u8 *)0xFF60

/* Multiplication/division data register A0 */

/* A0 low */

/* A0 low low */
#define GET_MDA0LL_VAL			*(u8 *)0xFF62
#define SET_MDA0LL_VAL(v)		*(u8 *)0xFF62 = (v)

/* A0 low high */
#define GET_MDA0LH_VAL			*(u8 *)0xFF63
#define SET_MDA0LH_VAL(v)		*(u8 *)0xFF63 = (v)

/* A0 high */

/* A0 high low */
#define GET_MDA0HL_VAL			*(u8 *)0xFF64
#define SET_MDA0HL_VAL(v)		*(u8 *)0xFF64 = (v)

/* A0 high high */
#define GET_MDA0HH_VAL			*(u8 *)0xFF65
#define SET_MDA0HH_VAL(v)		*(u8 *)0xFF65 = (v)

/* Multiplication/division data register B0 */

/* B0 low */
#define GET_MDB0L_VAL			*(u8 *)0xFF66
#define SET_MDB0L_VAL(v)		*(u8 *)0xFF66 = (v)

/* B0 High */
#define GET_MDB0H_VAL			*(u8 *)0xFF66
#define SET_MDB0H_VAL(v)		*(u8 *)0xFF66 = (v)

/* Multiplier/divider control register 0 */
#define GET_DMUC0_VAL			*(u8 *)0xFF68
#define SET_DMUC0_VAL(v)		*(u8 *)0xFF68 = (v)

/* 8-bit timer H mode register 0  */
#define GET_TMHMD0_VAL			*(u8 *)0xFF69
#define SET_TMHMD0_VAL(v)		*(u8 *)0xFF69 = (v)

/* Timer clock selection register 50  */
#define GET_TCL50_VAL			*(u8 *)0xFF6A
#define SET_TCL50_VAL(v)		*(u8 *)0xFF6A = (v)

/* 8-bit timer mode control register 50  */
#define GET_TMC50_VAL			*(u8 *)0xFF6B
#define SET_TMC50_VAL(v)		*(u8 *)0xFF6B = (v)

/* 8-bit timer H mode register 1  */
#define GET_TMHMD1_VAL			*(u8 *)0xFF6C
#define SET_TMHMD1_VAL(v)		*(u8 *)0xFF6C = (v)

/* 8-bit timer H carrier control register 1  */
#define GET_TMCYC1_VAL			*(u8 *)0xFF6D
#define SET_TMCYC1_VAL(v)		*(u8 *)0xFF6D = (v)

/* Key return mode register  */
#define GET_KRM_VAL				*(u8 *)0xFF6E
#define SET_KRM_VAL(v)			*(u8 *)0xFF6E = (v)

/* Watch timer operation mode register */
#define GET_WTM_VAL				*(u8 *)0xFF6F
#define SET_WTM_VAL(v)			*(u8 *)0xFF6F = (v)

/* Asynchronous serial interface operation mode register 0 */
#define GET_ASIM0_VAL			*(u8 *)0xFF70
#define SET_ASIM0_VAL(v)		*(u8 *)0xFF70 = (v)

/* Baud rate generator control register 0 */
#define GET_BRGC0_VAL			*(u8 *)0xFF71
#define SET_BRGC0_VAL(v)		*(u8 *)0xFF71 = (v)

/* Receive buffer register 0 */
#define GET_RXB0_VAL			*(u8 *)0xFF72

/* Asynchronous serial interface reception error status register 0 */
#define GET_ASIS0_VAL			*(u8 *)0xFF73

/* Transmit shift register 0 */
#define SET_TXS0_VAL(v)			*(u8 *)0xFF74 = (v)

/* Serial operation mode register 10 */
#define GET_CSIM10_VAL			*(u8 *)0xFF80
#define SET_CSIM10_VAL(v)		*(u8 *)0xFF80 = (v)

/* Serial clock selection register 10 */
#define GET_CSIC10_VAL			*(u8 *)0xFF81
#define SET_CSIC10_VAL(v)		*(u8 *)0xFF81 = (v)

/* Transmit buffer register 10 */
#define GET_SOTB10_VAL			*(u8 *)0xFF84
#define SET_SOTB10_VAL(v)		*(u8 *)0xFF84 = (v)

/* Serial operation mode register 11 */
#define GET_CSIM11_VAL			*(u8 *)0xFF88
#define SET_CSIM11_VAL(v)		*(u8 *)0xFF88 = (v)

/* Serial clock selection register 11 */
#define GET_CSIC11_VAL			*(u8 *)0xFF89
#define SET_CSIC11_VAL(v)		*(u8 *)0xFF89 = (v)

/* Timer clock selection register 51 */
#define GET_TCL51_VAL			*(u8 *)0xFF8C
#define SET_TCL51_VAL(v)		*(u8 *)0xFF8C = (v)

/* Watchdog timer enable register */
#define GET_WDTE_VAL			*(u8 *)0xFF99
#define SET_WDTE_VAL(v)			*(u8 *)0xFF99 = (v)

/* Clock operation mode select register */
#define GET_OSCCTL_VAL			*(u8 *)0xFF9F
#define SET_OSCCTL_VAL(v)		*(u8 *)0xFF9F = (v)

/* Internal oscillation mode register */
#define GET_RCM_VAL			*(u8 *)0xFFA0
#define SET_RCM_VAL(v)		*(u8 *)0xFFA0 = (v)

/* Main clock mode register */
#define GET_MCM_VAL			*(u8 *)0xFFA1
#define SET_MCM_VAL(v)		*(u8 *)0xFFA1 = (v)

/* Main OSC control register */
#define GET_MOC_VAL			*(u8 *)0xFFA2
#define SET_MOC_VAL(v)		*(u8 *)0xFFA2 = (v)

/* Oscillation stabilization time counter status register */
#define GET_OSTC_VAL		*(u8 *)0xFFA3

/* Oscillation stabilization time select register */
#define GET_OSTS_VAL		*(u8 *)0xFFA4
#define SET_OSTS_VAL(v)		*(u8 *)0xFFA4 = (v)

/* IIC shift register 0 */
#define GET_IIC0_VAL		*(u8 *)0xFFA5
#define SET_IIC0_VAL(v)		*(u8 *)0xFFA5 = (v)

/* IIC control register 0 */
#define GET_IICC0_VAL		*(u8 *)0xFFA6
#define SET_IICC0_VAL(v)	*(u8 *)0xFFA6 = (v)

/* Slave address register 0 */
#define GET_SVA0_VAL		*(u8 *)0xFFA7
#define SET_SVA0_VAL(v)		*(u8 *)0xFFA7 = (v)

/* IIC clock selection register 0 */
#define GET_IICCL0_VAL		*(u8 *)0xFFA8
#define SET_IICCL0_VAL(v)	*(u8 *)0xFFA8 = (v)

/* IIC function expansion register 0 */
#define GET_IICX0_VAL		*(u8 *)0xFFA9
#define SET_IICX0_VAL(v)	*(u8 *)0xFFA9 = (v)

/* IIC status register 0 */
#define GET_IICS0_VAL		*(u8 *)0xFFAA

/* IIC flag register 0 */
#define GET_IICF0_VAL		*(u8 *)0xFFAB
#define SET_IICF0_VAL(v)	*(u8 *)0xFFAB = (v)

/* Reset control flag register */
#define GET_RESF_VAL		*(u8 *)0xFFAC

/* 16-bit timer counter 01 */
#define GET_IICF0_VAL		*(u16 *)0xFFB0

/* 16-bit timer capture/compare register 001 */
#define GET_CR001_VAL		*(u16 *)0xFFB2
#define SET_CR001_VAL(v)	*(u16 *)0xFFB2 = (v)

/* 16-bit timer capture/compare register 011 */
#define GET_CR011_VAL		*(u16 *)0xFFB4
#define SET_CR011_VAL(v)	*(u16 *)0xFFB4 = (v)

/* 16-bit timer mode control register 01 */
#define GET_TMC01_VAL		*(u8 *)0xFFB6
#define SET_TMC01_VAL(v)	*(u8 *)0xFFB6 = (v)

/* Prescaler mode register 01 */
#define GET_PRM01_VAL		*(u8 *)0xFFB7
#define SET_PRM01_VAL(v)	*(u8 *)0xFFB7 = (v)

/* Capture/compare control register 01 */
#define GET_CRC01_VAL		*(u8 *)0xFFB8
#define SET_CRC01_VAL(v)	*(u8 *)0xFFB8 = (v)

/* 16-bit timer output control register 01 */
#define GET_TOC01_VAL		*(u8 *)0xFFB9
#define SET_TOC01_VAL(v)	*(u8 *)0xFFB9 = (v)

/* 16-bit timer mode control register 00 */
#define GET_TMC00_VAL		*(u8 *)0xFFBA
#define SET_TMC00_VAL(v)	*(u8 *)0xFFBA = (v)

/* Prescaler mode register 00 */
#define GET_PRM00_VAL		*(u8 *)0xFFBB
#define SET_PRM00_VAL(v)	*(u8 *)0xFFBB = (v)

/* Capture/compare control register 00 */
#define GET_CRC00_VAL		*(u8 *)0xFFBC
#define SET_CRC00_VAL(v)	*(u8 *)0xFFBC = (v)

/* 16-bit timer output control register 00 */
#define GET_TOC00_VAL		*(u8 *)0xFFBD
#define SET_TOC00_VAL(v)	*(u8 *)0xFFBD = (v)

/* Low-voltage detection register */
#define GET_LVIM_VAL		*(u8 *)0xFFBE
#define SET_LVIM_VAL(v)		*(u8 *)0xFFBE = (v)

/* Low-voltage detection level selection register */
#define GET_LVIS_VAL		*(u8 *)0xFFBF
#define SET_LVIS_VAL(v)		*(u8 *)0xFFBF = (v)

/* Interrupt request flag register 0L */
#define GET_IF0L_VAL		*(u8 *)0xFFE0
#define SET_IF0L_VAL(v)		*(u8 *)0xFFE0 = (v)

/* Interrupt request flag register 0H */
#define GET_IF0H_VAL		*(u8 *)0xFFE1
#define SET_IF0H_VAL(v)		*(u8 *)0xFFE1 = (v)

/* Interrupt request flag register 1L */
#define GET_IF1L_VAL		*(u8 *)0xFFE2
#define SET_IF1L_VAL(v)		*(u8 *)0xFFE2 = (v)

/* Interrupt request flag register 1H */
#define GET_IF1H_VAL		*(u8 *)0xFFE3
#define SET_IF1H_VAL(v)		*(u8 *)0xFFE3 = (v)

/* Interrupt mask flag register 0L */
#define GET_MK0L_VAL		*(u8 *)0xFFE4
#define SET_MK0L_VAL(v)		*(u8 *)0xFFE4 = (v)

/* Interrupt mask flag register 0H */
#define GET_MK0H_VAL		*(u8 *)0xFFE5
#define SET_MK0H_VAL(v)		*(u8 *)0xFFE5 = (v)

/* Interrupt mask flag register 1L */
#define GET_MK1L_VAL		*(u8 *)0xFFE6
#define SET_MK1L_VAL(v)		*(u8 *)0xFFE6 = (v)

/* Interrupt mask flag register 1H */
#define GET_MK1H_VAL		*(u8 *)0xFFE7
#define SET_MK1H_VAL(v)		*(u8 *)0xFFE7 = (v)

/* Priority specification flag register 0L */
#define GET_PR0L_VAL		*(u8 *)0xFFE8
#define SET_PR0L_VAL(v)		*(u8 *)0xFFE8 = (v)

/* Priority specification flag register 0H */
#define GET_PR0H_VAL		*(u8 *)0xFFE9
#define SET_PR0H_VAL(v)		*(u8 *)0xFFE9 = (v)

/* Priority specification flag register 1L */
#define GET_PR1L_VAL		*(u8 *)0xFFEA
#define SET_PR1L_VAL(v)		*(u8 *)0xFFEA = (v)

/* Priority specification flag register 1H */
#define GET_PR1H_VAL		*(u8 *)0xFFEB
#define SET_PR1H_VAL(v)		*(u8 *)0xFFEB = (v)

/* Internal memory size switching register */
#define GET_IMS_VAL			*(u8 *)0xFFF0
#define SET_IMS_VAL(v)		*(u8 *)0xFFF0 = (v)

/* Memory bank select register */
#define GET_BANK_VAL		*(u8 *)0xFFF3
#define SET_BANK_VAL(v)		*(u8 *)0xFFF3 = (v)

/* Internal expansion RAM size switching register */
#define GET_IXS_VAL			*(u8 *)0xFFF4
#define SET_IXS_VAL(v)		*(u8 *)0xFFF4 = (v)

/* Processor clock control register */
#define GET_PCC_VAL			*(u8 *)0xFFFB
#define SET_PCC_VAL(v)		*(u8 *)0xFFFB = (v)

#endif /* SFR_H */
