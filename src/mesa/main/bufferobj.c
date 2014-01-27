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


/**
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex/pixel_buffer_object extensions.
 * \author Brian Paul, Ian Romanick
 */

#include <stdbool.h>
#include "glheader.h"
#include "enums.h"
#include "hash.h"
#include "imports.h"
#include "image.h"
#include "context.h"
#include "bufferobj.h"
#include "fbobject.h"
#include "mtypes.h"
#include "texobj.h"
#include "teximage.h"
#include "glformats.h"
#include "texstore.h"
#include "transformfeedback.h"
#include "dispatch.h"


/* Debug flags */
/*#define VBO_DEBUG*/
/*#define BOUNDS_CHECK*/


/**
 * Used as a placeholder for buffer objects between glGenBuffers() and
 * glBindBuffer() so that glIsBuffer() can work correctly.
 */
static struct gl_buffer_object DummyBufferObject;


/**
 * Return pointer to address of a buffer object target.
 * \param ctx  the GL context
 * \param target  the buffer object target to be retrieved.
 * \return   pointer to pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static inline struct gl_buffer_object **
get_buffer_target(struct gl_context *ctx, GLenum target)
{
   /* Other targets are only supported in desktop OpenGL and OpenGL ES 3.0.
    */
   if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx)
       && target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER)
      return NULL;

   switch (target) {
   case GL_ARRAY_BUFFER_ARB:
      return &ctx->Array.ArrayBufferObj;
   case GL_ELEMENT_ARRAY_BUFFER_ARB:
      return &ctx->Array.VAO->IndexBufferObj;
   case GL_PIXEL_PACK_BUFFER_EXT:
      return &ctx->Pack.BufferObj;
   case GL_PIXEL_UNPACK_BUFFER_EXT:
      return &ctx->Unpack.BufferObj;
   case GL_COPY_READ_BUFFER:
      return &ctx->CopyReadBuffer;
   case GL_COPY_WRITE_BUFFER:
      return &ctx->CopyWriteBuffer;
   case GL_DRAW_INDIRECT_BUFFER:
      if (ctx->API == API_OPENGL_CORE &&
          ctx->Extensions.ARB_draw_indirect) {
         return &ctx->DrawIndirectBuffer;
      }
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      if (ctx->Extensions.EXT_transform_feedback) {
         return &ctx->TransformFeedback.CurrentBuffer;
      }
      break;
   case GL_TEXTURE_BUFFER:
      if (ctx->API == API_OPENGL_CORE &&
          ctx->Extensions.ARB_texture_buffer_object) {
         return &ctx->Texture.BufferObject;
      }
      break;
   case GL_UNIFORM_BUFFER:
      if (ctx->Extensions.ARB_uniform_buffer_object) {
         return &ctx->UniformBuffer;
      }
      break;
   case GL_ATOMIC_COUNTER_BUFFER:
      if (ctx->Extensions.ARB_shader_atomic_counters) {
         return &ctx->AtomicBuffer;
      }
      break;
   default:
      return NULL;
   }
   return NULL;
}


/**
 * Get the buffer object bound to the specified target in a GL context.
 * \param ctx  the GL context
 * \param target  the buffer object target to be retrieved.
 * \param error  the GL error to record if target is illegal.
 * \return   pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static inline struct gl_buffer_object *
get_buffer(struct gl_context *ctx, const char *func, GLenum target,
           GLenum error)
{
   struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);

   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
      return NULL;
   }

   if (!_mesa_is_bufferobj(*bufObj)) {
      _mesa_error(ctx, error, "%s(no buffer bound)", func);
      return NULL;
   }

   return *bufObj;
}


/**
 * Convert a GLbitfield describing the mapped buffer access flags
 * into one of GL_READ_WRITE, GL_READ_ONLY, or GL_WRITE_ONLY.
 */
static GLenum
simplified_access_mode(struct gl_context *ctx, GLbitfield access)
{
   const GLbitfield rwFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   if ((access & rwFlags) == rwFlags)
      return GL_READ_WRITE;
   if ((access & GL_MAP_READ_BIT) == GL_MAP_READ_BIT)
      return GL_READ_ONLY;
   if ((access & GL_MAP_WRITE_BIT) == GL_MAP_WRITE_BIT)
      return GL_WRITE_ONLY;

   /* Otherwise, AccessFlags is zero (the default state).
    *
    * Table 2.6 on page 31 (page 44 of the PDF) of the OpenGL 1.5 spec says:
    *
    * Name           Type  Initial Value  Legal Values
    * ...            ...   ...            ...
    * BUFFER_ACCESS  enum  READ_WRITE     READ_ONLY, WRITE_ONLY
    *                                     READ_WRITE
    *
    * However, table 6.8 in the GL_OES_mapbuffer extension says:
    *
    * Get Value         Type Get Command          Value          Description
    * ---------         ---- -----------          -----          -----------
    * BUFFER_ACCESS_OES Z1   GetBufferParameteriv WRITE_ONLY_OES buffer map flag
    *
    * The difference is because GL_OES_mapbuffer only supports mapping buffers
    * write-only.
    */
   assert(access == 0);

   return _mesa_is_gles(ctx) ? GL_WRITE_ONLY : GL_READ_WRITE;
}


/**
 * Test if the buffer is mapped, and if so, if the mapped range overlaps the
 * given range.
 * The regions do not overlap if and only if the end of the given
 * region is before the mapped region or the start of the given region
 * is after the mapped region.
 *
 * \param obj     Buffer object target on which to operate.
 * \param offset  Offset of the first byte of the subdata range.
 * \param size    Size, in bytes, of the subdata range.
 * \return   true if ranges overlap, false otherwise
 *
 */
static bool
bufferobj_range_mapped(const struct gl_buffer_object *obj,
                       GLintptr offset, GLsizeiptr size)
{
   if (_mesa_bufferobj_mapped(obj)) {
      const GLintptr end = offset + size;
      const GLintptr mapEnd = obj->Offset + obj->Length;

      if (!(end <= obj->Offset || offset >= mapEnd)) {
         return true;
      }
   }
   return false;
}


/**
 * Tests the subdata range parameters and sets the GL error code for
 * \c glBufferSubDataARB, \c glGetBufferSubDataARB and
 * \c glClearBufferSubData.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte of the subdata range.
 * \param size    Size, in bytes, of the subdata range.
 * \param mappedRange  If true, checks if an overlapping range is mapped.
 *                     If false, checks if buffer is mapped.
 * \param errorNoBuffer  Error code if no buffer is bound to target.
 * \param caller  Name of calling function for recording errors.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if any of the parameter or state
 *           conditions are invalid.
 *
 * \sa glBufferSubDataARB, glGetBufferSubDataARB, glClearBufferSubData
 */
static struct gl_buffer_object *
buffer_object_subdata_range_good(struct gl_context * ctx, GLenum target,
                                 GLintptrARB offset, GLsizeiptrARB size,
                                 bool mappedRange, GLenum errorNoBuffer,
                                 const char *caller)
{
   struct gl_buffer_object *bufObj;

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(size < 0)", caller);
      return NULL;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset < 0)", caller);
      return NULL;
   }

   bufObj = get_buffer(ctx, caller, target, errorNoBuffer);
   if (!bufObj)
      return NULL;

   if (offset + size > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(offset %lu + size %lu > buffer size %lu)", caller,
                  (unsigned long) offset,
                  (unsigned long) size,
                  (unsigned long) bufObj->Size);
      return NULL;
   }

   if (mappedRange) {
      if (bufferobj_range_mapped(bufObj, offset, size)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
         return NULL;
      }
   }
   else {
      if (_mesa_bufferobj_mapped(bufObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
         return NULL;
      }
   }

   return bufObj;
}


/**
 * Test the format and type parameters and set the GL error code for
 * \c glClearBufferData and \c glClearBufferSubData.
 *
 * \param ctx             GL context.
 * \param internalformat  Format to which the data is to be converted.
 * \param format          Format of the supplied data.
 * \param type            Type of the supplied data.
 * \param caller          Name of calling function for recording errors.
 * \return   If internalformat, format and type are legal the mesa_format
 *           corresponding to internalformat, otherwise MESA_FORMAT_NONE.
 *
 * \sa glClearBufferData and glClearBufferSubData
 */
static mesa_format
validate_clear_buffer_format(struct gl_context *ctx,
                             GLenum internalformat,
                             GLenum format, GLenum type,
                             const char *caller)
{
   mesa_format mesaFormat;
   GLenum errorFormatType;

   mesaFormat = _mesa_validate_texbuffer_format(ctx, internalformat);
   if (mesaFormat == MESA_FORMAT_NONE) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(invalid internalformat)", caller);
      return MESA_FORMAT_NONE;
   }

   /* NOTE: not mentioned in ARB_clear_buffer_object but according to
    * EXT_texture_integer there is no conversion between integer and
    * non-integer formats
   */
   if (_mesa_is_enum_format_signed_int(format) !=
       _mesa_is_format_integer_color(mesaFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(integer vs non-integer)", caller);
      return MESA_FORMAT_NONE;
   }

   if (!_mesa_is_color_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(format is not a color format)", caller);
      return MESA_FORMAT_NONE;
   }

   errorFormatType = _mesa_error_check_format_and_type(ctx, format, type);
   if (errorFormatType != GL_NO_ERROR) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "%s(invalid format or type)", caller);
      return MESA_FORMAT_NONE;
   }

   return mesaFormat;
}


