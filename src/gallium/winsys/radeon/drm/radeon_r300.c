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

#include "radeon_r300.h"
#include "radeon_buffer.h"

#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"
#include "state_tracker/drm_driver.h"

#include "util/u_memory.h"

static unsigned get_pb_usage_from_create_flags(unsigned bind, unsigned usage,
                                               enum r300_buffer_domain domain)
{
    unsigned res = 0;

    if (bind & (PIPE_BIND_DEPTH_STENCIL | PIPE_BIND_RENDER_TARGET |
                PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT))
        res |= PB_USAGE_GPU_WRITE;

    if (bind & PIPE_BIND_SAMPLER_VIEW)
        res |= PB_USAGE_GPU_READ | PB_USAGE_GPU_WRITE;

    if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
        res |= PB_USAGE_GPU_READ;

    if (bind & PIPE_BIND_TRANSFER_WRITE)
        res |= PB_USAGE_CPU_WRITE;

    if (bind & PIPE_BIND_TRANSFER_READ)
        res |= PB_USAGE_CPU_READ;

    /* Is usage of any use for us? Probably not. */

    /* Now add driver-specific usage flags. */
    if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
        res |= RADEON_PB_USAGE_VERTEX;

    if (domain & R300_DOMAIN_GTT)
        res |= RADEON_PB_USAGE_DOMAIN_GTT;

    if (domain & R300_DOMAIN_VRAM)
        res |= RADEON_PB_USAGE_DOMAIN_VRAM;

    return res;
}

static struct r300_winsys_buffer *
radeon_r300_winsys_buffer_create(struct r300_winsys_screen *rws,
                                 unsigned size,
                                 unsigned alignment,
                                 unsigned bind,
                                 unsigned usage,
                                 enum r300_buffer_domain domain)
{
    struct radeon_libdrm_winsys *ws = radeon_libdrm_winsys(rws);
    struct pb_desc desc;
    struct pb_manager *provider;
    struct pb_buffer *buffer;

    memset(&desc, 0, sizeof(desc));
    desc.alignment = alignment;
    desc.usage = get_pb_usage_from_create_flags(bind, usage, domain);

    /* Assign a buffer manager. */
    if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
	provider = ws->cman;
    else
        provider = ws->kman;

    buffer = provider->create_buffer(provider, size, &desc);
    if (!buffer)
	return NULL;

    return radeon_libdrm_winsys_buffer(buffer);
}

static void radeon_r300_winsys_buffer_reference(struct r300_winsys_screen *rws,
						struct r300_winsys_buffer **pdst,
						struct r300_winsys_buffer *src)
{
    struct pb_buffer *_src = radeon_pb_buffer(src);
    struct pb_buffer *_dst = radeon_pb_buffer(*pdst);

    pb_reference(&_dst, _src);

    *pdst = radeon_libdrm_winsys_buffer(_dst);
}

static struct r300_winsys_buffer *radeon_r300_winsys_buffer_from_handle(struct r300_winsys_screen *rws,
                                                                        struct winsys_handle *whandle,
                                                                        unsigned *stride,
                                                                        unsigned *size)
{
    struct radeon_libdrm_winsys *ws = radeon_libdrm_winsys(rws);
    struct pb_buffer *_buf;

    _buf = radeon_drm_bufmgr_create_buffer_from_handle(ws->kman, whandle->handle);

    if (stride)
        *stride = whandle->stride;
    if (size)
        *size = _buf->base.size;

    return radeon_libdrm_winsys_buffer(_buf);
}

static boolean radeon_r300_winsys_buffer_get_handle(struct r300_winsys_screen *rws,
						    struct r300_winsys_buffer *buffer,
                                                    unsigned stride,
                                                    struct winsys_handle *whandle)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buffer);
    whandle->stride = stride;
    return radeon_drm_bufmgr_get_handle(_buf, whandle);
}

static void radeon_r300_winsys_cs_set_flush(struct r300_winsys_cs *rcs,
                                            void (*flush)(void *),
                                            void *user)
{
    struct radeon_libdrm_cs *cs = radeon_libdrm_cs(rcs);
    cs->flush_cs = flush;
    cs->flush_data = user;
    radeon_cs_space_set_flush(cs->cs, flush, user);
}

static boolean radeon_r300_winsys_cs_validate(struct r300_winsys_cs *rcs)
{
    struct radeon_libdrm_cs *cs = radeon_libdrm_cs(rcs);

    return radeon_cs_space_check(cs->cs) >= 0;
}

static void radeon_r300_winsys_cs_reset_buffers(struct r300_winsys_cs *rcs)
{
    struct radeon_libdrm_cs *cs = radeon_libdrm_cs(rcs);
    radeon_cs_space_reset_bos(cs->cs);
}

static void radeon_r300_winsys_cs_flush(struct r300_winsys_cs *rcs)
{
    struct radeon_libdrm_cs *cs = radeon_libdrm_cs(rcs);
    int retval;

    /* Don't flush a zero-sized CS. */
    if (!cs->base.cdw) {
        return;
    }

    cs->cs->cdw = cs->base.cdw;

    radeon_drm_bufmgr_flush_maps(cs->ws->kman);

    /* Emit the CS. */
    retval = radeon_cs_emit(cs->cs);
    if (retval) {
        if (debug_get_bool_option("RADEON_DUMP_CS", FALSE)) {
            fprintf(stderr, "radeon: The kernel rejected CS, dumping...\n");
            radeon_cs_print(cs->cs, stderr);
        } else {
            fprintf(stderr, "radeon: The kernel rejected CS, "
                            "see dmesg for more information.\n");
        }
    }

    /* Reset CS.
     * Someday, when we care about performance, we should really find a way
     * to rotate between two or three CS objects so that the GPU can be
     * spinning through one CS while another one is being filled. */
    radeon_cs_erase(cs->cs);

    cs->base.ptr = cs->cs->packets;
    cs->base.cdw = cs->cs->cdw;
    cs->base.ndw = cs->cs->ndw;
}

