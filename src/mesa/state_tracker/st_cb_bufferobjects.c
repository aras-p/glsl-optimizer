/**************************************************************************
 *
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * Functions for pixel buffer objects and vertex/element buffer objects.
 */


#include "main/imports.h"
#include "main/mtypes.h"
#include "main/arrayobj.h"
#include "main/bufferobj.h"

#include "st_context.h"
#include "st_cb_bufferobjects.h"
#include "st_debug.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"


/**
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
st_bufferobj_alloc(struct gl_context *ctx, GLuint name, GLenum target)
{
   struct st_buffer_object *st_obj = ST_CALLOC_STRUCT(st_buffer_object);

   if (!st_obj)
      return NULL;

   _mesa_initialize_buffer_object(ctx, &st_obj->Base, name, target);

   return &st_obj->Base;
}



/**
 * Deallocate/free a vertex/pixel buffer object.
 * Called via glDeleteBuffersARB().
 */
static void
st_bufferobj_free(struct gl_context *ctx, struct gl_buffer_object *obj)
{
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   assert(obj->RefCount == 0);
   _mesa_buffer_unmap_all_mappings(ctx, obj);

   if (st_obj->buffer)
      pipe_resource_reference(&st_obj->buffer, NULL);

   free(st_obj->Base.Label);
   free(st_obj);
}



/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void
st_bufferobj_subdata(struct gl_context *ctx,
		     GLintptrARB offset,
		     GLsizeiptrARB size,
		     const GLvoid * data, struct gl_buffer_object *obj)
{
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   /* we may be called from VBO code, so double-check params here */
   ASSERT(offset >= 0);
   ASSERT(size >= 0);
   ASSERT(offset + size <= obj->Size);

   if (!size)
      return;

   /*
    * According to ARB_vertex_buffer_object specification, if data is null,
    * then the contents of the buffer object's data store is undefined. We just
    * ignore, and leave it unchanged.
    */
   if (!data)
      return;

   if (!st_obj->buffer) {
      /* we probably ran out of memory during buffer allocation */
      return;
   }

   /* Now that transfers are per-context, we don't have to figure out
    * flushing here.  Usually drivers won't need to flush in this case
    * even if the buffer is currently referenced by hardware - they
    * just queue the upload as dma rather than mapping the underlying
    * buffer directly.
    */
   pipe_buffer_write(st_context(ctx)->pipe,
		     st_obj->buffer,
		     offset, size, data);
}


/**
 * Called via glGetBufferSubDataARB().
 */
static void
st_bufferobj_get_subdata(struct gl_context *ctx,
                         GLintptrARB offset,
                         GLsizeiptrARB size,
                         GLvoid * data, struct gl_buffer_object *obj)
{
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   /* we may be called from VBO code, so double-check params here */
   ASSERT(offset >= 0);
   ASSERT(size >= 0);
   ASSERT(offset + size <= obj->Size);

   if (!size)
      return;

   if (!st_obj->buffer) {
      /* we probably ran out of memory during buffer allocation */
      return;
   }

   pipe_buffer_read(st_context(ctx)->pipe, st_obj->buffer,
                    offset, size, data);
}


/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via ctx->Driver.BufferData().
 * \return GL_TRUE for success, GL_FALSE if out of memory
 */
