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
 * Mostly coppied from \radeon\radeon_cs_legacy.c
 */

#include <errno.h>

#include "main/glheader.h"
#include "main/state.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "swrast/swrast.h"

#include "drm.h"
#include "radeon_drm.h"

#include "r600_context.h"
#include "radeon_reg.h"
#include "r600_cmdbuf.h"
#include "r600_emit.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_mipmap_tree.h"
#include "radeon_reg.h"



static struct radeon_cs * r600_cs_create(struct radeon_cs_manager *csm,
                                   uint32_t ndw)
{
    struct radeon_cs *cs;

    cs = (struct radeon_cs*)calloc(1, sizeof(struct radeon_cs));
    if (cs == NULL) {
        return NULL;
    }
    cs->csm = csm;
    cs->ndw = (ndw + 0x3FF) & (~0x3FF);
    cs->packets = (uint32_t*)malloc(4*cs->ndw);
    if (cs->packets == NULL) {
        free(cs);
        return NULL;
    }
    cs->relocs_total_size = 0;
    return cs;
}

static int r600_cs_write_reloc(struct radeon_cs *cs,
			       struct radeon_bo *bo,
			       uint32_t read_domain,
			       uint32_t write_domain,
			       uint32_t flags)
{
    struct r600_cs_reloc_legacy *relocs;
    int i;

    relocs = (struct r600_cs_reloc_legacy *)cs->relocs;
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
        uint32_t *reloc_indices;

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
            reloc_indices = (uint32_t*)realloc(relocs[i].reloc_indices,
                                               relocs[i].cindices * 4);
            if ( (indices == NULL) || (reloc_indices == NULL) ) {
                relocs[i].cindices -= 1;
                return -ENOMEM;
            }
            relocs[i].indices = indices;
            relocs[i].reloc_indices = reloc_indices;
            relocs[i].indices[relocs[i].cindices - 1] = cs->cdw;
            relocs[i].reloc_indices[relocs[i].cindices - 1] = cs->cdw;
            cs->section_cdw += 2;
	    cs->cdw += 2;

            return 0;
        }
    }
    /* add bo to reloc */
    relocs = (struct r600_cs_reloc_legacy*)
             realloc(cs->relocs,
                     sizeof(struct r600_cs_reloc_legacy) * (cs->crelocs + 1));
    if (relocs == NULL) {
        return -ENOMEM;
    }
    cs->relocs = relocs;
    relocs[cs->crelocs].base.bo = bo;
    relocs[cs->crelocs].base.read_domain = read_domain;
    relocs[cs->crelocs].base.write_domain = write_domain;
    relocs[cs->crelocs].base.flags = flags;
    relocs[cs->crelocs].indices = (uint32_t*)malloc(4);
    relocs[cs->crelocs].reloc_indices = (uint32_t*)malloc(4);
    if ( (relocs[cs->crelocs].indices == NULL) || (relocs[cs->crelocs].reloc_indices == NULL) )
    {
        return -ENOMEM;
    }

    relocs[cs->crelocs].indices[0] = cs->cdw;
    relocs[cs->crelocs].reloc_indices[0] = cs->cdw;
    cs->section_cdw += 2;
    cs->cdw += 2;
    relocs[cs->crelocs].cindices = 1;
    cs->relocs_total_size += radeon_bo_legacy_relocs_size(bo);
    cs->crelocs++;

    radeon_bo_ref(bo);

    return 0;
}

