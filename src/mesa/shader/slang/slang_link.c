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

static GLboolean entry_has_gl_prefix (slang_atom name, slang_atom_pool *atoms)
{
	const char *str = slang_atom_pool_id (atoms, name);
	return str[0] == 'g' && str[1] == 'l' && str[2] == '_';
}

/*
 * slang_active_variables
 */

static GLvoid slang_active_variables_ctr (slang_active_variables *self)
{
	self->table = NULL;
	self->count = 0;
}

static GLvoid slang_active_variables_dtr (slang_active_variables *self)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		slang_alloc_free (self->table[i].name);
	slang_alloc_free (self->table);
}

static GLboolean add_simple_variable (slang_active_variables *self, slang_export_data_quant *q,
	const char *name)
{
	const GLuint n = self->count;

	self->table = (slang_active_variable *) slang_alloc_realloc (self->table,
		n * sizeof (slang_active_variable), (n + 1) * sizeof (slang_active_variable));
	if (self->table == NULL)
		return GL_FALSE;

	self->table[n].quant = q;
	self->table[n].name = slang_string_duplicate (name);
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;

	return GL_TRUE;
}

static GLboolean add_complex_variable (slang_active_variables *self, slang_export_data_quant *q,
	char *name, slang_atom_pool *atoms)
{
	slang_string_concat (name, slang_atom_pool_id (atoms, q->name));
	if (slang_export_data_quant_array (q))
		slang_string_concat (name, "[0]");

	if (slang_export_data_quant_struct (q))
	{
		GLuint dot_pos, i;
		const GLuint fields = slang_export_data_quant_fields (q);

		slang_string_concat (name, ".");
		dot_pos = slang_string_length (name);

		for (i = 0; i < fields; i++)
		{
			if (!add_complex_variable (self, &q->structure[i], name, atoms))
				return GL_FALSE;

			name[dot_pos] = '\0';
		}

		return GL_TRUE;
	}

	return add_simple_variable (self, q, name);
}

static GLboolean gather_active_variables (slang_active_variables *self,
	slang_export_data_table *tbl, slang_export_data_access access)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == access)
		{
			char name[1024] = "";

			if (!add_complex_variable (self, &tbl->entries[i].quant, name, tbl->atoms))
				return GL_FALSE;
		}

	return GL_TRUE;
}

/*
 * slang_attrib_overrides
 */

static GLvoid slang_attrib_overrides_ctr (slang_attrib_overrides *self)
{
	self->table = NULL;
	self->count = 0;
}

static GLvoid slang_attrib_overrides_dtr (slang_attrib_overrides *self)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		slang_alloc_free (self->table[i].name);
	slang_alloc_free (self->table);
}

GLboolean slang_attrib_overrides_add (slang_attrib_overrides *self, GLuint index, const GLchar *name)
{
	const GLuint n = self->count;
	GLuint i;

	for (i = 0; i < n; i++)
		if (slang_string_compare (name, self->table[i].name) == 0)
		{
			self->table[i].index = index;
			return GL_TRUE;
		}

	self->table = (slang_attrib_override *) slang_alloc_realloc (self->table,
		n * sizeof (slang_attrib_override), (n + 1) * sizeof (slang_attrib_override));
	if (self->table == NULL)
		return GL_FALSE;

	self->table[n].index = index;
	self->table[n].name = slang_string_duplicate (name);
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;

	return GL_TRUE;
}

static GLuint lookup_attrib_override (slang_attrib_overrides *self, const GLchar *name)
{
	GLuint i;

	for (i = 0; i < self->count; i++)
		if (slang_string_compare (name, self->table[i].name) == 0)
			return self->table[i].index;
	return MAX_VERTEX_ATTRIBS;
}

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

static GLboolean add_simple_uniform_binding (slang_uniform_bindings *self,
	slang_export_data_quant *q, const char *name, GLuint index, GLuint addr)
{
	const GLuint n = self->count;
	GLuint i;

	for (i = 0; i < n; i++)
		if (slang_string_compare (self->table[i].name, name) == 0)
		{
			self->table[i].address[index] = addr;
			return GL_TRUE;
		}

	self->table = (slang_uniform_binding *) slang_alloc_realloc (self->table,
		n * sizeof (slang_uniform_binding), (n + 1) * sizeof (slang_uniform_binding));
	if (self->table == NULL)
		return GL_FALSE;

	self->table[n].quant = q;
	self->table[n].name = slang_string_duplicate (name);
	if (self->table[n].name == NULL)
		return GL_FALSE;
	for (i = 0; i < SLANG_SHADER_MAX; i++)
		self->table[n].address[i] = ~0;
	self->table[n].address[index] = addr;
	self->count++;

	return GL_TRUE;
}