/**
 * Convert user-specified clear value to the specified internal format.
 *
 * \param ctx             GL context.
 * \param internalformat  Format to which the data is converted.
 * \param clearValue      Points to the converted clear value.
 * \param format          Format of the supplied data.
 * \param type            Type of the supplied data.
 * \param data            Data which is to be converted to internalformat.
 * \param caller          Name of calling function for recording errors.
 * \return   true if data could be converted, false otherwise.
 *
 * \sa glClearBufferData, glClearBufferSubData
 */
static bool
convert_clear_buffer_data(struct gl_context *ctx,
                          mesa_format internalformat,
                          GLubyte *clearValue, GLenum format, GLenum type,
                          const GLvoid *data, const char *caller)
{
   GLenum internalformatBase = _mesa_get_format_base_format(internalformat);

   if (_mesa_texstore(ctx, 1, internalformatBase, internalformat,
                      0, &clearValue, 1, 1, 1,
                      format, type, data, &ctx->Unpack)) {
      return true;
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
      return false;
   }
}


/**
 * Allocate and initialize a new buffer object.
 * 
 * Default callback for the \c dd_function_table::NewBufferObject() hook.
 */
static struct gl_buffer_object *
_mesa_new_buffer_object( struct gl_context *ctx, GLuint name, GLenum target )
{
   struct gl_buffer_object *obj;

   (void) ctx;

   obj = MALLOC_STRUCT(gl_buffer_object);
   _mesa_initialize_buffer_object(ctx, obj, name, target);
   return obj;
}


/**
 * Delete a buffer object.
 * 
 * Default callback for the \c dd_function_table::DeleteBuffer() hook.
 */
static void
_mesa_delete_buffer_object(struct gl_context *ctx,
                           struct gl_buffer_object *bufObj)
{
   (void) ctx;

   _mesa_align_free(bufObj->Data);

   /* assign strange values here to help w/ debugging */
   bufObj->RefCount = -1000;
   bufObj->Name = ~0;

   _glthread_DESTROY_MUTEX(bufObj->Mutex);
   free(bufObj->Label);
   free(bufObj);
}



/**
 * Set ptr to bufObj w/ reference counting.
 * This is normally only called from the _mesa_reference_buffer_object() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_buffer_object_(struct gl_context *ctx,
                               struct gl_buffer_object **ptr,
                               struct gl_buffer_object *bufObj)
{
   if (*ptr) {
      /* Unreference the old buffer */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_buffer_object *oldObj = *ptr;

      _glthread_LOCK_MUTEX(oldObj->Mutex);
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
#if 0
      printf("BufferObj %p %d DECR to %d\n",
             (void *) oldObj, oldObj->Name, oldObj->RefCount);
