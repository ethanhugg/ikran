/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _UART_H_
#define _UART_H_

#include "cpr_types.h"


/*
 * Telecaster specific defines
 */
#define SYSCLOCK                ((unsigned long)25000000)  /* 25 MHZ */
#define UART_BASE_ADDRESS       (UART_16550_REGS *)0xFFFE4000

/*
 * 16550 register definition
 */
typedef struct {
    volatile uint8_t dc;        /* RBR/THR */
    volatile uint8_t ier;       /* Interrupt Enable Reg */
    volatile uint8_t iir;       /* Interrupt ID Reg */
    volatile uint8_t lcr;       /* Line Control Reg / FIFO Control Reg */
    volatile uint8_t mcr;       /* Modem Control Register */
    volatile uint8_t lsr;       /* Line Status Register */
    volatile uint8_t msr;       /* Modem Status Register */
    volatile uint8_t scr;       /* Scratch Register */
} UART_16550_REGS;

/*
 * UART register bit symbols
 */
#define UART_IER_MASK      0xF0
#define IER_RX_IE          0x01
#define IER_TX_IE          0x02
#define IER_LS_IE          0x04
#define IER_MS_IE          0x08

#define UART_IIR_RV        0x01
#define IIR_IPEND          0x01
#define IIR_MODEM_STAT     0x00
#define IIR_TX_EMPTY       0x02
#define IIR_RX_AVAIL       0x04
#define IIR_RX_LINE_STAT   0x06
#define IIR_CHAR_TIMOUT    0x0C

#define UART_FCR_MASK      0x30
#define FCR_FIFO_EN        0x01
#define FCR_CLR_RXF        0x02
#define FCR_CLR_TXF        0x04
#define FCR_TRIGR_1BYT     0x00
#define FCR_TRIGR_4BYT     0x40
#define FCR_TRIGR_8BYT     0x80
#define FCR_TRIGR_14BYT    0xC0

#define LCR_CHR_LEN_5BIT   0x00
#define LCR_CHR_LEN_6BIT   0x01
#define LCR_CHR_LEN_7BIT   0x02
#define LCR_CHR_LEN_8BIT   0x03
#define LCR_STOP1          0x00
#define LCR_STOP2          0x04
#define LCR_PAR_EN         0x08
#define LCR_PAR_ODD        0x10
#define LCR_FIX_PAR        0x20
#define LCR_BRK_CTL        0x40
#define LCR_DLAB           0x80

#define MCR_DTR            0x01
#define MCR_RTS            0x02
#define MCR_OUT1           0x04
#define MCR_OUT2           0x08
#define MCR_LPBK           0x10
#define MCR_AUTO_FLW_EN    0x20

#define UART_LSR_RV        0x60
#define LSR_RX_AVAIL       0x01
#define LSR_OVRUN          0x02
#define LSR_PAR_ERR        0x04
#define LSR_FRAM_ERR       0x08
#define LSR_BRK_INT        0x10
#define LSR_TX_EMPTY       0x20
#define LSR_XMITR_EMPTY    0x40
#define LSR_RX_FIFO_ERR    0x80

#define UART_MSR_RV        0x00
#define MSR_CTS_CHG        0x01
#define MSR_DSR_CHG        0x02
#define MSR_TERI           0x04
#define MSR_CD_CHG         0x08
#define MSR_CTS_IN         0x10
#define MSR_DSR_IN         0x20
#define MSR_RI_IN          0x40
#define MSR_CD_IN          0x80


#define DATAREADY     0x01
#define XMITFIFOEMPTY 0x20


#define RXLINESTATUS  (3<<1)
#define RXDATAAVAIL   (2<<1)
#define CHARTIMEOUT   (6<<1)
#define TXEMPTY       (1<<1)
#define MODEMSTATUS   (0<<1)
#define RXERRORS      0x8E
#define DATAREADY     0x01
#define XMITFIFOEMPTY 0x20


#define TX_EMPTY 0x01

//extern int uart_init(int speed, char *buf, int buf_size);
extern int32_t uart_init();
extern void uart_flush(int32_t dev);
extern int32_t uart_outc(int8_t c, void *);
extern int32_t uart_outs(int8_t *s, void *);

#endif
