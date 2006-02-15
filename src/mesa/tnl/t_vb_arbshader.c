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
#include "imports.h"
#include "macros.h"
#include "shaderobjects.h"
#include "t_pipeline.h"
#include "slang_utility.h"

typedef struct
{
	GLvector4f outputs[VERT_RESULT_MAX];
	GLvector4f ndc_coords;
	GLubyte *clipmask;
	GLubyte ormask;
	GLubyte andmask;
} arbvs_stage_data;

#define ARBVS_STAGE_DATA(stage) ((arbvs_stage_data *) stage->privatePtr)

static GLboolean construct_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;
	arbvs_stage_data *store;
	GLuint size = vb->Size;
	GLuint i;

	stage->privatePtr = _mesa_malloc (sizeof (arbvs_stage_data));
	store = ARBVS_STAGE_DATA(stage);
	if (store == NULL)
		return GL_FALSE;

	for (i = 0; i < VERT_RESULT_MAX; i++)
	{
		_mesa_vector4f_alloc (&store->outputs[i], 0, size, 32);
		store->outputs[i].size = 4;
	}
	_mesa_vector4f_alloc (&store->ndc_coords, 0, size, 32);
	store->clipmask = (GLubyte *) ALIGN_MALLOC (size, 32);

	return GL_TRUE;
}

static void destruct_arb_vertex_shader (struct tnl_pipeline_stage *stage)
{
	arbvs_stage_data *store = ARBVS_STAGE_DATA(stage);

	if (store != NULL)
	{
		GLuint i;

		for (i = 0; i < VERT_RESULT_MAX; i++)
			_mesa_vector4f_free (&store->outputs[i]);
		_mesa_vector4f_free (&store->ndc_coords);
		ALIGN_FREE (store->clipmask);

		_mesa_free (store);
		stage->privatePtr = NULL;
	}
}

static void validate_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
}

/* XXX */
extern void exec_vertex_shader (struct gl2_vertex_shader_intf **vs);
extern int _slang_fetch_float (struct gl2_vertex_shader_intf **, const char *, GLfloat *, int);
extern int _slang_fetch_vec3 (struct gl2_vertex_shader_intf **, const char *, GLfloat *, int);
extern int _slang_fetch_vec4 (struct gl2_vertex_shader_intf **, const char *, GLfloat *, GLuint, int);
extern int _slang_fetch_mat3 (struct gl2_vertex_shader_intf **, const char *, GLfloat *, GLuint, int);
extern int _slang_fetch_mat4 (struct gl2_vertex_shader_intf **, const char *, GLfloat *, GLuint, int);

static void fetch_input_float (const char *name, GLuint attr, GLuint i, struct vertex_buffer *vb,
	struct gl2_vertex_shader_intf **vs)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint size = vb->AttribPtr[attr]->size;
	const GLuint stride = vb->AttribPtr[attr]->stride;
	const GLfloat *data = (const GLfloat *) (ptr + stride * i);
	float vec[1];

	vec[0] = data[0];
	_slang_fetch_float (vs, name, vec, 1);
}

static void fetch_input_vec3 (const char *name, GLuint attr, GLuint i, struct vertex_buffer *vb,
	struct gl2_vertex_shader_intf **vs)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint size = vb->AttribPtr[attr]->size;
	const GLuint stride = vb->AttribPtr[attr]->stride;
	const GLfloat *data = (const GLfloat *) (ptr + stride * i);
	float vec[3];

	vec[0] = data[0];
	vec[1] = data[1];
	vec[2] = data[2];
	_slang_fetch_vec3 (vs, name, vec, 1);
}

static void fetch_input_vec4 (const char *name, GLuint attr, GLuint i, struct vertex_buffer *vb,
	struct gl2_vertex_shader_intf **vs)
{
	const GLubyte *ptr = (const GLubyte *) vb->AttribPtr[attr]->data;
	const GLuint size = vb->AttribPtr[attr]->size;
	const GLuint stride = vb->AttribPtr[attr]->stride;
	const GLfloat *data = (const GLfloat *) (ptr + stride * i);
	float vec[4];

	switch (size)
	{
	case 2:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = 0.0f;
		vec[3] = 1.0f;
		break;
	case 3:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = data[2];
		vec[3] = 1.0f;
		break;
	case 4:
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = data[2];
		vec[3] = data[3];
		break;
	}
	_slang_fetch_vec4 (vs, name, vec, 0, 1);
}

static void fetch_output_float (const char *name, GLuint attr, GLuint i, arbvs_stage_data *store,
	struct gl2_vertex_shader_intf **vs)
{
	float vec[1];

	_slang_fetch_float (vs, name, vec, 0);
	_mesa_memcpy (&store->outputs[attr].data[i], vec, 4);
}