static GLboolean
st_bufferobj_data(struct gl_context *ctx,
		  GLenum target,
		  GLsizeiptrARB size,
		  const GLvoid * data,
		  GLenum usage,
                  GLbitfield storageFlags,
		  struct gl_buffer_object *obj)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);
   unsigned bind, pipe_usage, pipe_flags = 0;

   if (size && data && st_obj->buffer &&
       st_obj->Base.Size == size &&
       st_obj->Base.Usage == usage &&
       st_obj->Base.StorageFlags == storageFlags) {
      /* Just discard the old contents and write new data.
       * This should be the same as creating a new buffer, but we avoid
       * a lot of validation in Mesa.
       */
      struct pipe_box box;

      u_box_1d(0, size, &box);
      pipe->transfer_inline_write(pipe, st_obj->buffer, 0,
                                  PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE,
                                  &box, data, 0, 0);
      return GL_TRUE;
   }

   st_obj->Base.Size = size;
   st_obj->Base.Usage = usage;
   st_obj->Base.StorageFlags = storageFlags;

   switch (target) {
   case GL_PIXEL_PACK_BUFFER_ARB:
   case GL_PIXEL_UNPACK_BUFFER_ARB:
      bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      break;
   case GL_ARRAY_BUFFER_ARB:
      bind = PIPE_BIND_VERTEX_BUFFER;
      break;
   case GL_ELEMENT_ARRAY_BUFFER_ARB:
      bind = PIPE_BIND_INDEX_BUFFER;
      break;
   case GL_TEXTURE_BUFFER:
      bind = PIPE_BIND_SAMPLER_VIEW;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      bind = PIPE_BIND_STREAM_OUTPUT;
      break;
   case GL_UNIFORM_BUFFER:
      bind = PIPE_BIND_CONSTANT_BUFFER;
      break;
   default:
      bind = 0;
   }

   /* Set usage. */
   if (st_obj->Base.Immutable) {
      /* BufferStorage */
      if (storageFlags & GL_CLIENT_STORAGE_BIT)
         pipe_usage = PIPE_USAGE_STAGING;
      else
         pipe_usage = PIPE_USAGE_DEFAULT;
   }
   else {
      /* BufferData */
      switch (usage) {
      case GL_STATIC_DRAW:
      case GL_STATIC_READ:
      case GL_STATIC_COPY:
      default:
	 pipe_usage = PIPE_USAGE_DEFAULT;
         break;
      case GL_DYNAMIC_DRAW:
      case GL_DYNAMIC_READ:
      case GL_DYNAMIC_COPY:
         pipe_usage = PIPE_USAGE_DYNAMIC;
         break;
      case GL_STREAM_DRAW:
      case GL_STREAM_READ:
      case GL_STREAM_COPY:
         pipe_usage = PIPE_USAGE_STREAM;
         break;
      }
   }

   /* Set flags. */
   if (storageFlags & GL_MAP_PERSISTENT_BIT)
      pipe_flags |= PIPE_RESOURCE_FLAG_MAP_PERSISTENT;
   if (storageFlags & GL_MAP_COHERENT_BIT)
      pipe_flags |= PIPE_RESOURCE_FLAG_MAP_COHERENT;

   pipe_resource_reference( &st_obj->buffer, NULL );

   if (ST_DEBUG & DEBUG_BUFFER) {
      debug_printf("Create buffer size %td bind 0x%x\n", size, bind);
   }

   if (size != 0) {
      struct pipe_resource buffer;

      memset(&buffer, 0, sizeof buffer);
      buffer.target = PIPE_BUFFER;
      buffer.format = PIPE_FORMAT_R8_UNORM; /* want TYPELESS or similar */
      buffer.bind = bind;
      buffer.usage = pipe_usage;
      buffer.flags = pipe_flags;
      buffer.width0 = size;
      buffer.height0 = 1;
      buffer.depth0 = 1;
      buffer.array_size = 1;

      st_obj->buffer = pipe->screen->resource_create(pipe->screen, &buffer);

      if (!st_obj->buffer) {
         /* out of memory */
         st_obj->Base.Size = 0;
         return GL_FALSE;
      }

      if (data)
         pipe_buffer_write(pipe, st_obj->buffer, 0, size, data);
   }

   /* BufferData may change an array or uniform buffer, need to update it */
   st->dirty.st |= ST_NEW_VERTEX_ARRAYS | ST_NEW_UNIFORM_BUFFER;

   return GL_TRUE;
}


/**
 * Called via glMapBufferRange().
 */
static void *
st_bufferobj_map_range(struct gl_context *ctx,
                       GLintptr offset, GLsizeiptr length, GLbitfield access,
                       struct gl_buffer_object *obj,
                       gl_map_buffer_index index)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);
   enum pipe_transfer_usage flags = 0x0;

   if (access & GL_MAP_WRITE_BIT)
      flags |= PIPE_TRANSFER_WRITE;

   if (access & GL_MAP_READ_BIT)
      flags |= PIPE_TRANSFER_READ;

   if (access & GL_MAP_FLUSH_EXPLICIT_BIT)
      flags |= PIPE_TRANSFER_FLUSH_EXPLICIT;

   if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
      flags |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
   }
   else if (access & GL_MAP_INVALIDATE_RANGE_BIT) {
      if (offset == 0 && length == obj->Size)
         flags |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
      else
         flags |= PIPE_TRANSFER_DISCARD_RANGE;
   }

   if (access & GL_MAP_UNSYNCHRONIZED_BIT)
      flags |= PIPE_TRANSFER_UNSYNCHRONIZED;

   if (access & GL_MAP_PERSISTENT_BIT)
      flags |= PIPE_TRANSFER_PERSISTENT;

   if (access & GL_MAP_COHERENT_BIT)
      flags |= PIPE_TRANSFER_COHERENT;

   /* ... other flags ...
    */

   if (access & MESA_MAP_NOWAIT_BIT)
      flags |= PIPE_TRANSFER_DONTBLOCK;

   assert(offset >= 0);
   assert(length >= 0);
   assert(offset < obj->Size);
   assert(offset + length <= obj->Size);

   obj->Mappings[index].Pointer = pipe_buffer_map_range(pipe,
                                        st_obj->buffer,
                                        offset, length,
                                        flags,
                                        &st_obj->transfer[index]);
   if (obj->Mappings[index].Pointer) {
      obj->Mappings[index].Offset = offset;
      obj->Mappings[index].Length = length;
      obj->Mappings[index].AccessFlags = access;
   }
   else {
      st_obj->transfer[index] = NULL;
   }

   return obj->Mappings[index].Pointer;
}


