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


static void fetch_input_vec4 (const char *name, GLfloat *vec, GLuint index,
	struct gl2_fragment_shader_intf **fs)
{
	_slang_fetch_vec4_f (fs, name, vec, index, 1);
}

static void fetch_output_vec4 (const char *name, GLfloat *vec, GLuint index,
	struct gl2_fragment_shader_intf **fs)
{
	_slang_fetch_vec4_f (fs, name, vec, index, 0);
}

void _swrast_exec_arbshader (GLcontext *ctx, struct sw_span *span)
{
	struct gl2_program_intf **prog;
	struct gl2_fragment_shader_intf **fs = NULL;
	GLuint count, i;

	prog = ctx->ShaderObjects.CurrentProgram;
	count = (**prog)._container.GetAttachedCount ((struct gl2_container_intf **) prog);
	for (i = 0; i < count; i++)
	{
		struct gl2_generic_intf **obj;
		struct gl2_unknown_intf **unk;

		obj = (**prog)._container.GetAttached ((struct gl2_container_intf **) prog, i);
		unk = (**obj)._unknown.QueryInterface ((struct gl2_unknown_intf **) obj, UIID_FRAGMENT_SHADER);
		(**obj)._unknown.Release ((struct gl2_unknown_intf **) obj);
		if (unk != NULL)
		{
			fs = (struct gl2_fragment_shader_intf **) unk;
			break;
		}
	}
	if (fs == NULL)
		return;

	for (i = span->start; i < span->end; i++)
	{
		GLfloat vec[4];
		GLuint j;
		GLboolean kill;

		vec[0] = (GLfloat) span->x + i;
		vec[1] = (GLfloat) span->y;
		vec[2] = (GLfloat) span->array->z[i] / ctx->DrawBuffer->_DepthMaxF;
		vec[3] = span->w + span->dwdx * i;
		fetch_input_vec4 ("gl_FragCoord", vec, 0, fs);
		vec[0] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
		vec[1] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
		vec[2] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
		vec[3] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);
		fetch_input_vec4 ("gl_Color", vec, 0, fs);
		for (j = 0; j < ctx->Const.MaxTextureCoordUnits; j++)
		{
			vec[0] = span->array->texcoords[j][i][0];
			vec[1] = span->array->texcoords[j][i][1];
			vec[2] = span->array->texcoords[j][i][2];
			vec[3] = span->array->texcoords[j][i][3];
			fetch_input_vec4 ("gl_TexCoord", vec, j, fs);
		}

		_slang_exec_fragment_shader (fs);

		_slang_fetch_discard (fs, &kill);
		if (kill)
		{
			span->array->mask[i] = GL_FALSE;
			span->writeAll = GL_FALSE;
		}
		fetch_output_vec4 ("gl_FragColor", vec, 0, fs);
		UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], vec[0]);
		UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], vec[1]);
		UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], vec[2]);
		UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], vec[3]);
	}
}

