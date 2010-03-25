/**************************************************************************

Copyright (C) 2005  Aapo Tahkola <aet@rasterburn.org>
Copyright (C) 2008  Oliver McFadden <z3ro.geek@gmail.com>

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* Radeon R5xx Acceleration, Revision 1.2 */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/program.h"
#include "shader/programopt.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "tnl/tnl.h"

#include "compiler/radeon_compiler.h"
#include "radeon_mesa_to_rc.h"
#include "r300_context.h"
#include "r300_fragprog_common.h"
#include "r300_state.h"

/**
 * Write parameter array for the given vertex program into dst.
 * Return the total number of components written.
 */
static int r300VertexProgUpdateParams(GLcontext * ctx, struct r300_vertex_program *vp, float *dst)
{
	int i;

	if (vp->Base->IsNVProgram) {
		_mesa_load_tracked_matrices(ctx);
	} else {
		if (vp->Base->Base.Parameters) {
			_mesa_load_state_parameters(ctx, vp->Base->Base.Parameters);
		}
	}

	for(i = 0; i < vp->code.constants.Count; ++i) {
		const float * src = 0;
		const struct rc_constant * constant = &vp->code.constants.Constants[i];

		switch(constant->Type) {
		case RC_CONSTANT_EXTERNAL:
			if (vp->Base->IsNVProgram) {
				src = ctx->VertexProgram.Parameters[constant->u.External];
			} else {
				src = vp->Base->Base.Parameters->ParameterValues[constant->u.External];
			}
			break;

		case RC_CONSTANT_IMMEDIATE:
			src = constant->u.Immediate;
			break;
		}

		assert(src);
		dst[4*i] = src[0];
		dst[4*i + 1] = src[1];
		dst[4*i + 2] = src[2];
		dst[4*i + 3] = src[3];
	}

	return 4 * vp->code.constants.Count;
}

static GLbitfield compute_required_outputs(struct gl_vertex_program * vp, GLbitfield fpreads)
{
	GLbitfield outputs = 0;
	int i;

#define ADD_OUTPUT(fp_attr, vp_result) \
	do { \
		if (fpreads & (1 << (fp_attr))) \
			outputs |= (1 << (vp_result)); \
	} while (0)

	ADD_OUTPUT(FRAG_ATTRIB_COL0, VERT_RESULT_COL0);
	ADD_OUTPUT(FRAG_ATTRIB_COL1, VERT_RESULT_COL1);

	for (i = 0; i <= 7; ++i) {
		ADD_OUTPUT(FRAG_ATTRIB_TEX0 + i, VERT_RESULT_TEX0 + i);
	}

#undef ADD_OUTPUT

	if ((fpreads & (1 << FRAG_ATTRIB_COL0)) &&
	    (vp->Base.OutputsWritten & (1 << VERT_RESULT_BFC0)))
		outputs |= 1 << VERT_RESULT_BFC0;
	if ((fpreads & (1 << FRAG_ATTRIB_COL1)) &&
	    (vp->Base.OutputsWritten & (1 << VERT_RESULT_BFC1)))
		outputs |= 1 << VERT_RESULT_BFC1;

	outputs |= 1 << VERT_RESULT_HPOS;
	if (vp->Base.OutputsWritten & (1 << VERT_RESULT_PSIZ))
		outputs |= 1 << VERT_RESULT_PSIZ;

	return outputs;
}


