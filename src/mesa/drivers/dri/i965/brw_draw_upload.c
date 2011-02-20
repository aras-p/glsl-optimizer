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

#undef NDEBUG

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

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

static GLuint half_float_types[5] = {
   0,
   BRW_SURFACEFORMAT_R16_FLOAT,
   BRW_SURFACEFORMAT_R16G16_FLOAT,
   BRW_SURFACEFORMAT_R16G16B16A16_FLOAT,
   BRW_SURFACEFORMAT_R16G16B16A16_FLOAT
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


/**
 * Given vertex array type/size/format/normalized info, return
 * the appopriate hardware surface type.
 * Format will be GL_RGBA or possibly GL_BGRA for GLubyte[4] color arrays.
 */
static GLuint get_surface_type( GLenum type, GLuint size,
                                GLenum format, GLboolean normalized )
{
   if (unlikely(INTEL_DEBUG & DEBUG_VERTS))
      printf("type %s size %d normalized %d\n", 
		   _mesa_lookup_enum_by_nr(type), size, normalized);

   if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT: return half_float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE:
         if (format == GL_BGRA) {
            /* See GL_EXT_vertex_array_bgra */
            assert(size == 4);
            return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
         }
         else {
            return ubyte_types_norm[size];
         }
      default: assert(0); return 0;
      }      
   }
   else {
      assert(format == GL_RGBA); /* sanity check */
      switch (type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT: return half_float_types[size];
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
   case GL_HALF_FLOAT: return sizeof(GLhalfARB);
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

static void
copy_array_to_vbo_array( struct brw_context *brw,
			 struct brw_vertex_element *element,
			 GLuint dst_stride)
{
   GLuint size = element->count * dst_stride;

   if (element->glarray->StrideB == 0) {
      assert(element->count == 1);
      element->stride = 0;
   } else {
      element->stride = dst_stride;
   }

   if (dst_stride == element->glarray->StrideB) {
      intel_upload_data(&brw->intel, element->glarray->Ptr, size,
			&element->bo, &element->offset);
   } else {
      const unsigned char *src = element->glarray->Ptr;
      char *dst = intel_upload_map(&brw->intel, size,
				   &element->bo, &element->offset);
      int i;

      for (i = 0; i < element->count; i++) {
	 memcpy(dst, src, dst_stride);
	 src += element->glarray->StrideB;
	 dst += dst_stride;
      }
   }
}

static void brw_prepare_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
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
      printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   brw->vb.nr_enabled = 0;
   while (vs_inputs) {
      GLuint i = _mesa_ffsll(vs_inputs) - 1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      vs_inputs &= ~(1 << i);
      brw->vb.enabled[brw->vb.nr_enabled++] = input;
   }

   if (brw->vb.nr_enabled == 0)
      return;

   /* XXX: In the rare cases where this happens we fallback all
    * the way to software rasterization, although a tnl fallback
    * would be sufficient.  I don't know of *any* real world
    * cases with > 17 vertex attributes enabled, so it probably
    * isn't an issue at this point.
    */
   if (brw->vb.nr_enabled >= BRW_VEP_MAX) {
      intel->Fallback = GL_TRUE; /* boolean, not bitfield */
      return;
   }

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];

      input->element_size = get_size(input->glarray->Type) * input->glarray->Size;

      if (_mesa_is_bufferobj(input->glarray->BufferObj)) {
	 struct intel_buffer_object *intel_buffer =
	    intel_buffer_object(input->glarray->BufferObj);
	 GLuint offset;

	 /* Named buffer object: Just reference its contents directly. */
	 drm_intel_bo_unreference(input->bo);
	 input->bo = intel_bufferobj_source(intel, intel_buffer, &offset);
	 drm_intel_bo_reference(input->bo);
	 input->offset = offset + (unsigned long)input->glarray->Ptr;
	 input->stride = input->glarray->StrideB;
	 input->count = input->glarray->_MaxElement;

	 /* This is a common place to reach if the user mistakenly supplies
	  * a pointer in place of a VBO offset.  If we just let it go through,
	  * we may end up dereferencing a pointer beyond the bounds of the
	  * GTT.  We would hope that the VBO's max_index would save us, but
	  * Mesa appears to hand us min/max values not clipped to the
	  * array object's _MaxElement, and _MaxElement frequently appears
	  * to be wrong anyway.
	  *
	  * The VBO spec allows application termination in this case, and it's
	  * probably a service to the poor programmer to do so rather than
	  * trying to just not render.
	  */
	 assert(input->offset < input->bo->size);
      } else {
	 input->count = input->glarray->StrideB ? max_index + 1 : 1;
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
	 if (input->attrib == VERT_ATTRIB_POS) {
	    /* Position array not properly enabled:
	     */
            if (input->glarray->StrideB == 0) {
               intel->Fallback = GL_TRUE; /* boolean, not bitfield */
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
	 drm_intel_bo_reference(upload[i]->bo);
      }
   }
   else {
      /* Upload non-interleaved arrays */
      for (i = 0; i < nr_uploads; i++) {
          copy_array_to_vbo_array(brw, upload[i], upload[i]->element_size);
      }
   }

   brw_prepare_query_begin(brw);

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];

      brw_add_validated_bo(brw, input->bo);
   }
}

