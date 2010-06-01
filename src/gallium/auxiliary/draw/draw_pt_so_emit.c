/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"
#include "util/u_format.h"

struct pt_so_emit {
   struct draw_context *draw;

   struct translate *translate;

   struct translate_cache *cache;
   unsigned prim;

   const struct vertex_info *vinfo;
   boolean has_so;
};

static void
prepare_so_emit( struct pt_so_emit *emit,
                 const struct vertex_info *vinfo )
{
   struct draw_context *draw = emit->draw;
   unsigned i;
   struct translate_key hw_key;
   unsigned dst_offset = 0;
   unsigned output_stride = 0;

   if (emit->has_so) {
      for (i = 0; i < draw->so.state.num_outputs; ++i) {
         unsigned src_offset = (draw->so.state.register_index[i] * 4 *
                                sizeof(float) );
         unsigned output_format;
         unsigned emit_sz = 0;
         /*unsigned output_bytes = util_format_get_blocksize(output_format);
           unsigned nr_compo = util_format_get_nr_components(output_format);*/

         output_format = draw_translate_vinfo_format(vinfo->attrib[i].emit);
         emit_sz = draw_translate_vinfo_size(vinfo->attrib[i].emit);

         /* doesn't handle EMIT_OMIT */
         assert(emit_sz != 0);

         hw_key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
         hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         hw_key.element[i].input_buffer = 0;
         hw_key.element[i].input_offset = src_offset;
         hw_key.element[i].instance_divisor = 0;
         hw_key.element[i].output_format = output_format;
         hw_key.element[i].output_offset = dst_offset;

         dst_offset += emit_sz;
         output_stride += emit_sz;
      }
      hw_key.nr_elements = draw->so.state.num_outputs;
      hw_key.output_stride = output_stride;

      if (!emit->translate ||
          translate_key_compare(&emit->translate->key, &hw_key) != 0)
      {
         translate_key_sanitize(&hw_key);
         emit->translate = translate_cache_find(emit->cache, &hw_key);
      }
   } else {
      /* no stream output */
      emit->translate = NULL;
   }
}


void draw_pt_so_emit_prepare( struct pt_so_emit *emit,
                              unsigned prim )
{
   struct draw_context *draw = emit->draw;
   boolean ok;

   emit->has_so = (draw->so.state.num_outputs > 0);

   if (!emit->has_so)
      return;

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??
    */
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   emit->prim = prim;

   ok = draw->render->set_primitive(draw->render, emit->prim);
   if (!ok) {
      assert(0);
      return;
   }

   /* Must do this after set_primitive() above: */
   emit->vinfo = draw->render->get_vertex_info(draw->render);

   prepare_so_emit( emit, emit->vinfo );
}


void draw_pt_so_emit( struct pt_so_emit *emit,
		   const float (*vertex_data)[4],
		   unsigned vertex_count,
		   unsigned stride )
{
   struct draw_context *draw = emit->draw;
   struct translate *translate = emit->translate;
   struct vbuf_render *render = draw->render;
   void *so_buffer;

   if (!emit->has_so)
      return;

   so_buffer = draw->so.buffers[0];

   /* XXX: need to flush to get prim_vbuf.c to release its allocation??*/
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );

   if (vertex_count == 0)
      return;

   if (vertex_count >= UNDEFINED_VERTEX_ID) {
      assert(0);
      return;
   }


   /* XXX we only support single output buffer right now */
   debug_assert(draw->so.num_buffers >= 0);

   translate->set_buffer(translate, 0, vertex_data,
                         stride, ~0);
   translate->run(translate, 0, vertex_count,
                  draw->instance_id, so_buffer);

   render->set_stream_output_info(render, 0, vertex_count);
}


struct pt_so_emit *draw_pt_so_emit_create( struct draw_context *draw )
{
   struct pt_so_emit *emit = CALLOC_STRUCT(pt_so_emit);
   if (!emit)
      return NULL;

   emit->draw = draw;
   emit->cache = translate_cache_create();
   if (!emit->cache) {
      FREE(emit);
      return NULL;
   }

   return emit;
}

void draw_pt_so_emit_destroy( struct pt_so_emit *emit )
{
   if (emit->cache)
      translate_cache_destroy(emit->cache);

   FREE(emit);
}
