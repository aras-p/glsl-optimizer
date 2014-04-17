/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_helpers.h"
#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "tgsi/tgsi_parse.h"

#include "svga_screen.h"
#include "svga_resource_buffer.h"
#include "svga_context.h"


static void svga_set_vertex_buffers(struct pipe_context *pipe,
                                    unsigned start_slot, unsigned count,
                                    const struct pipe_vertex_buffer *buffers)
{
   struct svga_context *svga = svga_context(pipe);

   util_set_vertex_buffers_count(svga->curr.vb,
                                 &svga->curr.num_vertex_buffers,
                                 buffers, start_slot, count);

   svga->dirty |= SVGA_NEW_VBUFFER;
}


static void svga_set_index_buffer(struct pipe_context *pipe,
                                  const struct pipe_index_buffer *ib)
{
   struct svga_context *svga = svga_context(pipe);

   if (ib) {
      pipe_resource_reference(&svga->curr.ib.buffer, ib->buffer);
      memcpy(&svga->curr.ib, ib, sizeof(svga->curr.ib));
   }
   else {
      pipe_resource_reference(&svga->curr.ib.buffer, NULL);
      memset(&svga->curr.ib, 0, sizeof(svga->curr.ib));
   }

   /* TODO make this more like a state */
}


/**
 * Given a gallium vertex element format, return the corresponding SVGA3D
 * format.  Return SVGA3D_DECLTYPE_MAX for unsupported gallium formats.
 */
static SVGA3dDeclType
translate_vertex_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R32_FLOAT:            return SVGA3D_DECLTYPE_FLOAT1;
   case PIPE_FORMAT_R32G32_FLOAT:         return SVGA3D_DECLTYPE_FLOAT2;
   case PIPE_FORMAT_R32G32B32_FLOAT:      return SVGA3D_DECLTYPE_FLOAT3;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:   return SVGA3D_DECLTYPE_FLOAT4;
   case PIPE_FORMAT_B8G8R8A8_UNORM:       return SVGA3D_DECLTYPE_D3DCOLOR;
   case PIPE_FORMAT_R8G8B8A8_USCALED:     return SVGA3D_DECLTYPE_UBYTE4;
   case PIPE_FORMAT_R16G16_SSCALED:       return SVGA3D_DECLTYPE_SHORT2;
   case PIPE_FORMAT_R16G16B16A16_SSCALED: return SVGA3D_DECLTYPE_SHORT4;
   case PIPE_FORMAT_R8G8B8A8_UNORM:       return SVGA3D_DECLTYPE_UBYTE4N;
   case PIPE_FORMAT_R16G16_SNORM:         return SVGA3D_DECLTYPE_SHORT2N;
   case PIPE_FORMAT_R16G16B16A16_SNORM:   return SVGA3D_DECLTYPE_SHORT4N;
   case PIPE_FORMAT_R16G16_UNORM:         return SVGA3D_DECLTYPE_USHORT2N;
   case PIPE_FORMAT_R16G16B16A16_UNORM:   return SVGA3D_DECLTYPE_USHORT4N;
   case PIPE_FORMAT_R10G10B10X2_USCALED:  return SVGA3D_DECLTYPE_UDEC3;
   case PIPE_FORMAT_R10G10B10X2_SNORM:    return SVGA3D_DECLTYPE_DEC3N;
   case PIPE_FORMAT_R16G16_FLOAT:         return SVGA3D_DECLTYPE_FLOAT16_2;
   case PIPE_FORMAT_R16G16B16A16_FLOAT:   return SVGA3D_DECLTYPE_FLOAT16_4;

   /* See attrib_needs_adjustment() and attrib_needs_w_to_1() below */
   case PIPE_FORMAT_R8G8B8_SNORM:         return SVGA3D_DECLTYPE_UBYTE4N;

   /* See attrib_needs_w_to_1() below */
   case PIPE_FORMAT_R16G16B16_SNORM:      return SVGA3D_DECLTYPE_SHORT4N;
   case PIPE_FORMAT_R16G16B16_UNORM:      return SVGA3D_DECLTYPE_USHORT4N;
   case PIPE_FORMAT_R8G8B8_UNORM:         return SVGA3D_DECLTYPE_UBYTE4N;

   default:
      /* There are many formats without hardware support.  This case
       * will be hit regularly, meaning we'll need swvfetch.
       */
      return SVGA3D_DECLTYPE_MAX;
   }
}


/**
 * Does the given vertex attrib format need range adjustment in the VS?
 * Range adjustment scales and biases values from [0,1] to [-1,1].
 * This lets us avoid the swtnl path.
 */
static boolean
attrib_needs_range_adjustment(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R8G8B8_SNORM:
      return TRUE;
   default:
      return FALSE;
   }
}


/**
 * Does the given vertex attrib format need to have the W component set
 * to one in the VS?
 */
static boolean
attrib_needs_w_to_1(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_R8G8B8_SNORM:
   case PIPE_FORMAT_R8G8B8_UNORM:
   case PIPE_FORMAT_R16G16B16_SNORM:
   case PIPE_FORMAT_R16G16B16_UNORM:
      return TRUE;
   default:
      return FALSE;
   }
}


static void *
svga_create_vertex_elements_state(struct pipe_context *pipe,
                                  unsigned count,
                                  const struct pipe_vertex_element *attribs)
{
   struct svga_velems_state *velems;
   assert(count <= PIPE_MAX_ATTRIBS);
   velems = (struct svga_velems_state *) MALLOC(sizeof(struct svga_velems_state));
   if (velems) {
      unsigned i;

      velems->count = count;
      memcpy(velems->velem, attribs, sizeof(*attribs) * count);

      velems->need_swvfetch = FALSE;
      velems->adjust_attrib_range = 0x0;
      velems->adjust_attrib_w_1 = 0x0;

      /* Translate Gallium vertex format to SVGA3dDeclType */
      for (i = 0; i < count; i++) {
         enum pipe_format f = attribs[i].src_format;
         velems->decl_type[i] = translate_vertex_format(f);
         if (velems->decl_type[i] == SVGA3D_DECLTYPE_MAX) {
            /* Unsupported format - use software fetch */
            velems->need_swvfetch = TRUE;
            break;
         }

         if (attrib_needs_range_adjustment(f)) {
            velems->adjust_attrib_range |= (1 << i);
         }
         if (attrib_needs_w_to_1(f)) {
            velems->adjust_attrib_w_1 |= (1 << i);
         }
      }
   }
   return velems;
}

static void svga_bind_vertex_elements_state(struct pipe_context *pipe,
                                            void *velems)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_velems_state *svga_velems = (struct svga_velems_state *) velems;

   svga->curr.velems = svga_velems;
   svga->dirty |= SVGA_NEW_VELEMENT;
}

static void svga_delete_vertex_elements_state(struct pipe_context *pipe,
                                              void *velems)
{
   FREE(velems);
}

void svga_cleanup_vertex_state( struct svga_context *svga )
{
   unsigned i;
   
   for (i = 0 ; i < svga->curr.num_vertex_buffers; i++)
      pipe_resource_reference(&svga->curr.vb[i].buffer, NULL);
}


void svga_init_vertex_functions( struct svga_context *svga )
{
   svga->pipe.set_vertex_buffers = svga_set_vertex_buffers;
   svga->pipe.set_index_buffer = svga_set_index_buffer;
   svga->pipe.create_vertex_elements_state = svga_create_vertex_elements_state;
   svga->pipe.bind_vertex_elements_state = svga_bind_vertex_elements_state;
   svga->pipe.delete_vertex_elements_state = svga_delete_vertex_elements_state;
}


