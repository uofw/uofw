/* Copyright (C) The uOFW team
   See the file COPYING for copying permission.
*/

#ifndef SFR_H
#define SFR_H

/* Special Function Registers (SFR) */

/* Port register 1 */
#define P0			(*(u8 *)0xFF00)

/* Port register 1 */
#define P1			(*(u8 *)0xFF01)

/* Port register 2 */
#define P2			(*(u8 *)0xFF02)

/* Port register 3 */
#define P3			(*(u8 *)0xFF03)

/* Port register 4 */
#define P4			(*(u8 *)0xFF04)

/* Port register 5 */
#define P5			(*(u8 *)0xFF05)

/* Port register 6 */
#define P6			(*(u8 *)0xFF06)

/* Port register 7 */
#define P7			(*(u8 *)0xFF07)

/* 10-bit A/D conversion result register */
#define ADCR		(*(const u16 *)0xFF08)

/* 8-bit A/D conversion result register */
#define ADCRH		(*(const u8 *)0xFF09)

/* Receive buffer register 6 */
#define RXB6		(*(const u8 *)0xFF0A)

/* Transmit buffer register 6 */
#define TXB6		(*(u8 *)0xFF0B))

/* Port register 12 */
#define P12			(*(u8 *)0xFF00)

/* Port register 14 */
#define P14			(*(u8 *)0xFF00)

/* Serial I/O shift register 10 */
#define SIO10		(*(const u8 *)0xFF0F)

/* 16-bit timer counter 00 */
#define TM00		(*(const u16 *)0xFF10)

/* 16-bit timer capture/compare register 000 */
#define CR000		(*(u16 *)0xFF12)

/* 16-bit timer capture/compare register 010 */
#define CR010		(*(u16 *)0xFF14)

/* 8-bit timer counter 50 */
#define TM50		(*(const u8 *)0xFF16)

/* 8-bit timer compare register 50 */
#define CR50		(*(u8 *)0xFF17)

/* 8-bit timer H compare register 00 */
#define CMP00		(*(u8 *)0xFF18)

/* 8-bit timer H compare register 10 */
#define CMP10		(*(u8 *)0xFF19)

/* 8-bit timer H compare register 01 */
#define CMP01		(*(u8 *)0xFF1A)

/* 8-bit timer H compare register 11 */
#define CMP11		(*(u8 *)0xFF1B)

/* 8-bit timer counter 51 */
#define TM51		(*(const u8 *)0xFF1F

/* Port mode register 0 */
#define PM0			(*(u8 *)0xFF20)

/* Port mode register 1 */
#define PM1			(*(u8 *)0xFF21)

/* Port mode register 2 */
#define PM2			(*(u8 *)0xFF22)

/* Port mode register 3 */
#define PM3			(*(u8 *)0xFF23)

/* Port mode register 4 */
#define PM4			(*(u8 *)0xFF24)

/* Port mode register 5 */
#define PM5			(*(u8 *)0xFF25)

/* Port mode register 6 */
#define PM6			(*(u8 *)0xFF26)

/* Port mode register 7 */
#define PM7			(*(u8 *)0xFF27)

/* A/D converter mode register */
#define ADM			(*(u8 *)0xFF28)			

/* Analog input channel specification register */
#define ADS			(*(u8 *)0xFF29)

/* Port mode register 12 */
#define PM12		(*(u8 *)0xFF2C)

/* Port mode register 14 */
#define PM14		(*(u8 *)0xFF2E)

/* A/D port configuration register */
#define ADPC		(*(u8 *)0xFF2F)	

/* Pull-up resistor option register 0 */
#define PU0			(*(u8 *)0xFF30)

/* Pull-up resistor option register 1 */
#define PU1			(*(u8 *)0xFF31)

/* Pull-up resistor option register 3 */
#define PU3			(*(u8 *)0xFF33)

/* Pull-up resistor option register 4 */
#define PU4			(*(u8 *)0xFF34)

/* Pull-up resistor option register 5 */
#define PU5			(*(u8 *)0xFF35)

/* Pull-up resistor option register 7 */
#define PU7			(*(u8 *)0xFF37)

/* Pull-up resistor option register 12 */
#define PU12		(*(u8 *)0xFF3C)

/* Pull-up resistor option register 14 */
#define PU14		(*(u8 *)0xFF3E)

/* Clock output selection register */
#define CKS			(*(u8 *)0xFF40)

/* 8-bit timer compare register 51 */
#define CR51		(*(u8 *)0xFF41)

/* 8-bit timer mode control register 51 */
#define TMC51		(*(u8 *)0xFF43)

/* External interrupt rising edge enable register */
#define EGP			(*(u8 *)0xFF48)

/* External interrupt falling edge enable register */
#define EGN			(*(u8 *)0xFF49)

/* Serial I/O shift register 11 */
#define SIO11		(*(const u8 *)0xFF4A)

/* Transmit buffer register 11 */
#define SOTB11		(*(u8 *)0xFF4C)

