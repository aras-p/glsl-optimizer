/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * Functions for allocating/managing software-based renderbuffers.
 * Also, routines for reading/writing software-based renderbuffer data as
 * ubytes, ushorts, uints, etc.
 */


#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/mtypes.h"
#include "main/renderbuffer.h"
#include "swrast/s_renderbuffer.h"


/*
 * Routines for get/put values in common buffer formats follow.
 */

/* Returns a bytes per pixel of the DataType in the get/put span
 * functions for at least a subset of the available combinations a
 * renderbuffer can have.
 *
 * It would be nice to see gl_renderbuffer start talking about a
 * gl_format instead of a GLenum DataType.
 */
static int
get_datatype_bytes(struct gl_renderbuffer *rb)
{
   int component_size;

   switch (rb->DataType) {
   case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
      component_size = 8;
      break;
   case GL_FLOAT:
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT_24_8_EXT:
      component_size = 4;
      break;
   case GL_UNSIGNED_SHORT:
      component_size = 2;
      break;
   case GL_UNSIGNED_BYTE:
      component_size = 1;
      break;
   default:
      component_size = 1;
      assert(0);
   }

   switch (rb->_BaseFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
      return component_size;
   default:
      return 4 * component_size;
   }
}

/* This is commonly used by most of the accessors. */
static void *
get_pointer_generic(struct gl_context *ctx, struct gl_renderbuffer *rb,
		    GLint x, GLint y)
{
   if (!rb->Data)
      return NULL;

   return ((char *) rb->Data +
	   (y * rb->RowStride + x) * _mesa_get_format_bytes(rb->Format));
}

/* GetRow() implementation for formats where DataType matches the rb->Format.
 */
static void
get_row_generic(struct gl_context *ctx, struct gl_renderbuffer *rb,
		GLuint count, GLint x, GLint y, void *values)
{
   void *src = rb->GetPointer(ctx, rb, x, y);
   memcpy(values, src, count * _mesa_get_format_bytes(rb->Format));
}

/* Only used for float textures currently, but might also be used for
 * RGBA8888, RGBA16, etc.
 */
static void
get_values_generic(struct gl_context *ctx, struct gl_renderbuffer *rb,
		   GLuint count, const GLint x[], const GLint y[], void *values)
{
   int format_bytes = _mesa_get_format_bytes(rb->Format) / sizeof(GLfloat);
   GLuint i;

   for (i = 0; i < count; i++) {
      const void *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      char *dst = (char *) values + i * format_bytes;
      memcpy(dst, src, format_bytes);
   }
}

/* For the GL_RED/GL_RG/GL_RGB format/DataType combinations (and
 * GL_LUMINANCE/GL_INTENSITY?), the Put functions are a matter of
 * storing those initial components of the value per pixel into the
 * destination.
 */
static void
put_row_generic(struct gl_context *ctx, struct gl_renderbuffer *rb,
		GLuint count, GLint x, GLint y,
		const void *values, const GLubyte *mask)
{
   void *row = rb->GetPointer(ctx, rb, x, y);
   int format_bytes = _mesa_get_format_bytes(rb->Format) / sizeof(GLfloat);
   int datatype_bytes = get_datatype_bytes(rb);
   unsigned int i;

   if (mask) {
      for (i = 0; i < count; i++) {
	 char *dst = (char *) row + i * format_bytes;
	 const char *src = (const char *) values + i * datatype_bytes;

         if (mask[i]) {
	    memcpy(dst, src, format_bytes);
         }
      }
   }
   else {
      for (i = 0; i < count; i++) {
	 char *dst = (char *) row + i * format_bytes;
	 const char *src = (const char *) values + i * datatype_bytes;
	 memcpy(dst, src, format_bytes);
      }
   }
}


static void
put_values_generic(struct gl_context *ctx, struct gl_renderbuffer *rb,
		   GLuint count, const GLint x[], const GLint y[],
		   const void *values, const GLubyte *mask)
{
   int format_bytes = _mesa_get_format_bytes(rb->Format) / sizeof(GLfloat);
   int datatype_bytes = get_datatype_bytes(rb);
   unsigned int i;

   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
	 void *dst = rb->GetPointer(ctx, rb, x[i], y[i]);
	 const char *src = (const char *) values + i * datatype_bytes;
	 memcpy(dst, src, format_bytes);
      }
   }
}



