/*
 * Mesa 3-D graphics library
 * Version:  7.6
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file bufferobj.c
 * \brief Functions for the GL_ARB_vertex/pixel_buffer_object extensions.
 * \author Brian Paul, Ian Romanick
 */


#include "glheader.h"
#include "hash.h"
#include "imports.h"
#include "image.h"
#include "context.h"
#include "bufferobj.h"
#include "fbobject.h"
#include "texobj.h"


/* Debug flags */
/*#define VBO_DEBUG*/
/*#define BOUNDS_CHECK*/


#ifdef FEATURE_OES_mapbuffer
#define DEFAULT_ACCESS GL_MAP_WRITE_BIT
#else
#define DEFAULT_ACCESS (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)
#endif


/**
 * Return pointer to address of a buffer object target.
 * \param ctx  the GL context
 * \param target  the buffer object target to be retrieved.
 * \return   pointer to pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static INLINE struct gl_buffer_object **
get_buffer_target(GLcontext *ctx, GLenum target)
{
   switch (target) {
   case GL_ARRAY_BUFFER_ARB:
      return &ctx->Array.ArrayBufferObj;
   case GL_ELEMENT_ARRAY_BUFFER_ARB:
      return &ctx->Array.ElementArrayBufferObj;
   case GL_PIXEL_PACK_BUFFER_EXT:
      return &ctx->Pack.BufferObj;
   case GL_PIXEL_UNPACK_BUFFER_EXT:
      return &ctx->Unpack.BufferObj;
   case GL_COPY_READ_BUFFER:
      if (ctx->Extensions.ARB_copy_buffer) {
         return &ctx->CopyReadBuffer;
      }
      break;
   case GL_COPY_WRITE_BUFFER:
      if (ctx->Extensions.ARB_copy_buffer) {
         return &ctx->CopyWriteBuffer;
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
 * \return   pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if \c target is invalid.
 */
static INLINE struct gl_buffer_object *
get_buffer(GLcontext *ctx, GLenum target)
{
   struct gl_buffer_object **bufObj = get_buffer_target(ctx, target);
   if (bufObj)
      return *bufObj;
   return NULL;
}


/**
 * Convert a GLbitfield describing the mapped buffer access flags
 * into one of GL_READ_WRITE, GL_READ_ONLY, or GL_WRITE_ONLY.
 */
static GLenum
simplified_access_mode(GLbitfield access)
{
   const GLbitfield rwFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
   if ((access & rwFlags) == rwFlags)
      return GL_READ_WRITE;
   if ((access & GL_MAP_READ_BIT) == GL_MAP_READ_BIT)
      return GL_READ_ONLY;
   if ((access & GL_MAP_WRITE_BIT) == GL_MAP_WRITE_BIT)
      return GL_WRITE_ONLY;
   return GL_READ_WRITE; /* this should never happen, but no big deal */
}


/**
 * Tests the subdata range parameters and sets the GL error code for
 * \c glBufferSubDataARB and \c glGetBufferSubDataARB.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte of the subdata range.
 * \param size    Size, in bytes, of the subdata range.
 * \param caller  Name of calling function for recording errors.
 * \return   A pointer to the buffer object bound to \c target in the
 *           specified context or \c NULL if any of the parameter or state
 *           conditions for \c glBufferSubDataARB or \c glGetBufferSubDataARB
 *           are invalid.
 *
 * \sa glBufferSubDataARB, glGetBufferSubDataARB
 */
static struct gl_buffer_object *
buffer_object_subdata_range_good( GLcontext * ctx, GLenum target, 
                                  GLintptrARB offset, GLsizeiptrARB size,
                                  const char *caller )
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", caller);
      return NULL;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      return NULL;
   }
   if (offset + size > bufObj->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "%s(size + offset > buffer size)", caller);
      return NULL;
   }
   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Buffer is currently mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      return NULL;
   }

   return bufObj;
}


/**
 * Allocate and initialize a new buffer object.
 * 
 * Default callback for the \c dd_function_table::NewBufferObject() hook.
 */
static struct gl_buffer_object *
_mesa_new_buffer_object( GLcontext *ctx, GLuint name, GLenum target )
{
   struct gl_buffer_object *obj;

   (void) ctx;

   obj = MALLOC_STRUCT(gl_buffer_object);
   _mesa_initialize_buffer_object(obj, name, target);
   return obj;
}


/**
 * Delete a buffer object.
 * 
 * Default callback for the \c dd_function_table::DeleteBuffer() hook.
 */
