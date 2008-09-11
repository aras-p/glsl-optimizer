
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

#include "main/glheader.h"
#include "main/context.h"
#include "main/colormac.h"
#include "main/simple_list.h"

#include "vf/vf.h"


/*
 * These functions take the NDC coordinates pointed to by 'in', apply the
 * NDC->Viewport mapping and store the results at 'v'.
 */

static INLINE void insert_4f_viewport_4( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
   out[2] = scale[2] * in[2] + trans[2];
   out[3] = in[3];
}

static INLINE void insert_4f_viewport_3( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
   out[2] = scale[2] * in[2] + trans[2];
   out[3] = 1;
}

static INLINE void insert_4f_viewport_2( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
   out[2] =                    trans[2];
   out[3] = 1;
}

static INLINE void insert_4f_viewport_1( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] =                    trans[1];
   out[2] =                    trans[2];
   out[3] = 1;
}

static INLINE void insert_3f_viewport_3( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
   out[2] = scale[2] * in[2] + trans[2];
}

static INLINE void insert_3f_viewport_2( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
   out[2] = scale[2] * in[2] + trans[2];
}

static INLINE void insert_3f_viewport_1( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] =                    trans[1];
   out[2] =                    trans[2];
}

static INLINE void insert_2f_viewport_2( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = scale[1] * in[1] + trans[1];
}

static INLINE void insert_2f_viewport_1( const struct vf_attr *a, GLubyte *v,
					 const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = scale[0] * in[0] + trans[0];
   out[1] = trans[1];
}


/*
 * These functions do the same as above, except for the viewport mapping.
 */

