/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_CMDBUF_H__
#define __R300_CMDBUF_H__

#include "r300_context.h"
#include "radeon_cs.h"

extern int r300FlushCmdBufLocked(r300ContextPtr r300, const char *caller);
extern int r300FlushCmdBuf(r300ContextPtr r300, const char *caller);

extern void r300EmitState(r300ContextPtr r300);

extern void r300InitCmdBuf(r300ContextPtr r300);
extern void r300DestroyCmdBuf(r300ContextPtr r300);
extern void r300EnsureCmdBufSpace(r300ContextPtr r300, int dwords, const char *caller);

void r300BeginBatch(r300ContextPtr r300,
		    int n,
		    int dostate,
                    const char *file,
                    const char *function,
                    int line);

/**
 * Every function writing to the command buffer needs to declare this
 * to get the necessary local variables.
 */
#define BATCH_LOCALS(r300) \
	const r300ContextPtr b_l_r300 = r300

/**
 * Prepare writing n dwords to the command buffer,
 * including producing any necessary state emits on buffer wraparound.
 */
#define BEGIN_BATCH(n) r300BeginBatch(b_l_r300, n, 1, __FILE__, __FUNCTION__, __LINE__)

/**
 * Same as BEGIN_BATCH, but do not cause automatic state emits.
 */
#define BEGIN_BATCH_NO_AUTOSTATE(n) r300BeginBatch(b_l_r300, n, 0, __FILE__, __FUNCTION__, __LINE__)

/**
 * Write one dword to the command buffer.
 */
#define OUT_BATCH(data) \
	do { \
        radeon_cs_write_dword(b_l_r300->cmdbuf.cs, data);\
	} while(0)

/**
 * Write a relocated dword to the command buffer.
 */
#define OUT_BATCH_RELOC(data, bo, offset, rd, wd, flags) \
	do { \
        if (offset) {\
            fprintf(stderr, "(%s:%s:%d) offset : %d\n",\
            __FILE__, __FUNCTION__, __LINE__, offset);\
        }\
        radeon_cs_write_dword(b_l_r300->cmdbuf.cs, offset);\
        radeon_cs_write_reloc(b_l_r300->cmdbuf.cs, \
                              bo, \
                              offset, \
                              (bo)->size, \
                              rd, \
                              wd, \
                              flags);\
	} while(0)

/**
 * Write n dwords from ptr to the command buffer.
 */
#define OUT_BATCH_TABLE(ptr,n) \
	do { \
		int _i; \
        for (_i=0; _i < n; _i++) {\
            radeon_cs_write_dword(b_l_r300->cmdbuf.cs, ptr[_i]);\
        }\
	} while(0)

/**
 * Finish writing dwords to the command buffer.
 * The number of (direct or indirect) OUT_BATCH calls between the previous
 * BEGIN_BATCH and END_BATCH must match the number specified at BEGIN_BATCH time.
 */
#define END_BATCH() \
	do { \
        radeon_cs_end(b_l_r300->cmdbuf.cs, __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

/**
 * After the last END_BATCH() of rendering, this indicates that flushing
 * the command buffer now is okay.
 */
#define COMMIT_BATCH() \
	do { \
	} while(0)

void emit_vpu(r300ContextPtr r300, struct r300_state_atom * atom);
int check_vpu(r300ContextPtr r300, struct r300_state_atom *atom);

#endif				/* __R300_CMDBUF_H__ */
