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

static void radeon_r300_set_flush_cb(struct r300_winsys *winsys,
				     void (*flush_cb)(void *),
				     void *data)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;

    radeon_cs_space_set_flush(priv->cs, flush_cb,
			      data);
}

static boolean radeon_r300_add_buffer(struct r300_winsys* winsys,
                                      struct pipe_buffer* pbuffer,
                                      uint32_t rd,
                                      uint32_t wd)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    struct radeon_bo* bo = ((struct radeon_pipe_buffer*)pbuffer)->bo;

    radeon_cs_space_add_persistent_bo(priv->cs, bo, rd, wd);
    return TRUE;
}

static boolean radeon_r300_validate(struct r300_winsys* winsys)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;

    if (radeon_cs_space_check(priv->cs) < 0) {
        return FALSE;
    }

    /* Things are fine, we can proceed as normal. */
    return TRUE;
}

static boolean radeon_r300_check_cs(struct r300_winsys* winsys, int size)
{
    /* XXX check size here, lazy ass! */
    /* XXX also validate buffers */
    return TRUE;
}

static void radeon_r300_begin_cs(struct r300_winsys* winsys,
                                 int size,
                                 const char* file,
                                 const char* function,
                                 int line)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;

    radeon_cs_begin(priv->cs, size, file, function, line);
}

static void radeon_r300_write_cs_dword(struct r300_winsys* winsys,
                                       uint32_t dword)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;

    radeon_cs_write_dword(priv->cs, dword);
}

static void radeon_r300_write_cs_reloc(struct r300_winsys* winsys,
                                       struct pipe_buffer* pbuffer,
                                       uint32_t rd,
                                       uint32_t wd,
                                       uint32_t flags)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    int retval = 0;

    retval = radeon_cs_write_reloc(priv->cs,
            ((struct radeon_pipe_buffer*)pbuffer)->bo, rd, wd, flags);

    if (retval) {
        debug_printf("radeon: Relocation of %p (%d, %d, %d) failed!\n",
                pbuffer, rd, wd, flags);
    }
}

static void radeon_r300_reset_bos(struct r300_winsys *winsys)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    radeon_cs_space_reset_bos(priv->cs);
}

static void radeon_r300_end_cs(struct r300_winsys* winsys,
                               const char* file,
                               const char* function,
                               int line)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;

    radeon_cs_end(priv->cs, file, function, line);
}

static void radeon_r300_flush_cs(struct r300_winsys* winsys)
{
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    int retval;

    /* Emit the CS. */
    retval = radeon_cs_emit(priv->cs);
    if (retval) {
        debug_printf("radeon: Bad CS, dumping...\n");
        radeon_cs_print(priv->cs, stderr);
    }

    /* Reset CS.
     * Someday, when we care about performance, we should really find a way
     * to rotate between two or three CS objects so that the GPU can be
     * spinning through one CS while another one is being filled. */
    radeon_cs_erase(priv->cs);
}

/* Helper function to do the ioctls needed for setup and init. */
static void do_ioctls(struct r300_winsys* winsys, int fd)
{
    struct drm_radeon_gem_info gem_info = {0};
    struct drm_radeon_info info = {0};
    int target = 0;
    int retval;

    info.value = (unsigned long)&target;

    /* We do things in a specific order here.
     *
     * First, the PCI ID. This is essential and should return usable numbers
     * for all Radeons. If this fails, we probably got handed an FD for some
     * non-Radeon card.
     *
     * The GB and Z pipe requests should always succeed, but they might not
     * return sensical values for all chipsets, but that's alright because
     * the pipe drivers already know that.
     *
     * The GEM info is actually bogus on the kernel side, as well as our side
     * (see radeon_gem_info_ioctl in radeon_gem.c) but that's alright because
     * we don't actually use the info for anything yet.
     * XXX update the above when we can safely use vram_size instead of vram_visible */
    info.request = RADEON_INFO_DEVICE_ID;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get PCI ID, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->pci_id = target;

    info.request = RADEON_INFO_NUM_GB_PIPES;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get GB pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->gb_pipes = target;

    info.request = RADEON_INFO_NUM_Z_PIPES;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get Z pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->z_pipes = target;

    retval = drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO,
            &gem_info, sizeof(gem_info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get MM info, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->gart_size = gem_info.gart_size;
    /* XXX */
    winsys->vram_size = gem_info.vram_visible;
}

struct r300_winsys*
radeon_create_r300_winsys(int fd, struct radeon_winsys* old_winsys)
{
    struct r300_winsys* winsys = CALLOC_STRUCT(r300_winsys);
    struct radeon_winsys_priv* priv;

    if (winsys == NULL) {
        return NULL;
    }

    priv = old_winsys->priv;

    do_ioctls(winsys, fd);

    priv->csm = radeon_cs_manager_gem_ctor(fd);

    priv->cs = radeon_cs_create(priv->csm, 1024 * 64 / 4);
    radeon_cs_set_limit(priv->cs,
            RADEON_GEM_DOMAIN_GTT, winsys->gart_size);
    radeon_cs_set_limit(priv->cs,
            RADEON_GEM_DOMAIN_VRAM, winsys->vram_size);

    winsys->add_buffer = radeon_r300_add_buffer;
    winsys->validate = radeon_r300_validate;

    winsys->check_cs = radeon_r300_check_cs;
    winsys->begin_cs = radeon_r300_begin_cs;
    winsys->write_cs_dword = radeon_r300_write_cs_dword;
    winsys->write_cs_reloc = radeon_r300_write_cs_reloc;
    winsys->end_cs = radeon_r300_end_cs;
    winsys->flush_cs = radeon_r300_flush_cs;
    winsys->reset_bos = radeon_r300_reset_bos;
    winsys->set_flush_cb = radeon_r300_set_flush_cb;

    memcpy(winsys, old_winsys, sizeof(struct radeon_winsys));

    return winsys;
}
