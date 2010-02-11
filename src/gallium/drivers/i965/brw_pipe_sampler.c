
#include "util/u_memory.h"
#include "util/u_math.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "brw_context.h"
#include "brw_defines.h"



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

static GLuint translate_img_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
      return BRW_MAPFILTER_NEAREST;
   case PIPE_TEX_FILTER_LINEAR:
      return BRW_MAPFILTER_LINEAR;
   default:
      assert(0);
      return BRW_MAPFILTER_NEAREST;
   }
}

static GLuint translate_mip_filter( unsigned filter )
{
   switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE: 
      return BRW_MIPFILTER_NONE;
   case PIPE_TEX_MIPFILTER_NEAREST:
      return BRW_MIPFILTER_NEAREST;
   case PIPE_TEX_MIPFILTER_LINEAR:
      return BRW_MIPFILTER_LINEAR;
   default:
      assert(0);
      return BRW_MIPFILTER_NONE;
   }
}

/* XXX: not sure why there are special translations for the shadow tex
 * compare functions.  In particular ALWAYS is translated to NEVER.
 * Is this a hardware issue?  Does i965 really suffer from this?
 */
static GLuint translate_shadow_compare_func( unsigned func )
{
   switch (func) {
   case PIPE_FUNC_NEVER: 
       return BRW_COMPAREFUNCTION_ALWAYS;
   case PIPE_FUNC_LESS: 
       return BRW_COMPAREFUNCTION_LEQUAL;
   case PIPE_FUNC_LEQUAL: 
       return BRW_COMPAREFUNCTION_LESS;
   case PIPE_FUNC_GREATER: 
       return BRW_COMPAREFUNCTION_GEQUAL;
   case PIPE_FUNC_GEQUAL: 
      return BRW_COMPAREFUNCTION_GREATER;
   case PIPE_FUNC_NOTEQUAL: 
      return BRW_COMPAREFUNCTION_EQUAL;
   case PIPE_FUNC_EQUAL: 
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case PIPE_FUNC_ALWAYS: 
       return BRW_COMPAREFUNCTION_NEVER;
   default:
      assert(0);
      return BRW_COMPAREFUNCTION_NEVER;
   }
}




static void *
brw_create_sampler_state( struct pipe_context *pipe,
                          const struct pipe_sampler_state *template )
{
   struct brw_sampler *sampler = CALLOC_STRUCT(brw_sampler);

   sampler->ss0.min_filter = translate_img_filter( template->min_img_filter );
   sampler->ss0.mag_filter = translate_img_filter( template->mag_img_filter );
   sampler->ss0.mip_filter = translate_mip_filter( template->min_mip_filter );


   /* XXX: anisotropy logic slightly changed: 
    */
   if (template->max_anisotropy > 1) {
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC; 
      sampler->ss0.mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      sampler->ss3.max_aniso = MIN2((template->max_anisotropy - 2) / 2,
                                    BRW_ANISORATIO_16);
   }

   sampler->ss1.r_wrap_mode = translate_wrap_mode(template->wrap_r);
   sampler->ss1.s_wrap_mode = translate_wrap_mode(template->wrap_s);
   sampler->ss1.t_wrap_mode = translate_wrap_mode(template->wrap_t);

   /* Set LOD bias: 
    */
   sampler->ss0.lod_bias = 
      util_signed_fixed(CLAMP(template->lod_bias, -16, 15), 6);


   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set shadow function: 
    */
   if (template->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {

      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss0.shadow_function =
	 translate_shadow_compare_func(template->compare_func);
   }

   /* Set BaseMipLevel, MaxLOD, MinLOD: 
    */
   sampler->ss0.base_level = 
      util_unsigned_fixed(0, 1);

   sampler->ss1.max_lod = 
      util_unsigned_fixed(CLAMP(template->max_lod, 0, 13), 6);

   sampler->ss1.min_lod = 
      util_unsigned_fixed(CLAMP(template->min_lod, 0, 13), 6);

   return (void *)sampler;
}

static void brw_bind_sampler_state(struct pipe_context *pipe,
                                   unsigned num, void **sampler)
{
   struct brw_context *brw = brw_context(pipe);
   int i;

   for (i = 0; i < num; i++)
      brw->curr.sampler[i] = sampler[i];

   for (i = num; i < brw->curr.num_samplers; i++)
      brw->curr.sampler[i] = NULL;

   brw->curr.num_samplers = num;
   brw->state.dirty.mesa |= PIPE_NEW_SAMPLERS;
}

static void brw_delete_sampler_state(struct pipe_context *pipe,
				  void *cso)
{
   FREE(cso);
}

static void brw_set_sampler_textures(struct pipe_context *pipe,
				     unsigned num,
				     struct pipe_texture **texture)
{
   struct brw_context *brw = brw_context(pipe);
   int i;

   for (i = 0; i < num; i++)
      pipe_texture_reference(&brw->curr.texture[i], texture[i]);

   for (i = num; i < brw->curr.num_textures; i++)
      pipe_texture_reference(&brw->curr.texture[i], NULL);

   brw->curr.num_textures = num;
   brw->state.dirty.mesa |= PIPE_NEW_BOUND_TEXTURES;
}

static void brw_set_vertex_sampler_textures(struct pipe_context *pipe,
                                            unsigned num,
                                            struct pipe_texture **texture)
{
}

static void brw_bind_vertex_sampler_state(struct pipe_context *pipe,
                                          unsigned num, void **sampler)
{
}


void brw_pipe_sampler_init( struct brw_context *brw )
{
   brw->base.create_sampler_state = brw_create_sampler_state;
   brw->base.delete_sampler_state = brw_delete_sampler_state;

   brw->base.set_fragment_sampler_textures = brw_set_sampler_textures;
   brw->base.bind_fragment_sampler_states = brw_bind_sampler_state;

   brw->base.set_vertex_sampler_textures = brw_set_vertex_sampler_textures;
   brw->base.bind_vertex_sampler_states = brw_bind_vertex_sampler_state;

}
void brw_pipe_sampler_cleanup( struct brw_context *brw )
{
}
