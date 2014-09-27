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


#include <inttypes.h>  /* for PRId64 macro */

#include "glheader.h"
#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "enable.h"
#include "enums.h"
#include "hash.h"
#include "image.h"
#include "macros.h"
#include "mtypes.h"
#include "varray.h"
#include "arrayobj.h"
#include "main/dispatch.h"


/** Used to do error checking for GL_EXT_vertex_array_bgra */
#define BGRA_OR_4  5


/** Used to indicate which GL datatypes are accepted by each of the
 * glVertex/Color/Attrib/EtcPointer() functions.
 */
#define BOOL_BIT                          (1 << 0)
#define BYTE_BIT                          (1 << 1)
#define UNSIGNED_BYTE_BIT                 (1 << 2)
#define SHORT_BIT                         (1 << 3)
#define UNSIGNED_SHORT_BIT                (1 << 4)
#define INT_BIT                           (1 << 5)
#define UNSIGNED_INT_BIT                  (1 << 6)
#define HALF_BIT                          (1 << 7)
#define FLOAT_BIT                         (1 << 8)
#define DOUBLE_BIT                        (1 << 9)
#define FIXED_ES_BIT                      (1 << 10)
#define FIXED_GL_BIT                      (1 << 11)
#define UNSIGNED_INT_2_10_10_10_REV_BIT   (1 << 12)
#define INT_2_10_10_10_REV_BIT            (1 << 13)
#define UNSIGNED_INT_10F_11F_11F_REV_BIT  (1 << 14)
#define ALL_TYPE_BITS                    ((1 << 15) - 1)


/** Convert GL datatype enum into a <type>_BIT value seen above */
static GLbitfield
type_to_bit(const struct gl_context *ctx, GLenum type)
{
   switch (type) {
   case GL_BOOL:
      return BOOL_BIT;
   case GL_BYTE:
      return BYTE_BIT;
   case GL_UNSIGNED_BYTE:
      return UNSIGNED_BYTE_BIT;
   case GL_SHORT:
      return SHORT_BIT;
   case GL_UNSIGNED_SHORT:
      return UNSIGNED_SHORT_BIT;
   case GL_INT:
      return INT_BIT;
   case GL_UNSIGNED_INT:
      return UNSIGNED_INT_BIT;
   case GL_HALF_FLOAT:
      if (ctx->Extensions.ARB_half_float_vertex)
         return HALF_BIT;
      else
         return 0x0;
   case GL_FLOAT:
      return FLOAT_BIT;
   case GL_DOUBLE:
      return DOUBLE_BIT;
   case GL_FIXED:
      return _mesa_is_desktop_gl(ctx) ? FIXED_GL_BIT : FIXED_ES_BIT;
   case GL_UNSIGNED_INT_2_10_10_10_REV:
      return UNSIGNED_INT_2_10_10_10_REV_BIT;
   case GL_INT_2_10_10_10_REV:
      return INT_2_10_10_10_REV_BIT;
   case GL_UNSIGNED_INT_10F_11F_11F_REV:
      return UNSIGNED_INT_10F_11F_11F_REV_BIT;
   default:
      return 0;
   }
}


/**
 * Sets the VertexBinding field in the vertex attribute given by attribIndex.
 */
static void
vertex_attrib_binding(struct gl_context *ctx, GLuint attribIndex,
                      GLuint bindingIndex)
{
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct gl_vertex_attrib_array *array = &vao->VertexAttrib[attribIndex];

   if (array->VertexBinding != bindingIndex) {
      const GLbitfield64 array_bit = VERT_BIT(attribIndex);

      FLUSH_VERTICES(ctx, _NEW_ARRAY);

      vao->VertexBinding[array->VertexBinding]._BoundArrays &= ~array_bit;
      vao->VertexBinding[bindingIndex]._BoundArrays |= array_bit;

      array->VertexBinding = bindingIndex;

      vao->NewArrays |= array_bit;
   }
}


/**
 * Binds a buffer object to the vertex buffer binding point given by index,
 * and sets the Offset and Stride fields.
 */
static void
bind_vertex_buffer(struct gl_context *ctx, GLuint index,
                   struct gl_buffer_object *vbo,
                   GLintptr offset, GLsizei stride)
{
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct gl_vertex_buffer_binding *binding = &vao->VertexBinding[index];

   if (binding->BufferObj != vbo ||
       binding->Offset != offset ||
       binding->Stride != stride) {

      FLUSH_VERTICES(ctx, _NEW_ARRAY);

      _mesa_reference_buffer_object(ctx, &binding->BufferObj, vbo);

      binding->Offset = offset;
      binding->Stride = stride;

      vao->NewArrays |= binding->_BoundArrays;
   }
}


/**
 * Sets the InstanceDivisor field in the vertex buffer binding point
 * given by bindingIndex.
 */
static void
vertex_binding_divisor(struct gl_context *ctx, GLuint bindingIndex,
                       GLuint divisor)
{
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct gl_vertex_buffer_binding *binding =
      &vao->VertexBinding[bindingIndex];

   if (binding->InstanceDivisor != divisor) {
      FLUSH_VERTICES(ctx, _NEW_ARRAY);
      binding->InstanceDivisor = divisor;
      vao->NewArrays |= binding->_BoundArrays;
   }
}


/**
 * Examine the API profile and extensions to determine which types are legal
 * for vertex arrays.  This is called once from update_array_format().
 */
static GLbitfield
get_legal_types_mask(const struct gl_context *ctx)
{
   GLbitfield legalTypesMask = ALL_TYPE_BITS;

   if (_mesa_is_gles(ctx)) {
      legalTypesMask &= ~(FIXED_GL_BIT |
                          DOUBLE_BIT |
                          UNSIGNED_INT_10F_11F_11F_REV_BIT);

      /* GL_INT and GL_UNSIGNED_INT data is not allowed in OpenGL ES until
       * 3.0.  The 2_10_10_10 types are added in OpenGL ES 3.0 or
       * GL_OES_vertex_type_10_10_10_2.  GL_HALF_FLOAT data is not allowed
       * until 3.0 or with the GL_OES_vertex_half float extension, which isn't
       * quite as trivial as we'd like because it uses a different enum value
       * for GL_HALF_FLOAT_OES.
       */
      if (ctx->Version < 30) {
         legalTypesMask &= ~(UNSIGNED_INT_BIT |
                             INT_BIT |
                             UNSIGNED_INT_2_10_10_10_REV_BIT |
                             INT_2_10_10_10_REV_BIT |
                             HALF_BIT);
      }
   }
   else {
      legalTypesMask &= ~FIXED_ES_BIT;

      if (!ctx->Extensions.ARB_ES2_compatibility)
         legalTypesMask &= ~FIXED_GL_BIT;

      if (!ctx->Extensions.ARB_vertex_type_2_10_10_10_rev)
         legalTypesMask &= ~(UNSIGNED_INT_2_10_10_10_REV_BIT |
                             INT_2_10_10_10_REV_BIT);

      if (!ctx->Extensions.ARB_vertex_type_10f_11f_11f_rev)
         legalTypesMask &= ~UNSIGNED_INT_10F_11F_11F_REV_BIT;
   }

   return legalTypesMask;
}


