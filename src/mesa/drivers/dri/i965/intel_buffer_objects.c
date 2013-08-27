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

/**
 * @file intel_buffer_objects.c
 *
 * This provides core GL buffer object functionality.
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"

#include "brw_context.h"
#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"

static GLboolean
intel_bufferobj_unmap(struct gl_context * ctx, struct gl_buffer_object *obj);

/** Allocates a new drm_intel_bo to store the data for the buffer object. */
static void
intel_bufferobj_alloc_buffer(struct brw_context *brw,
			     struct intel_buffer_object *intel_obj)
{
   intel_obj->buffer = drm_intel_bo_alloc(brw->bufmgr, "bufferobj",
					  intel_obj->Base.Size, 64);

   /* the buffer might be bound as a uniform buffer, need to update it
    */
   brw->state.dirty.brw |= BRW_NEW_UNIFORM_BUFFER;
}

static void
release_buffer(struct intel_buffer_object *intel_obj)
{
   drm_intel_bo_unreference(intel_obj->buffer);
   intel_obj->buffer = NULL;
   intel_obj->offset = 0;
}

/**
 * The NewBufferObject() driver hook.
 *
 * Allocates a new intel_buffer_object structure and initializes it.
 *
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
intel_bufferobj_alloc(struct gl_context * ctx, GLuint name, GLenum target)
{
   struct intel_buffer_object *obj = CALLOC_STRUCT(intel_buffer_object);

   _mesa_initialize_buffer_object(ctx, &obj->Base, name, target);

   obj->buffer = NULL;

   return &obj->Base;
}

/**
 * The DeleteBuffer() driver hook.
 *
 * Deletes a single OpenGL buffer object.  Used by glDeleteBuffers().
 */
static void
intel_bufferobj_free(struct gl_context * ctx, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   /* Buffer objects are automatically unmapped when deleting according
    * to the spec, but Mesa doesn't do UnmapBuffer for us at context destroy
    * (though it does if you call glDeleteBuffers)
    */
   if (obj->Pointer)
      intel_bufferobj_unmap(ctx, obj);

   drm_intel_bo_unreference(intel_obj->buffer);
   free(intel_obj);
}


/**
 * The BufferData() driver hook.
 *
 * Implements glBufferData(), which recreates a buffer object's data store
 * and populates it with the given data, if present.
 *
 * Any data that was previously stored in the buffer object is lost.
 *
 * \return true for success, false if out of memory
 */
static GLboolean
intel_bufferobj_data(struct gl_context * ctx,
                     GLenum target,
                     GLsizeiptrARB size,
                     const GLvoid * data,
                     GLenum usage, struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   /* Part of the ABI, but this function doesn't use it.
    */
   (void) target;

   intel_obj->Base.Size = size;
   intel_obj->Base.Usage = usage;

   assert(!obj->Pointer); /* Mesa should have unmapped it */

   if (intel_obj->buffer != NULL)
      release_buffer(intel_obj);

   if (size != 0) {
      intel_bufferobj_alloc_buffer(brw, intel_obj);
      if (!intel_obj->buffer)
         return false;

      if (data != NULL)
	 drm_intel_bo_subdata(intel_obj->buffer, 0, size, data);
   }

   return true;
}


/**
 * The BufferSubData() driver hook.
 *
 * Implements glBufferSubData(), which replaces a portion of the data in a
 * buffer object.
 *
 * If the data range specified by (size + offset) extends beyond the end of
 * the buffer or if data is NULL, no copy is performed.
 */
static void
intel_bufferobj_subdata(struct gl_context * ctx,
                        GLintptrARB offset,
                        GLsizeiptrARB size,
                        const GLvoid * data, struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   bool busy;

   if (size == 0)
      return;

   assert(intel_obj);

   busy =
      drm_intel_bo_busy(intel_obj->buffer) ||
      drm_intel_bo_references(brw->batch.bo, intel_obj->buffer);

   if (busy) {
      if (size == intel_obj->Base.Size) {
	 /* Replace the current busy bo with fresh data. */
	 drm_intel_bo_unreference(intel_obj->buffer);
	 intel_bufferobj_alloc_buffer(brw, intel_obj);
	 drm_intel_bo_subdata(intel_obj->buffer, 0, size, data);
      } else {
         perf_debug("Using a blit copy to avoid stalling on %ldb "
                    "glBufferSubData() to a busy buffer object.\n",
                    (long)size);
	 drm_intel_bo *temp_bo =
	    drm_intel_bo_alloc(brw->bufmgr, "subdata temp", size, 64);

	 drm_intel_bo_subdata(temp_bo, 0, size, data);

	 intel_emit_linear_blit(brw,
				intel_obj->buffer, offset,
				temp_bo, 0,
				size);

	 drm_intel_bo_unreference(temp_bo);
      }
   } else {
      drm_intel_bo_subdata(intel_obj->buffer, offset, size, data);
   }
}


/**
 * The GetBufferSubData() driver hook.
 *
 * Implements glGetBufferSubData(), which copies a subrange of a buffer
 * object into user memory.
 */
