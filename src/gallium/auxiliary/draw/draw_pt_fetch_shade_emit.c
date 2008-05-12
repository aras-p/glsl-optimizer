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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_vs.h"

#include "translate/translate.h"

struct fetch_shade_emit;

struct fse_shader {
   struct translate_key key;

   void (*run_linear)( const struct fetch_shade_emit *fse,
                       unsigned start,
                       unsigned count,
                       char *buffer );
};

/* Prototype fetch, shade, emit-hw-verts all in one go.
 */
struct fetch_shade_emit {
   struct draw_pt_front_end base;

   struct draw_context *draw;

   struct translate_key key;

   /* Temporaries:
    */
   const float *constants;
   unsigned pitch[PIPE_MAX_ATTRIBS];
   const ubyte *src[PIPE_MAX_ATTRIBS];
   unsigned prim;

   /* Points to one of the three hardwired example shaders, below:
    */
   struct fse_shader *active;

   /* Temporary: A list of hard-wired shaders.  Of course the plan
    * would be to generate these for a given (vertex-shader,
    * translate-key) pair...
    */
   struct fse_shader shader[10];
   int nr_shaders;
};



/* Not quite passthrough yet -- we're still running the 'shader' here,
 * inlined into the vertex fetch function.
 */
static void fetch_xyz_rgb_st( const struct fetch_shade_emit *fse,
			      unsigned start,
			      unsigned count,
                              char *buffer )
{
   unsigned i;

   const float *m = fse->constants;
   const float m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
   const float m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
   const float m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
   const float m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];

   const ubyte *xyz = fse->src[0] + start * fse->pitch[0];
   const ubyte *st = fse->src[2] + start * fse->pitch[2];
   
   float *out = (float *)buffer;


   assert(fse->pitch[1] == 0);

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (i = 0; i < count; i++) {
      {
	 const float *in = (const float *)xyz;
	 const float ix = in[0], iy = in[1], iz = in[2];

	 out[0] = m0 * ix + m4 * iy + m8  * iz + m12;
	 out[1] = m1 * ix + m5 * iy + m9  * iz + m13;
	 out[2] = m2 * ix + m6 * iy + m10 * iz + m14;
	 out[3] = m3 * ix + m7 * iy + m11 * iz + m15;
	 xyz += fse->pitch[0];
      }

      {
	 out[4] = 1.0f;
	 out[5] = 1.0f;
	 out[6] = 1.0f;
 	 out[7] = 1.0f;
      }

      {
	 const float *in = (const float *)st; st += fse->pitch[2];
	 out[8] = in[0];
	 out[9] = in[1];
	 out[10] = 0.0f;
 	 out[11] = 1.0f;
      }

      out += 12;
   }
}



static void fetch_xyz_rgb( const struct fetch_shade_emit *fse,
			   unsigned start,
			   unsigned count,
                           char *buffer )
{
   unsigned i;

   const float *m = (const float *)fse->constants;
   const float m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
   const float m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
   const float m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
   const float m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];

   const ubyte *xyz = fse->src[0] + start * fse->pitch[0];
   const ubyte *rgb = fse->src[1] + start * fse->pitch[1];

   float *out = (float *)buffer;

//   debug_printf("rgb %f %f %f\n", rgb[0], rgb[1], rgb[2]);


   for (i = 0; i < count; i++) {
      {
	 const float *in = (const float *)xyz;
	 const float ix = in[0], iy = in[1], iz = in[2];

	 out[0] = m0 * ix + m4 * iy + m8  * iz + m12;
	 out[1] = m1 * ix + m5 * iy + m9  * iz + m13;
	 out[2] = m2 * ix + m6 * iy + m10 * iz + m14;
	 out[3] = m3 * ix + m7 * iy + m11 * iz + m15;
	 xyz += fse->pitch[0];
      }

      {
	 const float *in = (const float *)rgb;
	 out[4] = in[0];
	 out[5] = in[1];
	 out[6] = in[2];
 	 out[7] = 1.0f;
         rgb += fse->pitch[1];
      }

      out += 8;
   }
}




