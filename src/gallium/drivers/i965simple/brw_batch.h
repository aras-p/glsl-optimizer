/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef BRW_BATCH_H
#define BRW_BATCH_H

#include "brw_winsys.h"

#define BATCH_LOCALS

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

#define BEGIN_BATCH( dwords, relocs ) \
   brw->winsys->batch_start(brw->winsys, dwords, relocs)

#define OUT_BATCH( dword ) \
   brw->winsys->batch_dword(brw->winsys, dword)

#define OUT_RELOC( buf, flags, delta ) \
   brw->winsys->batch_reloc(brw->winsys, buf, flags, delta)

#define ADVANCE_BATCH() \
   brw->winsys->batch_end( brw->winsys )

/* XXX: this is bogus - need proper handling for out-of-memory in batchbuffer.
 */
#define FLUSH_BATCH(fence) do {				\
   brw->winsys->batch_flush(brw->winsys, fence);	\
   brw->hardware_dirty = ~0;				\
} while (0)

#define BRW_BATCH_STRUCT(brw, s) brw_batchbuffer_data( brw->winsys, (s), sizeof(*(s)))

#endif
