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

#ifndef R300_SCREEN_H
#define R300_SCREEN_H

#include "pipe/p_screen.h"

#include "r300_chipset.h"

struct radeon_winsys;

struct r300_screen {
    /* Parent class */
    struct pipe_screen screen;

    struct radeon_winsys* radeon_winsys;

    /* Chipset capabilities */
    struct r300_capabilities* caps;

    /** Combination of DBG_xxx flags */
    unsigned debug;
};

struct r300_transfer {
    /* Parent class */
    struct pipe_transfer transfer;

    /* Offset from start of buffer. */
    unsigned offset;
};

/* Convenience cast wrapper. */
static INLINE struct r300_screen* r300_screen(struct pipe_screen* screen) {
    return (struct r300_screen*)screen;
}

/* Convenience cast wrapper. */
static INLINE struct r300_transfer*
r300_transfer(struct pipe_transfer* transfer)
{
    return (struct r300_transfer*)transfer;
}

/* Debug functionality. */

/**
 * Debug flags to disable/enable certain groups of debugging outputs.
 *
 * \note These may be rather coarse, and the grouping may be impractical.
 * If you find, while debugging the driver, that a different grouping
 * of these flags would be beneficial, just feel free to change them
 * but make sure to update the documentation in r300_debug.c to reflect
 * those changes.
 */
/*@{*/
#define DBG_HELP    0x0000001
#define DBG_FP      0x0000002
#define DBG_VP      0x0000004
#define DBG_CS      0x0000008
#define DBG_DRAW    0x0000010
#define DBG_TEX     0x0000020
#define DBG_FALL    0x0000040
/*@}*/

static INLINE boolean SCREEN_DBG_ON(struct r300_screen * screen, unsigned flags)
{
    return (screen->debug & flags) ? TRUE : FALSE;
}

static INLINE void SCREEN_DBG(struct r300_screen * screen, unsigned flags,
                              const char * fmt, ...)
{
    if (SCREEN_DBG_ON(screen, flags)) {
        va_list va;
        va_start(va, fmt);
        debug_vprintf(fmt, va);
        va_end(va);
    }
}

void r300_init_debug(struct r300_screen* ctx);

#endif /* R300_SCREEN_H */

