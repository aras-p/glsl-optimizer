/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
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
 *   CooperYuan <cooper.yuan@amd.com>, <cooperyuan@gmail.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "main/imports.h"

#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "program/program.h"

#include "r600_context.h"
#include "r600_cmdbuf.h"
#include "r600_emit.h"

#include "evergreen_vertprog.h"
#include "evergreen_fragprog.h"

void evergreen_insert_wpos_code(struct gl_context *ctx, struct gl_fragment_program *fprog)
{
    static const gl_state_index winstate[STATE_LENGTH]
         = { STATE_INTERNAL, STATE_FB_SIZE, 0, 0, 0};
    struct prog_instruction *newInst, *inst;
    GLint  win_size;  /* state reference */
    GLuint wpos_temp; /* temp register */
    int i, j;

    /* PARAM win_size = STATE_FB_SIZE */
    win_size = _mesa_add_state_reference(fprog->Base.Parameters, winstate);

    wpos_temp = fprog->Base.NumTemporaries++;

    /* scan program where WPOS is used and replace with wpos_temp */
    inst = fprog->Base.Instructions;
    for (i = 0; i < fprog->Base.NumInstructions; i++) {
        for (j=0; j < 3; j++) {
            if(inst->SrcReg[j].File == PROGRAM_INPUT && 
               inst->SrcReg[j].Index == FRAG_ATTRIB_WPOS) {
                inst->SrcReg[j].File = PROGRAM_TEMPORARY;
                inst->SrcReg[j].Index = wpos_temp;
            }
        }
        inst++;
    }

    _mesa_insert_instructions(&(fprog->Base), 0, 1);

    newInst = fprog->Base.Instructions;
    /* invert wpos.y
     * wpos_temp.xyzw = wpos.x-yzw + winsize.0y00 */
    newInst[0].Opcode = OPCODE_ADD;
    newInst[0].DstReg.File = PROGRAM_TEMPORARY;
    newInst[0].DstReg.Index = wpos_temp;
    newInst[0].DstReg.WriteMask = WRITEMASK_XYZW;

    newInst[0].SrcReg[0].File = PROGRAM_INPUT;
    newInst[0].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
    newInst[0].SrcReg[0].Swizzle = SWIZZLE_XYZW;
    newInst[0].SrcReg[0].Negate = NEGATE_Y;

    newInst[0].SrcReg[1].File = PROGRAM_STATE_VAR;
    newInst[0].SrcReg[1].Index = win_size;
    newInst[0].SrcReg[1].Swizzle = MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ZERO);

}

//TODO : Validate FP input with VP output.
void evergreen_Map_Fragment_Program(r700_AssemblerBase         *pAsm,
						  struct gl_fragment_program *mesa_fp,
                          struct gl_context *ctx) 
{
	unsigned int unBit;
    unsigned int i;

    /* match fp inputs with vp exports. */
    struct evergreen_vertex_program_cont *vpc =
		       (struct evergreen_vertex_program_cont *)ctx->VertexProgram._Current;
    GLbitfield OutputsWritten = vpc->mesa_program.Base.OutputsWritten;
    
	pAsm->number_used_registers = 0;

//Input mapping : mesa_fp->Base.InputsRead set the flag, set in 
	//The flags parsed in parse_attrib_binding. FRAG_ATTRIB_COLx, FRAG_ATTRIB_TEXx, ...
	//MUST match order in Map_Vertex_Output
	unBit = 1 << FRAG_ATTRIB_WPOS;
	if(mesa_fp->Base.InputsRead & unBit)
	{
		pAsm->uiFP_AttributeMap[FRAG_ATTRIB_WPOS] = pAsm->number_used_registers++;
	}

    unBit = 1 << VERT_RESULT_COL0;
	if(OutputsWritten & unBit)
	{
		pAsm->uiFP_AttributeMap[FRAG_ATTRIB_COL0] = pAsm->number_used_registers++;
	}

