/*
 * Copyright (C) 2008-2010  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com> 
 */

#include "main/imports.h"
#include "main/glheader.h"
#include "main/simple_list.h"

#include "r600_context.h"
#include "r600_cmdbuf.h"

#include "evergreen_chip.h"
#include "evergreen_off.h"
#include "evergreen_diff.h"
#include "evergreen_fragprog.h"
#include "evergreen_vertprog.h"

#include "radeon_mipmap_tree.h"

void evergreenCreateChip(context_t *context)
{
    EVERGREEN_CHIP_CONTEXT * evergreen = 
           (EVERGREEN_CHIP_CONTEXT*) CALLOC(sizeof(EVERGREEN_CHIP_CONTEXT));

    context->pChip = (void*)evergreen;
}

#define EVERGREEN_ALLOC_STATE( ATOM, CHK, SZ, EMIT )				\
do {									\
	context->evergreen_atoms.ATOM.cmd_size = (SZ);				\
	context->evergreen_atoms.ATOM.cmd = NULL;					\
	context->evergreen_atoms.ATOM.name = #ATOM;				\
	context->evergreen_atoms.ATOM.idx = 0;					\
	context->evergreen_atoms.ATOM.check = check_##CHK;			\
	context->evergreen_atoms.ATOM.dirty = GL_FALSE;				\
	context->evergreen_atoms.ATOM.emit = (EMIT);				\
	context->radeon.hw.max_state_size += (SZ);			\
	insert_at_tail(&context->radeon.hw.atomlist, &context->evergreen_atoms.ATOM); \
} while (0)

static int check_queryobj(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;
	int count;

	if (!query || query->emitted_begin)
		count = 0;
	else
		count = atom->cmd_size;
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count;
}

static void evergreenSendQueryBegin(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	struct radeon_query_object *query = radeon->query.current;
	BATCH_LOCALS(radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	/* clear the buffer */
	radeon_bo_map(query->bo, GL_FALSE);
	memset(query->bo->ptr, 0, 8 * 2 * sizeof(uint64_t)); /* 8 DBs, 2 qwords each */
	radeon_bo_unmap(query->bo);

	radeon_cs_space_check_with_bo(radeon->cmdbuf.cs,
				      query->bo,
				      0, RADEON_GEM_DOMAIN_GTT);

	BEGIN_BATCH_NO_AUTOSTATE(4 + 2);
	R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 2));
	R600_OUT_BATCH(R600_EVENT_TYPE(ZPASS_DONE) | R600_EVENT_INDEX(1));
	R600_OUT_BATCH(query->curr_offset); /* hw writes qwords */
	R600_OUT_BATCH(0x00000000);
	R600_OUT_BATCH_RELOC(VGT_EVENT_INITIATOR, query->bo, 0, 0, RADEON_GEM_DOMAIN_GTT, 0);
	END_BATCH();
	query->emitted_begin = GL_TRUE;
}

static void evergreen_init_query_stateobj(radeonContextPtr radeon, int SZ)
{
	radeon->query.queryobj.cmd_size = (SZ);
	radeon->query.queryobj.cmd = NULL;
	radeon->query.queryobj.name = "queryobj";
	radeon->query.queryobj.idx = 0;
	radeon->query.queryobj.check = check_queryobj;
	radeon->query.queryobj.dirty = GL_FALSE;
	radeon->query.queryobj.emit = evergreenSendQueryBegin;
	radeon->hw.max_state_size += (SZ);
	insert_at_tail(&radeon->hw.atomlist, &radeon->query.queryobj);
}


static int check_always(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	return atom->cmd_size;
}

static void evergreenSendTexState(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *) context->selected_vp;

	struct radeon_bo *bo = NULL;
	unsigned int i;
    unsigned int nBorderSet = 0;
	BATCH_LOCALS(&context->radeon);

    radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {            
			radeonTexObj *t = evergreen->textures[i];

			if (t) {
                /* Tex resource */
				if (!t->image_override) {
					bo = t->mt->bo;
				} else {
					bo = t->bo;
				}
				if (bo) 
                {                    

					r700SyncSurf(context, bo,
						     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
						     0, TC_ACTION_ENA_bit);                    

					BEGIN_BATCH_NO_AUTOSTATE(10 + 4);
					R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 8));

                    if( (1<<i) & vp->r700AsmCode.unVetTexBits )                    
                    {   /* vs texture */                                     
                        R600_OUT_BATCH((i + VERT_ATTRIB_MAX + EG_SQ_FETCH_RESOURCE_VS_OFFSET) * EG_FETCH_RESOURCE_STRIDE);
                    }
                    else
                    {
					    R600_OUT_BATCH(i * EG_FETCH_RESOURCE_STRIDE);
                    }

					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE0);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE1);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE2);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE3);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE4);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE5);
					R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE6);
                    R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_RESOURCE7);

					R600_OUT_BATCH_RELOC(evergreen->textures[i]->SQ_TEX_RESOURCE2,
							     bo,
							     evergreen->textures[i]->SQ_TEX_RESOURCE2,
							     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
					R600_OUT_BATCH_RELOC(evergreen->textures[i]->SQ_TEX_RESOURCE3,
							     bo,
							     evergreen->textures[i]->SQ_TEX_RESOURCE3,
							     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
					END_BATCH();
					COMMIT_BATCH();
				}
                /* Tex sampler */
                BEGIN_BATCH_NO_AUTOSTATE(5);
				R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_SAMPLER, 3));
             
                if( (1<<i) & vp->r700AsmCode.unVetTexBits )                    
                {   /* vs texture */
                    R600_OUT_BATCH((i+SQ_TEX_SAMPLER_VS_OFFSET) * 3); 
                }
                else
                {
				    R600_OUT_BATCH(i * 3);
                }
				R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_SAMPLER0);
				R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_SAMPLER1);
				R600_OUT_BATCH(evergreen->textures[i]->SQ_TEX_SAMPLER2);

				END_BATCH();
				COMMIT_BATCH();

                /* Tex border color */
                if(0 == nBorderSet)
                {
                    BEGIN_BATCH_NO_AUTOSTATE(2 + 4);
				    R600_OUT_BATCH_REGSEQ(EG_TD_PS_BORDER_COLOR_RED, 4);
				    R600_OUT_BATCH(evergreen->textures[i]->TD_PS_SAMPLER0_BORDER_RED);
				    R600_OUT_BATCH(evergreen->textures[i]->TD_PS_SAMPLER0_BORDER_GREEN);
				    R600_OUT_BATCH(evergreen->textures[i]->TD_PS_SAMPLER0_BORDER_BLUE);
				    R600_OUT_BATCH(evergreen->textures[i]->TD_PS_SAMPLER0_BORDER_ALPHA);
				    END_BATCH();
				    COMMIT_BATCH();

                    nBorderSet = 1;
                }
			}
		}
	}
}