static void fetch_xyz_rgb_psiz( const struct fetch_shade_emit *fse,
				unsigned start,
				unsigned count,
                                char *buffer )
{
   unsigned i;

   const float *m = (const float *)fse->constants;
   const float m0 = m[0],  m4 = m[4],  m8 = m[8],  m12 = m[12];
   const float m1 = m[1],  m5 = m[5],  m9 = m[9],  m13 = m[13];
   const float m2 = m[2],  m6 = m[6],  m10 = m[10],  m14 = m[14];
   const float m3 = m[3],  m7 = m[7],  m11 = m[11],  m15 = m[15];

   const ubyte *xyz = fse->src[0] + start * fse->pitch[0];
   const float *rgb = (const float *)(fse->src[1] + start * fse->pitch[1]);
   const float psiz = 1.0;

   float *out = (float *)buffer;


   assert(fse->pitch[1] == 0);

   for (i = 0; i < count; i++) {
      {
	 const float *in = (const float *)xyz;
	 const float ix = in[0], iy = in[1], iz = in[2];

	 out[0] = m0 * ix + m4 * iy + m8  * iz + m12;
	 out[1] = m1 * ix + m5 * iy + m9  * iz + m13;
	 out[2] = m2 * ix + m6 * iy + m10 * iz + m14;
	 out[3] = m3 * ix + m7 * iy + m11 * iz + m15;
	 xyz += fse->pitch[0];
      }

      {
	 out[4] = rgb[0];
	 out[5] = rgb[1];
	 out[6] = rgb[2];
 	 out[7] = 1.0f;
      }

      {
	 out[8] = psiz;
      }

      out += 9;
   }
}




static boolean set_prim( struct fetch_shade_emit *fse,
                         unsigned prim,
                         unsigned count )
{
   struct draw_context *draw = fse->draw;

   fse->prim = prim;

   switch (prim) { 
   case PIPE_PRIM_LINE_LOOP:
      if (count > 1024)
         return FALSE;
      draw->render->set_primitive( draw->render, PIPE_PRIM_LINE_STRIP );
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      if (count > 1024)
         return FALSE;
      draw->render->set_primitive( draw->render, prim );
      break;

   case PIPE_PRIM_QUADS:
   case PIPE_PRIM_QUAD_STRIP:
      draw->render->set_primitive( draw->render, PIPE_PRIM_TRIANGLES );
      break;

   default:
      draw->render->set_primitive( draw->render, prim );
      break;
   }

   return TRUE;
}





			       
static void fse_prepare( struct draw_pt_front_end *fe,
                            unsigned prim,
                            struct draw_pt_middle_end *unused,
                            unsigned opt )
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)fe;
   struct draw_context *draw = fse->draw;
   unsigned num_vs_inputs = draw->vertex_shader->info.num_inputs;
   unsigned num_vs_outputs = draw->vertex_shader->info.num_outputs;
   const struct vertex_info *vinfo;
   unsigned i;
   boolean need_psize = 0;
   

   if (draw->pt.user.elts) {
      assert(0);
      return ;
   }

   if (!set_prim(fse, prim, /*count*/1022 )) {
      assert(0);
      return ;
   }

   /* Must do this after set_primitive() above:
    */
   vinfo = draw->render->get_vertex_info(draw->render);
   


   fse->key.nr_elements = MAX2(num_vs_outputs,     /* outputs - translate to hw format */
                               num_vs_inputs);     /* inputs - fetch from api format */

   fse->key.output_stride = vinfo->size * 4;
   memset(fse->key.element, 0, 
          fse->key.nr_elements * sizeof(fse->key.element[0]));

   for (i = 0; i < num_vs_inputs; i++) {
      const struct pipe_vertex_element *src = &draw->pt.vertex_element[i];
      fse->key.element[i].input_format = src->src_format;

      /* Consider ignoring these at this point, ie make generated
       * programs independent of this state:
       */
      fse->key.element[i].input_buffer = 0; //src->vertex_buffer_index;
      fse->key.element[i].input_offset = 0; //src->src_offset;
   }
   

   {
      unsigned dst_offset = 0;

      for (i = 0; i < vinfo->num_attribs; i++) {
         unsigned emit_sz = 0;
         unsigned output_format = PIPE_FORMAT_NONE;
         unsigned vs_output = vinfo->src_index[i];

         switch (vinfo->emit[i]) {
         case EMIT_4F:
            output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
            emit_sz = 4 * sizeof(float);
            break;
         case EMIT_3F:
            output_format = PIPE_FORMAT_R32G32B32_FLOAT;
            emit_sz = 3 * sizeof(float);
            break;
         case EMIT_2F:
            output_format = PIPE_FORMAT_R32G32_FLOAT;
            emit_sz = 2 * sizeof(float);
            break;
         case EMIT_1F:
            output_format = PIPE_FORMAT_R32_FLOAT;
            emit_sz = 1 * sizeof(float);
            break;
         case EMIT_1F_PSIZE:
            need_psize = 1;
            output_format = PIPE_FORMAT_R32_FLOAT;
            emit_sz = 1 * sizeof(float);
            vs_output = num_vs_outputs + 1;
         
            break;
         default:
            assert(0);
            break;
         }

         /* The elements in the key correspond to vertex shader output
          * numbers, not to positions in the hw vertex description --
          * that's handled by the output_offset field.
          */
         fse->key.element[vs_output].output_format = output_format;
         fse->key.element[vs_output].output_offset = dst_offset;
      
         dst_offset += emit_sz;
         assert(fse->key.output_stride >= dst_offset);
      }
   }

   /* To make psize work, really need to tell the vertex shader to
    * copy that value from input->output.  For 'translate' this was
    * implicit for all elements.
    */
