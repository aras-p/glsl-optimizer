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

static struct r300_winsys_buffer *
radeon_r300_winsys_buffer_create(struct r300_winsys_screen *rws,
				 unsigned alignment,
				 unsigned usage,
                                 enum r300_buffer_domain domain,
				 unsigned size)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    struct pb_desc desc;
    struct pb_manager *provider;
    struct pb_buffer *buffer;

    /* XXX this is hackish, but it's the only way to pass these flags
     * to the real create function. */
    usage &= ~(RADEON_USAGE_DOMAIN_GTT | RADEON_USAGE_DOMAIN_VRAM);
    if (domain & R300_DOMAIN_GTT)
        usage |= RADEON_USAGE_DOMAIN_GTT;
    if (domain & R300_DOMAIN_VRAM)
        usage |= RADEON_USAGE_DOMAIN_VRAM;

    memset(&desc, 0, sizeof(desc));
    desc.alignment = alignment;
    desc.usage = usage;

    if (usage & PIPE_BIND_CONSTANT_BUFFER)
        provider = ws->mman;
    else if ((usage & PIPE_BIND_VERTEX_BUFFER) ||
	     (usage & PIPE_BIND_INDEX_BUFFER))
	provider = ws->cman;
    else
        provider = ws->kman;
    buffer = provider->create_buffer(provider, size, &desc);
    if (!buffer)
	return NULL;

    return radeon_libdrm_winsys_buffer(buffer);
}

static void radeon_r300_winsys_buffer_destroy(struct r300_winsys_buffer *buf)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    pb_destroy(_buf);
}
static void radeon_r300_winsys_buffer_set_tiling(struct r300_winsys_screen *rws,
						  struct r300_winsys_buffer *buf,
						  uint32_t pitch,
						  enum r300_buffer_tiling microtiled,
						  enum r300_buffer_tiling macrotiled)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);
    radeon_drm_bufmgr_set_tiling(_buf, microtiled, macrotiled, pitch);
}

static void radeon_r300_winsys_buffer_get_tiling(struct r300_winsys_screen *rws,
						  struct r300_winsys_buffer *buf,
						  enum r300_buffer_tiling *microtiled,
						  enum r300_buffer_tiling *macrotiled)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);
    radeon_drm_bufmgr_get_tiling(_buf, microtiled, macrotiled);
}

static void *radeon_r300_winsys_buffer_map(struct r300_winsys_screen *ws,
					   struct r300_winsys_buffer *buf,
					   unsigned usage)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    return pb_map(_buf, usage);
}

static void radeon_r300_winsys_buffer_unmap(struct r300_winsys_screen *ws,
					    struct r300_winsys_buffer *buf)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    pb_unmap(_buf);
}

static void radeon_r300_winsys_buffer_wait(struct r300_winsys_screen *ws,
                                           struct r300_winsys_buffer *buf)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);
    radeon_drm_bufmgr_wait(_buf);
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

static boolean radeon_r300_winsys_is_buffer_referenced(struct r300_winsys_screen *rws,
						       struct r300_winsys_buffer *buf,
                                                       enum r300_reference_domain domain)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    return radeon_drm_bufmgr_is_buffer_referenced(_buf, domain);
}

static struct r300_winsys_buffer *radeon_r300_winsys_buffer_from_handle(struct r300_winsys_screen *rws,
                                                                        struct winsys_handle *whandle,
                                                                        unsigned *stride)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    struct pb_buffer *_buf;

    *stride = whandle->stride;

    _buf = radeon_drm_bufmgr_create_buffer_from_handle(ws->kman, whandle->handle);
    return radeon_libdrm_winsys_buffer(_buf);
}

static boolean radeon_r300_winsys_buffer_get_handle(struct r300_winsys_screen *rws,
						    struct r300_winsys_buffer *buffer,
                                                    struct winsys_handle *whandle,
                                                    unsigned stride)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buffer);
    whandle->stride = stride;
    return radeon_drm_bufmgr_get_handle(_buf, whandle);
}

static void radeon_set_flush_cb(struct r300_winsys_screen *rws,
                                void (*flush_cb)(void *),
                                void *data)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    ws->flush_cb = flush_cb;
    ws->flush_data = data;
    radeon_cs_space_set_flush(ws->cs, flush_cb, data);
}

static boolean radeon_add_buffer(struct r300_winsys_screen *rws,
                                 struct r300_winsys_buffer *buf,
                                 enum r300_buffer_domain rd,
                                 enum r300_buffer_domain wd)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    return radeon_drm_bufmgr_add_buffer(_buf, rd, wd);
}

static boolean radeon_validate(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    if (radeon_cs_space_check(ws->cs) < 0) {
        return FALSE;
    }

    /* Things are fine, we can proceed as normal. */
    return TRUE;
}

static unsigned radeon_get_cs_free_dwords(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    struct radeon_cs *cs = ws->cs;

    return cs->ndw - cs->cdw;
}