#endif
      deleteFlag = (oldObj->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldObj->Mutex);

      if (deleteFlag) {

         /* some sanity checking: don't delete a buffer still in use */
#if 0
         /* unfortunately, these tests are invalid during context tear-down */
	 ASSERT(ctx->Array.ArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.VAO->IndexBufferObj != bufObj);
	 ASSERT(ctx->Array.VAO->Vertex.BufferObj != bufObj);
#endif

	 ASSERT(ctx->Driver.DeleteBuffer);
         ctx->Driver.DeleteBuffer(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (bufObj) {
      /* reference new buffer */
      _glthread_LOCK_MUTEX(bufObj->Mutex);
      if (bufObj->RefCount == 0) {
         /* this buffer's being deleted (look just above) */
         /* Not sure this can every really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted buffer object");
         *ptr = NULL;
      }
      else {
         bufObj->RefCount++;
#if 0
         printf("BufferObj %p %d INCR to %d\n",
                (void *) bufObj, bufObj->Name, bufObj->RefCount);
#endif
         *ptr = bufObj;
      }
      _glthread_UNLOCK_MUTEX(bufObj->Mutex);
   }
}


/**
 * Initialize a buffer object to default values.
 */
void
_mesa_initialize_buffer_object( struct gl_context *ctx,
				struct gl_buffer_object *obj,
				GLuint name, GLenum target )
{
   (void) target;

   memset(obj, 0, sizeof(struct gl_buffer_object));
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Usage = GL_STATIC_DRAW_ARB;
   obj->AccessFlags = 0;
}



/**
 * Callback called from _mesa_HashWalk()
 */
static void
count_buffer_size(GLuint key, void *data, void *userData)
{
   const struct gl_buffer_object *bufObj =
      (const struct gl_buffer_object *) data;
   GLuint *total = (GLuint *) userData;

   *total = *total + bufObj->Size;
}


/**
 * Compute total size (in bytes) of all buffer objects for the given context.
 * For debugging purposes.
 */
GLuint
_mesa_total_buffer_object_memory(struct gl_context *ctx)
{
   GLuint total = 0;

   _mesa_HashWalk(ctx->Shared->BufferObjects, count_buffer_size, &total);

   return total;
}


/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If \c data is \c NULL,
 * memory will be allocated, but no copy will occur.
 *
 * This is the default callback for \c dd_function_table::BufferData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param size    Size, in bytes, of the new data store.
 * \param data    Pointer to the data to store in the buffer object.  This
 *                pointer may be \c NULL.
 * \param usage   Hints about how the data will be used.
 * \param bufObj  Object to be used.
 *
 * \return GL_TRUE for success, GL_FALSE for failure
 * \sa glBufferDataARB, dd_function_table::BufferData.
 */
static GLboolean
_mesa_buffer_data( struct gl_context *ctx, GLenum target, GLsizeiptrARB size,
		   const GLvoid * data, GLenum usage, GLenum storageFlags,
		   struct gl_buffer_object * bufObj )
{
   void * new_data;

   (void) target;

   _mesa_align_free( bufObj->Data );

   new_data = _mesa_align_malloc( size, ctx->Const.MinMapBufferAlignment );
   if (new_data) {
      bufObj->Data = (GLubyte *) new_data;
      bufObj->Size = size;
      bufObj->Usage = usage;
      bufObj->StorageFlags = storageFlags;

      if (data) {
	 memcpy( bufObj->Data, data, size );
      }

      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This is the default callback for \c dd_function_table::BufferSubData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx     GL context.
 * \param offset  Offset of the first byte to be modified.
 * \param size    Size, in bytes, of the data range.
 * \param data    Pointer to the data to store in the buffer object.
 * \param bufObj  Object to be used.
 *
 * \sa glBufferSubDataARB, dd_function_table::BufferSubData.
 */
static void
_mesa_buffer_subdata( struct gl_context *ctx, GLintptrARB offset,
		      GLsizeiptrARB size, const GLvoid * data,
		      struct gl_buffer_object * bufObj )
{
   (void) ctx;

   /* this should have been caught in _mesa_BufferSubData() */
   ASSERT(size + offset <= bufObj->Size);

   if (bufObj->Data) {
      memcpy( (GLubyte *) bufObj->Data + offset, data, size );
   }
}


/**
 * Retrieve data from a subrange of buffer object.  If the data range
 * specified by \c size + \c offset extends beyond the end of the buffer or
 * if \c data is \c NULL, no copy is performed.
 *
 * This is the default callback for \c dd_function_table::GetBufferSubData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte to be fetched.
 * \param size    Size, in bytes, of the data range.
 * \param data    Destination for data
 * \param bufObj  Object to be used.
 *
 * \sa glBufferGetSubDataARB, dd_function_table::GetBufferSubData.
 */
static void
_mesa_buffer_get_subdata( struct gl_context *ctx, GLintptrARB offset,
			  GLsizeiptrARB size, GLvoid * data,
			  struct gl_buffer_object * bufObj )
{
   (void) ctx;

   if (bufObj->Data && ((GLsizeiptrARB) (size + offset) <= bufObj->Size)) {
      memcpy( data, (GLubyte *) bufObj->Data + offset, size );
   }
}


/**
 * Clear a subrange of the buffer object with copies of the supplied data.
 * If data is NULL the buffer is filled with zeros.
 *
 * This is the default callback for \c dd_function_table::ClearBufferSubData()
 * Note that all GL error checking will have been done already.
 *
 * \param ctx             GL context.
 * \param offset          Offset of the first byte to be cleared.
 * \param size            Size, in bytes, of the to be cleared range.
 * \param clearValue      Source of the data.
 * \param clearValueSize  Size, in bytes, of the supplied data.
 * \param bufObj          Object to be cleared.
 *
 * \sa glClearBufferSubData, glClearBufferData and
 * dd_function_table::ClearBufferSubData.
 */
static void
_mesa_buffer_clear_subdata(struct gl_context *ctx,
                           GLintptr offset, GLsizeiptr size,
                           const GLvoid *clearValue,
                           GLsizeiptr clearValueSize,
                           struct gl_buffer_object *bufObj)
{
   GLsizeiptr i;
   GLubyte *dest;

   if (_mesa_bufferobj_mapped(bufObj)) {
      GLubyte *data = malloc(size);
      GLubyte *dataStart = data;
      if (data == NULL) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClearBuffer[Sub]Data");
         return;
      }

      if (clearValue == NULL) {
         /* Clear with zeros, per the spec */
         memset(data, 0, size);
      }
      else {
         for (i = 0; i < size/clearValueSize; ++i) {
            memcpy(data, clearValue, clearValueSize);
            data += clearValueSize;
         }
      }
      ctx->Driver.BufferSubData(ctx, offset, size, dataStart, bufObj);
      return;
   }

   ASSERT(ctx->Driver.MapBufferRange);
   dest = ctx->Driver.MapBufferRange(ctx, offset, size,
                                     GL_MAP_WRITE_BIT |
                                     GL_MAP_INVALIDATE_RANGE_BIT,
                                     bufObj);

   if (!dest) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClearBuffer[Sub]Data");
      return;
   }

   if (clearValue == NULL) {
      /* Clear with zeros, per the spec */
      memset(dest, 0, size);
      ctx->Driver.UnmapBuffer(ctx, bufObj);
      return;
   }

   for (i = 0; i < size/clearValueSize; ++i) {
      memcpy(dest, clearValue, clearValueSize);
      dest += clearValueSize;
   }

   ctx->Driver.UnmapBuffer(ctx, bufObj);
}


/**
 * Default fallback for \c dd_function_table::MapBufferRange().
 * Called via glMapBufferRange().
 */
static void *
_mesa_buffer_map_range( struct gl_context *ctx, GLintptr offset,
                        GLsizeiptr length, GLbitfield access,
                        struct gl_buffer_object *bufObj )
{
   (void) ctx;
   assert(!_mesa_bufferobj_mapped(bufObj));
   /* Just return a direct pointer to the data */
   bufObj->Pointer = bufObj->Data + offset;
   bufObj->Length = length;
   bufObj->Offset = offset;
   bufObj->AccessFlags = access;
   return bufObj->Pointer;
}


/**
 * Default fallback for \c dd_function_table::FlushMappedBufferRange().
 * Called via glFlushMappedBufferRange().
 */
static void
_mesa_buffer_flush_mapped_range( struct gl_context *ctx,
                                 GLintptr offset, GLsizeiptr length,
                                 struct gl_buffer_object *obj )
{
   (void) ctx;
   (void) offset;
   (void) length;
   (void) obj;
   /* no-op */
}


/**
 * Default callback for \c dd_function_table::MapBuffer().
 *
 * The input parameters will have been already tested for errors.
 *
 * \sa glUnmapBufferARB, dd_function_table::UnmapBuffer
 */
static GLboolean
_mesa_buffer_unmap( struct gl_context *ctx, struct gl_buffer_object *bufObj )
{
   (void) ctx;
   /* XXX we might assert here that bufObj->Pointer is non-null */
   bufObj->Pointer = NULL;
   bufObj->Length = 0;
   bufObj->Offset = 0;
   bufObj->AccessFlags = 0x0;
   return GL_TRUE;
}


/**
 * Default fallback for \c dd_function_table::CopyBufferSubData().
 * Called via glCopyBufferSubData().
 */
static void
_mesa_copy_buffer_subdata(struct gl_context *ctx,
                          struct gl_buffer_object *src,
                          struct gl_buffer_object *dst,
                          GLintptr readOffset, GLintptr writeOffset,
                          GLsizeiptr size)
{
   GLubyte *srcPtr, *dstPtr;

   /* the buffers should not be mapped */
   assert(!_mesa_bufferobj_mapped(src));
   assert(!_mesa_bufferobj_mapped(dst));

   if (src == dst) {
      srcPtr = dstPtr = ctx->Driver.MapBufferRange(ctx, 0, src->Size,
						   GL_MAP_READ_BIT |
						   GL_MAP_WRITE_BIT, src);

      if (!srcPtr)
	 return;

      srcPtr += readOffset;
      dstPtr += writeOffset;
   } else {
      srcPtr = ctx->Driver.MapBufferRange(ctx, readOffset, size,
					  GL_MAP_READ_BIT, src);
      dstPtr = ctx->Driver.MapBufferRange(ctx, writeOffset, size,
					  (GL_MAP_WRITE_BIT |
					   GL_MAP_INVALIDATE_RANGE_BIT), dst);
   }

   /* Note: the src and dst regions will never overlap.  Trying to do so
    * would generate GL_INVALID_VALUE earlier.
    */
   if (srcPtr && dstPtr)
      memcpy(dstPtr, srcPtr, size);

   ctx->Driver.UnmapBuffer(ctx, src);
   if (dst != src)
      ctx->Driver.UnmapBuffer(ctx, dst);
}



/**
 * Initialize the state associated with buffer objects
 */
void
_mesa_init_buffer_objects( struct gl_context *ctx )
{
   GLuint i;

   memset(&DummyBufferObject, 0, sizeof(DummyBufferObject));
   _glthread_INIT_MUTEX(DummyBufferObject.Mutex);
   DummyBufferObject.RefCount = 1000*1000*1000; /* never delete */

   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj,
                                 ctx->Shared->NullBufferObj);

   _mesa_reference_buffer_object(ctx, &ctx->CopyReadBuffer,
                                 ctx->Shared->NullBufferObj);
   _mesa_reference_buffer_object(ctx, &ctx->CopyWriteBuffer,
                                 ctx->Shared->NullBufferObj);

   _mesa_reference_buffer_object(ctx, &ctx->UniformBuffer,
				 ctx->Shared->NullBufferObj);

   _mesa_reference_buffer_object(ctx, &ctx->DrawIndirectBuffer,
				 ctx->Shared->NullBufferObj);

   for (i = 0; i < MAX_COMBINED_UNIFORM_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
				    &ctx->UniformBufferBindings[i].BufferObject,
				    ctx->Shared->NullBufferObj);
      ctx->UniformBufferBindings[i].Offset = -1;
      ctx->UniformBufferBindings[i].Size = -1;
   }
}


void
_mesa_free_buffer_objects( struct gl_context *ctx )
{
   GLuint i;

   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj, NULL);

   _mesa_reference_buffer_object(ctx, &ctx->CopyReadBuffer, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->CopyWriteBuffer, NULL);

   _mesa_reference_buffer_object(ctx, &ctx->UniformBuffer, NULL);

   _mesa_reference_buffer_object(ctx, &ctx->DrawIndirectBuffer, NULL);

   for (i = 0; i < MAX_COMBINED_UNIFORM_BUFFERS; i++) {
      _mesa_reference_buffer_object(ctx,
				    &ctx->UniformBufferBindings[i].BufferObject,
				    NULL);
   }
}

bool
_mesa_handle_bind_buffer_gen(struct gl_context *ctx,
                             GLenum target,
                             GLuint buffer,
                             struct gl_buffer_object **buf_handle,
                             const char *caller)
{
   struct gl_buffer_object *buf = *buf_handle;

   if (!buf && ctx->API == API_OPENGL_CORE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(non-gen name)", caller);
      return false;
   }

   if (!buf || buf == &DummyBufferObject) {
      /* If this is a new buffer object id, or one which was generated but
       * never used before, allocate a buffer object now.
       */
      ASSERT(ctx->Driver.NewBufferObject);
      buf = ctx->Driver.NewBufferObject(ctx, buffer, target);
      if (!buf) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
	 return false;
      }
      _mesa_HashInsert(ctx->Shared->BufferObjects, buffer, buf);
      *buf_handle = buf;
   }

   return true;
}

/**
 * Bind the specified target to buffer for the specified context.
 * Called by glBindBuffer() and other functions.
 */
static void
bind_buffer_object(struct gl_context *ctx, GLenum target, GLuint buffer)
{
   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = NULL;
   struct gl_buffer_object **bindTarget = NULL;

   bindTarget = get_buffer_target(ctx, target);
   if (!bindTarget) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target 0x%x)", target);
      return;
   }

   /* Get pointer to old buffer object (to be unbound) */
   oldBufObj = *bindTarget;
   if (oldBufObj && oldBufObj->Name == buffer && !oldBufObj->DeletePending)
      return;   /* rebinding the same buffer object- no change */

   /*
    * Get pointer to new buffer object (newBufObj)
    */
   if (buffer == 0) {
      /* The spec says there's not a buffer object named 0, but we use
       * one internally because it simplifies things.
       */
      newBufObj = ctx->Shared->NullBufferObj;
   }
   else {
      /* non-default buffer object */
      newBufObj = _mesa_lookup_bufferobj(ctx, buffer);
      if (!_mesa_handle_bind_buffer_gen(ctx, target, buffer,
                                        &newBufObj, "glBindBuffer"))
         return;
   }
   
   /* bind new buffer */
   _mesa_reference_buffer_object(ctx, bindTarget, newBufObj);
}


