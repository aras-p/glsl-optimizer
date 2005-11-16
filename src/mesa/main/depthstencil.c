/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "fbobject.h"
#include "mtypes.h"
#include "depthstencil.h"
#include "renderbuffer.h"


/**
 * Adaptor/wrappers for GL_DEPTH_STENCIL renderbuffers.
 *
 * The problem with a GL_DEPTH_STENCIL renderbuffer is that sometimes we
 * want to treat it as a stencil buffer, other times we want to treat it
 * as a depth/z buffer and still other times when we want to treat it as
 * a combined Z+stencil buffer!  That implies we need three different sets
 * of Get/Put functions.
 *
 * We solve this by wrapping the Z24_S8 renderbuffer with depth and stencil
 * adaptors, each with the right kind of depth/stencil Get/Put functions.
 */


static void *
nop_get_pointer(GLcontext *ctx, struct gl_renderbuffer *rb, GLint x, GLint y)
{
   return NULL;
}


/**
 * Delete a depth or stencil renderbuffer.
 */
static void
delete_wrapper(struct gl_renderbuffer *rb)
{
   struct gl_renderbuffer *dsrb = rb->Wrapped;
   assert(dsrb);
   assert(rb->InternalFormat == GL_DEPTH_COMPONENT24 ||
          rb->InternalFormat == GL_STENCIL_INDEX8_EXT);
   /* decrement refcount on the wrapped buffer and delete it if necessary */
   dsrb->RefCount--;
   if (dsrb->RefCount <= 0) {
      dsrb->Delete(dsrb);
   }
   _mesa_free(rb);
}


/*======================================================================
 * Depth wrapper around depth/stencil renderbuffer
 */

static void
get_row_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb, GLuint count,
            GLint x, GLint y, void *values)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   GLuint *dst = (GLuint *) values;
   const GLuint *src = (const GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(z24rb->DataType == GL_UNSIGNED_INT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (!src) {
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      src = temp;
   }
   for (i = 0; i < count; i++) {
      dst[i] = src[i] >> 8;
   }
}

static void
get_values_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb, GLuint count,
               const GLint x[], const GLint y[], void *values)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   GLuint *dst = (GLuint *) values;
   ASSERT(z24rb->DataType == GL_UNSIGNED_INT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   ASSERT(count <= MAX_WIDTH);
   /* don't bother trying direct access */
   dsrb->GetValues(ctx, dsrb, count, x, y, temp);
   for (i = 0; i < count; i++) {
      dst[i] = temp[i] >> 8;
   }
}

static void
put_row_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb, GLuint count,
            GLint x, GLint y, const void *values, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   const GLuint *src = (const GLuint *) values;
   GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(z24rb->DataType == GL_UNSIGNED_INT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dst) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i] = (src[i] << 8) | (dst[i] & 0xff);
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = (src[i] << 8) | (temp[i] & 0xff);
         }
      }
      dsrb->PutRow(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_mono_row_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb, GLuint count,
                 GLint x, GLint y, const void *value, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   const GLuint shiftedVal = *((GLuint *) value) << 8;
   GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(z24rb->DataType == GL_UNSIGNED_INT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dst) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i] = shiftedVal | (dst[i] & 0xff);
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = shiftedVal | (temp[i] & 0xff);
         }
      }
      dsrb->PutRow(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_values_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb, GLuint count,
               const GLint x[], const GLint y[],
               const void *values, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   const GLubyte *src = (const GLubyte *) values;
   ASSERT(z24rb->DataType == GL_UNSIGNED_INT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dsrb->GetPointer(ctx, dsrb, 0, 0)) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x[i], y[i]);
            *dst = (src[i] << 8) | (*dst & 0xff);
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetValues(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = (src[i] << 8) | (temp[i] & 0xff);
         }
      }
      dsrb->PutValues(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_mono_values_z24(GLcontext *ctx, struct gl_renderbuffer *z24rb,
                    GLuint count, const GLint x[], const GLint y[],
                    const void *value, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = z24rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   const GLuint shiftedVal = *((GLuint *) value) << 8;
   /* get, modify, put */
   dsrb->GetValues(ctx, dsrb, count, x, y, temp);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         temp[i] = shiftedVal | (temp[i] & 0xff);
      }
   }
   dsrb->PutValues(ctx, dsrb, count, x, y, temp, mask);
}


/**
 * Wrap the given GL_DEPTH_STENCIL renderbuffer so that it acts like
 * a depth renderbuffer.
 * \return new depth renderbuffer
 */
struct gl_renderbuffer *
_mesa_new_z24_renderbuffer_wrapper(GLcontext *ctx,
                                   struct gl_renderbuffer *dsrb)
{
   struct gl_renderbuffer *z24rb;

   ASSERT(dsrb->_BaseFormat == GL_DEPTH_STENCIL_EXT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);