static void
_mesa_delete_buffer_object( GLcontext *ctx, struct gl_buffer_object *bufObj )
{
   (void) ctx;

   if (bufObj->Data)
      free(bufObj->Data);

   /* assign strange values here to help w/ debugging */
   bufObj->RefCount = -1000;
   bufObj->Name = ~0;

   _glthread_DESTROY_MUTEX(bufObj->Mutex);
   free(bufObj);
}



/**
 * Set ptr to bufObj w/ reference counting.
 */
void
_mesa_reference_buffer_object(GLcontext *ctx,
                              struct gl_buffer_object **ptr,
                              struct gl_buffer_object *bufObj)
{
   if (*ptr == bufObj)
      return;

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
	 ASSERT(ctx->Array.ElementArrayBufferObj != bufObj);
	 ASSERT(ctx->Array.ArrayObj->Vertex.BufferObj != bufObj);
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
_mesa_initialize_buffer_object( struct gl_buffer_object *obj,
				GLuint name, GLenum target )
{
   (void) target;

   memset(obj, 0, sizeof(struct gl_buffer_object));
   _glthread_INIT_MUTEX(obj->Mutex);
   obj->RefCount = 1;
   obj->Name = name;
   obj->Usage = GL_STATIC_DRAW_ARB;
   obj->AccessFlags = DEFAULT_ACCESS;
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
_mesa_buffer_data( GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		   const GLvoid * data, GLenum usage,
		   struct gl_buffer_object * bufObj )
{
   void * new_data;

   (void) ctx; (void) target;

   new_data = _mesa_realloc( bufObj->Data, bufObj->Size, size );
   if (new_data) {
      bufObj->Data = (GLubyte *) new_data;
      bufObj->Size = size;
      bufObj->Usage = usage;

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
 * \param target  Buffer object target on which to operate.
 * \param offset  Offset of the first byte to be modified.
 * \param size    Size, in bytes, of the data range.
 * \param data    Pointer to the data to store in the buffer object.
 * \param bufObj  Object to be used.
 *
 * \sa glBufferSubDataARB, dd_function_table::BufferSubData.
 */
static void
_mesa_buffer_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
		      GLsizeiptrARB size, const GLvoid * data,
		      struct gl_buffer_object * bufObj )
{
   (void) ctx; (void) target;

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
_mesa_buffer_get_subdata( GLcontext *ctx, GLenum target, GLintptrARB offset,
			  GLsizeiptrARB size, GLvoid * data,
			  struct gl_buffer_object * bufObj )
{
   (void) ctx; (void) target;

   if (bufObj->Data && ((GLsizeiptrARB) (size + offset) <= bufObj->Size)) {
      memcpy( data, (GLubyte *) bufObj->Data + offset, size );
   }
}


/**
 * Default callback for \c dd_function_tabel::MapBuffer().
 *
 * The function parameters will have been already tested for errors.
 *
 * \param ctx     GL context.
 * \param target  Buffer object target on which to operate.
 * \param access  Information about how the buffer will be accessed.
 * \param bufObj  Object to be mapped.
 * \return  A pointer to the object's internal data store that can be accessed
 *          by the processor
 *
 * \sa glMapBufferARB, dd_function_table::MapBuffer
 */
static void *
_mesa_buffer_map( GLcontext *ctx, GLenum target, GLenum access,
		  struct gl_buffer_object *bufObj )
{
   (void) ctx;
   (void) target;
   (void) access;
   /* Just return a direct pointer to the data */
   if (_mesa_bufferobj_mapped(bufObj)) {
      /* already mapped! */
      return NULL;
   }
   bufObj->Pointer = bufObj->Data;
   bufObj->Length = bufObj->Size;
   bufObj->Offset = 0;
   return bufObj->Pointer;
}


/**
 * Default fallback for \c dd_function_table::MapBufferRange().
 * Called via glMapBufferRange().
 */
static void *
_mesa_buffer_map_range( GLcontext *ctx, GLenum target, GLintptr offset,
                        GLsizeiptr length, GLbitfield access,
                        struct gl_buffer_object *bufObj )
{
   (void) ctx;
   (void) target;
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
_mesa_buffer_flush_mapped_range( GLcontext *ctx, GLenum target, 
                                 GLintptr offset, GLsizeiptr length,
                                 struct gl_buffer_object *obj )
{
   (void) ctx;
   (void) target;
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
_mesa_buffer_unmap( GLcontext *ctx, GLenum target,
                    struct gl_buffer_object *bufObj )
{
   (void) ctx;
   (void) target;
   /* XXX we might assert here that bufObj->Pointer is non-null */
   bufObj->Pointer = NULL;
   bufObj->Length = 0;
   bufObj->Offset = 0;
   bufObj->AccessFlags = 0x0;
   return GL_TRUE;
}


/**
 * Default fallback for \c dd_function_table::CopyBufferSubData().
 * Called via glCopyBuffserSubData().
 */
static void
_mesa_copy_buffer_subdata(GLcontext *ctx,
                          struct gl_buffer_object *src,
                          struct gl_buffer_object *dst,
                          GLintptr readOffset, GLintptr writeOffset,
                          GLsizeiptr size)
{
   GLubyte *srcPtr, *dstPtr;

   /* buffer should not already be mapped */
   assert(!_mesa_bufferobj_mapped(src));
   assert(!_mesa_bufferobj_mapped(dst));

   srcPtr = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_COPY_READ_BUFFER,
                                              GL_READ_ONLY, src);
   dstPtr = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_COPY_WRITE_BUFFER,
                                              GL_WRITE_ONLY, dst);

   if (srcPtr && dstPtr)
      memcpy(dstPtr + writeOffset, srcPtr + readOffset, size);

   ctx->Driver.UnmapBuffer(ctx, GL_COPY_READ_BUFFER, src);
   ctx->Driver.UnmapBuffer(ctx, GL_COPY_WRITE_BUFFER, dst);
}



/**
 * Initialize the state associated with buffer objects
 */
void
_mesa_init_buffer_objects( GLcontext *ctx )
{
   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj,
                                 ctx->Shared->NullBufferObj);
   _mesa_reference_buffer_object(ctx, &ctx->Array.ElementArrayBufferObj,
                                 ctx->Shared->NullBufferObj);

   _mesa_reference_buffer_object(ctx, &ctx->CopyReadBuffer,
                                 ctx->Shared->NullBufferObj);
   _mesa_reference_buffer_object(ctx, &ctx->CopyWriteBuffer,
                                 ctx->Shared->NullBufferObj);
}


void
_mesa_free_buffer_objects( GLcontext *ctx )
{
   _mesa_reference_buffer_object(ctx, &ctx->Array.ArrayBufferObj, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->Array.ElementArrayBufferObj, NULL);

   _mesa_reference_buffer_object(ctx, &ctx->CopyReadBuffer, NULL);
   _mesa_reference_buffer_object(ctx, &ctx->CopyWriteBuffer, NULL);
}


/**
 * Bind the specified target to buffer for the specified context.
 * Called by glBindBuffer() and other functions.
 */
static void
bind_buffer_object(GLcontext *ctx, GLenum target, GLuint buffer)
{
   struct gl_buffer_object *oldBufObj;
   struct gl_buffer_object *newBufObj = NULL;
   struct gl_buffer_object **bindTarget = NULL;

   bindTarget = get_buffer_target(ctx, target);
   if (!bindTarget) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferARB(target 0x%x)");
      return;
   }

   /* Get pointer to old buffer object (to be unbound) */
   oldBufObj = *bindTarget;
   if (oldBufObj && oldBufObj->Name == buffer)
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
      if (!newBufObj) {
         /* if this is a new buffer object id, allocate a buffer object now */
         ASSERT(ctx->Driver.NewBufferObject);
         newBufObj = ctx->Driver.NewBufferObject(ctx, buffer, target);
         if (!newBufObj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindBufferARB");
            return;
         }
         _mesa_HashInsert(ctx->Shared->BufferObjects, buffer, newBufObj);
      }
   }
   
   /* bind new buffer */
   _mesa_reference_buffer_object(ctx, bindTarget, newBufObj);

   /* Pass BindBuffer call to device driver */
   if (ctx->Driver.BindBuffer)
      ctx->Driver.BindBuffer( ctx, target, newBufObj );
}


/**
 * Update the default buffer objects in the given context to reference those
 * specified in the shared state and release those referencing the old 
 * shared state.
 */
void
_mesa_update_default_objects_buffer_objects(GLcontext *ctx)
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
 * When we're about to read pixel data out of a PBO (via glDrawPixels,
 * glTexImage, etc) or write data into a PBO (via glReadPixels,
 * glGetTexImage, etc) we call this function to check that we're not
 * going to read out of bounds.
 *
 * XXX This would also be a convenient time to check that the PBO isn't
 * currently mapped.  Whoever calls this function should check for that.
 * Remember, we can't use a PBO when it's mapped!
 *
 * If we're not using a PBO, this is a no-op.
 *
 * \param width  width of image to read/write
 * \param height  height of image to read/write
 * \param depth  depth of image to read/write
 * \param format  format of image to read/write
 * \param type  datatype of image to read/write
 * \param ptr  the user-provided pointer/offset
 * \return GL_TRUE if the PBO access is OK, GL_FALSE if the access would
 *         go out of bounds.
 */
