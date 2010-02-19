/*
 * Copyright 2009 Maciej Cencora <m.cencora@gmail.com>
 *
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

#include "radeon_buffer_objects.h"

#include "main/imports.h"
#include "main/mtypes.h"
#include "main/bufferobj.h"

#include "radeon_common.h"

struct radeon_buffer_object *
get_radeon_buffer_object(struct gl_buffer_object *obj)
{
    return (struct radeon_buffer_object *) obj;
}

static struct gl_buffer_object *
radeonNewBufferObject(GLcontext * ctx,
                      GLuint name,
                      GLenum target)
{
    struct radeon_buffer_object *obj = CALLOC_STRUCT(radeon_buffer_object);

    _mesa_initialize_buffer_object(&obj->Base, name, target);

    obj->bo = NULL;

    return &obj->Base;
}

/**
 * Called via glDeleteBuffersARB().
 */
static void
radeonDeleteBufferObject(GLcontext * ctx,
                         struct gl_buffer_object *obj)
{
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    if (obj->Pointer) {
        radeon_bo_unmap(radeon_obj->bo);
    }

    if (radeon_obj->bo) {
        radeon_bo_unref(radeon_obj->bo);
    }

    free(radeon_obj);
}


/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via ctx->Driver.BufferData().
 * \return GL_TRUE for success, GL_FALSE if out of memory
 */
static GLboolean
radeonBufferData(GLcontext * ctx,
                 GLenum target,
                 GLsizeiptrARB size,
                 const GLvoid * data,
                 GLenum usage,
                 struct gl_buffer_object *obj)
{
    radeonContextPtr radeon = RADEON_CONTEXT(ctx);
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    radeon_obj->Base.Size = size;
    radeon_obj->Base.Usage = usage;

    if (radeon_obj->bo != NULL) {
        radeon_bo_unref(radeon_obj->bo);
        radeon_obj->bo = NULL;
    }

    if (size != 0) {
        radeon_obj->bo = radeon_bo_open(radeon->radeonScreen->bom,
                                        0,
                                        size,
                                        32,
                                        RADEON_GEM_DOMAIN_GTT,
                                        0);

        if (!radeon_obj->bo)
            return GL_FALSE;

        if (data != NULL) {
            radeon_bo_map(radeon_obj->bo, GL_TRUE);

            memcpy(radeon_obj->bo->ptr, data, size);

            radeon_bo_unmap(radeon_obj->bo);
        }
    }
    return GL_TRUE;
}

/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void
radeonBufferSubData(GLcontext * ctx,
                    GLenum target,
                    GLintptrARB offset,
                    GLsizeiptrARB size,
                    const GLvoid * data,
                    struct gl_buffer_object *obj)
{
    radeonContextPtr radeon = RADEON_CONTEXT(ctx);
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    if (radeon_bo_is_referenced_by_cs(radeon_obj->bo, radeon->cmdbuf.cs)) {
        radeon_firevertices(radeon);
    }

    radeon_bo_map(radeon_obj->bo, GL_TRUE);

    memcpy(radeon_obj->bo->ptr + offset, data, size);

    radeon_bo_unmap(radeon_obj->bo);
}

/**
 * Called via glGetBufferSubDataARB()
 */
static void
radeonGetBufferSubData(GLcontext * ctx,
                       GLenum target,
                       GLintptrARB offset,
                       GLsizeiptrARB size,
                       GLvoid * data,
                       struct gl_buffer_object *obj)
{
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    radeon_bo_map(radeon_obj->bo, GL_FALSE);

    memcpy(data, radeon_obj->bo->ptr + offset, size);

    radeon_bo_unmap(radeon_obj->bo);
}

/**
 * Called via glMapBufferARB()
 */
static void *
radeonMapBuffer(GLcontext * ctx,
                GLenum target,
                GLenum access,
                struct gl_buffer_object *obj)
{
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    if (access == GL_WRITE_ONLY_ARB) {
        ctx->Driver.Flush(ctx);
    }

    if (radeon_obj->bo == NULL) {
        obj->Pointer = NULL;
        return NULL;
    }

    radeon_bo_map(radeon_obj->bo, access == GL_WRITE_ONLY_ARB);

    obj->Pointer = radeon_obj->bo->ptr;
    obj->Length = obj->Size;
    obj->Offset = 0;

    return obj->Pointer;
}


/**
 * Called via glUnmapBufferARB()
 */
static GLboolean
radeonUnmapBuffer(GLcontext * ctx,
                  GLenum target,
                  struct gl_buffer_object *obj)
{
    struct radeon_buffer_object *radeon_obj = get_radeon_buffer_object(obj);

    if (radeon_obj->bo != NULL) {
        radeon_bo_unmap(radeon_obj->bo);
    }

    obj->Pointer = NULL;
    obj->Offset = 0;
    obj->Length = 0;

    return GL_TRUE;
}

void
radeonInitBufferObjectFuncs(struct dd_function_table *functions)
{
    functions->NewBufferObject = radeonNewBufferObject;
    functions->DeleteBuffer = radeonDeleteBufferObject;
    functions->BufferData = radeonBufferData;
    functions->BufferSubData = radeonBufferSubData;
    functions->GetBufferSubData = radeonGetBufferSubData;
    functions->MapBuffer = radeonMapBuffer;
    functions->UnmapBuffer = radeonUnmapBuffer;
}
