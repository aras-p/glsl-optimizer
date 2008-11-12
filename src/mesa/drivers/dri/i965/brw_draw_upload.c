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

#include <stdlib.h>

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/enums.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_fallback.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_tex.h"

static GLuint double_types[5] = {
   0,
   BRW_SURFACEFORMAT_R64_FLOAT,
   BRW_SURFACEFORMAT_R64G64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64A64_FLOAT
};

static GLuint float_types[5] = {
   0,
   BRW_SURFACEFORMAT_R32_FLOAT,
   BRW_SURFACEFORMAT_R32G32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32A32_FLOAT
};

static GLuint uint_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_UNORM,
   BRW_SURFACEFORMAT_R32G32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_UNORM
};

static GLuint uint_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_USCALED,
   BRW_SURFACEFORMAT_R32G32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_USCALED
};

static GLuint int_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SNORM,
   BRW_SURFACEFORMAT_R32G32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_SNORM
};

static GLuint int_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SSCALED,
   BRW_SURFACEFORMAT_R32G32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_SSCALED
};

static GLuint ushort_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_UNORM,
   BRW_SURFACEFORMAT_R16G16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_UNORM
};

static GLuint ushort_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_USCALED,
   BRW_SURFACEFORMAT_R16G16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_USCALED
};

static GLuint short_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SNORM,
   BRW_SURFACEFORMAT_R16G16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_SNORM
};

static GLuint short_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SSCALED,
   BRW_SURFACEFORMAT_R16G16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_SSCALED
};

static GLuint ubyte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_UNORM,
   BRW_SURFACEFORMAT_R8G8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_UNORM
};

static GLuint ubyte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_USCALED,
   BRW_SURFACEFORMAT_R8G8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_USCALED
};

static GLuint byte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SNORM,
   BRW_SURFACEFORMAT_R8G8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_SNORM
};

static GLuint byte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SSCALED,
   BRW_SURFACEFORMAT_R8G8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_SSCALED
};


static GLuint get_surface_type( GLenum type, GLuint size, GLboolean normalized )
{
   if (INTEL_DEBUG & DEBUG_VERTS)
      _mesa_printf("type %s size %d normalized %d\n", 
		   _mesa_lookup_enum_by_nr(type), size, normalized);

   if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_norm[size];
      default: assert(0); return 0;
      }      
   }
   else {
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_INT: return int_types_scale[size];
      case GL_SHORT: return short_types_scale[size];
      case GL_BYTE: return byte_types_scale[size];
      case GL_UNSIGNED_INT: return uint_types_scale[size];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size];
      default: assert(0); return 0;
      }      
   }
}


static GLuint get_size( GLenum type )
{
   switch (type) {
   case GL_DOUBLE: return sizeof(GLdouble);
   case GL_FLOAT: return sizeof(GLfloat);
   case GL_INT: return sizeof(GLint);
   case GL_SHORT: return sizeof(GLshort);
   case GL_BYTE: return sizeof(GLbyte);
   case GL_UNSIGNED_INT: return sizeof(GLuint);
   case GL_UNSIGNED_SHORT: return sizeof(GLushort);
   case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
   default: return 0;
   }      
}

static GLuint get_index_type(GLenum type) 
{
   switch (type) {
   case GL_UNSIGNED_BYTE:  return BRW_INDEX_BYTE;
   case GL_UNSIGNED_SHORT: return BRW_INDEX_WORD;
   case GL_UNSIGNED_INT:   return BRW_INDEX_DWORD;
   default: assert(0); return 0;
   }
}

