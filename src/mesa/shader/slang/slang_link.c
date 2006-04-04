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
 */

/**
 * \file slang_link.c
 * slang linker
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_link.h"
#include "slang_analyse.h"

/*
 * slang_uniform_bindings
 */

static GLvoid slang_uniform_bindings_ctr (slang_uniform_bindings *self)
{
	self->table = NULL;
	self->count = 0;
}

static GLvoid slang_uniform_bindings_dtr (slang_uniform_bindings *self)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		slang_alloc_free (self->table[i].name);
	slang_alloc_free (self->table);
}

static GLboolean slang_uniform_bindings_add (slang_uniform_bindings *self, slang_export_data_quant *q,
	const char *name, GLuint index, GLuint address)
{
	const GLuint n = self->count;
	GLuint i;

	for (i = 0; i < n; i++)
		if (slang_string_compare (self->table[i].name, name) == 0)
		{
			self->table[i].address[index] = address;
			return GL_TRUE;
		}

	self->table = (slang_uniform_binding *) slang_alloc_realloc (self->table,
		n * sizeof (slang_uniform_binding), (n + 1) * sizeof (slang_uniform_binding));
	if (self->table == NULL)
		return GL_FALSE;
	self->table[n].quant = q;
	self->table[n].name = slang_string_duplicate (name);
	for (i = 0; i < SLANG_SHADER_MAX; i++)
		self->table[n].address[i] = ~0;
	self->table[n].address[index] = address;
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;
	return GL_TRUE;
}

static GLboolean insert_uniform_binding (slang_uniform_bindings *bind, slang_export_data_quant *q,
	char *name, slang_atom_pool *atoms, GLuint index, GLuint addr)
{
	GLuint count, i;

	slang_string_concat (name, slang_atom_pool_id (atoms, q->name));
	count = slang_export_data_quant_elements (q);
	for (i = 0; i < count; i++)
	{
		GLuint save;

		save = slang_string_length (name);
		if (slang_export_data_quant_array (q))
			_mesa_sprintf (name + slang_string_length (name), "[%d]", i);

		if (slang_export_data_quant_struct (q))
		{
			GLuint save, i;
			const GLuint fields = slang_export_data_quant_fields (q);

			slang_string_concat (name, ".");
			save = slang_string_length (name);

			for (i = 0; i < fields; i++)
			{
				if (!insert_uniform_binding (bind, &q->structure[i], name, atoms, index, addr))
					return GL_FALSE;
				name[save] = '\0';
				addr += slang_export_data_quant_size (&q->structure[i]);
			}
		}
		else
		{
			if (!slang_uniform_bindings_add (bind, q, name, index, addr))
				return GL_FALSE;
			addr += slang_export_data_quant_size (q);
		}
		name[save] = '\0';
	}

	return GL_TRUE;
}

static GLboolean gather_uniform_bindings (slang_uniform_bindings *bind,
	slang_export_data_table *tbl, GLuint index)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_uniform)
		{
			char name[1024] = "";

			if (!insert_uniform_binding (bind, &tbl->entries[i].quant, name, tbl->atoms, index,
					tbl->entries[i].address))
				return GL_FALSE;
		}

	return GL_TRUE;
}

/*
 * slang_active_uniforms
 */

static GLvoid slang_active_uniforms_ctr (slang_active_uniforms *self)
{
	self->table = NULL;
	self->count = 0;
}

static GLvoid slang_active_uniforms_dtr (slang_active_uniforms *self)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		slang_alloc_free (self->table[i].name);
	slang_alloc_free (self->table);
}

static GLboolean slang_active_uniforms_add (slang_active_uniforms *self, slang_export_data_quant *q,
	const char *name)
{
	const GLuint n = self->count;

	self->table = (slang_active_uniform *) slang_alloc_realloc (self->table,
		n * sizeof (slang_active_uniform), (n + 1) * sizeof (slang_active_uniform));
	if (self->table == NULL)
		return GL_FALSE;
	self->table[n].quant = q;
	self->table[n].name = slang_string_duplicate (name);
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;
	return GL_TRUE;
}