#if 0
   if (need_psize) {
      unsigned input = num_vs_inputs + 1;
      const struct pipe_vertex_element *src = &draw->pt.vertex_element[i];
      fse->key.element[i].input_format = PIPE_FORMAT_R32_FLOAT;
      fse->key.element[i].input_buffer = 0; //nr_buffers + 1;
      fse->key.element[i].input_offset = 0; 

      fse->key.nr_elements += 1;
      
   }
#endif
   
   fse->constants = draw->pt.user.constants;

   /* Would normally look up a vertex shader and peruse its list of
    * varients somehow.  We omitted that step and put all the
    * hardcoded "shaders" into an array.  We're just making the
    * assumption that this happens to be a matching shader...  ie
    * you're running isosurf, aren't you?
    */
   fse->active = NULL;
   for (i = 0; i < fse->nr_shaders; i++) {
      if (translate_key_compare( &fse->key, &fse->shader[i].key) == 0)
         fse->active = &fse->shader[i];
   }

   if (!fse->active) {
      assert(0);
      return ;
   }

   /* Now set buffer pointers:
    */
   for (i = 0; i < num_vs_inputs; i++) {
      unsigned buf = draw->pt.vertex_element[i].vertex_buffer_index;

      fse->src[i] = ((const ubyte *) draw->pt.user.vbuffer[buf] + 
                     draw->pt.vertex_buffer[buf].buffer_offset + 
                     draw->pt.vertex_element[i].src_offset);

      fse->pitch[i] = draw->pt.vertex_buffer[buf].pitch;

   }

   
   //return TRUE;
}


