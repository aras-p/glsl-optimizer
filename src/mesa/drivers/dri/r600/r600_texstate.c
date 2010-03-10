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
 *
 * \todo Enable R300 texture tiling code?
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/enums.h"
#include "main/simple_list.h"

#include "r600_context.h"
#include "radeon_mipmap_tree.h"
#include "r600_tex.h"
#include "r700_fragprog.h"
#include "r700_vertprog.h"

void r600UpdateTextureState(GLcontext * ctx);

void r600UpdateTextureState(GLcontext * ctx)
{
	context_t *context = R700_CONTEXT(ctx);
	R700_CHIP_CONTEXT *r700 = (R700_CHIP_CONTEXT*)(&context->hw);
	struct gl_texture_unit *texUnit;
	struct radeon_tex_obj *t;
	GLuint    unit;

	R600_STATECHANGE(context, tx);
	R600_STATECHANGE(context, tx_smplr);
	R600_STATECHANGE(context, tx_brdr_clr);

	for (unit = 0; unit < R700_MAX_TEXTURE_UNITS; unit++) {
		texUnit = &ctx->Texture.Unit[unit];
		t = radeon_tex_obj(ctx->Texture.Unit[unit]._Current);
		r700->textures[unit] = NULL;
		if (texUnit->_ReallyEnabled) {
			if (!t)
				continue;
			r700->textures[unit] = t;
		}
	}
}