static GLboolean insert_uniform (slang_active_uniforms *u, slang_export_data_quant *q, char *name,
	slang_atom_pool *atoms)
{
	slang_string_concat (name, slang_atom_pool_id (atoms, q->name));
	if (slang_export_data_quant_array (q))
		slang_string_concat (name, "[0]");

	if (slang_export_data_quant_struct (q))
	{
		GLuint save, i;
		const GLuint fields = slang_export_data_quant_fields (q);

		slang_string_concat (name, ".");
		save = slang_string_length (name);

		for (i = 0; i < fields; i++)
		{
			if (!insert_uniform (u, &q->structure[i], name, atoms))
				return GL_FALSE;
			name[save] = '\0';
		}

		return GL_TRUE;
	}

	return slang_active_uniforms_add (u, q, name);
}

static GLboolean gather_active_uniforms (slang_active_uniforms *u, slang_export_data_table *tbl)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_uniform)
		{
			char name[1024] = "";

			if (!insert_uniform (u, &tbl->entries[i].quant, name, tbl->atoms))
				return GL_FALSE;
		}

	return GL_TRUE;
}

/*
 * slang_varying_bindings
 */

static GLvoid slang_varying_bindings_ctr (slang_varying_bindings *self)
{
	self->count = 0;
	self->total = 0;
}

static GLvoid slang_varying_bindings_dtr (slang_varying_bindings *self)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		slang_alloc_free (self->table[i].name);
}

static GLvoid update_varying_slots (slang_varying_slot *slots, GLuint count, GLboolean vert,
	GLuint address, GLuint do_offset)
{
	GLuint i;

	for (i = 0; i < count; i++)
		if (vert)
			slots[i].vert_addr = address + i * 4 * do_offset;
		else
			slots[i].frag_addr = address + i * 4 * do_offset;
}

static GLboolean slang_varying_bindings_add (slang_varying_bindings *self,
	slang_export_data_quant *q, const char *name, GLboolean vert, GLuint address)
{
	const GLuint n = self->count;
	const GLuint total_components =
		slang_export_data_quant_components (q) * slang_export_data_quant_elements (q);
	GLuint i;

	for (i = 0; i < n; i++)
		if (slang_string_compare (self->table[i].name, name) == 0)
		{
			/* TODO: data quantities must match, or else link fails */
			update_varying_slots (&self->slots[self->table[i].slot], total_components, vert,
				address, 1);
			return GL_TRUE;
		}

	if (self->total + total_components > MAX_VARYING_FLOATS)
	{
		/* TODO: info log: error: MAX_VARYING_FLOATS exceeded */
		return GL_FALSE;
	}

	self->table[n].quant = q;
	self->table[n].slot = self->total;
	self->table[n].name = slang_string_duplicate (name);
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;

	update_varying_slots (&self->slots[self->table[n].slot], total_components, vert, address, 1);
	update_varying_slots (&self->slots[self->table[n].slot], total_components, !vert, ~0, 0);
	self->total += total_components;

	return GL_TRUE;
}

static GLboolean entry_has_gl_prefix (slang_atom name, slang_atom_pool *atoms)
{
	const char *str = slang_atom_pool_id (atoms, name);
	return str[0] == 'g' && str[1] == 'l' && str[2] == '_';
}

static GLboolean gather_varying_bindings (slang_varying_bindings *bind,
	slang_export_data_table *tbl, GLboolean vert)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_varying &&
			!entry_has_gl_prefix (tbl->entries[i].quant.name, tbl->atoms))
		{
			if (!slang_varying_bindings_add (bind, &tbl->entries[i].quant,
				slang_atom_pool_id (tbl->atoms, tbl->entries[i].quant.name), vert,
				tbl->entries[i].address))
				return GL_FALSE;
		}

	return GL_TRUE;
}

/*
 * slang_texture_bindings
 */

GLvoid slang_texture_usages_ctr (slang_texture_usages *self)
{
	self->table = NULL;
	self->count = 0;
}

GLvoid slang_texture_usages_dtr (slang_texture_usages *self)
{
	slang_alloc_free (self->table);
}

/*
 * slang_program
 */