static int check_evergreen_tx(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	unsigned int i, count = 0;
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);

	for (i = 0; i < R700_TEXTURE_NUMBERUNITS; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			radeonTexObj *t = evergreen->textures[i];
			if (t)
				count++;
		}
	}
	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count * 37 + 6;
}

static void evergreenSendSQConfig(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);    

    BEGIN_BATCH_NO_AUTOSTATE(19);
    //6
    EVERGREEN_OUT_BATCH_REGVAL(EG_SPI_CONFIG_CNTL,   evergreen->evergreen_config.SPI_CONFIG_CNTL.u32All);
	EVERGREEN_OUT_BATCH_REGVAL(EG_SPI_CONFIG_CNTL_1, evergreen->evergreen_config.SPI_CONFIG_CNTL_1.u32All);
    //6
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_CONFIG, 4);
    R600_OUT_BATCH(evergreen->evergreen_config.SQ_CONFIG.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_2.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_GPR_RESOURCE_MGMT_3.u32All);
    //7
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_THREAD_RESOURCE_MGMT, 5);
    R600_OUT_BATCH(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_THREAD_RESOURCE_MGMT_2.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_1.u32All);
	R600_OUT_BATCH(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_2.u32All);
    R600_OUT_BATCH(evergreen->evergreen_config.SQ_STACK_RESOURCE_MGMT_3.u32All);

    END_BATCH();

    COMMIT_BATCH();
}

extern int evergreen_getTypeSize(GLenum type);
static void evergreenSetupVTXConstants(struct gl_context  * ctx,
				  void *       pAos,
				  StreamDesc * pStreamDesc)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct radeon_aos * paos = (struct radeon_aos *)pAos;
    BATCH_LOCALS(&context->radeon);

    unsigned int uSQ_VTX_CONSTANT_WORD0_0;
    unsigned int uSQ_VTX_CONSTANT_WORD1_0;
    unsigned int uSQ_VTX_CONSTANT_WORD2_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD3_0 = 0;
    unsigned int uSQ_VTX_CONSTANT_WORD7_0 = 0;

    if (!paos->bo)
	    return;

    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_CEDAR) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_PALM) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_SUMO) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_SUMO2))
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, TC_ACTION_ENA_bit);
    else
	    r700SyncSurf(context, paos->bo, RADEON_GEM_DOMAIN_GTT, 0, VC_ACTION_ENA_bit);

    //uSQ_VTX_CONSTANT_WORD0_0
    uSQ_VTX_CONSTANT_WORD0_0 = paos->offset;

    //uSQ_VTX_CONSTANT_WORD1_0
    uSQ_VTX_CONSTANT_WORD1_0 = paos->bo->size - paos->offset - 1;
        
    //uSQ_VTX_CONSTANT_WORD2_0
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, 
             pStreamDesc->stride, 
             SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift, 
	         SQ_VTX_CONSTANT_WORD2_0__STRIDE_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, GetSurfaceFormat(pStreamDesc->type, pStreamDesc->size, NULL),
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_shift,
	     SQ_VTX_CONSTANT_WORD2_0__DATA_FORMAT_mask); // TODO : trace back api for initial data type, not only GL_FLOAT     
    SETfield(uSQ_VTX_CONSTANT_WORD2_0, 0, BASE_ADDRESS_HI_shift, BASE_ADDRESS_HI_mask); // TODO    

	SETfield(uSQ_VTX_CONSTANT_WORD2_0, 
#ifdef MESA_BIG_ENDIAN 
			 SQ_ENDIAN_8IN32,
#else
			 SQ_ENDIAN_NONE,
#endif
             SQ_VTX_CONSTANT_WORD2_0__ENDIAN_SWAP_shift,
             SQ_VTX_CONSTANT_WORD2_0__ENDIAN_SWAP_mask);

    if(GL_TRUE == pStreamDesc->normalize)
    {
        SETfield(uSQ_VTX_CONSTANT_WORD2_0, SQ_NUM_FORMAT_NORM,
	             SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift, SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_mask);
    }
    else
    {
        SETfield(uSQ_VTX_CONSTANT_WORD2_0, SQ_NUM_FORMAT_SCALED,
	             SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_shift, SQ_VTX_CONSTANT_WORD2_0__NUM_FORMAT_ALL_mask);
    }
    if(1 == pStreamDesc->_signed)
    {
        SETbit(uSQ_VTX_CONSTANT_WORD2_0, SQ_VTX_CONSTANT_WORD2_0__FORMAT_COMP_ALL_bit);
    }

    //uSQ_VTX_CONSTANT_WORD3_0
    SETfield(uSQ_VTX_CONSTANT_WORD3_0, SQ_SEL_X, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_shift, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD3_0, SQ_SEL_Y, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_shift, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD3_0, SQ_SEL_Z, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_shift, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_mask);
    SETfield(uSQ_VTX_CONSTANT_WORD3_0, SQ_SEL_W, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_shift, 
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_mask);

    //uSQ_VTX_CONSTANT_WORD7_0
    SETfield(uSQ_VTX_CONSTANT_WORD7_0, SQ_TEX_VTX_VALID_BUFFER,
	     SQ_TEX_RESOURCE_WORD6_0__TYPE_shift, SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

    BEGIN_BATCH_NO_AUTOSTATE(10 + 2);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 8));
    R600_OUT_BATCH((pStreamDesc->element + EG_SQ_FETCH_RESOURCE_VS_OFFSET) * EG_FETCH_RESOURCE_STRIDE);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD0_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD1_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD2_0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD3_0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(uSQ_VTX_CONSTANT_WORD7_0);
    R600_OUT_BATCH_RELOC(uSQ_VTX_CONSTANT_WORD0_0,
                         paos->bo,
                         uSQ_VTX_CONSTANT_WORD0_0,
                         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    
    COMMIT_BATCH();
}

static int check_evergreen_vtx(struct gl_context *ctx, struct radeon_state_atom *atom)
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	int count = context->radeon.tcl.aos_count * 12;

	if (count)
		count += 6;

	radeon_print(RADEON_STATE, RADEON_TRACE, "%s %d\n", __func__, count);
	return count;
}

