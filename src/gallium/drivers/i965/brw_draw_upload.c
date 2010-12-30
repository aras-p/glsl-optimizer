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
#include "brw_resource.h"




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
      struct pipe_resource *upload_buf = NULL;
      unsigned offset;
      
      if (BRW_DEBUG & DEBUG_VERTS)
	 debug_printf("%s vb[%d] user:%d offset:0x%x sz:0x%x stride:0x%x\n",
		      __FUNCTION__, i,
		      brw_buffer_is_user_buffer(vb->buffer),
		      vb->buffer_offset,
		      vb->buffer->width0,
		      vb->stride);

      if (brw_buffer_is_user_buffer(vb->buffer)) {

	 /* XXX: simplify this.  Stop the state trackers from generating
	  * zero-stride buffers & have them use additional constants (or
	  * add support for >1 constant buffer) instead.
	  */
	 unsigned size = (vb->stride == 0 ? 
			  vb->buffer->width0 - vb->buffer_offset :
			  MAX2(vb->buffer->width0 - vb->buffer_offset,
			       vb->stride * (max_index + 1 - min_index)));
	 boolean flushed;

	 ret = u_upload_buffer( brw->vb.upload_vertex,
				0,
				vb->buffer_offset + min_index * vb->stride,
				size,
				vb->buffer,
				&offset,
				&upload_buf,
				&flushed );
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
      pipe_resource_reference( &upload_buf, NULL );
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
      if (brw->gen == 5) {
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
   const struct brw_vertex_element_packet *brw_velems = brw->curr.velems;
   unsigned size = brw_velems->header.length + 2;

   /* why is this here */
   brw_emit_query_begin(brw);

   brw_batchbuffer_data(brw->batch, brw_velems, size * 4, IGNORE_CLIPRECTS);

   return 0;
}


static int brw_emit_vertices( struct brw_context *brw )
{
   int ret;

   ret = brw_emit_vertex_buffers( brw );
   if (ret)
      return ret;

   /* XXX should separate this? */
   ret = brw_emit_vertex_elements( brw );
   if (ret)
      return ret;

   return 0;
}


const struct brw_tracked_state brw_vertices = {
   .dirty = {
      .mesa = (PIPE_NEW_INDEX_RANGE |
               PIPE_NEW_VERTEX_BUFFER |
               PIPE_NEW_VERTEX_ELEMENT),
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = brw_prepare_vertices,
   .emit = brw_emit_vertices,
};


static int brw_prepare_indices(struct brw_context *brw)
{
   struct pipe_resource *index_buffer = brw->curr.index_buffer;
   struct pipe_resource *upload_buf = NULL;
   struct brw_winsys_buffer *bo = NULL;
   GLuint offset;
   GLuint index_size, index_offset;
   GLuint ib_size;
   int ret;

   if (index_buffer == NULL)
      return 0;

   if (BRW_DEBUG & DEBUG_VERTS)
      debug_printf("%s: index_size:%d index_buffer->size:%d\n",
		   __FUNCTION__,
		   brw->curr.index_size,
		   brw->curr.index_buffer->width0);

   ib_size = index_buffer->width0;
   index_size = brw->curr.index_size;
   index_offset = brw->curr.index_offset;

   /* Turn userbuffer into a proper hardware buffer?
    */
   if (brw_buffer_is_user_buffer(index_buffer)) {
      boolean flushed;

      ret = u_upload_buffer( brw->vb.upload_index,
                             0,
			     index_offset,
			     ib_size,
			     index_buffer,
			     &offset,
			     &upload_buf,
			     &flushed );
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
      offset = index_offset;
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

   pipe_resource_reference( &upload_buf, NULL );
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