/**
 * Does error checking and updates the format in an attrib array.
 *
 * Called by update_array() and VertexAttrib*Format().
 *
 * \param func         Name of calling function used for error reporting
 * \param attrib       The index of the attribute array
 * \param legalTypes   Bitmask of *_BIT above indicating legal datatypes
 * \param sizeMin      Min allowable size value
 * \param sizeMax      Max allowable size value (may also be BGRA_OR_4)
 * \param size         Components per element (1, 2, 3 or 4)
 * \param type         Datatype of each component (GL_FLOAT, GL_INT, etc)
 * \param normalized   Whether integer types are converted to floats in [-1, 1]
 * \param integer      Integer-valued values (will not be normalized to [-1, 1])
 * \param relativeOffset Offset of the first element relative to the binding offset.
 */
static bool
update_array_format(struct gl_context *ctx,
                    const char *func,
                    GLuint attrib, GLbitfield legalTypesMask,
                    GLint sizeMin, GLint sizeMax,
                    GLint size, GLenum type,
                    GLboolean normalized, GLboolean integer,
                    GLuint relativeOffset)
{
   struct gl_vertex_attrib_array *array;
   GLbitfield typeBit;
   GLuint elementSize;
   GLenum format = GL_RGBA;

   if (ctx->Array.LegalTypesMask == 0) {
      /* One-time initialization.  We can't do this in _mesa_init_varrays()
       * below because extensions are not yet enabled at that point.
       */
      ctx->Array.LegalTypesMask = get_legal_types_mask(ctx);
   }

   legalTypesMask &= ctx->Array.LegalTypesMask;

   if (_mesa_is_gles(ctx) && sizeMax == BGRA_OR_4) {
      /* BGRA ordering is not supported in ES contexts.
       */
      sizeMax = 4;
   }

   typeBit = type_to_bit(ctx, type);
   if (typeBit == 0x0 || (typeBit & legalTypesMask) == 0x0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(type = %s)",
                  func, _mesa_lookup_enum_by_nr(type));
      return false;
   }

   /* Do size parameter checking.
    * If sizeMax = BGRA_OR_4 it means that size = GL_BGRA is legal and
    * must be handled specially.
    */
   if (ctx->Extensions.EXT_vertex_array_bgra &&
       sizeMax == BGRA_OR_4 &&
       size == GL_BGRA) {
      /* Page 298 of the PDF of the OpenGL 4.3 (Core Profile) spec says:
       *
       * "An INVALID_OPERATION error is generated under any of the following
       *  conditions:
       *    ...
       *    • size is BGRA and type is not UNSIGNED_BYTE, INT_2_10_10_10_REV
       *      or UNSIGNED_INT_2_10_10_10_REV;
       *    ...
       *    • size is BGRA and normalized is FALSE;"
       */
      bool bgra_error = false;

      if (ctx->Extensions.ARB_vertex_type_2_10_10_10_rev) {
         if (type != GL_UNSIGNED_INT_2_10_10_10_REV &&
             type != GL_INT_2_10_10_10_REV &&
             type != GL_UNSIGNED_BYTE)
            bgra_error = true;
      } else if (type != GL_UNSIGNED_BYTE)
         bgra_error = true;

      if (bgra_error) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s(size=GL_BGRA and type=%s)",
                     func, _mesa_lookup_enum_by_nr(type));
         return false;
      }

      if (!normalized) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "%s(size=GL_BGRA and normalized=GL_FALSE)", func);
         return false;
      }

      format = GL_BGRA;
      size = 4;
   }
   else if (size < sizeMin || size > sizeMax || size > 4) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size=%d)", func, size);
      return false;
   }

   if (ctx->Extensions.ARB_vertex_type_2_10_10_10_rev &&
       (type == GL_UNSIGNED_INT_2_10_10_10_REV ||
        type == GL_INT_2_10_10_10_REV) && size != 4) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(size=%d)", func, size);
      return false;
   }

   /* The ARB_vertex_attrib_binding_spec says:
    *
    *   An INVALID_VALUE error is generated if <relativeoffset> is larger than
    *   the value of MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.
    */
   if (relativeOffset > ctx->Const.MaxVertexAttribRelativeOffset) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(relativeOffset=%d > "
                  "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET)",
                  func, relativeOffset);
      return false;
   }

   if (ctx->Extensions.ARB_vertex_type_10f_11f_11f_rev &&
         type == GL_UNSIGNED_INT_10F_11F_11F_REV && size != 3) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(size=%d)", func, size);
      return false;
   }

   ASSERT(size <= 4);

   elementSize = _mesa_bytes_per_vertex_attrib(size, type);
   assert(elementSize != -1);

   array = &ctx->Array.VAO->VertexAttrib[attrib];
   array->Size = size;
   array->Type = type;
   array->Format = format;
   array->Normalized = normalized;
   array->Integer = integer;
   array->RelativeOffset = relativeOffset;
   array->_ElementSize = elementSize;

   ctx->Array.VAO->NewArrays |= VERT_BIT(attrib);
   ctx->NewState |= _NEW_ARRAY;

   return true;
}


/**
 * Do error checking and update state for glVertex/Color/TexCoord/...Pointer
 * functions.
 *
 * \param func  name of calling function used for error reporting
 * \param attrib  the attribute array index to update
 * \param legalTypes  bitmask of *_BIT above indicating legal datatypes
 * \param sizeMin  min allowable size value
 * \param sizeMax  max allowable size value (may also be BGRA_OR_4)
 * \param size  components per element (1, 2, 3 or 4)
 * \param type  datatype of each component (GL_FLOAT, GL_INT, etc)
 * \param stride  stride between elements, in elements
 * \param normalized  are integer types converted to floats in [-1, 1]?
 * \param integer  integer-valued values (will not be normalized to [-1,1])
 * \param ptr  the address (or offset inside VBO) of the array data
 */
