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

#include "pipe/p_context.h"

#include "util/u_upload_mgr.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_fallback.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_tex.h"




unsigned brw_translate_surface_format( unsigned id )
{
   switch (id) {
   case PIPE_FORMAT_R64_FLOAT:
      return BRW_SURFACEFORMAT_R64_FLOAT;
   case PIPE_FORMAT_R64G64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64_FLOAT;
   case PIPE_FORMAT_R64G64B64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64B64_FLOAT;
   case PIPE_FORMAT_R64G64B64A64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64B64A64_FLOAT;

   case PIPE_FORMAT_R32_FLOAT:
      return BRW_SURFACEFORMAT_R32_FLOAT;
   case PIPE_FORMAT_R32G32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32_FLOAT;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32B32_FLOAT;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   case PIPE_FORMAT_R32_UNORM:
      return BRW_SURFACEFORMAT_R32_UNORM;
   case PIPE_FORMAT_R32G32_UNORM:
      return BRW_SURFACEFORMAT_R32G32_UNORM;
   case PIPE_FORMAT_R32G32B32_UNORM:
      return BRW_SURFACEFORMAT_R32G32B32_UNORM;
   case PIPE_FORMAT_R32G32B32A32_UNORM:
      return BRW_SURFACEFORMAT_R32G32B32A32_UNORM;

   case PIPE_FORMAT_R32_USCALED:
      return BRW_SURFACEFORMAT_R32_USCALED;
   case PIPE_FORMAT_R32G32_USCALED:
      return BRW_SURFACEFORMAT_R32G32_USCALED;
   case PIPE_FORMAT_R32G32B32_USCALED:
      return BRW_SURFACEFORMAT_R32G32B32_USCALED;
   case PIPE_FORMAT_R32G32B32A32_USCALED:
      return BRW_SURFACEFORMAT_R32G32B32A32_USCALED;

   case PIPE_FORMAT_R32_SNORM:
      return BRW_SURFACEFORMAT_R32_SNORM;
   case PIPE_FORMAT_R32G32_SNORM:
      return BRW_SURFACEFORMAT_R32G32_SNORM;
   case PIPE_FORMAT_R32G32B32_SNORM:
      return BRW_SURFACEFORMAT_R32G32B32_SNORM;
   case PIPE_FORMAT_R32G32B32A32_SNORM:
      return BRW_SURFACEFORMAT_R32G32B32A32_SNORM;

   case PIPE_FORMAT_R32_SSCALED:
      return BRW_SURFACEFORMAT_R32_SSCALED;
   case PIPE_FORMAT_R32G32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32_SSCALED;
   case PIPE_FORMAT_R32G32B32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32B32_SSCALED;
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32B32A32_SSCALED;

   case PIPE_FORMAT_R16_UNORM:
      return BRW_SURFACEFORMAT_R16_UNORM;
   case PIPE_FORMAT_R16G16_UNORM:
      return BRW_SURFACEFORMAT_R16G16_UNORM;
   case PIPE_FORMAT_R16G16B16_UNORM:
      return BRW_SURFACEFORMAT_R16G16B16_UNORM;
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      return BRW_SURFACEFORMAT_R16G16B16A16_UNORM;

   case PIPE_FORMAT_R16_USCALED:
      return BRW_SURFACEFORMAT_R16_USCALED;
   case PIPE_FORMAT_R16G16_USCALED:
      return BRW_SURFACEFORMAT_R16G16_USCALED;
   case PIPE_FORMAT_R16G16B16_USCALED:
      return BRW_SURFACEFORMAT_R16G16B16_USCALED;
   case PIPE_FORMAT_R16G16B16A16_USCALED:
      return BRW_SURFACEFORMAT_R16G16B16A16_USCALED;

   case PIPE_FORMAT_R16_SNORM:
      return BRW_SURFACEFORMAT_R16_SNORM;
   case PIPE_FORMAT_R16G16_SNORM:
      return BRW_SURFACEFORMAT_R16G16_SNORM;
   case PIPE_FORMAT_R16G16B16_SNORM:
      return BRW_SURFACEFORMAT_R16G16B16_SNORM;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return BRW_SURFACEFORMAT_R16G16B16A16_SNORM;

   case PIPE_FORMAT_R16_SSCALED:
      return BRW_SURFACEFORMAT_R16_SSCALED;
   case PIPE_FORMAT_R16G16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16_SSCALED;
   case PIPE_FORMAT_R16G16B16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16B16_SSCALED;
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16B16A16_SSCALED;

   case PIPE_FORMAT_R8_UNORM:
      return BRW_SURFACEFORMAT_R8_UNORM;
   case PIPE_FORMAT_R8G8_UNORM:
      return BRW_SURFACEFORMAT_R8G8_UNORM;
   case PIPE_FORMAT_R8G8B8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case PIPE_FORMAT_R8_USCALED:
      return BRW_SURFACEFORMAT_R8_USCALED;
   case PIPE_FORMAT_R8G8_USCALED:
      return BRW_SURFACEFORMAT_R8G8_USCALED;
   case PIPE_FORMAT_R8G8B8_USCALED:
      return BRW_SURFACEFORMAT_R8G8B8_USCALED;
   case PIPE_FORMAT_R8G8B8A8_USCALED:
      return BRW_SURFACEFORMAT_R8G8B8A8_USCALED;

   case PIPE_FORMAT_R8_SNORM:
      return BRW_SURFACEFORMAT_R8_SNORM;
   case PIPE_FORMAT_R8G8_SNORM:
      return BRW_SURFACEFORMAT_R8G8_SNORM;
   case PIPE_FORMAT_R8G8B8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8_SNORM;
   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_SNORM;

   case PIPE_FORMAT_R8_SSCALED:
      return BRW_SURFACEFORMAT_R8_SSCALED;
   case PIPE_FORMAT_R8G8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8_SSCALED;
   case PIPE_FORMAT_R8G8B8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8B8_SSCALED;
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8B8A8_SSCALED;

   default:
      assert(0);
      return 0;
   }
}

