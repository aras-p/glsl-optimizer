/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Vertex transform feedback support.
 *
 * Authors:
 *   Brian Paul
 */


#include "buffers.h"
#include "bufferobj.h"
#include "context.h"
#include "transformfeedback.h"

#include "shader/prog_parameter.h"
#include "shader/shader_api.h"


/**
 * Check if the given primitive mode (as in glBegin(mode)) is compatible
 * with the current transform feedback mode (if it's enabled).
 * This is to be called from glBegin(), glDrawArrays(), glDrawElements(), etc.
 *
 * \return GL_TRUE if the mode is OK, GL_FALSE otherwise.
 */
GLboolean
_mesa_validate_primitive_mode(GLcontext *ctx, GLenum mode)
{
   if (ctx->TransformFeedback.Active) {
      switch (mode) {
      case GL_POINTS:
         return ctx->TransformFeedback.Mode == GL_POINTS;
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
         return ctx->TransformFeedback.Mode == GL_LINES;
      default:
         return ctx->TransformFeedback.Mode == GL_TRIANGLES;
      }
   }
   return GL_TRUE;
}


/**
 * Check that all the buffer objects currently bound for transform
 * feedback actually exist.  Raise a GL_INVALID_OPERATION error if
 * any buffers are missing.
 * \return GL_TRUE for success, GL_FALSE if error
 */
GLboolean
_mesa_validate_transform_feedback_buffers(GLcontext *ctx)
{

   return GL_TRUE;
}



/**
 * Per-context init for transform feedback.
 */
void
_mesa_init_transform_feedback(GLcontext *ctx)
{
   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 ctx->Shared->NullBufferObj);
}


/**
 * Per-context free/clean-up for transform feedback.
 */
void
_mesa_free_transform_feedback(GLcontext *ctx)
{
   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 NULL);
}


void GLAPIENTRY
_mesa_BeginTransformFeedback(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (mode) {
   case GL_POINTS:
   case GL_LINES:
   case GL_TRIANGLES:
      /* legal */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginTransformFeedback(mode)");
      return;
   }

   if (ctx->TransformFeedback.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginTransformFeedback(already active)");
      return;
   }

   ctx->TransformFeedback.Active = GL_TRUE;
   ctx->TransformFeedback.Mode = mode;
}


void GLAPIENTRY
_mesa_EndTransformFeedback(void)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->TransformFeedback.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndTransformFeedback(not active)");
      return;
   }

   ctx->TransformFeedback.Active = GL_FALSE;
}


/**
 * Helper used by BindBufferRange() and BindBufferBase().
 */
static void
bind_buffer_range(GLcontext *ctx, GLuint index,
                  struct gl_buffer_object *bufObj,
                  GLintptr offset, GLsizeiptr size)
{
   /* The general binding point */
   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 bufObj);

   /* The per-attribute binding point */
   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.Buffers[index],
                                 bufObj);

   ctx->TransformFeedback.BufferNames[index] = bufObj->Name;

   ctx->TransformFeedback.Offset[index] = offset;
   ctx->TransformFeedback.Size[index] = size;
}


/**
 * Specify a buffer object to receive vertex shader results.  Plus,
 * specify the starting offset to place the results, and max size.
 */
void GLAPIENTRY
_mesa_BindBufferRange(GLenum target, GLuint index,
                      GLuint buffer, GLintptr offset, GLsizeiptr size)
{
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);

   if (target != GL_TRANSFORM_FEEDBACK_BUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferRange(target)");
      return;
   }

   if (ctx->TransformFeedback.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
      return;
   }

   if ((size <= 0) || (size & 0x3)) {
      /* must be positive and multiple of four */
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(size%d)", size);
      return;
   }  

   if (offset & 0x3) {
      /* must be multiple of four */
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferRange(offset=%d)", offset);
      return;
   }  

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(invalid buffer=%u)", buffer);
      return;
   }

   if (offset + size >= bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferRange(offset + size > buffer size)", size);
      return;
   }  

   bind_buffer_range(ctx, index, bufObj, offset, size);
}


/**
 * Specify a buffer object to receive vertex shader results.
 * As above, but start at offset = 0.
 */
