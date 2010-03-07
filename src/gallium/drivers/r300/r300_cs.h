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

#ifndef R300_CS_H
#define R300_CS_H

#include "util/u_math.h"

#include "r300_reg.h"

#include "radeon_winsys.h"

/* Yes, I know macros are ugly. However, they are much prettier than the code
 * that they neatly hide away, and don't have the cost of function setup,so
 * we're going to use them. */

#define MAX_CS_SIZE 64 * 1024 / 4

#define VERY_VERBOSE_CS 1
#define VERY_VERBOSE_REGISTERS 1

/* XXX stolen from radeon_drm.h */
#define RADEON_GEM_DOMAIN_CPU  0x1
#define RADEON_GEM_DOMAIN_GTT  0x2
#define RADEON_GEM_DOMAIN_VRAM 0x4

/* XXX stolen from radeon_reg.h */
#define RADEON_CP_PACKET0 0x0

#define CP_PACKET0(register, count) \
    (RADEON_CP_PACKET0 | ((count) << 16) | ((register) >> 2))

#define CS_LOCALS(context) \
    struct r300_context* const cs_context_copy = (context); \
    struct radeon_winsys* cs_winsys = cs_context_copy->winsys; \
    int cs_count = 0; (void) cs_count;

#define CHECK_CS(size) \
    assert(cs_winsys->check_cs(cs_winsys, (size)))

#define BEGIN_CS(size) do { \
    CHECK_CS(size); \
    if (VERY_VERBOSE_CS) { \
        DBG(cs_context_copy, DBG_CS, "r300: BEGIN_CS, count %d, in %s (%s:%d)\n", \
                size, __FUNCTION__, __FILE__, __LINE__); \
    } \
    cs_winsys->begin_cs(cs_winsys, (size), \
            __FILE__, __FUNCTION__, __LINE__); \
    cs_count = size; \
} while (0)

#define OUT_CS(value) do { \
    if (VERY_VERBOSE_CS || VERY_VERBOSE_REGISTERS) { \
        DBG(cs_context_copy, DBG_CS, "r300: writing %08x\n", value); \
    } \
    cs_winsys->write_cs_dword(cs_winsys, (value)); \
    cs_count--; \
} while (0)

#define OUT_CS_32F(value) do { \
    if (VERY_VERBOSE_CS || VERY_VERBOSE_REGISTERS) { \
        DBG(cs_context_copy, DBG_CS, "r300: writing %f\n", value); \
    } \
    cs_winsys->write_cs_dword(cs_winsys, fui(value)); \
    cs_count--; \
} while (0)

#define OUT_CS_REG(register, value) do { \
    if (VERY_VERBOSE_REGISTERS) \
        DBG(cs_context_copy, DBG_CS, "r300: writing 0x%08X to register 0x%04X\n", \
            value, register); \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0(register, 0)); \
    cs_winsys->write_cs_dword(cs_winsys, value); \
    cs_count -= 2; \
} while (0)

/* Note: This expects count to be the number of registers,
 * not the actual packet0 count! */
#define OUT_CS_REG_SEQ(register, count) do { \
    if (VERY_VERBOSE_REGISTERS) \
        DBG(cs_context_copy, DBG_CS, "r300: writing register sequence of %d to 0x%04X\n", \
            count, register); \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0((register), ((count) - 1))); \
    cs_count--; \
} while (0)

#define OUT_CS_RELOC(bo, offset, rd, wd, flags) do { \
    DBG(cs_context_copy, DBG_CS, "r300: writing relocation for buffer %p, offset %d, " \
            "domains (%d, %d, %d)\n", \
        bo, offset, rd, wd, flags); \
    assert(bo); \
    cs_winsys->write_cs_dword(cs_winsys, offset); \
    cs_winsys->write_cs_reloc(cs_winsys, bo, rd, wd, flags); \
    cs_count -= 3; \
} while (0)

#define OUT_CS_RELOC_NO_OFFSET(bo, rd, wd, flags) do { \
    DBG(cs_context_copy, DBG_CS, "r300: writing relocation for buffer %p, " \
            "domains (%d, %d, %d)\n", \
        bo, rd, wd, flags); \
    assert(bo); \
    cs_winsys->write_cs_reloc(cs_winsys, bo, rd, wd, flags); \
    cs_count -= 2; \
} while (0)

#define END_CS do { \
    if (VERY_VERBOSE_CS) { \
        DBG(cs_context_copy, DBG_CS, "r300: END_CS in %s (%s:%d)\n", __FUNCTION__, \
                __FILE__, __LINE__); \
    } \
    if (cs_count != 0) \
        debug_printf("r300: Warning: cs_count off by %d\n", cs_count); \
    cs_winsys->end_cs(cs_winsys, __FILE__, __FUNCTION__, __LINE__); \
} while (0)

#define FLUSH_CS do { \
    if (VERY_VERBOSE_CS) { \
        DBG(cs_context_copy, DBG_CS, "r300: FLUSH_CS in %s (%s:%d)\n\n", __FUNCTION__, \
                __FILE__, __LINE__); \
    } \
    cs_winsys->flush_cs(cs_winsys); \
} while (0)

#define RADEON_ONE_REG_WR        (1 << 15)

#define OUT_CS_ONE_REG(register, count) do { \
    if (VERY_VERBOSE_REGISTERS) \
        DBG(cs_context_copy, DBG_CS, "r300: writing data sequence of %d to 0x%04X\n", \
            count, register); \
    assert(register); \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET0((register), ((count) - 1)) | RADEON_ONE_REG_WR); \
    cs_count--; \
} while (0)

#define CP_PACKET3(op, count) \
    (RADEON_CP_PACKET3 | (op) | ((count) << 16))

#define OUT_CS_PKT3(op, count) do { \
    cs_winsys->write_cs_dword(cs_winsys, CP_PACKET3(op, count)); \
    cs_count--; \
} while (0)

#define OUT_CS_INDEX_RELOC(bo, offset, count, rd, wd, flags) do { \
    DBG(cs_context_copy, DBG_CS, "r300: writing relocation for index buffer %p," \
            "offset %d\n", bo, offset); \
    assert(bo); \
    cs_winsys->write_cs_dword(cs_winsys, offset); \
    cs_winsys->write_cs_dword(cs_winsys, count); \
    cs_winsys->write_cs_reloc(cs_winsys, bo, rd, wd, flags); \
    cs_count -= 4; \
} while (0)

#endif /* R300_CS_H */
