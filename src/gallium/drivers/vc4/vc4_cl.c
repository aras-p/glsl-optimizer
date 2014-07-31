/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "util/u_math.h"
#include "vc4_context.h"

void
vc4_init_cl(struct vc4_context *vc4, struct vc4_cl *cl)
{
}

void
vc4_grow_cl(struct vc4_cl *cl)
{
        uint32_t size = MAX2((cl->end - cl->base) * 2, 4096);
        uint32_t offset = cl->next -cl->base;

        cl->base = realloc(cl->base, size);
        cl->end = cl->base + size;
        cl->next = cl->base + offset;
}

void
vc4_reset_cl(struct vc4_cl *cl)
{
        assert(cl->reloc_count == 0);
        cl->next = cl->base;
}

uint32_t
vc4_gem_hindex(struct vc4_context *vc4, struct vc4_bo *bo)
{
        uint32_t hindex;
        uint32_t *current_handles = vc4->bo_handles.base;

        for (hindex = 0;
             hindex < (vc4->bo_handles.next - vc4->bo_handles.base) / 4;
             hindex++) {
                if (current_handles[hindex] == bo->handle)
                        return hindex;
        }

        cl_u32(&vc4->bo_handles, bo->handle);
        cl_ptr(&vc4->bo_pointers, vc4_bo_reference(bo));

        return hindex;
}