GLvoid slang_program_ctr (slang_program *self)
{
	GLuint i;

	slang_uniform_bindings_ctr (&self->uniforms);
	slang_active_uniforms_ctr (&self->active_uniforms);
	slang_varying_bindings_ctr (&self->varyings);
	slang_texture_usages_ctr (&self->texture_usage);
	for (i = 0; i < SLANG_SHADER_MAX; i++)
	{
		GLuint j;

		for (j = 0; j < SLANG_COMMON_FIXED_MAX; j++)
			self->common_fixed_entries[i][j] = ~0;
		for (j = 0; j < SLANG_COMMON_CODE_MAX; j++)
			self->code[i][j] = ~0;
		self->machines[i] = NULL;
		self->assemblies[i] = NULL;
	}
	for (i = 0; i < SLANG_VERTEX_FIXED_MAX; i++)
		self->vertex_fixed_entries[i] = ~0;
	for (i = 0; i < SLANG_FRAGMENT_FIXED_MAX; i++)
		self->fragment_fixed_entries[i] = ~0;
}

GLvoid slang_program_dtr (slang_program *self)
{
	slang_uniform_bindings_dtr (&self->uniforms);
	slang_active_uniforms_dtr (&self->active_uniforms);
	slang_varying_bindings_dtr (&self->varyings);
	slang_texture_usages_dtr (&self->texture_usage);
}

/*
 * _slang_link()
 */

static GLuint gd (slang_export_data_table *tbl, const char *name)
{
	slang_atom atom;
	GLuint i;

	atom = slang_atom_pool_atom (tbl->atoms, name);
	if (atom == SLANG_ATOM_NULL)
		return ~0;

	for (i = 0; i < tbl->count; i++)
		if (atom == tbl->entries[i].quant.name)
			return tbl->entries[i].address;
	return ~0;
}