static void evergreenSendVTX(struct gl_context *ctx, struct radeon_state_atom *atom)
{    
    context_t         *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_vertex_program *vp = (struct evergreen_vertex_program *)(context->selected_vp);
    unsigned int i, j = 0;
    BATCH_LOCALS(&context->radeon);
    (void) b_l_rmesa;  /* silence unused var warning */

	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    if (context->radeon.tcl.aos_count == 0)
	    return;

    for(i=0; i<VERT_ATTRIB_MAX; i++) {
	    if(vp->mesa_program->Base.InputsRead & (1 << i))
	    {
            evergreenSetupVTXConstants(ctx,
				      (void*)(&context->radeon.tcl.aos[j]),
				      &(context->stream_desc[j]));
		    j++;
	    }
    }
}
static void evergreenSendPA(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);
    int id = 0;

    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_PA_SU_HARDWARE_SCREEN_OFFSET, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(22);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_SCREEN_SCISSOR_TL, 2);
    R600_OUT_BATCH(evergreen->PA_SC_SCREEN_SCISSOR_TL.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_SCREEN_SCISSOR_BR.u32All);  

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_WINDOW_OFFSET, 12);
    R600_OUT_BATCH(evergreen->PA_SC_WINDOW_OFFSET.u32All);       
    R600_OUT_BATCH(evergreen->PA_SC_WINDOW_SCISSOR_TL.u32All);   
    R600_OUT_BATCH(evergreen->PA_SC_WINDOW_SCISSOR_BR.u32All);   
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_RULE.u32All);       
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_0_TL.u32All);       
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_0_BR.u32All);       
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_1_TL.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_1_BR.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_2_TL.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_2_BR.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_3_TL.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_CLIPRECT_3_BR.u32All);  
    
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_GENERIC_SCISSOR_TL, 2);
    R600_OUT_BATCH(evergreen->PA_SC_GENERIC_SCISSOR_TL.u32All);  
    R600_OUT_BATCH(evergreen->PA_SC_GENERIC_SCISSOR_BR.u32All); 
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_PA_SC_EDGERULE, evergreen->PA_SC_EDGERULE.u32All);
    END_BATCH();
        

    BEGIN_BATCH_NO_AUTOSTATE(18);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_VPORT_SCISSOR_0_TL, 4);
    R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All);    
    R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All);    
    R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_TL.u32All);    
    R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_SCISSOR_0_BR.u32All);        

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_VPORT_ZMIN_0, 2);
	R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_ZMIN_0.u32All);  
    R600_OUT_BATCH(evergreen->viewport[id].PA_SC_VPORT_ZMAX_0.u32All);  

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_CL_VPORT_XSCALE, 6);
	R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_XSCALE.u32All);     
    R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_XOFFSET.u32All);    
    R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_YSCALE.u32All);     
    R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_YOFFSET.u32All);    
    R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_ZSCALE.u32All);     
    R600_OUT_BATCH(evergreen->viewport[id].PA_CL_VPORT_ZOFFSET.u32All);    
    END_BATCH();

    
    for (id = 0; id < EVERGREEN_MAX_UCP; id++) {
		if (evergreen->ucp[id].enabled) {
			BEGIN_BATCH_NO_AUTOSTATE(6);
			EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_CL_UCP_0_X + (4 * id), 4);
			R600_OUT_BATCH(evergreen->ucp[id].PA_CL_UCP_0_X.u32All);
			R600_OUT_BATCH(evergreen->ucp[id].PA_CL_UCP_0_Y.u32All);
			R600_OUT_BATCH(evergreen->ucp[id].PA_CL_UCP_0_Z.u32All);
			R600_OUT_BATCH(evergreen->ucp[id].PA_CL_UCP_0_W.u32All);
			END_BATCH();			
		}
	}

    BEGIN_BATCH_NO_AUTOSTATE(42);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_CL_CLIP_CNTL, 5);
    R600_OUT_BATCH(evergreen->PA_CL_CLIP_CNTL.u32All);                           
    R600_OUT_BATCH(evergreen->PA_SU_SC_MODE_CNTL.u32All);                       
    R600_OUT_BATCH(evergreen->PA_CL_VTE_CNTL.u32All);                          
    R600_OUT_BATCH(evergreen->PA_CL_VS_OUT_CNTL.u32All);                       
    R600_OUT_BATCH(evergreen->PA_CL_NANINF_CNTL.u32All);                     
     
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SU_POINT_SIZE, 3);
    R600_OUT_BATCH(evergreen->PA_SU_POINT_SIZE.u32All);                        
    R600_OUT_BATCH(evergreen->PA_SU_POINT_MINMAX.u32All);                        
    R600_OUT_BATCH(evergreen->PA_SU_LINE_CNTL.u32All);                           
    
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_MODE_CNTL_0, 2);
    R600_OUT_BATCH(evergreen->PA_SC_MODE_CNTL_0.u32All);                         
    R600_OUT_BATCH(evergreen->PA_SC_MODE_CNTL_1.u32All);                       

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SU_POLY_OFFSET_DB_FMT_CNTL, 6);
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_DB_FMT_CNTL.u32All);            
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_CLAMP.u32All);                  
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_FRONT_SCALE.u32All);          
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_FRONT_OFFSET.u32All);        
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_BACK_SCALE.u32All);            
    R600_OUT_BATCH(evergreen->PA_SU_POLY_OFFSET_BACK_OFFSET.u32All);            

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_LINE_CNTL, 16);
    R600_OUT_BATCH(evergreen->PA_SC_LINE_CNTL.u32All);                           
    R600_OUT_BATCH(evergreen->PA_SC_AA_CONFIG.u32All);                            
    R600_OUT_BATCH(evergreen->PA_SU_VTX_CNTL.u32All);                           
    R600_OUT_BATCH(evergreen->PA_CL_GB_VERT_CLIP_ADJ.u32All);                    
    R600_OUT_BATCH(evergreen->PA_CL_GB_VERT_DISC_ADJ.u32All);                    
    R600_OUT_BATCH(evergreen->PA_CL_GB_HORZ_CLIP_ADJ.u32All);                    
    R600_OUT_BATCH(evergreen->PA_CL_GB_HORZ_DISC_ADJ.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_0.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_1.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_2.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_3.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_4.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_5.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_6.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_SAMPLE_LOCS_7.u32All);                    
    R600_OUT_BATCH(evergreen->PA_SC_AA_MASK.u32All);    
    
    END_BATCH();

    COMMIT_BATCH();
}
static void evergreenSendTP(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    /*
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    COMMIT_BATCH();
    */
}

