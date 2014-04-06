/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/glformats.h"

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

static GLuint fixed_point_types[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SFIXED,
   BRW_SURFACEFORMAT_R32G32_SFIXED,
   BRW_SURFACEFORMAT_R32G32B32_SFIXED,
   BRW_SURFACEFORMAT_R32G32B32A32_SFIXED,
};

static GLuint uint_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R32_UINT,
   BRW_SURFACEFORMAT_R32G32_UINT,
   BRW_SURFACEFORMAT_R32G32B32_UINT,
   BRW_SURFACEFORMAT_R32G32B32A32_UINT
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

static GLuint int_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SINT,
   BRW_SURFACEFORMAT_R32G32_SINT,
   BRW_SURFACEFORMAT_R32G32B32_SINT,
   BRW_SURFACEFORMAT_R32G32B32A32_SINT
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

static GLuint ushort_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R16_UINT,
   BRW_SURFACEFORMAT_R16G16_UINT,
   BRW_SURFACEFORMAT_R16G16B16A16_UINT,
   BRW_SURFACEFORMAT_R16G16B16A16_UINT
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

static GLuint short_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SINT,
   BRW_SURFACEFORMAT_R16G16_SINT,
   BRW_SURFACEFORMAT_R16G16B16A16_SINT,
   BRW_SURFACEFORMAT_R16G16B16A16_SINT
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

static GLuint ubyte_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R8_UINT,
   BRW_SURFACEFORMAT_R8G8_UINT,
   BRW_SURFACEFORMAT_R8G8B8A8_UINT,
   BRW_SURFACEFORMAT_R8G8B8A8_UINT
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

static GLuint byte_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SINT,
   BRW_SURFACEFORMAT_R8G8_SINT,
   BRW_SURFACEFORMAT_R8G8B8A8_SINT,
   BRW_SURFACEFORMAT_R8G8B8A8_SINT
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
unsigned
brw_get_vertex_surface_type(struct brw_context *brw,
                            const struct gl_client_array *glarray)
{
   int size = glarray->Size;

   if (unlikely(INTEL_DEBUG & DEBUG_VERTS))
      fprintf(stderr, "type %s size %d normalized %d\n",
              _mesa_lookup_enum_by_nr(glarray->Type),
              glarray->Size, glarray->Normalized);

   if (glarray->Integer) {
      assert(glarray->Format == GL_RGBA); /* sanity check */
      switch (glarray->Type) {
      case GL_INT: return int_types_direct[size];
      case GL_SHORT: return short_types_direct[size];
      case GL_BYTE: return byte_types_direct[size];
      case GL_UNSIGNED_INT: return uint_types_direct[size];
      case GL_UNSIGNED_SHORT: return ushort_types_direct[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_direct[size];
      default: assert(0); return 0;
      }
   } else if (glarray->Type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
      return BRW_SURFACEFORMAT_R11G11B10_FLOAT;
   } else if (glarray->Normalized) {
      switch (glarray->Type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT: return half_float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE:
         if (glarray->Format == GL_BGRA) {
            /* See GL_EXT_vertex_array_bgra */
            assert(size == 4);
            return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
         }
         else {
            return ubyte_types_norm[size];
         }
      case GL_FIXED:
         if (brw->gen >= 8 || brw->is_haswell)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: Pre-Haswell, the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      case GL_INT_2_10_10_10_REV:
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_SNORM
               : BRW_SURFACEFORMAT_R10G10B10A2_SNORM;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_UNORM
               : BRW_SURFACEFORMAT_R10G10B10A2_UNORM;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      default: assert(0); return 0;
      }
   }
   else {
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      if (glarray->Type == GL_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_SSCALED
               : BRW_SURFACEFORMAT_R10G10B10A2_SSCALED;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      } else if (glarray->Type == GL_UNSIGNED_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_USCALED
               : BRW_SURFACEFORMAT_R10G10B10A2_USCALED;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      }
      assert(glarray->Format == GL_RGBA); /* sanity check */
      switch (glarray->Type) {
      case GL_DOUBLE: return double_types[size];
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT: return half_float_types[size];
      case GL_INT: return int_types_scale[size];
      case GL_SHORT: return short_types_scale[size];
      case GL_BYTE: return byte_types_scale[size];
      case GL_UNSIGNED_INT: return uint_types_scale[size];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size];
      case GL_FIXED:
         if (brw->gen >= 8 || brw->is_haswell)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      default: assert(0); return 0;
      }
   }
}