	unBit = 1 << VERT_RESULT_COL1;
	if(OutputsWritten & unBit)
	{
		pAsm->uiFP_AttributeMap[FRAG_ATTRIB_COL1] = pAsm->number_used_registers++;
	}

    unBit = 1 << VERT_RESULT_FOGC;
    if(OutputsWritten & unBit)
    {
        pAsm->uiFP_AttributeMap[FRAG_ATTRIB_FOGC] = pAsm->number_used_registers++;
    }

	for(i=0; i<8; i++)
	{
		unBit = 1 << (VERT_RESULT_TEX0 + i);
		if(OutputsWritten & unBit)
		{
			pAsm->uiFP_AttributeMap[FRAG_ATTRIB_TEX0 + i] = pAsm->number_used_registers++;
		}
	}
 
/* order has been taken care of */ 
#if 1
    for(i=VERT_RESULT_VAR0; i<VERT_RESULT_MAX; i++)
	{
        unBit = 1 << i;
        if(OutputsWritten & unBit)
		{
            pAsm->uiFP_AttributeMap[i-VERT_RESULT_VAR0+FRAG_ATTRIB_VAR0] = pAsm->number_used_registers++;
        }
    }
#else
    if( (mesa_fp->Base.InputsRead >> FRAG_ATTRIB_VAR0) > 0 )
    {
	    struct evergreen_vertex_program_cont *vpc =
		       (struct evergreen_vertex_program_cont *)ctx->VertexProgram._Current;
        struct gl_program_parameter_list * VsVarying = vpc->mesa_program.Base.Varying;
        struct gl_program_parameter_list * PsVarying = mesa_fp->Base.Varying;
        struct gl_program_parameter      * pVsParam;
        struct gl_program_parameter      * pPsParam;
        GLuint j, k;
        GLuint unMaxVarying = 0;

        for(i=0; i<VsVarying->NumParameters; i++)
        {
            pAsm->uiFP_AttributeMap[i + FRAG_ATTRIB_VAR0] = 0;
        }

        for(i=FRAG_ATTRIB_VAR0; i<FRAG_ATTRIB_MAX; i++)
	    {
            unBit = 1 << i;
            if(mesa_fp->Base.InputsRead & unBit)
		    {
                j = i - FRAG_ATTRIB_VAR0;
                pPsParam = PsVarying->Parameters + j;

                for(k=0; k<VsVarying->NumParameters; k++)
                {					
                    pVsParam = VsVarying->Parameters + k;

			        if( strcmp(pPsParam->Name, pVsParam->Name) == 0)
                    {
                        pAsm->uiFP_AttributeMap[i] = pAsm->number_used_registers + k;                  
                        if(k > unMaxVarying)
                        {
                            unMaxVarying = k;
                        }
                        break;
                    }
                }
		    }
        }

        pAsm->number_used_registers += unMaxVarying + 1;
    }
#endif
    unBit = 1 << FRAG_ATTRIB_FACE;
    if(mesa_fp->Base.InputsRead & unBit)
    {
        pAsm->uiFP_AttributeMap[FRAG_ATTRIB_FACE] = pAsm->number_used_registers++;
    }

    unBit = 1 << FRAG_ATTRIB_PNTC;
    if(mesa_fp->Base.InputsRead & unBit)
    {
        pAsm->uiFP_AttributeMap[FRAG_ATTRIB_PNTC] = pAsm->number_used_registers++;
    }

    pAsm->uIIns = pAsm->number_used_registers;

/* Map temporary registers (GPRs) */
    pAsm->starting_temp_register_number = pAsm->number_used_registers;