/**********************************************************************
 * Functions for buffers of 1 X GLubyte values.
 * Typically stencil.
 */

static void
get_values_ubyte(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                 const GLint x[], const GLint y[], void *values)
{
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      const GLubyte *src = (GLubyte *) rb->Data + y[i] * rb->RowStride + x[i];
      dst[i] = *src;
   }
}


static void
put_row_ubyte(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
              GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + y * rb->RowStride + x;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      memcpy(dst, values, count * sizeof(GLubyte));
   }
}


static void
put_values_ubyte(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                 const GLint x[], const GLint y[],
                 const void *values, const GLubyte *mask)
{
   const GLubyte *src = (const GLubyte *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + y[i] * rb->RowStride + x[i];
         *dst = src[i];
      }
   }
}


/**********************************************************************
 * Functions for buffers of 1 X GLushort values.
 * Typically depth/Z.
 */

static void
get_values_ushort(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < count; i++) {
      const GLushort *src = (GLushort *) rb->Data + y[i] * rb->RowStride + x[i];
      dst[i] = *src;
   }
}


static void
put_row_ushort(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLushort *dst = (GLushort *) rb->Data + y * rb->RowStride + x;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      memcpy(dst, src, count * sizeof(GLushort));
   }
}


static void
put_values_ushort(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLushort *dst = (GLushort *) rb->Data + y[i] * rb->RowStride + x[i];
         *dst = src[i];
      }
   }
}
 

/**********************************************************************
 * Functions for buffers of 1 X GLuint values.
 * Typically depth/Z or color index.
 */

static void
get_values_uint(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                const GLint x[], const GLint y[], void *values)
{
   GLuint *dst = (GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   for (i = 0; i < count; i++) {
      const GLuint *src = (GLuint *) rb->Data + y[i] * rb->RowStride + x[i];
      dst[i] = *src;
   }
}


static void
put_row_uint(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
             GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLuint *src = (const GLuint *) values;
   GLuint *dst = (GLuint *) rb->Data + y * rb->RowStride + x;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      memcpy(dst, src, count * sizeof(GLuint));
   }
}


static void
put_values_uint(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                const GLint x[], const GLint y[], const void *values,
                const GLubyte *mask)
{
   const GLuint *src = (const GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_INT ||
          rb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + y[i] * rb->RowStride + x[i];
         *dst = src[i];
      }
   }
}


/**********************************************************************
 * Functions for buffers of 3 X GLubyte (or GLbyte) values.
 * Typically color buffers.
 * NOTE: the incoming and outgoing colors are RGBA!  We ignore incoming
 * alpha values and return 255 for outgoing alpha values.
 */

static void *
get_pointer_ubyte3(struct gl_context *ctx, struct gl_renderbuffer *rb,
                   GLint x, GLint y)
{
   ASSERT(rb->Format == MESA_FORMAT_RGB888);
   /* No direct access since this buffer is RGB but caller will be
    * treating it as if it were RGBA.
    */
   return NULL;
}


static void
get_row_ubyte3(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, void *values)
{
   const GLubyte *src = ((const GLubyte *) rb->Data) +
					   3 * (y * rb->RowStride + x);
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->Format == MESA_FORMAT_RGB888);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      dst[i * 4 + 0] = src[i * 3 + 0];
      dst[i * 4 + 1] = src[i * 3 + 1];
      dst[i * 4 + 2] = src[i * 3 + 2];
      dst[i * 4 + 3] = 255;
   }
}


static void
get_values_ubyte3(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   GLubyte *dst = (GLubyte *) values;
   GLuint i;
   ASSERT(rb->Format == MESA_FORMAT_RGB888);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      const GLubyte *src
         = (GLubyte *) rb->Data + 3 * (y[i] * rb->RowStride + x[i]);
      dst[i * 4 + 0] = src[0];
      dst[i * 4 + 1] = src[1];
      dst[i * 4 + 2] = src[2];
      dst[i * 4 + 3] = 255;
   }
}


static void
put_row_ubyte3(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* note: incoming values are RGB+A! */
   const GLubyte *src = (const GLubyte *) values;
   GLubyte *dst = (GLubyte *) rb->Data + 3 * (y * rb->RowStride + x);
   GLuint i;
   ASSERT(rb->Format == MESA_FORMAT_RGB888);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         dst[i * 3 + 0] = src[i * 4 + 0];
         dst[i * 3 + 1] = src[i * 4 + 1];
         dst[i * 3 + 2] = src[i * 4 + 2];
      }
   }
}