unsigned
brw_get_index_type(GLenum type)
{
   switch (type) {
   case GL_UNSIGNED_BYTE:  return BRW_INDEX_BYTE;
   case GL_UNSIGNED_SHORT: return BRW_INDEX_WORD;
   case GL_UNSIGNED_INT:   return BRW_INDEX_DWORD;
   default: assert(0); return 0;
   }
}

static void
copy_array_to_vbo_array(struct brw_context *brw,
			struct brw_vertex_element *element,
			int min, int max,
			struct brw_vertex_buffer *buffer,
			GLuint dst_stride)
{
   const int src_stride = element->glarray->StrideB;

   /* If the source stride is zero, we just want to upload the current
    * attribute once and set the buffer's stride to 0.  There's no need
    * to replicate it out.
    */
   if (src_stride == 0) {
      intel_upload_data(brw, element->glarray->Ptr,
                        element->glarray->_ElementSize,
                        element->glarray->_ElementSize,
			&buffer->bo, &buffer->offset);

      buffer->stride = 0;
      return;
   }

   const unsigned char *src = element->glarray->Ptr + min * src_stride;
   int count = max - min + 1;
   GLuint size = count * dst_stride;
   uint8_t *dst = intel_upload_space(brw, size, dst_stride,
                                     &buffer->bo, &buffer->offset);

   if (dst_stride == src_stride) {
      memcpy(dst, src, size);
   } else {
      while (count--) {
	 memcpy(dst, src, dst_stride);
	 src += src_stride;
	 dst += dst_stride;
      }
   }
   buffer->stride = dst_stride;
}