    if(mesa_fp->Base.NumNativeTemporaries >= mesa_fp->Base.NumTemporaries)
    {
	    pAsm->number_used_registers += mesa_fp->Base.NumNativeTemporaries;
    }
    else
    {
        pAsm->number_used_registers += mesa_fp->Base.NumTemporaries;
    }

/* Output mapping */
	pAsm->number_of_exports = 0;
	pAsm->number_of_colorandz_exports = 0; /* don't include stencil and mask out. */
	pAsm->starting_export_register_number = pAsm->number_used_registers;
	unBit = 1 << FRAG_RESULT_COLOR;
	if(mesa_fp->Base.OutputsWritten & unBit)
	{
		pAsm->uiFP_OutputMap[FRAG_RESULT_COLOR] = pAsm->number_used_registers++;
		pAsm->number_of_exports++;
		pAsm->number_of_colorandz_exports++;
	}
	unBit = 1 << FRAG_RESULT_DEPTH;
	if(mesa_fp->Base.OutputsWritten & unBit)
	{
        pAsm->depth_export_register_number = pAsm->number_used_registers;
		pAsm->uiFP_OutputMap[FRAG_RESULT_DEPTH] = pAsm->number_used_registers++;
		pAsm->number_of_exports++;
		pAsm->number_of_colorandz_exports++;
		pAsm->pR700Shader->depthIsExported = 1;
	}

    pAsm->flag_reg_index = pAsm->number_used_registers++;

    pAsm->uFirstHelpReg = pAsm->number_used_registers;
}

GLboolean evergreen_Find_Instruction_Dependencies_fp(struct evergreen_fragment_program *fp,
					                	struct gl_fragment_program   *mesa_fp)
{
    GLuint i, j;
    GLint * puiTEMPwrites;
    GLint * puiTEMPreads;
    struct prog_instruction * pILInst;
    InstDeps         *pInstDeps;
    struct prog_instruction * texcoord_DepInst;
    GLint              nDepInstID;

    puiTEMPwrites = (GLint*) MALLOC(sizeof(GLuint)*mesa_fp->Base.NumTemporaries);
    puiTEMPreads = (GLint*) MALLOC(sizeof(GLuint)*mesa_fp->Base.NumTemporaries);

    for(i=0; i<mesa_fp->Base.NumTemporaries; i++)
    {
        puiTEMPwrites[i] = -1;
        puiTEMPreads[i] = -1;
    }

    pInstDeps = (InstDeps*)MALLOC(sizeof(InstDeps)*mesa_fp->Base.NumInstructions);

    for(i=0; i<mesa_fp->Base.NumInstructions; i++)
    {
        pInstDeps[i].nDstDep = -1;
        pILInst = &(mesa_fp->Base.Instructions[i]);

        //Dst
        if(pILInst->DstReg.File == PROGRAM_TEMPORARY)
        {
            //Set lastwrite for the temp
            puiTEMPwrites[pILInst->DstReg.Index] = i;
        }

        //Src
        for(j=0; j<3; j++)
        {
            if(pILInst->SrcReg[j].File == PROGRAM_TEMPORARY)
            {
                //Set dep.
                pInstDeps[i].nSrcDeps[j] = puiTEMPwrites[pILInst->SrcReg[j].Index];
                //Set first read
                if(puiTEMPreads[pILInst->SrcReg[j].Index] < 0 )
                {
                    puiTEMPreads[pILInst->SrcReg[j].Index] = i;
                }
            }
            else
            {
                pInstDeps[i].nSrcDeps[j] = -1;
            }
        }
    }

    fp->r700AsmCode.pInstDeps = pInstDeps;