static void evergreenSendPSresource(struct gl_context *ctx)
{    
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct radeon_bo * pbo;
    struct radeon_bo * pbo_const;
    /* const size reg is in units of 16 consts */
    int const_size = ((evergreen->ps.num_consts * 4) + 15) & ~15;

    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    pbo = (struct radeon_bo *)evergreenGetActiveFpShaderBo(GL_CONTEXT(context));

    if (!pbo)
	    return;

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_START_PS, 1);
    R600_OUT_BATCH(evergreen->ps.SQ_PGM_START_PS.u32All);
    R600_OUT_BATCH_RELOC(evergreen->ps.SQ_PGM_START_PS.u32All,
		                 pbo,
		                 evergreen->ps.SQ_PGM_START_PS.u32All,
		                 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
   
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_LOOP_CONST_0, 0x01000FFF);
    END_BATCH();

	pbo_const = (struct radeon_bo *)(context->fp_Constbo);
	 
    if(NULL != pbo_const)
    {                  
        r700SyncSurf(context, pbo_const, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit); 

	BEGIN_BATCH_NO_AUTOSTATE(3);
	EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_ALU_CONST_BUFFER_SIZE_PS_0, const_size / 16);
        END_BATCH();

        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);            
        EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_ALU_CONST_CACHE_PS_0, 1);
        R600_OUT_BATCH(context->fp_bo_offset >> 8);
        R600_OUT_BATCH_RELOC(0,
	                         pbo_const,
	                         0,
	                         RADEON_GEM_DOMAIN_GTT, 0, 0);
        END_BATCH();            
    }        

    COMMIT_BATCH();    
}

static void evergreenSendVSresource(struct gl_context *ctx, struct radeon_state_atom *atom)
{    
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct radeon_bo * pbo;
    struct radeon_bo * pbo_const;
    /* const size reg is in units of 16 consts */
    int const_size = ((evergreen->vs.num_consts * 4) + 15) & ~15;

    BATCH_LOCALS(&context->radeon);
    radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    pbo = (struct radeon_bo *)evergreenGetActiveVpShaderBo(GL_CONTEXT(context));

    if (!pbo)
	    return;	

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_START_VS, 1);
    R600_OUT_BATCH(evergreen->vs.SQ_PGM_START_VS.u32All);
    R600_OUT_BATCH_RELOC(evergreen->vs.SQ_PGM_START_VS.u32All,
		                 pbo,
		                 evergreen->vs.SQ_PGM_START_VS.u32All,
		                 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL((EG_SQ_LOOP_CONST_0 + 32*1), 0x0100000F); //consts == 1
    //EVERGREEN_OUT_BATCH_REGVAL((EG_SQ_LOOP_CONST_0 + (SQ_LOOP_CONST_vs<2)), 0x0100000F);
    END_BATCH();
            
	pbo_const = (struct radeon_bo *)(context->vp_Constbo);    

    if(NULL != pbo_const)
    {                  
        r700SyncSurf(context, pbo_const, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

	BEGIN_BATCH_NO_AUTOSTATE(3);
	EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_ALU_CONST_BUFFER_SIZE_VS_0, const_size / 16);
        END_BATCH();

        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);            
        EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_ALU_CONST_CACHE_VS_0, 1);
        R600_OUT_BATCH(context->vp_bo_offset >> 8);
        R600_OUT_BATCH_RELOC(0,
	                         pbo_const,
	                         0,
	                         RADEON_GEM_DOMAIN_GTT, 0, 0);
        END_BATCH();            
    }        

    COMMIT_BATCH();    
}