/**
 * Update the default buffer objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_buffer_objects(struct gl_context *ctx)
{
   /* Bind the NullBufferObj to remove references to those
    * in the shared context hash table.
    */
   bind_buffer_object( ctx, GL_ARRAY_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_PIXEL_PACK_BUFFER_ARB, 0);
   bind_buffer_object( ctx, GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}



/**
 * Return the gl_buffer_object for the given ID.
 * Always return NULL for ID 0.
 */
struct gl_buffer_object *
_mesa_lookup_bufferobj(struct gl_context *ctx, GLuint buffer)
{
   if (buffer == 0)
      return NULL;
   else
      return (struct gl_buffer_object *)
         _mesa_HashLookup(ctx->Shared->BufferObjects, buffer);
}


/**
 * If *ptr points to obj, set ptr = the Null/default buffer object.
 * This is a helper for buffer object deletion.
 * The GL spec says that deleting a buffer object causes it to get
 * unbound from all arrays in the current context.
 */
static void
unbind(struct gl_context *ctx,
       struct gl_buffer_object **ptr,
       struct gl_buffer_object *obj)
{
   if (*ptr == obj) {
      _mesa_reference_buffer_object(ctx, ptr, ctx->Shared->NullBufferObj);
   }
}


/**
 * Plug default/fallback buffer object functions into the device
 * driver hooks.
 */
void
_mesa_init_buffer_object_functions(struct dd_function_table *driver)
{
   /* GL_ARB_vertex/pixel_buffer_object */
   driver->NewBufferObject = _mesa_new_buffer_object;
   driver->DeleteBuffer = _mesa_delete_buffer_object;
   driver->BufferData = _mesa_buffer_data;
   driver->BufferSubData = _mesa_buffer_subdata;
   driver->GetBufferSubData = _mesa_buffer_get_subdata;
   driver->UnmapBuffer = _mesa_buffer_unmap;

   /* GL_ARB_clear_buffer_object */
   driver->ClearBufferSubData = _mesa_buffer_clear_subdata;

   /* GL_ARB_map_buffer_range */
   driver->MapBufferRange = _mesa_buffer_map_range;
   driver->FlushMappedBufferRange = _mesa_buffer_flush_mapped_range;

   /* GL_ARB_copy_buffer */
   driver->CopyBufferSubData = _mesa_copy_buffer_subdata;
}



/**********************************************************************/
/* API Functions                                                      */
/**********************************************************************/

void GLAPIENTRY
_mesa_BindBuffer(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBindBuffer(%s, %u)\n",
                  _mesa_lookup_enum_by_nr(target), buffer);

   bind_buffer_object(ctx, target, buffer);
}


/**
 * Delete a set of buffer objects.
 * 
 * \param n      Number of buffer objects to delete.
 * \param ids    Array of \c n buffer object IDs.
 */
void GLAPIENTRY
_mesa_DeleteBuffers(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;
   FLUSH_VERTICES(ctx, 0);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, ids[i]);
      if (bufObj) {
         struct gl_vertex_array_object *vao = ctx->Array.VAO;
         GLuint j;

         ASSERT(bufObj->Name == ids[i] || bufObj == &DummyBufferObject);

         if (_mesa_bufferobj_mapped(bufObj)) {
            /* if mapped, unmap it now */
            ctx->Driver.UnmapBuffer(ctx, bufObj);
            bufObj->AccessFlags = 0;
            bufObj->Pointer = NULL;
         }

         /* unbind any vertex pointers bound to this buffer */
         for (j = 0; j < Elements(vao->VertexBinding); j++) {
            unbind(ctx, &vao->VertexBinding[j].BufferObj, bufObj);
         }

         if (ctx->Array.ArrayBufferObj == bufObj) {
            _mesa_BindBuffer( GL_ARRAY_BUFFER_ARB, 0 );
         }
         if (vao->IndexBufferObj == bufObj) {
            _mesa_BindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
         }

         /* unbind ARB_draw_indirect binding point */
         if (ctx->DrawIndirectBuffer == bufObj) {
            _mesa_BindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
         }

         /* unbind ARB_copy_buffer binding points */
         if (ctx->CopyReadBuffer == bufObj) {
            _mesa_BindBuffer( GL_COPY_READ_BUFFER, 0 );
         }
         if (ctx->CopyWriteBuffer == bufObj) {
            _mesa_BindBuffer( GL_COPY_WRITE_BUFFER, 0 );
         }

         /* unbind transform feedback binding points */
         if (ctx->TransformFeedback.CurrentBuffer == bufObj) {
            _mesa_BindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
         }
         for (j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
            if (ctx->TransformFeedback.CurrentObject->Buffers[j] == bufObj) {
               _mesa_BindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, j, 0 );
            }
         }

         /* unbind UBO binding points */
         for (j = 0; j < ctx->Const.MaxUniformBufferBindings; j++) {
            if (ctx->UniformBufferBindings[j].BufferObject == bufObj) {
               _mesa_BindBufferBase( GL_UNIFORM_BUFFER, j, 0 );
            }
         }

         if (ctx->UniformBuffer == bufObj) {
            _mesa_BindBuffer( GL_UNIFORM_BUFFER, 0 );
         }

         /* unbind any pixel pack/unpack pointers bound to this buffer */
         if (ctx->Pack.BufferObj == bufObj) {
            _mesa_BindBuffer( GL_PIXEL_PACK_BUFFER_EXT, 0 );
         }
         if (ctx->Unpack.BufferObj == bufObj) {
            _mesa_BindBuffer( GL_PIXEL_UNPACK_BUFFER_EXT, 0 );
         }

         if (ctx->Texture.BufferObject == bufObj) {
            _mesa_BindBuffer( GL_TEXTURE_BUFFER, 0 );
         }

         /* The ID is immediately freed for re-use */
         _mesa_HashRemove(ctx->Shared->BufferObjects, ids[i]);
         /* Make sure we do not run into the classic ABA problem on bind.
          * We don't want to allow re-binding a buffer object that's been
          * "deleted" by glDeleteBuffers().
          *
          * The explicit rebinding to the default object in the current context
          * prevents the above in the current context, but another context
          * sharing the same objects might suffer from this problem.
          * The alternative would be to do the hash lookup in any case on bind
          * which would introduce more runtime overhead than this.
          */
         bufObj->DeletePending = GL_TRUE;
         _mesa_reference_buffer_object(ctx, &bufObj, NULL);
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Generate a set of unique buffer object IDs and store them in \c buffer.
 * 
 * \param n       Number of IDs to generate.
 * \param buffer  Array of \c n locations to store the IDs.
 */
