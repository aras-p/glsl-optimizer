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
 * \author Maciej Cencora <m.cencora@gmail.com>
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

#include "r300_context.h"
#include "r300_state.h"
#include "r300_emit.h"
#include "r300_ioctl.h"
#include "r300_render.h"
#include "r300_swtcl.h"

GLuint r300VAPInputCntl0(GLcontext * ctx, GLuint InputsRead)
{
	/* No idea what this value means. I have seen other values written to
	 * this register... */
	return 0x5555;
}

GLuint r300VAPInputCntl1(GLcontext * ctx, GLuint InputsRead)
{
	GLuint i, vic_1 = 0;

	if (InputsRead & (1 << VERT_ATTRIB_POS))
		vic_1 |= R300_INPUT_CNTL_POS;

	if (InputsRead & (1 << VERT_ATTRIB_NORMAL))
		vic_1 |= R300_INPUT_CNTL_NORMAL;

	if (InputsRead & (1 << VERT_ATTRIB_COLOR0))
		vic_1 |= R300_INPUT_CNTL_COLOR;

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		if (InputsRead & (1 << (VERT_ATTRIB_TEX0 + i))) {
			vic_1 |= R300_INPUT_CNTL_TC0 << i;
		}

	return vic_1;
}

GLuint r300VAPOutputCntl0(GLcontext * ctx, GLuint OutputsWritten)
{
	GLuint ret = 0;

	if (OutputsWritten & (1 << VERT_RESULT_HPOS))
		ret |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL0))
		ret |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_COL1))
		ret |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_BFC0)
	    || OutputsWritten & (1 << VERT_RESULT_BFC1))
		ret |=
		    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_1_PRESENT |
		    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_2_PRESENT |
		    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_3_PRESENT;

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ))
		ret |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;

	return ret;
}

GLuint r300VAPOutputCntl1(GLcontext * ctx, GLuint OutputsWritten)
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
		ret |= 1 << (3 * first_free_texcoord);
	}

	return ret;
}

GLboolean r300EmitArrays(GLcontext * ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_buffer *vbuf = &r300->vbuf;
	GLuint InputsRead, OutputsWritten;

	r300ChooseSwtclVertexFormat(ctx, &InputsRead, &OutputsWritten);

	r300SwitchFallback(ctx, R300_FALLBACK_AOS_LIMIT, vbuf->num_attribs > R300_MAX_AOS_ARRAYS);
	if (r300->fallback & R300_RASTER_FALLBACK_MASK)
		return GL_FALSE;

	{
		struct vertex_buffer *mesa_vb = &TNL_CONTEXT(ctx)->vb;
		GLuint attr, i;

		for (i = 0; i < vbuf->num_attribs; i++) {
			attr = vbuf->attribs[i].element;
			rcommon_emit_vector(ctx, &r300->radeon.tcl.aos[i], mesa_vb->AttribPtr[attr]->data,
					mesa_vb->AttribPtr[attr]->size, mesa_vb->AttribPtr[attr]->stride, mesa_vb->Count);
		}

		r300->radeon.tcl.aos_count = vbuf->num_attribs;

		/* Fill index buffer info */
		r300->ind_buf.ptr = mesa_vb->Elts;
		r300->ind_buf.is_32bit = GL_TRUE;
		r300->ind_buf.free_needed = GL_FALSE;
	}

	r300SetupVAP(ctx, InputsRead, OutputsWritten);

	return GL_TRUE;
}

void r300EmitCacheFlush(r300ContextPtr rmesa)
{
	BATCH_LOCALS(&rmesa->radeon);

	BEGIN_BATCH_NO_AUTOSTATE(4);
	OUT_BATCH_REGVAL(R300_RB3D_DSTCACHE_CTLSTAT,
		R300_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
		R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
	OUT_BATCH_REGVAL(R300_ZB_ZCACHE_CTLSTAT,
		R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
		R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
	END_BATCH();
	COMMIT_BATCH();
}
