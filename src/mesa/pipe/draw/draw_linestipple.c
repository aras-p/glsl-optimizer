/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

/* Implement line stipple by cutting lines up into smaller lines.
 * There are hundreds of ways to implement line stipple, this is one
 * choice that should work in all situations, requires no state
 * manipulations, but with a penalty in terms of large amounts of
 * generated geometry.
 */

#include "imports.h"
#include "macros.h"

#define CLIP_PRIVATE
#include "clip/clip_context.h"

#define CLIP_PIPE_PRIVATE
#include "clip/clip_pipe.h"



struct stipple_stage {
   struct clip_pipe_stage stage;

   GLuint hw_data_offset;

   GLfloat counter;
   GLuint pattern;
   GLuint factor;
};




static INLINE struct stipple_stage *stipple_stage( struct clip_pipe_stage *stage )
{
   return (struct stipple_stage *)stage;
}




static void interp_attr( const struct vf_attr *a,
			 GLubyte *vdst,
			 GLfloat t,
			 const GLubyte *vin,
			 const GLubyte *vout )
{
   GLuint offset = a->vertoffset;
   GLfloat fin[4], fout[4], fdst[4];
   
   a->extract( a, fin, vin + offset );
   a->extract( a, fout, vout + offset );

   fdst[0] = LINTERP( t, fout[0], fin[0] );
   fdst[1] = LINTERP( t, fout[1], fin[1] );
   fdst[2] = LINTERP( t, fout[2], fin[2] );
   fdst[3] = LINTERP( t, fout[3], fin[3] );

   a->insert[4-1]( a, vdst + offset, fdst );
}




/* Weird screen-space interpolation??  Otherwise do something special
 * with pos.w or fix vertices to always have clip coords available.
 */
static void screen_interp( struct vertex_fetch *vf,
			   struct vertex_header *dst,
			   GLfloat t,
			   const struct vertex_header *out, 
			   const struct vertex_header *in )
{
   GLubyte *vdst = (GLubyte *)dst;
   const GLubyte *vin  = (const GLubyte *)in;
   const GLubyte *vout = (const GLubyte *)out;

   const struct vf_attr *a = vf->attr;
   const GLuint attr_count = vf->attr_count;
   GLuint j;

   /* Vertex header.
    */
   {
      assert(a[0].attrib == VF_ATTRIB_VERTEX_HEADER);
      dst->clipmask = 0;
      dst->edgeflag = 0;
      dst->pad = 0;
      dst->index = 0xffff;
   }

   
   /* Other attributes
    */
   for (j = 1; j < attr_count; j++) {
      interp_attr(&a[j], vdst, t, vin, vout);
   }
}



/* Clip a line against the viewport and user clip planes.
 */
static void draw_line_segment( struct clip_pipe_stage *stage,
			       struct prim_header *header,
			       GLfloat t0,
			       GLfloat t1 )
{
   struct vertex_fetch *vf = stage->pipe->draw->vb.vf;
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];

   struct prim_header newprim = *header;
   header = &newprim;

   _mesa_printf("%s %f %f\n", __FUNCTION__, t0, t1);

   if (t0 > 0.0) {
      screen_interp( vf, stage->tmp[0], t0, v0, v1 );
      header->v[0] = stage->tmp[0];
   }

   if (t1 < 1.0) {
      screen_interp( vf, stage->tmp[1], t1, v0, v1 );
      header->v[1] = stage->tmp[1];
   }

   stage->next->line( stage->next, header );
}



/* XXX: don't really want to iterate like this.
 */
static INLINE unsigned
stipple_test(int counter, ushort pattern, int factor)
{
   int b = (counter / factor) & 0xf;
   return (1 << b) & pattern;
}



/* XXX:  Need to have precalculated flatshading already.
 */
static void stipple_line( struct clip_pipe_stage *stage,
			  struct prim_header *header )
{
   struct stipple_stage *stipple = stipple_stage(stage);
   GLuint hw_data_offset = stipple->hw_data_offset;
   const GLfloat *pos0 = (GLfloat *)&(header->v[0]->data[hw_data_offset]);
   const GLfloat *pos1 = (GLfloat *)&(header->v[1]->data[hw_data_offset]);
   GLfloat start = 0;
   int state = 0;

   GLfloat x0 = (GLfloat)pos0[0];
   GLfloat x1 = (GLfloat)pos1[0];
   GLfloat y0 = (GLfloat)pos0[1];
   GLfloat y1 = (GLfloat)pos1[1];

   GLfloat dx = x0 > x1 ? x0 - x1 : x1 - x0;
   GLfloat dy = y0 > y1 ? y0 - y1 : y1 - y0;

   GLfloat length = MAX2(dx, dy);
   GLint i;
   
   if (header->reset_line_stipple)   
      stipple->counter = 0;

   /* XXX: iterating like this is lame
    */
   for (i = 0; i < length; i++) {
      int result = !!stipple_test( stipple->counter+i, stipple->pattern, stipple->factor );
//      _mesa_printf("%d %f %d\n", i, length, result);
      if (result != state) {
	 if (state) {
	    if (start != i)
	       draw_line_segment( stage, header, start / length, i / length );
	 }
	 else {
	    start = i;
	 }
	 state = result;	   
      }
   }

   if (state && start < length)
      draw_line_segment( stage, header, start / length, 1.0 );

   stipple->counter += length;
}



static void stipple_begin( struct clip_pipe_stage *stage )
{
   struct stipple_stage *stipple = stipple_stage(stage);
   struct clip_context *draw = stage->pipe->draw;

   if (stage->pipe->draw->vb_state.clipped_prims)
      stipple->hw_data_offset = 16;
   else
      stipple->hw_data_offset = 0;	

   stipple->stage.tri = clip_passthrough_tri;
   stipple->stage.point = clip_passthrough_point;

   stipple->stage.line = stipple_line;
   stipple->factor = draw->state.line_stipple_factor + 1;
   stipple->pattern = draw->state.line_stipple_pattern;

   stage->next->begin( stage->next );
}



static void stipple_end( struct clip_pipe_stage *stage )
{
   stage->next->end( stage->next );
}

struct clip_pipe_stage *clip_pipe_stipple( struct clip_pipeline *pipe )
{
   struct stipple_stage *stipple = CALLOC_STRUCT(stipple_stage);

   clip_pipe_alloc_tmps( &stipple->stage, 4 );

   stipple->stage.pipe = pipe;
   stipple->stage.next = NULL;
   stipple->stage.begin = stipple_begin;
   stipple->stage.point = clip_passthrough_point;
   stipple->stage.line = stipple_line;
   stipple->stage.tri = clip_passthrough_tri;
   stipple->stage.reset_tmps = clip_pipe_reset_tmps;
   stipple->stage.end = stipple_end;

   return &stipple->stage;
}
