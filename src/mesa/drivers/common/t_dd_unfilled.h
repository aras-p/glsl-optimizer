
static void tdfx_unfilled_tri( GLcontext *ctx, 
			       GLenum mode,
			       GLuint e0, GLuint e1, GLuint e2 )
{
   tdfxContextPtr imesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ef = VB->EdgeFlag;
   GLubyte *tdfxverts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   tdfxVertex *v0 = (tdfxVertex *)(tdfxverts + (e0 << shift));
   tdfxVertex *v1 = (tdfxVertex *)(tdfxverts + (e1 << shift));
   tdfxVertex *v2 = (tdfxVertex *)(tdfxverts + (e2 << shift));
   GLuint c[2];
   GLuint s[2];
   GLuint coloroffset = 0;

/*     fprintf(stderr, "%s %s %d %d %d (vertsize %d)\n", __FUNCTION__,  */
/*  	   gl_lookup_enum_by_nr(mode), e0, e1, e2, imesa->vertsize); */

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      coloroffset = (imesa->vertsize == 4) ? 3 : 4;
      TDFX_COPY_COLOR(c[0], v0->ui[coloroffset]);
      TDFX_COPY_COLOR(c[1], v1->ui[coloroffset]);
      TDFX_COPY_COLOR(v0->ui[coloroffset], v2->ui[coloroffset]);
      TDFX_COPY_COLOR(v1->ui[coloroffset], v2->ui[coloroffset]);

      if (coloroffset == 4) {
	 TDFX_COPY_COLOR(s[0], v0->v.specular);
	 TDFX_COPY_COLOR(s[1], v1->v.specular);
	 TDFX_COPY_COLOR(v0->v.specular, v2->v.specular);
	 TDFX_COPY_COLOR(v1->v.specular, v2->v.specular);
      }
   }

   if (mode == GL_POINT) {
      tdfxRasterPrimitive( ctx, GL_POINTS, PR_LINES );
      if (ef[e0]) imesa->draw_point( imesa, v0 ); 
      if (ef[e1]) imesa->draw_point( imesa, v1 ); 
      if (ef[e2]) imesa->draw_point( imesa, v2 ); 
   }
   else {
      tdfxRasterPrimitive( ctx, GL_LINES, PR_LINES );

      if (imesa->render_primitive == GL_POLYGON) {
	 if (ef[e2]) imesa->draw_line( imesa, v2, v0 ); 
	 if (ef[e0]) imesa->draw_line( imesa, v0, v1 ); 
	 if (ef[e1]) imesa->draw_line( imesa, v1, v2 ); 
      } 
      else {
	 if (ef[e0]) imesa->draw_line( imesa, v0, v1 ); 
	 if (ef[e1]) imesa->draw_line( imesa, v1, v2 ); 
	 if (ef[e2]) imesa->draw_line( imesa, v2, v0 ); 
      }
   }

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      TDFX_COPY_COLOR(v0->ui[coloroffset], c[0]);
      TDFX_COPY_COLOR(v1->ui[coloroffset], c[1]);
      if (coloroffset == 4) {
	 TDFX_COPY_COLOR(v0->v.specular, s[0]);
	 TDFX_COPY_COLOR(v1->v.specular, s[1]);
      }
   }
}


static void tdfx_unfilled_quad( GLcontext *ctx, 
				GLenum mode,
				GLuint e0, GLuint e1, 
				GLuint e2, GLuint e3 )
{
   tdfxContextPtr imesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ef = VB->EdgeFlag;
   GLubyte *tdfxverts = (GLubyte *)imesa->verts;
   GLuint shift = imesa->vertex_stride_shift;
   tdfxVertex *v0 = (tdfxVertex *)(tdfxverts + (e0 << shift));
   tdfxVertex *v1 = (tdfxVertex *)(tdfxverts + (e1 << shift));
   tdfxVertex *v2 = (tdfxVertex *)(tdfxverts + (e2 << shift));
   tdfxVertex *v3 = (tdfxVertex *)(tdfxverts + (e3 << shift));
   GLuint c[3];
   GLuint s[3];
   GLuint coloroffset = 0;

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      coloroffset = (imesa->vertsize == 4) ? 3 : 4;

      TDFX_COPY_COLOR(c[0], v0->ui[coloroffset]);
      TDFX_COPY_COLOR(c[1], v1->ui[coloroffset]);
      TDFX_COPY_COLOR(c[2], v2->ui[coloroffset]);
      TDFX_COPY_COLOR(v0->ui[coloroffset], v3->ui[coloroffset]);
      TDFX_COPY_COLOR(v1->ui[coloroffset], v3->ui[coloroffset]);
      TDFX_COPY_COLOR(v2->ui[coloroffset], v3->ui[coloroffset]);
      
      if (coloroffset == 4) {
	 TDFX_COPY_COLOR(s[0], v0->v.specular);
	 TDFX_COPY_COLOR(s[1], v1->v.specular);
	 TDFX_COPY_COLOR(s[2], v2->v.specular);
	 TDFX_COPY_COLOR(v0->v.specular, v3->v.specular);
	 TDFX_COPY_COLOR(v1->v.specular, v3->v.specular);
	 TDFX_COPY_COLOR(v2->v.specular, v3->v.specular);
      }
   }

   if (mode == GL_POINT) {
      if (imesa->reduced_primitive != GL_POINTS)
	 tdfxRasterPrimitive( ctx, GL_POINTS, PR_LINES );

      if (ef[e0]) imesa->draw_point( imesa, v0 ); 
      if (ef[e1]) imesa->draw_point( imesa, v1 ); 
      if (ef[e2]) imesa->draw_point( imesa, v2 ); 
      if (ef[e3]) imesa->draw_point( imesa, v3 ); 
   }
   else {
      if (imesa->reduced_primitive != GL_LINES)
	 tdfxRasterPrimitive( ctx, GL_LINES, PR_LINES );

      if (ef[e0]) imesa->draw_line( imesa, v0, v1 ); 
      if (ef[e1]) imesa->draw_line( imesa, v1, v2 ); 
      if (ef[e2]) imesa->draw_line( imesa, v2, v3 ); 
      if (ef[e3]) imesa->draw_line( imesa, v3, v0 ); 
   }

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      TDFX_COPY_COLOR(v0->ui[coloroffset], c[0]);
      TDFX_COPY_COLOR(v1->ui[coloroffset], c[1]);
      TDFX_COPY_COLOR(v2->ui[coloroffset], c[2]);
      if (coloroffset == 4) {
	 TDFX_COPY_COLOR(v0->v.specular, s[0]);
	 TDFX_COPY_COLOR(v1->v.specular, s[1]);
	 TDFX_COPY_COLOR(v2->v.specular, s[2]);
      }
   }
}
