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






struct attr {
   int attrib;
   int vertoffset;
   int vertattrsize;
   int *inputptr;
   int inputstride;
   void (*insert)( const struct attr *a, char *v, const GLfloat *input );
   void (*extract)( const struct attr *a, GLfloat *output, const char *v );
   const GLfloat *vp;
};


static void insert_4f_viewport( const struct attr *a, char *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
   out[3] = in[3];
}

static void insert_3f_viewport( const struct attr *a, char *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
   out[2] = vp[10] * in[2] + vp[14];
}

static void insert_2f_viewport( const struct attr *a, char *v,
				const GLfloat *in )
{
   GLfloat *out = (GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = vp[0] * in[0] + vp[12];
   out[1] = vp[5] * in[1] + vp[13];
}


static void insert_4f( const struct attr *a, char *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void insert_3f_xyw( const struct attr *a, char *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
}


static void insert_3f( const struct attr *a, char *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}


static void insert_2f( const struct attr *a, char *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
   out[1] = in[1];
}

static void insert_1f( const struct attr *a, char *v, const GLfloat *in )
{
   GLfloat *out = (GLfloat *)(v);
   
   out[0] = in[0];
}

static void insert_4ub_4f_rgba( const struct attr *a, char *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void insert_4ub_4f_bgra( const struct attr *a, char *v, 
				const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void insert_3ub_3f_rgb( const struct attr *a, char *v, 
			       const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
}

static void insert_3ub_3f_bgr( const struct attr *a, char *v, 
			       const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
}

static void insert_1ub_1f( const struct attr *a, char *v, 
			   const GLfloat *in )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
}


/***********************************************************************
 * Functions to perform the reverse operations to the above, for
 * swrast translation and clip-interpolation.
 * 
 * Currently always extracts a full 4 floats.
 */

static void extract_4f_viewport( const struct attr *a, GLfloat *out, 
				 const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = (in[2] - vp[14]) / vp[10];
   out[3] = in[3];
}

static void extract_3f_viewport( const struct attr *a, GLfloat *out, 
				 const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = (in[2] - vp[14]) / vp[10];
   out[3] = 1;
}


static void extract_2f_viewport( const struct attr *a, GLfloat *out, 
				 const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   const GLfloat * const vp = a->vp;
   
   out[0] = (in[0] - vp[12]) / vp[0];
   out[1] = (in[1] - vp[13]) / vp[5];
   out[2] = 0;
   out[3] = 1;
}


static void extract_4f( const struct attr *a, GLfloat *out, const char *v  )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = in[3];
}

static void extract_3f_xyw( const struct attr *a, GLfloat *out, const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[3];
   out[3] = 1;
}


static void extract_3f( const struct attr *a, GLfloat *out, const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
   out[3] = 1;
}


static void extract_2f( const struct attr *a, GLfloat *out, const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = in[1];
   out[2] = 0;
   out[3] = 1;
}

static void extract_1f( const struct attr *a, GLfloat *out, const char *v )
{
   const GLfloat *in = (const GLfloat *)v;
   
   out[0] = in[0];
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

static void extract_4ub_4f_rgba( const struct attr *a, GLfloat *out, 
				 const char *v )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void extract_4ub_4f_bgra( const struct attr *a, GLfloat *out, 
				 const char *v )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], in[3]);
}

static void extract_3ub_3f_rgb( const struct attr *a, GLfloat *out, 
				const char *v )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[2]);
   out[3] = 1;
}

static void extract_3ub_3f_bgr( const struct attr *a, GLfloat *out, 
				const char *v )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], in[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], in[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[2]);
   out[3] = 1;
}

static void extract_1ub_1f( const struct attr *a, GLfloat *out, const char *v )
{
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], in[0]);
   out[1] = 0;
   out[2] = 0;
   out[3] = 1;
}

/***********************************************************************
 * Generic (non-codegen) functions for whole vertices or groups of
 * vertices
 */