static void
update_array(struct gl_context *ctx,
             const char *func,
             GLuint attrib, GLbitfield legalTypesMask,
             GLint sizeMin, GLint sizeMax,
             GLint size, GLenum type, GLsizei stride,
             GLboolean normalized, GLboolean integer,
             const GLvoid *ptr)
{
   struct gl_vertex_attrib_array *array;
   GLsizei effectiveStride;

   /* Page 407 (page 423 of the PDF) of the OpenGL 3.0 spec says:
    *
    *     "Client vertex arrays - all vertex array attribute pointers must
    *     refer to buffer objects (section 2.9.2). The default vertex array
    *     object (the name zero) is also deprecated. Calling
    *     VertexAttribPointer when no buffer object or no vertex array object
    *     is bound will generate an INVALID_OPERATION error..."
    *
    * The check for VBOs is handled below.
    */
   if (ctx->API == API_OPENGL_CORE
       && (ctx->Array.VAO == ctx->Array.DefaultVAO)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(no array object bound)",
                  func);
      return;
   }

   if (stride < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "%s(stride=%d)", func, stride );
      return;
   }

   if (ctx->API == API_OPENGL_CORE && ctx->Version >= 44 &&
       stride > ctx->Const.MaxVertexAttribStride) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(stride=%d > "
                  "GL_MAX_VERTEX_ATTRIB_STRIDE)", func, stride);
      return;
   }

   /* Page 29 (page 44 of the PDF) of the OpenGL 3.3 spec says:
    *
    *     "An INVALID_OPERATION error is generated under any of the following
    *     conditions:
    *
    *     ...
    *
    *     * any of the *Pointer commands specifying the location and
    *       organization of vertex array data are called while zero is bound
    *       to the ARRAY_BUFFER buffer object binding point (see section
    *       2.9.6), and the pointer argument is not NULL."
    */
   if (ptr != NULL && ctx->Array.VAO->ARBsemantics &&
       !_mesa_is_bufferobj(ctx->Array.ArrayBufferObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(non-VBO array)", func);
      return;
   }

   if (!update_array_format(ctx, func, attrib, legalTypesMask, sizeMin,
                            sizeMax, size, type, normalized, integer, 0)) {
      return;
   }

   /* Reset the vertex attrib binding */
   vertex_attrib_binding(ctx, attrib, attrib);

   /* The Stride and Ptr fields are not set by update_array_format() */
   array = &ctx->Array.VAO->VertexAttrib[attrib];
   array->Stride = stride;
   array->Ptr = (const GLvoid *) ptr;

   /* Update the vertex buffer binding */
   effectiveStride = stride != 0 ? stride : array->_ElementSize;
   bind_vertex_buffer(ctx, attrib, ctx->Array.ArrayBufferObj,
                      (GLintptr) ptr, effectiveStride);
}


void GLAPIENTRY
_mesa_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   GLbitfield legalTypes = (ctx->API == API_OPENGLES)
      ? (BYTE_BIT | SHORT_BIT | FLOAT_BIT | FIXED_ES_BIT)
      : (SHORT_BIT | INT_BIT | FLOAT_BIT |
         DOUBLE_BIT | HALF_BIT |
         UNSIGNED_INT_2_10_10_10_REV_BIT |
         INT_2_10_10_10_REV_BIT);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glVertexPointer", VERT_ATTRIB_POS,
                legalTypes, 2, 4,
                size, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr )
{
   GET_CURRENT_CONTEXT(ctx);
   const GLbitfield legalTypes = (ctx->API == API_OPENGLES)
      ? (BYTE_BIT | SHORT_BIT | FLOAT_BIT | FIXED_ES_BIT)
      : (BYTE_BIT | SHORT_BIT | INT_BIT |
         HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
         UNSIGNED_INT_2_10_10_10_REV_BIT |
         INT_2_10_10_10_REV_BIT);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glNormalPointer", VERT_ATTRIB_NORMAL,
                legalTypes, 3, 3,
                3, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLbitfield legalTypes = (ctx->API == API_OPENGLES)
      ? (UNSIGNED_BYTE_BIT | HALF_BIT | FLOAT_BIT | FIXED_ES_BIT)
      : (BYTE_BIT | UNSIGNED_BYTE_BIT |
         SHORT_BIT | UNSIGNED_SHORT_BIT |
         INT_BIT | UNSIGNED_INT_BIT |
         HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
         UNSIGNED_INT_2_10_10_10_REV_BIT |
         INT_2_10_10_10_REV_BIT);
   const GLint sizeMin = (ctx->API == API_OPENGLES) ? 4 : 3;

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glColorPointer", VERT_ATTRIB_COLOR0,
                legalTypes, sizeMin, BGRA_OR_4,
                size, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_FogCoordPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (HALF_BIT | FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glFogCoordPointer", VERT_ATTRIB_FOG,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (UNSIGNED_BYTE_BIT | SHORT_BIT | INT_BIT |
                                  FLOAT_BIT | DOUBLE_BIT);
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glIndexPointer", VERT_ATTRIB_COLOR_INDEX,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_SecondaryColorPointer(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glSecondaryColorPointer", VERT_ATTRIB_COLOR1,
                legalTypes, 3, BGRA_OR_4,
                size, type, stride, GL_TRUE, GL_FALSE, ptr);
}


void GLAPIENTRY
_mesa_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                      const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   GLbitfield legalTypes = (ctx->API == API_OPENGLES)
      ? (BYTE_BIT | SHORT_BIT | FLOAT_BIT | FIXED_ES_BIT)
      : (SHORT_BIT | INT_BIT |
         HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
         UNSIGNED_INT_2_10_10_10_REV_BIT |
         INT_2_10_10_10_REV_BIT);
   const GLint sizeMin = (ctx->API == API_OPENGLES) ? 2 : 1;
   const GLuint unit = ctx->Array.ActiveTexture;

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glTexCoordPointer", VERT_ATTRIB_TEX(unit),
                legalTypes, sizeMin, 4,
                size, type, stride, GL_FALSE, GL_FALSE,
                ptr);
}


void GLAPIENTRY
_mesa_EdgeFlagPointer(GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = UNSIGNED_BYTE_BIT;
   /* this is the same type that glEdgeFlag uses */
   const GLboolean integer = GL_FALSE;
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   update_array(ctx, "glEdgeFlagPointer", VERT_ATTRIB_EDGEFLAG,
                legalTypes, 1, 1,
                1, GL_UNSIGNED_BYTE, stride, GL_FALSE, integer, ptr);
}


void GLAPIENTRY
_mesa_PointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (FLOAT_BIT | FIXED_ES_BIT);
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (ctx->API != API_OPENGLES) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glPointSizePointer(ES 1.x only)");
      return;
   }
      
   update_array(ctx, "glPointSizePointer", VERT_ATTRIB_POINT_SIZE,
                legalTypes, 1, 1,
                1, type, stride, GL_FALSE, GL_FALSE, ptr);
}


/**
 * Set a generic vertex attribute array.
 * Note that these arrays DO NOT alias the conventional GL vertex arrays
 * (position, normal, color, fog, texcoord, etc).
 */
