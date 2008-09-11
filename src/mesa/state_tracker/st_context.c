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

#include "main/imports.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/matrix.h"
#include "main/buffers.h"
#include "main/scissor.h"
#include "vbo/vbo.h"
#include "shader/shader_api.h"
#include "glapi/glapi.h"
#include "st_public.h"
#include "st_context.h"
#include "st_cb_accum.h"
#include "st_cb_bitmap.h"
#include "st_cb_blit.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_clear.h"
#if FEATURE_drawpix
#include "st_cb_drawpixels.h"
#include "st_cb_rasterpos.h"
#endif
#ifdef FEATURE_OES_draw_texture
#include "st_cb_drawtex.h"
#endif
#include "st_cb_fbo.h"
#include "st_cb_get.h"
#if FEATURE_feedback
#include "st_cb_feedback.h"
#endif
#include "st_cb_program.h"
#include "st_cb_queryobj.h"
#include "st_cb_readpixels.h"
#include "st_cb_texture.h"
#include "st_cb_flush.h"
#include "st_cb_strings.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_extensions.h"
#include "st_gen_mipmap.h"
#include "st_program.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "draw/draw_context.h"
#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_context.h"


/**
 * Called via ctx->Driver.UpdateState()
 */
void st_invalidate_state(GLcontext * ctx, GLuint new_state)
{
   struct st_context *st = st_context(ctx);

   st->dirty.mesa |= new_state;
   st->dirty.st |= ST_NEW_MESA;

   /* This is the only core Mesa module we depend upon.
    * No longer use swrast, swsetup, tnl.
    */
   _vbo_InvalidateState(ctx, new_state);
}


static struct st_context *
st_create_context_priv( GLcontext *ctx, struct pipe_context *pipe )
{
   uint i;
   struct st_context *st = CALLOC_STRUCT( st_context );
   
   ctx->st = st;

   st->ctx = ctx;
   st->pipe = pipe;

   /* state tracker needs the VBO module */
   _vbo_CreateContext(ctx);

#if FEATURE_feedback || FEATURE_drawpix
   st->draw = draw_create(); /* for selection/feedback */

   /* Disable draw options that might convert points/lines to tris, etc.
    * as that would foul-up feedback/selection mode.
    */
   draw_wide_line_threshold(st->draw, 1000.0f);
   draw_wide_point_threshold(st->draw, 1000.0f);
   draw_enable_line_stipple(st->draw, FALSE);
   draw_enable_point_sprites(st->draw, FALSE);
#endif

   st->dirty.mesa = ~0;
   st->dirty.st = ~0;

   st->cso_context = cso_create_context(pipe);

   st_init_atoms( st );
   st_init_bitmap(st);
   st_init_clear(st);
   st_init_draw( st );
   st_init_generate_mipmap(st);
   st_init_blit(st);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      st->state.sampler_list[i] = &st->state.samplers[i];

   /* we want all vertex data to be placed in buffer objects */
   vbo_use_buffer_objects(ctx);

   /* Need these flags:
    */
   st->ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   st->ctx->FragmentProgram._UseTexEnvProgram = GL_TRUE;

   st->ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;

   st->pixel_xfer.cache = _mesa_new_program_cache();

   /* GL limits and extensions */
   st_init_limits(st);
   st_init_extensions(st);

   return st;
}


struct st_context *st_create_context(struct pipe_context *pipe,
                                     const __GLcontextModes *visual,
                                     struct st_context *share)
{
   GLcontext *ctx;
   GLcontext *shareCtx = share ? share->ctx : NULL;
   struct dd_function_table funcs;

   memset(&funcs, 0, sizeof(funcs));
   st_init_driver_functions(&funcs);

   ctx = _mesa_create_context(visual, shareCtx, &funcs, NULL);

   return st_create_context_priv(ctx, pipe);
}