static void
intel_bufferobj_get_subdata(struct gl_context * ctx,
                            GLintptrARB offset,
                            GLsizeiptrARB size,
                            GLvoid * data, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   struct brw_context *brw = brw_context(ctx);

   assert(intel_obj);
   if (drm_intel_bo_references(brw->batch.bo, intel_obj->buffer)) {
      intel_batchbuffer_flush(brw);
   }
   drm_intel_bo_get_subdata(intel_obj->buffer, offset, size, data);
}


/**
 * The MapBufferRange() driver hook.
 *
 * This implements both glMapBufferRange() and glMapBuffer().
 *
 * The goal of this extension is to allow apps to accumulate their rendering
 * at the same time as they accumulate their buffer object.  Without it,
 * you'd end up blocking on execution of rendering every time you mapped
 * the buffer to put new data in.
 *
 * We support it in 3 ways: If unsynchronized, then don't bother
 * flushing the batchbuffer before mapping the buffer, which can save blocking
 * in many cases.  If we would still block, and they allow the whole buffer
 * to be invalidated, then just allocate a new buffer to replace the old one.
 * If not, and we'd block, and they allow the subrange of the buffer to be
 * invalidated, then we can make a new little BO, let them write into that,
 * and blit it into the real BO at unmap time.
 */
static void *
intel_bufferobj_map_range(struct gl_context * ctx,
			  GLintptr offset, GLsizeiptr length,
			  GLbitfield access, struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   /* _mesa_MapBufferRange (GL entrypoint) sets these, but the vbo module also
    * internally uses our functions directly.
    */
   obj->Offset = offset;
   obj->Length = length;
   obj->AccessFlags = access;

   if (intel_obj->buffer == NULL) {
      obj->Pointer = NULL;
      return NULL;
   }

   /* If the access is synchronized (like a normal buffer mapping), then get
    * things flushed out so the later mapping syncs appropriately through GEM.
    * If the user doesn't care about existing buffer contents and mapping would
    * cause us to block, then throw out the old buffer.
    *
    * If they set INVALIDATE_BUFFER, we can pitch the current contents to
    * achieve the required synchronization.
    */
   if (!(access & GL_MAP_UNSYNCHRONIZED_BIT)) {
      if (drm_intel_bo_references(brw->batch.bo, intel_obj->buffer)) {
	 if (access & GL_MAP_INVALIDATE_BUFFER_BIT) {
	    drm_intel_bo_unreference(intel_obj->buffer);
	    intel_bufferobj_alloc_buffer(brw, intel_obj);
	 } else {
            perf_debug("Stalling on the GPU for mapping a busy buffer "
                       "object\n");
	    intel_batchbuffer_flush(brw);
	 }
      } else if (drm_intel_bo_busy(intel_obj->buffer) &&
		 (access & GL_MAP_INVALIDATE_BUFFER_BIT)) {
	 drm_intel_bo_unreference(intel_obj->buffer);
	 intel_bufferobj_alloc_buffer(brw, intel_obj);
      }
   }

   /* If the user is mapping a range of an active buffer object but
    * doesn't require the current contents of that range, make a new
    * BO, and we'll copy what they put in there out at unmap or
    * FlushRange time.
    */
   if ((access & GL_MAP_INVALIDATE_RANGE_BIT) &&
       drm_intel_bo_busy(intel_obj->buffer)) {
      if (access & GL_MAP_FLUSH_EXPLICIT_BIT) {
	 intel_obj->range_map_buffer = malloc(length);
	 obj->Pointer = intel_obj->range_map_buffer;
      } else {
	 intel_obj->range_map_bo = drm_intel_bo_alloc(brw->bufmgr,
						      "range map",
						      length, 64);
	 if (!(access & GL_MAP_READ_BIT)) {
	    drm_intel_gem_bo_map_gtt(intel_obj->range_map_bo);
	 } else {
	    drm_intel_bo_map(intel_obj->range_map_bo,
			     (access & GL_MAP_WRITE_BIT) != 0);
	 }
	 obj->Pointer = intel_obj->range_map_bo->virtual;
      }
      return obj->Pointer;
   }

   if (access & GL_MAP_UNSYNCHRONIZED_BIT)
      drm_intel_gem_bo_map_unsynchronized(intel_obj->buffer);
   else if (!(access & GL_MAP_READ_BIT)) {
      drm_intel_gem_bo_map_gtt(intel_obj->buffer);
   } else {
      drm_intel_bo_map(intel_obj->buffer, (access & GL_MAP_WRITE_BIT) != 0);
   }

   obj->Pointer = intel_obj->buffer->virtual + offset;
   return obj->Pointer;
}

/**
 * The FlushMappedBufferRange() driver hook.
 *
 * Implements glFlushMappedBufferRange(), which signifies that modifications
 * have been made to a range of a mapped buffer, and it should be flushed.
 *
 * This is only used for buffers mapped with GL_MAP_FLUSH_EXPLICIT_BIT.
 *
 * Ideally we'd use a BO to avoid taking up cache space for the temporary
 * data, but FlushMappedBufferRange may be followed by further writes to
 * the pointer, so we would have to re-map after emitting our blit, which
 * would defeat the point.
 */
