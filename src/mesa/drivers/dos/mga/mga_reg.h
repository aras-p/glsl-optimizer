/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver v1.3 for Mesa 5.0  --  MGA2064W register mnemonics
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef MGA_REG_H_included
#define MGA_REG_H_included

/* Matrox hardware registers: */
#define M_AR0                 0x1C60
#define M_AR1                 0x1C64
#define M_AR2                 0x1C68
#define M_AR3                 0x1C6C
#define M_AR4                 0x1C70
#define M_AR5                 0x1C74
#define M_AR6                 0x1C78
#define M_BCOL                0x1C20
#define M_CXBNDRY             0x1C80
#define M_CXLEFT              0x1CA0
#define M_CXRIGHT             0x1CA4
#define M_DR0                 0x1CC0
#define M_DR2                 0x1CC8
#define M_DR3                 0x1CCC
#define M_DR4                 0x1CD0
#define M_DR6                 0x1CD8
#define M_DR7                 0x1CDC
#define M_DR8                 0x1CE0
#define M_DR10                0x1CE8
#define M_DR11                0x1CEC
#define M_DR12                0x1CF0
#define M_DR14                0x1CF8
#define M_DR15                0x1CFC
#define M_DWGCTL              0x1C00
#define M_FCOL                0x1C24
#define M_FIFOSTATUS          0x1E10
#define M_FXBNDRY             0x1C84
#define M_FXLEFT              0x1CA8
#define M_FXRIGHT             0x1CAC
#define M_ICLEAR              0x1E18
#define M_IEN                 0x1E1C
#define M_LEN                 0x1C5C
#define M_MACCESS             0x1C04
#define M_OPMODE              0x1E54
#define M_PAT0                0x1C10
#define M_PAT1                0x1C14
#define M_PITCH               0x1C8C
#define M_PLNWT               0x1C1C
#define M_RESET               0x1E40
#define M_SGN                 0x1C58
#define M_SHIFT               0x1C50
#define M_SRC0                0x1C30
#define M_SRC1                0x1C34
#define M_SRC2                0x1C38
#define M_SRC3                0x1C3C
#define M_STATUS              0x1E14
#define M_VCOUNT              0x1E20
#define M_XDST                0x1CB0
#define M_XYEND               0x1C44
#define M_XYSTRT              0x1C40
#define M_YBOT                0x1C9C
#define M_YDST                0x1C90
#define M_YDSTLEN             0x1C88
#define M_YDSTORG             0x1C94
#define M_YTOP                0x1C98
#define M_ZORG                0x1C0C

#define M_EXEC                0x0100

/* DWGCTL: opcod */
#define M_DWG_LINE_OPEN       0x0
#define M_DWG_AUTOLINE_OPEN   0x1
#define M_DWG_LINE_CLOSE      0x2
#define M_DWG_AUTOLINE_CLOSE  0x3
#define M_DWG_TRAP            0x4
#define M_DWG_TEXTURE_TRAP    0x5
#define M_DWG_BITBLT          0x8
#define M_DWG_FBITBLT         0xC
#define M_DWG_ILOAD           0x9
#define M_DWG_ILOAD_SCALE     0xD
#define M_DWG_ILOAD_FILTER    0xF
#define M_DWG_IDUMP           0xA

/* DWGCTL: atype */
#define M_DWG_RPL             (0x0 << 4)
#define M_DWG_RSTR            (0x1 << 4)
#define M_DWG_ZI              (0x3 << 4)
#define M_DWG_BLK             (0x4 << 4)
#define M_DWG_I               (0x7 << 4)

/* DWGCTL: linear */
#define M_DWG_LINEAR          (0x1 << 7)

/* DWGCTL: zmode */
#define M_DWG_NOZCMP          (0x0 << 8)
#define M_DWG_ZE              (0x2 << 8)
#define M_DWG_ZNE             (0x3 << 8)
#define M_DWG_ZLT             (0x4 << 8)
#define M_DWG_ZLTE            (0x5 << 8)
#define M_DWG_ZGT             (0x6 << 8)
#define M_DWG_ZGTE            (0x7 << 8)

/* DWGCTL: solid */
#define M_DWG_SOLID           (0x1 << 11)

/* DWGCTL: arzero */
#define M_DWG_ARZERO          (0x1 << 12)

/* DWGCTL: sgnzero */
#define M_DWG_SGNZERO         (0x1 << 13)

/* DWGCTL: shiftzero */
#define M_DWG_SHFTZERO        (0x1 << 14)

/* DWGCTL: bop */
#define M_DWG_BOP_XOR         (0x6 << 16)
#define M_DWG_BOP_AND         (0x8 << 16)
#define M_DWG_BOP_SRC         (0xC << 16)
#define M_DWG_BOP_OR          (0xE << 16)

/* DWGCTL: trans */
#define M_DWG_TRANS_0         (0x0 << 20)
#define M_DWG_TRANS_1         (0x1 << 20)
#define M_DWG_TRANS_2         (0x2 << 20)
#define M_DWG_TRANS_3         (0x3 << 20)
#define M_DWG_TRANS_4         (0x4 << 20)
#define M_DWG_TRANS_5         (0x5 << 20)
#define M_DWG_TRANS_6         (0x6 << 20)
#define M_DWG_TRANS_7         (0x7 << 20)
#define M_DWG_TRANS_8         (0x8 << 20)
#define M_DWG_TRANS_9         (0x9 << 20)
#define M_DWG_TRANS_A         (0xA << 20)
#define M_DWG_TRANS_B         (0xB << 20)
#define M_DWG_TRANS_C         (0xC << 20)
#define M_DWG_TRANS_D         (0xD << 20)
#define M_DWG_TRANS_E         (0xE << 20)
#define M_DWG_TRANS_F         (0xF << 20)

/* DWGCTL: bltmod */
#define M_DWG_BMONOLEF        (0x0 << 25)
#define M_DWG_BMONOWF         (0x4 << 25)
#define M_DWG_BPLAN           (0x1 << 25)
#define M_DWG_BFCOL           (0x2 << 25)
#define M_DWG_BUYUV           (0xE << 25)
#define M_DWG_BU32BGR         (0x3 << 25)
#define M_DWG_BU32RGB         (0x7 << 25)
#define M_DWG_BU24BGR         (0xB << 25)
#define M_DWG_BU24RGB         (0xF << 25)

/* DWGCTL: pattern */
#define M_DWG_PATTERN         (0x1 << 29)

/* DWGCTL: transc */
#define M_DWG_TRANSC          (0x1 << 30)

/* OPMODE: */
#define M_DMA_GENERAL         (0x0 << 2)
#define M_DMA_BLIT            (0x1 << 2)
#define M_DMA_VECTOR          (0x2 << 2)

/* SGN: */
#define M_SDXL                (0x1 << 1)
#define M_SDXR                (0x1 << 5)



/* VGAREG */
#define M_CRTC_INDEX          0x1FD4
#define M_CRTC_DATA           0x1FD5

#define M_CRTC_EXT_INDEX      0x1FDE
#define M_CRTC_EXT_DATA       0x1FDF

#define M_MISC_R              0x1FCC
#define M_MISC_W              0x1FC2

/* CRTCEXT3: */
#define M_MGAMODE             (0x1 << 7)

#endif