static void generic_emit( GLcontext *ctx,
			  GLuint start, GLuint end,
			  void *dest,
			  GLuint stride )
{
   int i, j;
   char *v = (char *)dest;

   vtx->vertex_buf = v - start * stride;
   vtx->vertex_stride = stride;

   end -= start;

   for (j = 0; j < vtx->attr_count; j++) {
      GLvector4f *vptr = VB->AttrPtr[a[j].attrib];
      a[j].inputptr = STRIDE_4F(vptr->data, start * vptr->stride);
   }

   for (i = 0 ; i < end ; i++, v += stride) {
      for (j = 0; j < vtx->attr_count; j++) {
	 int *in = a[j].inputptr;
	 (char *)a[j].inputptr += a[j].inputstride;
	 a[j].out( &a[j], v + a[j].vertoffset, in );
      }
   }
}


static void generic_interp( GLcontext *ctx,
			    GLfloat t,
			    GLuint edst, GLuint eout, GLuint ein,
			    GLboolean force_boundary )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   char *vin  = vtx->vertex_buf + ein  * vtx->vertex_stride;
   char *vout = vtx->vertex_buf + eout * vtx->vertex_stride;
   char *vdst = vtx->vertex_buf + edst * vtx->vertex_stride;
   const struct attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;
   
   for (j = 0; j < attr_count; j++) {
      GLfloat fin[4], fout[4], fdst[4];
      
      a[j].extract( &a[j], vin, fin );
      a[j].extract( &a[j], vout, fout );

      INTERP_F( t, fdst[3], fout[3], fin[3] );
      INTERP_F( t, fdst[2], fout[2], fin[2] );
      INTERP_F( t, fdst[1], fout[1], fin[1] );
      INTERP_F( t, fdst[0], fout[0], fin[0] );

      a[j].insert( &a[j], vdst, fdst );
   }
}


/* Extract color attributes from one vertex and insert them into
 * another.  (Shortcircuit extract/insert with memcpy).
 */
static void generic_copy_colors( GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   char *vsrc = vert_start + esrc * vert_stride;
   char *vdst = vert_start + edst * vert_stride;
   const struct attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attribute == VERT_ATTRIB_COLOR0 ||
	  a[j].attribute == VERT_ATTRIB_COLOR1) {

	 memcpy( vdst + a[j].vertoffset,
		 vsrc + a[j].vertoffset,
		 a[j].vertattrsize );
   }
}

static void generic_get_attr( GLcontext *ctx, const char *vertex,
			      GLenum attr, GLfloat *dest )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   const struct attr *a = vtx->attr;
   int attr_count = vtx->attr_count;
   int j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attribute == attr) {
	 a[j].extract( &a[j], vin, dest );
	 return;
      }
   }

   /* Else return the value from ctx->Current
    */
   memcpy( dest, ctx->Current.Attrib[attr], 4*sizeof(GLfloat));
}







/***********************************************************************
 * Build codegen functions or return generic ones:
 */


static void choose_emit_func( GLcontext *ctx,
			      GLuint start, GLuint end,
			      void *dest,
			      GLuint stride )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->emit_func = generic_emit_func;
   vtx->emit_func( ctx, start, end, dest, stride );
}


static void choose_interp_func( GLcontext *ctx,
				GLfloat t,
				GLuint edst, GLuint eout, GLuint ein,
				GLboolean force_boundary )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->interp_func = generic_interp_func;
   vtx->interp_func( ctx, t, edst, eout, ein, force_boundary );
}


static void choose_copy_color_func(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->copy_color_func = generic_copy_color_func;
   vtx->copy_color_func( ctx, edst, esrc );
}


/***********************************************************************
 * Public entrypoints, mostly dispatch to the above:
 */

void _tnl_emit( GLcontext *ctx,
		GLuint start, GLuint end,
		void *dest,
		GLuint stride )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->emit_func( ctx, start, end, dest, stride );
}

/* Interpolate between two vertices to produce a third:
 */