static void t_inputs_outputs(struct r300_vertex_program_compiler * c)
{
	int i;
	int cur_reg;
	GLuint OutputsWritten, InputsRead;

	OutputsWritten = c->Base.Program.OutputsWritten;
	InputsRead = c->Base.Program.InputsRead;

	cur_reg = -1;
	for (i = 0; i < VERT_ATTRIB_MAX; i++) {
		if (InputsRead & (1 << i))
			c->code->inputs[i] = ++cur_reg;
		else
			c->code->inputs[i] = -1;
	}

	cur_reg = 0;
	for (i = 0; i < VERT_RESULT_MAX; i++)
		c->code->outputs[i] = -1;

	assert(OutputsWritten & (1 << VERT_RESULT_HPOS));

	if (OutputsWritten & (1 << VERT_RESULT_HPOS)) {
		c->code->outputs[VERT_RESULT_HPOS] = cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ)) {
		c->code->outputs[VERT_RESULT_PSIZ] = cur_reg++;
	}

	/* If we're writing back facing colors we need to send
	 * four colors to make front/back face colors selection work.
	 * If the vertex program doesn't write all 4 colors, lets
	 * pretend it does by skipping output index reg so the colors
	 * get written into appropriate output vectors.
	 */
	if (OutputsWritten & (1 << VERT_RESULT_COL0)) {
		c->code->outputs[VERT_RESULT_COL0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_COL1)) {
		c->code->outputs[VERT_RESULT_COL1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		c->code->outputs[VERT_RESULT_BFC0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		c->code->outputs[VERT_RESULT_BFC1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		cur_reg++;
	}

	for (i = VERT_RESULT_TEX0; i <= VERT_RESULT_TEX7; i++) {
		if (OutputsWritten & (1 << i)) {
			c->code->outputs[i] = cur_reg++;
		}
	}

	if (OutputsWritten & (1 << VERT_RESULT_FOGC)) {
		c->code->outputs[VERT_RESULT_FOGC] = cur_reg++;
	}
}

/**
 * The NV_vertex_program spec mandates that all registers be
 * initialized to zero. We do this here unconditionally.
 *
 * \note We rely on dead-code elimination in the compiler.
 */
static void initialize_NV_registers(struct radeon_compiler * compiler)
{
	unsigned int reg;
	struct rc_instruction * inst;

	for(reg = 0; reg < 12; ++reg) {
		inst = rc_insert_new_instruction(compiler, &compiler->Program.Instructions);
		inst->U.I.Opcode = RC_OPCODE_MOV;
		inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
		inst->U.I.DstReg.Index = reg;
		inst->U.I.SrcReg[0].File = RC_FILE_NONE;
		inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
	}

	inst = rc_insert_new_instruction(compiler, &compiler->Program.Instructions);
	inst->U.I.Opcode = RC_OPCODE_ARL;
	inst->U.I.DstReg.File = RC_FILE_ADDRESS;
	inst->U.I.DstReg.Index = 0;
	inst->U.I.DstReg.WriteMask = WRITEMASK_X;
	inst->U.I.SrcReg[0].File = RC_FILE_NONE;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_0000;
}

static struct r300_vertex_program *build_program(GLcontext *ctx,
						 struct r300_vertex_program_key *wanted_key,
						 const struct gl_vertex_program *mesa_vp)
{
	struct r300_vertex_program *vp;
	struct r300_vertex_program_compiler compiler;

	vp = calloc(1, sizeof(*vp));
	vp->Base = _mesa_clone_vertex_program(ctx, mesa_vp);
	memcpy(&vp->key, wanted_key, sizeof(vp->key));

	rc_init(&compiler.Base);
	compiler.Base.Debug = (RADEON_DEBUG & RADEON_VERTS) ? GL_TRUE : GL_FALSE;

	compiler.code = &vp->code;
	compiler.RequiredOutputs = compute_required_outputs(vp->Base, vp->key.FpReads);
	compiler.SetHwInputOutput = &t_inputs_outputs;

	if (compiler.Base.Debug) {
		fprintf(stderr, "Initial vertex program:\n");
		_mesa_print_program(&vp->Base->Base);
		fflush(stderr);
	}

	if (mesa_vp->IsPositionInvariant) {
		_mesa_insert_mvp_code(ctx, vp->Base);
	}

	radeon_mesa_to_rc_program(&compiler.Base, &vp->Base->Base);

	if (mesa_vp->IsNVProgram)
		initialize_NV_registers(&compiler.Base);

	rc_move_output(&compiler.Base, VERT_RESULT_PSIZ, VERT_RESULT_PSIZ, WRITEMASK_X);

	if (vp->key.WPosAttr != FRAG_ATTRIB_MAX) {
		unsigned int vp_wpos_attr = vp->key.WPosAttr - FRAG_ATTRIB_TEX0 + VERT_RESULT_TEX0;

		/* Set empty writemask for instructions writing to vp_wpos_attr
		 * before moving the wpos attr there.
		 * Such instructions will be removed by DCE.
		 */
		rc_move_output(&compiler.Base, vp_wpos_attr, vp->key.WPosAttr, 0);
		rc_copy_output(&compiler.Base, VERT_RESULT_HPOS, vp_wpos_attr);
	}

	if (vp->key.FogAttr != FRAG_ATTRIB_MAX) {
		unsigned int vp_fog_attr = vp->key.FogAttr - FRAG_ATTRIB_TEX0 + VERT_RESULT_TEX0;

		/* Set empty writemask for instructions writing to vp_fog_attr
		 * before moving the fog attr there.
		 * Such instructions will be removed by DCE.
		 */
		rc_move_output(&compiler.Base, vp_fog_attr, vp->key.FogAttr, 0);
		rc_move_output(&compiler.Base, VERT_RESULT_FOGC, vp_fog_attr, WRITEMASK_X);
	}

	r3xx_compile_vertex_program(&compiler);

	if (vp->code.constants.Count > ctx->Const.VertexProgram.MaxParameters) {
		rc_error(&compiler.Base, "Program exceeds constant buffer size limit\n");
	}

	vp->error = compiler.Base.Error;

	vp->Base->Base.InputsRead = vp->code.InputsRead;
	vp->Base->Base.OutputsWritten = vp->code.OutputsWritten;

	rc_destroy(&compiler.Base);

	return vp;
}

struct r300_vertex_program * r300SelectAndTranslateVertexShader(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_program_key wanted_key = { 0 };
	struct r300_vertex_program_cont *vpc;
	struct r300_vertex_program *vp;

	vpc = (struct r300_vertex_program_cont *)ctx->VertexProgram._Current;

	if (!r300->selected_fp) {
		/* This can happen when GetProgramiv is called to check
		 * whether the program runs natively.
		 *
		 * To be honest, this is not a very good solution,
		 * but solving the problem of reporting good values
		 * for those queries is tough anyway considering that
		 * we recompile vertex programs based on the precise
		 * fragment program that is in use.
		 */
		r300SelectAndTranslateFragmentShader(ctx);
	}

	assert(r300->selected_fp);
	wanted_key.FpReads = r300->selected_fp->InputsRead;
	wanted_key.FogAttr = r300->selected_fp->fog_attr;
	wanted_key.WPosAttr = r300->selected_fp->wpos_attr;

	for (vp = vpc->progs; vp; vp = vp->next) {
		if (memcmp(&vp->key, &wanted_key, sizeof(wanted_key)) == 0) {
			return r300->selected_vp = vp;
		}
	}

	vp = build_program(ctx, &wanted_key, &vpc->mesa_program);
	vp->next = vpc->progs;
	vpc->progs = vp;

	return r300->selected_vp = vp;
}

#define bump_vpu_count(ptr, new_count)   do { \
		drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr)); \
		int _nc=(new_count)/4; \
		if(_nc>_p->vpu.count)_p->vpu.count=_nc; \
	} while(0)

