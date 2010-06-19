/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

/**
 * This file contains macros for immediate command submission.
 */

#ifndef R300_CS_H
#define R300_CS_H

#include "r300_reg.h"
#include "r300_context.h"
#include "r300_winsys.h"

/* Yes, I know macros are ugly. However, they are much prettier than the code
 * that they neatly hide away, and don't have the cost of function setup,so
 * we're going to use them. */

#ifdef DEBUG
#define CS_DEBUG(x) x
#else
#define CS_DEBUG(x)
#endif

/**
 * Command submission setup.
 */

#define CS_LOCALS(context) \
    struct r300_context* const cs_context_copy = (context); \
    struct r300_winsys_screen *cs_winsys = cs_context_copy->rws; \
    CS_DEBUG(int cs_count = 0; (void) cs_count;)

#define BEGIN_CS(size) do { \
    assert(r300_check_cs(cs_context_copy, (size))); \
    CS_DEBUG(cs_count = size;) \
} while (0)

#ifdef DEBUG
#define END_CS do { \
    if (cs_count != 0) \
        debug_printf("r300: Warning: cs_count off by %d at (%s, %s:%i)\n", \
                     cs_count, __FUNCTION__, __FILE__, __LINE__); \
    cs_count = 0; \
} while (0)
#else
#define END_CS
#endif

/**
 * Writing pure DWORDs.
 */

#define OUT_CS(value) do { \
    cs_winsys->write_cs_dword(cs_winsys, (value)); \
    CS_DEBUG(cs_count--;) \
} while (0)

#define OUT_CS_32F(value) do { \
    cs_winsys->write_cs_dword(cs_winsys, fui(value)); \
    CS_DEBUG(cs_count--;) \
} while (0)

#define OUT_CS_REG(register, value) do { \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0(register, 0)); \
    cs_winsys->write_cs_dword(cs_winsys, value); \
    CS_DEBUG(cs_count -= 2;) \
} while (0)

/* Note: This expects count to be the number of registers,
 * not the actual packet0 count! */
#define OUT_CS_REG_SEQ(register, count) do { \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0((register), ((count) - 1))); \
    CS_DEBUG(cs_count--;) \
} while (0)

#define OUT_CS_TABLE(values, count) do { \
    cs_winsys->write_cs_table(cs_winsys, values, count); \
    CS_DEBUG(cs_count -= count;) \
} while (0)

#define OUT_CS_ONE_REG(register, count) do { \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0((register), ((count) - 1)) | RADEON_ONE_REG_WR); \
    CS_DEBUG(cs_count--;) \
} while (0)

#define OUT_CS_PKT3(op, count) do { \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET3(op, count)); \
    CS_DEBUG(cs_count--;) \
} while (0)


/**
 * Writing relocations.
 */

#define OUT_CS_RELOC(bo, offset, rd, wd, flags) do { \
    assert(bo); \
    cs_winsys->write_cs_dword(cs_winsys, offset); \
    cs_winsys->write_cs_reloc(cs_winsys, bo, rd, wd, flags); \
    CS_DEBUG(cs_count -= 3;) \
} while (0)

#define OUT_CS_BUF_RELOC(bo, offset, rd, wd, flags) do { \
    assert(bo); \
    OUT_CS_RELOC(r300_buffer(bo)->buf, offset, rd, wd, flags); \
} while (0)

#define OUT_CS_TEX_RELOC(tex, offset, rd, wd, flags) do { \
    assert(tex); \
    OUT_CS_RELOC(tex->buffer, offset, rd, wd, flags); \
} while (0)

#define OUT_CS_BUF_RELOC_NO_OFFSET(bo, rd, wd, flags) do { \
    assert(bo); \
    cs_winsys->write_cs_reloc(cs_winsys, r300_buffer(bo)->buf, rd, wd, flags); \
    CS_DEBUG(cs_count -= 2;) \
} while (0)


/**
 * Command buffer emission.
 */

#define WRITE_CS_TABLE(values, count) do { \
    CS_DEBUG(assert(cs_count == 0);) \
    cs_winsys->write_cs_table(cs_winsys, values, count); \
} while (0)

#endif /* R300_CS_H */