/* Input switch control register */
#define ISC			(*(u8 *)0xFF4F)

/* Asynchronous serial interface operation mode register 6 */
#define ASIM6		(*(u8 *)0xFF50)

/* Asynchronous serial interface reception error status register 6 */
#define ASIS6		(*(const u8 *)0xFF53)

/* Asynchronous serial interface transmission status register 6 */
#define ASIF6		(*(const u8 *)0xFF55)

/* Clock selection register 6 */
#define CKSR6		(*(u8 *)0xFF56)

/* Baud rate generator control register 6 */
#define BRGC6		(*(u8 *)0xFF57)

/* Asynchronous serial interface control register 6 */
#define ASICL6		(*(u8 *)0xFF58)

/* Remainder data register 0 */
#define SDR0		(*(const u8 *)0xFF60)

/* Multiplication/division data register A0 */

/* A0 low */

/* A0 low low */
#define MDA0LL		(*(u8 *)0xFF62)

/* A0 low high */
#define MDA0LH		(*(u8 *)0xFF63)

/* A0 high */

/* A0 high low */
#define MDA0HL		(*(u8 *)0xFF64)

/* A0 high high */
#define MDA0HH		(*(u8 *)0xFF65)

/* Multiplication/division data register B0 */

/* B0 low */
#define MDB0L		(*(u8 *)0xFF66)

/* B0 High */
#define MDB0H		(*(u8 *)0xFF66)

/* Multiplier/divider control register 0 */
#define DMUC0		(*(u8 *)0xFF68)

/* 8-bit timer H mode register 0  */
#define TMHMD0		(*(u8 *)0xFF69)

/* Timer clock selection register 50  */
#define TCL50		(*(u8 *)0xFF6A)

/* 8-bit timer mode control register 50  */
#define TMC50		(*(u8 *)0xFF6B)

/* 8-bit timer H mode register 1  */
#define TMHMD1		(*(u8 *)0xFF6C)

/* 8-bit timer H carrier control register 1  */
#define TMCYC1		(*(u8 *)0xFF6D)

/* Key return mode register  */
#define KRM			(*(u8 *)0xFF6E)

/* Watch timer operation mode register */
#define WTM			(*(u8 *)0xFF6F)

/* Asynchronous serial interface operation mode register 0 */
#define ASIM0		(*(u8 *)0xFF70)

/* Baud rate generator control register 0 */
#define BRGC0		(*(u8 *)0xFF71)

/* Receive buffer register 0 */
#define RXB0		(*(const u8 *)0xFF72)

/* Asynchronous serial interface reception error status register 0 */
#define ASIS0		(*(const u8 *)0xFF73)

/* Transmit shift register 0 (write-only) */
#define TXS0		(*(u8 *)0xFF74)

/* Serial operation mode register 10 */
#define CSIM10		(*(u8 *)0xFF80)

/* Serial clock selection register 10 */
#define CSIC10		(*(u8 *)0xFF81)

/* Transmit buffer register 10 */
#define SOTB10		(*(u8 *)0xFF84)

/* Serial operation mode register 11 */
#define CSIM11		(*(u8 *)0xFF88)

/* Serial clock selection register 11 */
#define CSIC11		(*(u8 *)0xFF89)

/* Timer clock selection register 51 */
#define TCL51		(*(u8 *)0xFF8C)

/*
 * When this constant is written to the WDTE register, the watchdog timer counter is cleared and couting starts
 * again.
 */
#define WATCHDOG_TIMER_ENABLE_REGISTER_RESET_WATCHDOG_TIMER		0xAC

/* Watchdog timer enable register */
#define WDTE		(*(u8 *)0xFF99)

/* Clock operation mode select register */
#define OSCCTL		(*(u8 *)0xFF9F)

/* Internal oscillation mode register */
#define RCM			(*(u8 *)0xFFA0)

/* Main clock mode register */
#define MCM			(*(u8 *)0xFFA1)

/* Main OSC control register */
#define MOC			(*(u8 *)0xFFA2)

/* Oscillation stabilization time counter status register */
#define OSTC		(*(const u8 *)0xFFA3)

/* Oscillation stabilization time select register */
#define OSTS		(*(u8 *)0xFFA4)

/* IIC shift register 0 */
#define IIC0		(*(u8 *)0xFFA5)

/* IIC control register 0 */
#define IICC0		(*(u8 *)0xFFA6)

/* Slave address register 0 */
#define SVA0		(*(u8 *)0xFFA7)

/* IIC clock selection register 0 */
#define IICCL0		(*(u8 *)0xFFA8)

/* IIC function expansion register 0 */
#define IICX0		(*(u8 *)0xFFA9)

/* IIC status register 0 */
#define IICS0		(*(const u8 *)0xFFAA)

/* IIC flag register 0 */
#define IICF0		(*(u8 *)0xFFAB)

