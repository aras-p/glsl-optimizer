/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013  Timothy Arceri   All Rights Reserved.
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


#include "arrayobj.h"
#include "bufferobj.h"
#include "context.h"
#include "dlist.h"
#include "enums.h"
#include "fbobject.h"
#include "objectlabel.h"
#include "queryobj.h"
#include "samplerobj.h"
#include "shaderobj.h"
#include "syncobj.h"
#include "texobj.h"
#include "transformfeedback.h"


/**
 * Helper for _mesa_ObjectLabel() and _mesa_ObjectPtrLabel().
 */
static void
set_label(struct gl_context *ctx, char **labelPtr, const char *label,
          int length, const char *caller)
{
   if (*labelPtr) {
      /* free old label string */
      free(*labelPtr);
      *labelPtr = NULL;
   }

   /* set new label string */
   if (label) {
      if (length >= 0) {
         if (length >= MAX_LABEL_LENGTH)
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "%s(length=%d, which is not less than "
                        "GL_MAX_LABEL_LENGTH=%d)", caller, length,
                        MAX_LABEL_LENGTH);

         /* explicit length */
         *labelPtr = (char *) malloc(length+1);
         if (*labelPtr) {
            memcpy(*labelPtr, label, length);
            /* length is not required to include the null terminator so
             * add one just in case
             */
            (*labelPtr)[length] = '\0';
         }
      }
      else {
         int len = strlen(label);
         if (len >= MAX_LABEL_LENGTH)
            _mesa_error(ctx, GL_INVALID_VALUE,
                "%s(label length=%d, which is not less than "
                "GL_MAX_LABEL_LENGTH=%d)", caller, len,
                MAX_LABEL_LENGTH);

         /* null-terminated string */
         *labelPtr = _mesa_strdup(label);
      }
   }
}

/**
 * Helper for _mesa_GetObjectLabel() and _mesa_GetObjectPtrLabel().
 */
static void
copy_label(char **labelPtr, char *label, int *length, int bufSize)
{
   int labelLen = 0;

   if (*labelPtr)
      labelLen = strlen(*labelPtr);

   if (label) {
      if (bufSize <= labelLen)
         labelLen =  bufSize-1;

      memcpy(label, *labelPtr, labelLen);
      label[labelLen] = '\0';
   }

   if (length)
      *length = labelLen;
}

/**
 * Helper for _mesa_ObjectLabel() and _mesa_GetObjectLabel().
 */
static char **
get_label_pointer(struct gl_context *ctx, GLenum identifier, GLuint name,
                  const char *caller)
{
   char **labelPtr = NULL;

   switch (identifier) {
   case GL_BUFFER:
      {
         struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, name);
         if (bufObj)
            labelPtr = &bufObj->Label;
      }
      break;
   case GL_SHADER:
      {
         struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
         if (shader)
            labelPtr = &shader->Label;
      }
      break;
   case GL_PROGRAM:
      {
         struct gl_shader_program *program =
            _mesa_lookup_shader_program(ctx, name);
         if (program)
            labelPtr = &program->Label;
      }
      break;
   case GL_VERTEX_ARRAY:
      {
         struct gl_array_object *obj = _mesa_lookup_arrayobj(ctx, name);
         if (obj)
            labelPtr = &obj->Label;
      }
      break;
   case GL_QUERY:
      {
         struct gl_query_object *query = _mesa_lookup_query_object(ctx, name);
         if (query)
            labelPtr = &query->Label;
      }
      break;
   case GL_TRANSFORM_FEEDBACK:
      {
         struct gl_transform_feedback_object *tfo =
            _mesa_lookup_transform_feedback_object(ctx, name);
         if (tfo)
            labelPtr = &tfo->Label;
      }
      break;
   case GL_SAMPLER:
      {
         struct gl_sampler_object *so = _mesa_lookup_samplerobj(ctx, name);
         if (so)
            labelPtr = &so->Label;
      }
      break;
   case GL_TEXTURE:
      {
         struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, name);
         if (texObj)
            labelPtr = &texObj->Label;
      }
      break;
   case GL_RENDERBUFFER:
      {
         struct gl_renderbuffer *rb = _mesa_lookup_renderbuffer(ctx, name);
         if (rb)
            labelPtr = &rb->Label;
      }
      break;
   case GL_FRAMEBUFFER:
      {
         struct gl_framebuffer *rb = _mesa_lookup_framebuffer(ctx, name);
         if (rb)
            labelPtr = &rb->Label;
      }
      break;
   case GL_DISPLAY_LIST:
      if (ctx->API == API_OPENGL_COMPAT) {
         struct gl_display_list *list = _mesa_lookup_list(ctx, name);
         if (list)
            labelPtr = &list->Label;
      }
      else {
         goto invalid_enum;
      }
      break;
   case GL_PROGRAM_PIPELINE:
      /* requires GL 4.2 */
      goto invalid_enum;
   default:
      goto invalid_enum;
   }

   if (NULL == labelPtr) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glObjectLabel(name = %u)", name);
   }

   return labelPtr;

invalid_enum:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(identifier = %s)",
               caller, _mesa_lookup_enum_by_nr(identifier));
   return NULL;
}

void GLAPIENTRY
_mesa_ObjectLabel(GLenum identifier, GLuint name, GLsizei length,
                  const GLchar *label)
{
   GET_CURRENT_CONTEXT(ctx);
   char **labelPtr;

   labelPtr = get_label_pointer(ctx, identifier, name, "glObjectLabel");
   if (!labelPtr)
      return;

   set_label(ctx, labelPtr, label, length, "glObjectLabel");
}

void GLAPIENTRY
_mesa_GetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize,
                     GLsizei *length, GLchar *label)
{
   GET_CURRENT_CONTEXT(ctx);
   char **labelPtr;

   labelPtr = get_label_pointer(ctx, identifier, name, "glGetObjectLabel");
   if (!labelPtr)
      return;

   copy_label(labelPtr, label, length, bufSize);
}

void GLAPIENTRY
_mesa_ObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label)
{
   GET_CURRENT_CONTEXT(ctx);
   char **labelPtr;
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) ptr;

   if (!_mesa_validate_sync(ctx, syncObj)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glObjectPtrLabel (not a valid sync object)");
      return;
   }

   labelPtr = &syncObj->Label;

   set_label(ctx, labelPtr, label, length, "glObjectPtrLabel");
}

void GLAPIENTRY
_mesa_GetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length,
                        GLchar *label)
{
   GET_CURRENT_CONTEXT(ctx);
   char **labelPtr;
   struct gl_sync_object *const syncObj = (struct gl_sync_object *) ptr;

   if (!_mesa_validate_sync(ctx, syncObj)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetObjectPtrLabel (not a valid sync object)");
      return;
   }

   labelPtr = &syncObj->Label;

   copy_label(labelPtr, label, length, bufSize);
}