void GLAPIENTRY
_mesa_VertexAttribPointer(GLuint index, GLint size, GLenum type,
                             GLboolean normalized,
                             GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT |
                                  HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                  FIXED_ES_BIT | FIXED_GL_BIT |
                                  UNSIGNED_INT_2_10_10_10_REV_BIT |
                                  INT_2_10_10_10_REV_BIT |
                                  UNSIGNED_INT_10F_11F_11F_REV_BIT);
   GET_CURRENT_CONTEXT(ctx);

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribPointerARB(index)");
      return;
   }

   update_array(ctx, "glVertexAttribPointer", VERT_ATTRIB_GENERIC(index),
                legalTypes, 1, BGRA_OR_4,
                size, type, stride, normalized, GL_FALSE, ptr);
}


/**
 * GL_EXT_gpu_shader4 / GL 3.0.
 * Set an integer-valued vertex attribute array.
 * Note that these arrays DO NOT alias the conventional GL vertex arrays
 * (position, normal, color, fog, texcoord, etc).
 */
void GLAPIENTRY
_mesa_VertexAttribIPointer(GLuint index, GLint size, GLenum type,
                           GLsizei stride, const GLvoid *ptr)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT);
   const GLboolean normalized = GL_FALSE;
   const GLboolean integer = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribIPointer(index)");
      return;
   }

   update_array(ctx, "glVertexAttribIPointer", VERT_ATTRIB_GENERIC(index),
                legalTypes, 1, 4,
                size, type, stride, normalized, integer, ptr);
}



void GLAPIENTRY
_mesa_EnableVertexAttribArray(GLuint index)
{
   struct gl_vertex_array_object *vao;
   GET_CURRENT_CONTEXT(ctx);

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glEnableVertexAttribArrayARB(index)");
      return;
   }

   vao = ctx->Array.VAO;

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(vao->VertexAttrib));

   if (!vao->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled) {
      /* was disabled, now being enabled */
      FLUSH_VERTICES(ctx, _NEW_ARRAY);
      vao->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled = GL_TRUE;
      vao->_Enabled |= VERT_BIT_GENERIC(index);
      vao->NewArrays |= VERT_BIT_GENERIC(index);
   }
}


void GLAPIENTRY
_mesa_DisableVertexAttribArray(GLuint index)
{
   struct gl_vertex_array_object *vao;
   GET_CURRENT_CONTEXT(ctx);

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDisableVertexAttribArrayARB(index)");
      return;
   }

   vao = ctx->Array.VAO;

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(vao->VertexAttrib));

   if (vao->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled) {
      /* was enabled, now being disabled */
      FLUSH_VERTICES(ctx, _NEW_ARRAY);
      vao->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Enabled = GL_FALSE;
      vao->_Enabled &= ~VERT_BIT_GENERIC(index);
      vao->NewArrays |= VERT_BIT_GENERIC(index);
   }
}


/**
 * Return info for a vertex attribute array (no alias with legacy
 * vertex attributes (pos, normal, color, etc)).  This function does
 * not handle the 4-element GL_CURRENT_VERTEX_ATTRIB_ARB query.
 */
static GLuint
get_vertex_array_attrib(struct gl_context *ctx, GLuint index, GLenum pname,
                  const char *caller)
{
   const struct gl_vertex_array_object *vao = ctx->Array.VAO;
   const struct gl_vertex_attrib_array *array;

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)", caller, index);
      return 0;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(vao->VertexAttrib));

   array = &vao->VertexAttrib[VERT_ATTRIB_GENERIC(index)];

   switch (pname) {
   case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
      return array->Enabled;
   case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
      return (array->Format == GL_BGRA) ? GL_BGRA : array->Size;
   case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
      return array->Stride;
   case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
      return array->Type;
   case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
      return array->Normalized;
   case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
      return vao->VertexBinding[array->VertexBinding].BufferObj->Name;
   case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
      if ((_mesa_is_desktop_gl(ctx)
           && (ctx->Version >= 30 || ctx->Extensions.EXT_gpu_shader4))
          || _mesa_is_gles3(ctx)) {
         return array->Integer;
      }
      goto error;
   case GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB:
      if ((_mesa_is_desktop_gl(ctx) && ctx->Extensions.ARB_instanced_arrays)
          || _mesa_is_gles3(ctx)) {
         return vao->VertexBinding[array->VertexBinding].InstanceDivisor;
      }
      goto error;
   case GL_VERTEX_ATTRIB_BINDING:
      if (_mesa_is_desktop_gl(ctx)) {
         return array->VertexBinding - VERT_ATTRIB_GENERIC0;
      }
      goto error;
   case GL_VERTEX_ATTRIB_RELATIVE_OFFSET:
      if (_mesa_is_desktop_gl(ctx)) {
         return array->RelativeOffset;
      }
      goto error;
   default:
      ; /* fall-through */
   }

error:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=0x%x)", caller, pname);
   return 0;
}


static const GLfloat *
get_current_attrib(struct gl_context *ctx, GLuint index, const char *function)
{
   if (index == 0) {
      if (_mesa_attr_zero_aliases_vertex(ctx)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "%s(index==0)", function);
	 return NULL;
      }
   }
   else if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(index>=GL_MAX_VERTEX_ATTRIBS)", function);
      return NULL;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.VAO->VertexAttrib));

   FLUSH_CURRENT(ctx, 0);
   return ctx->Current.Attrib[VERT_ATTRIB_GENERIC(index)];
}

void GLAPIENTRY
_mesa_GetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribfv");
      if (v != NULL) {
         COPY_4V(params, v);
      }
   }
   else {
      params[0] = (GLfloat) get_vertex_array_attrib(ctx, index, pname,
                                                    "glGetVertexAttribfv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribdv");
      if (v != NULL) {
         params[0] = (GLdouble) v[0];
         params[1] = (GLdouble) v[1];
         params[2] = (GLdouble) v[2];
         params[3] = (GLdouble) v[3];
      }
   }
   else {
      params[0] = (GLdouble) get_vertex_array_attrib(ctx, index, pname,
                                                     "glGetVertexAttribdv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribiv(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLfloat *v = get_current_attrib(ctx, index, "glGetVertexAttribiv");
      if (v != NULL) {
         /* XXX should floats in[0,1] be scaled to full int range? */
         params[0] = (GLint) v[0];
         params[1] = (GLint) v[1];
         params[2] = (GLint) v[2];
         params[3] = (GLint) v[3];
      }
   }
   else {
      params[0] = (GLint) get_vertex_array_attrib(ctx, index, pname,
                                                  "glGetVertexAttribiv");
   }
}


/** GL 3.0 */
void GLAPIENTRY
_mesa_GetVertexAttribIiv(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLint *v = (const GLint *)
	 get_current_attrib(ctx, index, "glGetVertexAttribIiv");
      if (v != NULL) {
         COPY_4V(params, v);
      }
   }
   else {
      params[0] = (GLint) get_vertex_array_attrib(ctx, index, pname,
                                                  "glGetVertexAttribIiv");
   }
}


