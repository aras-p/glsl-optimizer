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

#include "util/u_memory.h"

#include "vc4_screen.h"
#include "vc4_context.h"
#include "kernel/vc4_drv.h"
#include "vc4_simulator_validate.h"
#include "simpenrose/simpenrose.h"

#define OVERFLOW_SIZE (32 * 1024 * 1024)

static struct drm_gem_cma_object *
vc4_wrap_bo_with_cma(struct drm_device *dev, struct vc4_bo *bo)
{
        struct vc4_context *vc4 = dev->vc4;
        struct vc4_screen *screen = vc4->screen;
        struct drm_gem_cma_object *obj = CALLOC_STRUCT(drm_gem_cma_object);
        uint32_t size = align(bo->size, 4096);

        obj->bo = bo;
        obj->base.size = size;
        obj->vaddr = screen->simulator_mem_base + dev->simulator_mem_next;
        obj->paddr = simpenrose_hw_addr(obj->vaddr);

        dev->simulator_mem_next += size;
        dev->simulator_mem_next = align(dev->simulator_mem_next, 4096);
        assert(dev->simulator_mem_next <= screen->simulator_mem_size);

        return obj;
}

struct drm_gem_cma_object *
drm_gem_cma_create(struct drm_device *dev, size_t size)
{
        struct vc4_context *vc4 = dev->vc4;
        struct vc4_screen *screen = vc4->screen;

        struct vc4_bo *bo = vc4_bo_alloc(screen, size, "simulator validate");
        return vc4_wrap_bo_with_cma(dev, bo);
}

static int
vc4_simulator_pin_bos(struct drm_device *dev, struct exec_info *exec)
{
        struct drm_vc4_submit_cl *args = exec->args;
        struct vc4_context *vc4 = dev->vc4;
        struct vc4_bo **bos = vc4->bo_pointers.base;

        exec->bo_count = args->bo_handle_count;
        exec->bo = calloc(exec->bo_count, sizeof(struct vc4_bo_exec_state));
        for (int i = 0; i < exec->bo_count; i++) {
                struct vc4_bo *bo = bos[i];
                struct drm_gem_cma_object *obj = vc4_wrap_bo_with_cma(dev, bo);

#if 0
                fprintf(stderr, "bo hindex %d: %s\n", i, bo->name);
#endif

                vc4_bo_map(bo);
                memcpy(obj->vaddr, bo->map, bo->size);

                exec->bo[i].bo = obj;
        }
        return 0;
}

static int
vc4_simulator_unpin_bos(struct exec_info *exec)
{
        for (int i = 0; i < exec->bo_count; i++) {
                struct drm_gem_cma_object *obj = exec->bo[i].bo;
                struct vc4_bo *bo = obj->bo;

                memcpy(bo->map, obj->vaddr, bo->size);

                free(obj);
        }

        free(exec->bo);

        return 0;
}

int
vc4_simulator_flush(struct vc4_context *vc4, struct drm_vc4_submit_cl *args)
{
        struct vc4_surface *csurf = vc4_surface(vc4->framebuffer.cbufs[0]);
        struct vc4_resource *ctex = csurf ? vc4_resource(csurf->base.texture) : NULL;
        uint32_t winsys_stride = ctex ? ctex->bo->simulator_winsys_stride : 0;
        uint32_t sim_stride = ctex ? ctex->slices[0].stride : 0;
        uint32_t row_len = MIN2(sim_stride, winsys_stride);
        struct exec_info exec;
        struct drm_device local_dev = {
                .vc4 = vc4,
                .simulator_mem_next = OVERFLOW_SIZE,
        };
        struct drm_device *dev = &local_dev;
        int ret;

        memset(&exec, 0, sizeof(exec));

        if (ctex && ctex->bo->simulator_winsys_map) {
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

        exec.args = args;

        ret = vc4_simulator_pin_bos(dev, &exec);
        if (ret)
                return ret;

        ret = vc4_cl_validate(dev, &exec);
        if (ret)
                return ret;

        simpenrose_do_binning(exec.ct0ca, exec.ct0ea);
        simpenrose_do_rendering(exec.ct1ca, exec.ct1ea);

        ret = vc4_simulator_unpin_bos(&exec);
        if (ret)
                return ret;

        free(exec.exec_bo);

        if (ctex && ctex->bo->simulator_winsys_map) {
                for (int y = 0; y < ctex->base.b.height0; y++) {
                        memcpy(ctex->bo->simulator_winsys_map + y * winsys_stride,
                               ctex->bo->map + y * sim_stride,
                               row_len);
                }
        }

        return 0;
}

void
vc4_simulator_init(struct vc4_screen *screen)
{
        screen->simulator_mem_size = 256 * 1024 * 1024;
        screen->simulator_mem_base = malloc(screen->simulator_mem_size);

        /* We supply our own memory so that we can have more aperture
         * available (256MB instead of simpenrose's default 64MB).
         */
        simpenrose_init_hardware_supply_mem(screen->simulator_mem_base,
                                            screen->simulator_mem_size);

        /* Carve out low memory for tile allocation overflow.  The kernel
         * should be automatically handling overflow memory setup on real
         * hardware, but for simulation we just get one shot to set up enough
         * overflow memory before execution.  This overflow mem will be used
         * up over the whole lifetime of simpenrose (not reused on each
         * flush), so it had better be big.
         */
        simpenrose_supply_overflow_mem(0, OVERFLOW_SIZE);
}

#endif /* USE_VC4_SIMULATOR */
