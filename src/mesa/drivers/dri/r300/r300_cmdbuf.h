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

#define CACHE_FLUSH_BUFSZ      (4*2)
#define PRE_EMIT_STATE_BUFSZ   (2+2)
#define AOS_BUFSZ(nr)          (3+(nr >>1)*3 + (nr&1)*2 + (nr*2))
#define FIREAOS_BUFSZ          (3)
#define SCISSORS_BUFSZ         (3)

void r300InitCmdBuf(r300ContextPtr r300);
void r300_emit_scissor(GLcontext *ctx);

void r300_emit_vpu(struct r300_context *ctx,
                   uint32_t *data,
                   unsigned len,
                   uint32_t addr);

void r500_emit_fp(struct r300_context *r300,
                  uint32_t *data,
                  unsigned len,
                  uint32_t addr,
                  unsigned type,
                  unsigned clamp);

void r300_emit_cb_setup(struct r300_context *r300,
                        struct radeon_bo *bo,
                        uint32_t offset,
                        GLuint format,
                        unsigned cpp,
                        unsigned pitch);

#endif /* __R300_CMDBUF_H__ */
