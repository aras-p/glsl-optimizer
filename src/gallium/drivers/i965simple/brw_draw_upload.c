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

#include "brw_batch.h"
#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"


struct brw_array_state {
   union header_union header;

   struct {
      union {
	 struct {
	    unsigned pitch:11;
	    unsigned pad:15;
	    unsigned access_type:1;
	    unsigned vb_index:5;
	 } bits;
	 unsigned dword;
      } vb0;

      struct pipe_buffer *buffer;
      unsigned offset;

      unsigned max_index;
      unsigned instance_data_step_rate;

   } vb[BRW_VBP_MAX];
};



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


boolean brw_upload_vertex_buffers( struct brw_context *brw )
{
   struct brw_array_state vbp;
   unsigned nr_enabled = 0;
   unsigned i;

   memset(&vbp, 0, sizeof(vbp));

   /* This is a hardware limit:
    */

   for (i = 0; i < BRW_VEP_MAX; i++)
   {
      if (brw->vb.vbo_array[i] == NULL) {
	 nr_enabled = i;
	 break;
      }

      vbp.vb[i].vb0.bits.pitch = brw->vb.vbo_array[i]->pitch;
      vbp.vb[i].vb0.bits.pad = 0;
      vbp.vb[i].vb0.bits.access_type = BRW_VERTEXBUFFER_ACCESS_VERTEXDATA;
      vbp.vb[i].vb0.bits.vb_index = i;
      vbp.vb[i].offset = brw->vb.vbo_array[i]->buffer_offset;
      vbp.vb[i].buffer = brw->vb.vbo_array[i]->buffer;
      vbp.vb[i].max_index = brw->vb.vbo_array[i]->max_index;
   }


   vbp.header.bits.length = (1 + nr_enabled * 4) - 2;
   vbp.header.bits.opcode = CMD_VERTEX_BUFFER;

   BEGIN_BATCH(vbp.header.bits.length+2, 0);
   OUT_BATCH( vbp.header.dword );

   for (i = 0; i < nr_enabled; i++) {
      OUT_BATCH( vbp.vb[i].vb0.dword );
      OUT_RELOC( vbp.vb[i].buffer,  PIPE_BUFFER_USAGE_GPU_READ,
		 vbp.vb[i].offset);
      OUT_BATCH( vbp.vb[i].max_index );
      OUT_BATCH( vbp.vb[i].instance_data_step_rate );
   }
   ADVANCE_BATCH();
   return TRUE;
}



boolean brw_upload_vertex_elements( struct brw_context *brw )
{
   struct brw_vertex_element_packet vep;

   unsigned i;
   unsigned nr_enabled = brw->attribs.VertexProgram->info.num_inputs;

   memset(&vep, 0, sizeof(vep));

   for (i = 0; i < nr_enabled; i++) 
      vep.ve[i] = brw->vb.inputs[i];


   vep.header.length = (1 + nr_enabled * sizeof(vep.ve[0])/4) - 2;
   vep.header.opcode = CMD_VERTEX_ELEMENT;
   brw_cached_batch_struct(brw, &vep, 4 + nr_enabled * sizeof(vep.ve[0]));

   return TRUE;
}

boolean brw_upload_indices( struct brw_context *brw,
                            const struct pipe_buffer *index_buffer,
                            int ib_size, int start, int count)
{
   /* Emit the indexbuffer packet:
    */
   {
      struct brw_indexbuffer ib;

      memset(&ib, 0, sizeof(ib));

      ib.header.bits.opcode = CMD_INDEX_BUFFER;
      ib.header.bits.length = sizeof(ib)/4 - 2;
      ib.header.bits.index_format = get_index_type(ib_size);
      ib.header.bits.cut_index_enable = 0;


      BEGIN_BATCH(4, 0);
      OUT_BATCH( ib.header.dword );
      OUT_RELOC( index_buffer, PIPE_BUFFER_USAGE_GPU_READ, start);
      OUT_RELOC( index_buffer, PIPE_BUFFER_USAGE_GPU_READ, start + count);
      OUT_BATCH( 0 );
      ADVANCE_BATCH();
   }
   return TRUE;
}