static void st_destroy_context_priv( struct st_context *st )
{
   uint i;

#if FEATURE_feedback || FEATURE_drawpix
   draw_destroy(st->draw);
#endif
   st_destroy_atoms( st );
   st_destroy_draw( st );
   st_destroy_generate_mipmap(st);
#if FEATURE_EXT_framebuffer_blit
   st_destroy_blit(st);
#endif
   st_destroy_clear(st);
#if FEATURE_drawpix
   st_destroy_bitmap(st);
   st_destroy_drawpix(st);
#endif
#ifdef FEATURE_OES_draw_texture
   st_destroy_drawtex(st);
#endif

   _vbo_DestroyContext(st->ctx);

   for (i = 0; i < Elements(st->state.sampler_texture); i++) {
      pipe_texture_reference(&st->state.sampler_texture[i], NULL);
   }

   for (i = 0; i < Elements(st->state.constants); i++) {
      if (st->state.constants[i].buffer) {
         pipe_buffer_reference(st->pipe->screen, &st->state.constants[i].buffer, NULL);
      }
   }

   if (st->default_texture) {
      st->ctx->Driver.DeleteTexture(st->ctx, st->default_texture);
      st->default_texture = NULL;
   }

   free( st );
}

 
void st_destroy_context( struct st_context *st )
{
   struct pipe_context *pipe = st->pipe;
   struct cso_context *cso = st->cso_context;
   GLcontext *ctx = st->ctx;

   /* need to unbind and destroy CSO objects before anything else */
   cso_release_all(st->cso_context);

   st_reference_fragprog(st, &st->fp, NULL);
   st_reference_vertprog(st, &st->vp, NULL);

   _mesa_delete_program_cache(st->ctx, st->pixel_xfer.cache);

   _mesa_free_context_data(ctx);

   st_destroy_context_priv(st);

   cso_destroy_context(cso);

   pipe->destroy( pipe );

   free(ctx);
}


void st_make_current(struct st_context *st,
                     struct st_framebuffer *draw,
                     struct st_framebuffer *read)
{
   if (st) {
      GLboolean firstTime = st->ctx->FirstTimeCurrent;
      _mesa_make_current(st->ctx, &draw->Base, &read->Base);
      /* Need to initialize viewport here since draw->Base->Width/Height
       * will still be zero at this point.
       * This could be improved, but would require rather extensive work
       * elsewhere (allocate rb surface storage sooner)
       */
      if (firstTime) {
         GLuint w = draw->InitWidth, h = draw->InitHeight;
         _mesa_set_viewport(st->ctx, 0, 0, w, h);
         _mesa_set_scissor(st->ctx, 0, 0, w, h);

      }
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }
}


void st_copy_context_state(struct st_context *dst,
                           struct st_context *src,
                           uint mask)
{
   _mesa_copy_context(dst->ctx, src->ctx, mask);
}



st_proc st_get_proc_address(const char *procname)
{
   return (st_proc) _glapi_get_proc_address(procname);
}



void st_init_driver_functions(struct dd_function_table *functions)
{
   _mesa_init_glsl_driver_functions(functions);

#if FEATURE_accum
   st_init_accum_functions(functions);
#endif
#if FEATURE_EXT_framebuffer_blit
   st_init_blit_functions(functions);
#endif
   st_init_bufferobject_functions(functions);
   st_init_clear_functions(functions);
#if FEATURE_drawpix
   st_init_bitmap_functions(functions);
   st_init_drawpixels_functions(functions);
   st_init_rasterpos_functions(functions);
#endif
   st_init_fbo_functions(functions);
   st_init_get_functions(functions);
#if FEATURE_feedback
   st_init_feedback_functions(functions);
#endif
   st_init_program_functions(functions);
#if FEATURE_ARB_occlusion_query
   st_init_query_functions(functions);
#endif
   st_init_readpixels_functions(functions);
   st_init_texture_functions(functions);
   st_init_flush_functions(functions);
   st_init_string_functions(functions);

   functions->UpdateState = st_invalidate_state;
}