/** GL 3.0 */
void GLAPIENTRY
_mesa_GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
      const GLuint *v = (const GLuint *)
	 get_current_attrib(ctx, index, "glGetVertexAttribIuiv");
      if (v != NULL) {
         COPY_4V(params, v);
      }
   }
   else {
      params[0] = get_vertex_array_attrib(ctx, index, pname,
                                          "glGetVertexAttribIuiv");
   }
}


void GLAPIENTRY
_mesa_GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerARB(index)");
      return;
   }

   if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerARB(pname)");
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(index) < Elements(ctx->Array.VAO->VertexAttrib));

   *pointer = (GLvoid *) ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_GENERIC(index)].Ptr;
}


void GLAPIENTRY
_mesa_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_VertexPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr)
{
   (void) count;
   _mesa_NormalPointer(type, stride, ptr);
}


void GLAPIENTRY
_mesa_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_ColorPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   (void) count;
   _mesa_IndexPointer(type, stride, ptr);
}


void GLAPIENTRY
_mesa_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                         GLsizei count, const GLvoid *ptr)
{
   (void) count;
   _mesa_TexCoordPointer(size, type, stride, ptr);
}


void GLAPIENTRY
_mesa_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   (void) count;
   _mesa_EdgeFlagPointer(stride, ptr);
}


void GLAPIENTRY
_mesa_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean tflag, cflag, nflag;  /* enable/disable flags */
   GLint tcomps, ccomps, vcomps;   /* components per texcoord, color, vertex */
   GLenum ctype = 0;               /* color type */
   GLint coffset = 0, noffset = 0, voffset;/* color, normal, vertex offsets */
   const GLint toffset = 0;        /* always zero */
   GLint defstride;                /* default stride */
   GLint c, f;

   FLUSH_VERTICES(ctx, 0);

   f = sizeof(GLfloat);
   c = f * ((4 * sizeof(GLubyte) + (f - 1)) / f);

   if (stride < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glInterleavedArrays(stride)" );
      return;
   }

   switch (format) {
      case GL_V2F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 2;
         voffset = 0;
         defstride = 2*f;
         break;
      case GL_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         voffset = 0;
         defstride = 3*f;
         break;
      case GL_C4UB_V2F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 2;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 2*f;
         break;
      case GL_C4UB_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 0;
         voffset = c;
         defstride = c + 3*f;
         break;
      case GL_C3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 0;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 0;  vcomps = 3;
         noffset = 0;
         voffset = 3*f;
         defstride = 6*f;
         break;
      case GL_C4F_N3F_V3F:
         tflag = GL_FALSE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 0;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 0;
         noffset = 4*f;
         voffset = 7*f;
         defstride = 10*f;
         break;
      case GL_T2F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         voffset = 2*f;
         defstride = 5*f;
         break;
      case GL_T4F_V4F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_FALSE;
         tcomps = 4;  ccomps = 0;  vcomps = 4;
         voffset = 4*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4UB_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_UNSIGNED_BYTE;
         coffset = 2*f;
         voffset = c+2*f;
         defstride = c+5*f;
         break;
      case GL_T2F_C3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_FALSE;
         tcomps = 2;  ccomps = 3;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_FALSE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 0;  vcomps = 3;
         noffset = 2*f;
         voffset = 5*f;
         defstride = 8*f;
         break;
      case GL_T2F_C4F_N3F_V3F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 2;  ccomps = 4;  vcomps = 3;
         ctype = GL_FLOAT;
         coffset = 2*f;
         noffset = 6*f;
         voffset = 9*f;
         defstride = 12*f;
         break;
      case GL_T4F_C4F_N3F_V4F:
         tflag = GL_TRUE;  cflag = GL_TRUE;  nflag = GL_TRUE;
         tcomps = 4;  ccomps = 4;  vcomps = 4;
         ctype = GL_FLOAT;
         coffset = 4*f;
         noffset = 8*f;
         voffset = 11*f;
         defstride = 15*f;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glInterleavedArrays(format)" );
         return;
   }

   if (stride==0) {
      stride = defstride;
   }

   _mesa_DisableClientState( GL_EDGE_FLAG_ARRAY );
   _mesa_DisableClientState( GL_INDEX_ARRAY );
   /* XXX also disable secondary color and generic arrays? */

   /* Texcoords */
   if (tflag) {
      _mesa_EnableClientState( GL_TEXTURE_COORD_ARRAY );
      _mesa_TexCoordPointer( tcomps, GL_FLOAT, stride,
                             (GLubyte *) pointer + toffset );
   }
   else {
      _mesa_DisableClientState( GL_TEXTURE_COORD_ARRAY );
   }

   /* Color */
   if (cflag) {
      _mesa_EnableClientState( GL_COLOR_ARRAY );
      _mesa_ColorPointer( ccomps, ctype, stride,
			  (GLubyte *) pointer + coffset );
   }
   else {
      _mesa_DisableClientState( GL_COLOR_ARRAY );
   }


   /* Normals */
   if (nflag) {
      _mesa_EnableClientState( GL_NORMAL_ARRAY );
      _mesa_NormalPointer( GL_FLOAT, stride, (GLubyte *) pointer + noffset );
   }
   else {
      _mesa_DisableClientState( GL_NORMAL_ARRAY );
   }

   /* Vertices */
   _mesa_EnableClientState( GL_VERTEX_ARRAY );
   _mesa_VertexPointer( vcomps, GL_FLOAT, stride,
			(GLubyte *) pointer + voffset );
}


void GLAPIENTRY
_mesa_LockArraysEXT(GLint first, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glLockArrays %d %d\n", first, count);

   if (first < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLockArraysEXT(first)" );
      return;
   }
   if (count <= 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLockArraysEXT(count)" );
      return;
   }
   if (ctx->Array.LockCount != 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glLockArraysEXT(reentry)" );
      return;
   }

   ctx->Array.LockFirst = first;
   ctx->Array.LockCount = count;

   ctx->NewState |= _NEW_ARRAY;
}


void GLAPIENTRY
_mesa_UnlockArraysEXT( void )
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glUnlockArrays\n");

   if (ctx->Array.LockCount == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glUnlockArraysEXT(reexit)" );
      return;
   }

   ctx->Array.LockFirst = 0;
   ctx->Array.LockCount = 0;
   ctx->NewState |= _NEW_ARRAY;
}


/* GL_EXT_multi_draw_arrays */
void GLAPIENTRY
_mesa_MultiDrawArrays( GLenum mode, const GLint *first,
                          const GLsizei *count, GLsizei primcount )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   FLUSH_VERTICES(ctx, 0);

   for (i = 0; i < primcount; i++) {
      if (count[i] > 0) {
         CALL_DrawArrays(ctx->CurrentDispatch, (mode, first[i], count[i]));
      }
   }
}


