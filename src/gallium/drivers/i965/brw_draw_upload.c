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
#include "util/u_inlines.h"

#include "util/u_upload_mgr.h"
#include "util/u_math.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_screen.h"
#include "brw_batchbuffer.h"
#include "brw_debug.h"




static unsigned brw_translate_surface_format( unsigned id )
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


static int brw_prepare_vertices(struct brw_context *brw)
{
   unsigned int min_index = brw->curr.min_index;
   unsigned int max_index = brw->curr.max_index;
   GLuint i;
   int ret;

   if (BRW_DEBUG & DEBUG_VERTS)
      debug_printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);


   for (i = 0; i < brw->curr.num_vertex_buffers; i++) {
      struct pipe_vertex_buffer *vb = &brw->curr.vertex_buffer[i];
      struct brw_winsys_buffer *bo;
      struct pipe_buffer *upload_buf = NULL;
      unsigned offset;
      
      if (BRW_DEBUG & DEBUG_VERTS)
	 debug_printf("%s vb[%d] user:%d offset:0x%x sz:0x%x stride:0x%x\n",
		      __FUNCTION__, i,
		      brw_buffer_is_user_buffer(vb->buffer),
		      vb->buffer_offset,
		      vb->buffer->size,
		      vb->stride);

      if (brw_buffer_is_user_buffer(vb->buffer)) {

	 /* XXX: simplify this.  Stop the state trackers from generating
	  * zero-stride buffers & have them use additional constants (or
	  * add support for >1 constant buffer) instead.
	  */
	 unsigned size = (vb->stride == 0 ? 
			  vb->buffer->size - vb->buffer_offset :
			  MAX2(vb->buffer->size - vb->buffer_offset,
			       vb->stride * (max_index + 1 - min_index)));

	 ret = u_upload_buffer( brw->vb.upload_vertex, 
				vb->buffer_offset + min_index * vb->stride,
				size,
				vb->buffer,
				&offset,
				&upload_buf );
	 if (ret)
	    return ret;

	 bo = brw_buffer(upload_buf)->bo;
	 
	 assert(offset + size <= bo->size);
      }
      else
      {
	 offset = vb->buffer_offset;
	 bo = brw_buffer(vb->buffer)->bo;
      }

      assert(offset < bo->size);
      
      /* Set up post-upload info about this vertex buffer:
       */
      brw->vb.vb[i].offset = offset;
      brw->vb.vb[i].stride = vb->stride;
      brw->vb.vb[i].vertex_count = (vb->stride == 0 ?
				    1 :
				    (bo->size - offset) / vb->stride);

      bo_reference( &brw->vb.vb[i].bo,  bo );

      /* Don't need to retain this reference.  We have a reference on
       * the underlying winsys buffer:
       */
      pipe_buffer_reference( &upload_buf, NULL );
   }

   brw->vb.nr_vb = i;
   brw_prepare_query_begin(brw);

   for (i = 0; i < brw->vb.nr_vb; i++) {
      brw_add_validated_bo(brw, brw->vb.vb[i].bo);
   }

   return 0;
}

static int brw_emit_vertex_buffers( struct brw_context *brw )
{
   int i;

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), just bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (brw->vb.nr_vb == 0) {
      if (BRW_DEBUG & DEBUG_VERTS)
	 debug_printf("%s: no active vertex buffers\n", __FUNCTION__);

      return 0;
   }

   /* Emit VB state packets.
    */
   BEGIN_BATCH(1 + brw->vb.nr_vb * 4, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_BUFFER << 16) |
	     ((1 + brw->vb.nr_vb * 4) - 2));

   for (i = 0; i < brw->vb.nr_vb; i++) {
      OUT_BATCH((i << BRW_VB0_INDEX_SHIFT) |
		BRW_VB0_ACCESS_VERTEXDATA |
		(brw->vb.vb[i].stride << BRW_VB0_PITCH_SHIFT));
      OUT_RELOC(brw->vb.vb[i].bo,
		BRW_USAGE_VERTEX,
		brw->vb.vb[i].offset);
      if (BRW_IS_IGDNG(brw)) {
	 OUT_RELOC(brw->vb.vb[i].bo,
		   BRW_USAGE_VERTEX,
		   brw->vb.vb[i].bo->size - 1);
      } else
	 OUT_BATCH(brw->vb.vb[i].stride ? brw->vb.vb[i].vertex_count : 0);
      OUT_BATCH(0); /* Instance data step rate */
   }
   ADVANCE_BATCH();
   return 0;
}