static uint32_t radeon_get_value(struct r300_winsys_screen *rws,
                                 enum r300_value_id id)
{
    struct radeon_libdrm_winsys *ws = (struct radeon_libdrm_winsys *)rws;

    switch(id) {
    case R300_VID_PCI_ID:
	return ws->pci_id;
    case R300_VID_GB_PIPES:
	return ws->gb_pipes;
    case R300_VID_Z_PIPES:
	return ws->z_pipes;
    case R300_VID_SQUARE_TILING_SUPPORT:
        return ws->squaretiling;
    case R300_VID_DRM_2_3_0:
        return ws->drm_2_3_0;
    case R300_VID_DRM_2_6_0:
        return ws->drm_2_6_0;
    case R300_CAN_HYPERZ:
        return ws->hyperz;
    }
    return 0;
}

static struct r300_winsys_cs *radeon_r300_winsys_cs_create(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = radeon_libdrm_winsys(rws);
    struct radeon_libdrm_cs *cs = CALLOC_STRUCT(radeon_libdrm_cs);

    if (!cs)
        return NULL;

    /* Size limit on IBs is 64 kibibytes. */
    cs->cs = radeon_cs_create(ws->csm, 1024 * 64 / 4);
    if (!cs->cs) {
        FREE(cs);
        return NULL;
    }

    radeon_cs_set_limit(cs->cs,
            RADEON_GEM_DOMAIN_GTT, ws->gart_size);
    radeon_cs_set_limit(cs->cs,
            RADEON_GEM_DOMAIN_VRAM, ws->vram_size);

    cs->ws = ws;
    cs->base.ptr = cs->cs->packets;
    cs->base.cdw = cs->cs->cdw;
    cs->base.ndw = cs->cs->ndw;
    return &cs->base;
}

static void radeon_r300_winsys_cs_destroy(struct r300_winsys_cs *rcs)
{
    struct radeon_libdrm_cs *cs = radeon_libdrm_cs(rcs);
    radeon_cs_destroy(cs->cs);
    FREE(cs);
}

static void radeon_winsys_destroy(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = (struct radeon_libdrm_winsys *)rws;

    ws->cman->destroy(ws->cman);
    ws->kman->destroy(ws->kman);

    radeon_bo_manager_gem_dtor(ws->bom);
    radeon_cs_manager_gem_dtor(ws->csm);

    FREE(rws);
}

boolean radeon_setup_winsys(int fd, struct radeon_libdrm_winsys* ws)
{
    ws->csm = radeon_cs_manager_gem_ctor(fd);
    if (!ws->csm)
	goto fail;
    ws->bom = radeon_bo_manager_gem_ctor(fd);
    if (!ws->bom)
	goto fail;
    ws->kman = radeon_drm_bufmgr_create(ws);
    if (!ws->kman)
	goto fail;

    ws->cman = pb_cache_manager_create(ws->kman, 1000000);
    if (!ws->cman)
	goto fail;

    ws->base.destroy = radeon_winsys_destroy;
    ws->base.get_value = radeon_get_value;

    ws->base.buffer_create = radeon_r300_winsys_buffer_create;
    ws->base.buffer_set_tiling = radeon_drm_bufmgr_set_tiling;
    ws->base.buffer_get_tiling = radeon_drm_bufmgr_get_tiling;
    ws->base.buffer_map = radeon_drm_buffer_map;
    ws->base.buffer_unmap = radeon_drm_buffer_unmap;
    ws->base.buffer_wait = radeon_drm_bufmgr_wait;
    ws->base.buffer_reference = radeon_r300_winsys_buffer_reference;
    ws->base.buffer_from_handle = radeon_r300_winsys_buffer_from_handle;
    ws->base.buffer_get_handle = radeon_r300_winsys_buffer_get_handle;

    ws->base.cs_create = radeon_r300_winsys_cs_create;
    ws->base.cs_destroy = radeon_r300_winsys_cs_destroy;
    ws->base.cs_add_buffer = radeon_drm_bufmgr_add_buffer;
    ws->base.cs_validate = radeon_r300_winsys_cs_validate;
    ws->base.cs_write_reloc = radeon_drm_bufmgr_write_reloc;
    ws->base.cs_flush = radeon_r300_winsys_cs_flush;
    ws->base.cs_reset_buffers = radeon_r300_winsys_cs_reset_buffers;
    ws->base.cs_set_flush = radeon_r300_winsys_cs_set_flush;
    ws->base.cs_is_buffer_referenced = radeon_drm_bufmgr_is_buffer_referenced;
    return TRUE;

fail:
    if (ws->csm)
	radeon_cs_manager_gem_dtor(ws->csm);

    if (ws->bom)
	radeon_bo_manager_gem_dtor(ws->bom);

    if (ws->cman)
	ws->cman->destroy(ws->cman);
    if (ws->kman)
	ws->kman->destroy(ws->kman);

    return FALSE;
}