void GLAPIENTRY
_mesa_GenBuffers(GLsizei n, GLuint *buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGenBuffers(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenBuffersARB");
      return;
   }

   if (!buffer) {
      return;
   }

   /*
    * This must be atomic (generation and allocation of buffer object IDs)
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->BufferObjects, n);

   /* Insert the ID and pointer to dummy buffer object into hash table */
   for (i = 0; i < n; i++) {
      _mesa_HashInsert(ctx->Shared->BufferObjects, first + i,
                       &DummyBufferObject);
      buffer[i] = first + i;
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
}


/**
 * Determine if ID is the name of a buffer object.
 * 
 * \param id  ID of the potential buffer object.
 * \return  \c GL_TRUE if \c id is the name of a buffer object, 
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsBuffer(GLuint id)
{
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   bufObj = _mesa_lookup_bufferobj(ctx, id);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return bufObj && bufObj != &DummyBufferObject;
}


void GLAPIENTRY
_mesa_BufferStorage(GLenum target, GLsizeiptr size, const GLvoid *data,
                    GLbitfield flags)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   if (size <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferStorage(size <= 0)");
      return;
   }

   if (flags & ~(GL_MAP_READ_BIT |
                 GL_MAP_WRITE_BIT |
                 GL_MAP_PERSISTENT_BIT |
                 GL_MAP_COHERENT_BIT |
                 GL_DYNAMIC_STORAGE_BIT |
                 GL_CLIENT_STORAGE_BIT)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferStorage(flags)");
      return;
   }

   if (flags & GL_MAP_PERSISTENT_BIT &&
       !(flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferStorage(flags!=READ/WRITE)");
      return;
   }

   if (flags & GL_MAP_COHERENT_BIT && !(flags & GL_MAP_PERSISTENT_BIT)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferStorage(flags!=PERSISTENT)");
      return;
   }

   bufObj = get_buffer(ctx, "glBufferStorage", target, GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   if (bufObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferStorage(immutable)");
      return;
   }

   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
      ctx->Driver.UnmapBuffer(ctx, bufObj);
      bufObj->AccessFlags = 0;
      ASSERT(bufObj->Pointer == NULL);
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFER_OBJECT);

   bufObj->Written = GL_TRUE;
   bufObj->Immutable = GL_TRUE;

   ASSERT(ctx->Driver.BufferData);
   if (!ctx->Driver.BufferData(ctx, target, size, data, GL_DYNAMIC_DRAW,
                               flags, bufObj)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBufferStorage()");
   }
}


void GLAPIENTRY
_mesa_BufferData(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   bool valid_usage;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBufferData(%s, %ld, %p, %s)\n",
                  _mesa_lookup_enum_by_nr(target),
                  (long int) size, data,
                  _mesa_lookup_enum_by_nr(usage));

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBufferDataARB(size < 0)");
      return;
   }

   switch (usage) {
   case GL_STREAM_DRAW_ARB:
      valid_usage = (ctx->API != API_OPENGLES);
      break;

   case GL_STATIC_DRAW_ARB:
   case GL_DYNAMIC_DRAW_ARB:
      valid_usage = true;
      break;

   case GL_STREAM_READ_ARB:
   case GL_STREAM_COPY_ARB:
   case GL_STATIC_READ_ARB:
   case GL_STATIC_COPY_ARB:
   case GL_DYNAMIC_READ_ARB:
   case GL_DYNAMIC_COPY_ARB:
      valid_usage = _mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx);
      break;

   default:
      valid_usage = false;
      break;
   }

   if (!valid_usage) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBufferData(usage)");
      return;
   }

   bufObj = get_buffer(ctx, "glBufferDataARB", target, GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   if (bufObj->Immutable) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferData(immutable)");
      return;
   }

   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
      ctx->Driver.UnmapBuffer(ctx, bufObj);
      bufObj->AccessFlags = 0;
      ASSERT(bufObj->Pointer == NULL);
   }  

   FLUSH_VERTICES(ctx, _NEW_BUFFER_OBJECT);

   bufObj->Written = GL_TRUE;

#ifdef VBO_DEBUG
   printf("glBufferDataARB(%u, sz %ld, from %p, usage 0x%x)\n",
                bufObj->Name, size, data, usage);
#endif

#ifdef BOUNDS_CHECK
   size += 100;
#endif

   ASSERT(ctx->Driver.BufferData);
   if (!ctx->Driver.BufferData(ctx, target, size, data, usage,
                               GL_MAP_READ_BIT |
                               GL_MAP_WRITE_BIT |
                               GL_DYNAMIC_STORAGE_BIT,
                               bufObj)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBufferDataARB()");
   }
}


void GLAPIENTRY
_mesa_BufferSubData(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              false, GL_INVALID_OPERATION,
                                              "glBufferSubDataARB" );
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   if (bufObj->Immutable &&
       !(bufObj->StorageFlags & GL_DYNAMIC_STORAGE_BIT)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferSubData");
      return;
   }

   if (size == 0)
      return;

   bufObj->Written = GL_TRUE;

   ASSERT(ctx->Driver.BufferSubData);
   ctx->Driver.BufferSubData( ctx, offset, size, data, bufObj );
}


void GLAPIENTRY
_mesa_GetBufferSubData(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   bufObj = buffer_object_subdata_range_good(ctx, target, offset, size,
                                             false, GL_INVALID_OPERATION,
                                             "glGetBufferSubDataARB");
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   ASSERT(ctx->Driver.GetBufferSubData);
   ctx->Driver.GetBufferSubData( ctx, offset, size, data, bufObj );
}


void GLAPIENTRY
_mesa_ClearBufferData(GLenum target, GLenum internalformat, GLenum format,
                      GLenum type, const GLvoid* data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object* bufObj;
   mesa_format mesaFormat;
   GLubyte clearValue[MAX_PIXEL_BYTES];
   GLsizeiptr clearValueSize;

   bufObj = get_buffer(ctx, "glClearBufferData", target, GL_INVALID_VALUE);
   if (!bufObj) {
      return;
   }

   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glClearBufferData(buffer currently mapped)");
      return;
   }

   mesaFormat = validate_clear_buffer_format(ctx, internalformat,
                                             format, type,
                                             "glClearBufferData");
   if (mesaFormat == MESA_FORMAT_NONE) {
      return;
   }

   clearValueSize = _mesa_get_format_bytes(mesaFormat);
   if (bufObj->Size % clearValueSize != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glClearBufferData(size is not a multiple of "
                  "internalformat size)");
      return;
   }

   if (data == NULL) {
      /* clear to zeros, per the spec */
      ctx->Driver.ClearBufferSubData(ctx, 0, bufObj->Size,
                                     NULL, 0, bufObj);
      return;
   }

   if (!convert_clear_buffer_data(ctx, mesaFormat, clearValue,
                                  format, type, data, "glClearBufferData")) {
      return;
   }

   ctx->Driver.ClearBufferSubData(ctx, 0, bufObj->Size,
                                  clearValue, clearValueSize, bufObj);
}


void GLAPIENTRY
_mesa_ClearBufferSubData(GLenum target, GLenum internalformat,
                         GLintptr offset, GLsizeiptr size,
                         GLenum format, GLenum type,
                         const GLvoid* data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object* bufObj;
   mesa_format mesaFormat;
   GLubyte clearValue[MAX_PIXEL_BYTES];
   GLsizeiptr clearValueSize;

   bufObj = buffer_object_subdata_range_good(ctx, target, offset, size,
                                             true, GL_INVALID_VALUE,
                                             "glClearBufferSubData");
   if (!bufObj) {
      return;
   }

   mesaFormat = validate_clear_buffer_format(ctx, internalformat,
                                             format, type,
                                             "glClearBufferSubData");
   if (mesaFormat == MESA_FORMAT_NONE) {
      return;
   }

   clearValueSize = _mesa_get_format_bytes(mesaFormat);
   if (offset % clearValueSize != 0 || size % clearValueSize != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glClearBufferSubData(offset or size is not a multiple of "
                  "internalformat size)");
      return;
   }

   if (data == NULL) {
      /* clear to zeros, per the spec */
      ctx->Driver.ClearBufferSubData(ctx, offset, size,
                                     NULL, 0, bufObj);
      return;
   }

   if (!convert_clear_buffer_data(ctx, mesaFormat, clearValue,
                                  format, type, data,
                                  "glClearBufferSubData")) {
      return;
   }

   ctx->Driver.ClearBufferSubData(ctx, offset, size,
                                  clearValue, clearValueSize, bufObj);
}


void * GLAPIENTRY
_mesa_MapBuffer(GLenum target, GLenum access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   GLbitfield accessFlags;
   void *map;
   bool valid_access;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   switch (access) {
   case GL_READ_ONLY_ARB:
      accessFlags = GL_MAP_READ_BIT;
      valid_access = _mesa_is_desktop_gl(ctx);
      break;
   case GL_WRITE_ONLY_ARB:
      accessFlags = GL_MAP_WRITE_BIT;
      valid_access = true;
      break;
   case GL_READ_WRITE_ARB:
      accessFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
      valid_access = _mesa_is_desktop_gl(ctx);
      break;
   default:
      valid_access = false;
      break;
   }

   if (!valid_access) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(access)");
      return NULL;
   }

   bufObj = get_buffer(ctx, "glMapBufferARB", target, GL_INVALID_OPERATION);
   if (!bufObj)
      return NULL;

   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(already mapped)");
      return NULL;
   }

   if (!bufObj->Size) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glMapBuffer(buffer size = 0)");
      return NULL;
   }

   ASSERT(ctx->Driver.MapBufferRange);
   map = ctx->Driver.MapBufferRange(ctx, 0, bufObj->Size, accessFlags, bufObj);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMapBufferARB(map failed)");
      return NULL;
   }
   else {
      /* The driver callback should have set these fields.
       * This is important because other modules (like VBO) might call
       * the driver function directly.
       */
      ASSERT(bufObj->Pointer == map);
      ASSERT(bufObj->Length == bufObj->Size);
      ASSERT(bufObj->Offset == 0);
      bufObj->AccessFlags = accessFlags;
   }

   if (access == GL_WRITE_ONLY_ARB || access == GL_READ_WRITE_ARB)
      bufObj->Written = GL_TRUE;

#ifdef VBO_DEBUG
   printf("glMapBufferARB(%u, sz %ld, access 0x%x)\n",
	  bufObj->Name, bufObj->Size, access);
   if (access == GL_WRITE_ONLY_ARB) {
      GLuint i;
      GLubyte *b = (GLubyte *) bufObj->Pointer;
      for (i = 0; i < bufObj->Size; i++)
         b[i] = i & 0xff;
   }
