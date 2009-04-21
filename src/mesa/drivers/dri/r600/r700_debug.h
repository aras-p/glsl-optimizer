/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
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
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */

#ifndef _R700_DEBUG_H_
#define _R700_DEBUG_H_

enum R700_ERROR
{
    ERROR_ASM_VTX_CLAUSE     = 0x1000,
    ERROR_ASM_UNKOWNCLAUSE   = 0x1001,
    ERROR_ASM_ALLOCEXPORTCF  = 0x1002,
    ERROR_ASM_ALLOCVTXCF     = 0x1003,
    ERROR_ASM_ALLOCTEXCF     = 0x1004,
    ERROR_ASM_ALLOCALUCF     = 0x1005,
    ERROR_ASM_UNKNOWNILINST  = 0x1006,
    ERROR_ASM_SRCARGUMENT    = 0x1007,
    ERROR_ASM_DSTARGUMENT    = 0x1008,
    ERROR_ASM_TEXINSTRUCTION = 0x1009,
    ERROR_ASM_ALUINSTRUCTION = 0x100A,
    ERROR_ASM_INSTDSTTRACK   = 0x100B,
    ERROR_ASM_TEXDSTBADTYPE  = 0x100C,
    ERROR_ASM_ALUSRCBADTYPE  = 0x100D,
    ERROR_ASM_ALUSRCSELECT   = 0x100E,
    ERROR_ASM_ALUSRCNUMBER   = 0x100F,
    ERROR_ASM_ALUDSTBADTYPE  = 0x1010,
    ERROR_ASM_CONSTCHANNEL   = 0x1011,
    ERROR_ASM_BADSCALARBZ    = 0x1012,
    ERROR_ASM_BADGPRRESERVE  = 0x1013,
    ERROR_ASM_BADVECTORBZ    = 0x1014,
    ERROR_ASM_BADTEXINST     = 0x1015,
    ERROR_ASM_BADTEXSRC      = 0x1016,
    ERROR_ASM_BADEXPORTTYPE  = 0x1017,


    TODO_ASM_CONSTTEXADDR   = 0x8000,
    TODO_ASM_NEEDIMPINST    = 0x8001,
    TODO_ASM_TXB            = 0x8002,
    TODO_ASM_TXP            = 0x8003
};

enum R700_DUMP_TYPE
{
    DUMP_VERTEX_SHADER      = 0x1,
    DUMP_PIXEL_SHADER       = 0x2,
    DUMP_FETCH_SHADER       = 0x4,
};

#define DEBUGF printf
#define DEBUGP printf

void NormalizeLogErrorCode(int nError);
/*NormalizeLogErrorCode(nLocalError); */
void r700_error(int nLocalError, char *fmt, ...);      
extern void DumpHwBinary(int, void *, int);

#ifdef STANDALONE_COMPILER
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

void LogString(char* szStr);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif /*STANDALONE_COMPILER*/

#endif /*_R700_DEBUG_H_*/