static GLvoid resolve_common_fixed (GLuint e[], slang_export_data_table *tbl)
{
	e[SLANG_COMMON_FIXED_MODELVIEWMATRIX] = gd (tbl, "gl_ModelViewMatrix");
	e[SLANG_COMMON_FIXED_PROJECTIONMATRIX] = gd (tbl, "gl_ProjectionMatrix");
	e[SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIX] = gd (tbl, "gl_ModelViewProjectionMatrix");
	e[SLANG_COMMON_FIXED_TEXTUREMATRIX] = gd (tbl, "gl_TextureMatrix");
	e[SLANG_COMMON_FIXED_NORMALMATRIX] = gd (tbl, "gl_NormalMatrix");
	e[SLANG_COMMON_FIXED_MODELVIEWMATRIXINVERSE] = gd (tbl, "gl_ModelViewMatrixInverse");
	e[SLANG_COMMON_FIXED_PROJECTIONMATRIXINVERSE] = gd (tbl, "gl_ProjectionMatrixInverse");
	e[SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXINVERSE] =
		gd (tbl, "gl_ModelViewProjectionMatrixInverse");
	e[SLANG_COMMON_FIXED_TEXTUREMATRIXINVERSE] = gd (tbl, "gl_TextureMatrixInverse");
	e[SLANG_COMMON_FIXED_MODELVIEWMATRIXTRANSPOSE] = gd (tbl, "gl_ModelViewMatrixTranspose");
	e[SLANG_COMMON_FIXED_PROJECTIONMATRIXTRANSPOSE] = gd (tbl, "gl_ProjectionMatrixTranspose");
	e[SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXTRANSPOSE] =
		gd (tbl, "gl_ModelViewProjectionMatrixTranspose");
	e[SLANG_COMMON_FIXED_TEXTUREMATRIXTRANSPOSE] = gd (tbl, "gl_TextureMatrixTranspose");
	e[SLANG_COMMON_FIXED_MODELVIEWMATRIXINVERSETRANSPOSE] =
		gd (tbl, "gl_ModelViewMatrixInverseTranspose");
	e[SLANG_COMMON_FIXED_PROJECTIONMATRIXINVERSETRANSPOSE] =
		gd (tbl, "gl_ProjectionMatrixInverseTranspose");
	e[SLANG_COMMON_FIXED_MODELVIEWPROJECTIONMATRIXINVERSETRANSPOSE] =
		gd (tbl, "gl_ModelViewProjectionMatrixInverseTranspose");
	e[SLANG_COMMON_FIXED_TEXTUREMATRIXINVERSETRANSPOSE] =
		gd (tbl, "gl_TextureMatrixInverseTranspose");
	e[SLANG_COMMON_FIXED_NORMALSCALE] = gd (tbl, "gl_NormalScale");
	e[SLANG_COMMON_FIXED_DEPTHRANGE] = gd (tbl, "gl_DepthRange");
	e[SLANG_COMMON_FIXED_CLIPPLANE] = gd (tbl, "gl_ClipPlane");
	e[SLANG_COMMON_FIXED_POINT] = gd (tbl, "gl_Point");
	e[SLANG_COMMON_FIXED_FRONTMATERIAL] = gd (tbl, "gl_FrontMaterial");
	e[SLANG_COMMON_FIXED_BACKMATERIAL] = gd (tbl, "gl_BackMaterial");
	e[SLANG_COMMON_FIXED_LIGHTSOURCE] = gd (tbl, "gl_LightSource");
	e[SLANG_COMMON_FIXED_LIGHTMODEL] = gd (tbl, "gl_LightModel");
	e[SLANG_COMMON_FIXED_FRONTLIGHTMODELPRODUCT] = gd (tbl, "gl_FrontLightModelProduct");
	e[SLANG_COMMON_FIXED_BACKLIGHTMODELPRODUCT] = gd (tbl, "gl_BackLightModelProduct");
	e[SLANG_COMMON_FIXED_FRONTLIGHTPRODUCT] = gd (tbl, "gl_FrontLightProduct");
	e[SLANG_COMMON_FIXED_BACKLIGHTPRODUCT] = gd (tbl, "gl_BackLightProduct");
	e[SLANG_COMMON_FIXED_TEXTUREENVCOLOR] = gd (tbl, "gl_TextureEnvColor");
	e[SLANG_COMMON_FIXED_EYEPLANES] = gd (tbl, "gl_EyePlaneS");
	e[SLANG_COMMON_FIXED_EYEPLANET] = gd (tbl, "gl_EyePlaneT");
	e[SLANG_COMMON_FIXED_EYEPLANER] = gd (tbl, "gl_EyePlaneR");
	e[SLANG_COMMON_FIXED_EYEPLANEQ] = gd (tbl, "gl_EyePlaneQ");
	e[SLANG_COMMON_FIXED_OBJECTPLANES] = gd (tbl, "gl_ObjectPlaneS");
	e[SLANG_COMMON_FIXED_OBJECTPLANET] = gd (tbl, "gl_ObjectPlaneT");
	e[SLANG_COMMON_FIXED_OBJECTPLANER] = gd (tbl, "gl_ObjectPlaneR");
	e[SLANG_COMMON_FIXED_OBJECTPLANEQ] = gd (tbl, "gl_ObjectPlaneQ");
	e[SLANG_COMMON_FIXED_FOG] = gd (tbl, "gl_Fog");
}

static GLvoid resolve_vertex_fixed (GLuint e[], slang_export_data_table *tbl)
{
	e[SLANG_VERTEX_FIXED_POSITION] = gd (tbl, "gl_Position");
	e[SLANG_VERTEX_FIXED_POINTSIZE] = gd (tbl,  "gl_PointSize");
	e[SLANG_VERTEX_FIXED_CLIPVERTEX] = gd (tbl, "gl_ClipVertex");
	e[SLANG_VERTEX_FIXED_COLOR] = gd (tbl, "gl_Color");
	e[SLANG_VERTEX_FIXED_SECONDARYCOLOR] = gd (tbl, "gl_SecondaryColor");
	e[SLANG_VERTEX_FIXED_NORMAL] = gd (tbl, "gl_Normal");
	e[SLANG_VERTEX_FIXED_VERTEX] = gd (tbl, "gl_Vertex");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD0] = gd (tbl, "gl_MultiTexCoord0");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD1] = gd (tbl, "gl_MultiTexCoord1");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD2] = gd (tbl, "gl_MultiTexCoord2");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD3] = gd (tbl, "gl_MultiTexCoord3");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD4] = gd (tbl, "gl_MultiTexCoord4");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD5] = gd (tbl, "gl_MultiTexCoord5");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD6] = gd (tbl, "gl_MultiTexCoord6");
	e[SLANG_VERTEX_FIXED_MULTITEXCOORD7] = gd (tbl, "gl_MultiTexCoord7");
	e[SLANG_VERTEX_FIXED_FOGCOORD] = gd (tbl, "gl_FogCoord");
	e[SLANG_VERTEX_FIXED_FRONTCOLOR] = gd (tbl, "gl_FrontColor");
	e[SLANG_VERTEX_FIXED_BACKCOLOR] = gd (tbl, "gl_BackColor");
	e[SLANG_VERTEX_FIXED_FRONTSECONDARYCOLOR] = gd (tbl, "gl_FrontSecondaryColor");
	e[SLANG_VERTEX_FIXED_BACKSECONDARYCOLOR] = gd (tbl, "gl_BackSecondaryColor");
	e[SLANG_VERTEX_FIXED_TEXCOORD] = gd (tbl, "gl_TexCoord");
	e[SLANG_VERTEX_FIXED_FOGFRAGCOORD] = gd (tbl, "gl_FogFragCoord");
}

