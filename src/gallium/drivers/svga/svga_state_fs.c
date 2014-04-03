/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"
#include "tgsi/tgsi_ureg.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_shader.h"
#include "svga_resource_texture.h"
#include "svga_tgsi.h"

#include "svga_hw_reg.h"



static INLINE int
compare_fs_keys(const struct svga_fs_compile_key *a,
                const struct svga_fs_compile_key *b)
{
   unsigned keysize_a = svga_fs_key_size( a );
   unsigned keysize_b = svga_fs_key_size( b );

   if (keysize_a != keysize_b) {
      return (int)(keysize_a - keysize_b);
   }
   return memcmp( a, b, keysize_a );
}


/** Search for a fragment shader variant */
static struct svga_shader_variant *
search_fs_key(const struct svga_fragment_shader *fs,
              const struct svga_fs_compile_key *key)
{
   struct svga_shader_variant *variant = fs->base.variants;

   assert(key);

   for ( ; variant; variant = variant->next) {
      if (compare_fs_keys( key, &variant->key.fkey ) == 0)
         return variant;
   }
   
   return NULL;
}


/**
 * If we fail to compile a fragment shader (because it uses too many
 * registers, for example) we'll use a dummy/fallback shader that
 * simply emits a constant color (red for debug, black for release).
 * We hit this with the Unigine/Heaven demo when Shaders = High.
 * With black, the demo still looks good.
 */
static const struct tgsi_token *
get_dummy_fragment_shader(void)
{
#ifdef DEBUG
   static const float color[4] = { 1.0, 0.0, 0.0, 0.0 }; /* red */
#else
   static const float color[4] = { 0.0, 0.0, 0.0, 0.0 }; /* black */
#endif
   struct ureg_program *ureg;
   const struct tgsi_token *tokens;
   struct ureg_src src;
   struct ureg_dst dst;
   unsigned num_tokens;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!ureg)
      return NULL;

   dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   src = ureg_DECL_immediate(ureg, color, 4);
   ureg_MOV(ureg, dst, src);
   ureg_END(ureg);

   tokens = ureg_get_tokens(ureg, &num_tokens);

   ureg_destroy(ureg);

   return tokens;
}


/**
 * Replace the given shader's instruction with a simple constant-color
 * shader.  We use this when normal shader translation fails.
 */
static struct svga_shader_variant *
get_compiled_dummy_shader(struct svga_fragment_shader *fs,
                          const struct svga_fs_compile_key *key)
{
   const struct tgsi_token *dummy = get_dummy_fragment_shader();
   struct svga_shader_variant *variant;

   if (!dummy) {
      return NULL;
   }

   FREE((void *) fs->base.tokens);
   fs->base.tokens = dummy;

   variant = svga_translate_fragment_program(fs, key);
   return variant;
}


/**
 * Translate TGSI shader into an svga shader variant.
 */
static enum pipe_error
compile_fs(struct svga_context *svga,
           struct svga_fragment_shader *fs,
           const struct svga_fs_compile_key *key,
           struct svga_shader_variant **out_variant)
{
   struct svga_shader_variant *variant;
   enum pipe_error ret = PIPE_ERROR;

   variant = svga_translate_fragment_program( fs, key );
   if (variant == NULL) {
      debug_printf("Failed to compile fragment shader,"
                   " using dummy shader instead.\n");
      variant = get_compiled_dummy_shader(fs, key);
      if (!variant) {
         ret = PIPE_ERROR;
         goto fail;
      }
   }

   if (variant->nr_tokens * sizeof(variant->tokens[0])
       + sizeof(SVGA3dCmdDefineShader) + sizeof(SVGA3dCmdHeader)
       >= SVGA_CB_MAX_COMMAND_SIZE) {
      /* too big, use dummy shader */
      debug_printf("Shader too large (%lu bytes),"
                   " using dummy shader instead.\n",
                   (unsigned long ) variant->nr_tokens * sizeof(variant->tokens[0]));
      variant = get_compiled_dummy_shader(fs, key);
      if (!variant) {
         ret = PIPE_ERROR;
         goto fail;
      }
   }

   ret = svga_define_shader(svga, SVGA3D_SHADERTYPE_PS, variant);
   if (ret != PIPE_OK)
      goto fail;

   *out_variant = variant;

   /* insert variants at head of linked list */
   variant->next = fs->base.variants;
   fs->base.variants = variant;

   return PIPE_OK;

fail:
   if (variant) {
      svga_destroy_shader_variant(svga, SVGA3D_SHADERTYPE_PS, variant);
   }
   return ret;
}