#endif

#ifdef BOUNDS_CHECK
   {
      GLubyte *buf = (GLubyte *) bufObj->Pointer;
      GLuint i;
      /* buffer is 100 bytes larger than requested, fill with magic value */
      for (i = 0; i < 100; i++) {
         buf[bufObj->Size - i - 1] = 123;
      }
   }
#endif

   return bufObj->Pointer;
}


GLboolean GLAPIENTRY
_mesa_UnmapBuffer(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   GLboolean status = GL_TRUE;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   bufObj = get_buffer(ctx, "glUnmapBufferARB", target, GL_INVALID_OPERATION);
   if (!bufObj)
      return GL_FALSE;

   if (!_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB");
      return GL_FALSE;
   }

#ifdef BOUNDS_CHECK
   if (bufObj->Access != GL_READ_ONLY_ARB) {
      GLubyte *buf = (GLubyte *) bufObj->Pointer;
      GLuint i;
      /* check that last 100 bytes are still = magic value */
      for (i = 0; i < 100; i++) {
         GLuint pos = bufObj->Size - i - 1;
         if (buf[pos] != 123) {
            _mesa_warning(ctx, "Out of bounds buffer object write detected"
                          " at position %d (value = %u)\n",
                          pos, buf[pos]);
         }
      }
   }
#endif

#ifdef VBO_DEBUG
   if (bufObj->AccessFlags & GL_MAP_WRITE_BIT) {
      GLuint i, unchanged = 0;
      GLubyte *b = (GLubyte *) bufObj->Pointer;
      GLint pos = -1;
      /* check which bytes changed */
      for (i = 0; i < bufObj->Size - 1; i++) {
         if (b[i] == (i & 0xff) && b[i+1] == ((i+1) & 0xff)) {
            unchanged++;
            if (pos == -1)
               pos = i;
         }
      }
      if (unchanged) {
         printf("glUnmapBufferARB(%u): %u of %ld unchanged, starting at %d\n",
                      bufObj->Name, unchanged, bufObj->Size, pos);
      }
   }
#endif

   status = ctx->Driver.UnmapBuffer( ctx, bufObj );
   bufObj->AccessFlags = 0;
   ASSERT(bufObj->Pointer == NULL);
   ASSERT(bufObj->Offset == 0);
   ASSERT(bufObj->Length == 0);

   return status;
}


void GLAPIENTRY
_mesa_GetBufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   bufObj = get_buffer(ctx, "glGetBufferParameterivARB", target,
                       GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = (GLint) bufObj->Size;
      return;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      return;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(ctx, bufObj->AccessFlags);
      return;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      return;
   case GL_BUFFER_ACCESS_FLAGS:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->AccessFlags;
      return;
   case GL_BUFFER_MAP_OFFSET:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = (GLint) bufObj->Offset;
      return;
   case GL_BUFFER_MAP_LENGTH:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = (GLint) bufObj->Length;
      return;
   case GL_BUFFER_IMMUTABLE_STORAGE:
      if (!ctx->Extensions.ARB_buffer_storage)
         goto invalid_pname;
      *params = bufObj->Immutable;
      return;
   case GL_BUFFER_STORAGE_FLAGS:
      if (!ctx->Extensions.ARB_buffer_storage)
         goto invalid_pname;
      *params = bufObj->StorageFlags;
      return;
   default:
      ; /* fall-through */
   }

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterivARB(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
}


/**
 * New in GL 3.2
 * This is pretty much a duplicate of GetBufferParameteriv() but the
 * GL_BUFFER_SIZE_ARB attribute will be 64-bits on a 64-bit system.
 */
void GLAPIENTRY
_mesa_GetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   bufObj = get_buffer(ctx, "glGetBufferParameteri64v", target,
                       GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = bufObj->Size;
      return;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      return;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(ctx, bufObj->AccessFlags);
      return;
   case GL_BUFFER_ACCESS_FLAGS:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->AccessFlags;
      return;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      return;
   case GL_BUFFER_MAP_OFFSET:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->Offset;
      return;
   case GL_BUFFER_MAP_LENGTH:
      if (!ctx->Extensions.ARB_map_buffer_range)
         goto invalid_pname;
      *params = bufObj->Length;
      return;
   case GL_BUFFER_IMMUTABLE_STORAGE:
      if (!ctx->Extensions.ARB_buffer_storage)
         goto invalid_pname;
      *params = bufObj->Immutable;
      return;
   case GL_BUFFER_STORAGE_FLAGS:
      if (!ctx->Extensions.ARB_buffer_storage)
         goto invalid_pname;
      *params = bufObj->StorageFlags;
      return;
   default:
      ; /* fall-through */
   }

invalid_pname:
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameteri64v(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
}


void GLAPIENTRY
_mesa_GetBufferPointerv(GLenum target, GLenum pname, GLvoid **params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;

   if (pname != GL_BUFFER_MAP_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(pname)");
      return;
   }

   bufObj = get_buffer(ctx, "glGetBufferPointervARB", target,
                       GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   *params = bufObj->Pointer;
}


void GLAPIENTRY
_mesa_CopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                        GLintptr readOffset, GLintptr writeOffset,
                        GLsizeiptr size)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *src, *dst;

   src = get_buffer(ctx, "glCopyBufferSubData", readTarget,
                    GL_INVALID_OPERATION);
   if (!src)
      return;

   dst = get_buffer(ctx, "glCopyBufferSubData", writeTarget,
                    GL_INVALID_OPERATION);
   if (!dst)
      return;

   if (_mesa_bufferobj_mapped(src)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyBufferSubData(readBuffer is mapped)");
      return;
   }

   if (_mesa_bufferobj_mapped(dst)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyBufferSubData(writeBuffer is mapped)");
      return;
   }

   if (readOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBufferSubData(readOffset = %d)", (int) readOffset);
      return;
   }

   if (writeOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBufferSubData(writeOffset = %d)", (int) writeOffset);
      return;
   }

   if (size < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBufferSubData(writeOffset = %d)", (int) size);
      return;
   }

   if (readOffset + size > src->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBufferSubData(readOffset + size = %d)",
                  (int) (readOffset + size));
      return;
   }

   if (writeOffset + size > dst->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBufferSubData(writeOffset + size = %d)",
                  (int) (writeOffset + size));
      return;
   }

   if (src == dst) {
      if (readOffset + size <= writeOffset) {
         /* OK */
      }
      else if (writeOffset + size <= readOffset) {
         /* OK */
      }
      else {
         /* overlapping src/dst is illegal */
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyBufferSubData(overlapping src/dst)");
         return;
      }
   }

   ctx->Driver.CopyBufferSubData(ctx, src, dst, readOffset, writeOffset, size);
}


/**
 * See GL_ARB_map_buffer_range spec
 */
void * GLAPIENTRY
_mesa_MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                     GLbitfield access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   void *map;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(extension not supported)");
      return NULL;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(offset = %ld)", (long)offset);
      return NULL;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(length = %ld)", (long)length);
      return NULL;
   }

   /* Page 38 of the PDF of the OpenGL ES 3.0 spec says:
    *
    *     "An INVALID_OPERATION error is generated for any of the following
    *     conditions:
    *
    *     * <length> is zero."
    */
   if (_mesa_is_gles(ctx) && length == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(length = 0)");
      return NULL;
   }

   if (access & ~(GL_MAP_READ_BIT |
                  GL_MAP_WRITE_BIT |
                  GL_MAP_INVALIDATE_RANGE_BIT |
                  GL_MAP_INVALIDATE_BUFFER_BIT |
                  GL_MAP_FLUSH_EXPLICIT_BIT |
                  GL_MAP_UNSYNCHRONIZED_BIT)) {
      /* generate an error if any undefind bit is set */
      _mesa_error(ctx, GL_INVALID_VALUE, "glMapBufferRange(access)");
      return NULL;
   }

   if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(access indicates neither read or write)");
      return NULL;
   }

   if ((access & GL_MAP_READ_BIT) &&
       (access & (GL_MAP_INVALIDATE_RANGE_BIT |
                  GL_MAP_INVALIDATE_BUFFER_BIT |
                  GL_MAP_UNSYNCHRONIZED_BIT))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(invalid access flags)");
      return NULL;
   }

   if ((access & GL_MAP_FLUSH_EXPLICIT_BIT) &&
       ((access & GL_MAP_WRITE_BIT) == 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(invalid access flags)");
      return NULL;
   }

   bufObj = get_buffer(ctx, "glMapBufferRange", target, GL_INVALID_OPERATION);
   if (!bufObj)
      return NULL;

   if (offset + length > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(offset + length > size)");
      return NULL;
   }

   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(buffer already mapped)");
      return NULL;
   }

   if (!bufObj->Size) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glMapBufferRange(buffer size = 0)");
      return NULL;
   }

   /* Mapping zero bytes should return a non-null pointer. */
   if (!length) {
      static long dummy = 0;
      bufObj->Pointer = &dummy;
      bufObj->Length = length;
      bufObj->Offset = offset;
      bufObj->AccessFlags = access;
      return bufObj->Pointer;
   }

   ASSERT(ctx->Driver.MapBufferRange);
   map = ctx->Driver.MapBufferRange(ctx, offset, length, access, bufObj);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMapBufferARB(map failed)");
   }
   else {
      /* The driver callback should have set all these fields.
       * This is important because other modules (like VBO) might call
       * the driver function directly.
       */
      ASSERT(bufObj->Pointer == map);
      ASSERT(bufObj->Length == length);
      ASSERT(bufObj->Offset == offset);
      ASSERT(bufObj->AccessFlags == access);
   }

   return map;
}


