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

static void radeon_r300_add_buffer(struct r300_winsys* winsys,
                                   struct pipe_buffer* pbuffer,
                                   uint32_t rd,
                                   uint32_t wd)
{
    int i;
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    struct radeon_cs_space_check* sc = priv->sc;
    struct radeon_bo* bo = ((struct radeon_pipe_buffer*)pbuffer)->bo;

    /* Check to see if this BO is already in line for validation;
     * find a slot for it otherwise. */
    for (i = 0; i < RADEON_MAX_BOS; i++) {
        if (sc[i].bo == bo) {
            return;
        } else if (sc[i].bo == NULL) {
            sc[i].bo = bo;
            sc[i].read_domains = rd;
            sc[i].write_domain = wd;
            priv->bo_count = i + 1;
            return;
        }
    }

    assert(FALSE && "Oh God too many BOs!");
}

static boolean radeon_r300_validate(struct r300_winsys* winsys)
{
    int retval;
    struct radeon_winsys_priv* priv =
        (struct radeon_winsys_priv*)winsys->radeon_winsys;
    struct radeon_cs_space_check* sc = priv->sc;

    retval = radeon_cs_space_check(priv->cs, sc, priv->bo_count);

    if (retval == RADEON_CS_SPACE_OP_TO_BIG) {
        /* We might as well HCF, since this is not going to fit in the card,
         * period. */
	exit(1);
    } else if (retval == RADEON_CS_SPACE_FLUSH) {
        /* We must flush before more rendering can commence. */
        return TRUE;
    }

    /* Things are fine, we can proceed as normal. */
    return FALSE;
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

    radeon_cs_write_reloc(priv->cs,
            ((struct radeon_pipe_buffer*)pbuffer)->bo, rd, wd, flags);
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
    int retval = 0;

    retval = radeon_cs_emit(priv->cs);
    if (retval) {
        debug_printf("radeon: Bad CS, dumping...\n");
        radeon_cs_print(priv->cs, stderr);
    }
    radeon_cs_erase(priv->cs);
}

/* Helper function to do the ioctls needed for setup and init. */
static void do_ioctls(struct r300_winsys* winsys, int fd)
{
    struct drm_radeon_gem_info info;
    drm_radeon_getparam_t gp;
    int target;
    int retval;

    gp.value = &target;

    /* First, get the number of pixel pipes */
    gp.param = RADEON_PARAM_NUM_GB_PIPES;
    retval = drmCommandWriteRead(fd, DRM_RADEON_GETPARAM, &gp, sizeof(gp));
    if (retval) {
        fprintf(stderr, "%s: Failed to get GB pipe count, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->gb_pipes = target;

    /* Then, get PCI ID */
    gp.param = RADEON_PARAM_DEVICE_ID;
    retval = drmCommandWriteRead(fd, DRM_RADEON_GETPARAM, &gp, sizeof(gp));
    if (retval) {
        fprintf(stderr, "%s: Failed to get PCI ID, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->pci_id = target;

    /* Finally, retrieve MM info */
    retval = drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO,
            &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get MM info, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->gart_size = info.gart_size;
    /* XXX */
    winsys->vram_size = info.vram_visible;
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

    memcpy(winsys, old_winsys, sizeof(struct radeon_winsys));

    return winsys;
}