static void
put_values_ubyte3(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   /* note: incoming values are RGB+A! */
   const GLubyte *src = (const GLubyte *) values;
   GLuint i;
   ASSERT(rb->Format == MESA_FORMAT_RGB888);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLubyte *dst = (GLubyte *) rb->Data + 3 * (y[i] * rb->RowStride + x[i]);
         dst[0] = src[i * 4 + 0];
         dst[1] = src[i * 4 + 1];
         dst[2] = src[i * 4 + 2];
      }
   }
}


/**********************************************************************
 * Functions for buffers of 4 X GLubyte (or GLbyte) values.
 * Typically color buffers.
 */

static void
get_values_ubyte4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], void *values)
{
   /* treat 4*GLubyte as 1*GLuint */
   GLuint *dst = (GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->Format == MESA_FORMAT_RGBA8888 ||
          rb->Format == MESA_FORMAT_RGBA8888_REV);
   for (i = 0; i < count; i++) {
      const GLuint *src = (GLuint *) rb->Data + (y[i] * rb->RowStride + x[i]);
      dst[i] = *src;
   }
}


static void
put_row_ubyte4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
               GLint x, GLint y, const void *values, const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint *src = (const GLuint *) values;
   GLuint *dst = (GLuint *) rb->Data + (y * rb->RowStride + x);
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->Format == MESA_FORMAT_RGBA8888 ||
          rb->Format == MESA_FORMAT_RGBA8888_REV);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i] = src[i];
         }
      }
   }
   else {
      memcpy(dst, src, 4 * count * sizeof(GLubyte));
   }
}


static void
put_values_ubyte4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                  const GLint x[], const GLint y[], const void *values,
                  const GLubyte *mask)
{
   /* treat 4*GLubyte as 1*GLuint */
   const GLuint *src = (const GLuint *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(rb->Format == MESA_FORMAT_RGBA8888 ||
          rb->Format == MESA_FORMAT_RGBA8888_REV);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLuint *dst = (GLuint *) rb->Data + (y[i] * rb->RowStride + x[i]);
         *dst = src[i];
      }
   }
}


/**********************************************************************
 * Functions for buffers of 4 X GLushort (or GLshort) values.
 * Typically accum buffer.
 */

static void
get_values_ushort4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = (GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   for (i = 0; i < count; i++) {
      const GLushort *src
         = (GLushort *) rb->Data + 4 * (y[i] * rb->RowStride + x[i]);
      dst[i] = *src;
   }
}


static void
put_row_ushort4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLushort *dst = (GLushort *) rb->Data + 4 * (y * rb->RowStride + x);
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   if (mask) {
      GLuint i;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            dst[i * 4 + 0] = src[i * 4 + 0];
            dst[i * 4 + 1] = src[i * 4 + 1];
            dst[i * 4 + 2] = src[i * 4 + 2];
            dst[i * 4 + 3] = src[i * 4 + 3];
         }
      }
   }
   else {
      memcpy(dst, src, 4 * count * sizeof(GLushort));
   }
}


static void
put_values_ushort4(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], const void *values,
                   const GLubyte *mask)
{
   const GLushort *src = (const GLushort *) values;
   GLuint i;
   ASSERT(rb->DataType == GL_UNSIGNED_SHORT || rb->DataType == GL_SHORT);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         GLushort *dst =
            ((GLushort *) rb->Data) + 4 * (y[i] * rb->RowStride + x[i]);
         dst[0] = src[i * 4 + 0];
         dst[1] = src[i * 4 + 1];
         dst[2] = src[i * 4 + 2];
         dst[3] = src[i * 4 + 3];
      }
   }
}


/**********************************************************************
 * Functions for MESA_FORMAT_R8.
 */
static void
get_row_r8(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	   GLint x, GLint y, void *values)
{
   const GLubyte *src = rb->GetPointer(ctx, rb, x, y);
   GLuint *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i] = 0xff000000 | src[i];
   }
}

static void
get_values_r8(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	      const GLint x[], const GLint y[], void *values)
{
   GLuint *dst = (GLuint *) values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLubyte *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i] = 0xff000000 | *src;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_GR88.
 */
