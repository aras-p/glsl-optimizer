/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "translate/translate.h"


struct pt_fetch {
   struct draw_context *draw;

   struct translate *translate;
   
   unsigned vertex_size;
};



/* Perform the fetch from API vertex elements & vertex buffers, to a
 * contiguous set of float[4] attributes as required for the
 * vertex_shader->run_linear() method.
 *
 * This is used in all cases except pure passthrough
 * (draw_pt_fetch_emit.c) which has its own version to translate
 * directly to hw vertices.
 *
 */
void draw_pt_fetch_prepare( struct pt_fetch *fetch,
			    unsigned vertex_size )
{
   struct draw_context *draw = fetch->draw;
   unsigned i, nr = 0;
   unsigned dst_offset = 0;
   struct translate_key key;

   fetch->vertex_size = vertex_size;

   memset(&key, 0, sizeof(key));

   /* Always emit/leave space for a vertex header.
    *
    * It's worth considering whether the vertex headers should contain
    * a pointer to the 'data', rather than having it inline.
    * Something to look at after we've fully switched over to the pt
    * paths.
    */
   {
      /* Need to set header->vertex_id = 0xffff somehow.
       */
      key.element[nr].input_format = PIPE_FORMAT_R32_FLOAT;
      key.element[nr].input_buffer = draw->pt.nr_vertex_buffers;
      key.element[nr].input_offset = 0;
      key.element[nr].output_format = PIPE_FORMAT_R32_FLOAT;
      key.element[nr].output_offset = dst_offset;
      dst_offset += 1 * sizeof(float);
      nr++;


      /* Just leave the clip[] array untouched.
       */
      dst_offset += 4 * sizeof(float);
   }
      

   for (i = 0; i < draw->pt.nr_vertex_elements; i++) {
      key.element[nr].input_format = draw->pt.vertex_element[i].src_format;
      key.element[nr].input_buffer = draw->pt.vertex_element[i].vertex_buffer_index;
      key.element[nr].input_offset = draw->pt.vertex_element[i].src_offset;
      key.element[nr].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      key.element[nr].output_offset = dst_offset;

      dst_offset += 4 * sizeof(float);
      nr++;
   }

   assert(dst_offset <= vertex_size);

   key.nr_elements = nr;
   key.output_stride = vertex_size;


   /* Don't bother with caching at this stage:
    */
   if (!fetch->translate ||
       memcmp(&fetch->translate->key, &key, sizeof(key)) != 0) 
   {
      if (fetch->translate)
	 fetch->translate->release(fetch->translate);

      fetch->translate = translate_create( &key );

      {
	 static struct vertex_header vh = { 0, 0, 0, 0xffff };
	 fetch->translate->set_buffer(fetch->translate, 
				      draw->pt.nr_vertex_buffers, 
				      &vh,
				      0);
      }
   }
}




void draw_pt_fetch_run( struct pt_fetch *fetch,
			const unsigned *elts,
			unsigned count,
			char *verts )
{
   struct draw_context *draw = fetch->draw;
   struct translate *translate = fetch->translate;
   unsigned i;

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      translate->set_buffer(translate, 
			    i, 
			    ((char *)draw->pt.user.vbuffer[i] + 
			     draw->pt.vertex_buffer[i].buffer_offset),
			    draw->pt.vertex_buffer[i].pitch );
   }

   translate->run_elts( translate,
			elts, 
			count,
			verts );
}


struct pt_fetch *draw_pt_fetch_create( struct draw_context *draw )
{
   struct pt_fetch *fetch = CALLOC_STRUCT(pt_fetch);
   if (!fetch)
      return NULL;
	 
   fetch->draw = draw;
   return fetch;
}

void draw_pt_fetch_destroy( struct pt_fetch *fetch )
{
   if (fetch->translate)
      fetch->translate->release( fetch->translate );

   FREE(fetch);
}