static void wrap_buffers( struct brw_context *brw,
			  GLuint size )
{
   if (size < BRW_UPLOAD_INIT_SIZE)
      size = BRW_UPLOAD_INIT_SIZE;

   brw->vb.upload.offset = 0;

   if (brw->vb.upload.bo != NULL)
      dri_bo_unreference(brw->vb.upload.bo);
   brw->vb.upload.bo = dri_bo_alloc(brw->intel.bufmgr, "temporary VBO",
				    size, 1);

   /* Set the internal VBO\ to no-backing-store.  We only use them as a
    * temporary within a brw_try_draw_prims while the lock is held.
    */
   /* DON'T DO THIS AS IF WE HAVE TO RE-ORG MEMORY WE NEED SOMEWHERE WITH
      FAKE TO PUSH THIS STUFF */
//   if (!brw->intel.ttm)
//      dri_bo_fake_disable_backing_store(brw->vb.upload.bo, NULL, NULL);
}

static void get_space( struct brw_context *brw,
		       GLuint size,
		       dri_bo **bo_return,
		       GLuint *offset_return )
{
   size = ALIGN(size, 64);

   if (brw->vb.upload.bo == NULL ||
       brw->vb.upload.offset + size > brw->vb.upload.bo->size) {
      wrap_buffers(brw, size);
   }

   assert(*bo_return == NULL);
   dri_bo_reference(brw->vb.upload.bo);
   *bo_return = brw->vb.upload.bo;
   *offset_return = brw->vb.upload.offset;
   brw->vb.upload.offset += size;
}

static void
copy_array_to_vbo_array( struct brw_context *brw,
			 struct brw_vertex_element *element,
			 GLuint dst_stride)
{
   GLuint size = element->count * dst_stride;

   get_space(brw, size, &element->bo, &element->offset);

   if (element->glarray->StrideB == 0) {
      assert(element->count == 1);
      element->stride = 0;
   } else {
      element->stride = dst_stride;
   }

   if (dst_stride == element->glarray->StrideB) {
      dri_bo_subdata(element->bo,
		     element->offset,
		     size,
		     element->glarray->Ptr);
   } else {
      void *data;
      char *dest;
      const char *src = element->glarray->Ptr;
      int i;

      data = _mesa_malloc(dst_stride * element->count);
      dest = data;
      for (i = 0; i < element->count; i++) {
	 memcpy(dest, src, dst_stride);
	 src += element->glarray->StrideB;
	 dest += dst_stride;
      }

      dri_bo_subdata(element->bo,
		     element->offset,
		     size,
		     data);
      _mesa_free(data);
   }
}

