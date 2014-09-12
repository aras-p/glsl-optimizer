/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/imports.h"
#include "main/accum.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/samplerobj.h"
#include "main/shaderobj.h"
#include "main/version.h"
#include "main/vtxfmt.h"
#include "main/hash.h"
#include "program/prog_cache.h"
#include "vbo/vbo.h"
#include "glapi/glapi.h"
#include "st_context.h"
#include "st_debug.h"
#include "st_cb_bitmap.h"
#include "st_cb_blit.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_clear.h"
#include "st_cb_condrender.h"
#include "st_cb_drawpixels.h"
#include "st_cb_rasterpos.h"
#include "st_cb_drawtex.h"
#include "st_cb_eglimage.h"
#include "st_cb_fbo.h"
#include "st_cb_feedback.h"
#include "st_cb_msaa.h"
#include "st_cb_program.h"
#include "st_cb_queryobj.h"
#include "st_cb_readpixels.h"
#include "st_cb_texture.h"
#include "st_cb_xformfb.h"
#include "st_cb_flush.h"
#include "st_cb_syncobj.h"
#include "st_cb_strings.h"
#include "st_cb_texturebarrier.h"
#include "st_cb_viewport.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_extensions.h"
#include "st_gen_mipmap.h"
#include "st_program.h"
#include "st_vdpau.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_upload_mgr.h"
#include "cso_cache/cso_context.h"


DEBUG_GET_ONCE_BOOL_OPTION(mesa_mvp_dp4, "MESA_MVP_DP4", FALSE)


/**
 * Called via ctx->Driver.UpdateState()
 */
void st_invalidate_state(struct gl_context * ctx, GLuint new_state)
{
   struct st_context *st = st_context(ctx);

   /* Replace _NEW_FRAG_CLAMP with ST_NEW_FRAGMENT_PROGRAM for the fallback. */
   if (st->clamp_frag_color_in_shader && (new_state & _NEW_FRAG_CLAMP)) {
      new_state &= ~_NEW_FRAG_CLAMP;
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   }

   /* Update the vertex shader if ctx->Light._ClampVertexColor was changed. */
   if (st->clamp_vert_color_in_shader && (new_state & _NEW_LIGHT)) {
      st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   }

   st->dirty.mesa |= new_state;
   st->dirty.st |= ST_NEW_MESA;

   /* This is the only core Mesa module we depend upon.
    * No longer use swrast, swsetup, tnl.
    */
   _vbo_InvalidateState(ctx, new_state);
}


static void
st_destroy_context_priv(struct st_context *st)
{
   uint shader, i;

   st_destroy_atoms( st );
   st_destroy_draw( st );
   st_destroy_clear(st);
   st_destroy_bitmap(st);
   st_destroy_drawpix(st);
   st_destroy_drawtex(st);

   for (shader = 0; shader < Elements(st->state.sampler_views); shader++) {
      for (i = 0; i < Elements(st->state.sampler_views[0]); i++) {
         pipe_sampler_view_release(st->pipe,
                                   &st->state.sampler_views[shader][i]);
      }
   }

   if (st->default_texture) {
      st->ctx->Driver.DeleteTexture(st->ctx, st->default_texture);
      st->default_texture = NULL;
   }

   u_upload_destroy(st->uploader);
   if (st->indexbuf_uploader) {
      u_upload_destroy(st->indexbuf_uploader);
   }
   if (st->constbuf_uploader) {
      u_upload_destroy(st->constbuf_uploader);
   }
   free( st );
}


