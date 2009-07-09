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

struct r600_cs_manager_legacy
{
    struct radeon_cs_manager    base;
    struct radeon_context       *ctx;
    /* hack for scratch stuff */
    uint32_t                    pending_age;
    uint32_t                    pending_count;
};

struct r600_cs_reloc_legacy {
    struct radeon_cs_reloc  base;
    uint32_t                cindices;
    uint32_t                *indices;
    uint32_t                *reloc_indices;
    struct offset_modifiers offset_mod;
};

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

int r600_cs_write_reloc(struct radeon_cs *cs,
                        struct radeon_bo *bo,
                        uint32_t read_domain,
                        uint32_t write_domain,
                        uint32_t flags,
                        offset_modifiers* poffset_mod)
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
            relocs[i].indices[relocs[i].cindices - 1] = cs->cdw - 1;
            relocs[i].reloc_indices[relocs[i].cindices - 1] = cs->section_cdw;
            cs->section_ndw += 2;
            cs->section_cdw += 2;

            relocs[i].offset_mod.shift     = poffset_mod->shift;
            relocs[i].offset_mod.shiftbits = poffset_mod->shiftbits;
            relocs[i].offset_mod.mask      = poffset_mod->mask;

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
    relocs[cs->crelocs].offset_mod.shift     = poffset_mod->shift;
    relocs[cs->crelocs].offset_mod.shiftbits = poffset_mod->shiftbits;
    relocs[cs->crelocs].offset_mod.mask      = poffset_mod->mask;

    relocs[cs->crelocs].indices[0] = cs->cdw - 1;
    relocs[cs->crelocs].reloc_indices[0] = cs->section_cdw;
    cs->section_ndw += 2;
    cs->section_cdw += 2;
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

    if (cs->cdw + ndw + 32 > cs->ndw) { /* Left 32 DWORD (8 offset+pitch) spare room for reloc indices */
        uint32_t tmp, *ptr;
	int num = (ndw > 0x3FF) ? ndw : 0x3FF;

        tmp = (cs->cdw + 1 + num) & (~num);
        ptr = (uint32_t*)realloc(cs->packets, 4 * tmp);
        if (ptr == NULL) {
            return -ENOMEM;
        }
        cs->packets = ptr;
        cs->ndw = tmp;
    }

    cs->section = 1;
    cs->section_ndw = 0; 
    cs->section_cdw = cs->cdw + ndw; /* start of reloc indices. */
    cs->section_file = file;
    cs->section_func = func;
    cs->section_line = line;

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

    if ( (cs->section_ndw + cs->cdw) != cs->section_cdw ) 
    {
        fprintf(stderr, "CS section size missmatch start at (%s,%s,%d) %d vs %d\n",
                cs->section_file, cs->section_func, cs->section_line, cs->section_ndw, cs->section_cdw);
        fprintf(stderr, "cs->section_ndw = %d, cs->cdw = %d, cs->section_cdw = %d \n",
                cs->section_ndw, cs->cdw, cs->section_cdw);
        fprintf(stderr, "CS section end at (%s,%s,%d)\n",
                file, func, line);
        return -EPIPE;
    }

    cs->cdw = cs->section_cdw;
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
    for (i = 0; i < cs->crelocs; i++) 
    {
        for (j = 0; j < relocs[i].cindices; j++) 
        {
            uint32_t soffset, eoffset, asicoffset;

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
            asicoffset = soffset;
            if (asicoffset >= eoffset) 
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
            /* apply offset operator */
            switch (relocs[i].offset_mod.shift)
            {
            case NO_SHIFT:
                asicoffset = asicoffset & relocs[i].offset_mod.mask;
                break;
            case LEFT_SHIFT:
                asicoffset = (asicoffset << relocs[i].offset_mod.shiftbits) & relocs[i].offset_mod.mask;
                break;
            case RIGHT_SHIFT:
                asicoffset = (asicoffset >> relocs[i].offset_mod.shiftbits) & relocs[i].offset_mod.mask;
                break;
            default:
                break;
            };              

            /* pkt3 nop header in ib chunk */
            cs->packets[relocs[i].reloc_indices[j]] = 0xC0001000;

            /* reloc index in ib chunk */
            cs->packets[relocs[i].reloc_indices[j] + 1] = offset_dw;
            
            /* asic offset in reloc chunk */ /* see alex drm r600_nomm_relocate */
            reloc_chunk[offset_dw] = asicoffset;
            reloc_chunk[offset_dw + 3] = 0;

            offset_dw += 4;
        }
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

static void dump_cmdbuf(struct radeon_cs *cs)
{
	int i;
	fprintf(stderr,"--start--\n");
	for (i = 0; i < cs->cdw; i++){
		fprintf(stderr,"0x%08x\n", cs->packets[i]);
	}
	fprintf(stderr,"--end--\n");

}

static int r600_cs_emit(struct radeon_cs *cs)
{
    struct r600_cs_manager_legacy *csm = (struct r600_cs_manager_legacy*)cs->csm;
    struct drm_radeon_cs       cs_cmd;
    struct drm_radeon_cs_chunk cs_chunk[2];
    drm_radeon_cmd_buffer_t cmd; 
    /* drm_r300_cmd_header_t age; */
    uint32_t length_dw_reloc_chunk;
    uint64_t ull;
    uint64_t chunk_ptrs[2];
    uint32_t reloc_chunk[128]; 
    int r;
    int retry = 0;

    /* TODO : put chip level things here if need. */
    /* csm->ctx->vtbl.emit_cs_header(cs, csm->ctx); */

    BATCH_LOCALS(csm->ctx);
    drm_radeon_getparam_t gp;
    uint32_t              current_scratchx_age;

    gp.param = RADEON_PARAM_LAST_CLEAR;
    gp.value = (int *)&current_scratchx_age;
    r = drmCommandWriteRead(cs->csm->fd, 
                            DRM_RADEON_GETPARAM,
                            &gp, 
                            sizeof(gp));
    if (r) 
    {
        fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__, r);
        exit(1);
    }

    csm->pending_age = 0;
    csm->pending_count = 1;

    current_scratchx_age++;
    csm->pending_age = current_scratchx_age;

    BEGIN_BATCH_NO_AUTOSTATE(2);
    R600_OUT_BATCH(0x2142); /* scratch 2 */
    R600_OUT_BATCH(current_scratchx_age);
    END_BATCH();
    COMMIT_BATCH();

    //TODO ioctl to get back cs id assigned in drm
    //csm->pending_age = cs_id_back;
    
    r = r600_cs_process_relocs(cs, &(reloc_chunk[0]), &length_dw_reloc_chunk);
    if (r) {
        return 0;
    }
      
    /* raw ib chunk */
    cs_chunk[0].chunk_id   = RADEON_CHUNK_ID_IB;
    cs_chunk[0].length_dw  = cs->cdw;
    cs_chunk[0].chunk_data = (unsigned long)(cs->packets);

    /* reloc chaunk */
    cs_chunk[1].chunk_id   = RADEON_CHUNK_ID_RELOCS;
    cs_chunk[1].length_dw  = length_dw_reloc_chunk;
    cs_chunk[1].chunk_data = (unsigned long)&(reloc_chunk[0]);

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
        return r;
    }

    r600_cs_set_age(cs);

    cs->csm->read_used = 0;
    cs->csm->vram_write_used = 0;
    cs->csm->gart_write_used = 0;

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

