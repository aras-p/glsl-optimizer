/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex_buffer_object extension.
 * \author Brian Paul
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "bufferobj.h"


void
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target)");
      return;
   }
}

void
_mesa_DeleteBuffersARB(GLsizei n, const GLuint *buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

}

void
_mesa_GenBuffersARB(GLsizei n, GLuint *buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenBuffersARB(n)");
      return;
   }
}

GLboolean
_mesa_IsBufferARB(GLuint buffer)
{
   return GL_FALSE;
}

void
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferDataARB(size < 0)");
      return;
   }

   switch (usage) {
      case GL_STREAM_DRAW_ARB:
      case GL_STREAM_READ_ARB:
      case GL_STREAM_COPY_ARB:
      case GL_STATIC_DRAW_ARB:
      case GL_STATIC_READ_ARB:
      case GL_STATIC_COPY_ARB:
      case GL_DYNAMIC_DRAW_ARB:
      case GL_DYNAMIC_READ_ARB:
      case GL_DYNAMIC_COPY_ARB:
         /* OK */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glBufferDataARB(usage)");
         return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBufferDataARB(target)");
      return;
   }
}

void
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferSubDataARB(size < 0)");
      return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBufferSubDataARB(target)");
      return;
   }
}

void
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetBufferSubDataARB(size < 0)");
      return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferSubDataARB(target)");
      return;
   }
}

void
_mesa_MapBufferARB(GLenum target, GLenum access)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (access) {
      case GL_READ_ONLY_ARB:
      case GL_WRITE_ONLY_ARB:
      case GL_READ_WRITE_ARB:
         /* OK */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(access)");
         return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(target)");
      return;
   }
}

GLboolean
_mesa_UnmapBufferARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glUnmapBufferARB(target)");
      return GL_FALSE;
   }
   return GL_FALSE;
}

void
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_BUFFER_SIZE_ARB:
      case GL_BUFFER_USAGE_ARB:
      case GL_BUFFER_ACCESS_ARB:
      case GL_BUFFER_MAPPED_ARB:
         /* ok */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterivARB(pname)");
         return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterARB(target)");
      return;
   }
}

void
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname != GL_BUFFER_MAP_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(pname)");
      return;
   }

   if (target == GL_ARRAY_BUFFER_ARB) {

   }
   else if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {

   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(target)");
      return;
   }
}