static void brw_prepare_vertices(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   GLuint tmp = brw->vs.prog_data->inputs_read; 
   GLuint i;
   const unsigned char *ptr = NULL;
   GLuint interleave = 0;
   unsigned int min_index = brw->vb.min_index;
   unsigned int max_index = brw->vb.max_index;

   struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
   GLuint nr_enabled = 0;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;

   /* First build an array of pointers to ve's in vb.inputs_read
    */
   if (0)
      _mesa_printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   while (tmp) {
      GLuint i = _mesa_ffsll(tmp)-1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      tmp &= ~(1<<i);
      enabled[nr_enabled++] = input;
   }

   /* XXX: In the rare cases where this happens we fallback all
    * the way to software rasterization, although a tnl fallback
    * would be sufficient.  I don't know of *any* real world
    * cases with > 17 vertex attributes enabled, so it probably
    * isn't an issue at this point.
    */
   if (nr_enabled >= BRW_VEP_MAX) {
      intel->Fallback = 1;
      return;
   }

   for (i = 0; i < nr_enabled; i++) {
      struct brw_vertex_element *input = enabled[i];

      input->element_size = get_size(input->glarray->Type) * input->glarray->Size;
      input->count = input->glarray->StrideB ? max_index + 1 - min_index : 1;

      if (input->glarray->BufferObj->Name != 0) {
	 struct intel_buffer_object *intel_buffer =
	    intel_buffer_object(input->glarray->BufferObj);

	 /* Named buffer object: Just reference its contents directly. */
	 dri_bo_unreference(input->bo);
	 input->bo = intel_bufferobj_buffer(intel, intel_buffer,
					    INTEL_READ);
	 dri_bo_reference(input->bo);
	 input->offset = (unsigned long)input->glarray->Ptr;
	 input->stride = input->glarray->StrideB;
      } else {
	 if (input->bo != NULL) {
	    /* Already-uploaded vertex data is present from a previous
	     * prepare_vertices, but we had to re-validate state due to
	     * check_aperture failing and a new batch being produced.
	     */
	    continue;
	 }

	 /* Queue the buffer object up to be uploaded in the next pass,
	  * when we've decided if we're doing interleaved or not.
	  */
	 if (i == 0) {
	    /* Position array not properly enabled:
	     */
            if (input->glarray->StrideB == 0) {
               intel->Fallback = 1;
               return;
            }

	    interleave = input->glarray->StrideB;
	    ptr = input->glarray->Ptr;
	 }
	 else if (interleave != input->glarray->StrideB ||
		  (const unsigned char *)input->glarray->Ptr - ptr < 0 ||
		  (const unsigned char *)input->glarray->Ptr - ptr > interleave)
	 {
	    interleave = 0;
	 }

	 upload[nr_uploads++] = input;
	 
	 /* We rebase drawing to start at element zero only when
	  * varyings are not in vbos, which means we can end up
	  * uploading non-varying arrays (stride != 0) when min_index
	  * is zero.  This doesn't matter as the amount to upload is
	  * the same for these arrays whether the draw call is rebased
	  * or not - we just have to upload the one element.
	  */
	 assert(min_index == 0 || input->glarray->StrideB == 0);
      }
   }

   /* Handle any arrays to be uploaded. */
   if (nr_uploads > 1 && interleave && interleave <= 256) {
      /* All uploads are interleaved, so upload the arrays together as
       * interleaved.  First, upload the contents and set up upload[0].
       */
      copy_array_to_vbo_array(brw, upload[0], interleave);

      for (i = 1; i < nr_uploads; i++) {
	 /* Then, just point upload[i] at upload[0]'s buffer. */
	 upload[i]->stride = interleave;
	 upload[i]->offset = upload[0]->offset +
	    ((const unsigned char *)upload[i]->glarray->Ptr - ptr);
	 upload[i]->bo = upload[0]->bo;
	 dri_bo_reference(upload[i]->bo);
      }
   }
   else {
      /* Upload non-interleaved arrays */
      for (i = 0; i < nr_uploads; i++) {
          copy_array_to_vbo_array(brw, upload[i], upload[i]->element_size);
      }
   }

   brw_prepare_query_begin(brw);

   for (i = 0; i < nr_enabled; i++) {
      struct brw_vertex_element *input = enabled[i];

      brw_add_validated_bo(brw, input->bo);
   }
}

static void brw_emit_vertices(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   GLuint tmp = brw->vs.prog_data->inputs_read;
   struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
   GLuint i;
   GLuint nr_enabled = 0;

  /* Accumulate the list of enabled arrays. */
   while (tmp) {
      i = _mesa_ffsll(tmp)-1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      tmp &= ~(1<<i);
      enabled[nr_enabled++] = input;
   }

   brw_emit_query_begin(brw);

   /* Now emit VB and VEP state packets.
    *
    * This still defines a hardware VB for each input, even if they
    * are interleaved or from the same VBO.  TBD if this makes a
    * performance difference.
    */
   BEGIN_BATCH(1 + nr_enabled * 4, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_BUFFER << 16) |
	     ((1 + nr_enabled * 4) - 2));

   for (i = 0; i < nr_enabled; i++) {
      struct brw_vertex_element *input = enabled[i];

      OUT_BATCH((i << BRW_VB0_INDEX_SHIFT) |
		BRW_VB0_ACCESS_VERTEXDATA |
		(input->stride << BRW_VB0_PITCH_SHIFT));
      OUT_RELOC(input->bo,
		I915_GEM_DOMAIN_VERTEX, 0,
		input->offset);
      OUT_BATCH(brw->vb.max_index);
      OUT_BATCH(0); /* Instance data step rate */
   }
   ADVANCE_BATCH();

   BEGIN_BATCH(1 + nr_enabled * 2, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | ((1 + nr_enabled * 2) - 2));
   for (i = 0; i < nr_enabled; i++) {
      struct brw_vertex_element *input = enabled[i];
      uint32_t format = get_surface_type(input->glarray->Type,
					 input->glarray->Size,
					 input->glarray->Normalized);
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      switch (input->glarray->Size) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0;
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0;
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0;
      case 3: comp3 = BRW_VE1_COMPONENT_STORE_1_FLT;
	 break;
      }

      OUT_BATCH((i << BRW_VE0_INDEX_SHIFT) |
		BRW_VE0_VALID |
		(format << BRW_VE0_FORMAT_SHIFT) |
		(0 << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
		(comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
		(comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
		(comp3 << BRW_VE1_COMPONENT_3_SHIFT) |
		((i * 4) << BRW_VE1_DST_OFFSET_SHIFT));
   }
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_vertices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_VERTICES,
      .cache = 0,
   },
   .prepare = brw_prepare_vertices,
   .emit = brw_emit_vertices,
};

