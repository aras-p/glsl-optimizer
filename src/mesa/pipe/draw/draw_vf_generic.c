
/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_util.h"

#include "draw_vf.h"



static INLINE void insert_4f_4( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static INLINE void insert_4f_3( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}

static INLINE void insert_4f_2( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_4f_1( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_3f_xyw_4( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
}

static INLINE void insert_3f_xyw_err( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   (void) a; (void) v; (void) in;
   assert(0);
}

static INLINE void insert_3f_3( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}

static INLINE void insert_3f_2( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
}

static INLINE void insert_3f_1( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
}


static INLINE void insert_2f_2( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
}

static INLINE void insert_2f_1( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
}

static INLINE void insert_1f_1( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   float *out = (float *)(v);
   (void) a;

   out[0] = in[0];
}

static INLINE void insert_null( const struct draw_vf_attr *a, uint8_t *v, const float *in )
{
   (void) a; (void) v; (void) in;
}

static INLINE void insert_4ub_4f_rgba_4( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_rgba_3( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_2( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_1( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_4( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_bgra_3( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_2( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_1( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_argb_4( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_argb_3( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_2( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_1( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   v[2] = 0x00;
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_4( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_abgr_3( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_2( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_1( const struct draw_vf_attr *a, uint8_t *v, 
					 const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   v[2] = 0x00;
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_3ub_3f_rgb_3( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
}

static INLINE void insert_3ub_3f_rgb_2( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
}

static INLINE void insert_3ub_3f_rgb_1( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
}

static INLINE void insert_3ub_3f_bgr_3( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
}

static INLINE void insert_3ub_3f_bgr_2( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
}

static INLINE void insert_3ub_3f_bgr_1( const struct draw_vf_attr *a, uint8_t *v, 
					const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
}


static INLINE void insert_1ub_1f_1( const struct draw_vf_attr *a, uint8_t *v, 
				    const float *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
}


const struct draw_vf_format_info draw_vf_format_info[DRAW_EMIT_MAX] = 
{
   { "1f",
     { insert_1f_1, insert_1f_1, insert_1f_1, insert_1f_1 },
     sizeof(float), FALSE },

   { "2f",
     { insert_2f_1, insert_2f_2, insert_2f_2, insert_2f_2 },
     2 * sizeof(float), FALSE },

   { "3f",
     { insert_3f_1, insert_3f_2, insert_3f_3, insert_3f_3 },
     3 * sizeof(float), FALSE },

   { "4f",
     { insert_4f_1, insert_4f_2, insert_4f_3, insert_4f_4 },
     4 * sizeof(float), FALSE },

   { "3f_xyw",
     { insert_3f_xyw_err, insert_3f_xyw_err, insert_3f_xyw_err, 
       insert_3f_xyw_4 },
     3 * sizeof(float), FALSE },

   { "1ub_1f",
     { insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1 },
     sizeof(uint8_t), FALSE },

   { "3ub_3f_rgb",
     { insert_3ub_3f_rgb_1, insert_3ub_3f_rgb_2, insert_3ub_3f_rgb_3,
       insert_3ub_3f_rgb_3 },
     3 * sizeof(uint8_t), FALSE },

   { "3ub_3f_bgr",
     { insert_3ub_3f_bgr_1, insert_3ub_3f_bgr_2, insert_3ub_3f_bgr_3,
       insert_3ub_3f_bgr_3 },
     3 * sizeof(uint8_t), FALSE },

   { "4ub_4f_rgba",
     { insert_4ub_4f_rgba_1, insert_4ub_4f_rgba_2, insert_4ub_4f_rgba_3, 
       insert_4ub_4f_rgba_4 },
     4 * sizeof(uint8_t), FALSE },

   { "4ub_4f_bgra",
     { insert_4ub_4f_bgra_1, insert_4ub_4f_bgra_2, insert_4ub_4f_bgra_3,
       insert_4ub_4f_bgra_4 },
     4 * sizeof(uint8_t), FALSE },

   { "4ub_4f_argb",
     { insert_4ub_4f_argb_1, insert_4ub_4f_argb_2, insert_4ub_4f_argb_3,
       insert_4ub_4f_argb_4 },
     4 * sizeof(uint8_t), FALSE },

   { "4ub_4f_abgr",
     { insert_4ub_4f_abgr_1, insert_4ub_4f_abgr_2, insert_4ub_4f_abgr_3,
       insert_4ub_4f_abgr_4 },
     4 * sizeof(uint8_t), FALSE },

   { "1f_const",
     { insert_1f_1, insert_1f_1, insert_1f_1, insert_1f_1 },
     sizeof(float), TRUE },
   
   { "2f_const",
     { insert_2f_1, insert_2f_2, insert_2f_2, insert_2f_2 },
     2 * sizeof(float), TRUE },
   
   { "3f_const",
     { insert_3f_1, insert_3f_2, insert_3f_3, insert_3f_3 },
     3 * sizeof(float), TRUE },
   
   { "4f_const",
     { insert_4f_1, insert_4f_2, insert_4f_3, insert_4f_4 },
     4 * sizeof(float), TRUE },

   { "pad",
     { NULL, NULL, NULL, NULL },
     0, FALSE },

};



    
/***********************************************************************
 * Hardwired fastpaths for emitting whole vertices or groups of
 * vertices
 */
#define EMIT5(NR, F0, F1, F2, F3, F4, NAME)				\
static void NAME( struct draw_vertex_fetch *vf,				\
		  unsigned count,						\
		  uint8_t *v )						\
{									\
   struct draw_vf_attr *a = vf->attr;				\
   unsigned i;								\
									\
   for (i = 0 ; i < count ; i++, v += vf->vertex_stride) {		\
      if (NR > 0) {							\
	 F0( &a[0], v + a[0].vertoffset, (float *)a[0].inputptr );	\
	 a[0].inputptr += a[0].inputstride;				\
      }									\
      									\
      if (NR > 1) {							\
	 F1( &a[1], v + a[1].vertoffset, (float *)a[1].inputptr );	\
	 a[1].inputptr += a[1].inputstride;				\
      }									\
      									\
      if (NR > 2) {							\
	 F2( &a[2], v + a[2].vertoffset, (float *)a[2].inputptr );	\
	 a[2].inputptr += a[2].inputstride;				\
      }									\
      									\
      if (NR > 3) {							\
	 F3( &a[3], v + a[3].vertoffset, (float *)a[3].inputptr );	\
	 a[3].inputptr += a[3].inputstride;				\
      }									\
									\
      if (NR > 4) {							\
	 F4( &a[4], v + a[4].vertoffset, (float *)a[4].inputptr );	\
	 a[4].inputptr += a[4].inputstride;				\
      }									\
   }									\
}

   
#define EMIT2(F0, F1, NAME) EMIT5(2, F0, F1, insert_null, \
				  insert_null, insert_null, NAME)

#define EMIT3(F0, F1, F2, NAME) EMIT5(3, F0, F1, F2, insert_null, \
				      insert_null, NAME)
   
#define EMIT4(F0, F1, F2, F3, NAME) EMIT5(4, F0, F1, F2, F3, \
				          insert_null, NAME)
   

EMIT2(insert_3f_3, insert_4ub_4f_rgba_4, emit_xyz3_rgba4)

EMIT3(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, emit_xyzw4_rgba4_st2)

EMIT4(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, insert_2f_2, emit_xyzw4_rgba4_st2_st2)


/* Use the codegen paths to select one of a number of hardwired
 * fastpaths.
 */
void draw_vf_generate_hardwired_emit( struct draw_vertex_fetch *vf )
{
   draw_vf_emit_func func = NULL;

   /* Does it fit a hardwired fastpath?  Help! this is growing out of
    * control!
    */
   switch (vf->attr_count) {
   case 2:
      if (vf->attr[0].do_insert == insert_3f_3 &&
	  vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
 	 func = emit_xyz3_rgba4; 
      }
      break;
   case 3:
      if (vf->attr[2].do_insert == insert_2f_2) {
	 if (vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
	    if (vf->attr[0].do_insert == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2;
	 }
      }
      break;
   case 4:
      if (vf->attr[2].do_insert == insert_2f_2 &&
	  vf->attr[3].do_insert == insert_2f_2) {
	 if (vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
	    if (vf->attr[0].do_insert == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2_st2;
	 }
      }
      break;
   }

   vf->emit = func;
}

/***********************************************************************
 * Generic (non-codegen) functions for whole vertices or groups of
 * vertices
 */

void draw_vf_generic_emit( struct draw_vertex_fetch *vf,
		      unsigned count,
		      uint8_t *v )
{
   struct draw_vf_attr *a = vf->attr;
   const unsigned attr_count = vf->attr_count;
   const unsigned stride = vf->vertex_stride;
   unsigned i, j;

   for (i = 0 ; i < count ; i++, v += stride) {
      for (j = 0; j < attr_count; j++) {
	 float *in = (float *)a[j].inputptr;
	 a[j].inputptr += a[j].inputstride;
	 a[j].do_insert( &a[j], v + a[j].vertoffset, in );
      }
   }
}