void GLAPIENTRY
_mesa_BindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
   GLsizeiptr size;

   if (target != GL_TRANSFORM_FEEDBACK_BUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferBase(target)");
      return;
   }

   if (ctx->TransformFeedback.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
      return;
   }

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferBase(invalid buffer=%u)", buffer);
      return;
   }

   /* default size is the buffer size rounded down to nearest
    * multiple of four.
    */
   size = bufObj->Size & ~0x3;

   bind_buffer_range(ctx, index, bufObj, 0, size);
}


/**
 * Specify a buffer object to receive vertex shader results, plus the
 * offset in the buffer to start placing results.
 * This function is part of GL_EXT_transform_feedback, but not GL3.
 */
void GLAPIENTRY
_mesa_BindBufferOffsetEXT(GLenum target, GLuint index, GLuint buffer,
                          GLintptr offset)
{
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
   GLsizeiptr size;

   if (target != GL_TRANSFORM_FEEDBACK_BUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferOffsetEXT(target)");
      return;
   }

   if (ctx->TransformFeedback.Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackSeparateAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferOffsetEXT(index=%d)", index);
      return;
   }

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferOffsetEXT(invalid buffer=%u)", buffer);
      return;
   }

   /* default size is the buffer size rounded down to nearest
    * multiple of four.
    */
   size = (bufObj->Size - offset) & ~0x3;

   bind_buffer_range(ctx, index, bufObj, offset, size);
}


/**
 * This function specifies the vertex shader outputs to be written
 * to the feedback buffer(s), and in what order.
 */
void GLAPIENTRY
_mesa_TransformFeedbackVaryings(GLuint program, GLsizei count,
                                const GLchar **varyings, GLenum bufferMode)
{
   struct gl_shader_program *shProg;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);

   switch (bufferMode) {
   case GL_INTERLEAVED_ATTRIBS:
      break;
   case GL_SEPARATE_ATTRIBS:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glTransformFeedbackVaryings(bufferMode)");
      return;
   }

   if (count < 0 || count > ctx->Const.MaxTransformFeedbackSeparateAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTransformFeedbackVaryings(count=%d)", count);
      return;
   }

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTransformFeedbackVaryings(program=%u)", program);
      return;
   }

   /* free existing varyings, if any */
   for (i = 0; i < shProg->TransformFeedback.NumVarying; i++) {
      free(shProg->TransformFeedback.VaryingNames[i]);
   }
   free(shProg->TransformFeedback.VaryingNames);

   /* allocate new memory for varying names */
   shProg->TransformFeedback.VaryingNames =
      (GLchar **) malloc(count * sizeof(GLchar *));

   if (!shProg->TransformFeedback.VaryingNames) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTransformFeedbackVaryings()");
      return;
   }

   /* Save the new names and the count */
   for (i = 0; i < (GLuint) count; i++) {
      shProg->TransformFeedback.VaryingNames[i] = _mesa_strdup(varyings[i]);
   }
   shProg->TransformFeedback.NumVarying = count;

   shProg->TransformFeedback.BufferMode = bufferMode;

   /* The varyings won't be used until shader link time */
}


/**
 * Get info about the vertex shader's outputs which are to be written
 * to the feedback buffer(s).
 */
void GLAPIENTRY
_mesa_GetTransformFeedbackVarying(GLuint program, GLuint index,
                                  GLsizei bufSize, GLsizei *length,
                                  GLsizei *size, GLenum *type, GLchar *name)
{
   const struct gl_shader_program *shProg;
   const GLchar *varyingName;
   GLint v;
   GET_CURRENT_CONTEXT(ctx);

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetTransformFeedbackVaryings(program=%u)", program);
      return;
   }

   if (index >= shProg->TransformFeedback.NumVarying) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetTransformFeedbackVaryings(index=%u)", index);
      return;
   }

   varyingName = shProg->TransformFeedback.VaryingNames[index];

   v = _mesa_lookup_parameter_index(shProg->Varying, -1, varyingName);
   if (v >= 0) {
      struct gl_program_parameter *param = &shProg->Varying->Parameters[v];

      /* return the varying's name and length */
      _mesa_copy_string(name, bufSize, length, varyingName);

      /* return the datatype and value's size (in datatype units) */
      if (type)
         *type = param->DataType;
      if (size)
         *size = param->Size;
   }
   else {
      name[0] = 0;
      if (length)
         *length = 0;
      if (type)
         *type = 0;
      if (size)
         *size = 0;
   }
}