void _tnl_interp( GLcontext *ctx,
		  GLfloat t,
		  GLuint edst, GLuint eout, GLuint ein,
		  GLboolean force_boundary )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->interp_func( ctx, t, edst, eout, ein, force_boundary );
}

/* Copy colors from one vertex to another:
 */
void _tnl_copy_colors(  GLcontext *ctx, GLuint edst, GLuint esrc )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->copy_color_func( ctx, edst, esrc );
}


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied:
 */
void _tnl_get_attr( GLcontext *ctx, void *vertex, GLenum attrib,
		    GLfloat *dest )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   vtx->get_attr_func( ctx, vertex, attrib, dest );
}



void _tnl_install_attrs( GLcontext *ctx, const struct dri_attr_map *map,
			 GLuint nr, const GLfloat *vp )
{
   struct dri_vertex_state *vtx = GET_VERTEX_STATE(ctx);
   int i;

   assert(nr < _TNL_ATTR_MAX);

   vtx->attr_count = nr;
   vtx->emit_func = choose_emit_func;
   vtx->interp_func = choose_interp_func;
   vtx->copy_color_func = choose_copy_color_func;
   vtx->get_attr_func = choose_get_attr_func;

   for (i = 0; i < nr; i++) {
      GLuint attrib = map[i].attrib;
      vtx->attr[i].attrib = map[i].attrib;
      vtx->attr[i].hw_format = map[i].hw_format;
      vtx->attr[i].vp = vp;
      vtx->attr[i].insert = attrib_info[attrib].insert;
      vtx->attr[i].extract = attrib_info[attrib].extract;
      vtx->attr[i].vertattrsize = attrib_info[attrib].attrsize;
      vtx->attr[i].vertoffset = offset;
      offset += attrib_info[attrib].attrsize;
   }
}




/* Populate a swrast SWvertex from an attrib-style vertex.
 */
void _tnl_translate( GLcontext *ctx, const void *vertex, SWvertex *dest )
{
   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_POS, dest.win );

   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _tnl_get_attr( ctx, vertex, VERT_ATTRIB_TEX(i), dest.texcoord[i] );
	  
   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_COLOR0, tmp );
   UNCLAMPED_FLOAT_TO_CHAN_RGBA( dest.color, tmp );

   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_COLOR1, tmp );
   UNCLAMPED_FLOAT_TO_CHAN_RGB( dest.specular, tmp );

   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_FOG, tmp );
   dest.fog = tmp[0];

   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_INDEX, tmp );
   dest.index = (GLuint) tmp[0];

   _tnl_get_attr( ctx, vertex, VERT_ATTRIB_POINTSIZE, tmp );
   dest.pointSize = tmp[0];
}


static void interp_extras( GLcontext *ctx,
			   GLfloat t,
			   GLuint dst, GLuint out, GLuint in,
			   GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      assert(VB->ColorPtr[1]->stride == 4 * sizeof(GLfloat));

      INTERP_4F( t,
		 GET_COLOR(VB->ColorPtr[1], dst),
		 GET_COLOR(VB->ColorPtr[1], out),
		 GET_COLOR(VB->ColorPtr[1], in) );

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3F( t,
		    GET_COLOR(VB->SecondaryColorPtr[1], dst),
		    GET_COLOR(VB->SecondaryColorPtr[1], out),
		    GET_COLOR(VB->SecondaryColorPtr[1], in) );
      }
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   generic_interp(ctx, t, dst, out, in, force_boundary);
}

static void copy_pv_extras( GLcontext *ctx, 
			    GLuint dst, GLuint src )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      COPY_4FV( GET_COLOR(VB->ColorPtr[1], dst), 
		GET_COLOR(VB->ColorPtr[1], src) );

      if (VB->SecondaryColorPtr[1]) {
	 COPY_4FV( GET_COLOR(VB->SecondaryColorPtr[1], dst), 
		   GET_COLOR(VB->SecondaryColorPtr[1], src) );
      }
   }

   generic_copy_colors(ctx, dst, src);
}

#endif