static struct st_context *
st_create_context_priv( struct gl_context *ctx, struct pipe_context *pipe,
		const struct st_config_options *options)
{
   struct pipe_screen *screen = pipe->screen;
   uint i;
   struct st_context *st = ST_CALLOC_STRUCT( st_context );
   
   st->options = *options;

   ctx->st = st;

   st->ctx = ctx;
   st->pipe = pipe;

   /* XXX: this is one-off, per-screen init: */
   st_debug_init();
   
   /* state tracker needs the VBO module */
   _vbo_CreateContext(ctx);

   st->dirty.mesa = ~0;
   st->dirty.st = ~0;

   /* Create upload manager for vertex data for glBitmap, glDrawPixels,
    * glClear, etc.
    */
   st->uploader = u_upload_create(st->pipe, 65536, 4, PIPE_BIND_VERTEX_BUFFER);

   if (!screen->get_param(screen, PIPE_CAP_USER_INDEX_BUFFERS)) {
      st->indexbuf_uploader = u_upload_create(st->pipe, 128 * 1024, 4,
                                              PIPE_BIND_INDEX_BUFFER);
   }

   if (!screen->get_param(screen, PIPE_CAP_USER_CONSTANT_BUFFERS)) {
      unsigned alignment =
         screen->get_param(screen, PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT);

      st->constbuf_uploader = u_upload_create(pipe, 128 * 1024, alignment,
                                              PIPE_BIND_CONSTANT_BUFFER);
   }

   st->cso_context = cso_create_context(pipe);

   st_init_atoms( st );
   st_init_bitmap(st);
   st_init_clear(st);
   st_init_draw( st );

   /* Choose texture target for glDrawPixels, glBitmap, renderbuffers */
   if (pipe->screen->get_param(pipe->screen, PIPE_CAP_NPOT_TEXTURES))
      st->internal_target = PIPE_TEXTURE_2D;
   else
      st->internal_target = PIPE_TEXTURE_RECT;

   /* Vertex element objects used for drawing rectangles for glBitmap,
    * glDrawPixels, glClear, etc.
    */
   for (i = 0; i < Elements(st->velems_util_draw); i++) {
      memset(&st->velems_util_draw[i], 0, sizeof(struct pipe_vertex_element));
      st->velems_util_draw[i].src_offset = i * 4 * sizeof(float);
      st->velems_util_draw[i].instance_divisor = 0;
      st->velems_util_draw[i].vertex_buffer_index =
            cso_get_aux_vertex_buffer_slot(st->cso_context);
      st->velems_util_draw[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   }

   /* we want all vertex data to be placed in buffer objects */
   vbo_use_buffer_objects(ctx);


   /* make sure that no VBOs are left mapped when we're drawing. */
   vbo_always_unmap_buffers(ctx);

   /* Need these flags:
    */
   st->ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

   st->ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;

   st->pixel_xfer.cache = _mesa_new_program_cache();

   st->has_stencil_export =
      screen->get_param(screen, PIPE_CAP_SHADER_STENCIL_EXPORT);
   st->has_shader_model3 = screen->get_param(screen, PIPE_CAP_SM3);
   st->has_etc1 = screen->is_format_supported(screen, PIPE_FORMAT_ETC1_RGB8,
                                              PIPE_TEXTURE_2D, 0,
                                              PIPE_BIND_SAMPLER_VIEW);
   st->prefer_blit_based_texture_transfer = screen->get_param(screen,
                              PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER);

   st->needs_texcoord_semantic =
      screen->get_param(screen, PIPE_CAP_TGSI_TEXCOORD);
   st->apply_texture_swizzle_to_border_color =
      !!(screen->get_param(screen, PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK) &
         (PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_NV50 |
          PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_R600));
   st->has_time_elapsed =
      screen->get_param(screen, PIPE_CAP_QUERY_TIME_ELAPSED);

   /* GL limits and extensions */
   st_init_limits(st->pipe->screen, &ctx->Const, &ctx->Extensions);
   st_init_extensions(st->pipe->screen, &ctx->Const,
                      &ctx->Extensions, &st->options, ctx->Mesa_DXTn);

   /* Enable shader-based fallbacks for ARB_color_buffer_float if needed. */
   if (screen->get_param(screen, PIPE_CAP_VERTEX_COLOR_UNCLAMPED)) {
      if (!screen->get_param(screen, PIPE_CAP_VERTEX_COLOR_CLAMPED)) {
         st->clamp_vert_color_in_shader = GL_TRUE;
      }

      if (!screen->get_param(screen, PIPE_CAP_FRAGMENT_COLOR_CLAMPED)) {
         st->clamp_frag_color_in_shader = GL_TRUE;
      }

      /* For drivers which cannot do color clamping, it's better to just
       * disable ARB_color_buffer_float in the core profile, because
       * the clamping is deprecated there anyway. */
      if (ctx->API == API_OPENGL_CORE &&
          (st->clamp_frag_color_in_shader || st->clamp_vert_color_in_shader)) {
         st->clamp_vert_color_in_shader = GL_FALSE;
         st->clamp_frag_color_in_shader = GL_FALSE;
         ctx->Extensions.ARB_color_buffer_float = GL_FALSE;
      }
   }

   /* called after _mesa_create_context/_mesa_init_point, fix default user
    * settable max point size up
    */
   st->ctx->Point.MaxSize = MAX2(ctx->Const.MaxPointSize,
                                 ctx->Const.MaxPointSizeAA);

   _mesa_compute_version(ctx);

   if (ctx->Version == 0) {
      /* This can happen when a core profile was requested, but the driver
       * does not support some features of GL 3.1 or later.
       */
      st_destroy_context_priv(st);
      return NULL;
   }

   _mesa_initialize_dispatch_tables(ctx);
   _mesa_initialize_vbo_vtxfmt(ctx);

   return st;
}

static void st_init_driver_flags(struct gl_driver_flags *f)
{
   f->NewArray = ST_NEW_VERTEX_ARRAYS;
   f->NewRasterizerDiscard = ST_NEW_RASTERIZER;
   f->NewUniformBuffer = ST_NEW_UNIFORM_BUFFER;
}

struct st_context *st_create_context(gl_api api, struct pipe_context *pipe,
                                     const struct gl_config *visual,
                                     struct st_context *share,
                                     const struct st_config_options *options)
{
   struct gl_context *ctx;
   struct gl_context *shareCtx = share ? share->ctx : NULL;
   struct dd_function_table funcs;
   struct st_context *st;

   memset(&funcs, 0, sizeof(funcs));
   st_init_driver_functions(&funcs);

   ctx = _mesa_create_context(api, visual, shareCtx, &funcs);
   if (!ctx) {
      return NULL;
   }

   st_init_driver_flags(&ctx->DriverFlags);

   /* XXX: need a capability bit in gallium to query if the pipe
    * driver prefers DP4 or MUL/MAD for vertex transformation.
    */
   if (debug_get_option_mesa_mvp_dp4())
      ctx->Const.ShaderCompilerOptions[MESA_SHADER_VERTEX].OptimizeForAOS = GL_TRUE;

   st = st_create_context_priv(ctx, pipe, options);
   if (!st) {
      _mesa_destroy_context(ctx);
   }

   return st;
}


/**
 * Callback to release the sampler view attached to a texture object.
 * Called by _mesa_HashWalk().
 */
static void
destroy_tex_sampler_cb(GLuint id, void *data, void *userData)
{
   struct gl_texture_object *texObj = (struct gl_texture_object *) data;
   struct st_context *st = (struct st_context *) userData;

   st_texture_release_sampler_view(st, st_texture_object(texObj));
}
 
void st_destroy_context( struct st_context *st )
{
   struct pipe_context *pipe = st->pipe;
   struct cso_context *cso = st->cso_context;
   struct gl_context *ctx = st->ctx;
   GLuint i;

   _mesa_HashWalk(ctx->Shared->TexObjects, destroy_tex_sampler_cb, st);

   /* need to unbind and destroy CSO objects before anything else */
   cso_release_all(st->cso_context);

   st_reference_fragprog(st, &st->fp, NULL);
   st_reference_geomprog(st, &st->gp, NULL);
   st_reference_vertprog(st, &st->vp, NULL);

   /* release framebuffer surfaces */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&st->state.framebuffer.cbufs[i], NULL);
   }
   pipe_surface_reference(&st->state.framebuffer.zsbuf, NULL);

   pipe->set_index_buffer(pipe, NULL);

   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      pipe->set_constant_buffer(pipe, i, 0, NULL);
   }

   _mesa_delete_program_cache(st->ctx, st->pixel_xfer.cache);

   _vbo_DestroyContext(st->ctx);

   st_destroy_program_variants(st);

   _mesa_free_context_data(ctx);

   /* This will free the st_context too, so 'st' must not be accessed
    * afterwards. */
   st_destroy_context_priv(st);
   st = NULL;

   cso_destroy_context(cso);

   pipe->destroy( pipe );

   free(ctx);
}


void st_init_driver_functions(struct dd_function_table *functions)
{
   _mesa_init_shader_object_functions(functions);
   _mesa_init_sampler_object_functions(functions);

   functions->Accum = _mesa_accum;

   st_init_blit_functions(functions);
   st_init_bufferobject_functions(functions);
   st_init_clear_functions(functions);
   st_init_bitmap_functions(functions);
   st_init_drawpixels_functions(functions);
   st_init_rasterpos_functions(functions);

   st_init_drawtex_functions(functions);

   st_init_eglimage_functions(functions);

   st_init_fbo_functions(functions);
   st_init_feedback_functions(functions);
   st_init_msaa_functions(functions);
   st_init_program_functions(functions);
   st_init_query_functions(functions);
   st_init_cond_render_functions(functions);
   st_init_readpixels_functions(functions);
   st_init_texture_functions(functions);
   st_init_texture_barrier_functions(functions);
   st_init_flush_functions(functions);
   st_init_string_functions(functions);
   st_init_viewport_functions(functions);

   st_init_xformfb_functions(functions);
   st_init_syncobj_functions(functions);

   st_init_vdpau_functions(functions);

   functions->UpdateState = st_invalidate_state;
}