static void
get_row_rg88(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	     GLint x, GLint y, void *values)
{
   const GLushort *src = rb->GetPointer(ctx, rb, x, y);
   GLuint *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i] = 0xff000000 | src[i];
   }
}

static void
get_values_rg88(struct gl_context *ctx, struct gl_renderbuffer *rb,
		GLuint count, const GLint x[], const GLint y[], void *values)
{
   GLuint *dst = (GLuint *) values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLshort *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i] = 0xff000000 | *src;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_R16.
 */
static void
get_row_r16(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	    GLint x, GLint y, void *values)
{
   const GLushort *src = rb->GetPointer(ctx, rb, x, y);
   GLushort *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] = src[i];
      dst[i * 4 + GCOMP] = 0;
      dst[i * 4 + BCOMP] = 0;
      dst[i * 4 + ACOMP] = 0xffff;
   }
}

static void
get_values_r16(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	       const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLushort *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] = *src;
      dst[i * 4 + GCOMP] = 0;
      dst[i * 4 + BCOMP] = 0;
      dst[i * 4 + ACOMP] = 0xffff;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_RG1616.
 */
static void
get_row_rg1616(struct gl_context *ctx, struct gl_renderbuffer *rb, GLuint count,
	       GLint x, GLint y, void *values)
{
   const GLushort *src = rb->GetPointer(ctx, rb, x, y);
   GLushort *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] = src[i * 2];
      dst[i * 4 + GCOMP] = src[i * 2 + 1];
      dst[i * 4 + BCOMP] = 0;
      dst[i * 4 + ACOMP] = 0xffff;
   }
}

static void
get_values_rg1616(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, const GLint x[], const GLint y[], void *values)
{
   GLushort *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLshort *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] = src[0];
      dst[i * 4 + GCOMP] = src[1];
      dst[i * 4 + BCOMP] = 0;
      dst[i * 4 + ACOMP] = 0xffff;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_INTENSITY_FLOAT32.
 */
static void
get_row_i_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, GLint x, GLint y, void *values)
{
   const GLfloat *src = rb->GetPointer(ctx, rb, x, y);
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] =
      dst[i * 4 + GCOMP] =
      dst[i * 4 + BCOMP] =
      dst[i * 4 + ACOMP] = src[i];
   }
}

static void
get_values_i_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		     GLuint count, const GLint x[], const GLint y[],
		     void *values)
{
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLfloat *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] =
      dst[i * 4 + GCOMP] =
      dst[i * 4 + BCOMP] =
      dst[i * 4 + ACOMP] = src[0];
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_LUMINANCE_FLOAT32.
 */
static void
get_row_l_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, GLint x, GLint y, void *values)
{
   const GLfloat *src = rb->GetPointer(ctx, rb, x, y);
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] =
      dst[i * 4 + GCOMP] =
      dst[i * 4 + BCOMP] = src[i];
      dst[i * 4 + ACOMP] = 1.0;
   }
}

static void
get_values_l_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		     GLuint count, const GLint x[], const GLint y[],
		     void *values)
{
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLfloat *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] =
      dst[i * 4 + GCOMP] =
      dst[i * 4 + BCOMP] = src[0];
      dst[i * 4 + ACOMP] = 1.0;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_ALPHA_FLOAT32.
 */
static void
get_row_a_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, GLint x, GLint y, void *values)
{
   const GLfloat *src = rb->GetPointer(ctx, rb, x, y);
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] = 0.0;
      dst[i * 4 + GCOMP] = 0.0;
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = src[i];
   }
}

static void
get_values_a_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		     GLuint count, const GLint x[], const GLint y[],
		     void *values)
{
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLfloat *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] = 0.0;
      dst[i * 4 + GCOMP] = 0.0;
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = src[0];
   }
}

static void
put_row_a_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, GLint x, GLint y,
		  const void *values, const GLubyte *mask)
{
   float *dst = rb->GetPointer(ctx, rb, x, y);
   const float *src = values;
   unsigned int i;

   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
	    dst[i] = src[i * 4 + ACOMP];
         }
      }
   }
   else {
      for (i = 0; i < count; i++) {
	 dst[i] = src[i * 4 + ACOMP];
      }
   }
}