static int brw_emit_vertex_elements(struct brw_context *brw)
{
   GLuint nr = brw->curr.num_vertex_elements;
   GLuint i;

   brw_emit_query_begin(brw);

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (nr == 0) {
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
      return 0;
   }

   /* Now emit vertex element (VEP) state packets.
    *
    */
   BEGIN_BATCH(1 + nr * 2, IGNORE_CLIPRECTS);
   OUT_BATCH((CMD_VERTEX_ELEMENT << 16) | ((1 + nr * 2) - 2));
   for (i = 0; i < nr; i++) {
      const struct pipe_vertex_element *input = &brw->curr.vertex_element[i];
      uint32_t format = brw_translate_surface_format( input->src_format );
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      switch (input->nr_components) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 3: comp3 = BRW_VE1_COMPONENT_STORE_1_FLT;
	 break;
      }

      OUT_BATCH((input->vertex_buffer_index << BRW_VE0_INDEX_SHIFT) |
		BRW_VE0_VALID |
		(format << BRW_VE0_FORMAT_SHIFT) |
		(input->src_offset << BRW_VE0_SRC_OFFSET_SHIFT));

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
   return 0;
}


static int brw_emit_vertices( struct brw_context *brw )
{
   int ret;

   ret = brw_emit_vertex_buffers( brw );
   if (ret)
      return ret;

   ret = brw_emit_vertex_elements( brw );
   if (ret)
      return ret;
   
   return 0;
}


const struct brw_tracked_state brw_vertices = {
   .dirty = {
      .mesa = (PIPE_NEW_INDEX_RANGE |
               PIPE_NEW_VERTEX_BUFFER),
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = brw_prepare_vertices,
   .emit = brw_emit_vertices,
};


static int brw_prepare_indices(struct brw_context *brw)
{
   struct pipe_buffer *index_buffer = brw->curr.index_buffer;
   struct pipe_buffer *upload_buf = NULL;
   struct brw_winsys_buffer *bo = NULL;
   GLuint offset;
   GLuint index_size;
   GLuint ib_size;
   int ret;

   if (index_buffer == NULL)
      return 0;

   if (BRW_DEBUG & DEBUG_VERTS)
      debug_printf("%s: index_size:%d index_buffer->size:%d\n",
		   __FUNCTION__,
		   brw->curr.index_size,
		   brw->curr.index_buffer->size);

   ib_size = index_buffer->size;
   index_size = brw->curr.index_size;

   /* Turn userbuffer into a proper hardware buffer?
    */
   if (brw_buffer_is_user_buffer(index_buffer)) {

      ret = u_upload_buffer( brw->vb.upload_index,
			     0,
			     ib_size,
			     index_buffer,
			     &offset,
			     &upload_buf );
      if (ret)
	 return ret;

      bo = brw_buffer(upload_buf)->bo;

      /* XXX: annotate the userbuffer with the upload information so
       * that successive calls don't get re-uploaded.
       */
   }
   else {
      bo = brw_buffer(index_buffer)->bo;
      ib_size = bo->size;
      offset = 0;
   }

   /* Use CMD_3D_PRIM's start_vertex_offset to avoid re-uploading the
    * index buffer state when we're just moving the start index of our
    * drawing.
    *
    * In gallium this will happen in the case where successive draw
    * calls are made with (distinct?) userbuffers, but the upload_mgr
    * places the data into a single winsys buffer.
    * 
    * This statechange doesn't raise any state flags and is always
    * just merged into the final draw packet:
    */
   if (1) {
      assert((offset & (index_size - 1)) == 0);
      brw->ib.start_vertex_offset = offset / index_size;
   }

   /* These statechanges trigger a new CMD_INDEX_BUFFER packet:
    */
   if (brw->ib.bo != bo ||
       brw->ib.size != ib_size)
   {
      bo_reference(&brw->ib.bo, bo);
      brw->ib.size = ib_size;
      brw->state.dirty.brw |= BRW_NEW_INDEX_BUFFER;
   }

   pipe_buffer_reference( &upload_buf, NULL );
   brw_add_validated_bo(brw, brw->ib.bo);
   return 0;
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = PIPE_NEW_INDEX_BUFFER,
      .brw = 0,
      .cache = 0,
   },
   .prepare = brw_prepare_indices,
};

static int brw_emit_index_buffer(struct brw_context *brw)
{
   /* Emit the indexbuffer packet:
    */
   if (brw->ib.bo)
   {
      struct brw_indexbuffer ib;

      memset(&ib, 0, sizeof(ib));

      ib.header.bits.opcode = CMD_INDEX_BUFFER;
      ib.header.bits.length = sizeof(ib)/4 - 2;
      ib.header.bits.index_format = get_index_type(brw->ib.size);
      ib.header.bits.cut_index_enable = 0;

      BEGIN_BATCH(4, IGNORE_CLIPRECTS);
      OUT_BATCH( ib.header.dword );
      OUT_RELOC(brw->ib.bo,
		BRW_USAGE_VERTEX,
		brw->ib.offset);
      OUT_RELOC(brw->ib.bo,
		BRW_USAGE_VERTEX,
		brw->ib.offset + brw->ib.size - 1);
      OUT_BATCH( 0 );
      ADVANCE_BATCH();
   }

   return 0;
}

const struct brw_tracked_state brw_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_INDEX_BUFFER,
      .cache = 0,
   },
   .emit = brw_emit_index_buffer,
};
