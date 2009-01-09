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

#include "radeon_cs.h"
#include "radeon_reg.h"

/* Yes, I know macros are ugly. However, they are much prettier than the code
 * that they neatly hide away, and don't have the cost of function setup,so
 * we're going to use them. */

#define MAX_CS_SIZE 64 * 1024 / 4

#define CP_PACKET0(register, count) \
    (RADEON_CP_PACKET0 | ((count) << 16) | ((register) >> 2))

#define CS_LOCALS(context) \
    struct radeon_cs* cs = context->cs


#define CHECK_CS(size) do { \
    if ((cs->cdw + (size) + 128) > MAX_CS_SIZE || radeon_cs_need_flush(cs)) { \
        /* XXX flush the CS */ \
    } } while (0)

/* XXX radeon_cs_begin is currently unimplemented on the backend, but let's
 * be future-proof, yeah? */
#define BEGIN_CS(size) do { \
    CHECK_CS(size); \
    radeon_cs_begin(cs, (size), __FILE__, __FUNCTION__, __LINE__); \
} while (0)

#define OUT_CS(value) \
    radeon_cs_write_dword(cs, value)

#define OUT_CS_REG(register, value) do { \
    OUT_CS(CP_PACKET0(register, 0)); \
    OUT_CS(value); } while (0)

#define OUT_CS_RELOC(bo, offset, rd, wd, flags) do { \
    radeon_cs_write_dword(cs, offset); \
    radeon_cs_write_reloc(cs, bo, rd, wd, flags); \
} while (0)

/* XXX more future-proofing */
#define END_CS \
    radeon_cs_end(cs, __FILE__, __FUNCTION__, __LINE__)

#endif /* R300_CS_H */