static void
st_bufferobj_flush_mapped_range(struct gl_context *ctx,
                                GLintptr offset, GLsizeiptr length,
                                struct gl_buffer_object *obj,
                                gl_map_buffer_index index)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   /* Subrange is relative to mapped range */
   assert(offset >= 0);
   assert(length >= 0);
   assert(offset + length <= obj->Mappings[index].Length);
   assert(obj->Mappings[index].Pointer);

   if (!length)
      return;

   pipe_buffer_flush_mapped_range(pipe, st_obj->transfer[index],
                                  obj->Mappings[index].Offset + offset,
                                  length);
}


/**
 * Called via glUnmapBufferARB().
 */
static GLboolean
st_bufferobj_unmap(struct gl_context *ctx, struct gl_buffer_object *obj,
                   gl_map_buffer_index index)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *st_obj = st_buffer_object(obj);

   if (obj->Mappings[index].Length)
      pipe_buffer_unmap(pipe, st_obj->transfer[index]);

   st_obj->transfer[index] = NULL;
   obj->Mappings[index].Pointer = NULL;
   obj->Mappings[index].Offset = 0;
   obj->Mappings[index].Length = 0;
   return GL_TRUE;
}


/**
 * Called via glCopyBufferSubData().
 */
static void
st_copy_buffer_subdata(struct gl_context *ctx,
                       struct gl_buffer_object *src,
                       struct gl_buffer_object *dst,
                       GLintptr readOffset, GLintptr writeOffset,
                       GLsizeiptr size)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *srcObj = st_buffer_object(src);
   struct st_buffer_object *dstObj = st_buffer_object(dst);
   struct pipe_box box;

   if (!size)
      return;

   /* buffer should not already be mapped */
   assert(!_mesa_check_disallowed_mapping(src));
   assert(!_mesa_check_disallowed_mapping(dst));

   u_box_1d(readOffset, size, &box);

   pipe->resource_copy_region(pipe, dstObj->buffer, 0, writeOffset, 0, 0,
                              srcObj->buffer, 0, &box);
}

/**
 * Called via glClearBufferSubData().
 */
static void
st_clear_buffer_subdata(struct gl_context *ctx,
                        GLintptr offset, GLsizeiptr size,
                        const GLvoid *clearValue,
                        GLsizeiptr clearValueSize,
                        struct gl_buffer_object *bufObj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_buffer_object *buf = st_buffer_object(bufObj);
   static const char zeros[16] = {0};

   if (!pipe->clear_buffer) {
      _mesa_buffer_clear_subdata(ctx, offset, size,
                                 clearValue, clearValueSize, bufObj);
      return;
   }

   if (!clearValue)
      clearValue = zeros;

   pipe->clear_buffer(pipe, buf->buffer, offset, size,
                      clearValue, clearValueSize);
}


/* TODO: if buffer wasn't created with appropriate usage flags, need
 * to recreate it now and copy contents -- or possibly create a
 * gallium entrypoint to extend the usage flags and let the driver
 * decide if a copy is necessary.
 */
void
st_bufferobj_validate_usage(struct st_context *st,
			    struct st_buffer_object *obj,
			    unsigned usage)
{
}


void
st_init_bufferobject_functions(struct dd_function_table *functions)
{
   /* plug in default driver fallbacks (such as for ClearBufferSubData) */
   _mesa_init_buffer_object_functions(functions);

   functions->NewBufferObject = st_bufferobj_alloc;
   functions->DeleteBuffer = st_bufferobj_free;
   functions->BufferData = st_bufferobj_data;
   functions->BufferSubData = st_bufferobj_subdata;
   functions->GetBufferSubData = st_bufferobj_get_subdata;
   functions->MapBufferRange = st_bufferobj_map_range;
   functions->FlushMappedBufferRange = st_bufferobj_flush_mapped_range;
   functions->UnmapBuffer = st_bufferobj_unmap;
   functions->CopyBufferSubData = st_copy_buffer_subdata;
   functions->ClearBufferSubData = st_clear_buffer_subdata;

   /* For GL_APPLE_vertex_array_object */
   functions->NewArrayObject = _mesa_new_vao;
   functions->DeleteArrayObject = _mesa_delete_vao;
}
