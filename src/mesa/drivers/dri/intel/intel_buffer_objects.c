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


#include "main/imports.h"
#include "main/mtypes.h"
#include "main/bufferobj.h"

#include "intel_context.h"
#include "intel_buffer_objects.h"
#include "intel_batchbuffer.h"
#include "intel_regions.h"


/** Allocates a new dri_bo to store the data for the buffer object. */
static void
intel_bufferobj_alloc_buffer(struct intel_context *intel,
			     struct intel_buffer_object *intel_obj)
{
   intel_obj->buffer = dri_bo_alloc(intel->bufmgr, "bufferobj",
				    intel_obj->Base.Size, 64);
}

/**
 * There is some duplication between mesa's bufferobjects and our
 * bufmgr buffers.  Both have an integer handle and a hashtable to
 * lookup an opaque structure.  It would be nice if the handles and
 * internal structure where somehow shared.
 */
static struct gl_buffer_object *
intel_bufferobj_alloc(GLcontext * ctx, GLuint name, GLenum target)
{
   struct intel_buffer_object *obj = CALLOC_STRUCT(intel_buffer_object);

   _mesa_initialize_buffer_object(&obj->Base, name, target);

   obj->buffer = NULL;

   return &obj->Base;
}

/* Break the COW tie to the region.  The region gets to keep the data.
 */
void
intel_bufferobj_release_region(struct intel_context *intel,
                               struct intel_buffer_object *intel_obj)
{
   assert(intel_obj->region->buffer == intel_obj->buffer);
   intel_obj->region->pbo = NULL;
   intel_obj->region = NULL;

   dri_bo_unreference(intel_obj->buffer);
   intel_obj->buffer = NULL;
}

/* Break the COW tie to the region.  Both the pbo and the region end
 * up with a copy of the data.
 */
void
intel_bufferobj_cow(struct intel_context *intel,
                    struct intel_buffer_object *intel_obj)
{
   assert(intel_obj->region);
   intel_region_cow(intel, intel_obj->region);
}


/**
 * Deallocate/free a vertex/pixel buffer object.
 * Called via glDeleteBuffersARB().
 */
static void
intel_bufferobj_free(GLcontext * ctx, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   assert(!obj->Pointer); /* Mesa should have unmapped it */

   _mesa_free(intel_obj->sys_buffer);
   if (intel_obj->region) {
      intel_bufferobj_release_region(intel, intel_obj);
   }
   else if (intel_obj->buffer) {
      dri_bo_unreference(intel_obj->buffer);
   }

   _mesa_free(intel_obj);
}



/**
 * Allocate space for and store data in a buffer object.  Any data that was
 * previously stored in the buffer object is lost.  If data is NULL,
 * memory will be allocated, but no copy will occur.
 * Called via glBufferDataARB().
 */
static void
intel_bufferobj_data(GLcontext * ctx,
                     GLenum target,
                     GLsizeiptrARB size,
                     const GLvoid * data,
                     GLenum usage, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   intel_obj->Base.Size = size;
   intel_obj->Base.Usage = usage;

   assert(!obj->Pointer); /* Mesa should have unmapped it */

   if (intel_obj->region)
      intel_bufferobj_release_region(intel, intel_obj);

   if (intel_obj->buffer != NULL) {
      dri_bo_unreference(intel_obj->buffer);
      intel_obj->buffer = NULL;
   }
   _mesa_free(intel_obj->sys_buffer);
   intel_obj->sys_buffer = NULL;

   if (size != 0) {
#ifdef I915
      /* On pre-965, stick VBOs in system memory, as we're always doing swtnl
       * with their contents anyway.
       */
      if (target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) {
	 intel_obj->sys_buffer = _mesa_malloc(size);
	 if (intel_obj->sys_buffer != NULL) {
	    if (data != NULL)
	       memcpy(intel_obj->sys_buffer, data, size);
	    return;
	 }
      }
#endif
      intel_bufferobj_alloc_buffer(intel, intel_obj);

      if (data != NULL)
	 dri_bo_subdata(intel_obj->buffer, 0, size, data);
   }
}


/**
 * Replace data in a subrange of buffer object.  If the data range
 * specified by size + offset extends beyond the end of the buffer or
 * if data is NULL, no copy is performed.
 * Called via glBufferSubDataARB().
 */
