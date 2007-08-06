/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "imports.h"
#include "mtypes.h"
#include "bufferobj.h"

#include "st_context.h"
#include "st_cb_bufferobjects.h"

#include "pipe/p_context.h"

/* Pixel buffers and Vertex/index buffers are handled through these
 * mesa callbacks.  Framebuffer/Renderbuffer objects are
 * created/managed elsewhere.
 */



/**
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
st_bufferobj_alloc(GLcontext *ctx, GLuint name, GLenum target)
{
   struct st_context *st = st_context(ctx);
   struct st_buffer_object *st_obj = CALLOC_STRUCT(st_buffer_object);

   _mesa_initialize_buffer_object(&st_obj->Base, name, target);

   st_obj->buffer = st->pipe->create_buffer( st->pipe, 32, 0 );

   return &st_obj->Base;
}



/**
 * Deallocate/free a vertex/pixel buffer object.
 * Called via glDeleteBuffersARB().
 */
static void
st_bufferobj_free(GLcontext *ctx, struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   if (st_obj->buffer) 
      pipe->buffer_unreference(pipe, &st_obj->buffer);

   FREE(st_obj);
}



/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via glBufferDataARB().
 */
static void
st_bufferobj_data(GLcontext *ctx,
		  GLenum target,
		  GLsizeiptrARB size,
		  const GLvoid * data,
		  GLenum usage, 
		  struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   st_obj->Base.Size = size;
   st_obj->Base.Usage = usage;

   pipe->buffer_data( pipe, st_obj->buffer, size, data );
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void
st_bufferobj_subdata(GLcontext *ctx,
		     GLenum target,
		     GLintptrARB offset,
		     GLsizeiptrARB size,
		     const GLvoid * data, struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   pipe->buffer_subdata(pipe, st_obj->buffer, offset, size, data);
}


/**
 * Called via glGetBufferSubDataARB().
 */
static void
st_bufferobj_get_subdata(GLcontext *ctx,
                            GLenum target,
                            GLintptrARB offset,
                            GLsizeiptrARB size,
                            GLvoid * data, struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   pipe->buffer_get_subdata(pipe, st_obj->buffer, offset, size, data);
}



/**
 * Called via glMapBufferARB().
 */
static void *
st_bufferobj_map(GLcontext *ctx,
                    GLenum target,
                    GLenum access, struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);
   GLuint flags;

   switch (access) {
   case GL_WRITE_ONLY:
      flags = PIPE_BUFFER_FLAG_WRITE;
      break;


      flags = PIPE_BUFFER_FLAG_READ;
      break;

   case GL_READ_WRITE:
   default:
      flags = PIPE_BUFFER_FLAG_READ | PIPE_BUFFER_FLAG_WRITE;
      break;      
   }

   obj->Pointer = pipe->buffer_map(pipe, st_obj->buffer, flags);
   return obj->Pointer;
}


/**
 * Called via glMapBufferARB().
 */
static GLboolean
st_bufferobj_unmap(GLcontext *ctx,
		   GLenum target, struct gl_buffer_object *obj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   pipe->buffer_unmap(pipe, st_obj->buffer);
   obj->Pointer = NULL;
   return GL_TRUE;
}


void
st_init_bufferobject_functions(struct dd_function_table *functions)
{
   functions->NewBufferObject = st_bufferobj_alloc;
   functions->DeleteBuffer = st_bufferobj_free;
   functions->BufferData = st_bufferobj_data;
   functions->BufferSubData = st_bufferobj_subdata;
   functions->GetBufferSubData = st_bufferobj_get_subdata;
   functions->MapBuffer = st_bufferobj_map;
   functions->UnmapBuffer = st_bufferobj_unmap;
}
