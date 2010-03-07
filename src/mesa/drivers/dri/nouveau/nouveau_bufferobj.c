/*
 * Copyright (C) 2009 Francisco Jerez.
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

#include "nouveau_driver.h"
#include "nouveau_bufferobj.h"
#include "nouveau_context.h"

#include "main/bufferobj.h"

static struct gl_buffer_object *
nouveau_bufferobj_new(GLcontext *ctx, GLuint buffer, GLenum target)
{
	struct nouveau_bufferobj *nbo;

	nbo = CALLOC_STRUCT(nouveau_bufferobj);
	if (!nbo)
		return NULL;

	_mesa_initialize_buffer_object(&nbo->base, buffer, target);

	return &nbo->base;
}

static void
nouveau_bufferobj_del(GLcontext *ctx, struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);

	nouveau_bo_ref(NULL, &nbo->bo);
	FREE(nbo);
}

static GLboolean
nouveau_bufferobj_data(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		       const GLvoid *data, GLenum usage,
		       struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);
	int ret;

	obj->Size = size;
	obj->Usage = usage;

	nouveau_bo_ref(NULL, &nbo->bo);
	ret = nouveau_bo_new(context_dev(ctx),
			     NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
			     size, &nbo->bo);
	assert(!ret);

	if (data) {
		nouveau_bo_map(nbo->bo, NOUVEAU_BO_WR);
		memcpy(nbo->bo->map, data, size);
		nouveau_bo_unmap(nbo->bo);
	}

	return GL_TRUE;
}

static void
nouveau_bufferobj_subdata(GLcontext *ctx, GLenum target, GLintptrARB offset,
			  GLsizeiptrARB size, const GLvoid *data,
			  struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);

	nouveau_bo_map(nbo->bo, NOUVEAU_BO_WR);
	memcpy(nbo->bo->map + offset, data, size);
	nouveau_bo_unmap(nbo->bo);
}

static void
nouveau_bufferobj_get_subdata(GLcontext *ctx, GLenum target, GLintptrARB offset,
			   GLsizeiptrARB size, GLvoid *data,
			   struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);

	nouveau_bo_map(nbo->bo, NOUVEAU_BO_RD);
	memcpy(data, nbo->bo->map + offset, size);
	nouveau_bo_unmap(nbo->bo);
}

static void *
nouveau_bufferobj_map(GLcontext *ctx, GLenum target, GLenum access,
		   struct gl_buffer_object *obj)
{
	return ctx->Driver.MapBufferRange(ctx, target, 0, obj->Size, access,
					  obj);
}

static void *
nouveau_bufferobj_map_range(GLcontext *ctx, GLenum target, GLintptr offset,
			    GLsizeiptr length, GLenum access,
			    struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);
	uint32_t flags = 0;

	assert(!obj->Pointer);

	if (!nbo->bo)
		return NULL;

	if (access == GL_READ_ONLY_ARB ||
	    access == GL_READ_WRITE_ARB)
		flags |= NOUVEAU_BO_RD;
	if (access == GL_WRITE_ONLY_ARB ||
	    access == GL_READ_WRITE_ARB)
		flags |= NOUVEAU_BO_WR;

	nouveau_bo_map_range(nbo->bo, offset, length, flags);

	obj->Pointer = nbo->bo->map;
	obj->Offset = offset;
	obj->Length = length;
	obj->AccessFlags = access;

	return obj->Pointer;
}

static GLboolean
nouveau_bufferobj_unmap(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)
{
	struct nouveau_bufferobj *nbo = to_nouveau_bufferobj(obj);

	assert(obj->Pointer);

	nouveau_bo_unmap(nbo->bo);

	obj->Pointer = NULL;
	obj->Offset = 0;
	obj->Length = 0;
	obj->AccessFlags = 0;

	return GL_TRUE;
}

void
nouveau_bufferobj_functions_init(struct dd_function_table *functions)
{
	functions->NewBufferObject = nouveau_bufferobj_new;
	functions->DeleteBuffer	= nouveau_bufferobj_del;
	functions->BufferData = nouveau_bufferobj_data;
	functions->BufferSubData = nouveau_bufferobj_subdata;
	functions->GetBufferSubData = nouveau_bufferobj_get_subdata;
	functions->MapBuffer = nouveau_bufferobj_map;
	functions->MapBufferRange = nouveau_bufferobj_map_range;
	functions->UnmapBuffer = nouveau_bufferobj_unmap;
}
