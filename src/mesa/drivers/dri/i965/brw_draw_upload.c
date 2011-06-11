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
      /* This produces GL_FIXED inputs as values between INT32_MIN and
       * INT32_MAX, which will be scaled down by 1/65536 by the VS.
       */
      case GL_FIXED: return int_types_scale[size];
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
   case GL_FIXED: return sizeof(GLuint);
   default: assert(0); return 0;
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
copy_array_to_vbo_array(struct brw_context *brw,
			struct brw_vertex_element *element,
			int min, int max,
			struct brw_vertex_buffer *buffer,
			GLuint dst_stride)
{
   int src_stride = element->glarray->StrideB;
   const unsigned char *src = element->glarray->Ptr + min * src_stride;
   int count = max - min + 1;
   GLuint size = count * dst_stride;

   if (dst_stride == src_stride) {
      intel_upload_data(&brw->intel, src, size, dst_stride,
			&buffer->bo, &buffer->offset);
   } else {
      char * const map = intel_upload_map(&brw->intel, size, dst_stride);
      char *dst = map;

      while (count--) {
	 memcpy(dst, src, dst_stride);
	 src += src_stride;
	 dst += dst_stride;
      }
      intel_upload_unmap(&brw->intel, map, size, dst_stride,
			 &buffer->bo, &buffer->offset);
   }
   buffer->stride = dst_stride;
}