static GLboolean add_complex_uniform_binding (slang_uniform_bindings *self,
	slang_export_data_quant *q, char *name, slang_atom_pool *atoms, GLuint index, GLuint addr)
{
	GLuint count, i;

	slang_string_concat (name, slang_atom_pool_id (atoms, q->name));
	count = slang_export_data_quant_elements (q);
	for (i = 0; i < count; i++)
	{
		GLuint bracket_pos;

		bracket_pos = slang_string_length (name);
		if (slang_export_data_quant_array (q))
			_mesa_sprintf (name + slang_string_length (name), "[%d]", i);

		if (slang_export_data_quant_struct (q))
		{
			GLuint dot_pos, i;
			const GLuint fields = slang_export_data_quant_fields (q);

			slang_string_concat (name, ".");
			dot_pos = slang_string_length (name);

			for (i = 0; i < fields; i++)
			{
				if (!add_complex_uniform_binding (self, &q->structure[i], name, atoms, index, addr))
					return GL_FALSE;

				name[dot_pos] = '\0';
				addr += slang_export_data_quant_size (&q->structure[i]);
			}
		}
		else
		{
			if (!add_simple_uniform_binding (self, q, name, index, addr))
				return GL_FALSE;

			addr += slang_export_data_quant_size (q);
		}

		name[bracket_pos] = '\0';
	}

	return GL_TRUE;
}

static GLboolean gather_uniform_bindings (slang_uniform_bindings *self,
	slang_export_data_table *tbl, GLuint index)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_uniform)
		{
			char name[1024] = "";

			if (!add_complex_uniform_binding (self, &tbl->entries[i].quant, name, tbl->atoms, index,
				tbl->entries[i].address))
				return GL_FALSE;
		}

	return GL_TRUE;
}

/*
 * slang_attrib_bindings
 */

static GLvoid slang_attrib_bindings_ctr (slang_attrib_bindings *self)
{
	GLuint i;

	self->binding_count = 0;
	for (i = 0; i < MAX_VERTEX_ATTRIBS; i++)
		self->slots[i].addr = ~0;
}

static GLvoid slang_attrib_bindings_dtr (slang_attrib_bindings *self)
{
	GLuint i;

	for (i = 0; i < self->binding_count; i++)
		slang_alloc_free (self->bindings[i].name);
}

/*
 * NOTE: If conventional vertex attribute gl_Vertex is used, application cannot use
 *       vertex attrib index 0 for binding override. Currently this is not checked.
 *       Although attrib index 0 is not used when not explicitly asked.
 */

static GLuint can_allocate_attrib_slots (slang_attrib_bindings *self, GLuint index, GLuint count)
{
	GLuint i;

	for (i = 0; i < count; i++)
		if (self->slots[index + i].addr != ~0)
			break;
	return i;
}

static GLuint allocate_attrib_slots (slang_attrib_bindings *self, GLuint count)
{
	GLuint i;

   for (i = 1; i <= MAX_VERTEX_ATTRIBS - count; i++)
	{
		GLuint size;
		
		size = can_allocate_attrib_slots (self, i, count);
		if (size == count)
			return i;

		/* speed-up the search a bit */
      i += size;
	}
	return MAX_VERTEX_ATTRIBS;
}

static GLboolean
add_attrib_binding (slang_attrib_bindings *self, slang_export_data_quant *q, const char *name,
                    GLuint addr, GLuint index_override)
{
	const GLuint n = self->binding_count;
   GLuint slot_span, slot_fill, slot_index;
	GLuint i;

	assert (slang_export_data_quant_simple (q));

	switch (slang_export_data_quant_type (q))
	{
	case GL_FLOAT:
      slot_span = 1;
      slot_fill = 1;
      break;
	case GL_FLOAT_VEC2:
      slot_span = 1;
      slot_fill = 2;
      break;
	case GL_FLOAT_VEC3:
      slot_span = 1;
      slot_fill = 3;
      break;
	case GL_FLOAT_VEC4:
		slot_span = 1;
      slot_fill = 4;
		break;
	case GL_FLOAT_MAT2:
		slot_span = 2;
      slot_fill = 2;
		break;
	case GL_FLOAT_MAT3:
		slot_span = 3;
      slot_fill = 3;
		break;
	case GL_FLOAT_MAT4:
		slot_span = 4;
      slot_fill = 4;
		break;
	default:
		assert (0);
	}

	if (index_override == MAX_VERTEX_ATTRIBS)
		slot_index = allocate_attrib_slots (self, slot_span);
	else if (can_allocate_attrib_slots (self, index_override, slot_span) == slot_span)
		slot_index = index_override;
	else
		slot_index = MAX_VERTEX_ATTRIBS;
			
	if (slot_index == MAX_VERTEX_ATTRIBS)
	{
		/* TODO: info log: error: MAX_VERTEX_ATTRIBS exceeded */
		return GL_FALSE;
	}

	self->bindings[n].quant = q;
	self->bindings[n].name = slang_string_duplicate (name);
	if (self->bindings[n].name == NULL)
		return GL_FALSE;
	self->bindings[n].first_slot_index = slot_index;
	self->binding_count++;

   for (i = 0; i < slot_span; i++) {
      slang_attrib_slot *slot = &self->slots[self->bindings[n].first_slot_index + i];
      slot->addr = addr + i * slot_fill * 4;
      slot->fill = slot_fill;
   }

	return GL_TRUE;
}