static void fetch_output_vec4 (const char *name, GLuint attr, GLuint i, GLuint index,
	arbvs_stage_data *store, struct gl2_vertex_shader_intf **vs)
{
	float vec[4];

	_slang_fetch_vec4 (vs, name, vec, index, 0);
	_mesa_memcpy (&store->outputs[attr].data[i], vec, 16);
}

static void fetch_uniform_mat4 (const char *name, const GLmatrix *matrix, GLuint index,
	struct gl2_vertex_shader_intf **vs)
{
	/* XXX: transpose? */
	_slang_fetch_mat4 (vs, name, matrix->m, index, 1);
}

static void fetch_normal_matrix (const char *name, GLmatrix *matrix,
	struct gl2_vertex_shader_intf **vs)
{
	GLfloat mat[9];

	_math_matrix_analyse (matrix);
	mat[0] = matrix->inv[0];
	mat[1] = matrix->inv[1];
	mat[2] = matrix->inv[2];
	mat[3] = matrix->inv[4];
	mat[4] = matrix->inv[5];
	mat[5] = matrix->inv[6];
	mat[6] = matrix->inv[8];
	mat[7] = matrix->inv[9];
	mat[8] = matrix->inv[10];
	_slang_fetch_mat3 (vs, name, mat, 0, 1);
}