static int r600_cs_begin(struct radeon_cs *cs,
                    uint32_t ndw,
                    const char *file,
                    const char *func,
                    int line)
{
    if (cs->section) {
        fprintf(stderr, "CS already in a section(%s,%s,%d)\n",
                cs->section_file, cs->section_func, cs->section_line);
        fprintf(stderr, "CS can't start section(%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }

    cs->section = 1;
    cs->section_ndw = ndw;
    cs->section_cdw = 0;
    cs->section_file = file;
    cs->section_func = func;
    cs->section_line = line;

    if (cs->cdw + ndw > cs->ndw) {
        uint32_t tmp, *ptr;
	int num = (ndw > 0x400) ? ndw : 0x400;

        tmp = (cs->cdw + num + 0x3FF) & (~0x3FF);
        ptr = (uint32_t*)realloc(cs->packets, 4 * tmp);
        if (ptr == NULL) {
            return -ENOMEM;
        }
        cs->packets = ptr;
        cs->ndw = tmp;
    }

    return 0;
}

static int r600_cs_end(struct radeon_cs *cs,
                  const char *file,
                  const char *func,
                  int line)

{
    if (!cs->section) {
        fprintf(stderr, "CS no section to end at (%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }
    cs->section = 0;

    if ( cs->section_ndw != cs->section_cdw ) {
        fprintf(stderr, "CS section size missmatch start at (%s,%s,%d) %d vs %d\n",
                cs->section_file, cs->section_func, cs->section_line, cs->section_ndw, cs->section_cdw);
        fprintf(stderr, "cs->section_ndw = %d, cs->cdw = %d, cs->section_cdw = %d \n",
                cs->section_ndw, cs->cdw, cs->section_cdw);
        fprintf(stderr, "CS section end at (%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }

    if (cs->cdw > cs->ndw) {
	    fprintf(stderr, "CS section overflow at (%s,%s,%d) cdw %d ndw %d\n",
		    cs->section_file, cs->section_func, cs->section_line,cs->cdw,cs->ndw);
	    fprintf(stderr, "CS section end at (%s,%s,%d)\n",
		    file, func, line);
	    assert(0);
    }

    return 0;
}

static int r600_cs_process_relocs(struct radeon_cs *cs, 
                                  uint32_t * reloc_chunk,
                                  uint32_t * length_dw_reloc_chunk) 
{
    struct r600_cs_manager_legacy *csm = (struct r600_cs_manager_legacy*)cs->csm;
    struct r600_cs_reloc_legacy *relocs;
    int i, j, r;

    uint32_t offset_dw = 0;

    csm = (struct r600_cs_manager_legacy*)cs->csm;
    relocs = (struct r600_cs_reloc_legacy *)cs->relocs;
restart:
    for (i = 0; i < cs->crelocs; i++) {
            uint32_t soffset, eoffset;

            r = radeon_bo_legacy_validate(relocs[i].base.bo,
					  &soffset, &eoffset);
	    if (r == -EAGAIN) {
		    goto restart;
            }
            if (r) {
		    fprintf(stderr, "invalid bo(%p) [0x%08X, 0x%08X]\n",
			    relocs[i].base.bo, soffset, eoffset);
		    return r;
            }

	    for (j = 0; j < relocs[i].cindices; j++) {
		    /* pkt3 nop header in ib chunk */
		    cs->packets[relocs[i].reloc_indices[j]] = 0xC0001000;
		    /* reloc index in ib chunk */
		    cs->packets[relocs[i].reloc_indices[j] + 1] = offset_dw;
	    }

	    /* asic offset in reloc chunk */ /* see alex drm r600_nomm_relocate */
	    reloc_chunk[offset_dw] = soffset;
	    reloc_chunk[offset_dw + 3] = 0;

	    offset_dw += 4;
    }

    *length_dw_reloc_chunk = offset_dw;

    return 0;
}

static int r600_cs_set_age(struct radeon_cs *cs) /* -------------- */
{
    struct r600_cs_manager_legacy *csm = (struct r600_cs_manager_legacy*)cs->csm;
    struct r600_cs_reloc_legacy *relocs;
    int i;

    relocs = (struct r600_cs_reloc_legacy *)cs->relocs;
    for (i = 0; i < cs->crelocs; i++) {
        radeon_bo_legacy_pending(relocs[i].base.bo, csm->pending_age);
        radeon_bo_unref(relocs[i].base.bo);
    }
    return 0;
}

#if 0
static void dump_cmdbuf(struct radeon_cs *cs)
{
	int i;
	fprintf(stderr,"--start--\n");
	for (i = 0; i < cs->cdw; i++){
		fprintf(stderr,"0x%08x\n", cs->packets[i]);
	}
	fprintf(stderr,"--end--\n");

}
#endif

static int r600_cs_emit(struct radeon_cs *cs)
{
    struct r600_cs_manager_legacy *csm = (struct r600_cs_manager_legacy*)cs->csm;
    struct drm_radeon_cs       cs_cmd;
    struct drm_radeon_cs_chunk cs_chunk[2];
    uint32_t length_dw_reloc_chunk;
    uint64_t chunk_ptrs[2];
    uint32_t *reloc_chunk;
    int r;
    int retry = 0;

    /* TODO : put chip level things here if need. */
    /* csm->ctx->vtbl.emit_cs_header(cs, csm->ctx); */

    csm->pending_count = 1;

    reloc_chunk = (uint32_t*)calloc(1, cs->crelocs * 4 * 4);

    r = r600_cs_process_relocs(cs, reloc_chunk, &length_dw_reloc_chunk);
    if (r) {
	free(reloc_chunk);
        return 0;
    }

    /* raw ib chunk */
    cs_chunk[0].chunk_id   = RADEON_CHUNK_ID_IB;
    cs_chunk[0].length_dw  = cs->cdw;
    cs_chunk[0].chunk_data = (unsigned long)(cs->packets);

    /* reloc chaunk */
    cs_chunk[1].chunk_id   = RADEON_CHUNK_ID_RELOCS;
    cs_chunk[1].length_dw  = length_dw_reloc_chunk;
    cs_chunk[1].chunk_data = (unsigned long)reloc_chunk;

    chunk_ptrs[0] = (uint64_t)(unsigned long)&(cs_chunk[0]);
    chunk_ptrs[1] = (uint64_t)(unsigned long)&(cs_chunk[1]);

    cs_cmd.num_chunks = 2;
    /* cs_cmd.cs_id      = 0; */
    cs_cmd.chunks     = (uint64_t)(unsigned long)chunk_ptrs;

    //dump_cmdbuf(cs);

    do 
    {
        r = drmCommandWriteRead(cs->csm->fd, DRM_RADEON_CS, &cs_cmd, sizeof(cs_cmd));
        retry++;
    } while (r == -EAGAIN && retry < 1000);

    if (r) {
	free(reloc_chunk);
        return r;
    }

    csm->pending_age = cs_cmd.cs_id;

    r600_cs_set_age(cs);

    cs->csm->read_used = 0;
    cs->csm->vram_write_used = 0;
    cs->csm->gart_write_used = 0;

    free(reloc_chunk);

    return 0;
}

static void inline r600_cs_free_reloc(void *relocs_p, int crelocs)
{
    struct r600_cs_reloc_legacy *relocs = relocs_p;
    int i;
    if (!relocs_p)
      return;
    for (i = 0; i < crelocs; i++)
    {
        free(relocs[i].indices);
        free(relocs[i].reloc_indices);
    }
}

static int r600_cs_destroy(struct radeon_cs *cs)
{
    r600_cs_free_reloc(cs->relocs, cs->crelocs);
    free(cs->relocs);
    free(cs->packets);
    free(cs);
    return 0;
}

static int r600_cs_erase(struct radeon_cs *cs)
{
    r600_cs_free_reloc(cs->relocs, cs->crelocs);
    free(cs->relocs);
    cs->relocs_total_size = 0;
    cs->relocs = NULL;
    cs->crelocs = 0;
    cs->cdw = 0;
    cs->section = 0;
    return 0;
}

static int r600_cs_need_flush(struct radeon_cs *cs)
{
    /* this function used to flush when the BO usage got to
     * a certain size, now the higher levels handle this better */
    return 0;
}

static void r600_cs_print(struct radeon_cs *cs, FILE *file)
{
}

static struct radeon_cs_funcs  r600_cs_funcs = {
    r600_cs_create,
    r600_cs_write_reloc,
    r600_cs_begin,
    r600_cs_end,
    r600_cs_emit,
    r600_cs_destroy,
    r600_cs_erase,
    r600_cs_need_flush,
    r600_cs_print
};

struct radeon_cs_manager * r600_radeon_cs_manager_legacy_ctor(struct radeon_context *ctx)
{
    struct r600_cs_manager_legacy *csm;

    csm = (struct r600_cs_manager_legacy*)
          calloc(1, sizeof(struct r600_cs_manager_legacy));
    if (csm == NULL) {
        return NULL;
    }
    csm->base.funcs = &r600_cs_funcs;
    csm->base.fd = ctx->dri.fd;
    csm->ctx = ctx;
    csm->pending_age = 1;
    return (struct radeon_cs_manager*)csm;
}

void r600InitCmdBuf(context_t *r600) /* from rcommonInitCmdBuf */
{
	radeonContextPtr rmesa = &r600->radeon;
	GLuint size;

	r600InitAtoms(r600);

	/* Initialize command buffer */
	size = 256 * driQueryOptioni(&rmesa->optionCache,
				     "command_buffer_size");
	if (size < 2 * rmesa->hw.max_state_size) {
		size = 2 * rmesa->hw.max_state_size + 65535;
	}
	if (size > 64 * 256)
		size = 64 * 256;

	if (rmesa->radeonScreen->kernel_mm) {
		int fd = rmesa->radeonScreen->driScreen->fd;
		rmesa->cmdbuf.csm = radeon_cs_manager_gem_ctor(fd);
	} else {
		rmesa->cmdbuf.csm = r600_radeon_cs_manager_legacy_ctor(rmesa);
	}
	if (rmesa->cmdbuf.csm == NULL) {
		/* FIXME: fatal error */
		return;
	}
	rmesa->cmdbuf.cs = radeon_cs_create(rmesa->cmdbuf.csm, size);
	assert(rmesa->cmdbuf.cs != NULL);
	rmesa->cmdbuf.size = size;

	radeon_cs_space_set_flush(rmesa->cmdbuf.cs,
				  (void (*)(void *))rmesa->glCtx->Driver.Flush, rmesa->glCtx);

	if (!rmesa->radeonScreen->kernel_mm) {
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_VRAM, rmesa->radeonScreen->texSize[0]);
		radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_GTT, rmesa->radeonScreen->gartTextures.size);
	} else {
		struct drm_radeon_gem_info mminfo;

		if (!drmCommandWriteRead(rmesa->dri.fd, DRM_RADEON_GEM_INFO, &mminfo, sizeof(mminfo)))
		{
			radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_VRAM, mminfo.vram_visible);
			radeon_cs_set_limit(rmesa->cmdbuf.cs, RADEON_GEM_DOMAIN_GTT, mminfo.gart_size);
		}
	}
}