static void
put_values_a_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		     GLuint count, const GLint x[], const GLint y[],
		     const void *values, const GLubyte *mask)
{
   const float *src = values;
   unsigned int i;

   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
	 float *dst = rb->GetPointer(ctx, rb, x[i], y[i]);

	 *dst = src[i * 4 + ACOMP];
      }
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_R_FLOAT32.
 */
static void
get_row_r_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		  GLuint count, GLint x, GLint y, void *values)
{
   const GLfloat *src = rb->GetPointer(ctx, rb, x, y);
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] = src[i];
      dst[i * 4 + GCOMP] = 0.0;
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = 1.0;
   }
}

static void
get_values_r_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		     GLuint count, const GLint x[], const GLint y[],
		     void *values)
{
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLfloat *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] = src[0];
      dst[i * 4 + GCOMP] = 0.0;
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = 1.0;
   }
}

/**********************************************************************
 * Functions for MESA_FORMAT_RG_FLOAT32.
 */
static void
get_row_rg_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		   GLuint count, GLint x, GLint y, void *values)
{
   const GLfloat *src = rb->GetPointer(ctx, rb, x, y);
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      dst[i * 4 + RCOMP] = src[i * 2 + 0];
      dst[i * 4 + GCOMP] = src[i * 2 + 1];
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = 1.0;
   }
}

static void
get_values_rg_float32(struct gl_context *ctx, struct gl_renderbuffer *rb,
		      GLuint count, const GLint x[], const GLint y[],
		      void *values)
{
   GLfloat *dst = values;
   GLuint i;

   for (i = 0; i < count; i++) {
      const GLfloat *src = rb->GetPointer(ctx, rb, x[i], y[i]);
      dst[i * 4 + RCOMP] = src[0];
      dst[i * 4 + GCOMP] = src[1];
      dst[i * 4 + BCOMP] = 0.0;
      dst[i * 4 + ACOMP] = 1.0;
   }
}

/**
 * This is the default software fallback for gl_renderbuffer's span
 * access functions.
 *
 * The assumptions are that rb->Data will be a pointer to (0,0), that pixels
 * are packed in the type of rb->Format, and that subsequent rows appear
 * rb->RowStride pixels later.
 */
void
_swrast_set_renderbuffer_accessors(struct gl_renderbuffer *rb)
{
   rb->GetPointer = get_pointer_generic;
   rb->GetRow = get_row_generic;

   switch (rb->Format) {
   case MESA_FORMAT_RGB888:
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetPointer = get_pointer_ubyte3;
      rb->GetRow = get_row_ubyte3;
      rb->GetValues = get_values_ubyte3;
      rb->PutRow = put_row_ubyte3;
      rb->PutValues = put_values_ubyte3;
      break;

   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_RGBA8888_REV:
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetValues = get_values_ubyte4;
      rb->PutRow = put_row_ubyte4;
      rb->PutValues = put_values_ubyte4;
      break;

   case MESA_FORMAT_R8:
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetValues = get_values_r8;
      rb->GetRow = get_row_r8;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_GR88:
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetValues = get_values_rg88;
      rb->GetRow = get_row_rg88;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_R16:
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetValues = get_values_r16;
      rb->GetRow = get_row_r16;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_RG1616:
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetValues = get_values_rg1616;
      rb->GetRow = get_row_rg1616;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_SIGNED_RGBA_16:
      rb->DataType = GL_SHORT;
      rb->GetValues = get_values_ushort4;
      rb->PutRow = put_row_ushort4;
      rb->PutValues = put_values_ushort4;
      break;

   case MESA_FORMAT_S8:
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->GetValues = get_values_ubyte;
      rb->PutRow = put_row_ubyte;
      rb->PutValues = put_values_ubyte;
      break;

   case MESA_FORMAT_Z16:
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->GetValues = get_values_ushort;
      rb->PutRow = put_row_ushort;
      rb->PutValues = put_values_ushort;
      break;

   case MESA_FORMAT_Z32:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_Z24_X8:
      rb->DataType = GL_UNSIGNED_INT;
      rb->GetValues = get_values_uint;
      rb->PutRow = put_row_uint;
      rb->PutValues = put_values_uint;
      break;

   case MESA_FORMAT_Z24_S8:
   case MESA_FORMAT_S8_Z24:
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->GetValues = get_values_uint;
      rb->PutRow = put_row_uint;
      rb->PutValues = put_values_uint;
      break;

   case MESA_FORMAT_RGBA_FLOAT32:
      rb->GetRow = get_row_generic;
      rb->GetValues = get_values_generic;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_INTENSITY_FLOAT32:
      rb->GetRow = get_row_i_float32;
      rb->GetValues = get_values_i_float32;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_LUMINANCE_FLOAT32:
      rb->GetRow = get_row_l_float32;
      rb->GetValues = get_values_l_float32;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_ALPHA_FLOAT32:
      rb->GetRow = get_row_a_float32;
      rb->GetValues = get_values_a_float32;
      rb->PutRow = put_row_a_float32;
      rb->PutValues = put_values_a_float32;
      break;

   case MESA_FORMAT_RG_FLOAT32:
      rb->GetRow = get_row_rg_float32;
      rb->GetValues = get_values_rg_float32;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   case MESA_FORMAT_R_FLOAT32:
      rb->GetRow = get_row_r_float32;
      rb->GetValues = get_values_r_float32;
      rb->PutRow = put_row_generic;
      rb->PutValues = put_values_generic;
      break;

   default:
      break;
   }
}