/* GL_IBM_multimode_draw_arrays */
void GLAPIENTRY
_mesa_MultiModeDrawArraysIBM( const GLenum * mode, const GLint * first,
			      const GLsizei * count,
			      GLsizei primcount, GLint modestride )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   FLUSH_VERTICES(ctx, 0);

   for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
         GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
	 CALL_DrawArrays(ctx->CurrentDispatch, ( m, first[i], count[i] ));
      }
   }
}


/* GL_IBM_multimode_draw_arrays */
void GLAPIENTRY
_mesa_MultiModeDrawElementsIBM( const GLenum * mode, const GLsizei * count,
				GLenum type, const GLvoid * const * indices,
				GLsizei primcount, GLint modestride )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   FLUSH_VERTICES(ctx, 0);

   /* XXX not sure about ARB_vertex_buffer_object handling here */

   for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
         GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
	 CALL_DrawElements(ctx->CurrentDispatch, ( m, count[i], type,
                                                   indices[i] ));
      }
   }
}


/**
 * GL_NV_primitive_restart and GL 3.1
 */
void GLAPIENTRY
_mesa_PrimitiveRestartIndex(GLuint index)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.NV_primitive_restart && ctx->Version < 31) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPrimitiveRestartIndexNV()");
      return;
   }

   if (ctx->Array.RestartIndex != index) {
      FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
      ctx->Array.RestartIndex = index;
   }
}


/**
 * See GL_ARB_instanced_arrays.
 * Note that the instance divisor only applies to generic arrays, not
 * the legacy vertex arrays.
 */
void GLAPIENTRY
_mesa_VertexAttribDivisor(GLuint index, GLuint divisor)
{
   GET_CURRENT_CONTEXT(ctx);

   const GLuint genericIndex = VERT_ATTRIB_GENERIC(index);

   if (!ctx->Extensions.ARB_instanced_arrays) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glVertexAttribDivisor()");
      return;
   }

   if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glVertexAttribDivisor(index = %u)",
                  index);
      return;
   }

   ASSERT(genericIndex < Elements(ctx->Array.VAO->VertexAttrib));

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "The command
    *
    *       void VertexAttribDivisor(uint index, uint divisor);
    *
    *     is equivalent to (assuming no errors are generated):
    *
    *       VertexAttribBinding(index, index);
    *       VertexBindingDivisor(index, divisor);"
    */
   vertex_attrib_binding(ctx, genericIndex, genericIndex);
   vertex_binding_divisor(ctx, genericIndex, divisor);
}


unsigned
_mesa_primitive_restart_index(const struct gl_context *ctx, GLenum ib_type)
{
   /* From the OpenGL 4.3 core specification, page 302:
    * "If both PRIMITIVE_RESTART and PRIMITIVE_RESTART_FIXED_INDEX are
    *  enabled, the index value determined by PRIMITIVE_RESTART_FIXED_INDEX
    *  is used."
    */
   if (ctx->Array.PrimitiveRestartFixedIndex) {
      switch (ib_type) {
      case GL_UNSIGNED_BYTE:
         return 0xff;
      case GL_UNSIGNED_SHORT:
         return 0xffff;
      case GL_UNSIGNED_INT:
         return 0xffffffff;
      default:
         assert(!"_mesa_primitive_restart_index: Invalid index buffer type.");
      }
   }

   return ctx->Array.RestartIndex;
}


/**
 * GL_ARB_vertex_attrib_binding
 */
void GLAPIENTRY
_mesa_BindVertexBuffer(GLuint bindingIndex, GLuint buffer, GLintptr offset,
                       GLsizei stride)
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_vertex_array_object *vao = ctx->Array.VAO;
   struct gl_buffer_object *vbo;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated if no vertex array object
    *     is bound."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindVertexBuffer(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_VALUE error is generated if <bindingindex> is greater than
    *     the value of MAX_VERTEX_ATTRIB_BINDINGS."
    */
   if (bindingIndex >= ctx->Const.MaxVertexAttribBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindVertexBuffer(bindingindex=%u > "
                  "GL_MAX_VERTEX_ATTRIB_BINDINGS)",
                  bindingIndex);
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "The error INVALID_VALUE is generated if <stride> or <offset>
    *     are negative."
    */
   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindVertexBuffer(offset=%" PRId64 " < 0)",
                  (int64_t) offset);
      return;
   }

   if (stride < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindVertexBuffer(stride=%d < 0)", stride);
      return;
   }

   if (ctx->API == API_OPENGL_CORE && ctx->Version >= 44 &&
       stride > ctx->Const.MaxVertexAttribStride) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindVertexBuffer(stride=%d > "
                  "GL_MAX_VERTEX_ATTRIB_STRIDE)", stride);
      return;
   }

   if (buffer == vao->VertexBinding[VERT_ATTRIB_GENERIC(bindingIndex)].BufferObj->Name) {
      vbo = vao->VertexBinding[VERT_ATTRIB_GENERIC(bindingIndex)].BufferObj;
   } else if (buffer != 0) {
      vbo = _mesa_lookup_bufferobj(ctx, buffer);

      /* From the GL_ARB_vertex_attrib_array spec:
       *
       *   "[Core profile only:]
       *    An INVALID_OPERATION error is generated if buffer is not zero or a
       *    name returned from a previous call to GenBuffers, or if such a name
       *    has since been deleted with DeleteBuffers.
       *
       * Otherwise, we fall back to the same compat profile behavior as other
       * object references (automatically gen it).
       */
      if (!_mesa_handle_bind_buffer_gen(ctx, GL_ARRAY_BUFFER, buffer,
                                        &vbo, "glBindVertexBuffer"))
         return;
   } else {
      /* The ARB_vertex_attrib_binding spec says:
       *
       *    "If <buffer> is zero, any buffer object attached to this
       *     bindpoint is detached."
       */
      vbo = ctx->Shared->NullBufferObj;
   }

   bind_vertex_buffer(ctx, VERT_ATTRIB_GENERIC(bindingIndex),
                      vbo, offset, stride);
}