GLboolean
_mesa_validate_pbo_access(GLuint dimensions,
                          const struct gl_pixelstore_attrib *pack,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type, const GLvoid *ptr)
{
   GLvoid *start, *end;
   const GLubyte *sizeAddr; /* buffer size, cast to a pointer */

   if (!_mesa_is_bufferobj(pack->BufferObj))
      return GL_TRUE;  /* no PBO, OK */

   if (pack->BufferObj->Size == 0)
      /* no buffer! */
      return GL_FALSE;

   /* get address of first pixel we'll read */
   start = _mesa_image_address(dimensions, pack, ptr, width, height,
                               format, type, 0, 0, 0);

   /* get address just past the last pixel we'll read */
   end =  _mesa_image_address(dimensions, pack, ptr, width, height,
                              format, type, depth-1, height-1, width);


   sizeAddr = ((const GLubyte *) 0) + pack->BufferObj->Size;

   if ((const GLubyte *) start > sizeAddr) {
      /* This will catch negative values / wrap-around */
      return GL_FALSE;
   }
   if ((const GLubyte *) end > sizeAddr) {
      /* Image read goes beyond end of buffer */
      return GL_FALSE;
   }

   /* OK! */
   return GL_TRUE;
}


/**
 * For commands that read from a PBO (glDrawPixels, glTexImage,
 * glPolygonStipple, etc), if we're reading from a PBO, map it read-only
 * and return the pointer into the PBO.  If we're not reading from a
 * PBO, return \p src as-is.
 * If non-null return, must call _mesa_unmap_pbo_source() when done.
 *
 * \return NULL if error, else pointer to start of data
 */