static void evergreenSendSQ(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    evergreenSendPSresource(ctx); //16 entries now

    BEGIN_BATCH_NO_AUTOSTATE(77);

    //34
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_VTX_SEMANTIC_0, 32);
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_0.u32All);    ////                      // = 0x28380, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_1.u32All);    ////                      // = 0x28384, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_2.u32All);    ////                      // = 0x28388, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_3.u32All);    ////                      // = 0x2838C, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_4.u32All);    ////                      // = 0x28390, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_5.u32All);    ////                      // = 0x28394, // SAME
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_6.u32All);    ////                      // = 0x28398, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_7.u32All);    ////                      // = 0x2839C, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_8.u32All);    ////                      // = 0x283A0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_9.u32All);    ////                      // = 0x283A4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_10.u32All);   ////                      // = 0x283A8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_11.u32All);   ////                      // = 0x283AC, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_12.u32All);   ////                      // = 0x283B0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_13.u32All);   ////                      // = 0x283B4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_14.u32All);   ////                      // = 0x283B8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_15.u32All);   ////                      // = 0x283BC, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_16.u32All);   ////                      // = 0x283C0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_17.u32All);   ////                      // = 0x283C4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_18.u32All);   ////                      // = 0x283C8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_19.u32All);   ////                      // = 0x283CC, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_20.u32All);   ////                      // = 0x283D0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_21.u32All);   ////                      // = 0x283D4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_22.u32All);   ////                      // = 0x283D8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_23.u32All);   ////                      // = 0x283DC, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_24.u32All);   ////                      // = 0x283E0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_25.u32All);   ////                      // = 0x283E4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_26.u32All);   ////                      // = 0x283E8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_27.u32All);   ////                      // = 0x283EC, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_28.u32All);   ////                      // = 0x283F0, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_29.u32All);   ////                      // = 0x283F4, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_30.u32All);   ////                      // = 0x283F8, // SAME 
    R600_OUT_BATCH(evergreen->SQ_VTX_SEMANTIC_31.u32All);   ////                      // = 0x283FC, // SAME 
    

    //3
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_DYN_GPR_RESOURCE_LIMIT_1, 1);
    R600_OUT_BATCH(evergreen->SQ_DYN_GPR_RESOURCE_LIMIT_1.u32All);////                // = 0x28838, // 
    
    //5
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_PS, 3);
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_PS.u32All);  ////                      // = 0x28844, // DIFF 0x28850 
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_PS.u32All); ////                     // = 0x28848, // 
    R600_OUT_BATCH(evergreen->SQ_PGM_EXPORTS_PS.u32All); ////                         // = 0x2884C, // SAME 0x28854 
    
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_VS, 2);
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_VS.u32All);////                        // = 0x28860, // DIFF 0x28868 
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_VS.u32All);  ////                    // = 0x28864, // 

    //5
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_GS, 2);
    /*
    R600_OUT_BATCH(evergreen->SQ_PGM_START_GS.u32All); ////                           // = 0x28874, // SAME 0x2886C 
    */
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_GS.u32All); ////                       // = 0x28878, // DIFF 0x2887C 
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_GS.u32All); ////                     // = 0x2887C, // 

    //5
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_ES, 2);
    /*
    R600_OUT_BATCH(evergreen->SQ_PGM_START_ES.u32All);  ////                          // = 0x2888C, // SAME 0x28880 
    */
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_ES.u32All); ////                       // = 0x28890, // DIFF 
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_ES.u32All); ////                     // = 0x28894, // 

    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_FS, 1);
    /*
    R600_OUT_BATCH(evergreen->SQ_PGM_START_FS.u32All); ////                           // = 0x288A4, // SAME 0x28894 
    */
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_FS.u32All);  ////                      // = 0x288A8, // DIFF 0x288A4 
    
    //3
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_2_HS, 1);
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_HS.u32All);////                      // = 0x288C0, // 
    
    //3
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_2_LS, 1);
    R600_OUT_BATCH(evergreen->SQ_PGM_RESOURCES_2_LS.u32All);  ////                    // = 0x288D8, // 
    
    //3
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_LDS_ALLOC_PS, 1);
    R600_OUT_BATCH(evergreen->SQ_LDS_ALLOC_PS.u32All); ////                           // = 0x288EC, // 
    
    //8
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_ESGS_RING_ITEMSIZE, 6);
    R600_OUT_BATCH(evergreen->SQ_ESGS_RING_ITEMSIZE.u32All); ////                     // = 0x28900, // SAME 0x288A8 
    R600_OUT_BATCH(evergreen->SQ_GSVS_RING_ITEMSIZE.u32All); ////                     // = 0x28904, // SAME 0x288AC 
    R600_OUT_BATCH(evergreen->SQ_ESTMP_RING_ITEMSIZE.u32All);  ////                   // = 0x28908, // SAME 0x288B0 
    R600_OUT_BATCH(evergreen->SQ_GSTMP_RING_ITEMSIZE.u32All);  ////                   // = 0x2890C, // SAME 0x288B4 
    R600_OUT_BATCH(evergreen->SQ_VSTMP_RING_ITEMSIZE.u32All);  ////                   // = 0x28910, // SAME 0x288B8 
    R600_OUT_BATCH(evergreen->SQ_PSTMP_RING_ITEMSIZE.u32All);  ////                   // = 0x28914, // SAME 0x288BC 

    //3
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_GS_VERT_ITEMSIZE, 1);
    R600_OUT_BATCH(evergreen->SQ_GS_VERT_ITEMSIZE.u32All);     ////                   // = 0x2891C, // SAME 0x288C8    
    
    END_BATCH();

    COMMIT_BATCH();

}
static void evergreenSendSPI(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    BEGIN_BATCH_NO_AUTOSTATE(59);

    EVERGREEN_OUT_BATCH_REGSEQ(EG_SPI_VS_OUT_ID_0, 10);
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_0.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_1.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_2.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_3.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_4.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_5.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_6.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_7.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_8.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_ID_9.u32All);                        

    EVERGREEN_OUT_BATCH_REGSEQ(EG_SPI_PS_INPUT_CNTL_0, 45);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[0].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[1].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[2].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[3].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[4].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[5].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[6].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[7].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[8].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[9].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[10].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[11].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[12].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[13].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[14].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[15].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[16].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[17].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[18].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[19].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[20].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[21].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[22].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[23].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[24].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[25].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[26].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[27].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[28].u32All); 
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[29].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[30].u32All);
    R600_OUT_BATCH(evergreen->SPI_PS_INPUT_CNTL[31].u32All);
    R600_OUT_BATCH(evergreen->SPI_VS_OUT_CONFIG.u32All);                  
    R600_OUT_BATCH(evergreen->SPI_THREAD_GROUPING.u32All);                 
    R600_OUT_BATCH(evergreen->SPI_PS_IN_CONTROL_0.u32All);                 
    R600_OUT_BATCH(evergreen->SPI_PS_IN_CONTROL_1.u32All);                 
    R600_OUT_BATCH(evergreen->SPI_INTERP_CONTROL_0.u32All);                
    R600_OUT_BATCH(evergreen->SPI_INPUT_Z.u32All);                         
    R600_OUT_BATCH(evergreen->SPI_FOG_CNTL.u32All);                        
    R600_OUT_BATCH(evergreen->SPI_BARYC_CNTL.u32All);  
    R600_OUT_BATCH(evergreen->SPI_PS_IN_CONTROL_2.u32All); 
    R600_OUT_BATCH(evergreen->SPI_COMPUTE_INPUT_CNTL.u32All);                     
    R600_OUT_BATCH(evergreen->SPI_COMPUTE_NUM_THREAD_X.u32All);                   
    R600_OUT_BATCH(evergreen->SPI_COMPUTE_NUM_THREAD_Y.u32All);                   
    R600_OUT_BATCH(evergreen->SPI_COMPUTE_NUM_THREAD_Z.u32All);                   

    END_BATCH();

    COMMIT_BATCH();
}
static void evergreenSendSX(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

    BEGIN_BATCH_NO_AUTOSTATE(9);
    
    EVERGREEN_OUT_BATCH_REGVAL(EG_SX_MISC,               evergreen->SX_MISC.u32All); 
    EVERGREEN_OUT_BATCH_REGVAL(EG_SX_ALPHA_TEST_CONTROL, evergreen->SX_ALPHA_TEST_CONTROL.u32All);
    EVERGREEN_OUT_BATCH_REGVAL(EG_SX_ALPHA_REF,          evergreen->SX_ALPHA_REF.u32All);

    END_BATCH();

    COMMIT_BATCH();
}

static void evergreenSetDepthTarget(context_t *context)
{
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct radeon_renderbuffer *rrb;
    unsigned int nPitchInPixel, height, offtostencil;

    rrb = radeon_get_depthbuffer(&context->radeon);
    if (!rrb)
    {
        return;
    }

    EVERGREEN_STATECHANGE(context, db);

    evergreen->DB_DEPTH_SIZE.u32All  = 0;        
    
    nPitchInPixel = rrb->pitch/rrb->cpp;

    if (context->radeon.radeonScreen->driScreen->dri2.enabled)
    {
        height = rrb->base.Height;
    }
    else
    {
        height =  context->radeon.radeonScreen->driScreen->fbHeight;
    }

    SETfield(evergreen->DB_DEPTH_SIZE.u32All, (nPitchInPixel/8)-1,
             EG_DB_DEPTH_SIZE__PITCH_TILE_MAX_shift, 
             EG_DB_DEPTH_SIZE__PITCH_TILE_MAX_mask);
    SETfield(evergreen->DB_DEPTH_SIZE.u32All, (height/8)-1,
             EG_DB_DEPTH_SIZE__HEIGHT_TILE_MAX_shift, 
             EG_DB_DEPTH_SIZE__HEIGHT_TILE_MAX_mask);
    evergreen->DB_DEPTH_SLICE.u32All = ( (nPitchInPixel * height)/64 )-1;

    if(4 == rrb->cpp)
    {
        SETfield(evergreen->DB_Z_INFO.u32All, EG_Z_24,
                 EG_DB_Z_INFO__FORMAT_shift, 
                 EG_DB_Z_INFO__FORMAT_mask);
    }
    else
    {
        SETfield(evergreen->DB_Z_INFO.u32All, EG_Z_16,
                 EG_DB_Z_INFO__FORMAT_shift, 
                 EG_DB_Z_INFO__FORMAT_mask);
    }
    SETfield(evergreen->DB_Z_INFO.u32All, ARRAY_1D_TILED_THIN1,
             EG_DB_Z_INFO__ARRAY_MODE_shift, 
             EG_DB_Z_INFO__ARRAY_MODE_mask);        


    offtostencil = ((height * rrb->pitch) + 255) & ~255;
    evergreen->DB_STENCIL_WRITE_BASE.u32All = offtostencil >> 8;
    evergreen->DB_STENCIL_READ_BASE.u32All = offtostencil >> 8;

}

