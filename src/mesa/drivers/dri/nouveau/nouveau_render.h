/*
 * Copyright (C) 2009-2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NOUVEAU_RENDER_H__
#define __NOUVEAU_RENDER_H__

#include "vbo/vbo_context.h"

struct nouveau_array_state;

typedef void (*dispatch_t)(GLcontext *, unsigned int, int, unsigned int);
typedef unsigned (*extract_u_t)(struct nouveau_array_state *a, int i, int j);
typedef float (*extract_f_t)(struct nouveau_array_state *a, int i, int j);

struct nouveau_attr_info {
	int vbo_index;
	int imm_method;
	int imm_fields;

	void (*emit)(GLcontext *, struct nouveau_array_state *, const void *);
};

struct nouveau_array_state {
	int attr;
	int stride, fields, type;

	struct nouveau_bo *bo;
	unsigned offset;
	const void *buf;

	extract_u_t extract_u;
	extract_f_t extract_f;
};

#define RENDER_SCRATCH_COUNT 32
#define RENDER_SCRATCH_SIZE 64*1024

struct nouveau_scratch_state {
	struct nouveau_bo *bo[RENDER_SCRATCH_COUNT];

	int index;
	int offset;
	void *buf;
};

struct nouveau_swtnl_state {
	struct nouveau_bo *vbo;
	void *buf;
	unsigned vertex_count;
	GLenum primitive;
};

struct nouveau_render_state {
	enum {
		VBO,
		IMM
	} mode;

	struct nouveau_array_state ib;
	struct nouveau_array_state attrs[VERT_ATTRIB_MAX];

	/* Maps a HW VBO index or IMM emission order to an index in
	 * the attrs array above (or -1 if unused). */
	int map[VERT_ATTRIB_MAX];

	int attr_count;
	int vertex_size;

	struct nouveau_scratch_state scratch;
	struct nouveau_swtnl_state swtnl;
};

#define to_render_state(ctx) (&to_nouveau_context(ctx)->render)

#endif