static uint32_t *radeon_get_cs_pointer(struct r300_winsys_screen *rws,
                                       unsigned count)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    struct radeon_cs *cs = ws->cs;
    uint32_t *ptr = cs->packets + cs->cdw;

    cs->cdw += count;
    return ptr;
}

static void radeon_write_cs_dword(struct r300_winsys_screen *rws,
                                  uint32_t dword)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    radeon_cs_write_dword(ws->cs, dword);
}

static void radeon_write_cs_table(struct r300_winsys_screen *rws,
                                  const void *table, unsigned count)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    radeon_cs_write_table(ws->cs, table, count);
}

static void radeon_write_cs_reloc(struct r300_winsys_screen *rws,
                                  struct r300_winsys_buffer *buf,
                                  enum r300_buffer_domain rd,
                                  enum r300_buffer_domain wd,
                                  uint32_t flags)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);
    radeon_drm_bufmgr_write_reloc(_buf, rd, wd, flags);
}

static void radeon_reset_bos(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    radeon_cs_space_reset_bos(ws->cs);
}

static void radeon_flush_cs(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = radeon_winsys_screen(rws);
    int retval;

    /* Don't flush a zero-sized CS. */
    if (!ws->cs->cdw) {
        return;
    }

    radeon_drm_bufmgr_flush_maps(ws->kman);
    /* Emit the CS. */
    retval = radeon_cs_emit(ws->cs);
    if (retval) {
        if (debug_get_bool_option("RADEON_DUMP_CS", FALSE)) {
            fprintf(stderr, "radeon: The kernel rejected CS, dumping...\n");
            radeon_cs_print(ws->cs, stderr);
        } else {
            fprintf(stderr, "radeon: The kernel rejected CS, "
                            "see dmesg for more information.\n");
        }
    }

    /* Reset CS.
     * Someday, when we care about performance, we should really find a way
     * to rotate between two or three CS objects so that the GPU can be
     * spinning through one CS while another one is being filled. */
    radeon_cs_erase(ws->cs);
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
    }
    return 0;
}

static void
radeon_winsys_destroy(struct r300_winsys_screen *rws)
{
    struct radeon_libdrm_winsys *ws = (struct radeon_libdrm_winsys *)rws;
    radeon_cs_destroy(ws->cs);

    ws->cman->destroy(ws->cman);
    ws->kman->destroy(ws->kman);
    ws->mman->destroy(ws->mman);

    radeon_bo_manager_gem_dtor(ws->bom);
    radeon_cs_manager_gem_dtor(ws->csm);
}

boolean
radeon_setup_winsys(int fd, struct radeon_libdrm_winsys* ws)
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

    ws->cman = pb_cache_manager_create(ws->kman, 100000);
    if (!ws->cman)
	goto fail;

    ws->mman = pb_malloc_bufmgr_create();
    if (!ws->mman)
	goto fail;

    /* Size limit on IBs is 64 kibibytes. */
    ws->cs = radeon_cs_create(ws->csm, 1024 * 64 / 4);
    if (!ws->cs)
	goto fail;
    radeon_cs_set_limit(ws->cs,
            RADEON_GEM_DOMAIN_GTT, ws->gart_size);
    radeon_cs_set_limit(ws->cs,
            RADEON_GEM_DOMAIN_VRAM, ws->vram_size);

    ws->base.add_buffer = radeon_add_buffer;
    ws->base.validate = radeon_validate;
    ws->base.destroy = radeon_winsys_destroy;
    ws->base.get_cs_free_dwords = radeon_get_cs_free_dwords;
    ws->base.get_cs_pointer = radeon_get_cs_pointer;
    ws->base.write_cs_dword = radeon_write_cs_dword;
    ws->base.write_cs_table = radeon_write_cs_table;
    ws->base.write_cs_reloc = radeon_write_cs_reloc;
    ws->base.flush_cs = radeon_flush_cs;
    ws->base.reset_bos = radeon_reset_bos;
    ws->base.set_flush_cb = radeon_set_flush_cb;
    ws->base.get_value = radeon_get_value;

    ws->base.buffer_create = radeon_r300_winsys_buffer_create;
    ws->base.buffer_destroy = radeon_r300_winsys_buffer_destroy;
    ws->base.buffer_set_tiling = radeon_r300_winsys_buffer_set_tiling;
    ws->base.buffer_get_tiling = radeon_r300_winsys_buffer_get_tiling;
    ws->base.buffer_map = radeon_r300_winsys_buffer_map;
    ws->base.buffer_unmap = radeon_r300_winsys_buffer_unmap;
    ws->base.buffer_wait = radeon_r300_winsys_buffer_wait;
    ws->base.buffer_reference = radeon_r300_winsys_buffer_reference;
    ws->base.buffer_from_handle = radeon_r300_winsys_buffer_from_handle;
    ws->base.buffer_get_handle = radeon_r300_winsys_buffer_get_handle;
    ws->base.is_buffer_referenced = radeon_r300_winsys_is_buffer_referenced;
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
    if (ws->mman)
	ws->mman->destroy(ws->mman);

    if (ws->cs)
	radeon_cs_destroy(ws->cs);
    return FALSE;
}
