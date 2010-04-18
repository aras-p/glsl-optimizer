/* 
 * Copyright © 2008 Nicolai Haehnle
 * Copyright © 2008 Jérôme Glisse
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Aapo Tahkola <aet@rasterburn.org>
 *      Nicolai Haehnle <prefect_@gmx.net>
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include "drm.h"
#include "radeon_drm.h"

#include "radeon_bocs_wrapper.h"
#include "radeon_common.h"
#ifdef HAVE_LIBDRM_RADEON
#include "radeon_cs_int.h"
#else
#include "radeon_cs_int_drm.h"
#endif
struct cs_manager_legacy {
    struct radeon_cs_manager    base;
    struct radeon_context       *ctx;
    /* hack for scratch stuff */
    uint32_t                    pending_age;
    uint32_t                    pending_count;


};

struct cs_reloc_legacy {
    struct radeon_cs_reloc  base;
    uint32_t                cindices;
    uint32_t                *indices;
};


static struct radeon_cs_int *cs_create(struct radeon_cs_manager *csm,
				       uint32_t ndw)
{
    struct radeon_cs_int *csi;

    csi = (struct radeon_cs_int*)calloc(1, sizeof(struct radeon_cs_int));
    if (csi == NULL) {
        return NULL;
    }
    csi->csm = csm;
    csi->ndw = (ndw + 0x3FF) & (~0x3FF);
    csi->packets = (uint32_t*)malloc(4*csi->ndw);
    if (csi->packets == NULL) {
        free(csi);
        return NULL;
    }
    csi->relocs_total_size = 0;
    return csi;
}

static int cs_write_reloc(struct radeon_cs_int *cs,
                          struct radeon_bo *bo,
                          uint32_t read_domain,
                          uint32_t write_domain,
                          uint32_t flags)
{
    struct cs_reloc_legacy *relocs;
    int i;

    relocs = (struct cs_reloc_legacy *)cs->relocs;
    /* check domains */
    if ((read_domain && write_domain) || (!read_domain && !write_domain)) {
        /* in one CS a bo can only be in read or write domain but not
         * in read & write domain at the same sime
         */
        return -EINVAL;
    }
    if (read_domain == RADEON_GEM_DOMAIN_CPU) {
        return -EINVAL;
    }
    if (write_domain == RADEON_GEM_DOMAIN_CPU) {
        return -EINVAL;
    }
    /* check if bo is already referenced */
    for(i = 0; i < cs->crelocs; i++) {
        uint32_t *indices;

        if (relocs[i].base.bo->handle == bo->handle) {
            /* Check domains must be in read or write. As we check already
             * checked that in argument one of the read or write domain was
             * set we only need to check that if previous reloc as the read
             * domain set then the read_domain should also be set for this
             * new relocation.
             */
            if (relocs[i].base.read_domain && !read_domain) {
                return -EINVAL;
            }
            if (relocs[i].base.write_domain && !write_domain) {
                return -EINVAL;
            }
            relocs[i].base.read_domain |= read_domain;
            relocs[i].base.write_domain |= write_domain;
            /* save indice */
            relocs[i].cindices++;
            indices = (uint32_t*)realloc(relocs[i].indices,
                                         relocs[i].cindices * 4);
            if (indices == NULL) {
                relocs[i].cindices -= 1;
                return -ENOMEM;
            }
            relocs[i].indices = indices;
            relocs[i].indices[relocs[i].cindices - 1] = cs->cdw - 1;
            return 0;
        }
    }
    /* add bo to reloc */
    relocs = (struct cs_reloc_legacy*)
             realloc(cs->relocs,
                     sizeof(struct cs_reloc_legacy) * (cs->crelocs + 1));
    if (relocs == NULL) {
        return -ENOMEM;
    }
    cs->relocs = relocs;
    relocs[cs->crelocs].base.bo = bo;
    relocs[cs->crelocs].base.read_domain = read_domain;
    relocs[cs->crelocs].base.write_domain = write_domain;
    relocs[cs->crelocs].base.flags = flags;
    relocs[cs->crelocs].indices = (uint32_t*)malloc(4);
    if (relocs[cs->crelocs].indices == NULL) {
        return -ENOMEM;
    }
    relocs[cs->crelocs].indices[0] = cs->cdw - 1;
    relocs[cs->crelocs].cindices = 1;
    cs->relocs_total_size += radeon_bo_legacy_relocs_size(bo);
    cs->crelocs++;
    radeon_bo_ref(bo);
    return 0;
}

