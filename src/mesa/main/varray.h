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

struct gl_client_array;
struct gl_context;


/**
 * Compute the index of the last array element that can be safely accessed in
 * a vertex array.  We can really only do this when the array lives in a VBO.
 * The array->_MaxElement field will be updated.
 * Later in glDrawArrays/Elements/etc we can do some bounds checking.
 */
static inline void
_mesa_update_array_max_element(struct gl_client_array *array)
{
   assert(array->Enabled);

   if (array->BufferObj->Name) {
      GLsizeiptrARB offset = (GLsizeiptrARB) array->Ptr;
      GLsizeiptrARB bufSize = (GLsizeiptrARB) array->BufferObj->Size;

      if (offset < bufSize) {
	 array->_MaxElement = (bufSize - offset + array->StrideB
                               - array->_ElementSize) / array->StrideB;
      }
      else {
	 array->_MaxElement = 0;
      }
   }
   else {
      /* user-space array, no idea how big it is */
      array->_MaxElement = 2 * 1000 * 1000 * 1000; /* just a big number */
   }
}


extern void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride,
                    const GLvoid *ptr);

extern void GLAPIENTRY
_mesa_UnlockArraysEXT( void );

extern void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count);


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

extern void
_mesa_copy_client_array(struct gl_context *ctx,
                        struct gl_client_array *dst,
                        struct gl_client_array *src);


extern void
_mesa_print_arrays(struct gl_context *ctx);

extern void
_mesa_init_varray( struct gl_context * ctx );

extern void 
_mesa_free_varray_data(struct gl_context *ctx);

#endif