/* Reset control flag register */
#define RESF		(*(const u8 *)0xFFAC)

/* 16-bit timer counter 01 */
#define IICF0		(*(const u16 *)0xFFB0)

/* 16-bit timer capture/compare register 001 */
#define CR001		(*(u16 *)0xFFB2)

/* 16-bit timer capture/compare register 011 */
#define CR011		(*(u16 *)0xFFB4)

/* 16-bit timer mode control register 01 */
#define TMC01		(*(u8 *)0xFFB6)

/* Prescaler mode register 01 */
#define PRM01		(*(u8 *)0xFFB7)

/* Capture/compare control register 01 */
#define CRC01		(*(u8 *)0xFFB8)

/* 16-bit timer output control register 01 */
#define TOC01		(*(u8 *)0xFFB9)

/* 16-bit timer mode control register 00 */
#define TMC00		(*(u8 *)0xFFBA)

/* Prescaler mode register 00 */
#define PRM00		(*(u8 *)0xFFBB)

/* Capture/compare control register 00 */
#define CRC00		(*(u8 *)0xFFBC)

/* 16-bit timer output control register 00 */
#define TOC00		(*(u8 *)0xFFBD)

/* Low-voltage detection register */
#define LVIM		(*(u8 *)0xFFBE)

/* Low-voltage detection level selection register */
#define LVIS		(*(u8 *)0xFFBF)

#define IF0L_INTR_REQ_FLAG_LVI	(1 << 0)
#define IF0L_INTR_REQ_FLAG_PO	(1 << 1)
#define IF0L_INTR_REQ_FLAG_P1	(1 << 2)
#define IF0L_INTR_REQ_FLAG_P2	(1 << 3) /* Interrupt request flag for input pin 2 (P31) */
#define IF0L_INTR_REQ_FLAG_P3	(1 << 4)
#define IF0L_INTR_REQ_FLAG_P4	(1 << 5)
#define IF0L_INTR_REQ_FLAG_P5	(1 << 6)
#define IF0L_INTR_REQ_FLAG_SRE6	(1 << 7)

/* Interrupt request flag register 0L */
#define IF0L		(*(u8 *)0xFFE0)

#define IF0H_INTR_REQ_FLAG_SR6		(1 << 0)
#define IF0H_INTR_REQ_FLAG_ST6		(1 << 1)
#define IF0H_INTR_REQ_FLAG_CSI10	(1 << 2)
#define IF0H_INTR_REQ_FLAG_TMH1		(1 << 3)
#define IF0H_INTR_REQ_FLAG_TMH0		(1 << 4)
#define IF0H_INTR_REQ_FLAG_TM50		(1 << 5)
#define IF0H_INTR_REQ_FLAG_TM000	(1 << 6)
#define IF0H_INTR_REQ_FLAG_TM010	(1 << 7)

/* Interrupt request flag register 0H */
#define IF0H		(*(u8 *)0xFFE1)

/* Interrupt request flag register 1L */
#define IF1L		(*(u8 *)0xFFE2)

/* Interrupt request flag register 1H */
#define IF1H		(*(u8 *)0xFFE3)

#define MK0L_INTR_MASK_FLAG_LVI		(1 << 0)
#define MK0L_INTR_MASK_FLAG_P0		(1 << 1)
#define MK0L_INTR_MASK_FLAG_P1		(1 << 2)
#define MK0L_INTR_MASK_FLAG_P2		(1 << 3) /* Interrupt mask bit for input pin 2 (P31) */
#define MK0L_INTR_MASK_FLAG_P3		(1 << 4)
#define MK0L_INTR_MASK_FLAG_P4		(1 << 5)
#define MK0L_INTR_MASK_FLAG_P5		(1 << 6)
#define MK0L_INTR_MASK_FLAG_SRE6	(1 << 7)

/* Interrupt mask flag register 0L */
#define MK0L		(*(u8 *)0xFFE4)

/* Interrupt mask flag register 0H */
#define MK0H		(*(u8 *)0xFFE5)

/* Interrupt mask flag register 1L */
#define MK1L		(*(u8 *)0xFFE6)

/* Interrupt mask flag register 1H */
#define MK1H		(*(u8 *)0xFFE7)

/* Priority specification flag register 0L */
#define PR0L		(*(u8 *)0xFFE8)

/* Priority specification flag register 0H */
#define PR0H		(*(u8 *)0xFFE9)

/* Priority specification flag register 1L */
#define PR1L		(*(u8 *)0xFFEA)

/* Priority specification flag register 1H */
#define PR1H		(*(u8 *)0xFFEB)

/* Internal memory size switching register */
#define IMS			(*(u8 *)0xFFF0)

/* Memory bank select register */
#define BANK		(*(u8 *)0xFFF3)

/* Internal expansion RAM size switching register */
#define IXS			(*(u8 *)0xFFF4)

/* Processor clock control register */
#define PCC			(*(u8 *)0xFFFB)

#endif /* SFR_H */