static int cs_begin(struct radeon_cs_int *cs,
                    uint32_t ndw,
                    const char *file,
                    const char *func,
                    int line)
{
    if (cs->section_ndw) {
        fprintf(stderr, "CS already in a section(%s,%s,%d)\n",
                cs->section_file, cs->section_func, cs->section_line);
        fprintf(stderr, "CS can't start section(%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }
    cs->section_ndw = ndw;
    cs->section_cdw = 0;
    cs->section_file = file;
    cs->section_func = func;
    cs->section_line = line;


    if (cs->cdw + ndw > cs->ndw) {
        uint32_t tmp, *ptr;

        tmp = (cs->cdw + ndw + 0x3ff) & (~0x3ff);
        ptr = (uint32_t*)realloc(cs->packets, 4 * tmp);
        if (ptr == NULL) {
            return -ENOMEM;
        }
        cs->packets = ptr;
        cs->ndw = tmp;
    }

    return 0;
}

static int cs_end(struct radeon_cs_int *cs,
                  const char *file,
                  const char *func,
                  int line)

{
    if (!cs->section_ndw) {
        fprintf(stderr, "CS no section to end at (%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }
    if (cs->section_ndw != cs->section_cdw) {
        fprintf(stderr, "CS section size missmatch start at (%s,%s,%d) %d vs %d\n",
                cs->section_file, cs->section_func, cs->section_line, cs->section_ndw, cs->section_cdw);
        fprintf(stderr, "CS section end at (%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }
    cs->section_ndw = 0;

    return 0;
}

static int cs_process_relocs(struct radeon_cs_int *cs)
{
    struct cs_manager_legacy *csm = (struct cs_manager_legacy*)cs->csm;
    struct cs_reloc_legacy *relocs;
    int i, j, r;

    csm = (struct cs_manager_legacy*)cs->csm;
    relocs = (struct cs_reloc_legacy *)cs->relocs;
restart:
    for (i = 0; i < cs->crelocs; i++) 
    {
        for (j = 0; j < relocs[i].cindices; j++) 
        {
            uint32_t soffset, eoffset;

            r = radeon_bo_legacy_validate(relocs[i].base.bo,
                                           &soffset, &eoffset);
	        if (r == -EAGAIN)
            {
	             goto restart;
            }
            if (r) 
            {
                fprintf(stderr, "validated %p [0x%08X, 0x%08X]\n",
                        relocs[i].base.bo, soffset, eoffset);
                return r;
            }
            cs->packets[relocs[i].indices[j]] += soffset;
            if (cs->packets[relocs[i].indices[j]] >= eoffset) 
            {
	      /*                radeon_bo_debug(relocs[i].base.bo, 12); */
                fprintf(stderr, "validated %p [0x%08X, 0x%08X]\n",
                        relocs[i].base.bo, soffset, eoffset);
                fprintf(stderr, "above end: %p 0x%08X 0x%08X\n",
                        relocs[i].base.bo,
                        cs->packets[relocs[i].indices[j]],
                        eoffset);
                exit(0);
                return -EINVAL;
            }
        }
    }
    return 0;
}

static int cs_set_age(struct radeon_cs_int *cs)
{
    struct cs_manager_legacy *csm = (struct cs_manager_legacy*)cs->csm;
    struct cs_reloc_legacy *relocs;
    int i;

    relocs = (struct cs_reloc_legacy *)cs->relocs;
    for (i = 0; i < cs->crelocs; i++) {
        radeon_bo_legacy_pending(relocs[i].base.bo, csm->pending_age);
        radeon_bo_unref(relocs[i].base.bo);
    }
    return 0;
}

static int cs_emit(struct radeon_cs_int *cs)
{
    struct cs_manager_legacy *csm = (struct cs_manager_legacy*)cs->csm;
    drm_radeon_cmd_buffer_t cmd;
    drm_r300_cmd_header_t age;
    uint64_t ull;
    int r;

    csm->ctx->vtbl.emit_cs_header((struct radeon_cs *)cs, csm->ctx);

    /* append buffer age */
    if ( IS_R300_CLASS(csm->ctx->radeonScreen) )
    { 
      age.scratch.cmd_type = R300_CMD_SCRATCH;
      /* Scratch register 2 corresponds to what radeonGetAge polls */
      csm->pending_age = 0;
      csm->pending_count = 1;
      ull = (uint64_t) (intptr_t) &csm->pending_age;
      age.scratch.reg = 2;
      age.scratch.n_bufs = 1;
      age.scratch.flags = 0;
      radeon_cs_write_dword((struct radeon_cs *)cs, age.u);
      radeon_cs_write_qword((struct radeon_cs *)cs, ull);
      radeon_cs_write_dword((struct radeon_cs *)cs, 0);
    }

    r = cs_process_relocs(cs);
    if (r) {
        return 0;
    }

    cmd.buf = (char *)cs->packets;
    cmd.bufsz = cs->cdw * 4;
    if (csm->ctx->state.scissor.enabled) {
        cmd.nbox = csm->ctx->state.scissor.numClipRects;
        cmd.boxes = (drm_clip_rect_t *) csm->ctx->state.scissor.pClipRects;
    } else {
        cmd.nbox = csm->ctx->numClipRects;
        cmd.boxes = (drm_clip_rect_t *) csm->ctx->pClipRects;
    }

    //dump_cmdbuf(cs);

    r = drmCommandWrite(cs->csm->fd, DRM_RADEON_CMDBUF, &cmd, sizeof(cmd));
    if (r) {
        return r;
    }
    if ((!IS_R300_CLASS(csm->ctx->radeonScreen)) &&
        (!IS_R600_CLASS(csm->ctx->radeonScreen))) { /* +r6/r7 : No irq for r6/r7 yet. */
	drm_radeon_irq_emit_t emit_cmd;
	emit_cmd.irq_seq = (int*)&csm->pending_age;
	r = drmCommandWriteRead(cs->csm->fd, DRM_RADEON_IRQ_EMIT, &emit_cmd, sizeof(emit_cmd));
	if (r) {
		return r;
	}
    }
    cs_set_age(cs);

    cs->csm->read_used = 0;
    cs->csm->vram_write_used = 0;
    cs->csm->gart_write_used = 0;
    return 0;
}

static void inline cs_free_reloc(void *relocs_p, int crelocs)
{
    struct cs_reloc_legacy *relocs = relocs_p;
    int i;
    if (!relocs_p)
      return;
    for (i = 0; i < crelocs; i++)
      free(relocs[i].indices);
}

static int cs_destroy(struct radeon_cs_int *cs)
{
    cs_free_reloc(cs->relocs, cs->crelocs);
    free(cs->relocs);
    free(cs->packets);
    free(cs);
    return 0;
}

static int cs_erase(struct radeon_cs_int *cs)
{
    cs_free_reloc(cs->relocs, cs->crelocs);
    free(cs->relocs);
    cs->relocs_total_size = 0;
    cs->relocs = NULL;
    cs->crelocs = 0;
    cs->cdw = 0;
    cs->section_ndw = 0;
    return 0;
}

static int cs_need_flush(struct radeon_cs_int *cs)
{
    /* this function used to flush when the BO usage got to
     * a certain size, now the higher levels handle this better */
    return 0;
}

static void cs_print(struct radeon_cs_int *cs, FILE *file)
{
}

static struct radeon_cs_funcs  radeon_cs_legacy_funcs = {
    cs_create,
    cs_write_reloc,
    cs_begin,
    cs_end,
    cs_emit,
    cs_destroy,
    cs_erase,
    cs_need_flush,
    cs_print,
};

struct radeon_cs_manager *radeon_cs_manager_legacy_ctor(struct radeon_context *ctx)
{
    struct cs_manager_legacy *csm;

    csm = (struct cs_manager_legacy*)
          calloc(1, sizeof(struct cs_manager_legacy));
    if (csm == NULL) {
        return NULL;
    }
    csm->base.funcs = &radeon_cs_legacy_funcs;
    csm->base.fd = ctx->dri.fd;
    csm->ctx = ctx;
    csm->pending_age = 1;
    return (struct radeon_cs_manager*)csm;
}

void radeon_cs_manager_legacy_dtor(struct radeon_cs_manager *csm)
{
    free(csm);
}