   z24rb = _mesa_new_renderbuffer(ctx, 0);
   if (!z24rb)
      return NULL;

   z24rb->Wrapped = dsrb;
   z24rb->Name = dsrb->Name;
   z24rb->RefCount = 1;
   z24rb->Width = dsrb->Width;
   z24rb->Height = dsrb->Height;
   z24rb->InternalFormat = GL_DEPTH_COMPONENT24_ARB;
   z24rb->_BaseFormat = GL_DEPTH_COMPONENT;
   z24rb->DataType = GL_UNSIGNED_INT;
   z24rb->DepthBits = 24;
   z24rb->Data = NULL;
   z24rb->Delete = delete_wrapper;
   z24rb->GetPointer = nop_get_pointer;
   z24rb->GetRow = get_row_z24;
   z24rb->GetValues = get_values_z24;
   z24rb->PutRow = put_row_z24;
   z24rb->PutRowRGB = NULL;
   z24rb->PutMonoRow = put_mono_row_z24;
   z24rb->PutValues = put_values_z24;
   z24rb->PutMonoValues = put_mono_values_z24;

   return z24rb;
}


/*======================================================================
 * Stencil wrapper around depth/stencil renderbuffer
 */

static void
get_row_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
           GLint x, GLint y, void *values)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   GLubyte *dst = (GLubyte *) values;
   const GLuint *src = (const GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(s8rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (!src) {
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      src = temp;
   }
   for (i = 0; i < count; i++) {
      dst[i] = src[i] & 0xff;
   }
}

static void
get_values_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
              const GLint x[], const GLint y[], void *values)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   GLubyte *dst = (GLubyte *) values;
   ASSERT(s8rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   ASSERT(count <= MAX_WIDTH);
   /* don't bother trying direct access */
   dsrb->GetValues(ctx, dsrb, count, x, y, temp);
   for (i = 0; i < count; i++) {
      dst[i] = temp[i] & 0xff;
   }
}

static void
put_row_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
           GLint x, GLint y, const void *values, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   const GLubyte *src = (const GLubyte *) values;
   GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(s8rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dst) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i] = (dst[i] & 0xffffff00) | src[i];
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = (temp[i] & 0xffffff00) | src[i];
         }
      }
      dsrb->PutRow(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_mono_row_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
                GLint x, GLint y, const void *value, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   const GLubyte val = *((GLubyte *) value);
   GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x, y);
   ASSERT(s8rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dst) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            dst[i] = (dst[i] & 0xffffff00) | val;
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetRow(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = (temp[i] & 0xffffff00) | val;
         }
      }
      dsrb->PutRow(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_values_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
              const GLint x[], const GLint y[],
              const void *values, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   const GLubyte *src = (const GLubyte *) values;
   ASSERT(s8rb->DataType == GL_UNSIGNED_BYTE);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);
   if (dsrb->GetPointer(ctx, dsrb, 0, 0)) {
      /* direct access */
      GLuint i;
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            GLuint *dst = (GLuint *) dsrb->GetPointer(ctx, dsrb, x[i], y[i]);
            *dst = (*dst & 0xffffff00) | src[i];
         }
      }
   }
   else {
      /* get, modify, put */
      GLuint temp[MAX_WIDTH], i;
      dsrb->GetValues(ctx, dsrb, count, x, y, temp);
      for (i = 0; i < count; i++) {
         if (!mask || mask[i]) {
            temp[i] = (temp[i] & 0xffffff00) | src[i];
         }
      }
      dsrb->PutValues(ctx, dsrb, count, x, y, temp, mask);
   }
}

static void
put_mono_values_s8(GLcontext *ctx, struct gl_renderbuffer *s8rb, GLuint count,
                   const GLint x[], const GLint y[],
                   const void *value, const GLubyte *mask)
{
   struct gl_renderbuffer *dsrb = s8rb->Wrapped;
   GLuint temp[MAX_WIDTH], i;
   const GLubyte val = *((GLubyte *) value);
   /* get, modify, put */
   dsrb->GetValues(ctx, dsrb, count, x, y, temp);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         temp[i] = (temp[i] & 0xffffff) | val;
      }
   }
   dsrb->PutValues(ctx, dsrb, count, x, y, temp, mask);
}


/**
 * Wrap the given GL_DEPTH_STENCIL renderbuffer so that it acts like
 * a stencil renderbuffer.
 * \return new stencil renderbuffer
 */