static void
intel_bufferobj_flush_mapped_range(struct gl_context *ctx,
				   GLintptr offset, GLsizeiptr length,
				   struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   drm_intel_bo *temp_bo;

   /* Unless we're in the range map using a temporary system buffer,
    * there's no work to do.
    */
   if (intel_obj->range_map_buffer == NULL)
      return;

   if (length == 0)
      return;

   temp_bo = drm_intel_bo_alloc(brw->bufmgr, "range map flush", length, 64);

   drm_intel_bo_subdata(temp_bo, 0, length, intel_obj->range_map_buffer);

   intel_emit_linear_blit(brw,
			  intel_obj->buffer, obj->Offset + offset,
			  temp_bo, 0,
			  length);

   drm_intel_bo_unreference(temp_bo);
}


/**
 * The UnmapBuffer() driver hook.
 *
 * Implements glUnmapBuffer().
 */
static GLboolean
intel_bufferobj_unmap(struct gl_context * ctx, struct gl_buffer_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   assert(obj->Pointer);
   if (intel_obj->range_map_buffer != NULL) {
      /* Since we've emitted some blits to buffers that will (likely) be used
       * in rendering operations in other cache domains in this batch, emit a
       * flush.  Once again, we wish for a domain tracker in libdrm to cover
       * usage inside of a batchbuffer.
       */
      intel_batchbuffer_emit_mi_flush(brw);
      free(intel_obj->range_map_buffer);
      intel_obj->range_map_buffer = NULL;
   } else if (intel_obj->range_map_bo != NULL) {
      drm_intel_bo_unmap(intel_obj->range_map_bo);

      intel_emit_linear_blit(brw,
			     intel_obj->buffer, obj->Offset,
			     intel_obj->range_map_bo, 0,
			     obj->Length);

      /* Since we've emitted some blits to buffers that will (likely) be used
       * in rendering operations in other cache domains in this batch, emit a
       * flush.  Once again, we wish for a domain tracker in libdrm to cover
       * usage inside of a batchbuffer.
       */
      intel_batchbuffer_emit_mi_flush(brw);

      drm_intel_bo_unreference(intel_obj->range_map_bo);
      intel_obj->range_map_bo = NULL;
   } else if (intel_obj->buffer != NULL) {
      drm_intel_bo_unmap(intel_obj->buffer);
   }
   obj->Pointer = NULL;
   obj->Offset = 0;
   obj->Length = 0;

   return true;
}

drm_intel_bo *
intel_bufferobj_buffer(struct brw_context *brw,
                       struct intel_buffer_object *intel_obj,
		       GLuint flag)
{
   if (intel_obj->buffer == NULL)
      intel_bufferobj_alloc_buffer(brw, intel_obj);

   return intel_obj->buffer;
}

drm_intel_bo *
intel_bufferobj_source(struct brw_context *brw,
                       struct intel_buffer_object *intel_obj,
		       GLuint align, GLuint *offset)
{
   *offset = intel_obj->offset;
   return intel_obj->buffer;
}

/**
 * The CopyBufferSubData() driver hook.
 *
 * Implements glCopyBufferSubData(), which copies a portion of one buffer
 * object's data to another.  Independent source and destination offsets
 * are allowed.
 */
static void
intel_bufferobj_copy_subdata(struct gl_context *ctx,
			     struct gl_buffer_object *src,
			     struct gl_buffer_object *dst,
			     GLintptr read_offset, GLintptr write_offset,
			     GLsizeiptr size)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_buffer_object *intel_src = intel_buffer_object(src);
   struct intel_buffer_object *intel_dst = intel_buffer_object(dst);
   drm_intel_bo *src_bo, *dst_bo;
   GLuint src_offset;

   if (size == 0)
      return;

   dst_bo = intel_bufferobj_buffer(brw, intel_dst, INTEL_WRITE_PART);
   src_bo = intel_bufferobj_source(brw, intel_src, 64, &src_offset);

   intel_emit_linear_blit(brw,
			  dst_bo, write_offset,
			  src_bo, read_offset + src_offset, size);

   /* Since we've emitted some blits to buffers that will (likely) be used
    * in rendering operations in other cache domains in this batch, emit a
    * flush.  Once again, we wish for a domain tracker in libdrm to cover
    * usage inside of a batchbuffer.
    */
   intel_batchbuffer_emit_mi_flush(brw);
}

void
intelInitBufferObjectFuncs(struct dd_function_table *functions)
{
   functions->NewBufferObject = intel_bufferobj_alloc;
   functions->DeleteBuffer = intel_bufferobj_free;
   functions->BufferData = intel_bufferobj_data;
   functions->BufferSubData = intel_bufferobj_subdata;
   functions->GetBufferSubData = intel_bufferobj_get_subdata;
   functions->MapBufferRange = intel_bufferobj_map_range;
   functions->FlushMappedBufferRange = intel_bufferobj_flush_mapped_range;
   functions->UnmapBuffer = intel_bufferobj_unmap;
   functions->CopyBufferSubData = intel_bufferobj_copy_subdata;
}
