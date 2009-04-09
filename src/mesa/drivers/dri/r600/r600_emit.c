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
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/image.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "r600_context.h"
#include "r600_state.h"
#include "r600_emit.h"
#include "r600_ioctl.h"


#if SWIZZLE_X != R600_INPUT_ROUTE_SELECT_X || \
    SWIZZLE_Y != R600_INPUT_ROUTE_SELECT_Y || \
    SWIZZLE_Z != R600_INPUT_ROUTE_SELECT_Z || \
    SWIZZLE_W != R600_INPUT_ROUTE_SELECT_W || \
    SWIZZLE_ZERO != R600_INPUT_ROUTE_SELECT_ZERO || \
    SWIZZLE_ONE != R600_INPUT_ROUTE_SELECT_ONE
#error Cannot change these!
#endif

#define DEBUG_ALL DEBUG_VERTS

#define DW_SIZE(x) ((inputs[tab[(x)]] << R600_DST_VEC_LOC_SHIFT) |	\
		    (attribptr[tab[(x)]]->size - 1) << R600_DATA_TYPE_0_SHIFT)

GLuint r600VAPInputRoute0(uint32_t * dst, GLvector4f ** attribptr,
				 int *inputs, GLint * tab, GLuint nr)
{
	GLuint i, dw;

	/* type, inputs, stop bit, size */
	for (i = 0; i < nr; i += 2) {
		/* make sure input is valid, would lockup the gpu */
		assert(inputs[tab[i]] != -1);
		dw = (R600_SIGNED | DW_SIZE(i));
		if (i + 1 == nr) {
			dw |= R600_LAST_VEC << R600_DATA_TYPE_0_SHIFT;
		} else {
			assert(inputs[tab[i + 1]] != -1);
			dw |= (R600_SIGNED |
			       DW_SIZE(i + 1)) << R600_DATA_TYPE_1_SHIFT;
			if (i + 2 == nr) {
				dw |= R600_LAST_VEC << R600_DATA_TYPE_1_SHIFT;
			}
		}
		dst[i >> 1] = dw;
	}

	return (nr + 1) >> 1;
}

static GLuint r600VAPInputRoute1Swizzle(int swizzle[4])
{
	return (swizzle[0] << R600_SWIZZLE_SELECT_X_SHIFT) |
	    (swizzle[1] << R600_SWIZZLE_SELECT_Y_SHIFT) |
	    (swizzle[2] << R600_SWIZZLE_SELECT_Z_SHIFT) |
	    (swizzle[3] << R600_SWIZZLE_SELECT_W_SHIFT);
}

GLuint r600VAPInputRoute1(uint32_t * dst, int swizzle[][4], GLuint nr)
{
	GLuint i, dw;

	for (i = 0; i < nr; i += 2) {
		dw = (r600VAPInputRoute1Swizzle(swizzle[i]) |
		      ((R600_WRITE_ENA_X | R600_WRITE_ENA_Y |
			R600_WRITE_ENA_Z | R600_WRITE_ENA_W) << R600_WRITE_ENA_SHIFT)) << R600_SWIZZLE0_SHIFT;
		if (i + 1 < nr) {
			dw |= (r600VAPInputRoute1Swizzle(swizzle[i + 1]) |
			       ((R600_WRITE_ENA_X | R600_WRITE_ENA_Y |
				 R600_WRITE_ENA_Z | R600_WRITE_ENA_W) << R600_WRITE_ENA_SHIFT)) << R600_SWIZZLE1_SHIFT;
		}
		dst[i >> 1] = dw;
	}

	return (nr + 1) >> 1;
}

GLuint r600VAPInputCntl0(GLcontext * ctx, GLuint InputsRead)
{
	/* No idea what this value means. I have seen other values written to
	 * this register... */
	return 0x5555;
}

GLuint r600VAPInputCntl1(GLcontext * ctx, GLuint InputsRead)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	GLuint i, vic_1 = 0;

	if (InputsRead & (1 << VERT_ATTRIB_POS))
		vic_1 |= R600_INPUT_CNTL_POS;

	if (InputsRead & (1 << VERT_ATTRIB_NORMAL))
		vic_1 |= R600_INPUT_CNTL_NORMAL;

	if (InputsRead & (1 << VERT_ATTRIB_COLOR0))
		vic_1 |= R600_INPUT_CNTL_COLOR;

	rmesa->state.texture.tc_count = 0;
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (InputsRead & (1 << (VERT_ATTRIB_TEX0 + i))) {
			rmesa->state.texture.tc_count++;
			vic_1 |= R600_INPUT_CNTL_TC0 << i;
		}

	return vic_1;
}

