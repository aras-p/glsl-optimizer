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
#include "slang_utility.h"
#include "slang_compile.h"
#include "slang_export.h"
#include "slang_link.h"

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
	for (i = 0; i < SLANG_UNIFORM_BINDING_MAX; i++)
		self->table[n].address[i] = ~0;
	self->table[n].address[index] = address;
	if (self->table[n].name == NULL)
		return GL_FALSE;
	self->count++;
	return GL_TRUE;
}

static GLboolean insert_binding (slang_uniform_bindings *bind, slang_export_data_quant *q,
	char *name, slang_atom_pool *atoms, GLuint index, GLuint addr)
{
	GLuint count, i;

	slang_string_concat (name, slang_atom_pool_id (atoms, q->name));

	if (q->array_len == 0)
		count = 1;
	else
		count = q->array_len;

	for (i = 0; i < count; i++)
	{
		GLuint save;

		save = slang_string_length (name);
		if (q->array_len != 0)
			_mesa_sprintf (name + slang_string_length (name), "[%d]", i);

		if (q->structure != NULL)
		{
			GLuint save, i;

			slang_string_concat (name, ".");
			save = slang_string_length (name);

			for (i = 0; i < q->u.field_count; i++)
			{
				if (!insert_binding (bind, &q->structure[i], name, atoms, index, addr))
					return GL_FALSE;
				name[save] = '\0';
				addr += q->structure[i].size;
			}
		}
		else
		{
			if (!slang_uniform_bindings_add (bind, q, name, index, addr))
				return GL_FALSE;
			addr += q->size;
		}
		name[save] = '\0';
	}

	return GL_TRUE;
}

static GLboolean gather_uniform_bindings (slang_uniform_bindings *bind, slang_export_data_table *tbl,
	GLuint index)
{
	GLuint i;

	for (i = 0; i < tbl->count; i++)
	{
		char name[1024] = "";

		if (!insert_binding (bind, &tbl->entries[i].quant, name, tbl->atoms, index,
				tbl->entries[i].address))
			return GL_FALSE;
	}

	return GL_TRUE;
}

/*
 * slang_program
 */

GLvoid slang_program_ctr (slang_program *self)
{
	slang_uniform_bindings_ctr (&self->uniforms);
}

GLvoid slang_program_dtr (slang_program *self)
{
	slang_uniform_bindings_dtr (&self->uniforms);
}

/*
 * _slang_link()
 */

GLboolean _slang_link (slang_program *prog, slang_translation_unit **units, GLuint count)
{
	GLuint i;

	for (i = 0; i < count; i++)
	{
		GLuint index;

		if (units[i]->type == slang_unit_fragment_shader)
			index = SLANG_UNIFORM_BINDING_FRAGMENT;
		else
			index = SLANG_UNIFORM_BINDING_VERTEX;

		if (!gather_uniform_bindings (&prog->uniforms, &units[i]->exp_data, index))
			return GL_FALSE;
		prog->machines[index] = units[i]->machine;
	}

	return GL_TRUE;
}