static void r300EmitVertexProgram(r300ContextPtr r300, int dest, struct r300_vertex_program_code *code)
{
	int i;

	assert((code->length > 0) && (code->length % 4 == 0));

	switch ((dest >> 8) & 0xf) {
		case 0:
			R300_STATECHANGE(r300, vpi);
			for (i = 0; i < code->length; i++)
				r300->hw.vpi.cmd[R300_VPI_INSTR_0 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vpi.cmd, code->length + 4 * (dest & 0xff));
			break;
		case 2:
			R300_STATECHANGE(r300, vpp);
			for (i = 0; i < code->length; i++)
				r300->hw.vpp.cmd[R300_VPP_PARAM_0 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vpp.cmd, code->length + 4 * (dest & 0xff));
			break;
		case 4:
			R300_STATECHANGE(r300, vps);
			for (i = 0; i < code->length; i++)
				r300->hw.vps.cmd[1 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vps.cmd, code->length + 4 * (dest & 0xff));
			break;
		default:
			fprintf(stderr, "%s:%s don't know how to handle dest %04x\n", __FILE__, __FUNCTION__, dest);
			exit(-1);
	}
}

void r300SetupVertexProgram(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	struct r300_vertex_program *prog = rmesa->selected_vp;
	int inst_count = 0;
	int param_count = 0;

	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t *) rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vps.cmd)->vpu.count = 0;

	R300_STATECHANGE(rmesa, vap_cntl);
	R300_STATECHANGE(rmesa, vpp);
	param_count = r300VertexProgUpdateParams(ctx, prog, (float *)&rmesa->hw.vpp.cmd[R300_VPP_PARAM_0]);
	if (!rmesa->radeon.radeonScreen->kernel_mm && param_count > 255 * 4) {
		WARN_ONCE("Too many VP params, expect rendering errors\n");
	}
	/* Prevent the overflow (vpu.count is u8) */
	bump_vpu_count(rmesa->hw.vpp.cmd, MIN2(255 * 4, param_count));
	param_count /= 4;

	r300EmitVertexProgram(rmesa, R300_PVS_CODE_START, &(prog->code));
	inst_count = (prog->code.length / 4) - 1;

	r300VapCntl(rmesa, _mesa_bitcount(prog->code.InputsRead),
				 _mesa_bitcount(prog->code.OutputsWritten), prog->code.num_temporaries);

	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1] = (0 << R300_PVS_FIRST_INST_SHIFT) | (inst_count << R300_PVS_XYZW_VALID_INST_SHIFT) |
				(inst_count << R300_PVS_LAST_INST_SHIFT);

	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2] = (0 << R300_PVS_CONST_BASE_OFFSET_SHIFT) | ((param_count - 1) << R300_PVS_MAX_CONST_ADDR_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3] = (inst_count << R300_PVS_LAST_VTX_SRC_INST_SHIFT);
}
