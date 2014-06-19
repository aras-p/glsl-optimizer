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

#ifdef USE_VC4_SIMULATOR

#include <stdio.h>

#include "vc4_screen.h"
#include "vc4_context.h"
#include "simpenrose/simpenrose.h"

void
vc4_simulator_flush(struct vc4_context *vc4, struct vc4_surface *csurf)
{
        struct vc4_resource *ctex = vc4_resource(csurf->base.texture);
        uint32_t winsys_stride = ctex->bo->simulator_winsys_stride;
        uint32_t sim_stride = ctex->slices[0].stride;
        uint32_t row_len = MIN2(sim_stride, winsys_stride);

        if (ctex->bo->simulator_winsys_map) {
#if 0
                fprintf(stderr, "%dx%d %d %d %d\n",
                        ctex->base.b.width0, ctex->base.b.height0,
                        winsys_stride,
                        sim_stride,
                        ctex->bo->size);
#endif

                for (int y = 0; y < ctex->base.b.height0; y++) {
                        memcpy(ctex->bo->map + y * sim_stride,
                               ctex->bo->simulator_winsys_map + y * winsys_stride,
                               row_len);
                }
        }

        simpenrose_do_binning(simpenrose_hw_addr(vc4->bcl.base),
                              simpenrose_hw_addr(vc4->bcl.next));
        simpenrose_do_rendering(simpenrose_hw_addr(vc4->rcl.base),
                                simpenrose_hw_addr(vc4->rcl.next));

        if (ctex->bo->simulator_winsys_map) {
                for (int y = 0; y < ctex->base.b.height0; y++) {
                        memcpy(ctex->bo->simulator_winsys_map + y * winsys_stride,
                               ctex->bo->map + y * sim_stride,
                               row_len);
                }
        }
}

void
vc4_simulator_init(struct vc4_screen *screen)
{
        simpenrose_init_hardware();
        screen->simulator_mem_base = simpenrose_get_mem_start();
        screen->simulator_mem_size = simpenrose_get_mem_size();
}

/**
 * Allocates GPU memory in the simulator's address space.
 *
 * We just allocate for the lifetime of the context now, but some day we'll
 * want an actual memory allocator at runtime.
 */
void *
vc4_simulator_alloc(struct vc4_screen *screen, uint32_t size)
{
        void *alloc = screen->simulator_mem_base + screen->simulator_mem_next;

        screen->simulator_mem_next += size;
        assert(screen->simulator_mem_next < screen->simulator_mem_size);
        screen->simulator_mem_next = align(screen->simulator_mem_next, 4096);

        return alloc;
}

#endif /* USE_VC4_SIMULATOR */
