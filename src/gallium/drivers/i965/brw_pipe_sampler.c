
#include "util/u_memory.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_debug.h"



/* The brw (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static GLuint translate_wrap_mode( unsigned wrap )
{
   switch( wrap ) {
   case PIPE_TEX_WRAP_REPEAT: 
      return BRW_TEXCOORDMODE_WRAP;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return BRW_TEXCOORDMODE_CLAMP;
      
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      return BRW_TEXCOORDMODE_CLAMP_BORDER;

   case PIPE_TEX_WRAP_MIRROR_REPEAT: 
      return BRW_TEXCOORDMODE_MIRROR;

   case PIPE_TEX_WRAP_MIRROR_CLAMP: 
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE: 
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER: 
      return BRW_TEXCOORDMODE_MIRROR_ONCE;

   default: 
      return BRW_TEXCOORDMODE_WRAP;
   }
}



static void *brw_create_sampler_state( struct pipe_context *pipe,
				     const struct pipe_sampler_state *templ )
{
   struct brw_sampler_state *sampler = CALLOC_STRUCT(brw_sampler_state);

   switch (key->minfilter) {
   case GL_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   default:
      break;
   }

   /* Set Anisotropy: 
    */
   if (key->max_aniso > 1.0) {
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC; 
      sampler->ss0.mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      if (key->max_aniso > 2.0) {
	 sampler->ss3.max_aniso = MIN2((key->max_aniso - 2) / 2,
				       BRW_ANISORATIO_16);
      }
   }
   else {
      switch (key->magfilter) {
      case GL_NEAREST:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
	 break;
      default:
	 break;
      }
   }

   sampler->ss1.r_wrap_mode = translate_wrap_mode(key->wrap_r);
   sampler->ss1.s_wrap_mode = translate_wrap_mode(key->wrap_s);
   sampler->ss1.t_wrap_mode = translate_wrap_mode(key->wrap_t);

   /* Set LOD bias: 
    */
   sampler->ss0.lod_bias = S_FIXED(CLAMP(key->lod_bias, -16, 15), 6);

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set shadow function: 
    */
   if (key->comparemode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss0.shadow_function =
	 intel_translate_shadow_compare_func(key->comparefunc);
   }

   /* Set BaseMipLevel, MaxLOD, MinLOD: 
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(MIN2(MAX2(key->maxlod, 0), 13), 6);
   sampler->ss1.min_lod = U_FIXED(MIN2(MAX2(key->minlod, 0), 13), 6);

   return (void *)sampler;
}

static void brw_bind_sampler_state(struct pipe_context *pipe,
				 void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.sampler = (const struct brw_sampler_state *)cso;
   brw->state.dirty.mesa |= PIPE_NEW_SAMPLER;
}

static void brw_delete_sampler_state(struct pipe_context *pipe,
				  void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   FREE(cso);
}

static void brw_set_sampler_textures(struct pipe_context *pipe,
				     unsigned num_textures,
				     struct pipe_texture **tex)
{
   struct brw_context *brw = brw_context(pipe);

   brw->state.dirty.mesa |= PIPE_NEW_BOUND_TEXTURES;
}


void brw_pipe_sampler_init( struct brw_context *brw )
{
   brw->base.set_sampler_textures = brw_set_sampler_textures;
   brw->base.create_sampler_state = brw_create_sampler_state;
   brw->base.bind_sampler_state = brw_bind_sampler_state;
   brw->base.destroy_sampler_state = brw_destroy_sampler_state;
}

void brw_pipe_sampler_cleanup( struct brw_context *brw )
{
}