/**
 * This is a software fallback for the gl_renderbuffer->AllocStorage
 * function.
 * Device drivers will typically override this function for the buffers
 * which it manages (typically color buffers, Z and stencil).
 * Other buffers (like software accumulation and aux buffers) which the driver
 * doesn't manage can be handled with this function.
 *
 * This one multi-purpose function can allocate stencil, depth, accum, color
 * or color-index buffers!
 *
 * This function also plugs in the appropriate GetPointer, Get/PutRow and
 * Get/PutValues functions.
 */
static GLboolean
soft_renderbuffer_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          GLenum internalFormat,
                          GLuint width, GLuint height)
{
   switch (internalFormat) {
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->Format = MESA_FORMAT_RGB888;
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
#if 1
   case GL_RGB10_A2:
   case GL_RGBA12:
#endif
      if (_mesa_little_endian())
         rb->Format = MESA_FORMAT_RGBA8888_REV;
      else
         rb->Format = MESA_FORMAT_RGBA8888;
      break;
   case GL_RGBA16:
   case GL_RGBA16_SNORM:
      /* for accum buffer */
      rb->Format = MESA_FORMAT_SIGNED_RGBA_16;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      rb->Format = MESA_FORMAT_S8;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
      rb->Format = MESA_FORMAT_Z16;
      break;
   case GL_DEPTH_COMPONENT24:
      rb->Format = MESA_FORMAT_X8_Z24;
      break;
   case GL_DEPTH_COMPONENT32:
      rb->Format = MESA_FORMAT_Z32;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->Format = MESA_FORMAT_Z24_S8;
      break;
   default:
      /* unsupported format */
      return GL_FALSE;
   }

   _swrast_set_renderbuffer_accessors(rb);

   ASSERT(rb->DataType);
   ASSERT(rb->GetPointer);
   ASSERT(rb->GetRow);
   ASSERT(rb->GetValues);
   ASSERT(rb->PutRow);
   ASSERT(rb->PutValues);

   /* free old buffer storage */
   if (rb->Data) {
      free(rb->Data);
      rb->Data = NULL;
   }

   rb->RowStride = width;

   if (width > 0 && height > 0) {
      /* allocate new buffer storage */
      rb->Data = malloc(width * height * _mesa_get_format_bytes(rb->Format));

      if (rb->Data == NULL) {
         rb->Width = 0;
         rb->Height = 0;
	 rb->RowStride = 0;
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "software renderbuffer allocation (%d x %d x %d)",
                     width, height, _mesa_get_format_bytes(rb->Format));
         return GL_FALSE;
      }
   }

   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fbo_format(ctx, internalFormat);

   if (rb->Name == 0 &&
       internalFormat == GL_RGBA16_SNORM &&
       rb->_BaseFormat == 0) {
      /* NOTE: This is a special case just for accumulation buffers.
       * This is a very limited use case- there's no snorm texturing or
       * rendering going on.
       */
      rb->_BaseFormat = GL_RGBA;
   }
   else {
      /* the internalFormat should have been error checked long ago */
      ASSERT(rb->_BaseFormat);
   }

   return GL_TRUE;
}