void
brw_prepare_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* CACHE_NEW_VS_PROG */
   GLbitfield64 vs_inputs = brw->vs.prog_data->inputs_read;
   const unsigned char *ptr = NULL;
   GLuint interleaved = 0;
   unsigned int min_index = brw->vb.min_index + brw->basevertex;
   unsigned int max_index = brw->vb.max_index + brw->basevertex;
   int delta, i, j;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;

   /* _NEW_POLYGON
    *
    * On gen6+, edge flags don't end up in the VUE (either in or out of the
    * VS).  Instead, they're uploaded as the last vertex element, and the data
    * is passed sideband through the fixed function units.  So, we need to
    * prepare the vertex buffer for it, but it's not present in inputs_read.
    */
   if (brw->gen >= 6 && (ctx->Polygon.FrontMode != GL_FILL ||
                           ctx->Polygon.BackMode != GL_FILL)) {
      vs_inputs |= VERT_BIT_EDGEFLAG;
   }

   if (0)
      fprintf(stderr, "%s %d..%d\n", __FUNCTION__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   brw->vb.nr_enabled = 0;
   while (vs_inputs) {
      GLuint i = ffsll(vs_inputs) - 1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      vs_inputs &= ~BITFIELD64_BIT(i);
      brw->vb.enabled[brw->vb.nr_enabled++] = input;
   }

   if (brw->vb.nr_enabled == 0)
      return;

   if (brw->vb.nr_buffers)
      return;

   for (i = j = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct gl_client_array *glarray = input->glarray;

      if (_mesa_is_bufferobj(glarray->BufferObj)) {
	 struct intel_buffer_object *intel_buffer =
	    intel_buffer_object(glarray->BufferObj);
	 int k;

	 /* If we have a VB set to be uploaded for this buffer object
	  * already, reuse that VB state so that we emit fewer
	  * relocations.
	  */
	 for (k = 0; k < i; k++) {
	    const struct gl_client_array *other = brw->vb.enabled[k]->glarray;
	    if (glarray->BufferObj == other->BufferObj &&
		glarray->StrideB == other->StrideB &&
		glarray->InstanceDivisor == other->InstanceDivisor &&
		(uintptr_t)(glarray->Ptr - other->Ptr) < glarray->StrideB)
	    {
	       input->buffer = brw->vb.enabled[k]->buffer;
	       input->offset = glarray->Ptr - other->Ptr;
	       break;
	    }
	 }
	 if (k == i) {
	    struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];

	    /* Named buffer object: Just reference its contents directly. */
	    buffer->offset = (uintptr_t)glarray->Ptr;
	    buffer->stride = glarray->StrideB;
	    buffer->step_rate = glarray->InstanceDivisor;

            uint32_t offset, size;
            if (glarray->InstanceDivisor) {
               offset = buffer->offset;
               size = (buffer->stride * ((brw->num_instances /
                                          glarray->InstanceDivisor) - 1) +
                       glarray->_ElementSize);
            } else {
               if (min_index == -1) {
                  offset = 0;
                  size = intel_buffer->Base.Size;
               } else {
                  offset = buffer->offset + min_index * buffer->stride;
                  size = (buffer->stride * (max_index - min_index) +
                          glarray->_ElementSize);
               }
            }
            buffer->bo = intel_bufferobj_buffer(brw, intel_buffer,
                                                offset, size);
            drm_intel_bo_reference(buffer->bo);

	    input->buffer = j++;
	    input->offset = 0;
	 }

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
	 assert(input->offset < brw->vb.buffers[input->buffer].bo->size);
      } else {
	 /* Queue the buffer object up to be uploaded in the next pass,
	  * when we've decided if we're doing interleaved or not.
	  */
	 if (nr_uploads == 0) {
	    interleaved = glarray->StrideB;
	    ptr = glarray->Ptr;
	 }
	 else if (interleaved != glarray->StrideB ||
                  glarray->Ptr < ptr ||
                  (uintptr_t)(glarray->Ptr - ptr) + glarray->_ElementSize > interleaved)
	 {
            /* If our stride is different from the first attribute's stride,
             * or if the first attribute's stride didn't cover our element,
             * disable the interleaved upload optimization.  The second case
             * can most commonly occur in cases where there is a single vertex
             * and, for example, the data is stored on the application's
             * stack.
             *
             * NOTE: This will also disable the optimization in cases where
             * the data is in a different order than the array indices.
             * Something like:
             *
             *     float data[...];
             *     glVertexAttribPointer(0, 4, GL_FLOAT, 32, &data[4]);
             *     glVertexAttribPointer(1, 4, GL_FLOAT, 32, &data[0]);
             */
	    interleaved = 0;
	 }

	 upload[nr_uploads++] = input;
      }
   }

   /* If we need to upload all the arrays, then we can trim those arrays to
    * only the used elements [min_index, max_index] so long as we adjust all
    * the values used in the 3DPRIMITIVE i.e. by setting the vertex bias.
    */
   brw->vb.start_vertex_bias = 0;
   delta = min_index;
   if (nr_uploads == brw->vb.nr_enabled) {
      brw->vb.start_vertex_bias = -delta;
      delta = 0;
   }

   /* Handle any arrays to be uploaded. */
   if (nr_uploads > 1) {
      if (interleaved) {
	 struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
	 /* All uploads are interleaved, so upload the arrays together as
	  * interleaved.  First, upload the contents and set up upload[0].
	  */
	 copy_array_to_vbo_array(brw, upload[0], min_index, max_index,
				 buffer, interleaved);
	 buffer->offset -= delta * interleaved;

	 for (i = 0; i < nr_uploads; i++) {
	    /* Then, just point upload[i] at upload[0]'s buffer. */
	    upload[i]->offset =
	       ((const unsigned char *)upload[i]->glarray->Ptr - ptr);
	    upload[i]->buffer = j;
	 }
	 j++;

	 nr_uploads = 0;
      }
   }
   /* Upload non-interleaved arrays */
   for (i = 0; i < nr_uploads; i++) {
      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
      if (upload[i]->glarray->InstanceDivisor == 0) {
         copy_array_to_vbo_array(brw, upload[i], min_index, max_index,
                                 buffer, upload[i]->glarray->_ElementSize);
      } else {
         /* This is an instanced attribute, since its InstanceDivisor
          * is not zero. Therefore, its data will be stepped after the
          * instanced draw has been run InstanceDivisor times.
          */
         uint32_t instanced_attr_max_index =
            (brw->num_instances - 1) / upload[i]->glarray->InstanceDivisor;
         copy_array_to_vbo_array(brw, upload[i], 0, instanced_attr_max_index,
                                 buffer, upload[i]->glarray->_ElementSize);
      }
      buffer->offset -= delta * buffer->stride;
      buffer->step_rate = upload[i]->glarray->InstanceDivisor;
      upload[i]->buffer = j++;
      upload[i]->offset = 0;
   }

   brw->vb.nr_buffers = j;
}