static void brw_prepare_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   /* CACHE_NEW_VS_PROG */
   GLbitfield vs_inputs = brw->vs.prog_data->inputs_read;
   const unsigned char *ptr = NULL;
   GLuint interleaved = 0, total_size = 0;
   unsigned int min_index = brw->vb.min_index;
   unsigned int max_index = brw->vb.max_index;
   int delta, i, j;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;

   /* First build an array of pointers to ve's in vb.inputs_read
    */
   if (0)
      printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   brw->vb.nr_enabled = 0;
   while (vs_inputs) {
      GLuint i = ffs(vs_inputs) - 1;
      struct brw_vertex_element *input = &brw->vb.inputs[i];

      vs_inputs &= ~(1 << i);
      if (input->glarray->Size && get_size(input->glarray->Type))
         brw->vb.enabled[brw->vb.nr_enabled++] = input;
   }

   if (brw->vb.nr_enabled == 0)
      return;

   if (brw->vb.nr_buffers)
      goto validate;

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

   for (i = j = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct gl_client_array *glarray = input->glarray;
      int type_size = get_size(glarray->Type);

      input->element_size = type_size * glarray->Size;

      if (_mesa_is_bufferobj(glarray->BufferObj)) {
	 struct intel_buffer_object *intel_buffer =
	    intel_buffer_object(glarray->BufferObj);
	 int k;

	 for (k = 0; k < i; k++) {
	    const struct gl_client_array *other = brw->vb.enabled[k]->glarray;
	    if (glarray->BufferObj == other->BufferObj &&
		glarray->StrideB == other->StrideB &&
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
            buffer->bo = intel_bufferobj_source(intel,
                                                intel_buffer, type_size,
						&buffer->offset);
	    drm_intel_bo_reference(buffer->bo);
	    buffer->offset += (uintptr_t)glarray->Ptr;
	    buffer->stride = glarray->StrideB;

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
	    /* Position array not properly enabled:
	     */
	    if (input->attrib == VERT_ATTRIB_POS && glarray->StrideB == 0) {
               intel->Fallback = GL_TRUE; /* boolean, not bitfield */
               return;
            }

	    interleaved = glarray->StrideB;
	    ptr = glarray->Ptr;
	 }
	 else if (interleaved != glarray->StrideB ||
		  (uintptr_t)(glarray->Ptr - ptr) > interleaved)
	 {
	    interleaved = 0;
	 }
	 else if ((uintptr_t)(glarray->Ptr - ptr) & (type_size -1))
	 {
	    /* enforce natural alignment (for doubles) */
	    interleaved = 0;
	 }

	 upload[nr_uploads++] = input;
	 total_size = ALIGN(total_size, type_size);
	 total_size += input->element_size;
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
   if (delta && !brw->intel.intelScreen->relaxed_relocations)
      min_index = delta = 0;

   /* Handle any arrays to be uploaded. */
   if (nr_uploads > 1) {
      if (interleaved && interleaved <= 2*total_size) {
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
      else if (total_size < 2048) {
	 /* Upload non-interleaved arrays into a single interleaved array */
	 struct brw_vertex_buffer *buffer;
	 int count = max_index - min_index + 1;
	 int offset;
	 char *map;

	 map = intel_upload_map(&brw->intel, total_size * count, total_size);
	 for (i = offset = 0; i < nr_uploads; i++) {
	    const unsigned char *src = upload[i]->glarray->Ptr;
	    int size = upload[i]->element_size;
	    int stride = upload[i]->glarray->StrideB;
	    char *dst;
	    int n;

	    offset = ALIGN(offset, get_size(upload[i]->glarray->Type));
	    dst = map + offset;
	    src += min_index * stride;

	    for (n = 0; n < count; n++) {
	       memcpy(dst, src, size);
	       src += stride;
	       dst += total_size;
	    }

	    upload[i]->offset = offset;
	    upload[i]->buffer = j;

	    offset += size;
	 }
	 assert(offset == total_size);
	 buffer = &brw->vb.buffers[j++];
	 intel_upload_unmap(&brw->intel, map, offset * count, offset,
			    &buffer->bo, &buffer->offset);
	 buffer->stride = offset;
	 buffer->offset -= delta * offset;

	 nr_uploads = 0;
      }
   }
   /* Upload non-interleaved arrays */
   for (i = 0; i < nr_uploads; i++) {
      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
      copy_array_to_vbo_array(brw, upload[i], min_index, max_index,
			      buffer, upload[i]->element_size);
      buffer->offset -= delta * buffer->stride;
      upload[i]->buffer = j++;
      upload[i]->offset = 0;
   }

   /* can we simply extend the current vb? */
   if (j == brw->vb.nr_current_buffers) {
      int delta = 0;
      for (i = 0; i < j; i++) {
	 int d;

	 if (brw->vb.current_buffers[i].handle != brw->vb.buffers[i].bo->handle ||
	     brw->vb.current_buffers[i].stride != brw->vb.buffers[i].stride)
	    break;

	 d = brw->vb.buffers[i].offset - brw->vb.current_buffers[i].offset;
	 if (d < 0)
	    break;
	 if (i == 0)
	    delta = d / brw->vb.current_buffers[i].stride;
	 if (delta * brw->vb.current_buffers[i].stride != d)
	    break;
      }

      if (i == j) {
	 brw->vb.start_vertex_bias += delta;
	 while (--j >= 0)
	    drm_intel_bo_unreference(brw->vb.buffers[j].bo);
	 j = 0;
      }
   }

   brw->vb.nr_buffers = j;

validate:
   brw_prepare_query_begin(brw);
   for (i = 0; i < brw->vb.nr_buffers; i++) {
      brw_add_validated_bo(brw, brw->vb.buffers[i].bo);
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
    */

   if (brw->vb.nr_buffers) {
      BEGIN_BATCH(1 + 4*brw->vb.nr_buffers);
      OUT_BATCH((CMD_VERTEX_BUFFER << 16) | (4*brw->vb.nr_buffers - 1));
      for (i = 0; i < brw->vb.nr_buffers; i++) {
	 struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
	 uint32_t dw0;

	 if (intel->gen >= 6) {
	    dw0 = GEN6_VB0_ACCESS_VERTEXDATA | (i << GEN6_VB0_INDEX_SHIFT);
	 } else {
	    dw0 = BRW_VB0_ACCESS_VERTEXDATA | (i << BRW_VB0_INDEX_SHIFT);
	 }

	 if (intel->gen >= 7)
	    dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;

	 OUT_BATCH(dw0 | (buffer->stride << BRW_VB0_PITCH_SHIFT));
	 OUT_RELOC(buffer->bo, I915_GEM_DOMAIN_VERTEX, 0, buffer->offset);
	 if (intel->gen >= 5) {
	    OUT_RELOC(buffer->bo, I915_GEM_DOMAIN_VERTEX, 0, buffer->bo->size - 1);
	 } else
	    OUT_BATCH(0);
	 OUT_BATCH(0); /* Instance data step rate */

	 brw->vb.current_buffers[i].handle = buffer->bo->handle;
	 brw->vb.current_buffers[i].offset = buffer->offset;
	 brw->vb.current_buffers[i].stride = buffer->stride;
      }
      brw->vb.nr_current_buffers = i;
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(1 + brw->vb.nr_enabled * 2);
   OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | (2*brw->vb.nr_enabled - 1));
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
      .cache = CACHE_NEW_VS_PROG,
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

      /* Get new bufferobj, offset:
       */
      intel_upload_data(&brw->intel, index_buffer->ptr, ib_size, ib_type_size,
			&bo, &offset);
      brw->ib.start_vertex_offset = offset / ib_type_size;
   } else {
      offset = (GLuint) (unsigned long) index_buffer->ptr;

      /* If the index buffer isn't aligned to its element size, we have to
       * rebase it into a temporary.
       */
       if ((get_size(index_buffer->type) - 1) & offset) {
           GLubyte *map = ctx->Driver.MapBuffer(ctx,
                                                GL_ELEMENT_ARRAY_BUFFER_ARB,
                                                GL_DYNAMIC_DRAW_ARB,
                                                bufferobj);
           map += offset;

	   intel_upload_data(&brw->intel, map, ib_size, ib_type_size,
			     &bo, &offset);
	   brw->ib.start_vertex_offset = offset / ib_type_size;

           ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, bufferobj);
       } else {
	  /* Use CMD_3D_PRIM's start_vertex_offset to avoid re-uploading
	   * the index buffer state when we're just moving the start index
	   * of our drawing.
	   */
	  brw->ib.start_vertex_offset = offset / ib_type_size;

	  bo = intel_bufferobj_source(intel,
				      intel_buffer_object(bufferobj),
				      ib_type_size,
				      &offset);
	  drm_intel_bo_reference(bo);

	  brw->ib.start_vertex_offset += offset / ib_type_size;
       }
   }

   if (brw->ib.bo != bo) {
      drm_intel_bo_unreference(brw->ib.bo);
      brw->ib.bo = bo;

      brw_add_validated_bo(brw, brw->ib.bo);
      brw->state.dirty.brw |= BRW_NEW_INDEX_BUFFER;
   } else {
      drm_intel_bo_unreference(bo);
   }

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