void
_swrast_map_soft_renderbuffer(struct gl_context *ctx,
                              struct gl_renderbuffer *rb,
                              GLuint x, GLuint y, GLuint w, GLuint h,
                              GLbitfield mode,
                              GLubyte **out_map,
                              GLint *out_stride)
{
   GLubyte *map = rb->Data;
   int cpp = _mesa_get_format_bytes(rb->Format);
   int stride = rb->RowStride * cpp;

   ASSERT(rb->Data);

   map += y * stride;
   map += x * cpp;

   *out_map = map;
   *out_stride = stride;
}


void
_swrast_unmap_soft_renderbuffer(struct gl_context *ctx,
                                struct gl_renderbuffer *rb)
{
}



/**
 * Allocate a software-based renderbuffer.  This is called via the
 * ctx->Driver.NewRenderbuffer() function when the user creates a new
 * renderbuffer.
 * This would not be used for hardware-based renderbuffers.
 */
struct gl_renderbuffer *
_swrast_new_soft_renderbuffer(struct gl_context *ctx, GLuint name)
{
   struct gl_renderbuffer *rb = _mesa_new_renderbuffer(ctx, name);
   if (rb) {
      rb->AllocStorage = soft_renderbuffer_storage;
      /* Normally, one would setup the PutRow, GetRow, etc functions here.
       * But we're doing that in the soft_renderbuffer_storage() function
       * instead.
       */
   }
   return rb;
}


/**
 * Add software-based color renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_color_renderbuffers(struct gl_context *ctx, struct gl_framebuffer *fb,
                        GLuint rgbBits, GLuint alphaBits,
                        GLboolean frontLeft, GLboolean backLeft,
                        GLboolean frontRight, GLboolean backRight)
{
   gl_buffer_index b;

   if (rgbBits > 16 || alphaBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported bit depth in add_color_renderbuffers");
      return GL_FALSE;
   }

   assert(MAX_COLOR_ATTACHMENTS >= 4);

   for (b = BUFFER_FRONT_LEFT; b <= BUFFER_BACK_RIGHT; b++) {
      struct gl_renderbuffer *rb;

      if (b == BUFFER_FRONT_LEFT && !frontLeft)
         continue;
      else if (b == BUFFER_BACK_LEFT && !backLeft)
         continue;
      else if (b == BUFFER_FRONT_RIGHT && !frontRight)
         continue;
      else if (b == BUFFER_BACK_RIGHT && !backRight)
         continue;

      assert(fb->Attachment[b].Renderbuffer == NULL);

      rb = _mesa_new_renderbuffer(ctx, 0);
      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating color buffer");
         return GL_FALSE;
      }

      rb->InternalFormat = GL_RGBA;

      rb->AllocStorage = soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, b, rb);
   }

   return GL_TRUE;
}


/**
 * Add a software-based depth renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_depth_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                       GLuint depthBits)
{
   struct gl_renderbuffer *rb;

   if (depthBits > 32) {
      _mesa_problem(ctx,
                    "Unsupported depthBits in add_depth_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_DEPTH].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating depth buffer");
      return GL_FALSE;
   }

   if (depthBits <= 16) {
      rb->InternalFormat = GL_DEPTH_COMPONENT16;
   }
   else if (depthBits <= 24) {
      rb->InternalFormat = GL_DEPTH_COMPONENT24;
   }
   else {
      rb->InternalFormat = GL_DEPTH_COMPONENT32;
   }

   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);

   return GL_TRUE;
}


/**
 * Add a software-based stencil renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_stencil_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                         GLuint stencilBits)
{
   struct gl_renderbuffer *rb;

   if (stencilBits > 16) {
      _mesa_problem(ctx,
                  "Unsupported stencilBits in add_stencil_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_STENCIL].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating stencil buffer");
      return GL_FALSE;
   }

   assert(stencilBits <= 8);
   rb->InternalFormat = GL_STENCIL_INDEX8;

   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_STENCIL, rb);

   return GL_TRUE;
}


static GLboolean
add_depth_stencil_renderbuffer(struct gl_context *ctx,
                               struct gl_framebuffer *fb)
{
   struct gl_renderbuffer *rb;

   assert(fb->Attachment[BUFFER_DEPTH].Renderbuffer == NULL);
   assert(fb->Attachment[BUFFER_STENCIL].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating depth+stencil buffer");
      return GL_FALSE;
   }

   rb->InternalFormat = GL_DEPTH_STENCIL;

   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);
   _mesa_add_renderbuffer(fb, BUFFER_STENCIL, rb);

   return GL_TRUE;
}


/**
 * Add a software-based accumulation renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_accum_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                       GLuint redBits, GLuint greenBits,
                       GLuint blueBits, GLuint alphaBits)
{
   struct gl_renderbuffer *rb;

   if (redBits > 16 || greenBits > 16 || blueBits > 16 || alphaBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported accumBits in add_accum_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_ACCUM].Renderbuffer == NULL);

   rb = _mesa_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating accum buffer");
      return GL_FALSE;
   }

   rb->InternalFormat = GL_RGBA16_SNORM;
   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_ACCUM, rb);

   return GL_TRUE;
}



/**
 * Add a software-based aux renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 *
 * NOTE: color-index aux buffers not supported.
 */