static boolean split_prim_inplace(unsigned prim, unsigned *first, unsigned *incr)
{
   switch (prim) {
   case PIPE_PRIM_POINTS:
      *first = 1;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_LINES:
      *first = 2;
      *incr = 2;
      return TRUE;
   case PIPE_PRIM_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_TRIANGLES:
      *first = 3;
      *incr = 3;
      return TRUE;
   case PIPE_PRIM_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return TRUE;
   case PIPE_PRIM_QUADS:
      *first = 4;
      *incr = 4;
      return TRUE;
   case PIPE_PRIM_QUAD_STRIP:
      *first = 4;
      *incr = 2;
      return TRUE;
   default:
      *first = 0;
      *incr = 1;		/* set to one so that count % incr works */
      return FALSE;
   }
}




#define INDEX(i) (start + (i))
static void fse_render_linear( struct vbuf_render *render,
                               unsigned prim,
                               unsigned start,
                               unsigned length )
{
   ushort *tmp = NULL;
   unsigned i, j;

   switch (prim) {
   case PIPE_PRIM_LINE_LOOP:
      tmp = MALLOC( sizeof(ushort) * (length + 1) );

      for (i = 0; i < length; i++)
	 tmp[i] = INDEX(i);
      tmp[length] = 0;

      render->draw( render,
			  tmp,
			  length+1 );
      break;


   case PIPE_PRIM_QUAD_STRIP:
      tmp = MALLOC( sizeof(ushort) * (length / 2 * 6) );

      for (j = i = 0; i + 3 < length; i += 2, j += 6) {
	 tmp[j+0] = INDEX(i+0);
	 tmp[j+1] = INDEX(i+1);
	 tmp[j+2] = INDEX(i+3);

	 tmp[j+3] = INDEX(i+2);
	 tmp[j+4] = INDEX(i+0);
	 tmp[j+5] = INDEX(i+3);
      }

      if (j)
	 render->draw( render, tmp, j );
      break;

   case PIPE_PRIM_QUADS:
      tmp = MALLOC( sizeof(int) * (length / 4 * 6) );

      for (j = i = 0; i + 3 < length; i += 4, j += 6) {
	 tmp[j+0] = INDEX(i+0);
	 tmp[j+1] = INDEX(i+1);
	 tmp[j+2] = INDEX(i+3);

	 tmp[j+3] = INDEX(i+1);
	 tmp[j+4] = INDEX(i+2);
	 tmp[j+5] = INDEX(i+3);
      }

      if (j)
	 render->draw( render, tmp, j );
      break;

   default:
      render->draw_arrays( render,
                                 start,
                                 length );
      break;
   }

   if (tmp)
      FREE(tmp);
}



static boolean do_draw( struct fetch_shade_emit *fse, 
                        unsigned start, unsigned count )
{
   struct draw_context *draw = fse->draw;

   char *hw_verts = 
      draw->render->allocate_vertices( draw->render,
                                       (ushort)fse->key.output_stride,
                                       (ushort)count );

   if (!hw_verts)
      return FALSE;
					
   /* Single routine to fetch vertices, run shader and emit HW verts.
    * Clipping and viewport transformation are done on hardware.
    */
   fse->active->run_linear( fse, 
                            start, count,
                            hw_verts );

   /* Draw arrays path to avoid re-emitting index list again and
    * again.
    */
   fse_render_linear( draw->render,
                      fse->prim,
                      0,
                      count );
   

   draw->render->release_vertices( draw->render, 
				   hw_verts, 
				   fse->key.output_stride, 
				   count );

   return TRUE;
}


static void
fse_run(struct draw_pt_front_end *fe, 
        pt_elt_func elt_func,
        const void *elt_ptr,
        unsigned count)
{
   struct fetch_shade_emit *fse = (struct fetch_shade_emit *)fe;
   unsigned i = 0;
   unsigned first, incr;
   unsigned start = elt_func(elt_ptr, 0);
   
   //debug_printf("%s prim %d start %d count %d\n", __FUNCTION__, prim, start, count);
   
   split_prim_inplace(fse->prim, &first, &incr);

   count -= (count - first) % incr; 

   while (i + first <= count) {
      int nr = MIN2( count - i, 1024 );

      /* snap to prim boundary 
       */
      nr -= (nr - first) % incr; 

      if (!do_draw( fse, start + i, nr )) {
         assert(0);
         return ;
      }

      /* increment allowing for repeated vertices
       */
      i += nr - (first - incr);
   }

   //return TRUE;
}