static unsigned get_index_type(int type)
{
   switch (type) {
   case 1: return BRW_INDEX_BYTE;
   case 2: return BRW_INDEX_WORD;
   case 4: return BRW_INDEX_DWORD;
   default: assert(0); return 0;
   }
}



static boolean brw_prepare_vertices(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   GLbitfield vs_inputs = brw->vs.prog_data->inputs_read; 
   GLuint i;
   const unsigned char *ptr = NULL;
   GLuint interleave = 0;
   unsigned int min_index = brw->vb.min_index;
   unsigned int max_index = brw->vb.max_index;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;

   /* First build an array of pointers to ve's in vb.inputs_read
    */
   if (0)
      _mesa_printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);



   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];

      input->element_size = get_size(input->glarray->Type) * input->glarray->Size;

      if (brw_is_user_buffer(vb)) {
	 u_upload_buffer( brw->upload, 
			  min_index * vb->stride,
			  (max_index + 1 - min_index) * vb->stride,
			  &offset,
			  &buffer );
      }
      else
      {
	 offset = 0;
	 buffer = vb->buffer;
	 count = stride == 0 ? 1 : max_index + 1 - min_index;
      }

      /* Named buffer object: Just reference its contents directly. */
      dri_bo_unreference(input->bo);
      input->bo = intel_bufferobj_buffer(intel, intel_buffer,
					 INTEL_READ);
      dri_bo_reference(input->bo);

      input->offset = (unsigned long)offset;
      input->stride = vb->stride;
      input->count = count;

      assert(input->offset < input->bo->size);
   }

   brw_prepare_query_begin(brw);

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];

      brw_add_validated_bo(brw, input->bo);
   }
}

