/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Michal Krol
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "s_arbshader.h"
#include "s_context.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "slang_utility.h"
#include "slang_link.h"

void _swrast_exec_arbshader (GLcontext *ctx, struct sw_span *span)
{
	struct gl2_program_intf **pro;
	GLuint i;

	pro = ctx->ShaderObjects.CurrentProgram;
	if (pro == NULL)
		return;

	for (i = span->start; i < span->end; i++)
	{
		GLfloat vec[4];
		GLuint j;
		GLboolean discard;

		vec[0] = (GLfloat) span->x + i;
		vec[1] = (GLfloat) span->y;
		vec[2] = (GLfloat) span->array->z[i] / ctx->DrawBuffer->_DepthMaxF;
		vec[3] = span->w + span->dwdx * i;
		(**pro).UpdateFixedVarying (pro, SLANG_FRAGMENT_FIXED_FRAGCOORD, vec, 0,
			4 * sizeof (GLfloat), GL_TRUE);
		vec[0] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
		vec[1] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
		vec[2] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
		vec[3] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);
		(**pro).UpdateFixedVarying (pro, SLANG_FRAGMENT_FIXED_COLOR, vec, 0, 4 * sizeof (GLfloat),
			GL_TRUE);
		for (j = 0; j < ctx->Const.MaxTextureCoordUnits; j++)
		{
			vec[0] = span->array->texcoords[j][i][0];
			vec[1] = span->array->texcoords[j][i][1];
			vec[2] = span->array->texcoords[j][i][2];
			vec[3] = span->array->texcoords[j][i][3];
			(**pro).UpdateFixedVarying (pro, SLANG_FRAGMENT_FIXED_TEXCOORD, vec, j,
				4 * sizeof (GLfloat), GL_TRUE);
		}

		_slang_exec_fragment_shader (pro);

		_slang_fetch_discard (pro, &discard);
		if (discard)
		{
			span->array->mask[i] = GL_FALSE;
			span->writeAll = GL_FALSE;
		}
		else
		{
			(**pro).UpdateFixedVarying (pro, SLANG_FRAGMENT_FIXED_FRAGCOLOR, vec, 0,
				4 * sizeof (GLfloat), GL_FALSE);
			UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], vec[0]);
			UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], vec[1]);
			UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], vec[2]);
			UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], vec[3]);
		}
	}
}