static GLboolean
add_aux_renderbuffers(struct gl_context *ctx, struct gl_framebuffer *fb,
                      GLuint colorBits, GLuint numBuffers)
{
   GLuint i;

   if (colorBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported colorBits in add_aux_renderbuffers");
      return GL_FALSE;
   }

   assert(numBuffers <= MAX_AUX_BUFFERS);

   for (i = 0; i < numBuffers; i++) {
      struct gl_renderbuffer *rb = _mesa_new_renderbuffer(ctx, 0);

      assert(fb->Attachment[BUFFER_AUX0 + i].Renderbuffer == NULL);

      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating aux buffer");
         return GL_FALSE;
      }

      assert (colorBits <= 8);
      rb->InternalFormat = GL_RGBA;

      rb->AllocStorage = soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, BUFFER_AUX0 + i, rb);
   }
   return GL_TRUE;
}


/**
 * Create/attach software-based renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers.  Drivers can just as well
 * call the individual _mesa_add_*_renderbuffer() routines directly.
 */
void
_swrast_add_soft_renderbuffers(struct gl_framebuffer *fb,
                               GLboolean color,
                               GLboolean depth,
                               GLboolean stencil,
                               GLboolean accum,
                               GLboolean alpha,
                               GLboolean aux)
{
   GLboolean frontLeft = GL_TRUE;
   GLboolean backLeft = fb->Visual.doubleBufferMode;
   GLboolean frontRight = fb->Visual.stereoMode;
   GLboolean backRight = fb->Visual.stereoMode && fb->Visual.doubleBufferMode;

   if (color) {
      assert(fb->Visual.redBits == fb->Visual.greenBits);
      assert(fb->Visual.redBits == fb->Visual.blueBits);
      add_color_renderbuffers(NULL, fb,
                              fb->Visual.redBits,
                              fb->Visual.alphaBits,
                              frontLeft, backLeft,
                              frontRight, backRight);
   }

#if 0
   /* This is pretty much for debugging purposes only since there's a perf
    * hit for using combined depth/stencil in swrast.
    */
   if (depth && fb->Visual.depthBits == 24 &&
       stencil && fb->Visual.stencilBits == 8) {
      /* use combined depth/stencil buffer */
      add_depth_stencil_renderbuffer(NULL, fb);
   }
   else
#else
   (void) add_depth_stencil_renderbuffer;
#endif
   {
      if (depth) {
         assert(fb->Visual.depthBits > 0);
         add_depth_renderbuffer(NULL, fb, fb->Visual.depthBits);
      }

      if (stencil) {
         assert(fb->Visual.stencilBits > 0);
         add_stencil_renderbuffer(NULL, fb, fb->Visual.stencilBits);
      }
   }

   if (accum) {
      assert(fb->Visual.accumRedBits > 0);
      assert(fb->Visual.accumGreenBits > 0);
      assert(fb->Visual.accumBlueBits > 0);
      add_accum_renderbuffer(NULL, fb,
                             fb->Visual.accumRedBits,
                             fb->Visual.accumGreenBits,
                             fb->Visual.accumBlueBits,
                             fb->Visual.accumAlphaBits);
   }

   if (aux) {
      assert(fb->Visual.numAuxBuffers > 0);
      add_aux_renderbuffers(NULL, fb, fb->Visual.redBits,
                            fb->Visual.numAuxBuffers);
   }

#if 0
   if (multisample) {
      /* maybe someday */
   }
#endif
}