static GLboolean run_arb_vertex_shader (GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *vb = &tnl->vb;
	arbvs_stage_data *store = ARBVS_STAGE_DATA(stage);
	struct gl2_program_intf **prog;
	struct gl2_vertex_shader_intf **vs = NULL;
	GLsizei count, i, j;

	prog = ctx->ShaderObjects.CurrentProgram;
	if (prog == NULL)
		return GL_TRUE;

	count = (**prog)._container.GetAttachedCount ((struct gl2_container_intf **) prog);
	for (i = 0; i < count; i++)
	{
		struct gl2_generic_intf **obj;
		struct gl2_unknown_intf **unk;

		obj = (**prog)._container.GetAttached ((struct gl2_container_intf **) prog, i);
		unk = (**obj)._unknown.QueryInterface ((struct gl2_unknown_intf **) obj, UIID_VERTEX_SHADER);
		(**obj)._unknown.Release ((struct gl2_unknown_intf **) obj);
		if (unk != NULL)
		{
			vs = (struct gl2_vertex_shader_intf **) unk;
			break;
		}
	}
	if (vs == NULL)
		return GL_TRUE;

	fetch_uniform_mat4 ("gl_ModelViewMatrix", ctx->ModelviewMatrixStack.Top, 0, vs);
	fetch_uniform_mat4 ("gl_ProjectionMatrix", ctx->ProjectionMatrixStack.Top, 0, vs);
	fetch_uniform_mat4 ("gl_ModelViewProjectionMatrix", &ctx->_ModelProjectMatrix, 0, vs);
	for (j = 0; j < 8; j++)
		fetch_uniform_mat4 ("gl_TextureMatrix", ctx->TextureMatrixStack[j].Top, j, vs);
	fetch_normal_matrix ("gl_NormalMatrix", ctx->ModelviewMatrixStack.Top, vs);
	/* XXX: fetch uniform mat4 gl_ModelViewMatrixInverse */
	/* XXX: fetch uniform mat4 gl_ProjectionMatrixInverse */
	/* XXX: fetch uniform mat4 gl_ModelViewProjectionMatrixInverse */
	/* XXX: fetch uniform mat4 gl_TextureMatrixInverse */
	/* XXX: fetch uniform mat4 gl_ModelViewMatrixTranspose */
	/* XXX: fetch uniform mat4 gl_ProjectionMatrixTranspose */
	/* XXX: fetch uniform mat4 gl_ModelViewProjectionMatrixTranspose */
	/* XXX: fetch uniform mat4 gl_TextureMatrixTranspose */
	/* XXX: fetch uniform mat4 gl_ModelViewMatrixInverseTranspose */
	/* XXX: fetch uniform mat4 gl_ProjectionMatrixInverseTranspose */
	/* XXX: fetch uniform mat4 gl_ModelViewProjectionMatrixInverseTranspose */
	/* XXX: fetch uniform mat4 gl_TextureMatrixInverseTranspose */
	/* XXX: fetch uniform float gl_NormalScale */
	/* XXX: fetch uniform mat4 gl_ClipPlane */
	/* XXX: fetch uniform mat4 gl_TextureEnvColor */
	/* XXX: fetch uniform mat4 gl_EyePlaneS */
	/* XXX: fetch uniform mat4 gl_EyePlaneT */
	/* XXX: fetch uniform mat4 gl_EyePlaneR */
	/* XXX: fetch uniform mat4 gl_EyePlaneQ */
	/* XXX: fetch uniform mat4 gl_ObjectPlaneS */
	/* XXX: fetch uniform mat4 gl_ObjectPlaneT */
	/* XXX: fetch uniform mat4 gl_ObjectPlaneR */
	/* XXX: fetch uniform mat4 gl_ObjectPlaneQ */

	for (i = 0; i < vb->Count; i++)
	{
		fetch_input_vec4 ("gl_Vertex", _TNL_ATTRIB_POS, i, vb, vs);
		fetch_input_vec3 ("gl_Normal", _TNL_ATTRIB_NORMAL, i, vb, vs);
		fetch_input_vec4 ("gl_Color", _TNL_ATTRIB_COLOR0, i, vb, vs);
		fetch_input_vec4 ("gl_SecondaryColor", _TNL_ATTRIB_COLOR1, i, vb, vs);
		fetch_input_float ("gl_FogCoord", _TNL_ATTRIB_FOG, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord0", _TNL_ATTRIB_TEX0, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord1", _TNL_ATTRIB_TEX1, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord2", _TNL_ATTRIB_TEX2, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord3", _TNL_ATTRIB_TEX3, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord4", _TNL_ATTRIB_TEX4, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord5", _TNL_ATTRIB_TEX5, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord6", _TNL_ATTRIB_TEX6, i, vb, vs);
		fetch_input_vec4 ("gl_MultiTexCoord7", _TNL_ATTRIB_TEX7, i, vb, vs);

		exec_vertex_shader (vs);

		fetch_output_vec4 ("gl_Position", VERT_RESULT_HPOS, i, 0, store, vs);
		fetch_output_vec4 ("gl_FrontColor", VERT_RESULT_COL0, i, 0, store, vs);
		fetch_output_vec4 ("gl_FrontSecondaryColor", VERT_RESULT_COL1, i, 0, store, vs);
		fetch_output_float ("gl_FogFragCoord", VERT_RESULT_FOGC, i, store, vs);
		for (j = 0; j < 8; j++)
			fetch_output_vec4 ("gl_TexCoord", VERT_RESULT_TEX0 + j, i, j, store, vs);
		fetch_output_float ("gl_PointSize", VERT_RESULT_PSIZ, i, store, vs);
		fetch_output_vec4 ("gl_BackColor", VERT_RESULT_BFC0, i, 0, store, vs);
		fetch_output_vec4 ("gl_BackSecondaryColor", VERT_RESULT_BFC1, i, 0, store, vs);
		/* XXX: fetch output gl_ClipVertex */
	}

	(**vs)._shader._generic._unknown.Release ((struct gl2_unknown_intf **) vs);

	vb->ClipPtr = &store->outputs[VERT_RESULT_HPOS];
	vb->ClipPtr->count = vb->Count;
	vb->ColorPtr[0] = &store->outputs[VERT_RESULT_COL0];
	vb->SecondaryColorPtr[0] = &store->outputs[VERT_RESULT_COL1];
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		vb->TexCoordPtr[i] = &store->outputs[VERT_RESULT_TEX0 + i];
	vb->ColorPtr[1] = &store->outputs[VERT_RESULT_BFC0];
	vb->SecondaryColorPtr[1] = &store->outputs[VERT_RESULT_BFC1];
	vb->FogCoordPtr = &store->outputs[VERT_RESULT_FOGC];
	vb->PointSizePtr = &store->outputs[VERT_RESULT_PSIZ];

	vb->AttribPtr[VERT_ATTRIB_COLOR0] = vb->ColorPtr[0];
	vb->AttribPtr[VERT_ATTRIB_COLOR1] = vb->SecondaryColorPtr[0];
	vb->AttribPtr[VERT_ATTRIB_FOG] = vb->FogCoordPtr;
	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		vb->AttribPtr[VERT_ATTRIB_TEX0 + i] = vb->TexCoordPtr[i];
	vb->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &store->outputs[VERT_RESULT_PSIZ];

	store->ormask = 0;
	store->andmask = CLIP_ALL_BITS;

	if (tnl->NeedNdcCoords)
	{
		vb->NdcPtr = _mesa_clip_tab[vb->ClipPtr->size] (vb->ClipPtr, &store->ndc_coords,
			store->clipmask, &store->ormask, &store->andmask);
	}
	else
	{
		vb->NdcPtr = NULL;
		_mesa_clip_np_tab[vb->ClipPtr->size] (vb->ClipPtr, NULL, store->clipmask, &store->ormask,
			&store->andmask);
	}

	if (store->andmask)
		return GL_FALSE;

	vb->ClipAndMask = store->andmask;
	vb->ClipOrMask = store->ormask;
	vb->ClipMask = store->clipmask;

	return GL_TRUE;
}

const struct tnl_pipeline_stage _tnl_arb_vertex_shader_stage = {
	"ARB_vertex_shader",
	NULL,
	construct_arb_vertex_shader,
	destruct_arb_vertex_shader,
	validate_arb_vertex_shader,
	run_arb_vertex_shader
};