static void brw_prepare_indices(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;
   dri_bo *bo = NULL;
   struct gl_buffer_object *bufferobj;
   GLuint offset;

   if (index_buffer == NULL)
      return;

   ib_size = get_size(index_buffer->type) * index_buffer->count;
   bufferobj = index_buffer->obj;;

   /* Turn into a proper VBO:
    */
   if (!bufferobj->Name) {
     
      /* Get new bufferobj, offset:
       */
      get_space(brw, ib_size, &bo, &offset);

      /* Straight upload
       */
      dri_bo_subdata(bo, offset, ib_size, index_buffer->ptr);
   } else {
      offset = (GLuint)index_buffer->ptr;

      /* If the index buffer isn't aligned to its element size, we have to
       * rebase it into a temporary.
       */
       if ((get_size(index_buffer->type) - 1) & offset) {
           GLubyte *map = ctx->Driver.MapBuffer(ctx,
                                                GL_ELEMENT_ARRAY_BUFFER_ARB,
                                                GL_DYNAMIC_DRAW_ARB,
                                                bufferobj);
           map += offset;

	   get_space(brw, ib_size, &bo, &offset);

	   dri_bo_subdata(bo, offset, ib_size, map);

           ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, bufferobj);
       } else {
	  bo = intel_bufferobj_buffer(intel, intel_buffer_object(bufferobj),
				      INTEL_READ);
	  dri_bo_reference(bo);
       }
   }

   dri_bo_unreference(brw->ib.bo);
   brw->ib.bo = bo;
   brw->ib.offset = offset;

   brw_add_validated_bo(brw, brw->ib.bo);
}

static void brw_emit_indices(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;

   if (index_buffer == NULL)
      return;

   ib_size = get_size(index_buffer->type) * index_buffer->count;

   /* Emit the indexbuffer packet:
    */
   {
      struct brw_indexbuffer ib;

      memset(&ib, 0, sizeof(ib));
   
      ib.header.bits.opcode = CMD_INDEX_BUFFER;
      ib.header.bits.length = sizeof(ib)/4 - 2;
      ib.header.bits.index_format = get_index_type(index_buffer->type);
      ib.header.bits.cut_index_enable = 0;
   

      BEGIN_BATCH(4, IGNORE_CLIPRECTS);
      OUT_BATCH( ib.header.dword );
      OUT_RELOC(brw->ib.bo,
		I915_GEM_DOMAIN_VERTEX, 0,
		brw->ib.offset);
      OUT_RELOC(brw->ib.bo,
		I915_GEM_DOMAIN_VERTEX, 0,
		brw->ib.offset + ib_size);
      OUT_BATCH( 0 );
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_INDICES,
      .cache = 0,
   },
   .prepare = brw_prepare_indices,
   .emit = brw_emit_indices,
};