static void fse_finish( struct draw_pt_front_end *frontend )
{
}


static void
fse_destroy( struct draw_pt_front_end *frontend ) 
{
   FREE(frontend);
}

struct draw_pt_front_end *draw_pt_fetch_shade_emit( struct draw_context *draw )
{
   struct fetch_shade_emit *fse = CALLOC_STRUCT(fetch_shade_emit);
   if (!fse)
      return NULL;

   fse->base.prepare = fse_prepare;
   fse->base.run = fse_run;
   fse->base.finish = fse_finish;
   fse->base.destroy = fse_destroy;
   fse->draw = draw;

   fse->shader[0].run_linear = fetch_xyz_rgb_st;
   fse->shader[0].key.nr_elements = 3;
   fse->shader[0].key.output_stride = 12 * sizeof(float);

   fse->shader[0].key.element[0].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[0].key.element[0].input_buffer = 0; 
   fse->shader[0].key.element[0].input_offset = 0; 
   fse->shader[0].key.element[0].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[0].key.element[0].output_offset = 0; 

   fse->shader[0].key.element[1].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[0].key.element[1].input_buffer = 0; 
   fse->shader[0].key.element[1].input_offset = 0; 
   fse->shader[0].key.element[1].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[0].key.element[1].output_offset = 16; 

   fse->shader[0].key.element[1].input_format = PIPE_FORMAT_R32G32_FLOAT;
   fse->shader[0].key.element[1].input_buffer = 0; 
   fse->shader[0].key.element[1].input_offset = 0; 
   fse->shader[0].key.element[1].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[0].key.element[1].output_offset = 32; 

   fse->shader[1].run_linear = fetch_xyz_rgb;
   fse->shader[1].key.nr_elements = 2;
   fse->shader[1].key.output_stride = 8 * sizeof(float);

   fse->shader[1].key.element[0].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[1].key.element[0].input_buffer = 0; 
   fse->shader[1].key.element[0].input_offset = 0; 
   fse->shader[1].key.element[0].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[1].key.element[0].output_offset = 0; 

   fse->shader[1].key.element[1].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[1].key.element[1].input_buffer = 0; 
   fse->shader[1].key.element[1].input_offset = 0; 
   fse->shader[1].key.element[1].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[1].key.element[1].output_offset = 16; 

   fse->shader[2].run_linear = fetch_xyz_rgb_psiz;
   fse->shader[2].key.nr_elements = 3;
   fse->shader[2].key.output_stride = 9 * sizeof(float);

   fse->shader[2].key.element[0].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[2].key.element[0].input_buffer = 0; 
   fse->shader[2].key.element[0].input_offset = 0; 
   fse->shader[2].key.element[0].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[2].key.element[0].output_offset = 0; 

   fse->shader[2].key.element[1].input_format = PIPE_FORMAT_R32G32B32_FLOAT;
   fse->shader[2].key.element[1].input_buffer = 0; 
   fse->shader[2].key.element[1].input_offset = 0; 
   fse->shader[2].key.element[1].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fse->shader[2].key.element[1].output_offset = 16; 

   /* psize is special 
    *    -- effectively add it here as another input!?! 
    *    -- who knows how to add it as a buffer?
    */
   fse->shader[2].key.element[2].input_format = PIPE_FORMAT_R32_FLOAT;
   fse->shader[2].key.element[2].input_buffer = 0; 
   fse->shader[2].key.element[2].input_offset = 0; 
   fse->shader[2].key.element[2].output_format = PIPE_FORMAT_R32_FLOAT;
   fse->shader[2].key.element[2].output_offset = 32; 

   fse->nr_shaders = 3;

   return &fse->base;
}