const GLvoid *
_mesa_map_pbo_source(GLcontext *ctx,
                     const struct gl_pixelstore_attrib *unpack,
                     const GLvoid *src)
{
   const GLubyte *buf;

   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      /* unpack from PBO */
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                                              GL_READ_ONLY_ARB,
                                              unpack->BufferObj);
      if (!buf)
         return NULL;

      buf = ADD_POINTERS(buf, src);
   }
   else {
      /* unpack from normal memory */
      buf = src;
   }

   return buf;
}


/**
 * Combine PBO-read validation and mapping.
 * If any GL errors are detected, they'll be recorded and NULL returned.
 * \sa _mesa_validate_pbo_access
 * \sa _mesa_map_pbo_source
 * A call to this function should have a matching call to
 * _mesa_unmap_pbo_source().
 */
const GLvoid *
_mesa_map_validate_pbo_source(GLcontext *ctx,
                              GLuint dimensions,
                              const struct gl_pixelstore_attrib *unpack,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLenum format, GLenum type, const GLvoid *ptr,
                              const char *where)
{
   ASSERT(dimensions == 1 || dimensions == 2 || dimensions == 3);

   if (!_mesa_is_bufferobj(unpack->BufferObj)) {
      /* non-PBO access: no validation to be done */
      return ptr;
   }

   if (!_mesa_validate_pbo_access(dimensions, unpack,
                                  width, height, depth, format, type, ptr)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(out of bounds PBO access)", where);
      return NULL;
   }

   if (_mesa_bufferobj_mapped(unpack->BufferObj)) {
      /* buffer is already mapped - that's an error */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)", where);
      return NULL;
   }

   ptr = _mesa_map_pbo_source(ctx, unpack, ptr);
   return ptr;
}


/**
 * Counterpart to _mesa_map_pbo_source()
 */
void
_mesa_unmap_pbo_source(GLcontext *ctx,
                       const struct gl_pixelstore_attrib *unpack)
{
   ASSERT(unpack != &ctx->Pack); /* catch pack/unpack mismatch */
   if (_mesa_is_bufferobj(unpack->BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }
}


/**
 * For commands that write to a PBO (glReadPixels, glGetColorTable, etc),
 * if we're writing to a PBO, map it write-only and return the pointer
 * into the PBO.  If we're not writing to a PBO, return \p dst as-is.
 * If non-null return, must call _mesa_unmap_pbo_dest() when done.
 *
 * \return NULL if error, else pointer to start of data
 */
void *
_mesa_map_pbo_dest(GLcontext *ctx,
                   const struct gl_pixelstore_attrib *pack,
                   GLvoid *dest)
{
   void *buf;

   if (_mesa_is_bufferobj(pack->BufferObj)) {
      /* pack into PBO */
      buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT,
                                              GL_WRITE_ONLY_ARB,
                                              pack->BufferObj);
      if (!buf)
         return NULL;

      buf = ADD_POINTERS(buf, dest);
   }
   else {
      /* pack to normal memory */
      buf = dest;
   }

   return buf;
}


