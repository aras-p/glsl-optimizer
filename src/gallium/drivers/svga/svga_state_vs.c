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
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"
#include "translate/translate.h"
#include "tgsi/tgsi_ureg.h"

#include "svga_context.h"
#include "svga_state.h"
#include "svga_cmd.h"
#include "svga_shader.h"
#include "svga_tgsi.h"

#include "svga_hw_reg.h"


static INLINE int
compare_vs_keys(const struct svga_vs_compile_key *a,
                const struct svga_vs_compile_key *b)
{
   unsigned keysize = svga_vs_key_size( a );
   return memcmp( a, b, keysize );
}


/** Search for a vertex shader variant */
static struct svga_shader_variant *
search_vs_key(const struct svga_vertex_shader *vs,
              const struct svga_vs_compile_key *key)
{
   struct svga_shader_variant *variant = vs->base.variants;

   assert(key);

   for ( ; variant; variant = variant->next) {
      if (compare_vs_keys( key, &variant->key.vkey ) == 0)
         return variant;
   }
   
   return NULL;
}


/**
 * If we fail to compile a vertex shader we'll use a dummy/fallback shader
 * that simply emits a (0,0,0,1) vertex position.
 */
static const struct tgsi_token *
get_dummy_vertex_shader(void)
{
   static const float zero[4] = { 0.0, 0.0, 0.0, 1.0 };
   struct ureg_program *ureg;
   const struct tgsi_token *tokens;
   struct ureg_src src;
   struct ureg_dst dst;
   unsigned num_tokens;

   ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!ureg)
      return NULL;

   dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   src = ureg_DECL_immediate(ureg, zero, 4);
   ureg_MOV(ureg, dst, src);
   ureg_END(ureg);

   tokens = ureg_get_tokens(ureg, &num_tokens);

   ureg_destroy(ureg);

   return tokens;
}


/**
 * Translate TGSI shader into an svga shader variant.
 */
static enum pipe_error
compile_vs(struct svga_context *svga,
           struct svga_vertex_shader *vs,
           const struct svga_vs_compile_key *key,
           struct svga_shader_variant **out_variant)
{
   struct svga_shader_variant *variant;
   enum pipe_error ret = PIPE_ERROR;

   variant = svga_translate_vertex_program( vs, key );
   if (variant == NULL) {
      /* some problem during translation, try the dummy shader */
      const struct tgsi_token *dummy = get_dummy_vertex_shader();
      if (!dummy) {
         ret = PIPE_ERROR_OUT_OF_MEMORY;
         goto fail;
      }
      debug_printf("Failed to compile vertex shader, using dummy shader instead.\n");
      FREE((void *) vs->base.tokens);
      vs->base.tokens = dummy;
      variant = svga_translate_vertex_program(vs, key);
      if (variant == NULL) {
         ret = PIPE_ERROR;
         goto fail;
      }
   }

   ret = svga_define_shader(svga, SVGA3D_SHADERTYPE_VS, variant);
   if (ret != PIPE_OK)
      goto fail;

   *out_variant = variant;

   /* insert variants at head of linked list */
   variant->next = vs->base.variants;
   vs->base.variants = variant;

   return PIPE_OK;

fail:
   if (variant) {
      svga_destroy_shader_variant(svga, SVGA3D_SHADERTYPE_VS, variant);
   }
   return ret;
}

/* SVGA_NEW_PRESCALE, SVGA_NEW_RAST, SVGA_NEW_FS
 */
static void
make_vs_key(struct svga_context *svga, struct svga_vs_compile_key *key)
{
   memset(key, 0, sizeof *key);
   key->need_prescale = svga->state.hw_clear.prescale.enabled;
   key->allow_psiz = svga->curr.rast->templ.point_size_per_vertex;

   /* SVGA_NEW_FS */
   key->fs_generic_inputs = svga->curr.fs->generic_inputs;

   /* SVGA_NEW_VELEMENT */
   key->adjust_attrib_range = svga->curr.velems->adjust_attrib_range;
   key->adjust_attrib_w_1 = svga->curr.velems->adjust_attrib_w_1;
}


/**
 * svga_reemit_vs_bindings - Reemit the vertex shader bindings
 */
enum pipe_error
svga_reemit_vs_bindings(struct svga_context *svga)
{
   enum pipe_error ret;
   struct svga_winsys_gb_shader *gbshader =
      svga->state.hw_draw.vs ? svga->state.hw_draw.vs->gb_shader : NULL;

   assert(svga->rebind.vs);
   assert(svga_have_gb_objects(svga));

   ret = SVGA3D_SetGBShader(svga->swc, SVGA3D_SHADERTYPE_VS, gbshader);
   if (ret != PIPE_OK)
      return ret;

   svga->rebind.vs = FALSE;
   return PIPE_OK;
}


static enum pipe_error
emit_hw_vs(struct svga_context *svga, unsigned dirty)
{
   struct svga_shader_variant *variant = NULL;
   enum pipe_error ret = PIPE_OK;

   /* SVGA_NEW_NEED_SWTNL */
   if (!svga->state.sw.need_swtnl) {
      struct svga_vertex_shader *vs = svga->curr.vs;
      struct svga_vs_compile_key key;

      make_vs_key( svga, &key );

      variant = search_vs_key( vs, &key );
      if (!variant) {
         ret = compile_vs( svga, vs, &key, &variant );
         if (ret != PIPE_OK)
            return ret;
      }

      assert(variant);
   }

   if (variant != svga->state.hw_draw.vs) {
      if (svga_have_gb_objects(svga)) {
         struct svga_winsys_gb_shader *gbshader =
            variant ? variant->gb_shader : NULL;

         /*
          * Bind is necessary here only because pipebuffer_fenced may move
          * the shader contents around....
          */
         if (gbshader) {
            ret = SVGA3D_BindGBShader(svga->swc, gbshader);
            if (ret != PIPE_OK)
               return ret;
         }

         ret = SVGA3D_SetGBShader(svga->swc, SVGA3D_SHADERTYPE_VS, gbshader);
         if (ret != PIPE_OK)
            return ret;

         svga->rebind.vs = FALSE;
      }
      else {
         unsigned id = variant ? variant->id : SVGA_ID_INVALID;
         ret = SVGA3D_SetShader(svga->swc, SVGA3D_SHADERTYPE_VS, id);
         if (ret != PIPE_OK)
            return ret;
      }

      svga->dirty |= SVGA_NEW_VS_VARIANT;
      svga->state.hw_draw.vs = variant;      
   }

   return PIPE_OK;
}

struct svga_tracked_state svga_hw_vs = 
{
   "vertex shader (hwtnl)",
   (SVGA_NEW_VS |
    SVGA_NEW_FS |
    SVGA_NEW_PRESCALE |
    SVGA_NEW_VELEMENT |
    SVGA_NEW_NEED_SWTNL),
   emit_hw_vs
};
