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
#include "shader/prog_optimize.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "tnl/tnl.h"

#include "compiler/radeon_compiler.h"
#include "compiler/radeon_nqssadce.h"
#include "r300_context.h"
#include "r300_state.h"


static int r300VertexProgUpdateParams(GLcontext * ctx, struct gl_vertex_program *vp, float *dst)
{
	int pi;
	float *dst_o = dst;
	struct gl_program_parameter_list *paramList;

	if (vp->IsNVProgram) {
		_mesa_load_tracked_matrices(ctx);

		for (pi = 0; pi < MAX_NV_VERTEX_PROGRAM_PARAMS; pi++) {
			*dst++ = ctx->VertexProgram.Parameters[pi][0];
			*dst++ = ctx->VertexProgram.Parameters[pi][1];
			*dst++ = ctx->VertexProgram.Parameters[pi][2];
			*dst++ = ctx->VertexProgram.Parameters[pi][3];
		}
		return dst - dst_o;
	}

	if (!vp->Base.Parameters)
		return 0;

	_mesa_load_state_parameters(ctx, vp->Base.Parameters);

	if (vp->Base.Parameters->NumParameters * 4 >
	    VSF_MAX_FRAGMENT_LENGTH) {
		fprintf(stderr, "%s:Params exhausted\n", __FUNCTION__);
		_mesa_exit(-1);
	}

	paramList = vp->Base.Parameters;
	for (pi = 0; pi < paramList->NumParameters; pi++) {
		switch (paramList->Parameters[pi].Type) {
		case PROGRAM_STATE_VAR:
		case PROGRAM_NAMED_PARAM:
			//fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
		case PROGRAM_CONSTANT:
			*dst++ = paramList->ParameterValues[pi][0];
			*dst++ = paramList->ParameterValues[pi][1];
			*dst++ = paramList->ParameterValues[pi][2];
			*dst++ = paramList->ParameterValues[pi][3];
			break;
		default:
			_mesa_problem(NULL, "Bad param type in %s",
				      __FUNCTION__);
		}

	}

	return dst - dst_o;
}

static struct r300_vertex_program *build_program(GLcontext *ctx,
						 struct r300_vertex_program_external_state *wanted_key,
						 const struct gl_vertex_program *mesa_vp)
{
	struct r300_vertex_program *vp;
	struct r300_vertex_program_compiler compiler;

	vp = _mesa_calloc(sizeof(*vp));
	vp->Base = (struct gl_vertex_program *) _mesa_clone_program(ctx, &mesa_vp->Base);
	_mesa_memcpy(&vp->key, wanted_key, sizeof(vp->key));

	rc_init(&compiler.Base);
	compiler.Base.Debug = (RADEON_DEBUG & DEBUG_VERTS) ? GL_TRUE : GL_FALSE;

	compiler.code = &vp->code;
	compiler.state = vp->key;
	compiler.program = vp->Base;

	if (compiler.Base.Debug) {
		fprintf(stderr, "Initial vertex program:\n");
		_mesa_print_program(compiler.program);
		fflush(stdout);
	}

	if (mesa_vp->IsPositionInvariant) {
		_mesa_insert_mvp_code(ctx, (struct gl_vertex_program *)compiler.program);
	}

	if (!r3xx_compile_vertex_program(&compiler))
		vp->error = GL_TRUE;

	rc_destroy(&compiler.Base);

	return vp;
}

struct r300_vertex_program * r300SelectAndTranslateVertexShader(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_program_external_state wanted_key = { 0 };
	struct r300_vertex_program_cont *vpc;
	struct r300_vertex_program *vp;

	vpc = (struct r300_vertex_program_cont *)ctx->VertexProgram._Current;
	wanted_key.FpReads = r300->selected_fp->InputsRead;
	wanted_key.FogAttr = r300->selected_fp->code.fog_attr;
	wanted_key.WPosAttr = r300->selected_fp->code.wpos_attr;

	for (vp = vpc->progs; vp; vp = vp->next) {
		if (_mesa_memcmp(&vp->key, &wanted_key, sizeof(wanted_key))
		    == 0) {
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
		assert(_nc < 256); \
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
			_mesa_exit(-1);
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

	R300_STATECHANGE(rmesa, vpp);
	param_count = r300VertexProgUpdateParams(ctx, prog->Base, (float *)&rmesa->hw.vpp.cmd[R300_VPP_PARAM_0]);
	bump_vpu_count(rmesa->hw.vpp.cmd, param_count);
	param_count /= 4;

	r300EmitVertexProgram(rmesa, R300_PVS_CODE_START, &(prog->code));
	inst_count = (prog->code.length / 4) - 1;

	r300VapCntl(rmesa, _mesa_bitcount(prog->code.InputsRead),
				 _mesa_bitcount(prog->code.OutputsWritten), prog->code.num_temporaries);

	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1] = (0 << R300_PVS_FIRST_INST_SHIFT) | (inst_count << R300_PVS_XYZW_VALID_INST_SHIFT) |
				(inst_count << R300_PVS_LAST_INST_SHIFT);

	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2] = (0 << R300_PVS_CONST_BASE_OFFSET_SHIFT) | (param_count << R300_PVS_MAX_CONST_ADDR_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3] = (inst_count << R300_PVS_LAST_VTX_SRC_INST_SHIFT);
}