/**
 * Combine PBO-write validation and mapping.
 * If any GL errors are detected, they'll be recorded and NULL returned.
 * \sa _mesa_validate_pbo_access
 * \sa _mesa_map_pbo_dest
 * A call to this function should have a matching call to
 * _mesa_unmap_pbo_dest().
 */
GLvoid *
_mesa_map_validate_pbo_dest(GLcontext *ctx,
                            GLuint dimensions,
                            const struct gl_pixelstore_attrib *unpack,
                            GLsizei width, GLsizei height, GLsizei depth,
                            GLenum format, GLenum type, GLvoid *ptr,
                            const char *where)
{
   ASSERT(dimensions == 1 || dimensions == 2 || dimensions == 3);

   if (!_mesa_is_bufferobj(unpack->BufferObj)) {
      /* non-PBO access: no validation to be done */
      return ptr;
   }

   if (!_mesa_validate_pbo_access(dimensions, unpack,
                                  width, height, depth, format, type, ptr)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(out of bounds PBO access)", where);
      return NULL;
   }

   if (_mesa_bufferobj_mapped(unpack->BufferObj)) {
      /* buffer is already mapped - that's an error */
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)", where);
      return NULL;
   }

   ptr = _mesa_map_pbo_dest(ctx, unpack, ptr);
   return ptr;
}


/**
 * Counterpart to _mesa_map_pbo_dest()
 */
void
_mesa_unmap_pbo_dest(GLcontext *ctx,
                     const struct gl_pixelstore_attrib *pack)
{
   ASSERT(pack != &ctx->Unpack); /* catch pack/unpack mismatch */
   if (_mesa_is_bufferobj(pack->BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_PACK_BUFFER_EXT, pack->BufferObj);
   }
}



/**
 * Return the gl_buffer_object for the given ID.
 * Always return NULL for ID 0.
 */