static void evergreenSendDB(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);  
    
    evergreenSetDepthTarget(context);

    //8
    BEGIN_BATCH_NO_AUTOSTATE(7); 
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_RENDER_CONTROL, 5);
    R600_OUT_BATCH(evergreen->DB_RENDER_CONTROL.u32All);                        
    R600_OUT_BATCH(evergreen->DB_COUNT_CONTROL.u32All);                          
    R600_OUT_BATCH(evergreen->DB_DEPTH_VIEW.u32All);                             
    R600_OUT_BATCH(evergreen->DB_RENDER_OVERRIDE.u32All);                        
    R600_OUT_BATCH(evergreen->DB_RENDER_OVERRIDE2.u32All);  
    END_BATCH();

    //4
    BEGIN_BATCH_NO_AUTOSTATE(4); 
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_STENCIL_CLEAR, 2);
    R600_OUT_BATCH(evergreen->DB_STENCIL_CLEAR.u32All);                          
    R600_OUT_BATCH(evergreen->DB_DEPTH_CLEAR.u32All);  
    END_BATCH();

    //4    
    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_DEPTH_SIZE, 2);
    R600_OUT_BATCH(evergreen->DB_DEPTH_SIZE.u32All);                              
    R600_OUT_BATCH(evergreen->DB_DEPTH_SLICE.u32All);
    END_BATCH();    
    
    //3
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_DEPTH_CONTROL, evergreen->DB_DEPTH_CONTROL.u32All);
    END_BATCH();

    //3
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_SHADER_CONTROL, evergreen->DB_SHADER_CONTROL.u32All);  
    END_BATCH();

    //5
    BEGIN_BATCH_NO_AUTOSTATE(5);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_SRESULTS_COMPARE_STATE0, 3);                        
    R600_OUT_BATCH(evergreen->DB_SRESULTS_COMPARE_STATE0.u32All);               
    R600_OUT_BATCH(evergreen->DB_SRESULTS_COMPARE_STATE1.u32All);               
    R600_OUT_BATCH(evergreen->DB_PRELOAD_CONTROL.u32All); 
    END_BATCH();

    //3
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_ALPHA_TO_MASK, evergreen->DB_ALPHA_TO_MASK.u32All);    
    END_BATCH();
    
    rrb = radeon_get_depthbuffer(&context->radeon);
    
	if( (rrb != NULL) && (rrb->bo != NULL) )
    {

	/* make the hw happy */
        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
	    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_HTILE_DATA_BASE, evergreen->DB_HTILE_DATA_BASE.u32All);
	    R600_OUT_BATCH_RELOC(evergreen->DB_HTILE_DATA_BASE.u32All,
			     rrb->bo,
			     evergreen->DB_HTILE_DATA_BASE.u32All,
			     0, RADEON_GEM_DOMAIN_VRAM, 0);
	    END_BATCH();

        //5
        BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
        EVERGREEN_OUT_BATCH_REGVAL(EG_DB_Z_INFO, evergreen->DB_Z_INFO.u32All);
        R600_OUT_BATCH_RELOC(evergreen->DB_Z_INFO.u32All,
        		     rrb->bo,
        		     evergreen->DB_Z_INFO.u32All,
        		     0, RADEON_GEM_DOMAIN_VRAM, 0);
        END_BATCH();

        //5
        if((evergreen->DB_DEPTH_CONTROL.u32All & Z_ENABLE_bit) > 0)
        {
            BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
            EVERGREEN_OUT_BATCH_REGVAL(EG_DB_Z_READ_BASE, evergreen->DB_Z_READ_BASE.u32All);	    	
	        R600_OUT_BATCH_RELOC(evergreen->DB_Z_READ_BASE.u32All,
			                     rrb->bo,
			                     evergreen->DB_Z_READ_BASE.u32All,
			                     0, RADEON_GEM_DOMAIN_VRAM, 0);
            END_BATCH();
        }
        //5
        if((evergreen->DB_DEPTH_CONTROL.u32All & Z_WRITE_ENABLE_bit) > 0)
        {
            BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
            EVERGREEN_OUT_BATCH_REGVAL(EG_DB_Z_WRITE_BASE, evergreen->DB_Z_READ_BASE.u32All);	
	        R600_OUT_BATCH_RELOC(evergreen->DB_Z_WRITE_BASE.u32All,
			                     rrb->bo,
			                     evergreen->DB_Z_WRITE_BASE.u32All,
			                     0, RADEON_GEM_DOMAIN_VRAM, 0);
            END_BATCH();        
        }
	}    

    if (ctx->DrawBuffer) 
    {
		rrb	= radeon_get_renderbuffer(ctx->DrawBuffer, BUFFER_STENCIL);
		
        if((rrb != NULL) && (rrb->bo != NULL))
        {                 
            //5
            BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
            EVERGREEN_OUT_BATCH_REGVAL(EG_DB_STENCIL_INFO, evergreen->DB_STENCIL_INFO.u32All);
            R600_OUT_BATCH_RELOC(evergreen->DB_STENCIL_INFO.u32All,
            		     rrb->bo,
            		     evergreen->DB_STENCIL_INFO.u32All,
            		     0, RADEON_GEM_DOMAIN_VRAM, 0);
            END_BATCH();
            
            //4
            BEGIN_BATCH_NO_AUTOSTATE(4);
	        R600_OUT_BATCH_REGSEQ(DB_STENCILREFMASK, 2);
	        R600_OUT_BATCH(evergreen->DB_STENCILREFMASK.u32All);
	        R600_OUT_BATCH(evergreen->DB_STENCILREFMASK_BF.u32All);
	        END_BATCH();
            //------------------------

            //10
            if((evergreen->DB_DEPTH_CONTROL.u32All & STENCIL_ENABLE_bit) > 0)
            {

                BEGIN_BATCH_NO_AUTOSTATE(3 + 2);                
                EVERGREEN_OUT_BATCH_REGVAL(EG_DB_STENCIL_READ_BASE, evergreen->DB_STENCIL_READ_BASE.u32All);	
	            R600_OUT_BATCH_RELOC(evergreen->DB_STENCIL_READ_BASE.u32All,
			                         rrb->bo,
			                         evergreen->DB_STENCIL_READ_BASE.u32All,
			                         0, RADEON_GEM_DOMAIN_VRAM, 0);
                END_BATCH();

                BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
                EVERGREEN_OUT_BATCH_REGVAL(EG_DB_STENCIL_WRITE_BASE, evergreen->DB_STENCIL_WRITE_BASE.u32All);	
	            R600_OUT_BATCH_RELOC(evergreen->DB_STENCIL_WRITE_BASE.u32All,
			                         rrb->bo,
			                         evergreen->DB_STENCIL_WRITE_BASE.u32All,
			                         0, RADEON_GEM_DOMAIN_VRAM, 0);
                END_BATCH();                   
            }     
        }
	}   
    
    COMMIT_BATCH();
}