    //Find dep for tex inst    
    for(i=0; i<mesa_fp->Base.NumInstructions; i++)
    {
        pILInst = &(mesa_fp->Base.Instructions[i]);

        if(GL_TRUE == IsTex(pILInst->Opcode))
        {   //src0 is the tex coord register, src1 is texunit, src2 is textype
            nDepInstID = pInstDeps[i].nSrcDeps[0];
            if(nDepInstID >= 0)
            {
                texcoord_DepInst = &(mesa_fp->Base.Instructions[nDepInstID]);
                if(GL_TRUE == IsAlu(texcoord_DepInst->Opcode) )
                {
                    pInstDeps[nDepInstID].nDstDep = i;
                    pInstDeps[i].nDstDep = i;
                }
                else if(GL_TRUE == IsTex(texcoord_DepInst->Opcode) )
                {
                    pInstDeps[i].nDstDep = i;
                }
                else
                {   //... other deps?
                }
            }
            // make sure that we dont overwrite src used earlier
            nDepInstID = puiTEMPreads[pILInst->DstReg.Index];
            if(nDepInstID < i)
            {
                pInstDeps[i].nDstDep = puiTEMPreads[pILInst->DstReg.Index];
                texcoord_DepInst = &(mesa_fp->Base.Instructions[nDepInstID]);
                if(GL_TRUE == IsAlu(texcoord_DepInst->Opcode) )
                {
                    pInstDeps[nDepInstID].nDstDep = i;
                }
 
            }

        }
	}

    FREE(puiTEMPwrites);
    FREE(puiTEMPreads);

    return GL_TRUE;
}

GLboolean evergreenTranslateFragmentShader(struct evergreen_fragment_program *fp,
							     struct gl_fragment_program   *mesa_fp,
                                 struct gl_context *ctx) 
{
	GLuint    number_of_colors_exported;
	GLboolean z_enabled = GL_FALSE;
	GLuint    unBit, shadow_unit;
	int i;
	struct prog_instruction *inst;
	gl_state_index shadow_ambient[STATE_LENGTH]
	    = { STATE_INTERNAL, STATE_SHADOW_AMBIENT, 0, 0, 0};

    //Init_Program
	Init_r700_AssemblerBase( SPT_FP, &(fp->r700AsmCode), &(fp->r700Shader) );
        
    fp->constbo0 = NULL;
    fp->r700AsmCode.bUseMemConstant = GL_TRUE;  
    fp->r700AsmCode.unAsic = 8;

    if(mesa_fp->Base.InputsRead & FRAG_BIT_WPOS)
    {
        evergreen_insert_wpos_code(ctx, mesa_fp);
    }

    /* add/map  consts for ARB_shadow_ambient */
    if(mesa_fp->Base.ShadowSamplers)
    {
        inst = mesa_fp->Base.Instructions;
        for (i = 0; i < mesa_fp->Base.NumInstructions; i++)
        {
            if(inst->TexShadow == 1)
            {
                shadow_unit = inst->TexSrcUnit;
                shadow_ambient[2] = shadow_unit;
                fp->r700AsmCode.shadow_regs[shadow_unit] = 
                    _mesa_add_state_reference(mesa_fp->Base.Parameters, shadow_ambient);
            }
            inst++;
        }
    }

    evergreen_Map_Fragment_Program(&(fp->r700AsmCode), mesa_fp, ctx); 

    if( GL_FALSE == evergreen_Find_Instruction_Dependencies_fp(fp, mesa_fp) )
	{
		return GL_FALSE;
    }

    InitShaderProgram(&(fp->r700AsmCode));
	
    for(i=0; i < MAX_SAMPLERS; i++)
    {
         fp->r700AsmCode.SamplerUnits[i] = fp->mesa_program.Base.SamplerUnits[i];
    }

    fp->r700AsmCode.unCurNumILInsts = mesa_fp->Base.NumInstructions;

	if( GL_FALSE == AssembleInstr(0,
                                  0,
                                  mesa_fp->Base.NumInstructions,
                                  &(mesa_fp->Base.Instructions[0]), 
                                  &(fp->r700AsmCode)) )
	{
		return GL_FALSE;
	}

    if(GL_FALSE == Process_Fragment_Exports(&(fp->r700AsmCode), mesa_fp->Base.OutputsWritten) )
    {
        return GL_FALSE;
    }

    if( GL_FALSE == RelocProgram(&(fp->r700AsmCode), &(mesa_fp->Base)) )
    {
        return GL_FALSE;
    }

    fp->r700Shader.nRegs = (fp->r700AsmCode.number_used_registers == 0) ? 0 
                         : (fp->r700AsmCode.number_used_registers - 1);