struct gl_buffer_object *
_mesa_lookup_bufferobj(GLcontext *ctx, GLuint buffer)
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
unbind(GLcontext *ctx,
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
   driver->BindBuffer = NULL;
   driver->BufferData = _mesa_buffer_data;
   driver->BufferSubData = _mesa_buffer_subdata;
   driver->GetBufferSubData = _mesa_buffer_get_subdata;
   driver->MapBuffer = _mesa_buffer_map;
   driver->UnmapBuffer = _mesa_buffer_unmap;

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
_mesa_BindBufferARB(GLenum target, GLuint buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bind_buffer_object(ctx, target, buffer);
}


/**
 * Delete a set of buffer objects.
 * 
 * \param n      Number of buffer objects to delete.
 * \param ids    Array of \c n buffer object IDs.
 */
void GLAPIENTRY
_mesa_DeleteBuffersARB(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteBuffersARB(n)");
      return;
   }

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, ids[i]);
      if (bufObj) {
         struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
         GLuint j;

         ASSERT(bufObj->Name == ids[i]);

         if (_mesa_bufferobj_mapped(bufObj)) {
            /* if mapped, unmap it now */
            ctx->Driver.UnmapBuffer(ctx, 0, bufObj);
            bufObj->AccessFlags = DEFAULT_ACCESS;
            bufObj->Pointer = NULL;
         }

         /* unbind any vertex pointers bound to this buffer */
         unbind(ctx, &arrayObj->Vertex.BufferObj, bufObj);
         unbind(ctx, &arrayObj->Weight.BufferObj, bufObj);
         unbind(ctx, &arrayObj->Normal.BufferObj, bufObj);
         unbind(ctx, &arrayObj->Color.BufferObj, bufObj);
         unbind(ctx, &arrayObj->SecondaryColor.BufferObj, bufObj);
         unbind(ctx, &arrayObj->FogCoord.BufferObj, bufObj);
         unbind(ctx, &arrayObj->Index.BufferObj, bufObj);
         unbind(ctx, &arrayObj->EdgeFlag.BufferObj, bufObj);
         for (j = 0; j < Elements(arrayObj->TexCoord); j++) {
            unbind(ctx, &arrayObj->TexCoord[j].BufferObj, bufObj);
         }
         for (j = 0; j < Elements(arrayObj->VertexAttrib); j++) {
            unbind(ctx, &arrayObj->VertexAttrib[j].BufferObj, bufObj);
         }

         if (ctx->Array.ArrayBufferObj == bufObj) {
            _mesa_BindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
         }
         if (ctx->Array.ElementArrayBufferObj == bufObj) {
            _mesa_BindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
         }

         /* unbind any pixel pack/unpack pointers bound to this buffer */
         if (ctx->Pack.BufferObj == bufObj) {
            _mesa_BindBufferARB( GL_PIXEL_PACK_BUFFER_EXT, 0 );
         }
         if (ctx->Unpack.BufferObj == bufObj) {
            _mesa_BindBufferARB( GL_PIXEL_UNPACK_BUFFER_EXT, 0 );
         }

         /* The ID is immediately freed for re-use */
         _mesa_HashRemove(ctx->Shared->BufferObjects, bufObj->Name);
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
_mesa_GenBuffersARB(GLsizei n, GLuint *buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

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

   /* Allocate new, empty buffer objects and return identifiers */
   for (i = 0; i < n; i++) {
      struct gl_buffer_object *bufObj;
      GLuint name = first + i;
      GLenum target = 0;
      bufObj = ctx->Driver.NewBufferObject( ctx, name, target );
      if (!bufObj) {
         _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenBuffersARB");
         return;
      }
      _mesa_HashInsert(ctx->Shared->BufferObjects, first + i, bufObj);
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
_mesa_IsBufferARB(GLuint id)
{
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   bufObj = _mesa_lookup_bufferobj(ctx, id);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return bufObj ? GL_TRUE : GL_FALSE;
}


void GLAPIENTRY
_mesa_BufferDataARB(GLenum target, GLsizeiptrARB size,
                    const GLvoid * data, GLenum usage)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
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

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBufferDataARB(target)" );
      return;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBufferDataARB(buffer 0)" );
      return;
   }
   
   if (_mesa_bufferobj_mapped(bufObj)) {
      /* Unmap the existing buffer.  We'll replace it now.  Not an error. */
      ctx->Driver.UnmapBuffer(ctx, target, bufObj);
      bufObj->AccessFlags = DEFAULT_ACCESS;
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
   if (!ctx->Driver.BufferData( ctx, target, size, data, usage, bufObj )) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBufferDataARB()");
   }
}


void GLAPIENTRY
_mesa_BufferSubDataARB(GLenum target, GLintptrARB offset,
                       GLsizeiptrARB size, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "glBufferSubDataARB" );
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   bufObj->Written = GL_TRUE;

   ASSERT(ctx->Driver.BufferSubData);
   ctx->Driver.BufferSubData( ctx, target, offset, size, data, bufObj );
}


void GLAPIENTRY
_mesa_GetBufferSubDataARB(GLenum target, GLintptrARB offset,
                          GLsizeiptrARB size, void * data)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = buffer_object_subdata_range_good( ctx, target, offset, size,
                                              "glGetBufferSubDataARB" );
   if (!bufObj) {
      /* error already recorded */
      return;
   }

   ASSERT(ctx->Driver.GetBufferSubData);
   ctx->Driver.GetBufferSubData( ctx, target, offset, size, data, bufObj );
}


void * GLAPIENTRY
_mesa_MapBufferARB(GLenum target, GLenum access)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   GLbitfield accessFlags;
   void *map;

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, NULL);

   switch (access) {
   case GL_READ_ONLY_ARB:
      accessFlags = GL_MAP_READ_BIT;
      break;
   case GL_WRITE_ONLY_ARB:
      accessFlags = GL_MAP_WRITE_BIT;
      break;
   case GL_READ_WRITE_ARB:
      accessFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(access)");
      return NULL;
   }

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMapBufferARB(target)" );
      return NULL;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(buffer 0)" );
      return NULL;
   }
   if (_mesa_bufferobj_mapped(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMapBufferARB(already mapped)");
      return NULL;
   }

   ASSERT(ctx->Driver.MapBuffer);
   map = ctx->Driver.MapBuffer( ctx, target, access, bufObj );
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
_mesa_UnmapBufferARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   GLboolean status = GL_TRUE;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glUnmapBufferARB(target)" );
      return GL_FALSE;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUnmapBufferARB" );
      return GL_FALSE;
   }
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

   status = ctx->Driver.UnmapBuffer( ctx, target, bufObj );
   bufObj->AccessFlags = DEFAULT_ACCESS;
   ASSERT(bufObj->Pointer == NULL);
   ASSERT(bufObj->Offset == 0);
   ASSERT(bufObj->Length == 0);

   return status;
}