struct gl_renderbuffer *
_mesa_new_s8_renderbuffer_wrapper(GLcontext *ctx, struct gl_renderbuffer *dsrb)
{
   struct gl_renderbuffer *s8rb;

   ASSERT(dsrb->_BaseFormat == GL_DEPTH_STENCIL_EXT);
   ASSERT(dsrb->DataType == GL_UNSIGNED_INT_24_8_EXT);

   s8rb = _mesa_new_renderbuffer(ctx, 0);
   if (!s8rb)
      return NULL;

   s8rb->Wrapped = dsrb;
   s8rb->Name = dsrb->Name;
   s8rb->RefCount = 1;
   s8rb->Width = dsrb->Width;
   s8rb->Height = dsrb->Height;
   s8rb->InternalFormat = GL_STENCIL_INDEX8_EXT;
   s8rb->_BaseFormat = GL_STENCIL_INDEX;
   s8rb->DataType = GL_UNSIGNED_BYTE;
   s8rb->StencilBits = 8;
   s8rb->Data = NULL;
   s8rb->Delete = delete_wrapper;
   s8rb->GetPointer = nop_get_pointer;
   s8rb->GetRow = get_row_s8;
   s8rb->GetValues = get_values_s8;
   s8rb->PutRow = put_row_s8;
   s8rb->PutRowRGB = NULL;
   s8rb->PutMonoRow = put_mono_row_s8;
   s8rb->PutValues = put_values_s8;
   s8rb->PutMonoValues = put_mono_values_s8;

   return s8rb;
}


/**
 * Merge data from a depth renderbuffer and a stencil renderbuffer into a
 * combined depth/stencil renderbuffer.
 */
void
_mesa_merge_depth_stencil_buffers(GLcontext *ctx,
                                  struct gl_renderbuffer *dest,
                                  struct gl_renderbuffer *depth,
                                  struct gl_renderbuffer *stencil)
{
   GLuint depthVals[MAX_WIDTH];
   GLubyte stencilVals[MAX_WIDTH];
   GLuint combined[MAX_WIDTH];
   GLuint row, width;

   ASSERT(dest);
   ASSERT(depth);
   ASSERT(stencil);

   ASSERT(dest->InternalFormat == GL_DEPTH24_STENCIL8_EXT);
   ASSERT(dest->DataType == GL_UNSIGNED_INT_24_8_EXT);
   ASSERT(depth->InternalFormat == GL_DEPTH_COMPONENT24);
   ASSERT(depth->DataType == GL_UNSIGNED_INT);
   ASSERT(stencil->InternalFormat == GL_STENCIL_INDEX8_EXT);
   ASSERT(stencil->DataType == GL_UNSIGNED_BYTE);

   ASSERT(dest->Width == depth->Width);
   ASSERT(dest->Height == depth->Height);
   ASSERT(dest->Width == stencil->Width);
   ASSERT(dest->Height == stencil->Height);

   width = dest->Width;
   for (row = 0; row < dest->Height; row++) {
      GLuint i;
      depth->GetRow(ctx, depth, width, 0, row, depthVals);
      stencil->GetRow(ctx, stencil, width, 0, row, stencilVals);
      for (i = 0; i < width; i++) {
         combined[i] = (depthVals[i] << 8) | stencilVals[i];
      }
      dest->PutRow(ctx, dest, width, 0, row, combined, NULL);
   }
}


/**
 * Split combined depth/stencil renderbuffer data into separate depth/stencil
 * buffers.
 */
void
_mesa_split_depth_stencil_buffer(GLcontext *ctx,
                                 struct gl_renderbuffer *source,
                                 struct gl_renderbuffer *depth,
                                 struct gl_renderbuffer *stencil)
{
   GLuint depthVals[MAX_WIDTH];
   GLubyte stencilVals[MAX_WIDTH];
   GLuint combined[MAX_WIDTH];
   GLuint row, width;

   ASSERT(source);
   ASSERT(depth);
   ASSERT(stencil);

   ASSERT(source->InternalFormat == GL_DEPTH24_STENCIL8_EXT);
   ASSERT(source->DataType == GL_UNSIGNED_INT_24_8_EXT);
   ASSERT(depth->InternalFormat == GL_DEPTH_COMPONENT24);
   ASSERT(depth->DataType == GL_UNSIGNED_INT);
   ASSERT(stencil->InternalFormat == GL_STENCIL_INDEX8_EXT);
   ASSERT(stencil->DataType == GL_UNSIGNED_BYTE);

   ASSERT(source->Width == depth->Width);
   ASSERT(source->Height == depth->Height);
   ASSERT(source->Width == stencil->Width);
   ASSERT(source->Height == stencil->Height);

   width = source->Width;
   for (row = 0; row < source->Height; row++) {
      GLuint i;
      source->GetRow(ctx, source, width, 0, row, combined);
      for (i = 0; i < width; i++) {
         depthVals[i] = combined[i] >> 8;
         stencilVals[i] = combined[i] & 0xff;
      }
      depth->PutRow(ctx, depth, width, 0, row, depthVals, NULL);
      stencil->PutRow(ctx, stencil, width, 0, row, stencilVals, NULL);
   }
}