/* SVGA_NEW_TEXTURE_BINDING
 * SVGA_NEW_RAST
 * SVGA_NEW_NEED_SWTNL
 * SVGA_NEW_SAMPLER
 */
static enum pipe_error
make_fs_key(const struct svga_context *svga,
            struct svga_fragment_shader *fs,
            struct svga_fs_compile_key *key)
{
   unsigned i;
   int idx = 0;

   memset(key, 0, sizeof *key);

   /* Only need fragment shader fixup for twoside lighting if doing
    * hwtnl.  Otherwise the draw module does the whole job for us.
    *
    * SVGA_NEW_SWTNL
    */
   if (!svga->state.sw.need_swtnl) {
      /* SVGA_NEW_RAST
       */
      key->light_twoside = svga->curr.rast->templ.light_twoside;
      key->front_ccw = svga->curr.rast->templ.front_ccw;
   }

   /* The blend workaround for simulating logicop xor behaviour
    * requires that the incoming fragment color be white.  This change
    * achieves that by creating a variant of the current fragment
    * shader that overrides all output colors with 1,1,1,1
    *   
    * This will work for most shaders, including those containing
    * TEXKIL and/or depth-write.  However, it will break on the
    * combination of xor-logicop plus alphatest.
    *
    * Ultimately, we could implement alphatest in the shader using
    * texkil prior to overriding the outgoing fragment color.
    *   
    * SVGA_NEW_BLEND
    */
   if (svga->curr.blend->need_white_fragments) {
      key->white_fragments = 1;
   }

#ifdef DEBUG
   /*
    * We expect a consistent set of samplers and sampler views.
    * Do some debug checks/warnings here.
    */
   {
      static boolean warned = FALSE;
      unsigned i, n = MAX2(svga->curr.num_sampler_views,
                           svga->curr.num_samplers);
      /* Only warn once to prevent too much debug output */
      if (!warned) {
         if (svga->curr.num_sampler_views != svga->curr.num_samplers) {
            debug_printf("svga: mismatched number of sampler views (%u) "
                         "vs. samplers (%u)\n",
                         svga->curr.num_sampler_views,
                         svga->curr.num_samplers);
         }
         for (i = 0; i < n; i++) {
            if ((svga->curr.sampler_views[i] == NULL) !=
                (svga->curr.sampler[i] == NULL))
               debug_printf("sampler_view[%u] = %p but sampler[%u] = %p\n",
                            i, svga->curr.sampler_views[i],
                            i, svga->curr.sampler[i]);
         }
         warned = TRUE;
      }
   }
#endif

   /* XXX: want to limit this to the textures that the shader actually
    * refers to.
    *
    * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
    */
   for (i = 0; i < svga->curr.num_sampler_views; i++) {
      if (svga->curr.sampler_views[i] && svga->curr.sampler[i]) {
         assert(svga->curr.sampler_views[i]->texture);
         key->tex[i].texture_target = svga->curr.sampler_views[i]->texture->target;
         if (!svga->curr.sampler[i]->normalized_coords) {
            key->tex[i].width_height_idx = idx++;
            key->tex[i].unnormalized = TRUE;
            ++key->num_unnormalized_coords;
         }

         key->tex[i].swizzle_r = svga->curr.sampler_views[i]->swizzle_r;
         key->tex[i].swizzle_g = svga->curr.sampler_views[i]->swizzle_g;
         key->tex[i].swizzle_b = svga->curr.sampler_views[i]->swizzle_b;
         key->tex[i].swizzle_a = svga->curr.sampler_views[i]->swizzle_a;
      }
   }
   key->num_textures = svga->curr.num_sampler_views;

   idx = 0;
   for (i = 0; i < svga->curr.num_samplers; ++i) {
      if (svga->curr.sampler_views[i] && svga->curr.sampler[i]) {
         struct pipe_resource *tex = svga->curr.sampler_views[i]->texture;
         struct svga_texture *stex = svga_texture(tex);
         SVGA3dSurfaceFormat format = stex->key.format;

         if (format == SVGA3D_Z_D16 ||
             format == SVGA3D_Z_D24X8 ||
             format == SVGA3D_Z_D24S8) {
            /* If we're sampling from a SVGA3D_Z_D16, SVGA3D_Z_D24X8,
             * or SVGA3D_Z_D24S8 surface, we'll automatically get
             * shadow comparison.  But we only get LEQUAL mode.
             * Set TEX_COMPARE_NONE here so we don't emit the extra FS
             * code for shadow comparison.
             */
            key->tex[i].compare_mode = PIPE_TEX_COMPARE_NONE;
            key->tex[i].compare_func = PIPE_FUNC_NEVER;
            /* These depth formats _only_ support comparison mode and
             * not ordinary sampling so warn if the later is expected.
             */
            if (svga->curr.sampler[i]->compare_mode !=
                PIPE_TEX_COMPARE_R_TO_TEXTURE) {
               debug_warn_once("Unsupported shadow compare mode");
            }                   
            /* The only supported comparison mode is LEQUAL */
            if (svga->curr.sampler[i]->compare_func != PIPE_FUNC_LEQUAL) {
               debug_warn_once("Unsupported shadow compare function");
            }
         }
         else {
            /* For other texture formats, just use the compare func/mode
             * as-is.  Should be no-ops for color textures.  For depth
             * textures, we do not get automatic depth compare.  We have
             * to do it ourselves in the shader.  And we don't get PCF.
             */
            key->tex[i].compare_mode = svga->curr.sampler[i]->compare_mode;
            key->tex[i].compare_func = svga->curr.sampler[i]->compare_func;
         }
      }
   }