	fp->r700Shader.nParamExports = fp->r700AsmCode.number_of_exports;

	number_of_colors_exported = fp->r700AsmCode.number_of_colorandz_exports;

	unBit = 1 << FRAG_RESULT_DEPTH;
	if(mesa_fp->Base.OutputsWritten & unBit)
	{
		z_enabled = GL_TRUE;
		number_of_colors_exported--;
	}

	/* illegal to set this to 0 */
	if(number_of_colors_exported || z_enabled)
	{
	    fp->r700Shader.exportMode = number_of_colors_exported << 1 | z_enabled;
	}
	else
	{
	    fp->r700Shader.exportMode = (1 << 1);
	}

    fp->translated = GL_TRUE;

	return GL_TRUE;
}

void evergreenSelectFragmentShader(struct gl_context *ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    struct evergreen_fragment_program *fp = (struct evergreen_fragment_program *)
	    (ctx->FragmentProgram._Current);
    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)
    {
	    fp->r700AsmCode.bR6xx = 1;
    }

    if (GL_FALSE == fp->translated)
	    evergreenTranslateFragmentShader(fp, &(fp->mesa_program), ctx); 
}

void * evergreenGetActiveFpShaderBo(struct gl_context * ctx)
{
    struct evergreen_fragment_program *fp = (struct evergreen_fragment_program *)
	                                   (ctx->FragmentProgram._Current);

    return fp->shaderbo;
}

void * evergreenGetActiveFpShaderConstBo(struct gl_context * ctx)
{
    struct evergreen_fragment_program *fp = (struct evergreen_fragment_program *)
	                                   (ctx->FragmentProgram._Current);

    return fp->constbo0;
}