static void brw_emit_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   GLuint i, nr_elements;

   brw_prepare_vertices(brw);

   brw_emit_query_begin(brw);

   nr_elements = brw->vb.nr_enabled + brw->vs.prog_data->uses_vertexid;

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (nr_elements == 0) {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | 1);
      if (brw->gen >= 6) {
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
      ADVANCE_BATCH();
      return;
   }

   /* Now emit VB and VEP state packets.
    */

   if (brw->vb.nr_buffers) {
      if (brw->gen >= 6) {
	 assert(brw->vb.nr_buffers <= 33);
      } else {
	 assert(brw->vb.nr_buffers <= 17);
      }

      BEGIN_BATCH(1 + 4*brw->vb.nr_buffers);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (4*brw->vb.nr_buffers - 1));
      for (i = 0; i < brw->vb.nr_buffers; i++) {
	 struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
	 uint32_t dw0;

	 if (brw->gen >= 6) {
	    dw0 = buffer->step_rate
	             ? GEN6_VB0_ACCESS_INSTANCEDATA
	             : GEN6_VB0_ACCESS_VERTEXDATA;
	    dw0 |= i << GEN6_VB0_INDEX_SHIFT;
	 } else {
	    dw0 = buffer->step_rate
	             ? BRW_VB0_ACCESS_INSTANCEDATA
	             : BRW_VB0_ACCESS_VERTEXDATA;
	    dw0 |= i << BRW_VB0_INDEX_SHIFT;
	 }

	 if (brw->gen >= 7)
	    dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;

         if (brw->gen == 7)
	    dw0 |= GEN7_MOCS_L3 << 16;

         WARN_ONCE(buffer->stride >= (brw->gen >= 5 ? 2048 : 2047),
                   "VBO stride %d too large, bad rendering may occur\n",
                   buffer->stride);
	 OUT_BATCH(dw0 | (buffer->stride << BRW_VB0_PITCH_SHIFT));
	 OUT_RELOC(buffer->bo, I915_GEM_DOMAIN_VERTEX, 0, buffer->offset);
	 if (brw->gen >= 5) {
	    OUT_RELOC(buffer->bo, I915_GEM_DOMAIN_VERTEX, 0, buffer->bo->size - 1);
	 } else
	    OUT_BATCH(0);
	 OUT_BATCH(buffer->step_rate);
      }
      ADVANCE_BATCH();
   }

   /* The hardware allows one more VERTEX_ELEMENTS than VERTEX_BUFFERS, presumably
    * for VertexID/InstanceID.
    */
   if (brw->gen >= 6) {
      assert(nr_elements <= 34);
   } else {
      assert(nr_elements <= 18);
   }

   struct brw_vertex_element *gen6_edgeflag_input = NULL;

   BEGIN_BATCH(1 + nr_elements * 2);
   OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (2 * nr_elements - 1));
   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glarray);
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      if (input == &brw->vb.inputs[VERT_ATTRIB_EDGEFLAG]) {
         /* Gen6+ passes edgeflag as sideband along with the vertex, instead
          * of in the VUE.  We have to upload it sideband as the last vertex
          * element according to the B-Spec.
          */
         if (brw->gen >= 6) {
            gen6_edgeflag_input = input;
            continue;
         }
      }

      switch (input->glarray->Size) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0;
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0;
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0;
      case 3: comp3 = input->glarray->Integer ? BRW_VE1_COMPONENT_STORE_1_INT
                                              : BRW_VE1_COMPONENT_STORE_1_FLT;
	 break;
      }

      if (brw->gen >= 6) {
	 OUT_BATCH((input->buffer << GEN6_VE0_INDEX_SHIFT) |
		   GEN6_VE0_VALID |
		   (format << BRW_VE0_FORMAT_SHIFT) |
		   (input->offset << BRW_VE0_SRC_OFFSET_SHIFT));
      } else {
	 OUT_BATCH((input->buffer << BRW_VE0_INDEX_SHIFT) |
		   BRW_VE0_VALID |
		   (format << BRW_VE0_FORMAT_SHIFT) |
		   (input->offset << BRW_VE0_SRC_OFFSET_SHIFT));
      }

      if (brw->gen >= 5)
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

   if (brw->gen >= 6 && gen6_edgeflag_input) {
      uint32_t format =
         brw_get_vertex_surface_type(brw, gen6_edgeflag_input->glarray);

      OUT_BATCH((gen6_edgeflag_input->buffer << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                GEN6_VE0_EDGE_FLAG_ENABLE |
                (format << BRW_VE0_FORMAT_SHIFT) |
                (gen6_edgeflag_input->offset << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
   }

   if (brw->vs.prog_data->uses_vertexid) {
      uint32_t dw0 = 0, dw1 = 0;

      dw1 = ((BRW_VE1_COMPONENT_STORE_VID << BRW_VE1_COMPONENT_0_SHIFT) |
	     (BRW_VE1_COMPONENT_STORE_IID << BRW_VE1_COMPONENT_1_SHIFT) |
	     (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
	     (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));

      if (brw->gen >= 6) {
	 dw0 |= GEN6_VE0_VALID;
      } else {
	 dw0 |= BRW_VE0_VALID;
	 dw1 |= (i * 4) << BRW_VE1_DST_OFFSET_SHIFT;
      }

      /* Note that for gl_VertexID, gl_InstanceID, and gl_PrimitiveID values,
       * the format is ignored and the value is always int.
       */

      OUT_BATCH(dw0);
      OUT_BATCH(dw1);
   }

   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_vertices = {
   .dirty = {
      .mesa = _NEW_POLYGON,
      .brw = BRW_NEW_BATCH | BRW_NEW_VERTICES,
      .cache = CACHE_NEW_VS_PROG,
   },
   .emit = brw_emit_vertices,
};

static void brw_upload_indices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;
   drm_intel_bo *old_bo = brw->ib.bo;
   struct gl_buffer_object *bufferobj;
   GLuint offset;
   GLuint ib_type_size;

   if (index_buffer == NULL)
      return;

   ib_type_size = _mesa_sizeof_type(index_buffer->type);
   ib_size = ib_type_size * index_buffer->count;
   bufferobj = index_buffer->obj;

   /* Turn into a proper VBO:
    */
   if (!_mesa_is_bufferobj(bufferobj)) {
      /* Get new bufferobj, offset:
       */
      intel_upload_data(brw, index_buffer->ptr, ib_size, ib_type_size,
			&brw->ib.bo, &offset);
   } else {
      offset = (GLuint) (unsigned long) index_buffer->ptr;

      /* If the index buffer isn't aligned to its element size, we have to
       * rebase it into a temporary.
       */
      if ((ib_type_size - 1) & offset) {
         perf_debug("copying index buffer to a temporary to work around "
                    "misaligned offset %d\n", offset);

         GLubyte *map = ctx->Driver.MapBufferRange(ctx,
                                                   offset,
                                                   ib_size,
                                                   GL_MAP_READ_BIT,
                                                   bufferobj,
                                                   MAP_INTERNAL);

         intel_upload_data(brw, map, ib_size, ib_type_size,
                           &brw->ib.bo, &offset);

         ctx->Driver.UnmapBuffer(ctx, bufferobj, MAP_INTERNAL);
      } else {
         drm_intel_bo *bo =
            intel_bufferobj_buffer(brw, intel_buffer_object(bufferobj),
                                   offset, ib_size);
         if (bo != brw->ib.bo) {
            drm_intel_bo_unreference(brw->ib.bo);
            brw->ib.bo = bo;
            drm_intel_bo_reference(bo);
         }
      }
   }

   /* Use 3DPRIMITIVE's start_vertex_offset to avoid re-uploading
    * the index buffer state when we're just moving the start index
    * of our drawing.
    */
   brw->ib.start_vertex_offset = offset / ib_type_size;

   if (brw->ib.bo != old_bo)
      brw->state.dirty.brw |= BRW_NEW_INDEX_BUFFER;

   if (index_buffer->type != brw->ib.type) {
      brw->ib.type = index_buffer->type;
      brw->state.dirty.brw |= BRW_NEW_INDEX_BUFFER;
   }
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_INDICES,
      .cache = 0,
   },
   .emit = brw_upload_indices,
};

static void brw_emit_index_buffer(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint cut_index_setting;

   if (index_buffer == NULL)
      return;

   if (brw->prim_restart.enable_cut_index && !brw->is_haswell) {
      cut_index_setting = BRW_CUT_INDEX_ENABLE;
   } else {
      cut_index_setting = 0;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(CMD_INDEX_BUFFER << 16 |
             cut_index_setting |
             brw_get_index_type(index_buffer->type) << 8 |
             1);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
             0);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
	     brw->ib.bo->size - 1);
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