static int r600_cs_check_space(struct radeon_cs *cs, struct radeon_cs_space_check *bos, int num_bo)
{
    struct radeon_cs_manager *csm = cs->csm;
    int this_op_read = 0, this_op_gart_write = 0, this_op_vram_write = 0;
    uint32_t read_domains, write_domain;
    int i;
    struct radeon_bo *bo;

    /* check the totals for this operation */

    if (num_bo == 0)
        return 0;

    /* prepare */
    for (i = 0; i < num_bo; i++) 
    {
         bo = bos[i].bo;

         bos[i].new_accounted = 0;
         read_domains = bos[i].read_domains;
         write_domain = bos[i].write_domain;
		   
         /* pinned bos don't count */
         if (radeon_legacy_bo_is_static(bo))
	     continue;
 
         /* already accounted this bo */
         if (write_domain && (write_domain == bo->space_accounted))
	     continue;

         if (read_domains && ((read_domains << 16) == bo->space_accounted))
	     continue;
      
         if (bo->space_accounted == 0) 
         {
	         if (write_domain == RADEON_GEM_DOMAIN_VRAM)
	             this_op_vram_write += bo->size;
	         else if (write_domain == RADEON_GEM_DOMAIN_GTT)
	             this_op_gart_write += bo->size;
	         else
	             this_op_read += bo->size;
	         bos[i].new_accounted = (read_domains << 16) | write_domain;
         } 
         else 
         {
	        uint16_t old_read, old_write;
	     
	        old_read = bo->space_accounted >> 16;
	        old_write = bo->space_accounted & 0xffff;

	        if (write_domain && (old_read & write_domain)) 
            {
	            bos[i].new_accounted = write_domain;
	            /* moving from read to a write domain */
	            if (write_domain == RADEON_GEM_DOMAIN_VRAM) 
                {
		            this_op_read -= bo->size;
		            this_op_vram_write += bo->size;
	            } 
                else if (write_domain == RADEON_GEM_DOMAIN_VRAM) 
                {
		            this_op_read -= bo->size;
		            this_op_gart_write += bo->size;
	            }
	        } 
            else if (read_domains & old_write) 
            {
	            bos[i].new_accounted = bo->space_accounted & 0xffff;
	        } 
            else 
            {
	            /* rewrite the domains */
	            if (write_domain != old_write)
		            fprintf(stderr,"WRITE DOMAIN RELOC FAILURE 0x%x %d %d\n", bo->handle, write_domain, old_write);
	            if (read_domains != old_read)
		            fprintf(stderr,"READ DOMAIN RELOC FAILURE 0x%x %d %d\n", bo->handle, read_domains, old_read);
	            return RADEON_CS_SPACE_FLUSH;
	        }
         }
	}
	
	if (this_op_read < 0)
		this_op_read = 0;

	/* check sizes - operation first */
	if ((this_op_read + this_op_gart_write > csm->gart_limit) ||
	    (this_op_vram_write > csm->vram_limit)) {
	    return RADEON_CS_SPACE_OP_TO_BIG;
	}

	if (((csm->vram_write_used + this_op_vram_write) > csm->vram_limit) ||
	    ((csm->read_used + csm->gart_write_used + this_op_gart_write + this_op_read) > csm->gart_limit)) {
		return RADEON_CS_SPACE_FLUSH;
	}

	csm->gart_write_used += this_op_gart_write;
	csm->vram_write_used += this_op_vram_write;
	csm->read_used += this_op_read;
	/* commit */
	for (i = 0; i < num_bo; i++) {
		bo = bos[i].bo;
		bo->space_accounted = bos[i].new_accounted;
	}

	return RADEON_CS_SPACE_OK;
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
    r600_cs_print,
    r600_cs_check_space
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

void r600_sw_blit(char *srcp, int src_pitch, char *dstp, int dst_pitch,
		  int x, int y, int w, int h, int cpp)
{
	char *src = srcp;
	char *dst = dstp;

	src += (y * src_pitch) + (x * cpp);
	dst += (y * dst_pitch) + (x * cpp);

	while (h--) {
		memcpy(dst, src, w * cpp);
		src += src_pitch;
		dst += dst_pitch;
	}
}