GLuint r600VAPOutputCntl0(GLcontext * ctx, GLuint OutputsWritten)
{
	GLuint ret = 0;

	if (OutputsWritten & (1 << VERT_RESULT_HPOS))
		ret |= R600_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL0))
		ret |= R600_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL1))
		ret |= R600_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_BFC0)
	    || OutputsWritten & (1 << VERT_RESULT_BFC1))
		ret |=
		    R600_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT |
		    R600_VAP_OUTPUT_VTX_FMT_0__COLOR_2_PRESENT |
		    R600_VAP_OUTPUT_VTX_FMT_0__COLOR_3_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ))
		ret |= R600_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;

	return ret;
}

GLuint r600VAPOutputCntl1(GLcontext * ctx, GLuint OutputsWritten)
{
	GLuint i, ret = 0, first_free_texcoord = 0;

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (OutputsWritten & (1 << (VERT_RESULT_TEX0 + i))) {
			ret |= (4 << (3 * i));
			++first_free_texcoord;
		}
	}

	if (OutputsWritten & (1 << VERT_RESULT_FOGC)) {
		if (first_free_texcoord > 8) {
			fprintf(stderr, "\tout of free texcoords to write fog coord\n");
			_mesa_exit(-1);
		}
		ret |= 4 << (3 * first_free_texcoord);
	}

	return ret;
}

/* Emit vertex data to GART memory
 * Route inputs to the vertex processor
 * This function should never return R600_FALLBACK_TCL when using software tcl.
 */