   /* sprite coord gen state */
   for (i = 0; i < svga->curr.num_samplers; ++i) {
      key->tex[i].sprite_texgen =
         svga->curr.rast->templ.sprite_coord_enable & (1 << i);
   }

   key->sprite_origin_lower_left = (svga->curr.rast->templ.sprite_coord_mode
                                    == PIPE_SPRITE_COORD_LOWER_LEFT);

   /* SVGA_NEW_FRAME_BUFFER */
   if (fs->base.info.color0_writes_all_cbufs) {
      /* Replicate color0 output to N colorbuffers */
      key->write_color0_to_n_cbufs = svga->curr.framebuffer.nr_cbufs;
   }

   return PIPE_OK;
}


/**
 * svga_reemit_fs_bindings - Reemit the fragment shader bindings
 */
enum pipe_error
svga_reemit_fs_bindings(struct svga_context *svga)
{
   enum pipe_error ret;

   assert(svga->rebind.fs);
   assert(svga_have_gb_objects(svga));

   if (!svga->state.hw_draw.fs)
      return PIPE_OK;

   ret = SVGA3D_SetGBShader(svga->swc, SVGA3D_SHADERTYPE_PS,
                            svga->state.hw_draw.fs->gb_shader);
   if (ret != PIPE_OK)
      return ret;

   svga->rebind.fs = FALSE;
   return PIPE_OK;
}



static enum pipe_error
emit_hw_fs(struct svga_context *svga, unsigned dirty)
{
   struct svga_shader_variant *variant = NULL;
   enum pipe_error ret = PIPE_OK;
   struct svga_fragment_shader *fs = svga->curr.fs;
   struct svga_fs_compile_key key;

   /* SVGA_NEW_BLEND
    * SVGA_NEW_TEXTURE_BINDING
    * SVGA_NEW_RAST
    * SVGA_NEW_NEED_SWTNL
    * SVGA_NEW_SAMPLER
    * SVGA_NEW_FRAME_BUFFER
    */
   ret = make_fs_key( svga, fs, &key );
   if (ret != PIPE_OK)
      return ret;

   variant = search_fs_key( fs, &key );
   if (!variant) {
      ret = compile_fs( svga, fs, &key, &variant );
      if (ret != PIPE_OK)
         return ret;
   }

   assert(variant);

   if (variant != svga->state.hw_draw.fs) {
      if (svga_have_gb_objects(svga)) {
         /*
          * Bind is necessary here only because pipebuffer_fenced may move
          * the shader contents around....
          */
         ret = SVGA3D_BindGBShader(svga->swc, variant->gb_shader);
         if (ret != PIPE_OK)
            return ret;

         ret = SVGA3D_SetGBShader(svga->swc, SVGA3D_SHADERTYPE_PS,
                                  variant->gb_shader);
         if (ret != PIPE_OK)
            return ret;

         svga->rebind.fs = FALSE;
      }
      else {
         ret = SVGA3D_SetShader(svga->swc, SVGA3D_SHADERTYPE_PS, variant->id);
         if (ret != PIPE_OK)
            return ret;
      }

      svga->dirty |= SVGA_NEW_FS_VARIANT;
      svga->state.hw_draw.fs = variant;      
   }

   return PIPE_OK;
}

struct svga_tracked_state svga_hw_fs = 
{
   "fragment shader (hwtnl)",
   (SVGA_NEW_FS |
    SVGA_NEW_TEXTURE_BINDING |
    SVGA_NEW_NEED_SWTNL |
    SVGA_NEW_RAST |
    SVGA_NEW_SAMPLER |
    SVGA_NEW_FRAME_BUFFER |
    SVGA_NEW_BLEND),
   emit_hw_fs
};



