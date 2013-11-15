/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */



#ifndef BUFFEROBJ_H
#define BUFFEROBJ_H

#include <stdbool.h>
#include "mtypes.h"


/*
 * Internal functions
 */


/** Is the given buffer object currently mapped by the GL user? */
static inline GLboolean
_mesa_bufferobj_mapped(const struct gl_buffer_object *obj,
                       gl_map_buffer_index index)
{
   return obj->Mappings[index].Pointer != NULL;
}

/** Can we not use this buffer while mapped? */
static inline GLboolean
_mesa_check_disallowed_mapping(const struct gl_buffer_object *obj)
{
   return _mesa_bufferobj_mapped(obj, MAP_USER) &&
          !(obj->Mappings[MAP_USER].AccessFlags &
            GL_MAP_PERSISTENT_BIT);
}

/**
 * Is the given buffer object a user-created buffer object?
 * Mesa uses default buffer objects in several places.  Default buffers
 * always have Name==0.  User created buffers have Name!=0.
 */
static inline GLboolean
_mesa_is_bufferobj(const struct gl_buffer_object *obj)
{
   return obj != NULL && obj->Name != 0;
}


extern void
_mesa_init_buffer_objects(struct gl_context *ctx);

extern void
_mesa_free_buffer_objects(struct gl_context *ctx);

extern bool
_mesa_handle_bind_buffer_gen(struct gl_context *ctx,
                             GLenum target,
                             GLuint buffer,
                             struct gl_buffer_object **buf_handle,
                             const char *caller);

extern void
_mesa_update_default_objects_buffer_objects(struct gl_context *ctx);


extern struct gl_buffer_object *
_mesa_lookup_bufferobj(struct gl_context *ctx, GLuint buffer);

extern struct gl_buffer_object *
_mesa_lookup_bufferobj_locked(struct gl_context *ctx, GLuint buffer);

extern void
_mesa_begin_bufferobj_lookups(struct gl_context *ctx);

extern void
_mesa_end_bufferobj_lookups(struct gl_context *ctx);

extern struct gl_buffer_object *
_mesa_multi_bind_lookup_bufferobj(struct gl_context *ctx,
                                  const GLuint *buffers,
                                  GLuint index, const char *caller);

extern void
_mesa_initialize_buffer_object(struct gl_context *ctx,
                               struct gl_buffer_object *obj,
                               GLuint name, GLenum target);

extern void
_mesa_reference_buffer_object_(struct gl_context *ctx,
                               struct gl_buffer_object **ptr,
                               struct gl_buffer_object *bufObj);

static inline void
_mesa_reference_buffer_object(struct gl_context *ctx,
                              struct gl_buffer_object **ptr,
                              struct gl_buffer_object *bufObj)
{
   if (*ptr != bufObj)
      _mesa_reference_buffer_object_(ctx, ptr, bufObj);
}

extern GLuint
_mesa_total_buffer_object_memory(struct gl_context *ctx);

extern void
_mesa_init_buffer_object_functions(struct dd_function_table *driver);

extern void
_mesa_buffer_unmap_all_mappings(struct gl_context *ctx,
                                struct gl_buffer_object *bufObj);

extern void
_mesa_buffer_clear_subdata(struct gl_context *ctx,
                           GLintptr offset, GLsizeiptr size,
                           const GLvoid *clearValue,
                           GLsizeiptr clearValueSize,
                           struct gl_buffer_object *bufObj);

/*
 * API functions
 */
void GLAPIENTRY
_mesa_BindBuffer(GLenum target, GLuint buffer);

void GLAPIENTRY
_mesa_DeleteBuffers(GLsizei n, const GLuint * buffer);

void GLAPIENTRY
_mesa_GenBuffers(GLsizei n, GLuint * buffer);

GLboolean GLAPIENTRY
_mesa_IsBuffer(GLuint buffer);

void GLAPIENTRY
_mesa_BufferStorage(GLenum target, GLsizeiptr size, const GLvoid *data,
                    GLbitfield flags);

void GLAPIENTRY
_mesa_BufferData(GLenum target, GLsizeiptrARB size,
                 const GLvoid * data, GLenum usage);

void GLAPIENTRY
_mesa_BufferSubData(GLenum target, GLintptrARB offset,
                    GLsizeiptrARB size, const GLvoid * data);

void GLAPIENTRY
_mesa_GetBufferSubData(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, void * data);

void GLAPIENTRY
_mesa_ClearBufferData(GLenum target, GLenum internalformat,
                      GLenum format, GLenum type,
                      const GLvoid * data);

void GLAPIENTRY
_mesa_ClearBufferSubData(GLenum target, GLenum internalformat,
                         GLintptr offset, GLsizeiptr size,
                         GLenum format, GLenum type,
                         const GLvoid * data);

void * GLAPIENTRY
_mesa_MapBuffer(GLenum target, GLenum access);

GLboolean GLAPIENTRY
_mesa_UnmapBuffer(GLenum target);

void GLAPIENTRY
_mesa_GetBufferParameteriv(GLenum target, GLenum pname, GLint *params);

void GLAPIENTRY
_mesa_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params);

void GLAPIENTRY
_mesa_GetBufferPointerv(GLenum target, GLenum pname, GLvoid **params);

void GLAPIENTRY
_mesa_CopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                        GLintptr readOffset, GLintptr writeOffset,
                        GLsizeiptr size);

void * GLAPIENTRY
_mesa_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                     GLbitfield access);

void GLAPIENTRY
_mesa_FlushMappedBufferRange(GLenum target,
                             GLintptr offset, GLsizeiptr length);

GLenum GLAPIENTRY
_mesa_ObjectPurgeableAPPLE(GLenum objectType, GLuint name, GLenum option);

GLenum GLAPIENTRY
_mesa_ObjectUnpurgeableAPPLE(GLenum objectType, GLuint name, GLenum option);

void GLAPIENTRY
_mesa_GetObjectParameterivAPPLE(GLenum objectType, GLuint name,
                                GLenum pname, GLint* params);

void GLAPIENTRY
_mesa_BindBufferRange(GLenum target, GLuint index,
                      GLuint buffer, GLintptr offset, GLsizeiptr size);

void GLAPIENTRY
_mesa_BindBufferBase(GLenum target, GLuint index, GLuint buffer);

void GLAPIENTRY
_mesa_BindBuffersRange(GLenum target, GLuint first, GLsizei count,
                       const GLuint *buffers,
                       const GLintptr *offsets, const GLsizeiptr *sizes);
void GLAPIENTRY
_mesa_BindBuffersBase(GLenum target, GLuint first, GLsizei count,
                      const GLuint *buffers);
void GLAPIENTRY
_mesa_InvalidateBufferSubData(GLuint buffer, GLintptr offset,
                              GLsizeiptr length);

void GLAPIENTRY
_mesa_InvalidateBufferData(GLuint buffer);


#endif