int r600EmitArrays(GLcontext * ctx)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;
	GLuint nr;
	GLuint count = vb->Count;
	GLuint i;
	GLuint InputsRead = 0, OutputsWritten = 0;
	int *inputs = NULL;
	int vir_inputs[VERT_ATTRIB_MAX];
	GLint tab[VERT_ATTRIB_MAX];
	int swizzle[VERT_ATTRIB_MAX][4];
	struct r600_vertex_program *prog =
	    (struct r600_vertex_program *)CURRENT_VERTEX_SHADER(ctx);

	if (hw_tcl_on) {
		inputs = prog->inputs;
		InputsRead = prog->key.InputsRead;
		OutputsWritten = prog->key.OutputsWritten;
	} else {
		inputs = rmesa->state.sw_tcl_inputs;

		DECLARE_RENDERINPUTS(render_inputs_bitset);
		RENDERINPUTS_COPY(render_inputs_bitset, tnl->render_inputs_bitset);

		vb->AttribPtr[VERT_ATTRIB_POS] = vb->ClipPtr;

		assert(RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_POS));
		assert(RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_NORMAL) == 0);

		if (RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_POS)) {
			InputsRead |= 1 << VERT_ATTRIB_POS;
			OutputsWritten |= 1 << VERT_RESULT_HPOS;
		}

		if (RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_COLOR0)) {
			InputsRead |= 1 << VERT_ATTRIB_COLOR0;
			OutputsWritten |= 1 << VERT_RESULT_COL0;
		}

		if (RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_COLOR1)) {
			InputsRead |= 1 << VERT_ATTRIB_COLOR1;
			OutputsWritten |= 1 << VERT_RESULT_COL1;
		}

		for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
			if (RENDERINPUTS_TEST(render_inputs_bitset, _TNL_ATTRIB_TEX(i))) {
				InputsRead |= 1 << (VERT_ATTRIB_TEX0 + i);
				OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
			}
		}

		for (i = 0, nr = 0; i < VERT_ATTRIB_MAX; i++) {
			if (InputsRead & (1 << i)) {
				inputs[i] = nr++;
			} else {
				inputs[i] = -1;
			}
		}

		/* Fixed, apply to vir0 only */
		memcpy(vir_inputs, inputs, VERT_ATTRIB_MAX * sizeof(int));
		inputs = vir_inputs;
		if (InputsRead & VERT_ATTRIB_POS)
			inputs[VERT_ATTRIB_POS] = 0;
		if (InputsRead & (1 << VERT_ATTRIB_COLOR0))
			inputs[VERT_ATTRIB_COLOR0] = 2;
		if (InputsRead & (1 << VERT_ATTRIB_COLOR1))
			inputs[VERT_ATTRIB_COLOR1] = 3;
		for (i = VERT_ATTRIB_TEX0; i <= VERT_ATTRIB_TEX7; i++)
			if (InputsRead & (1 << i))
				inputs[i] = 6 + (i - VERT_ATTRIB_TEX0);

		RENDERINPUTS_COPY(rmesa->state.render_inputs_bitset, render_inputs_bitset);
	}

	assert(InputsRead);
	assert(OutputsWritten);

	for (i = 0, nr = 0; i < VERT_ATTRIB_MAX; i++) {
		if (InputsRead & (1 << i)) {
			tab[nr++] = i;
		}
	}

	if (nr > R600_MAX_AOS_ARRAYS) {
		return R600_FALLBACK_TCL;
	}

	for (i = 0; i < nr; i++) {
		int ci;

		swizzle[i][0] = SWIZZLE_ZERO;
		swizzle[i][1] = SWIZZLE_ZERO;
		swizzle[i][2] = SWIZZLE_ZERO;
		swizzle[i][3] = SWIZZLE_ONE;

		for (ci = 0; ci < vb->AttribPtr[tab[i]]->size; ci++) {
			swizzle[i][ci] = ci;
		}
		rcommon_emit_vector(ctx, &rmesa->radeon.tcl.aos[i],
				    vb->AttribPtr[tab[i]]->data,
				    vb->AttribPtr[tab[i]]->size,
				    vb->AttribPtr[tab[i]]->stride, count);
	}

	/* Setup INPUT_ROUTE. */
	if (rmesa->radeon.radeonScreen->kernel_mm) {
		R600_STATECHANGE(rmesa, vir[0]);
		rmesa->hw.vir[0].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[1].cmd[0] &= 0xC000FFFF;
		rmesa->hw.vir[0].cmd[0] |=
			(r600VAPInputRoute0(&rmesa->hw.vir[0].cmd[R600_VIR_CNTL_0],
					    vb->AttribPtr, inputs, tab, nr) & 0x3FFF) << 16;
		R600_STATECHANGE(rmesa, vir[1]);
		rmesa->hw.vir[1].cmd[0] |=
			(r600VAPInputRoute1(&rmesa->hw.vir[1].cmd[R600_VIR_CNTL_0], swizzle,
					    nr) & 0x3FFF) << 16;
	} else {
		R600_STATECHANGE(rmesa, vir[0]);
		((drm_r300_cmd_header_t *) rmesa->hw.vir[0].cmd)->packet0.count =
			r600VAPInputRoute0(&rmesa->hw.vir[0].cmd[R600_VIR_CNTL_0],
					   vb->AttribPtr, inputs, tab, nr);
		R600_STATECHANGE(rmesa, vir[1]);
		((drm_r300_cmd_header_t *) rmesa->hw.vir[1].cmd)->packet0.count =
			r600VAPInputRoute1(&rmesa->hw.vir[1].cmd[R600_VIR_CNTL_0], swizzle,
					   nr);
	}
	
	/* Setup INPUT_CNTL. */
	R600_STATECHANGE(rmesa, vic);
	rmesa->hw.vic.cmd[R600_VIC_CNTL_0] = r600VAPInputCntl0(ctx, InputsRead);
	rmesa->hw.vic.cmd[R600_VIC_CNTL_1] = r600VAPInputCntl1(ctx, InputsRead);

	/* Setup OUTPUT_VTX_FMT. */
	R600_STATECHANGE(rmesa, vof);
	rmesa->hw.vof.cmd[R600_VOF_CNTL_0] =
	    r600VAPOutputCntl0(ctx, OutputsWritten);
	rmesa->hw.vof.cmd[R600_VOF_CNTL_1] =
	    r600VAPOutputCntl1(ctx, OutputsWritten);

	rmesa->radeon.tcl.aos_count = nr;

	return R600_FALLBACK_NONE;
}

void r600EmitCacheFlush(r600ContextPtr rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(4);
	OUT_BATCH_REGVAL(R600_RB3D_DSTCACHE_CTLSTAT,
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
		R600_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
	OUT_BATCH_REGVAL(R600_ZB_ZCACHE_CTLSTAT,
		R600_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
		R600_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
	END_BATCH();
	COMMIT_BATCH();
}