static GLboolean gather_attrib_bindings (slang_attrib_bindings *self, slang_export_data_table *tbl,
	slang_attrib_overrides *ovr)
{
	GLuint i;

	/* First pass. Gather attribs that have overriden index slots. */
	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_attribute &&
			!entry_has_gl_prefix (tbl->entries[i].quant.name, tbl->atoms))
		{
			slang_export_data_quant *quant = &tbl->entries[i].quant;
			const GLchar *id = slang_atom_pool_id (tbl->atoms, quant->name);
			GLuint index = lookup_attrib_override (ovr, id);

			if (index != MAX_VERTEX_ATTRIBS)
			{
				if (!add_attrib_binding (self, quant, id, tbl->entries[i].address, index))
					return GL_FALSE;
			}
		}

	/* Second pass. Gather attribs that have *NOT* overriden index slots. */
	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_attribute &&
			!entry_has_gl_prefix (tbl->entries[i].quant.name, tbl->atoms))
		{
			slang_export_data_quant *quant = &tbl->entries[i].quant;
			const GLchar *id = slang_atom_pool_id (tbl->atoms, quant->name);
			GLuint index = lookup_attrib_override (ovr, id);

			if (index == MAX_VERTEX_ATTRIBS)
			{
				if (!add_attrib_binding (self, quant, id, tbl->entries[i].address, index))
					return GL_FALSE;
			}
		}

	return GL_TRUE;
}

/*
 * slang_varying_bindings
 */

static GLvoid slang_varying_bindings_ctr (slang_varying_bindings *self)
{
	self->binding_count = 0;
	self->slot_count = 0;
}

static GLvoid slang_varying_bindings_dtr (slang_varying_bindings *self)
{
	GLuint i;

	for (i = 0; i < self->binding_count; i++)
		slang_alloc_free (self->bindings[i].name);
}

static GLvoid update_varying_slots (slang_varying_slot *slots, GLuint count, GLboolean is_vert,
	GLuint addr, GLuint do_offset)
{
	GLuint i;

	for (i = 0; i < count; i++)
		*(is_vert ? &slots[i].vert_addr : &slots[i].frag_addr) = addr + i * 4 * do_offset;
}

static GLboolean add_varying_binding (slang_varying_bindings *self,
	slang_export_data_quant *q, const char *name, GLboolean is_vert, GLuint addr)
{
	const GLuint n = self->binding_count;
	const GLuint slot_span =
		slang_export_data_quant_components (q) * slang_export_data_quant_elements (q);
	GLuint i;

	for (i = 0; i < n; i++)
		if (slang_string_compare (self->bindings[i].name, name) == 0)
		{
			/* TODO: data quantities must match, or else link fails */
			update_varying_slots (&self->slots[self->bindings[i].first_slot_index], slot_span,
				is_vert, addr, 1);
			return GL_TRUE;
		}

	if (self->slot_count + slot_span > MAX_VARYING_FLOATS)
	{
		/* TODO: info log: error: MAX_VARYING_FLOATS exceeded */
		return GL_FALSE;
	}

	self->bindings[n].quant = q;
	self->bindings[n].name = slang_string_duplicate (name);
	if (self->bindings[n].name == NULL)
		return GL_FALSE;
	self->bindings[n].first_slot_index = self->slot_count;
	self->binding_count++;

	update_varying_slots (&self->slots[self->bindings[n].first_slot_index], slot_span, is_vert,
		addr, 1);
	update_varying_slots (&self->slots[self->bindings[n].first_slot_index], slot_span, !is_vert,
		~0, 0);
	self->slot_count += slot_span;

	return GL_TRUE;
}