void GLAPIENTRY
_mesa_GetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "GetBufferParameterivARB(target)" );
      return;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "GetBufferParameterivARB" );
      return;
   }

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = (GLint) bufObj->Size;
      break;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      break;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(bufObj->AccessFlags);
      break;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameterivARB(pname)");
      return;
   }
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
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "GetBufferParameteri64v(target)" );
      return;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "GetBufferParameteri64v" );
      return;
   }

   switch (pname) {
   case GL_BUFFER_SIZE_ARB:
      *params = bufObj->Size;
      break;
   case GL_BUFFER_USAGE_ARB:
      *params = bufObj->Usage;
      break;
   case GL_BUFFER_ACCESS_ARB:
      *params = simplified_access_mode(bufObj->AccessFlags);
      break;
   case GL_BUFFER_MAPPED_ARB:
      *params = _mesa_bufferobj_mapped(bufObj);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferParameteri64v(pname)");
      return;
   }
}


void GLAPIENTRY
_mesa_GetBufferPointervARB(GLenum target, GLenum pname, GLvoid **params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object * bufObj;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname != GL_BUFFER_MAP_POINTER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(pname)");
      return;
   }

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetBufferPointervARB(target)" );
      return;
   }
   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetBufferPointervARB" );
      return;
   }

   *params = bufObj->Pointer;
}


void GLAPIENTRY
_mesa_CopyBufferSubData(GLenum readTarget, GLenum writeTarget,
                        GLintptr readOffset, GLintptr writeOffset,
                        GLsizeiptr size)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_buffer_object *src, *dst;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   src = get_buffer(ctx, readTarget);
   if (!src || !_mesa_is_bufferobj(src)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glCopyBuffserSubData(readTarget = 0x%x)", readTarget);
      return;
   }

   dst = get_buffer(ctx, writeTarget);
   if (!dst || !_mesa_is_bufferobj(dst)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glCopyBuffserSubData(writeTarget = 0x%x)", writeTarget);
      return;
   }

   if (_mesa_bufferobj_mapped(src)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyBuffserSubData(readBuffer is mapped)");
      return;
   }

   if (_mesa_bufferobj_mapped(dst)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyBuffserSubData(writeBuffer is mapped)");
      return;
   }

   if (readOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBuffserSubData(readOffset = %d)", readOffset);
      return;
   }

   if (writeOffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBuffserSubData(writeOffset = %d)", writeOffset);
      return;
   }

   if (readOffset + size > src->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBuffserSubData(readOffset + size = %d)",
                  readOffset, size);
      return;
   }

   if (writeOffset + size > dst->Size) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyBuffserSubData(writeOffset + size = %d)",
                  writeOffset, size);
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
                     "glCopyBuffserSubData(overlapping src/dst)");
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
                  "glMapBufferRange(offset = %ld)", offset);
      return NULL;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(length = %ld)", length);
      return NULL;
   }

   if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(access indicates neither read or write)");
      return NULL;
   }

   if (access & GL_MAP_READ_BIT) {
      if ((access & GL_MAP_INVALIDATE_RANGE_BIT) ||
          (access & GL_MAP_INVALIDATE_BUFFER_BIT) ||
          (access & GL_MAP_UNSYNCHRONIZED_BIT)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glMapBufferRange(invalid access flags)");
         return NULL;
      }
   }

   if ((access & GL_MAP_FLUSH_EXPLICIT_BIT) &&
       ((access & GL_MAP_WRITE_BIT) == 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(invalid access flags)");
      return NULL;
   }

   bufObj = get_buffer(ctx, target);
   if (!bufObj || !_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glMapBufferRange(target = 0x%x)", target);
      return NULL;
   }

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
      
   ASSERT(ctx->Driver.MapBufferRange);
   map = ctx->Driver.MapBufferRange(ctx, target, offset, length,
                                    access, bufObj);
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
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ARB_map_buffer_range) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(extension not supported)");
      return;
   }

   if (offset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(offset = %ld)", offset);
      return;
   }

   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glMapBufferRange(length = %ld)", length);
      return;
   }

   bufObj = get_buffer(ctx, target);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glMapBufferRange(target = 0x%x)", target);
      return;
   }

   if (!_mesa_is_bufferobj(bufObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(current buffer is 0)");
      return;
   }

   if (!_mesa_bufferobj_mapped(bufObj)) {
      /* buffer is not mapped */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(buffer is not mapped)");
      return;
   }

   if ((bufObj->AccessFlags & GL_MAP_FLUSH_EXPLICIT_BIT) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glMapBufferRange(GL_MAP_FLUSH_EXPLICIT_BIT not set)");
      return;
   }

   if (offset + length > bufObj->Length) {
      _mesa_error(ctx, GL_INVALID_VALUE,
             "glMapBufferRange(offset %ld + length %ld > mapped length %ld)",
             offset, length, bufObj->Length);
      return;
   }

   ASSERT(bufObj->AccessFlags & GL_MAP_WRITE_BIT);

   if (ctx->Driver.FlushMappedBufferRange)
      ctx->Driver.FlushMappedBufferRange(ctx, target, offset, length, bufObj);
}