/**
 * See GL_ARB_map_buffer_range spec
 */
void GLAPIENTRY
_mesa_FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(extension not supported)");
      return;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFlushMappedBufferRange(offset = %ld)", (long)offset);
      return;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glFlushMappedBufferRange(length = %ld)", (long)length);
      return;
   }

   bufObj = get_buffer(ctx, "glFlushMappedBufferRange", target,
                       GL_INVALID_OPERATION);
   if (!bufObj)
      return;

   if (!_mesa_bufferobj_mapped(bufObj)) {
      /* buffer is not mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(buffer is not mapped)");
      return;
   }

   if ((bufObj->AccessFlags & GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFlushMappedBufferRange(GL_MAP_FLUSH_EXPLICIT_BIT not set)");
      return;
   }

   if (offset + length > bufObj->Length) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "glFlushMappedBufferRange(offset %ld + length %ld > mapped length %ld)",
		  (long)offset, (long)length, (long)bufObj->Length);
      return;
   }

   ASSERT(bufObj->AccessFlags & GL_MAP_WRITE_BIT);

   if (ctx->Driver.FlushMappedBufferRange)
      ctx->Driver.FlushMappedBufferRange(ctx, offset, length, bufObj);
}


static GLenum
buffer_object_purgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_buffer_object *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_bufferobj(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectPurgeable(name = 0x%x)", name);
      return 0;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glObjectPurgeable(buffer 0)" );
      return 0;
   }

   if (bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectPurgeable(name = 0x%x) is already purgeable", name);
      return GL_VOLATILE_APPLE;
   }

   bufObj->Purgeable = GL_TRUE;

   retval = GL_VOLATILE_APPLE;
   if (ctx->Driver.BufferObjectPurgeable)
      retval = ctx->Driver.BufferObjectPurgeable(ctx, bufObj, option);

   return retval;
}


static GLenum
renderbuffer_purgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_renderbuffer *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_renderbuffer(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return 0;
   }

   if (bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectPurgeable(name = 0x%x) is already purgeable", name);
      return GL_VOLATILE_APPLE;
   }

   bufObj->Purgeable = GL_TRUE;

   retval = GL_VOLATILE_APPLE;
   if (ctx->Driver.RenderObjectPurgeable)
      retval = ctx->Driver.RenderObjectPurgeable(ctx, bufObj, option);

   return retval;
}


static GLenum
texture_object_purgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_texture_object *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_texture(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectPurgeable(name = 0x%x)", name);
      return 0;
   }

   if (bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectPurgeable(name = 0x%x) is already purgeable", name);
      return GL_VOLATILE_APPLE;
   }

   bufObj->Purgeable = GL_TRUE;

   retval = GL_VOLATILE_APPLE;
   if (ctx->Driver.TextureObjectPurgeable)
      retval = ctx->Driver.TextureObjectPurgeable(ctx, bufObj, option);

   return retval;
}


GLenum GLAPIENTRY
_mesa_ObjectPurgeableAPPLE(GLenum objectType, GLuint name, GLenum option)
{
   GLenum retval;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectPurgeable(name = 0x%x)", name);
      return 0;
   }

   switch (option) {
   case GL_VOLATILE_APPLE:
   case GL_RELEASED_APPLE:
      /* legal */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glObjectPurgeable(name = 0x%x) invalid option: %d",
                  name, option);
      return 0;
   }

   switch (objectType) {
   case GL_TEXTURE:
      retval = texture_object_purgeable(ctx, name, option);
      break;
   case GL_RENDERBUFFER_EXT:
      retval = renderbuffer_purgeable(ctx, name, option);
      break;
   case GL_BUFFER_OBJECT_APPLE:
      retval = buffer_object_purgeable(ctx, name, option);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glObjectPurgeable(name = 0x%x) invalid type: %d",
                  name, objectType);
      return 0;
   }

   /* In strict conformance to the spec, we must only return VOLATILE when
    * when passed the VOLATILE option. Madness.
    *
    * XXX First fix the spec, then fix me.
    */
   return option == GL_VOLATILE_APPLE ? GL_VOLATILE_APPLE : retval;
}


static GLenum
buffer_object_unpurgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_buffer_object *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_bufferobj(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return 0;
   }

   if (! bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectUnpurgeable(name = 0x%x) object is "
                  " already \"unpurged\"", name);
      return 0;
   }

   bufObj->Purgeable = GL_FALSE;

   retval = option;
   if (ctx->Driver.BufferObjectUnpurgeable)
      retval = ctx->Driver.BufferObjectUnpurgeable(ctx, bufObj, option);

   return retval;
}


static GLenum
renderbuffer_unpurgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_renderbuffer *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_renderbuffer(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return 0;
   }

   if (! bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectUnpurgeable(name = 0x%x) object is "
                  " already \"unpurged\"", name);
      return 0;
   }

   bufObj->Purgeable = GL_FALSE;

   retval = option;
   if (ctx->Driver.RenderObjectUnpurgeable)
      retval = ctx->Driver.RenderObjectUnpurgeable(ctx, bufObj, option);

   return retval;
}


static GLenum
texture_object_unpurgeable(struct gl_context *ctx, GLuint name, GLenum option)
{
   struct gl_texture_object *bufObj;
   GLenum retval;

   bufObj = _mesa_lookup_texture(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return 0;
   }

   if (! bufObj->Purgeable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glObjectUnpurgeable(name = 0x%x) object is"
                  " already \"unpurged\"", name);
      return 0;
   }

   bufObj->Purgeable = GL_FALSE;

   retval = option;
   if (ctx->Driver.TextureObjectUnpurgeable)
      retval = ctx->Driver.TextureObjectUnpurgeable(ctx, bufObj, option);

   return retval;
}


GLenum GLAPIENTRY
_mesa_ObjectUnpurgeableAPPLE(GLenum objectType, GLuint name, GLenum option)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return 0;
   }

   switch (option) {
   case GL_RETAINED_APPLE:
   case GL_UNDEFINED_APPLE:
      /* legal */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glObjectUnpurgeable(name = 0x%x) invalid option: %d",
                  name, option);
      return 0;
   }

   switch (objectType) {
   case GL_BUFFER_OBJECT_APPLE:
      return buffer_object_unpurgeable(ctx, name, option);
   case GL_TEXTURE:
      return texture_object_unpurgeable(ctx, name, option);
   case GL_RENDERBUFFER_EXT:
      return renderbuffer_unpurgeable(ctx, name, option);
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glObjectUnpurgeable(name = 0x%x) invalid type: %d",
                  name, objectType);
      return 0;
   }
}


static void
get_buffer_object_parameteriv(struct gl_context *ctx, GLuint name,
                              GLenum pname, GLint *params)
{
   struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetObjectParameteriv(name = 0x%x) invalid object", name);
      return;
   }

   switch (pname) {
   case GL_PURGEABLE_APPLE:
      *params = bufObj->Purgeable;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetObjectParameteriv(name = 0x%x) invalid enum: %d",
                  name, pname);
      break;
   }
}


static void
get_renderbuffer_parameteriv(struct gl_context *ctx, GLuint name,
                             GLenum pname, GLint *params)
{
   struct gl_renderbuffer *rb = _mesa_lookup_renderbuffer(ctx, name);
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return;
   }

   switch (pname) {
   case GL_PURGEABLE_APPLE:
      *params = rb->Purgeable;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetObjectParameteriv(name = 0x%x) invalid enum: %d",
                  name, pname);
      break;
   }
}


static void
get_texture_object_parameteriv(struct gl_context *ctx, GLuint name,
                               GLenum pname, GLint *params)
{
   struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, name);
   if (!texObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
      return;
   }

   switch (pname) {
   case GL_PURGEABLE_APPLE:
      *params = texObj->Purgeable;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetObjectParameteriv(name = 0x%x) invalid enum: %d",
                  name, pname);
      break;
   }
}