void GLAPIENTRY
_mesa_BindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers,
                        const GLintptr *offsets, const GLsizei *strides)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object * const vao = ctx->Array.VAO;
   GLuint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated if no
    *     vertex array object is bound."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindVertexBuffers(No array object bound)");
      return;
   }

   /* The ARB_multi_bind spec says:
    *
    *    "An INVALID_OPERATION error is generated if <first> + <count>
    *     is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS."
    */
   if (first + count > ctx->Const.MaxVertexAttribBindings) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindVertexBuffers(first=%u + count=%d > the value of "
                  "GL_MAX_VERTEX_ATTRIB_BINDINGS=%u)",
                  first, count, ctx->Const.MaxVertexAttribBindings);
      return;
   }

   if (!buffers) {
      /**
       * The ARB_multi_bind spec says:
       *
       *    "If <buffers> is NULL, each affected vertex buffer binding point
       *     from <first> through <first>+<count>-1 will be reset to have no
       *     bound buffer object.  In this case, the offsets and strides
       *     associated with the binding points are set to default values,
       *     ignoring <offsets> and <strides>."
       */
      struct gl_buffer_object *vbo = ctx->Shared->NullBufferObj;

      for (i = 0; i < count; i++)
         bind_vertex_buffer(ctx, VERT_ATTRIB_GENERIC(first + i), vbo, 0, 16);

      return;
   }

   /* Note that the error semantics for multi-bind commands differ from
    * those of other GL commands.
    *
    * The Issues section in the ARB_multi_bind spec says:
    *
    *    "(11) Typically, OpenGL specifies that if an error is generated by
    *          a command, that command has no effect.  This is somewhat
    *          unfortunate for multi-bind commands, because it would require
    *          a first pass to scan the entire list of bound objects for
    *          errors and then a second pass to actually perform the
    *          bindings.  Should we have different error semantics?
    *
    *       RESOLVED:  Yes.  In this specification, when the parameters for
    *       one of the <count> binding points are invalid, that binding
    *       point is not updated and an error will be generated.  However,
    *       other binding points in the same command will be updated if
    *       their parameters are valid and no other error occurs."
    */

   _mesa_begin_bufferobj_lookups(ctx);

   for (i = 0; i < count; i++) {
      struct gl_buffer_object *vbo;

      /* The ARB_multi_bind spec says:
       *
       *    "An INVALID_VALUE error is generated if any value in
       *     <offsets> or <strides> is negative (per binding)."
       */
      if (offsets[i] < 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glBindVertexBuffers(offsets[%u]=%" PRId64 " < 0)",
                     i, (int64_t) offsets[i]);
         continue;
      }

      if (strides[i] < 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glBindVertexBuffers(strides[%u]=%d < 0)",
                     i, strides[i]);
         continue;
      }

      if (ctx->API == API_OPENGL_CORE && ctx->Version >= 44 &&
          strides[i] > ctx->Const.MaxVertexAttribStride) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glBindVertexBuffers(strides[%u]=%d > "
                     "GL_MAX_VERTEX_ATTRIB_STRIDE)", i, strides[i]);
         continue;
      }

      if (buffers[i]) {
         struct gl_vertex_buffer_binding *binding =
            &vao->VertexBinding[VERT_ATTRIB_GENERIC(first + i)];

         if (buffers[i] == binding->BufferObj->Name)
            vbo = binding->BufferObj;
         else
            vbo = _mesa_multi_bind_lookup_bufferobj(ctx, buffers, i,
                                                    "glBindVertexBuffers");

         if (!vbo)
            continue;
      } else {
         vbo = ctx->Shared->NullBufferObj;
      }

      bind_vertex_buffer(ctx, VERT_ATTRIB_GENERIC(first + i), vbo,
                         offsets[i], strides[i]);
   }

   _mesa_end_bufferobj_lookups(ctx);
}


void GLAPIENTRY
_mesa_VertexAttribFormat(GLuint attribIndex, GLint size, GLenum type,
                         GLboolean normalized, GLuint relativeOffset)
{
    const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                   SHORT_BIT | UNSIGNED_SHORT_BIT |
                                   INT_BIT | UNSIGNED_INT_BIT |
                                   HALF_BIT | FLOAT_BIT | DOUBLE_BIT |
                                   FIXED_GL_BIT |
                                   UNSIGNED_INT_2_10_10_10_REV_BIT |
                                   INT_2_10_10_10_REV_BIT |
                                   UNSIGNED_INT_10F_11F_11F_REV_BIT);

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated under any of the following
    *     conditions:
    *     - if no vertex array object is currently bound (see section 2.10);
    *     - ..."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glVertexAttribFormat(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *   "The error INVALID_VALUE is generated if index is greater than or equal
    *     to the value of MAX_VERTEX_ATTRIBS."
    */
   if (attribIndex >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexAttribFormat(attribindex=%u > "
                  "GL_MAX_VERTEX_ATTRIBS)",
                  attribIndex);
      return;
   }

   FLUSH_VERTICES(ctx, 0);

   update_array_format(ctx, "glVertexAttribFormat",
                       VERT_ATTRIB_GENERIC(attribIndex),
                       legalTypes, 1, BGRA_OR_4, size, type, normalized,
                       GL_FALSE, relativeOffset);
}


void GLAPIENTRY
_mesa_VertexAttribIFormat(GLuint attribIndex, GLint size, GLenum type,
                          GLuint relativeOffset)
{
   const GLbitfield legalTypes = (BYTE_BIT | UNSIGNED_BYTE_BIT |
                                  SHORT_BIT | UNSIGNED_SHORT_BIT |
                                  INT_BIT | UNSIGNED_INT_BIT);

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated under any of the following
    *     conditions:
    *     - if no vertex array object is currently bound (see section 2.10);
    *     - ..."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glVertexAttribIFormat(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *   "The error INVALID_VALUE is generated if index is greater than
    *    or equal to the value of MAX_VERTEX_ATTRIBS."
    */
   if (attribIndex >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexAttribIFormat(attribindex=%u > "
                  "GL_MAX_VERTEX_ATTRIBS)",
                  attribIndex);
      return;
   }

   FLUSH_VERTICES(ctx, 0);

   update_array_format(ctx, "glVertexAttribIFormat",
                       VERT_ATTRIB_GENERIC(attribIndex),
                       legalTypes, 1, 4, size, type, GL_FALSE, GL_TRUE,
                       relativeOffset);
}


void GLAPIENTRY
_mesa_VertexAttribLFormat(GLuint attribIndex, GLint size, GLenum type,
                          GLuint relativeOffset)
{
   const GLbitfield legalTypes = DOUBLE_BIT;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* Page 298 of the PDF of the OpenGL 4.3 (Core Profile) spec says:
    *
    *    "An INVALID_OPERATION error is generated under any of the following
    *     conditions:
    *     • if no vertex array object is currently bound (see section 10.4);
    *     • ..."
    *
    * This language is missing from the extension spec, but we assume
    * that this is an oversight.
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glVertexAttribLFormat(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *   "The error INVALID_VALUE is generated if <attribindex> is greater than
    *    or equal to the value of MAX_VERTEX_ATTRIBS."
    */
   if (attribIndex >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexAttribLFormat(attribindex=%u > "
                  "GL_MAX_VERTEX_ATTRIBS)",
                  attribIndex);
      return;
   }

   FLUSH_VERTICES(ctx, 0);

   update_array_format(ctx, "glVertexAttribLFormat",
                       VERT_ATTRIB_GENERIC(attribIndex),
                       legalTypes, 1, 4, size, type, GL_FALSE, GL_FALSE,
                       relativeOffset);
}


void GLAPIENTRY
_mesa_VertexAttribBinding(GLuint attribIndex, GLuint bindingIndex)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated if no vertex array object
    *     is bound."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glVertexAttribBinding(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "<attribindex> must be less than the value of MAX_VERTEX_ATTRIBS and
    *     <bindingindex> must be less than the value of
    *     MAX_VERTEX_ATTRIB_BINDINGS, otherwise the error INVALID_VALUE
    *     is generated."
    */
   if (attribIndex >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexAttribBinding(attribindex=%u >= "
                  "GL_MAX_VERTEX_ATTRIBS)",
                  attribIndex);
      return;
   }

   if (bindingIndex >= ctx->Const.MaxVertexAttribBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexAttribBinding(bindingindex=%u >= "
                  "GL_MAX_VERTEX_ATTRIB_BINDINGS)",
                  bindingIndex);
      return;
   }

   ASSERT(VERT_ATTRIB_GENERIC(attribIndex) <
          Elements(ctx->Array.VAO->VertexAttrib));

   vertex_attrib_binding(ctx, VERT_ATTRIB_GENERIC(attribIndex),
                         VERT_ATTRIB_GENERIC(bindingIndex));
}