static void evergreenSetRenderTarget(context_t *context, int id)
{
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    uint32_t format = COLOR_8_8_8_8, comp_swap = SWAP_ALT, number_type = NUMBER_UNORM, source_format = 1;
    struct radeon_renderbuffer *rrb;
    unsigned int nPitchInPixel, height;

    rrb = radeon_get_colorbuffer(&context->radeon);
    if (!rrb || !rrb->bo) {
	    return;
    }

    EVERGREEN_STATECHANGE(context, cb);

    /* addr */
    evergreen->render_target[id].CB_COLOR0_BASE.u32All = context->radeon.state.color.draw_offset / 256;

    /* pitch */
    nPitchInPixel = rrb->pitch/rrb->cpp;    

    if (context->radeon.radeonScreen->driScreen->dri2.enabled)
    {
        height = rrb->base.Height;
    }
    else
    {
        height =  context->radeon.radeonScreen->driScreen->fbHeight;
    }

    SETfield(evergreen->render_target[id].CB_COLOR0_PITCH.u32All, (nPitchInPixel/8)-1,
             EG_CB_COLOR0_PITCH__TILE_MAX_shift, 
             EG_CB_COLOR0_PITCH__TILE_MAX_mask);

    /* slice */
    SETfield(evergreen->render_target[id].CB_COLOR0_SLICE.u32All, 
             ( (nPitchInPixel * height)/64 )-1,
             EG_CB_COLOR0_SLICE__TILE_MAX_shift, 
             EG_CB_COLOR0_SLICE__TILE_MAX_mask);

    /* CB_COLOR0_ATTRIB */ /* TODO : for z clear, this should be set to 0 */
    SETbit(evergreen->render_target[id].CB_COLOR0_ATTRIB.u32All, 
           EG_CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit);

    /* CB_COLOR0_INFO */
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All, 
             ENDIAN_NONE, 
             EG_CB_COLOR0_INFO__ENDIAN_shift, 
             EG_CB_COLOR0_INFO__ENDIAN_mask);
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All, 
             ARRAY_LINEAR_GENERAL, 
             EG_CB_COLOR0_INFO__ARRAY_MODE_shift, 
             EG_CB_COLOR0_INFO__ARRAY_MODE_mask);   

    switch (rrb->base.Format) {
    case MESA_FORMAT_RGBA8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_SIGNED_RGBA8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_SNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGBA8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_SIGNED_RGBA8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_SNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB8888_REV:
    case MESA_FORMAT_XRGB8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB565:
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB565_REV:
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB4444:
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB4444_REV:
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB1555:
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB1555_REV:
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_AL88:
            format = COLOR_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_AL88_REV:
            format = COLOR_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB332:
            format = COLOR_3_3_2;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_A8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_I8:
    case MESA_FORMAT_CI8:
            format = COLOR_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_L8:
            format = COLOR_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGBA_FLOAT32:
            format = COLOR_32_32_32_32_FLOAT;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_RGBA_FLOAT16:
            format = COLOR_16_16_16_16_FLOAT;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_ALPHA_FLOAT32:
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_ALPHA_FLOAT16:
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT32:
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT16:
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
            format = COLOR_32_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
            format = COLOR_16_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 0;
            break;
    case MESA_FORMAT_X8_Z24:
    case MESA_FORMAT_S8_Z24:
            format = COLOR_8_24;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z24_S8:
            format = COLOR_24_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z16:
            format = COLOR_16;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z32:
            format = COLOR_32;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_SARGB8:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    case MESA_FORMAT_SLA8:
            format = COLOR_8_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    case MESA_FORMAT_SL8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    default:
	    _mesa_problem(context->radeon.glCtx, "unexpected format in evergreenSetRenderTarget()");
	    break;
    }

    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
	     format,
	     EG_CB_COLOR0_INFO__FORMAT_shift,
	     EG_CB_COLOR0_INFO__FORMAT_mask);
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
	     comp_swap,
	     EG_CB_COLOR0_INFO__COMP_SWAP_shift,
	     EG_CB_COLOR0_INFO__COMP_SWAP_mask);
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
             number_type,
             EG_CB_COLOR0_INFO__NUMBER_TYPE_shift,
             EG_CB_COLOR0_INFO__NUMBER_TYPE_mask);
    SETfield(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
             source_format,
             EG_CB_COLOR0_INFO__SOURCE_FORMAT_shift,
             EG_CB_COLOR0_INFO__SOURCE_FORMAT_mask);
    SETbit(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
           EG_CB_COLOR0_INFO__BLEND_CLAMP_bit);

    evergreen->render_target[id].CB_COLOR0_VIEW.u32All        = 0;
    evergreen->render_target[id].CB_COLOR0_CMASK.u32All       = 0;
    evergreen->render_target[id].CB_COLOR0_FMASK.u32All       = 0;
    evergreen->render_target[id].CB_COLOR0_FMASK_SLICE.u32All = 0;

    evergreen->render_target[id].enabled = GL_TRUE;
}