void GLAPIENTRY
_mesa_GetObjectParameterivAPPLE(GLenum objectType, GLuint name, GLenum pname,
                                GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetObjectParameteriv(name = 0x%x)", name);
      return;
   }

   switch (objectType) {
   case GL_TEXTURE:
      get_texture_object_parameteriv(ctx, name, pname, params);
      break;
   case GL_BUFFER_OBJECT_APPLE:
      get_buffer_object_parameteriv(ctx, name, pname, params);
      break;
   case GL_RENDERBUFFER_EXT:
      get_renderbuffer_parameteriv(ctx, name, pname, params);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetObjectParameteriv(name = 0x%x) invalid type: %d",
                  name, objectType);
   }
}

static void
set_ubo_binding(struct gl_context *ctx,
		int index,
		struct gl_buffer_object *bufObj,
		GLintptr offset,
		GLsizeiptr size,
		GLboolean autoSize)
{
   struct gl_uniform_buffer_binding *binding;

   binding = &ctx->UniformBufferBindings[index];
   if (binding->BufferObject == bufObj &&
       binding->Offset == offset &&
       binding->Size == size &&
       binding->AutomaticSize == autoSize) {
      return;
   }

   FLUSH_VERTICES(ctx, 0);
   ctx->NewDriverState |= ctx->DriverFlags.NewUniformBuffer;

   _mesa_reference_buffer_object(ctx, &binding->BufferObject, bufObj);
   binding->Offset = offset;
   binding->Size = size;
   binding->AutomaticSize = autoSize;
}

/**
 * Bind a region of a buffer object to a uniform block binding point.
 * \param index  the uniform buffer binding point index
 * \param bufObj  the buffer object
 * \param offset  offset to the start of buffer object region
 * \param size  size of the buffer object region
 */
static void
bind_buffer_range_uniform_buffer(struct gl_context *ctx,
				 GLuint index,
				 struct gl_buffer_object *bufObj,
				 GLintptr offset,
				 GLsizeiptr size)
{
   if (index >= ctx->Const.MaxUniformBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
      return;
   }

   if (offset & (ctx->Const.UniformBufferOffsetAlignment - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferRange(offset misalgned %d/%d)", (int) offset,
		  ctx->Const.UniformBufferOffsetAlignment);
      return;
   }

   if (bufObj == ctx->Shared->NullBufferObj) {
      offset = -1;
      size = -1;
   }

   _mesa_reference_buffer_object(ctx, &ctx->UniformBuffer, bufObj);
   set_ubo_binding(ctx, index, bufObj, offset, size, GL_FALSE);
}


/**
 * Bind a buffer object to a uniform block binding point.
 * As above, but offset = 0.
 */
static void
bind_buffer_base_uniform_buffer(struct gl_context *ctx,
				GLuint index,
				struct gl_buffer_object *bufObj)
{
   if (index >= ctx->Const.MaxUniformBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
      return;
   }

   _mesa_reference_buffer_object(ctx, &ctx->UniformBuffer, bufObj);
   if (bufObj == ctx->Shared->NullBufferObj)
      set_ubo_binding(ctx, index, bufObj, -1, -1, GL_TRUE);
   else
      set_ubo_binding(ctx, index, bufObj, 0, 0, GL_TRUE);
}

static void
set_atomic_buffer_binding(struct gl_context *ctx,
                          unsigned index,
                          struct gl_buffer_object *bufObj,
                          GLintptr offset,
                          GLsizeiptr size,
                          const char *name)
{
   struct gl_atomic_buffer_binding *binding;

   if (index >= ctx->Const.MaxAtomicBufferBindings) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%d)", name, index);
      return;
   }

   if (offset & (ATOMIC_COUNTER_SIZE - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(offset misalgned %d/%d)", name, (int) offset,
                  ATOMIC_COUNTER_SIZE);
      return;
   }

   _mesa_reference_buffer_object(ctx, &ctx->AtomicBuffer, bufObj);

   binding = &ctx->AtomicBufferBindings[index];
   if (binding->BufferObject == bufObj &&
       binding->Offset == offset &&
       binding->Size == size) {
      return;
   }

   FLUSH_VERTICES(ctx, 0);
   ctx->NewDriverState |= ctx->DriverFlags.NewAtomicBuffer;

   _mesa_reference_buffer_object(ctx, &binding->BufferObject, bufObj);

   if (bufObj == ctx->Shared->NullBufferObj) {
      binding->Offset = -1;
      binding->Size = -1;
   } else {
      binding->Offset = offset;
      binding->Size = size;
   }
}

void GLAPIENTRY
_mesa_BindBufferRange(GLenum target, GLuint index,
                      GLuint buffer, GLintptr offset, GLsizeiptr size)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   if (buffer == 0) {
      bufObj = ctx->Shared->NullBufferObj;
   } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   }
   if (!_mesa_handle_bind_buffer_gen(ctx, target, buffer,
                                     &bufObj, "glBindBufferRange"))
      return;

   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(invalid buffer=%u)", buffer);
      return;
   }

   if (buffer != 0) {
      if (size <= 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(size=%d)",
                     (int) size);
         return;
      }
   }

   switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      _mesa_bind_buffer_range_transform_feedback(ctx, index, bufObj,
						 offset, size);
      return;
   case GL_UNIFORM_BUFFER:
      bind_buffer_range_uniform_buffer(ctx, index, bufObj, offset, size);
      return;
   case GL_ATOMIC_COUNTER_BUFFER:
      set_atomic_buffer_binding(ctx, index, bufObj, offset, size,
                                "glBindBufferRange");
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferRange(target)");
      return;
   }
}

void GLAPIENTRY
_mesa_BindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   if (buffer == 0) {
      bufObj = ctx->Shared->NullBufferObj;
   } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   }
   if (!_mesa_handle_bind_buffer_gen(ctx, target, buffer,
                                     &bufObj, "glBindBufferBase"))
      return;

   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferBase(invalid buffer=%u)", buffer);
      return;
   }

   /* Note that there's some oddness in the GL 3.1-GL 3.3 specifications with
    * regards to BindBufferBase.  It says (GL 3.1 core spec, page 63):
    *
    *     "BindBufferBase is equivalent to calling BindBufferRange with offset
    *      zero and size equal to the size of buffer."
    *
    * but it says for glGetIntegeri_v (GL 3.1 core spec, page 230):
    *
    *     "If the parameter (starting offset or size) was not specified when the
    *      buffer object was bound, zero is returned."
    *
    * What happens if the size of the buffer changes?  Does the size of the
    * buffer at the moment glBindBufferBase was called still play a role, like
    * the first quote would imply, or is the size meaningless in the
    * glBindBufferBase case like the second quote would suggest?  The GL 4.1
    * core spec page 45 says:
    *
    *     "It is equivalent to calling BindBufferRange with offset zero, while
    *      size is determined by the size of the bound buffer at the time the
    *      binding is used."
    *
    * My interpretation is that the GL 4.1 spec was a clarification of the
    * behavior, not a change.  In particular, this choice will only make
    * rendering work in cases where it would have had undefined results.
    */

   switch (target) {
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      _mesa_bind_buffer_base_transform_feedback(ctx, index, bufObj);
      return;
   case GL_UNIFORM_BUFFER:
      bind_buffer_base_uniform_buffer(ctx, index, bufObj);
      return;
   case GL_ATOMIC_COUNTER_BUFFER:
      set_atomic_buffer_binding(ctx, index, bufObj, 0, 0,
                                "glBindBufferBase");
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferBase(target)");
      return;
   }
}

void GLAPIENTRY
_mesa_InvalidateBufferSubData(GLuint buffer, GLintptr offset,
                              GLsizeiptr length)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   const GLintptr end = offset + length;

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glInvalidateBufferSubData(name = 0x%x) invalid object",
                  buffer);
      return;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "An INVALID_VALUE error is generated if <offset> or <length> is
    *     negative, or if <offset> + <length> is greater than the value of
    *     BUFFER_SIZE."
    */
   if (end < 0 || end > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glInvalidateBufferSubData(invalid offset or length)");
      return;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "An INVALID_OPERATION error is generated if the buffer is currently
    *     mapped by MapBuffer, or if the invalidate range intersects the range
    *     currently mapped by MapBufferRange."
    */
   if (bufferobj_range_mapped(bufObj, offset, length)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glInvalidateBufferSubData(intersection with mapped "
                  "range)");
      return;
   }

   /* We don't actually do anything for this yet.  Just return after
    * validating the parameters and generating the required errors.
    */
   return;
}

void GLAPIENTRY
_mesa_InvalidateBufferData(GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glInvalidateBufferData(name = 0x%x) invalid object",
                  buffer);
      return;
   }

   /* The GL_ARB_invalidate_subdata spec says:
    *
    *     "An INVALID_OPERATION error is generated if the buffer is currently
    *     mapped by MapBuffer, or if the invalidate range intersects the range
    *     currently mapped by MapBufferRange."
    */
   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glInvalidateBufferData(intersection with mapped "
                  "range)");
      return;
   }

   /* We don't actually do anything for this yet.  Just return after
    * validating the parameters and generating the required errors.
    */
   return;
}