static INLINE void insert_4f_4( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static INLINE void insert_4f_3( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}

static INLINE void insert_4f_2( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_4f_1( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static INLINE void insert_3f_xyw_4( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
}

static INLINE void insert_3f_xyw_err( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   (void) a; (void) v; (void) in;
   _mesa_exit(1);
}

static INLINE void insert_3f_3( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}

static INLINE void insert_3f_2( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
}

static INLINE void insert_3f_1( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
}


static INLINE void insert_2f_2( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
}

static INLINE void insert_2f_1( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
}

static INLINE void insert_1f_1( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   (void) a;

   out[0] = in[0];
}

static INLINE void insert_null( const struct vf_attr *a, GLubyte *v, const GLfloat *in )
{
   (void) a; (void) v; (void) in;
}

static INLINE void insert_4chan_4f_rgba_4( const struct vf_attr *a, GLubyte *v, 
					   const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[3], in[3]);
}

static INLINE void insert_4chan_4f_rgba_3( const struct vf_attr *a, GLubyte *v, 
					   const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[2], in[2]); 
   c[3] = CHAN_MAX;
}

static INLINE void insert_4chan_4f_rgba_2( const struct vf_attr *a, GLubyte *v, 
					   const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   UNCLAMPED_FLOAT_TO_CHAN(c[1], in[1]); 
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static INLINE void insert_4chan_4f_rgba_1( const struct vf_attr *a, GLubyte *v, 
					   const GLfloat *in )
{
   GLchan *c = (GLchan *)v;
   (void) a;
   UNCLAMPED_FLOAT_TO_CHAN(c[0], in[0]); 
   c[1] = 0;
   c[2] = 0;
   c[3] = CHAN_MAX;
}

static INLINE void insert_4ub_4f_rgba_4( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_rgba_3( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_2( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_rgba_1( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_4( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static INLINE void insert_4ub_4f_bgra_3( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_2( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_bgra_1( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
   v[3] = 0xff;
}

static INLINE void insert_4ub_4f_argb_4( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_argb_3( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_2( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_argb_1( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[0]);
   v[2] = 0x00;
   v[3] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_4( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[3]);
}

static INLINE void insert_4ub_4f_abgr_3( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[2]);
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_2( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[1]);
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_4ub_4f_abgr_1( const struct vf_attr *a, GLubyte *v, 
					 const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[0]);
   v[2] = 0x00;
   v[1] = 0x00;
   v[0] = 0xff;
}

static INLINE void insert_3ub_3f_rgb_3( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
}

static INLINE void insert_3ub_3f_rgb_2( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[2] = 0;
}

static INLINE void insert_3ub_3f_rgb_1( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   v[1] = 0;
   v[2] = 0;
}

static INLINE void insert_3ub_3f_bgr_3( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
}

static INLINE void insert_3ub_3f_bgr_2( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   v[0] = 0;
}

static INLINE void insert_3ub_3f_bgr_1( const struct vf_attr *a, GLubyte *v, 
					const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   v[1] = 0;
   v[0] = 0;
}


static INLINE void insert_1ub_1f_1( const struct vf_attr *a, GLubyte *v, 
				    const GLfloat *in )
{
   (void) a;
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
}


/***********************************************************************
 * Functions to perform the reverse operations to the above, for
 * swrast translation and clip-interpolation.
 * 
 * Currently always extracts a full 4 floats.
 */

static void extract_4f_viewport( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   /* Although included for completeness, the position coordinate is
    * usually handled differently during clipping.
    */
   out[0] = (in[0] - trans[0]) / scale[0];
   out[1] = (in[1] - trans[1]) / scale[1];
   out[2] = (in[2] - trans[2]) / scale[2];
   out[3] = in[3];
}

static void extract_3f_viewport( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = (in[0] - trans[0]) / scale[0];
   out[1] = (in[1] - trans[1]) / scale[1];
   out[2] = (in[2] - trans[2]) / scale[2];
   out[3] = 1;
}


static void extract_2f_viewport( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat *scale = a->vf->vp;
   const GLfloat *trans = a->vf->vp + 4;
   
   out[0] = (in[0] - trans[0]) / scale[0];
   out[1] = (in[1] - trans[1]) / scale[1];
   out[2] = 0;
   out[3] = 1;
}


static void extract_4f( const struct vf_attr *a, GLfloat *out, const GLubyte *v  )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void extract_3f_xyw( const struct vf_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = in[2];
}


static void extract_3f( const struct vf_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}


static void extract_2f( const struct vf_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static void extract_1f( const struct vf_attr *a, GLfloat *out, const GLubyte *v )
{
   const GLfloat *in = (const GLfloat *)v;
   (void) a;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static void extract_4chan_4f_rgba( const struct vf_attr *a, GLfloat *out, 
				   const GLubyte *v )
{
   GLchan *c = (GLchan *)v;
   (void) a;

   out[0] = CHAN_TO_FLOAT(c[0]);
   out[1] = CHAN_TO_FLOAT(c[1]);
   out[2] = CHAN_TO_FLOAT(c[2]);
   out[3] = CHAN_TO_FLOAT(c[3]);
}

static void extract_4ub_4f_rgba( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_bgra( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_argb( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[3] = UBYTE_TO_FLOAT(v[0]);
   out[0] = UBYTE_TO_FLOAT(v[1]);
   out[1] = UBYTE_TO_FLOAT(v[2]);
   out[2] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_4ub_4f_abgr( const struct vf_attr *a, GLfloat *out, 
				 const GLubyte *v )
{
   (void) a;
   out[3] = UBYTE_TO_FLOAT(v[0]);
   out[2] = UBYTE_TO_FLOAT(v[1]);
   out[1] = UBYTE_TO_FLOAT(v[2]);
   out[0] = UBYTE_TO_FLOAT(v[3]);
}

static void extract_3ub_3f_rgb( const struct vf_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[2] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_3ub_3f_bgr( const struct vf_attr *a, GLfloat *out, 
				const GLubyte *v )
{
   (void) a;
   out[2] = UBYTE_TO_FLOAT(v[0]);
   out[1] = UBYTE_TO_FLOAT(v[1]);
   out[0] = UBYTE_TO_FLOAT(v[2]);
   out[3] = 1;
}

static void extract_1ub_1f( const struct vf_attr *a, GLfloat *out, const GLubyte *v )
{
   (void) a;
   out[0] = UBYTE_TO_FLOAT(v[0]);
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}


const struct vf_format_info vf_format_info[EMIT_MAX] = 
{
   { "1f",
     extract_1f,
     { insert_1f_1, insert_1f_1, insert_1f_1, insert_1f_1 },
     sizeof(GLfloat) },

   { "2f",
     extract_2f,
     { insert_2f_1, insert_2f_2, insert_2f_2, insert_2f_2 },
     2 * sizeof(GLfloat) },

   { "3f",
     extract_3f,
     { insert_3f_1, insert_3f_2, insert_3f_3, insert_3f_3 },
     3 * sizeof(GLfloat) },

   { "4f",
     extract_4f,
     { insert_4f_1, insert_4f_2, insert_4f_3, insert_4f_4 },
     4 * sizeof(GLfloat) },

   { "2f_viewport",
     extract_2f_viewport,
     { insert_2f_viewport_1, insert_2f_viewport_2, insert_2f_viewport_2,
       insert_2f_viewport_2 },
     2 * sizeof(GLfloat) },

   { "3f_viewport",
     extract_3f_viewport,
     { insert_3f_viewport_1, insert_3f_viewport_2, insert_3f_viewport_3,
       insert_3f_viewport_3 },
     3 * sizeof(GLfloat) },

   { "4f_viewport",
     extract_4f_viewport,
     { insert_4f_viewport_1, insert_4f_viewport_2, insert_4f_viewport_3,
       insert_4f_viewport_4 }, 
     4 * sizeof(GLfloat) },

   { "3f_xyw",
     extract_3f_xyw,
     { insert_3f_xyw_err, insert_3f_xyw_err, insert_3f_xyw_err, 
       insert_3f_xyw_4 },
     3 * sizeof(GLfloat) },

   { "1ub_1f",
     extract_1ub_1f,
     { insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1, insert_1ub_1f_1 },
     sizeof(GLubyte) },

   { "3ub_3f_rgb",
     extract_3ub_3f_rgb,
     { insert_3ub_3f_rgb_1, insert_3ub_3f_rgb_2, insert_3ub_3f_rgb_3,
       insert_3ub_3f_rgb_3 },
     3 * sizeof(GLubyte) },

   { "3ub_3f_bgr",
     extract_3ub_3f_bgr,
     { insert_3ub_3f_bgr_1, insert_3ub_3f_bgr_2, insert_3ub_3f_bgr_3,
       insert_3ub_3f_bgr_3 },
     3 * sizeof(GLubyte) },

   { "4ub_4f_rgba",
     extract_4ub_4f_rgba,
     { insert_4ub_4f_rgba_1, insert_4ub_4f_rgba_2, insert_4ub_4f_rgba_3, 
       insert_4ub_4f_rgba_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_bgra",
     extract_4ub_4f_bgra,
     { insert_4ub_4f_bgra_1, insert_4ub_4f_bgra_2, insert_4ub_4f_bgra_3,
       insert_4ub_4f_bgra_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_argb",
     extract_4ub_4f_argb,
     { insert_4ub_4f_argb_1, insert_4ub_4f_argb_2, insert_4ub_4f_argb_3,
       insert_4ub_4f_argb_4 },
     4 * sizeof(GLubyte) },

   { "4ub_4f_abgr",
     extract_4ub_4f_abgr,
     { insert_4ub_4f_abgr_1, insert_4ub_4f_abgr_2, insert_4ub_4f_abgr_3,
       insert_4ub_4f_abgr_4 },
     4 * sizeof(GLubyte) },

   { "4chan_4f_rgba",
     extract_4chan_4f_rgba,
     { insert_4chan_4f_rgba_1, insert_4chan_4f_rgba_2, insert_4chan_4f_rgba_3,
       insert_4chan_4f_rgba_4 },
     4 * sizeof(GLchan) },

   { "pad",
     NULL,
     { NULL, NULL, NULL, NULL },
     0 }

};



    
/***********************************************************************
 * Hardwired fastpaths for emitting whole vertices or groups of
 * vertices
 */
#define EMIT5(NR, F0, F1, F2, F3, F4, NAME)				\
static void NAME( struct vertex_fetch *vf,				\
		  GLuint count,						\
		  GLubyte *v )						\
{									\
   struct vf_attr *a = vf->attr;				\
   GLuint i;								\
									\
   for (i = 0 ; i < count ; i++, v += vf->vertex_stride) {		\
      if (NR > 0) {							\
	 F0( &a[0], v + a[0].vertoffset, (GLfloat *)a[0].inputptr );	\
	 a[0].inputptr += a[0].inputstride;				\
      }									\
      									\
      if (NR > 1) {							\
	 F1( &a[1], v + a[1].vertoffset, (GLfloat *)a[1].inputptr );	\
	 a[1].inputptr += a[1].inputstride;				\
      }									\
      									\
      if (NR > 2) {							\
	 F2( &a[2], v + a[2].vertoffset, (GLfloat *)a[2].inputptr );	\
	 a[2].inputptr += a[2].inputstride;				\
      }									\
      									\
      if (NR > 3) {							\
	 F3( &a[3], v + a[3].vertoffset, (GLfloat *)a[3].inputptr );	\
	 a[3].inputptr += a[3].inputstride;				\
      }									\
									\
      if (NR > 4) {							\
	 F4( &a[4], v + a[4].vertoffset, (GLfloat *)a[4].inputptr );	\
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
   

EMIT2(insert_3f_viewport_3, insert_4ub_4f_rgba_4, emit_viewport3_rgba4)
EMIT2(insert_3f_viewport_3, insert_4ub_4f_bgra_4, emit_viewport3_bgra4)
EMIT2(insert_3f_3, insert_4ub_4f_rgba_4, emit_xyz3_rgba4)

EMIT3(insert_4f_viewport_4, insert_4ub_4f_rgba_4, insert_2f_2, emit_viewport4_rgba4_st2)
EMIT3(insert_4f_viewport_4, insert_4ub_4f_bgra_4, insert_2f_2,  emit_viewport4_bgra4_st2)
EMIT3(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, emit_xyzw4_rgba4_st2)

EMIT4(insert_4f_viewport_4, insert_4ub_4f_rgba_4, insert_2f_2, insert_2f_2, emit_viewport4_rgba4_st2_st2)
EMIT4(insert_4f_viewport_4, insert_4ub_4f_bgra_4, insert_2f_2, insert_2f_2,  emit_viewport4_bgra4_st2_st2)
EMIT4(insert_4f_4, insert_4ub_4f_rgba_4, insert_2f_2, insert_2f_2, emit_xyzw4_rgba4_st2_st2)


/* Use the codegen paths to select one of a number of hardwired
 * fastpaths.
 */
void vf_generate_hardwired_emit( struct vertex_fetch *vf )
{
   vf_emit_func func = NULL;

   /* Does it fit a hardwired fastpath?  Help! this is growing out of
    * control!
    */
   switch (vf->attr_count) {
   case 2:
      if (vf->attr[0].do_insert == insert_3f_viewport_3) {
	 if (vf->attr[1].do_insert == insert_4ub_4f_bgra_4) 
	    func = emit_viewport3_bgra4;
	 else if (vf->attr[1].do_insert == insert_4ub_4f_rgba_4) 
	    func = emit_viewport3_rgba4;
      }
      else if (vf->attr[0].do_insert == insert_3f_3 &&
	       vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
 	 func = emit_xyz3_rgba4; 
      }
      break;
   case 3:
      if (vf->attr[2].do_insert == insert_2f_2) {
	 if (vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
	    if (vf->attr[0].do_insert == insert_4f_viewport_4)
	       func = emit_viewport4_rgba4_st2;
	    else if (vf->attr[0].do_insert == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2;
	 }
	 else if (vf->attr[1].do_insert == insert_4ub_4f_bgra_4 &&
		  vf->attr[0].do_insert == insert_4f_viewport_4)
	    func = emit_viewport4_bgra4_st2;
      }
      break;
   case 4:
      if (vf->attr[2].do_insert == insert_2f_2 &&
	  vf->attr[3].do_insert == insert_2f_2) {
	 if (vf->attr[1].do_insert == insert_4ub_4f_rgba_4) {
	    if (vf->attr[0].do_insert == insert_4f_viewport_4)
	       func = emit_viewport4_rgba4_st2_st2;
	    else if (vf->attr[0].do_insert == insert_4f_4) 
	       func = emit_xyzw4_rgba4_st2_st2;
	 }
	 else if (vf->attr[1].do_insert == insert_4ub_4f_bgra_4 &&
		  vf->attr[0].do_insert == insert_4f_viewport_4)
	    func = emit_viewport4_bgra4_st2_st2;
      }
      break;
   }

   vf->emit = func;
}

/***********************************************************************
 * Generic (non-codegen) functions for whole vertices or groups of
 * vertices
 */

void vf_generic_emit( struct vertex_fetch *vf,
		      GLuint count,
		      GLubyte *v )
{
   struct vf_attr *a = vf->attr;
   const GLuint attr_count = vf->attr_count;
   const GLuint stride = vf->vertex_stride;
   GLuint i, j;

   for (i = 0 ; i < count ; i++, v += stride) {
      for (j = 0; j < attr_count; j++) {
	 GLfloat *in = (GLfloat *)a[j].inputptr;
	 a[j].inputptr += a[j].inputstride;
	 a[j].do_insert( &a[j], v + a[j].vertoffset, in );
      }
   }
}


