#include "types.h"
#include "mem.h"

#include "t_context.h"
#include "t_clip.h"
#include "t_cva.h"
#include "t_dlist.h"
#include "t_eval.h"
#include "t_pipeline.h"
#include "t_shade.h"
#include "t_light.h"
#include "t_texture.h"
#include "t_stages.h"
#include "t_varray.h"
#include "t_vb.h"
#include "t_vbrender.h"
#include "t_vbxform.h"
#include "tnl.h"

#if !defined(THREADS)
struct immediate *_mesa_CurrentInput = NULL;
#endif


GLboolean 
_tnl_flush_vertices( GLcontext *ctx, GLuint flush_flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct immediate *IM = TNL_CURRENT_IM(ctx);

   if ((IM->Flag[IM->Count] & (VERT_BEGIN|VERT_END)) != VERT_END ||
       (flush_flags & (FLUSH_STORED_VERTICES|FLUSH_UPDATE_CURRENT))) 
   {
      if (IM->Flag[IM->Start]) 
	 _mesa_flush_vb( ctx );
      
      /* Although this code updates the ctx->Current values, that bit
       * is left set as there is no easy mechanism to set it
       * elsewhere.  This means that each time core wants to examine
       * ctx->Current, this function will be called.  After the first
       * time, however, it will be a no-op.
       */
      ctx->Driver.NeedFlush &= ~(FLUSH_STORED_VERTICES |
				 FLUSH_INSIDE_BEGIN_END);

      return (tnl->_CurrentPrimitive == GL_POLYGON+1);
   }
   else
      return GL_TRUE;
}


GLboolean
_tnl_CreateContext( GLcontext *ctx )
{
   TNLcontext *tnl;
   static int firsttime = 1;

   /* Onetime initializations.  Doesn't really matter if this gets
    * done twice: no need for mutexes. 
    */
   if (firsttime) {
      firsttime = 0;
      _tnl_clip_init( );
      _tnl_eval_init( );
      _tnl_shade_init( );
      _tnl_texture_init( );
      _tnl_trans_elt_init( );
      _tnl_vbrender_init( );
      _tnl_stages_init( );
   }

   /* Create the TNLcontext structure
    */
   ctx->swtnl_context = tnl = CALLOC( sizeof(TNLcontext) );
   if (!tnl) {
      return GL_FALSE;
   }

   /* Create and hook in the data structures available from ctx.
    */
   ctx->swtnl_vb = (void *)gl_vb_create_for_immediate( ctx );
   if (!ctx->swtnl_vb) {
      FREE(tnl);
      ctx->swtnl_context = 0;
      return GL_FALSE;
   }

   ctx->swtnl_im = (void *)TNL_VB(ctx)->IM;

   
   /* Initialize tnl state.
    */
   _tnl_dlist_init( ctx );
   _tnl_pipeline_init( ctx );

   tnl->_CurrentFlag = (VERT_NORM |
                        VERT_INDEX |
                        VERT_RGBA |
                        VERT_SPEC_RGB |
                        VERT_FOG_COORD |
                        VERT_EDGE |
                        VERT_TEX0_12 |
                        VERT_TEX1_12 |
                        VERT_TEX2_12 |
                        VERT_TEX3_12 |
                        VERT_MATERIAL);

   tnl->_CurrentPrimitive = GL_POLYGON+1;

   gl_reset_vb( TNL_VB(ctx) );
   gl_reset_input( ctx );


   /* Set a few default values in the driver struct.  This is a
    * temporary mechanism.  
    */
   ctx->Driver.RenderVBCulledTab = _tnl_render_tab_cull;
   ctx->Driver.RenderVBClippedTab = _tnl_render_tab_clipped;
   ctx->Driver.RenderVBRawTab = _tnl_render_tab_raw;
   ctx->Driver.NewList = _tnl_NewList;
   ctx->Driver.EndList = _tnl_EndList;
   ctx->Driver.FlushVertices = _tnl_flush_vertices;
   ctx->Driver.NeedFlush = FLUSH_UPDATE_CURRENT;
   ctx->Driver.LightingSpaceChange = _tnl_LightingSpaceChange;
   ctx->Driver.MakeCurrent = _tnl_MakeCurrent;
   ctx->Driver.VertexPointer = _tnl_VertexPointer;
   ctx->Driver.NormalPointer = _tnl_NormalPointer;
   ctx->Driver.ColorPointer = _tnl_ColorPointer;
   ctx->Driver.FogCoordPointer = _tnl_FogCoordPointer;
   ctx->Driver.IndexPointer = _tnl_IndexPointer;
   ctx->Driver.SecondaryColorPointer = _tnl_SecondaryColorPointer;
   ctx->Driver.TexCoordPointer = _tnl_TexCoordPointer;
   ctx->Driver.EdgeFlagPointer = _tnl_EdgeFlagPointer;

   return GL_TRUE;
}


void
_tnl_DestroyContext( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (TNL_CURRENT_IM(ctx) != TNL_VB(ctx)->IM)
      gl_immediate_free( TNL_CURRENT_IM(ctx) );

   gl_vb_free( TNL_VB(ctx) );

   /* Free cache of immediate buffers. */
   while (tnl->nr_im_queued-- > 0) {
      struct immediate * next = tnl->freed_im_queue->next;
      ALIGN_FREE( tnl->freed_im_queue );
      tnl->freed_im_queue = next;
   }
}


/* Update all state that references _NeedEyeCoords 
 */
void
_tnl_LightingSpaceChange( GLcontext *ctx )
{
   _tnl_update_normal_transform( ctx ); 
}


void 
_tnl_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   if (new_state & _NEW_LIGHT)
      gl_update_lighting_function(ctx);

   if (new_state & _NEW_ARRAY)
      gl_update_client_state( ctx );

   if (new_state & _NEW_TEXTURE)
      if (ctx->_Enabled & ENABLE_TEXGEN_ANY)
	 _tnl_update_texgen( ctx );
      
   if (new_state & (_NEW_LIGHT|_NEW_TEXTURE|_NEW_FOG|
		    _DD_NEW_TRI_LIGHT_TWOSIDE |
		    _DD_NEW_SEPERATE_SPECULAR |
		    _DD_NEW_TRI_UNFILLED ))
      gl_update_clipmask(ctx);

   if (new_state & _TNL_NEW_NORMAL_TRANSFORM)
      _tnl_update_normal_transform( ctx );

   gl_update_pipelines(ctx);
}

void
_tnl_MakeCurrent( GLcontext *ctx, 
		  GLframebuffer *drawBuffer, 
		  GLframebuffer *readBuffer )
{
#ifndef THREADS
   SET_IMMEDIATE(newCtx, newCtx->input);
#endif
}