static GLboolean gather_varying_bindings (slang_varying_bindings *self,
	slang_export_data_table *tbl, GLboolean is_vert)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
		if (tbl->entries[i].access == slang_exp_varying &&
			!entry_has_gl_prefix (tbl->entries[i].quant.name, tbl->atoms))
		{
			if (!add_varying_binding (self, &tbl->entries[i].quant, slang_atom_pool_id (tbl->atoms,
				tbl->entries[i].quant.name), is_vert, tbl->entries[i].address))
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

	slang_active_variables_ctr (&self->active_uniforms);
	slang_active_variables_ctr (&self->active_attribs);
	slang_attrib_overrides_ctr (&self->attrib_overrides);
	slang_uniform_bindings_ctr (&self->uniforms);
	slang_attrib_bindings_ctr (&self->attribs);
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
	slang_active_variables_dtr (&self->active_uniforms);
	slang_active_variables_dtr (&self->active_attribs);
	slang_attrib_overrides_dtr (&self->attrib_overrides);
	slang_uniform_bindings_dtr (&self->uniforms);
	slang_attrib_bindings_dtr (&self->attribs);
	slang_varying_bindings_dtr (&self->varyings);
	slang_texture_usages_dtr (&self->texture_usage);
}

GLvoid slang_program_rst (slang_program *self)
{
	GLuint i;

	slang_active_variables_dtr (&self->active_uniforms);
	slang_active_variables_dtr (&self->active_attribs);
	slang_uniform_bindings_dtr (&self->uniforms);
	slang_attrib_bindings_dtr (&self->attribs);
	slang_varying_bindings_dtr (&self->varyings);
	slang_texture_usages_dtr (&self->texture_usage);

	slang_active_variables_ctr (&self->active_uniforms);
	slang_active_variables_ctr (&self->active_attribs);
	slang_uniform_bindings_ctr (&self->uniforms);
	slang_attrib_bindings_ctr (&self->attribs);
	slang_varying_bindings_ctr (&self->varyings);
	slang_texture_usages_ctr (&self->texture_usage);
	for (i = 0; i < SLANG_SHADER_MAX; i++)
	{
		GLuint j;

		for (j = 0; j < SLANG_COMMON_FIXED_MAX; j++)
			self->common_fixed_entries[i][j] = ~0;
		for (j = 0; j < SLANG_COMMON_CODE_MAX; j++)
			self->code[i][j] = ~0;
	}
	for (i = 0; i < SLANG_VERTEX_FIXED_MAX; i++)
		self->vertex_fixed_entries[i] = ~0;
	for (i = 0; i < SLANG_FRAGMENT_FIXED_MAX; i++)
		self->fragment_fixed_entries[i] = ~0;
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

GLboolean
_slang_link (slang_program *prog, slang_code_object **objects, GLuint count)
{
	GLuint i;

	for (i = 0; i < count; i++)
	{
		GLuint index;

      if (objects[i]->unit.type == slang_unit_fragment_shader) {
			index = SLANG_SHADER_FRAGMENT;
         resolve_fragment_fixed (prog->fragment_fixed_entries, &objects[i]->expdata);
		}
		else
		{
			index = SLANG_SHADER_VERTEX;
         resolve_vertex_fixed (prog->vertex_fixed_entries, &objects[i]->expdata);
         if (!gather_attrib_bindings (&prog->attribs, &objects[i]->expdata,
                                      &prog->attrib_overrides))
				return GL_FALSE;
		}

      if (!gather_active_variables (&prog->active_uniforms, &objects[i]->expdata, slang_exp_uniform))
			return GL_FALSE;
      if (!gather_active_variables (&prog->active_attribs, &objects[i]->expdata, slang_exp_attribute))
			return GL_FALSE;
      if (!gather_uniform_bindings (&prog->uniforms, &objects[i]->expdata, index))
			return GL_FALSE;
      if (!gather_varying_bindings (&prog->varyings, &objects[i]->expdata,
                                    index == SLANG_SHADER_VERTEX))
			return GL_FALSE;
      resolve_common_fixed (prog->common_fixed_entries[index], &objects[i]->expdata);
      resolve_common_code (prog->code[index], &objects[i]->expcode);
      prog->machines[index] = &objects[i]->machine;
      prog->assemblies[index] = &objects[i]->assembly;
	}

	/* TODO: all varyings read by fragment shader must be written by vertex shader */

	if (!_slang_analyse_texture_usage (prog))
		return GL_FALSE;

	return GL_TRUE;
}