void GLAPIENTRY
_mesa_VertexBindingDivisor(GLuint bindingIndex, GLuint divisor)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ARB_instanced_arrays) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glVertexBindingDivisor()");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_OPERATION error is generated if no vertex array object
    *     is bound."
    */
   if (ctx->API == API_OPENGL_CORE &&
       ctx->Array.VAO == ctx->Array.DefaultVAO) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glVertexBindingDivisor(No array object bound)");
      return;
   }

   /* The ARB_vertex_attrib_binding spec says:
    *
    *    "An INVALID_VALUE error is generated if <bindingindex> is greater
    *     than or equal to the value of MAX_VERTEX_ATTRIB_BINDINGS."
    */
   if (bindingIndex >= ctx->Const.MaxVertexAttribBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glVertexBindingDivisor(bindingindex=%u > "
                  "GL_MAX_VERTEX_ATTRIB_BINDINGS)",
                  bindingIndex);
      return;
   }

   vertex_binding_divisor(ctx, VERT_ATTRIB_GENERIC(bindingIndex), divisor);
}


/**
 * Copy one client vertex array to another.
 */
void
_mesa_copy_client_array(struct gl_context *ctx,
                        struct gl_client_array *dst,
                        struct gl_client_array *src)
{
   dst->Size = src->Size;
   dst->Type = src->Type;
   dst->Format = src->Format;
   dst->Stride = src->Stride;
   dst->StrideB = src->StrideB;
   dst->Ptr = src->Ptr;
   dst->Enabled = src->Enabled;
   dst->Normalized = src->Normalized;
   dst->Integer = src->Integer;
   dst->InstanceDivisor = src->InstanceDivisor;
   dst->_ElementSize = src->_ElementSize;
   _mesa_reference_buffer_object(ctx, &dst->BufferObj, src->BufferObj);
}

void
_mesa_copy_vertex_attrib_array(struct gl_context *ctx,
                               struct gl_vertex_attrib_array *dst,
                               const struct gl_vertex_attrib_array *src)
{
   dst->Size           = src->Size;
   dst->Type           = src->Type;
   dst->Format         = src->Format;
   dst->VertexBinding  = src->VertexBinding;
   dst->RelativeOffset = src->RelativeOffset;
   dst->Format         = src->Format;
   dst->Integer        = src->Integer;
   dst->Normalized     = src->Normalized;
   dst->Ptr            = src->Ptr;
   dst->Enabled        = src->Enabled;
   dst->_ElementSize   = src->_ElementSize;
}

void
_mesa_copy_vertex_buffer_binding(struct gl_context *ctx,
                                 struct gl_vertex_buffer_binding *dst,
                                 const struct gl_vertex_buffer_binding *src)
{
   dst->Offset          = src->Offset;
   dst->Stride          = src->Stride;
   dst->InstanceDivisor = src->InstanceDivisor;
   dst->_BoundArrays    = src->_BoundArrays;

   _mesa_reference_buffer_object(ctx, &dst->BufferObj, src->BufferObj);
}

/**
 * Print vertex array's fields.
 */
static void
print_array(const char *name, GLint index, const struct gl_client_array *array)
{
   if (index >= 0)
      fprintf(stderr, "  %s[%d]: ", name, index);
   else
      fprintf(stderr, "  %s: ", name);
   fprintf(stderr, "Ptr=%p, Type=0x%x, Size=%d, ElemSize=%u, Stride=%d, Buffer=%u(Size %lu)\n",
	  array->Ptr, array->Type, array->Size,
	  array->_ElementSize, array->StrideB,
	  array->BufferObj->Name, (unsigned long) array->BufferObj->Size);
}


/**
 * Print current vertex object/array info.  For debug.
 */
void
_mesa_print_arrays(struct gl_context *ctx)
{
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   GLuint i;

   printf("Array Object %u\n", vao->Name);
   if (vao->_VertexAttrib[VERT_ATTRIB_POS].Enabled)
      print_array("Vertex", -1, &vao->_VertexAttrib[VERT_ATTRIB_POS]);
   if (vao->_VertexAttrib[VERT_ATTRIB_NORMAL].Enabled)
      print_array("Normal", -1, &vao->_VertexAttrib[VERT_ATTRIB_NORMAL]);
   if (vao->_VertexAttrib[VERT_ATTRIB_COLOR0].Enabled)
      print_array("Color", -1, &vao->_VertexAttrib[VERT_ATTRIB_COLOR0]);
   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++)
      if (vao->_VertexAttrib[VERT_ATTRIB_TEX(i)].Enabled)
         print_array("TexCoord", i, &vao->_VertexAttrib[VERT_ATTRIB_TEX(i)]);
   for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++)
      if (vao->_VertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
         print_array("Attrib", i, &vao->_VertexAttrib[VERT_ATTRIB_GENERIC(i)]);
}


/**
 * Initialize vertex array state for given context.
 */
void 
_mesa_init_varray(struct gl_context *ctx)
{
   ctx->Array.DefaultVAO = ctx->Driver.NewArrayObject(ctx, 0);
   _mesa_reference_vao(ctx, &ctx->Array.VAO, ctx->Array.DefaultVAO);
   ctx->Array.ActiveTexture = 0;   /* GL_ARB_multitexture */

   ctx->Array.Objects = _mesa_NewHashTable();
}


/**
 * Callback for deleting an array object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_arrayobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_vertex_array_object *vao = (struct gl_vertex_array_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   _mesa_delete_vao(ctx, vao);
}


/**
 * Free vertex array state for given context.
 */
void 
_mesa_free_varray_data(struct gl_context *ctx)
{
   _mesa_HashDeleteAll(ctx->Array.Objects, delete_arrayobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Array.Objects);
}