GLboolean evergreenSetupFragmentProgram(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct evergreen_fragment_program *fp = (struct evergreen_fragment_program *)
	                                   (ctx->FragmentProgram._Current);
    r700_AssemblerBase         *pAsm = &(fp->r700AsmCode);
    struct gl_fragment_program *mesa_fp = &(fp->mesa_program);    
    unsigned int ui, i;
    unsigned int unNumOfReg;
    unsigned int unBit;
    GLuint exportCount;
    GLboolean point_sprite = GL_FALSE;

    if(GL_FALSE == fp->loaded)
    {
	    if(fp->r700Shader.bNeedsAssembly == GL_TRUE)
	    {
		    Assemble( &(fp->r700Shader) );
	    }

        r600EmitShader(ctx,
                       &(fp->shaderbo),
                       (GLvoid *)(fp->r700Shader.pProgram),
                       fp->r700Shader.uShaderBinaryDWORDSize,
                       "FS");
        
        fp->loaded = GL_TRUE;
    }

    /* TODO : enable this after MemUse fixed *=
    (context->chipobj.MemUse)(context, fp->shadercode.buf->id);
    */

    EVERGREEN_STATECHANGE(context, sq);

    evergreen->SQ_PGM_RESOURCES_PS.u32All = 0;
    SETbit(evergreen->SQ_PGM_RESOURCES_PS.u32All, PGM_RESOURCES__PRIME_CACHE_ON_DRAW_bit);

    evergreen->ps.SQ_ALU_CONST_CACHE_PS_0.u32All = 0; 
    evergreen->ps.SQ_PGM_START_PS.u32All = 0;         

    EVERGREEN_STATECHANGE(context, spi);

    unNumOfReg = fp->r700Shader.nRegs + 1;

    ui = (evergreen->SPI_PS_IN_CONTROL_0.u32All & NUM_INTERP_mask) / (1 << NUM_INTERP_shift);

    /* PS uses fragment.position */
    if (mesa_fp->Base.InputsRead & (1 << FRAG_ATTRIB_WPOS))
    {
        ui += 1;
        SETfield(evergreen->SPI_PS_IN_CONTROL_0.u32All, ui, NUM_INTERP_shift, NUM_INTERP_mask);
        SETfield(evergreen->SPI_PS_IN_CONTROL_0.u32All, CENTERS_ONLY, BARYC_SAMPLE_CNTL_shift, BARYC_SAMPLE_CNTL_mask);
        SETbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, POSITION_ENA_bit);
        SETbit(evergreen->SPI_INPUT_Z.u32All, PROVIDE_Z_TO_SPI_bit);
    }
    else
    {
        CLEARbit(evergreen->SPI_PS_IN_CONTROL_0.u32All, POSITION_ENA_bit);
        CLEARbit(evergreen->SPI_INPUT_Z.u32All, PROVIDE_Z_TO_SPI_bit);
    }

    if (mesa_fp->Base.InputsRead & (1 << FRAG_ATTRIB_FACE))
    {
        ui += 1;
        SETfield(evergreen->SPI_PS_IN_CONTROL_0.u32All, ui, NUM_INTERP_shift, NUM_INTERP_mask);
        SETbit(evergreen->SPI_PS_IN_CONTROL_1.u32All, FRONT_FACE_ENA_bit);
        SETbit(evergreen->SPI_PS_IN_CONTROL_1.u32All, FRONT_FACE_ALL_BITS_bit);
        SETfield(evergreen->SPI_PS_IN_CONTROL_1.u32All, pAsm->uiFP_AttributeMap[FRAG_ATTRIB_FACE], FRONT_FACE_ADDR_shift, FRONT_FACE_ADDR_mask);
    }
    else
    {
        CLEARbit(evergreen->SPI_PS_IN_CONTROL_1.u32All, FRONT_FACE_ENA_bit);
    }

    /* see if we need any point_sprite replacements */
    for (i = VERT_RESULT_TEX0; i<= VERT_RESULT_TEX7; i++)
    {
        if(ctx->Point.CoordReplace[i - VERT_RESULT_TEX0] == GL_TRUE)
            point_sprite = GL_TRUE;
    }

    if ((mesa_fp->Base.InputsRead & (1 << FRAG_ATTRIB_PNTC)) || point_sprite)
    {
        /* for FRAG_ATTRIB_PNTC we need to increase num_interp */
        if(mesa_fp->Base.InputsRead & (1 << FRAG_ATTRIB_PNTC))
        {
            ui++;
            SETfield(evergreen->SPI_PS_IN_CONTROL_0.u32All, ui, NUM_INTERP_shift, NUM_INTERP_mask);
        }
        SETbit(evergreen->SPI_INTERP_CONTROL_0.u32All, PNT_SPRITE_ENA_bit);
        SETfield(evergreen->SPI_INTERP_CONTROL_0.u32All, SPI_PNT_SPRITE_SEL_S, PNT_SPRITE_OVRD_X_shift, PNT_SPRITE_OVRD_X_mask);
        SETfield(evergreen->SPI_INTERP_CONTROL_0.u32All, SPI_PNT_SPRITE_SEL_T, PNT_SPRITE_OVRD_Y_shift, PNT_SPRITE_OVRD_Y_mask);
        SETfield(evergreen->SPI_INTERP_CONTROL_0.u32All, SPI_PNT_SPRITE_SEL_0, PNT_SPRITE_OVRD_Z_shift, PNT_SPRITE_OVRD_Z_mask);
        SETfield(evergreen->SPI_INTERP_CONTROL_0.u32All, SPI_PNT_SPRITE_SEL_1, PNT_SPRITE_OVRD_W_shift, PNT_SPRITE_OVRD_W_mask);
        if(ctx->Point.SpriteOrigin == GL_LOWER_LEFT)
            SETbit(evergreen->SPI_INTERP_CONTROL_0.u32All, PNT_SPRITE_TOP_1_bit);
        else
            CLEARbit(evergreen->SPI_INTERP_CONTROL_0.u32All, PNT_SPRITE_TOP_1_bit);
    }
    else
    {
        CLEARbit(evergreen->SPI_INTERP_CONTROL_0.u32All, PNT_SPRITE_ENA_bit);
    }


    ui = (unNumOfReg < ui) ? ui : unNumOfReg;

    SETfield(evergreen->SQ_PGM_RESOURCES_PS.u32All, ui, NUM_GPRS_shift, NUM_GPRS_mask);

    CLEARbit(evergreen->SQ_PGM_RESOURCES_PS.u32All, UNCACHED_FIRST_INST_bit);

    if(fp->r700Shader.uStackSize) /* we don't use branch for now, it should be zero. */
	{
        SETfield(evergreen->SQ_PGM_RESOURCES_PS.u32All, fp->r700Shader.uStackSize,
                 STACK_SIZE_shift, STACK_SIZE_mask);
    }

    SETfield(evergreen->SQ_PGM_EXPORTS_PS.u32All, fp->r700Shader.exportMode,
             EXPORT_MODE_shift, EXPORT_MODE_mask);

    // emit ps input map
    struct evergreen_vertex_program_cont *vpc =
		       (struct evergreen_vertex_program_cont *)ctx->VertexProgram._Current;
    GLbitfield OutputsWritten = vpc->mesa_program.Base.OutputsWritten;
    
    for(ui = 0; ui < EVERGREEN_MAX_SHADER_EXPORTS; ui++)
        evergreen->SPI_PS_INPUT_CNTL[ui].u32All = 0;

    unBit = 1 << FRAG_ATTRIB_WPOS;
    if(mesa_fp->Base.InputsRead & unBit)
    {
            ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_WPOS];
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
            SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
                     SEMANTIC_shift, SEMANTIC_mask);
            CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
    }

    unBit = 1 << VERT_RESULT_COL0;
    if(OutputsWritten & unBit)
    {
	    ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_COL0];
	    SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
	    SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
		     SEMANTIC_shift, SEMANTIC_mask);
	    if (evergreen->SPI_INTERP_CONTROL_0.u32All & FLAT_SHADE_ENA_bit)
		    SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
	    else
		    CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
    }

    unBit = 1 << VERT_RESULT_COL1;
    if(OutputsWritten & unBit)
    {
	    ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_COL1];
	    SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
	    SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
		     SEMANTIC_shift, SEMANTIC_mask);
	    if (evergreen->SPI_INTERP_CONTROL_0.u32All & FLAT_SHADE_ENA_bit)
		    SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
	    else
		    CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
    }

    unBit = 1 << VERT_RESULT_FOGC;
    if(OutputsWritten & unBit)
    {
            ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_FOGC];
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
            SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
                     SEMANTIC_shift, SEMANTIC_mask);
            CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
    }

    for(i=0; i<8; i++)
    {
	    unBit = 1 << (VERT_RESULT_TEX0 + i);
	    if(OutputsWritten & unBit)
	    {
		    ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_TEX0 + i];
		    SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
		    SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
			     SEMANTIC_shift, SEMANTIC_mask);
		    CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
		    /* ARB_point_sprite */
		    if(ctx->Point.CoordReplace[i] == GL_TRUE)
		    {
			     SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, PT_SPRITE_TEX_bit);
		    }
	    }
    }

    unBit = 1 << FRAG_ATTRIB_FACE;
    if(mesa_fp->Base.InputsRead & unBit)
    {
            ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_FACE];
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
            SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
                     SEMANTIC_shift, SEMANTIC_mask);
            CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
    }
    unBit = 1 << FRAG_ATTRIB_PNTC;
    if(mesa_fp->Base.InputsRead & unBit)
    {
            ui = pAsm->uiFP_AttributeMap[FRAG_ATTRIB_PNTC];
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
            SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
                     SEMANTIC_shift, SEMANTIC_mask);
            CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, PT_SPRITE_TEX_bit);
    }




    for(i=VERT_RESULT_VAR0; i<VERT_RESULT_MAX; i++)
	{
        unBit = 1 << i;
        if(OutputsWritten & unBit)
		{
            ui = pAsm->uiFP_AttributeMap[i-VERT_RESULT_VAR0+FRAG_ATTRIB_VAR0];
            SETbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, SEL_CENTROID_bit);
            SETfield(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, ui,
		             SEMANTIC_shift, SEMANTIC_mask);
            CLEARbit(evergreen->SPI_PS_INPUT_CNTL[ui].u32All, FLAT_SHADE_bit);
        }
    }

    exportCount = (evergreen->SQ_PGM_EXPORTS_PS.u32All & EXPORT_MODE_mask) / (1 << EXPORT_MODE_shift);

    return GL_TRUE;
}