static void brw_emit_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
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
      BEGIN_BATCH(3);
      OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | 1);
      if (intel->gen >= 6) {
	 OUT_BATCH((0 << GEN6_VE0_INDEX_SHIFT) |
		   GEN6_VE0_VALID |
		   (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      } else {
	 OUT_BATCH((0 << BRW_VE0_INDEX_SHIFT) |
		   BRW_VE0_VALID |
		   (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      }
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_1_FLT << BRW_VE1_COMPONENT_3_SHIFT));
      CACHED_BATCH();
      return;
   }

   /* Now emit VB and VEP state packets.
    *
    * This still defines a hardware VB for each input, even if they
    * are interleaved or from the same VBO.  TBD if this makes a
    * performance difference.
    */
   BEGIN_BATCH(1 + brw->vb.nr_enabled * 4);
   OUT_BATCH((CMD_VERTEX_BUFFER << 16) |
	     ((1 + brw->vb.nr_enabled * 4) - 2));

   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t dw0;

      if (intel->gen >= 6) {
	 dw0 = GEN6_VB0_ACCESS_VERTEXDATA |
	    (i << GEN6_VB0_INDEX_SHIFT);
      } else {
	 dw0 = BRW_VB0_ACCESS_VERTEXDATA |
	    (i << BRW_VB0_INDEX_SHIFT);
      }

      OUT_BATCH(dw0 |
		(input->stride << BRW_VB0_PITCH_SHIFT));
      OUT_RELOC(input->bo,
		I915_GEM_DOMAIN_VERTEX, 0,
		input->offset);
      if (intel->gen >= 5) {
	 OUT_RELOC(input->bo,
		   I915_GEM_DOMAIN_VERTEX, 0,
		   input->bo->size - 1);
      } else
          OUT_BATCH(input->stride ? input->count : 0);
      OUT_BATCH(0); /* Instance data step rate */
   }
   ADVANCE_BATCH();

   BEGIN_BATCH(1 + brw->vb.nr_enabled * 2);
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

      if (intel->gen >= 6) {
	 OUT_BATCH((i << GEN6_VE0_INDEX_SHIFT) |
		   GEN6_VE0_VALID |
		   (format << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      } else {
	 OUT_BATCH((i << BRW_VE0_INDEX_SHIFT) |
		   BRW_VE0_VALID |
		   (format << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      }

      if (intel->gen >= 5)
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
   CACHED_BATCH();
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
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;
   drm_intel_bo *bo = NULL;
   struct gl_buffer_object *bufferobj;
   GLuint offset;
   GLuint ib_type_size;

   if (index_buffer == NULL)
      return;

   ib_type_size = get_size(index_buffer->type);
   ib_size = ib_type_size * index_buffer->count;
   bufferobj = index_buffer->obj;

   /* Turn into a proper VBO:
    */
   if (!_mesa_is_bufferobj(bufferobj)) {
      brw->ib.start_vertex_offset = 0;

      /* Get new bufferobj, offset:
       */
      intel_upload_data(&brw->intel, index_buffer->ptr, ib_size, &bo, &offset);
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

	   intel_upload_data(&brw->intel, map, ib_size, &bo, &offset);

           ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, bufferobj);
       } else {
	  /* Use CMD_3D_PRIM's start_vertex_offset to avoid re-uploading
	   * the index buffer state when we're just moving the start index
	   * of our drawing.
	   */
	  brw->ib.start_vertex_offset = offset / ib_type_size;

	  bo = intel_bufferobj_source(intel, intel_buffer_object(bufferobj),
				      &offset);

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

   BEGIN_BATCH(3);
   OUT_BATCH(CMD_INDEX_BUFFER << 16 |
             /* cut index enable << 10 */
             get_index_type(index_buffer->type) << 8 |
             1);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
             brw->ib.offset);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
             brw->ib.offset + brw->ib.size - 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_INDEX_BUFFER,
      .cache = 0,
   },
   .emit = brw_emit_index_buffer,
};