static void brw_emit_vertices(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   brw_emit_query_begin(brw);

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (brw->vb.nr_enabled == 0) {
      BEGIN_BATCH(3, IGNORE_CLIPRECTS);
      OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | 1);
      OUT_BATCH((0 << BRW_VE0_INDEX_SHIFT) |
		BRW_VE0_VALID |
		(BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
		(0 << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_1_FLT << BRW_VE1_COMPONENT_3_SHIFT));
      ADVANCE_BATCH();
      return;
   }

   /* Now emit VB and VEP state packets.
    *
    * This still defines a hardware VB for each input, even if they
    * are interleaved or from the same VBO.  TBD if this makes a
    * performance difference.
    */
   BEGIN_BATCH(1 + brw->vb.nr_enabled * 4, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_BUFFER << 16) |
	     ((1 + brw->vb.nr_enabled * 4) - 2));

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];

      OUT_BATCH((i << BRW_VB0_INDEX_SHIFT) |
		BRW_VB0_ACCESS_VERTEXDATA |
		(input->stride << BRW_VB0_PITCH_SHIFT));
      OUT_RELOC(input->bo,
		I915_GEM_DOMAIN_VERTEX, 0,
		input->offset);
      if (BRW_IS_IGDNG(brw)) {
          if (input->stride) {
              OUT_RELOC(input->bo,
                        I915_GEM_DOMAIN_VERTEX, 0,
                        input->offset + input->stride * input->count - 1);
          } else {
              assert(input->count == 1);
              OUT_RELOC(input->bo,
                        I915_GEM_DOMAIN_VERTEX, 0,
                        input->offset + input->element_size - 1);
          }
      } else
          OUT_BATCH(input->stride ? input->count : 0);
      OUT_BATCH(0); /* Instance data step rate */
   }
   ADVANCE_BATCH();

   BEGIN_BATCH(1 + brw->vb.nr_enabled * 2, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | ((1 + brw->vb.nr_enabled * 2) - 2));
   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = get_surface_type(input->glarray->Type,
					 input->glarray->Size,
					 input->glarray->Format,
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

      if (BRW_IS_IGDNG(brw))
          OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
                    (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
                    (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
                    (comp3 << BRW_VE1_COMPONENT_3_SHIFT));
      else
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
   GLuint ib_type_size;

   if (index_buffer == NULL)
      return;

   ib_type_size = get_size(index_buffer->type);
   ib_size = ib_type_size * index_buffer->count;
   bufferobj = index_buffer->obj;;

   /* Turn into a proper VBO:
    */
   if (!_mesa_is_bufferobj(bufferobj)) {
      brw->ib.start_vertex_offset = 0;

      /* Get new bufferobj, offset:
       */
      get_space(brw, ib_size, &bo, &offset);

      /* Straight upload
       */
      brw_bo_subdata(bo, offset, ib_size, index_buffer->ptr);

   } else {
      offset = (GLuint) (unsigned long) index_buffer->ptr;
      brw->ib.start_vertex_offset = 0;

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

	  /* Use CMD_3D_PRIM's start_vertex_offset to avoid re-uploading
	   * the index buffer state when we're just moving the start index
	   * of our drawing.
	   */
	  brw->ib.start_vertex_offset = offset / ib_type_size;
	  offset = 0;
	  ib_size = bo->size;
       }
   }

   if (brw->ib.bo != bo ||
       brw->ib.offset != offset ||
       brw->ib.size != ib_size)
   {
      drm_intel_bo_unreference(brw->ib.bo);
      brw->ib.bo = bo;
      brw->ib.offset = offset;
      brw->ib.size = ib_size;

      brw->state.dirty.brw |= BRW_NEW_INDEX_BUFFER;
   } else {
      drm_intel_bo_unreference(bo);
   }

   brw_add_validated_bo(brw, brw->ib.bo);
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_INDICES,
      .cache = 0,
   },
   .prepare = brw_prepare_indices,
};

static void brw_emit_index_buffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;

   if (index_buffer == NULL)
      return;

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
		brw->ib.offset + brw->ib.size - 1);
      OUT_BATCH( 0 );
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_INDEX_BUFFER,
      .cache = 0,
   },
   .emit = brw_emit_index_buffer,
};