static GLvoid resolve_fragment_fixed (GLuint e[], slang_export_data_table *tbl)
{
	e[SLANG_FRAGMENT_FIXED_FRAGCOORD] = gd (tbl, "gl_FragCoord");
	e[SLANG_FRAGMENT_FIXED_FRONTFACING] = gd (tbl, "gl_FrontFacing");
	e[SLANG_FRAGMENT_FIXED_FRAGCOLOR] = gd (tbl, "gl_FragColor");
	e[SLANG_FRAGMENT_FIXED_FRAGDATA] = gd (tbl, "gl_FragData");
	e[SLANG_FRAGMENT_FIXED_FRAGDEPTH] = gd (tbl, "gl_FragDepth");
	e[SLANG_FRAGMENT_FIXED_COLOR] = gd (tbl, "gl_Color");
	e[SLANG_FRAGMENT_FIXED_SECONDARYCOLOR] = gd (tbl, "gl_SecondaryColor");
	e[SLANG_FRAGMENT_FIXED_TEXCOORD] = gd (tbl, "gl_TexCoord");
	e[SLANG_FRAGMENT_FIXED_FOGFRAGCOORD] = gd (tbl, "gl_FogFragCoord");
}

static GLuint gc (slang_export_code_table *tbl, const char *name)
{
	slang_atom atom;
	GLuint i;

	atom = slang_atom_pool_atom (tbl->atoms, name);
	if (atom == SLANG_ATOM_NULL)
		return ~0;

	for (i = 0; i < tbl->count; i++)
		if (atom == tbl->entries[i].name)
			return tbl->entries[i].address;
	return ~0;
}

static GLvoid resolve_common_code (GLuint code[], slang_export_code_table *tbl)
{
	code[SLANG_COMMON_CODE_MAIN] = gc (tbl, "@main");
}

GLboolean _slang_link (slang_program *prog, slang_translation_unit **units, GLuint count)
{
	GLuint i;

	for (i = 0; i < count; i++)
	{
		GLuint index;

		if (units[i]->type == slang_unit_fragment_shader)
		{
			index = SLANG_SHADER_FRAGMENT;
			resolve_fragment_fixed (prog->fragment_fixed_entries, &units[i]->exp_data);
		}
		else
		{
			index = SLANG_SHADER_VERTEX;
			resolve_vertex_fixed (prog->vertex_fixed_entries, &units[i]->exp_data);
		}

		if (!gather_uniform_bindings (&prog->uniforms, &units[i]->exp_data, index))
			return GL_FALSE;
		if (!gather_active_uniforms (&prog->active_uniforms, &units[i]->exp_data))
			return GL_FALSE;
		if (!gather_varying_bindings (&prog->varyings, &units[i]->exp_data,
			index == SLANG_SHADER_VERTEX))
			return GL_FALSE;
		resolve_common_fixed (prog->common_fixed_entries[index], &units[i]->exp_data);
		resolve_common_code (prog->code[index], &units[i]->exp_code);
		prog->machines[index] = units[i]->machine;
		prog->assemblies[index] = units[i]->assembly;
	}

	/* TODO: all varyings read by fragment shader must be written by vertex shader */

	if (!_slang_analyse_texture_usage (prog))
		return GL_FALSE;

	return GL_TRUE;
}