static void evergreenSendCB(struct gl_context *ctx, struct radeon_state_atom *atom)
{    
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	struct radeon_renderbuffer *rrb;
	BATCH_LOCALS(&context->radeon);
	int id = 0;
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

	rrb = radeon_get_colorbuffer(&context->radeon);
	if (!rrb || !rrb->bo) {
		return;
	}

	evergreenSetRenderTarget(context, 0);

	if (!evergreen->render_target[id].enabled)
		return;

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_BASE + (4 * id), 1);
	R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_BASE.u32All);
	R600_OUT_BATCH_RELOC(evergreen->render_target[id].CB_COLOR0_BASE.u32All,
			             rrb->bo,
			             evergreen->render_target[id].CB_COLOR0_BASE.u32All,
			             0, RADEON_GEM_DOMAIN_VRAM, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGVAL(EG_CB_COLOR0_INFO, evergreen->render_target[id].CB_COLOR0_INFO.u32All);
    R600_OUT_BATCH_RELOC(evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		                 rrb->bo,
		                 evergreen->render_target[id].CB_COLOR0_INFO.u32All,
		                 0, RADEON_GEM_DOMAIN_VRAM, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(5);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_PITCH, 3);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_PITCH.u32All); 
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_SLICE.u32All); 
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_VIEW.u32All);  
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_ATTRIB, 1);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_ATTRIB.u32All); 
    R600_OUT_BATCH_RELOC(0,
			 rrb->bo,
			 0,
			 0, RADEON_GEM_DOMAIN_VRAM, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_DIM, 1);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_DIM.u32All); 
    /*
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_CMASK.u32All);  
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_CMASK_SLICE.u32All);    
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_FMASK.u32All); 
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_FMASK_SLICE.u32All); 
    */
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_TARGET_MASK, 2);
    R600_OUT_BATCH(evergreen->CB_TARGET_MASK.u32All);   
    R600_OUT_BATCH(evergreen->CB_SHADER_MASK.u32All);   
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_BLEND_RED, 4);
    R600_OUT_BATCH(evergreen->CB_BLEND_RED.u32All);    
    R600_OUT_BATCH(evergreen->CB_BLEND_GREEN.u32All);  
    R600_OUT_BATCH(evergreen->CB_BLEND_BLUE.u32All);   
    R600_OUT_BATCH(evergreen->CB_BLEND_ALPHA.u32All);  
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    EVERGREEN_OUT_BATCH_REGVAL(EG_CB_BLEND0_CONTROL, evergreen->CB_BLEND0_CONTROL.u32All);  
    EVERGREEN_OUT_BATCH_REGVAL(EG_CB_COLOR_CONTROL, evergreen->CB_COLOR_CONTROL.u32All);  
    END_BATCH();
    
    COMMIT_BATCH();
}

static void evergreenSendVGT(struct gl_context *ctx, struct radeon_state_atom *atom)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
	EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
	BATCH_LOCALS(&context->radeon);
	radeon_print(RADEON_STATE, RADEON_VERBOSE, "%s\n", __func__);

/* moved to draw:
    VGT_DRAW_INITIATOR             
    VGT_INDEX_TYPE                 
    VGT_PRIMITIVE_TYPE             
*/
    BEGIN_BATCH_NO_AUTOSTATE(5);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_MAX_VTX_INDX, 3);
    R600_OUT_BATCH(evergreen->VGT_MAX_VTX_INDX.u32All);    
    R600_OUT_BATCH(evergreen->VGT_MIN_VTX_INDX.u32All);    
    R600_OUT_BATCH(evergreen->VGT_INDX_OFFSET.u32All);     
    END_BATCH();
    
    BEGIN_BATCH_NO_AUTOSTATE(6);
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_OUTPUT_PATH_CNTL, evergreen->VGT_OUTPUT_PATH_CNTL.u32All); 
    
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_GS_MODE, evergreen->VGT_GS_MODE.u32All); 
    END_BATCH();
    
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_PRIMITIVEID_EN, 1);
    R600_OUT_BATCH(evergreen->VGT_PRIMITIVEID_EN.u32All);   
    END_BATCH();
    
    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_INSTANCE_STEP_RATE_0, 2);
    R600_OUT_BATCH(evergreen->VGT_INSTANCE_STEP_RATE_0.u32All); 
    R600_OUT_BATCH(evergreen->VGT_INSTANCE_STEP_RATE_1.u32All); 
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_REUSE_OFF, 2);
    R600_OUT_BATCH(evergreen->VGT_REUSE_OFF.u32All);      
    R600_OUT_BATCH(evergreen->VGT_VTX_CNT_EN.u32All);     
    END_BATCH();
    
    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_SHADER_STAGES_EN, evergreen->VGT_SHADER_STAGES_EN.u32All);  
    END_BATCH();
    
    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_STRMOUT_CONFIG, 2);
    R600_OUT_BATCH(evergreen->VGT_STRMOUT_CONFIG.u32All); 
    R600_OUT_BATCH(evergreen->VGT_STRMOUT_BUFFER_CONFIG.u32All);  
    END_BATCH();

    COMMIT_BATCH();
}

void evergreenInitAtoms(context_t *context)
{        
    radeon_print(RADEON_STATE, RADEON_NORMAL, "%s %p\n", __func__, context);
    context->radeon.hw.max_state_size = 10 + 5 + 14 + 3; /* start 3d, idle, cb/db flush, 3 for time stamp */

    /* Setup the atom linked list */
    make_empty_list(&context->radeon.hw.atomlist);
    context->radeon.hw.atomlist.name = "atom-list";

    EVERGREEN_ALLOC_STATE(init, always, 19, evergreenSendSQConfig);
    EVERGREEN_ALLOC_STATE(vtx,       evergreen_vtx, (VERT_ATTRIB_MAX * 12), evergreenSendVTX);
    EVERGREEN_ALLOC_STATE(pa,        always,        124, evergreenSendPA);
    EVERGREEN_ALLOC_STATE(tp,        always,        0,   evergreenSendTP);
    EVERGREEN_ALLOC_STATE(sq,        always,        86,  evergreenSendSQ); /* 85 */
    EVERGREEN_ALLOC_STATE(vs,        always,        16,  evergreenSendVSresource);
    EVERGREEN_ALLOC_STATE(spi,       always,        59,  evergreenSendSPI);
    EVERGREEN_ALLOC_STATE(sx,        always,        9,   evergreenSendSX);
    EVERGREEN_ALLOC_STATE(tx,        evergreen_tx,  (R700_TEXTURE_NUMBERUNITS * (21+5) + 6), evergreenSendTexState); /* 21 for resource, 5 for sampler */
    EVERGREEN_ALLOC_STATE(db,        always,        69,  evergreenSendDB); 
    EVERGREEN_ALLOC_STATE(cb,        always,        37,  evergreenSendCB);	
    EVERGREEN_ALLOC_STATE(vgt,       always,        29,  evergreenSendVGT);

    evergreen_init_query_stateobj(&context->radeon, 6 * 2);

    context->radeon.hw.is_dirty = GL_TRUE;
    context->radeon.hw.all_dirty = GL_TRUE;
}