GLboolean evergreenSetupFPconstants(struct gl_context * ctx)
{
    context_t *context = EVERGREEN_CONTEXT(ctx);
    EVERGREEN_CHIP_CONTEXT *evergreen = GET_EVERGREEN_CHIP(context);
    struct evergreen_fragment_program *fp = (struct evergreen_fragment_program *)
	                                   (ctx->FragmentProgram._Current);
    r700_AssemblerBase *pAsm = &(fp->r700AsmCode);

    struct gl_program_parameter_list *paramList;
    unsigned int unNumParamData;
    unsigned int ui;
    int alloc_size;

    /* sent out shader constants. */
    paramList = fp->mesa_program.Base.Parameters;

    if(NULL != paramList) 
    {
	    _mesa_load_state_parameters(ctx, paramList);

	    if (paramList->NumParameters > EVERGREEN_MAX_DX9_CONSTS)
		    return GL_FALSE;

	    EVERGREEN_STATECHANGE(context, sq);

	    evergreen->ps.num_consts = paramList->NumParameters;

	    unNumParamData = paramList->NumParameters;

	    for(ui=0; ui<unNumParamData; ui++) {
		        evergreen->ps.consts[ui][0].f32All = paramList->ParameterValues[ui][0];
		        evergreen->ps.consts[ui][1].f32All = paramList->ParameterValues[ui][1];
		        evergreen->ps.consts[ui][2].f32All = paramList->ParameterValues[ui][2];
		        evergreen->ps.consts[ui][3].f32All = paramList->ParameterValues[ui][3];
	    }

	    /* alloc multiple of 16 constants */
	    alloc_size = ((unNumParamData * 4 * 4) + 255) & ~255;

        /* Load fp constants to gpu */
        if(unNumParamData > 0) 
        {            
            radeonAllocDmaRegion(&context->radeon, 
                                &context->fp_Constbo, 
                                &context->fp_bo_offset, 
                                alloc_size,
                                256);            
            r600EmitShaderConsts(ctx,
                                 context->fp_Constbo,
                                 context->fp_bo_offset,         
                                 (GLvoid *)&(evergreen->ps.consts[0][0]),
                                 unNumParamData * 4 * 4);
        }
    } else
	    evergreen->ps.num_consts = 0;

    COMPILED_SUB * pCompiledSub;
    GLuint uj;
    GLuint unConstOffset = evergreen->ps.num_consts;
    for(ui=0; ui<pAsm->unNumPresub; ui++)
    {
        pCompiledSub = pAsm->presubs[ui].pCompiledSub;

        evergreen->ps.num_consts += pCompiledSub->NumParameters;

        for(uj=0; uj<pCompiledSub->NumParameters; uj++)
        {
            evergreen->ps.consts[uj + unConstOffset][0].f32All = pCompiledSub->ParameterValues[uj][0];
		    evergreen->ps.consts[uj + unConstOffset][1].f32All = pCompiledSub->ParameterValues[uj][1];
		    evergreen->ps.consts[uj + unConstOffset][2].f32All = pCompiledSub->ParameterValues[uj][2];
		    evergreen->ps.consts[uj + unConstOffset][3].f32All = pCompiledSub->ParameterValues[uj][3];
        }
        unConstOffset += pCompiledSub->NumParameters;
    }

    return GL_TRUE;
}