static void
intel_bufferobj_subdata(GLcontext * ctx,
                        GLenum target,
                        GLintptrARB offset,
                        GLsizeiptrARB size,
                        const GLvoid * data, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);

   if (intel_obj->region)
      intel_bufferobj_cow(intel, intel_obj);

   if (intel_obj->sys_buffer)
      memcpy((char *)intel_obj->sys_buffer + offset, data, size);
   else
      dri_bo_subdata(intel_obj->buffer, offset, size, data);
}


/**
 * Called via glGetBufferSubDataARB().
 */
static void
intel_bufferobj_get_subdata(GLcontext * ctx,
                            GLenum target,
                            GLintptrARB offset,
                            GLsizeiptrARB size,
                            GLvoid * data, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   dri_bo_get_subdata(intel_obj->buffer, offset, size, data);
}



/**
 * Called via glMapBufferARB().
 */
static void *
intel_bufferobj_map(GLcontext * ctx,
                    GLenum target,
                    GLenum access, struct gl_buffer_object *obj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);
   GLboolean read_only = (access == GL_READ_ONLY_ARB);
   GLboolean write_only = (access == GL_WRITE_ONLY_ARB);

   assert(intel_obj);

   if (intel_obj->sys_buffer) {
      obj->Pointer = intel_obj->sys_buffer;
      return obj->Pointer;
   }

   if (intel_obj->region)
      intel_bufferobj_cow(intel, intel_obj);

   if (intel_obj->buffer == NULL) {
      obj->Pointer = NULL;
      return NULL;
   }

   if (write_only && intel->intelScreen->kernel_exec_fencing) {
      drm_intel_gem_bo_map_gtt(intel_obj->buffer);
      intel_obj->mapped_gtt = GL_TRUE;
   } else {
      drm_intel_bo_map(intel_obj->buffer, !read_only);
      intel_obj->mapped_gtt = GL_FALSE;
   }

   obj->Pointer = intel_obj->buffer->virtual;
   return obj->Pointer;
}


/**
 * Called via glMapBufferARB().
 */
static GLboolean
intel_bufferobj_unmap(GLcontext * ctx,
                      GLenum target, struct gl_buffer_object *obj)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   assert(intel_obj);
   if (intel_obj->sys_buffer != NULL) {
      assert(obj->Pointer);
      obj->Pointer = NULL;
   } else if (intel_obj->buffer != NULL) {
      assert(obj->Pointer);
      if (intel_obj->mapped_gtt) {
	 drm_intel_gem_bo_unmap_gtt(intel_obj->buffer);
      } else {
	 drm_intel_bo_unmap(intel_obj->buffer);
      }
      obj->Pointer = NULL;
   }
   return GL_TRUE;
}

dri_bo *
intel_bufferobj_buffer(struct intel_context *intel,
                       struct intel_buffer_object *intel_obj, GLuint flag)
{
   if (intel_obj->region) {
      if (flag == INTEL_WRITE_PART)
         intel_bufferobj_cow(intel, intel_obj);
      else if (flag == INTEL_WRITE_FULL) {
         intel_bufferobj_release_region(intel, intel_obj);
	 intel_bufferobj_alloc_buffer(intel, intel_obj);
      }
   }

   if (intel_obj->buffer == NULL) {
      intel_bufferobj_alloc_buffer(intel, intel_obj);
      intel_bufferobj_subdata(&intel->ctx,
			      GL_ARRAY_BUFFER_ARB,
			      0,
			      intel_obj->Base.Size,
			      intel_obj->sys_buffer,
			      &intel_obj->Base);
      _mesa_free(intel_obj->sys_buffer);
      intel_obj->sys_buffer = NULL;
   }

   return intel_obj->buffer;
}

void
intel_bufferobj_init(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;

   ctx->Driver.NewBufferObject = intel_bufferobj_alloc;
   ctx->Driver.DeleteBuffer = intel_bufferobj_free;
   ctx->Driver.BufferData = intel_bufferobj_data;
   ctx->Driver.BufferSubData = intel_bufferobj_subdata;
   ctx->Driver.GetBufferSubData = intel_bufferobj_get_subdata;
   ctx->Driver.MapBuffer = intel_bufferobj_map;
   ctx->Driver.UnmapBuffer = intel_bufferobj_unmap;
}
