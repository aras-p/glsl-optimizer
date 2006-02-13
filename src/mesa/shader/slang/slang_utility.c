/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
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
 * \file slang_utility.c
 * slang utilities
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_utility.h"

char *slang_string_concat (char *dst, const char *src)
{
	return _mesa_strcpy (dst + _mesa_strlen (dst), src);
}

/* slang_atom_pool */

void slang_atom_pool_construct (slang_atom_pool *pool)
{
	GLuint i;

	for (i = 0; i < SLANG_ATOM_POOL_SIZE; i++)
		pool->entries[i] = NULL;
}

void slang_atom_pool_destruct (slang_atom_pool *pool)
{
	GLuint i;

	for (i = 0; i < SLANG_ATOM_POOL_SIZE; i++)
	{
		slang_atom_entry *entry;
		
		entry = pool->entries[i];
		while (entry != NULL)
		{
			slang_atom_entry *next;

			next = entry->next;
			slang_alloc_free (entry->id);
			slang_alloc_free (entry);
			entry = next;
		}
	}
}

slang_atom slang_atom_pool_atom (slang_atom_pool *pool, const char *id)
{
	GLuint hash;
	const char *p = id;
	slang_atom_entry **entry;

	hash = 0;
	while (*p != '\0')
	{
		GLuint g;

		hash = (hash << 4) + (GLuint) *p++;
		g = hash & 0xf0000000;
		if (g != 0)
			hash ^= g >> 24;
		hash &= ~g;
	}
	hash %= SLANG_ATOM_POOL_SIZE;

	entry = &pool->entries[hash];
	while (*entry != NULL)
	{
		if (slang_string_compare ((**entry).id, id) == 0)
			return (slang_atom) (**entry).id;
		entry = &(**entry).next;
	}

	*entry = (slang_atom_entry *) slang_alloc_malloc (sizeof (slang_atom_entry));
	if (*entry == NULL)
		return SLANG_ATOM_NULL;

	(**entry).next = NULL;
	(**entry).id = slang_string_duplicate (id);
	if ((**entry).id == NULL)
		return SLANG_ATOM_NULL;
	return (slang_atom) (**entry).id;
}

const char *slang_atom_pool_id (slang_atom_pool *pool, slang_atom atom)
{
	return (const char *) atom;
}

