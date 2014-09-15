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


#ifndef VARRAY_H
#define VARRAY_H


#include "glheader.h"
#include "bufferobj.h"

struct gl_client_array;
struct gl_context;

/**
 * Returns a pointer to the vertex attribute data in a client array,
 * or the offset into the vertex buffer for an array that resides in
 * a vertex buffer.
 */
static inline const GLubyte *
_mesa_vertex_attrib_address(const struct gl_vertex_attrib_array *array,
                            const struct gl_vertex_buffer_binding *binding)
{
   if (_mesa_is_bufferobj(binding->BufferObj))
      return (const GLubyte *) (binding->Offset + array->RelativeOffset);
   else
      return array->Ptr;       
}

/**
 * Sets the fields in a gl_client_array to values derived from a
 * gl_vertex_attrib_array and a gl_vertex_buffer_binding.
 */
static inline void
_mesa_update_client_array(struct gl_context *ctx,
                          struct gl_client_array *dst,
                          const struct gl_vertex_attrib_array *src,
                          const struct gl_vertex_buffer_binding *binding)
{
   dst->Size = src->Size;
   dst->Type = src->Type;
   dst->Format = src->Format;
   dst->Stride = src->Stride;
   dst->StrideB = binding->Stride;
   dst->Ptr = _mesa_vertex_attrib_address(src, binding);
   dst->Enabled = src->Enabled;
   dst->Normalized = src->Normalized;
   dst->Integer = src->Integer;
   dst->InstanceDivisor = binding->InstanceDivisor;
   dst->_ElementSize = src->_ElementSize;
   _mesa_reference_buffer_object(ctx, &dst->BufferObj, binding->BufferObj);
}

static inline bool
_mesa_attr_zero_aliases_vertex(struct gl_context *ctx)
{
   const bool is_forward_compatible_context =
      ctx->Const.ContextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT;

   /* In OpenGL 3.1 attribute 0 becomes non-magic, just like in OpenGL ES
    * 2.0.  Note that we cannot just check for API_OPENGL_COMPAT here because
    * that will erroneously allow this usage in a 3.0 forward-compatible
    * context too.
    */
   return (ctx->API == API_OPENGLES
           || (ctx->API == API_OPENGL_COMPAT
               && !is_forward_compatible_context));
}

extern void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride,
                    const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr);


extern void GLAPIENTRY
_mesa_FogCoordPointer(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_SecondaryColorPointer(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_PointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_VertexAttribPointer(GLuint index, GLint size, GLenum type,
                             GLboolean normalized, GLsizei stride,
                             const GLvoid *pointer);

void GLAPIENTRY
_mesa_VertexAttribIPointer(GLuint index, GLint size, GLenum type,
                           GLsizei stride, const GLvoid *ptr);


extern void GLAPIENTRY
_mesa_EnableVertexAttribArray(GLuint index);


extern void GLAPIENTRY
_mesa_DisableVertexAttribArray(GLuint index);


extern void GLAPIENTRY
_mesa_GetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribiv(GLuint index, GLenum pname, GLint *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribIiv(GLuint index, GLenum pname, GLint *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params);


extern void GLAPIENTRY
_mesa_GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid **pointer);


extern void GLAPIENTRY
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);


extern void GLAPIENTRY
_mesa_MultiDrawArrays( GLenum mode, const GLint *first,
                          const GLsizei *count, GLsizei primcount );

extern void GLAPIENTRY
_mesa_MultiDrawElementsEXT( GLenum mode, const GLsizei *count, GLenum type,
                            const GLvoid **indices, GLsizei primcount );

extern void GLAPIENTRY
_mesa_MultiDrawElementsBaseVertex( GLenum mode,
				   const GLsizei *count, GLenum type,
				   const GLvoid **indices, GLsizei primcount,
				   const GLint *basevertex);

extern void GLAPIENTRY
_mesa_MultiModeDrawArraysIBM( const GLenum * mode, const GLint * first,
			      const GLsizei * count,
			      GLsizei primcount, GLint modestride );


extern void GLAPIENTRY
_mesa_MultiModeDrawElementsIBM( const GLenum * mode, const GLsizei * count,
				GLenum type, const GLvoid * const * indices,
				GLsizei primcount, GLint modestride );

extern void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count);

extern void GLAPIENTRY
_mesa_UnlockArraysEXT( void );


extern void GLAPIENTRY
_mesa_DrawArrays(GLenum mode, GLint first, GLsizei count);

extern void GLAPIENTRY
_mesa_DrawArraysInstanced(GLenum mode, GLint first, GLsizei count,
                          GLsizei primcount);

extern void GLAPIENTRY
_mesa_DrawElements(GLenum mode, GLsizei count, GLenum type,
                   const GLvoid *indices);

extern void GLAPIENTRY
_mesa_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                        GLenum type, const GLvoid *indices);

extern void GLAPIENTRY
_mesa_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
			     const GLvoid *indices, GLint basevertex);

extern void GLAPIENTRY
_mesa_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
				  GLsizei count, GLenum type,
				  const GLvoid *indices,
				  GLint basevertex);

extern void GLAPIENTRY
_mesa_DrawTransformFeedback(GLenum mode, GLuint name);

extern void GLAPIENTRY
_mesa_PrimitiveRestartIndex(GLuint index);


extern void GLAPIENTRY
_mesa_VertexAttribDivisor(GLuint index, GLuint divisor);

extern unsigned
_mesa_primitive_restart_index(const struct gl_context *ctx, GLenum ib_type);

extern void GLAPIENTRY
_mesa_BindVertexBuffer(GLuint bindingIndex, GLuint buffer, GLintptr offset,
                       GLsizei stride);

extern void GLAPIENTRY
_mesa_BindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers,
                        const GLintptr *offsets, const GLsizei *strides);

extern void GLAPIENTRY
_mesa_VertexAttribFormat(GLuint attribIndex, GLint size, GLenum type,
                         GLboolean normalized, GLuint relativeOffset);

extern void GLAPIENTRY
_mesa_VertexAttribIFormat(GLuint attribIndex, GLint size, GLenum type,
                          GLuint relativeOffset);

extern void GLAPIENTRY
_mesa_VertexAttribLFormat(GLuint attribIndex, GLint size, GLenum type,
                          GLuint relativeOffset);

extern void GLAPIENTRY
_mesa_VertexAttribBinding(GLuint attribIndex, GLuint bindingIndex);

extern void GLAPIENTRY
_mesa_VertexBindingDivisor(GLuint bindingIndex, GLuint divisor);


extern void
_mesa_copy_client_array(struct gl_context *ctx,
                        struct gl_client_array *dst,
                        struct gl_client_array *src);

extern void
_mesa_copy_vertex_attrib_array(struct gl_context *ctx,
                               struct gl_vertex_attrib_array *dst,
                               const struct gl_vertex_attrib_array *src);

extern void
_mesa_copy_vertex_buffer_binding(struct gl_context *ctx,
                                 struct gl_vertex_buffer_binding *dst,
                                 const struct gl_vertex_buffer_binding *src);

extern void
_mesa_print_arrays(struct gl_context *ctx);

extern void
_mesa_init_varray( struct gl_context * ctx );

extern void 
_mesa_free_varray_data(struct gl_context *ctx);

#endif