static GLboolean r600GetTexFormat(struct gl_texture_object *tObj, gl_format mesa_format)
{
	radeonTexObj *t = radeon_tex_obj(tObj);

	CLEARfield(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	CLEARfield(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	CLEARfield(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	CLEARfield(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	CLEARbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);

	SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
		 FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
		 FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
		 FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
		 FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);

	CLEARbit(t->SQ_TEX_RESOURCE0, TILE_TYPE_bit);
	SETfield(t->SQ_TEX_RESOURCE0, ARRAY_LINEAR_GENERAL,
		 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);

	switch (mesa_format) /* This is mesa format. */
	{
	case MESA_FORMAT_RGBA8888:
	case MESA_FORMAT_SIGNED_RGBA8888:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		if (mesa_format == MESA_FORMAT_SIGNED_RGBA8888) {
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
		}
		break;
	case MESA_FORMAT_RGBA8888_REV:
	case MESA_FORMAT_SIGNED_RGBA8888_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		if (mesa_format == MESA_FORMAT_SIGNED_RGBA8888_REV) {
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_SIGNED,
				 FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
		}
		break;
	case MESA_FORMAT_ARGB8888:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_XRGB8888:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_XRGB8888_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ARGB8888_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB888:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB565:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_5_6_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB565_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_5_6_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ARGB4444:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_4_4_4_4,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ARGB4444_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_4_4_4_4,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ARGB1555:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_1_5_5_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ARGB1555_REV:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_1_5_5_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_AL88:
	case MESA_FORMAT_AL88_REV: /* TODO : Check this. */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB332:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_3_3_2,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_A8: /* ZERO, ZERO, ZERO, X */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_L8: /* X, X, X, ONE */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_I8: /* X, X, X, X */
	case MESA_FORMAT_CI8:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
		/* YUV422 TODO conversion */  /* X, Y, Z, ONE, G8R8_G8B8 */
		/*
		  case MESA_FORMAT_YCBCR:
		  t->SQ_TEX_RESOURCE1.bitfields.DATA_FORMAT = ;
		  break;
		*/
		/* VUY422 TODO conversion */  /* X, Y, Z, ONE, G8R8_G8B8 */
		/*
		  case MESA_FORMAT_YCBCR_REV:
		  t->SQ_TEX_RESOURCE1.bitfields.DATA_FORMAT = ;
		  break;
		*/
	case MESA_FORMAT_RGB_DXT1: /* not supported yet */
	case MESA_FORMAT_RGBA_DXT1: /* not supported yet */
	case MESA_FORMAT_RGBA_DXT3: /* not supported yet */
	case MESA_FORMAT_RGBA_DXT5: /* not supported yet */
	        return GL_FALSE;

	case MESA_FORMAT_RGBA_FLOAT32:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_32_32_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGBA_FLOAT16:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_16_16_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB_FLOAT32: /* X, Y, Z, ONE */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_32_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_RGB_FLOAT16:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_16_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ALPHA_FLOAT32: /* ZERO, ZERO, ZERO, X */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_ALPHA_FLOAT16: /* ZERO, ZERO, ZERO, X */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_LUMINANCE_FLOAT32: /* X, X, X, ONE */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_LUMINANCE_FLOAT16: /* X, X, X, ONE */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_32_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_16_FLOAT,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		break;
	case MESA_FORMAT_Z16:
	case MESA_FORMAT_X8_Z24:
	case MESA_FORMAT_S8_Z24:
	case MESA_FORMAT_Z24_S8:
	case MESA_FORMAT_Z32:
	case MESA_FORMAT_S8:
		SETbit(t->SQ_TEX_RESOURCE0, TILE_TYPE_bit);
		SETfield(t->SQ_TEX_RESOURCE0, ARRAY_1D_TILED_THIN1,
			 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
			 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
		switch (mesa_format) {
		case MESA_FORMAT_Z16:
			SETfield(t->SQ_TEX_RESOURCE1, FMT_16,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
			break;
		case MESA_FORMAT_X8_Z24:
		case MESA_FORMAT_S8_Z24:
			SETfield(t->SQ_TEX_RESOURCE1, FMT_8_24,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
			break;
		case MESA_FORMAT_Z24_S8:
			SETfield(t->SQ_TEX_RESOURCE1, FMT_24_8,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
			break;
		case MESA_FORMAT_Z32:
			SETfield(t->SQ_TEX_RESOURCE1, FMT_32,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
			break;
		case MESA_FORMAT_S8:
			SETfield(t->SQ_TEX_RESOURCE1, FMT_8,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
			break;
		default:
			break;
		};
		switch (tObj->DepthMode) {
		case GL_LUMINANCE:  /* X, X, X, ONE */
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
			break;
		case GL_INTENSITY:  /* X, X, X, X */
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
			break;
		case GL_ALPHA:     /* ZERO, ZERO, ZERO, X */
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_0,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
			break;
		default:
			return GL_FALSE;
		}
		break;
	/* EXT_texture_sRGB */
	case MESA_FORMAT_SRGBA8:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		SETbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
		break;
	case MESA_FORMAT_SLA8:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		SETbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
		break;
	case MESA_FORMAT_SL8: /* X, X, X, ONE */
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		SETbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
		break;
	default:
		/* Not supported format */
		return GL_FALSE;
	};

	return GL_TRUE;
}

static GLuint r600_translate_shadow_func(GLenum func)
{
   switch (func) {
   case GL_NEVER:
      return SQ_TEX_DEPTH_COMPARE_NEVER;
   case GL_LESS:
      return SQ_TEX_DEPTH_COMPARE_LESS;
   case GL_LEQUAL:
      return SQ_TEX_DEPTH_COMPARE_LESSEQUAL;
   case GL_GREATER:
      return SQ_TEX_DEPTH_COMPARE_GREATER;
   case GL_GEQUAL:
      return SQ_TEX_DEPTH_COMPARE_GREATEREQUAL;
   case GL_NOTEQUAL:
      return SQ_TEX_DEPTH_COMPARE_NOTEQUAL;
   case GL_EQUAL:
      return SQ_TEX_DEPTH_COMPARE_EQUAL;
   case GL_ALWAYS:
      return SQ_TEX_DEPTH_COMPARE_ALWAYS;
   default:
      WARN_ONCE("Unknown shadow compare function! %d", func);
      return 0;
   }
}

static INLINE uint32_t
S_FIXED(float value, uint32_t frac_bits)
{
   return value * (1 << frac_bits);
}

void r600SetDepthTexMode(struct gl_texture_object *tObj)
{
	radeonTexObjPtr t;

	if (!tObj)
		return;

	t = radeon_tex_obj(tObj);

	if(!r600GetTexFormat(tObj, tObj->Image[0][tObj->BaseLevel]->TexFormat))
	  t->validated = GL_FALSE;
}

/**
 * Compute the cached hardware register values for the given texture object.
 *
 * \param rmesa Context pointer
 * \param t the r300 texture object
 */
static GLboolean setup_hardware_state(GLcontext * ctx, struct gl_texture_object *texObj, int unit)
{
	context_t *rmesa = R700_CONTEXT(ctx);
	radeonTexObj *t = radeon_tex_obj(texObj);
	const struct gl_texture_image *firstImage;
	GLuint uTexelPitch, row_align;

	if (rmesa->radeon.radeonScreen->driScreen->dri2.enabled &&
	    t->image_override &&
	    t->bo)
		return GL_TRUE;

	firstImage = t->base.Image[0][t->minLod];

	if (!t->image_override) {
		if (!r600GetTexFormat(texObj, firstImage->TexFormat)) {
			radeon_warning("unsupported texture format in %s\n",
				       __FUNCTION__);
			return GL_FALSE;
		}
	}

	switch (texObj->Target) {
        case GL_TEXTURE_1D:
		SETfield(t->SQ_TEX_RESOURCE0, SQ_TEX_DIM_1D, DIM_shift, DIM_mask);
		SETfield(t->SQ_TEX_RESOURCE1, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);
		break;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_RECTANGLE_NV:
		SETfield(t->SQ_TEX_RESOURCE0, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);
		SETfield(t->SQ_TEX_RESOURCE1, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);
		break;
        case GL_TEXTURE_3D:
		SETfield(t->SQ_TEX_RESOURCE0, SQ_TEX_DIM_3D, DIM_shift, DIM_mask);
		SETfield(t->SQ_TEX_RESOURCE1, firstImage->Depth - 1, // ???
			 TEX_DEPTH_shift, TEX_DEPTH_mask);
		break;
        case GL_TEXTURE_CUBE_MAP:
		SETfield(t->SQ_TEX_RESOURCE0, SQ_TEX_DIM_CUBEMAP, DIM_shift, DIM_mask);
		SETfield(t->SQ_TEX_RESOURCE1, 0, TEX_DEPTH_shift, TEX_DEPTH_mask);
		break;
        default:
		radeon_error("unexpected texture target type in %s\n", __FUNCTION__);
		return GL_FALSE;
	}

	row_align = rmesa->radeon.texture_row_align - 1;
	uTexelPitch = (_mesa_format_row_stride(firstImage->TexFormat, firstImage->Width) + row_align) & ~row_align;
	uTexelPitch = uTexelPitch / _mesa_get_format_bytes(firstImage->TexFormat);
	uTexelPitch = (uTexelPitch + R700_TEXEL_PITCH_ALIGNMENT_MASK)
		& ~R700_TEXEL_PITCH_ALIGNMENT_MASK;

	/* min pitch is 8 */
	if (uTexelPitch < 8)
		uTexelPitch = 8;

	SETfield(t->SQ_TEX_RESOURCE0, (uTexelPitch/8)-1, PITCH_shift, PITCH_mask);
	SETfield(t->SQ_TEX_RESOURCE0, firstImage->Width - 1,
		 TEX_WIDTH_shift, TEX_WIDTH_mask);
	SETfield(t->SQ_TEX_RESOURCE1, firstImage->Height - 1,
		 TEX_HEIGHT_shift, TEX_HEIGHT_mask);

	t->SQ_TEX_RESOURCE2 = get_base_teximage_offset(t) / 256;

	t->SQ_TEX_RESOURCE3 = radeon_miptree_image_offset(t->mt, 0, t->minLod + 1) / 256;

	SETfield(t->SQ_TEX_RESOURCE4, 0, BASE_LEVEL_shift, BASE_LEVEL_mask);
	SETfield(t->SQ_TEX_RESOURCE5, t->maxLod - t->minLod, LAST_LEVEL_shift, LAST_LEVEL_mask);

	SETfield(t->SQ_TEX_SAMPLER1,
		S_FIXED(CLAMP(t->base.MinLod - t->minLod, 0, 15), 6),
		MIN_LOD_shift, MIN_LOD_mask);
	SETfield(t->SQ_TEX_SAMPLER1,
		S_FIXED(CLAMP(t->base.MaxLod - t->minLod, 0, 15), 6),
		MAX_LOD_shift, MAX_LOD_mask);
	SETfield(t->SQ_TEX_SAMPLER1,
		S_FIXED(CLAMP(ctx->Texture.Unit[unit].LodBias + t->base.LodBias, -16, 16), 6),
		SQ_TEX_SAMPLER_WORD1_0__LOD_BIAS_shift, SQ_TEX_SAMPLER_WORD1_0__LOD_BIAS_mask);

	if(texObj->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB)
	{
		SETfield(t->SQ_TEX_SAMPLER0, r600_translate_shadow_func(texObj->CompareFunc), DEPTH_COMPARE_FUNCTION_shift, DEPTH_COMPARE_FUNCTION_mask);
	}
	else
	{
		CLEARfield(t->SQ_TEX_SAMPLER0, DEPTH_COMPARE_FUNCTION_mask);
	}

	return GL_TRUE;
}

/**
 * Ensure the given texture is ready for rendering.
 *
 * Mostly this means populating the texture object's mipmap tree.
 */
static GLboolean r600_validate_texture(GLcontext * ctx, struct gl_texture_object *texObj, int unit)
{
	radeonTexObj *t = radeon_tex_obj(texObj);

	if (!radeon_validate_texture_miptree(ctx, texObj))
		return GL_FALSE;

	/* Configure the hardware registers (more precisely, the cached version
	 * of the hardware registers). */
	if (!setup_hardware_state(ctx, texObj, unit))
	        return GL_FALSE;

	t->validated = GL_TRUE;
	return GL_TRUE;
}

/**
 * Ensure all enabled and complete textures are uploaded along with any buffers being used.
 */
GLboolean r600ValidateBuffers(GLcontext * ctx)
{
	context_t *rmesa = R700_CONTEXT(ctx);
	struct radeon_renderbuffer *rrb;
	struct radeon_bo *pbo;
	int i;
	int ret;

	radeon_cs_space_reset_bos(rmesa->radeon.cmdbuf.cs);

	rrb = radeon_get_colorbuffer(&rmesa->radeon);
	/* color buffer */
	if (rrb && rrb->bo) {
		radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
						  rrb->bo, 0,
						  RADEON_GEM_DOMAIN_VRAM);
	}

	/* depth buffer */
	rrb = radeon_get_depthbuffer(&rmesa->radeon);
	if (rrb && rrb->bo) {
		radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
						  rrb->bo, 0,
						  RADEON_GEM_DOMAIN_VRAM);
	}
	
	for (i = 0; i < ctx->Const.MaxTextureImageUnits; ++i) {
		radeonTexObj *t;

		if (!ctx->Texture.Unit[i]._ReallyEnabled)
			continue;

		if (!r600_validate_texture(ctx, ctx->Texture.Unit[i]._Current, i)) {
			radeon_warning("failed to validate texture for unit %d.\n", i);
		}
		t = radeon_tex_obj(ctx->Texture.Unit[i]._Current);
		if (t->image_override && t->bo)
			radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
							  t->bo,
							  RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
		else if (t->mt->bo)
			radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
							  t->mt->bo,
							  RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0);
	}

	pbo = (struct radeon_bo *)r700GetActiveFpShaderBo(ctx);
	if (pbo) {
		radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, pbo,
						  RADEON_GEM_DOMAIN_GTT, 0);
	}

	pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(ctx);
	if (pbo) {
		radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs, pbo,
						  RADEON_GEM_DOMAIN_GTT, 0);
	}

	ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs, first_elem(&rmesa->radeon.dma.reserved)->bo, RADEON_GEM_DOMAIN_GTT, 0);
	if (ret)
		return GL_FALSE;
	return GL_TRUE;
}

void r600SetTexOffset(__DRIcontext * pDRICtx, GLint texname,
		      unsigned long long offset, GLint depth, GLuint pitch)
{
	context_t *rmesa = pDRICtx->driverPrivate;
	struct gl_texture_object *tObj =
	    _mesa_lookup_texture(rmesa->radeon.glCtx, texname);
	radeonTexObjPtr t = radeon_tex_obj(tObj);
	const struct gl_texture_image *firstImage;
	uint32_t pitch_val, size, row_align;

	if (!tObj)
		return;

	t->image_override = GL_TRUE;

	if (!offset)
		return;

	firstImage = t->base.Image[0][t->minLod];
	row_align = rmesa->radeon.texture_row_align - 1;
	size = ((_mesa_format_row_stride(firstImage->TexFormat, firstImage->Width) + row_align) & ~row_align) * firstImage->Height;
	if (t->bo) {
		radeon_bo_unref(t->bo);
		t->bo = NULL;
	}
	t->bo = radeon_legacy_bo_alloc_fake(rmesa->radeon.radeonScreen->bom, size, offset);
	t->override_offset = offset;
	pitch_val = pitch;
	switch (depth) {
	case 32:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		pitch_val /= 4;
		break;
	case 24:
	default:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		pitch_val /= 4;
		break;
	case 16:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_5_6_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		pitch_val /= 2;
		break;
	}

	pitch_val = (pitch_val + R700_TEXEL_PITCH_ALIGNMENT_MASK)
		& ~R700_TEXEL_PITCH_ALIGNMENT_MASK;

	/* min pitch is 8 */
	if (pitch_val < 8)
		pitch_val = 8;

	SETfield(t->SQ_TEX_RESOURCE0, (pitch_val/8)-1, PITCH_shift, PITCH_mask);
}

void r600SetTexBuffer2(__DRIcontext *pDRICtx, GLint target, GLint glx_texture_format, __DRIdrawable *dPriv)
{
	struct gl_texture_unit *texUnit;
	struct gl_texture_object *texObj;
	struct gl_texture_image *texImage;
	struct radeon_renderbuffer *rb;
	radeon_texture_image *rImage;
	radeonContextPtr radeon;
	context_t *rmesa;
	struct radeon_framebuffer *rfb;
	radeonTexObjPtr t;
	uint32_t pitch_val;
	uint32_t internalFormat, type, format;

	type = GL_BGRA;
	format = GL_UNSIGNED_BYTE;
	internalFormat = (glx_texture_format == __DRI_TEXTURE_FORMAT_RGB ? 3 : 4);

	radeon = pDRICtx->driverPrivate;
	rmesa = pDRICtx->driverPrivate;

	rfb = dPriv->driverPrivate;
        texUnit = &radeon->glCtx->Texture.Unit[radeon->glCtx->Texture.CurrentUnit];
	texObj = _mesa_select_tex_object(radeon->glCtx, texUnit, target);
        texImage = _mesa_get_tex_image(radeon->glCtx, texObj, target, 0);

	rImage = get_radeon_texture_image(texImage);
	t = radeon_tex_obj(texObj);
        if (t == NULL) {
    	    return;
    	}

	radeon_update_renderbuffers(pDRICtx, dPriv, GL_TRUE);
	rb = rfb->color_rb[0];
	if (rb->bo == NULL) {
		/* Failed to BO for the buffer */
		return;
	}

	_mesa_lock_texture(radeon->glCtx, texObj);
	if (t->bo) {
		radeon_bo_unref(t->bo);
		t->bo = NULL;
	}
	if (rImage->bo) {
		radeon_bo_unref(rImage->bo);
		rImage->bo = NULL;
	}

	radeon_miptree_unreference(&t->mt);
	radeon_miptree_unreference(&rImage->mt);

	_mesa_init_teximage_fields(radeon->glCtx, target, texImage,
				   rb->base.Width, rb->base.Height, 1, 0, rb->cpp);
	texImage->RowStride = rb->pitch / rb->cpp;

	rImage->bo = rb->bo;
	radeon_bo_ref(rImage->bo);
	t->bo = rb->bo;
	radeon_bo_ref(t->bo);
	t->image_override = GL_TRUE;
	t->override_offset = 0;
	pitch_val = rb->pitch;
	switch (rb->cpp) {
	case 4:
		if (glx_texture_format == __DRI_TEXTURE_FORMAT_RGB) {
			SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		} else {
			SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
				 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
			SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
				 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		}
		pitch_val /= 4;
		break;
	case 3:
	default:
		// FMT_8_8_8 ???
		SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		pitch_val /= 4;
		break;
	case 2:
		SETfield(t->SQ_TEX_RESOURCE1, FMT_5_6_5,
			 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
		SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_1,
			 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
		pitch_val /= 2;
		break;
	}

	pitch_val = (pitch_val + R700_TEXEL_PITCH_ALIGNMENT_MASK)
		& ~R700_TEXEL_PITCH_ALIGNMENT_MASK;

	/* min pitch is 8 */
	if (pitch_val < 8)
		pitch_val = 8;

	SETfield(t->SQ_TEX_RESOURCE0, (pitch_val/8)-1, PITCH_shift, PITCH_mask);
	SETfield(t->SQ_TEX_RESOURCE0, rb->base.Width - 1,
		 TEX_WIDTH_shift, TEX_WIDTH_mask);
	SETfield(t->SQ_TEX_RESOURCE1, rb->base.Height - 1,
		 TEX_HEIGHT_shift, TEX_HEIGHT_mask);

	t->validated = GL_TRUE;
	_mesa_unlock_texture(radeon->glCtx, texObj);
	return;
}

void r600SetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
        r600SetTexBuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}
