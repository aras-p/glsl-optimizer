
/* XXX: could split the primitive list to fallback only on the
 * non-conformant primitives.
 */
static GLboolean check_fallbacks( struct brw_context *brw,
				  const struct _mesa_prim *prim,
				  GLuint nr_prims )
{
   GLuint i;

   /* If we don't require strict OpenGL conformance, never 
    * use fallbacks.  If we're forcing fallbacks, always
    * use fallfacks.
    */
   if (brw->flags.no_swtnl)
      return GL_FALSE;

   if (brw->flags.force_swtnl)
      return GL_TRUE;

   if (brw->curr.rast->tmpl.smooth_polys) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_TRIANGLES) 
	    return GL_TRUE;
   }

   /* BRW hardware will do AA lines, but they are non-conformant it
    * seems.  TBD whether we keep this fallback:
    */
   if (ctx->Line.SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_LINES) 
	    return GL_TRUE;
   }

   /* Stipple -- these fallbacks could be resolved with a little
    * bit of work?
    */
   if (ctx->Line.StippleFlag) {
      for (i = 0; i < nr_prims; i++) {
	 /* GS doesn't get enough information to know when to reset
	  * the stipple counter?!?
	  */
	 if (prim[i].mode == GL_LINE_LOOP || prim[i].mode == GL_LINE_STRIP) 
	    return GL_TRUE;
	    
	 if (prim[i].mode == GL_POLYGON &&
	     (ctx->Polygon.FrontMode == GL_LINE ||
	      ctx->Polygon.BackMode == GL_LINE))
	    return GL_TRUE;
      }
   }

   if (ctx->Point.SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (prim[i].mode == GL_POINTS) 
	    return GL_TRUE;
   }

   /* BRW hardware doesn't handle GL_CLAMP texturing correctly;
    * brw_wm_sampler_state:translate_wrap_mode() treats GL_CLAMP
    * as GL_CLAMP_TO_EDGE instead.  If we're using GL_CLAMP, and
    * we want strict conformance, force the fallback.
    * Right now, we only do this for 2D textures.
    */
   {
      int u;
      for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
         struct gl_texture_unit *texUnit = &ctx->Texture.Unit[u];
         if (texUnit->Enabled) {
            if (texUnit->Enabled & TEXTURE_1D_BIT) {
               if (texUnit->CurrentTex[TEXTURE_1D_INDEX]->WrapS == GL_CLAMP) {
                   return GL_TRUE;
               }
            }
            if (texUnit->Enabled & TEXTURE_2D_BIT) {
               if (texUnit->CurrentTex[TEXTURE_2D_INDEX]->WrapS == GL_CLAMP ||
                   texUnit->CurrentTex[TEXTURE_2D_INDEX]->WrapT == GL_CLAMP) {
                   return GL_TRUE;
               }
            }
            if (texUnit->Enabled & TEXTURE_3D_BIT) {
               if (texUnit->CurrentTex[TEXTURE_3D_INDEX]->WrapS == GL_CLAMP ||
                   texUnit->CurrentTex[TEXTURE_3D_INDEX]->WrapT == GL_CLAMP ||
                   texUnit->CurrentTex[TEXTURE_3D_INDEX]->WrapR == GL_CLAMP) {
                   return GL_TRUE;
               }
            }
         }
      }
   }

   /* Exceeding hw limits on number of VS inputs?
    */
   if (brw->nr_ve == 0 ||
       brw->nr_ve >= BRW_VEP_MAX) {
      return TRUE;
   }

   /* Position array with zero stride?
    */
   if (brw->vs[brw->ve[0]]->stride == 0)
      return TRUE;


      
   /* Nothing stopping us from the fast path now */
   return GL_FALSE;
}