#if FEATURE_APPLE_object_purgeable
static GLenum
_mesa_BufferObjectPurgeable(GLcontext *ctx, GLuint name, GLenum option)
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
_mesa_RenderObjectPurgeable(GLcontext *ctx, GLuint name, GLenum option)
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
_mesa_TextureObjectPurgeable(GLcontext *ctx, GLuint name, GLenum option)
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
      retval = _mesa_TextureObjectPurgeable (ctx, name, option);
      break;
   case GL_RENDERBUFFER_EXT:
      retval = _mesa_RenderObjectPurgeable (ctx, name, option);
      break;
   case GL_BUFFER_OBJECT_APPLE:
      retval = _mesa_BufferObjectPurgeable (ctx, name, option);
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
_mesa_BufferObjectUnpurgeable(GLcontext *ctx, GLuint name, GLenum option)
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

   retval = GL_RETAINED_APPLE;
   if (ctx->Driver.BufferObjectUnpurgeable)
      retval = ctx->Driver.BufferObjectUnpurgeable(ctx, bufObj, option);

   return retval;
}


static GLenum
_mesa_RenderObjectUnpurgeable(GLcontext *ctx, GLuint name, GLenum option)
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

   retval = GL_RETAINED_APPLE;
   if (ctx->Driver.RenderObjectUnpurgeable)
      retval = ctx->Driver.RenderObjectUnpurgeable(ctx, bufObj, option);

   return option;
}


static GLenum
_mesa_TextureObjectUnpurgeable(GLcontext *ctx, GLuint name, GLenum option)
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

   retval = GL_RETAINED_APPLE;
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
      return _mesa_BufferObjectUnpurgeable(ctx, name, option);
   case GL_TEXTURE:
      return _mesa_TextureObjectUnpurgeable(ctx, name, option);
   case GL_RENDERBUFFER_EXT:
      return _mesa_RenderObjectUnpurgeable(ctx, name, option);
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glObjectUnpurgeable(name = 0x%x) invalid type: %d",
                  name, objectType);
      return 0;
   }
}


static void
_mesa_GetBufferObjectParameterivAPPLE(GLcontext *ctx, GLuint name,
                                      GLenum pname, GLint* params)
{
   struct gl_buffer_object *bufObj;

   bufObj = _mesa_lookup_bufferobj(ctx, name);
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
_mesa_GetRenderObjectParameterivAPPLE(GLcontext *ctx, GLuint name,
                                      GLenum pname, GLint* params)
{
   struct gl_renderbuffer *bufObj;

   bufObj = _mesa_lookup_renderbuffer(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
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
_mesa_GetTextureObjectParameterivAPPLE(GLcontext *ctx, GLuint name,
                                       GLenum pname, GLint* params)
{
   struct gl_texture_object *bufObj;

   bufObj = _mesa_lookup_texture(ctx, name);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glObjectUnpurgeable(name = 0x%x)", name);
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


void GLAPIENTRY
_mesa_GetObjectParameterivAPPLE(GLenum objectType, GLuint name, GLenum pname,
                                GLint* params)
{
   GET_CURRENT_CONTEXT(ctx);

   if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetObjectParameteriv(name = 0x%x)", name);
      return;
   }

   switch (objectType) {
   case GL_TEXTURE:
      _mesa_GetTextureObjectParameterivAPPLE (ctx, name, pname, params);
      break;
   case GL_BUFFER_OBJECT_APPLE:
      _mesa_GetBufferObjectParameterivAPPLE (ctx, name, pname, params);
      break;
   case GL_RENDERBUFFER_EXT:
      _mesa_GetRenderObjectParameterivAPPLE (ctx, name, pname, params);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetObjectParameteriv(name = 0x%x) invalid type: %d",
                  name, objectType);
   }
}

#endif /* FEATURE_APPLE_object_purgeable */
