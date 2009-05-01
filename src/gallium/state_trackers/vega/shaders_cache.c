/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "shaders_cache.h"

#include "vg_context.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_text.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "cso_cache/cso_hash.h"
#include "cso_cache/cso_context.h"

#include "VG/openvg.h"

#include "asm_fill.h"

/* Essentially we construct an ubber-shader based on the state
 * of the pipeline. The stages are:
 * 1) Fill (mandatory, solid color/gradient/pattern/image draw)
 * 2) Image composition (image mode multiply and stencil)
 * 3) Mask
 * 4) Extended blend (multiply/screen/darken/lighten)
 * 5) Premultiply/Unpremultiply
 * 6) Color transform (to black and white)
 */
#define SHADER_STAGES 6

struct cached_shader {
   void *driver_shader;
   struct pipe_shader_state state;
};

struct shaders_cache {
   struct vg_context *pipe;

   struct cso_hash *hash;
};


static INLINE struct tgsi_token *tokens_from_assembly(const char *txt, int num_tokens)
{
   struct tgsi_token *tokens;

   tokens = (struct tgsi_token *) MALLOC(num_tokens * sizeof(tokens[0]));

   tgsi_text_translate(txt, tokens, num_tokens);

#if DEBUG_SHADERS
   tgsi_dump(tokens, 0);
#endif

   return tokens;
}

#define ALL_FILLS (VEGA_SOLID_FILL_SHADER | \
   VEGA_LINEAR_GRADIENT_SHADER | \
   VEGA_RADIAL_GRADIENT_SHADER | \
   VEGA_PATTERN_SHADER         | \
   VEGA_IMAGE_NORMAL_SHADER)


/*
static const char max_shader_preamble[] =
   "FRAG1.1\n"
   "DCL IN[0], POSITION, LINEAR\n"
   "DCL IN[1], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR, CONSTANT\n"
   "DCL CONST[0..9], CONSTANT\n"
   "DCL TEMP[0..9], CONSTANT\n"
   "DCL SAMP[0..9], CONSTANT\n";

   max_shader_preamble strlen == 175
*/
#define MAX_PREAMBLE 175

static INLINE VGint range_min(VGint min, VGint current)
{
   if (min < 0)
      min = current;
   else
      min = MIN2(min, current);
   return min;
}

static INLINE VGint range_max(VGint max, VGint current)
{
   return MAX2(max, current);
}

static void
create_preamble(char *txt,
                const struct shader_asm_info *shaders[SHADER_STAGES],
                int num_shaders)
{
   VGboolean declare_input = VG_FALSE;
   VGint start_const   = -1, end_const   = 0;
   VGint start_temp    = -1, end_temp    = 0;
   VGint start_sampler = -1, end_sampler = 0;
   VGint i;
   VGint num_consts, num_temps, num_samplers;

   for (i = 0; i < num_shaders; ++i) {
      if (shaders[i]->num_consts)
         start_const = range_min(start_const, shaders[i]->start_const);
      if (shaders[i]->num_temps)
         start_temp = range_min(start_temp, shaders[i]->start_temp);
      if (shaders[i]->num_samplers)
         start_sampler = range_min(start_sampler, shaders[i]->start_sampler);

      end_const = range_max(end_const, shaders[i]->start_const +
                            shaders[i]->num_consts);
      end_temp = range_max(end_temp, shaders[i]->start_temp +
                            shaders[i]->num_temps);
      end_sampler = range_max(end_sampler, shaders[i]->start_sampler +
                            shaders[i]->num_samplers);
      if (shaders[i]->needs_position)
         declare_input = VG_TRUE;
   }
   /* if they're still unitialized, initialize them */
   if (start_const < 0)
      start_const = 0;
   if (start_temp < 0)
      start_temp = 0;
   if (start_sampler < 0)
      start_sampler = 0;

   num_consts   = end_const   - start_const;
   num_temps    = end_temp    - start_temp;
   num_samplers = end_sampler - start_sampler;
   /* end exclusive */
   --end_const;
   --end_temp;
   --end_sampler;

   sprintf(txt, "FRAG1.1\n");

   if (declare_input) {
      sprintf(txt + strlen(txt), "DCL IN[0], POSITION, LINEAR\n");
      sprintf(txt + strlen(txt), "DCL IN[1], GENERIC[0], PERSPECTIVE\n");
   }

   /* we always have a color output */
   sprintf(txt + strlen(txt), "DCL OUT[0], COLOR, CONSTANT\n");

   if (num_consts > 1)
      sprintf(txt + strlen(txt), "DCL CONST[%d..%d], CONSTANT\n", start_const, end_const);
   else if (num_consts == 1)
      sprintf(txt + strlen(txt), "DCL CONST[%d], CONSTANT\n", start_const);

   if (num_temps > 1)
      sprintf(txt + strlen(txt), "DCL TEMP[%d..%d], CONSTANT\n", start_temp, end_temp);
   else if (num_temps > 1)
      sprintf(txt + strlen(txt), "DCL TEMP[%d], CONSTANT\n", start_temp);

   if (num_samplers > 1)
      sprintf(txt + strlen(txt), "DCL SAMP[%d..%d], CONSTANT\n", start_sampler, end_sampler);
   else if (num_samplers == 1)
      sprintf(txt + strlen(txt), "DCL SAMP[%d], CONSTANT\n", start_sampler);
}

static void *
combine_shaders(const struct shader_asm_info *shaders[SHADER_STAGES], int num_shaders,
                struct pipe_context *pipe,
                struct pipe_shader_state *shader)
{
   char *combined_txt;
   int combined_len = MAX_PREAMBLE;
   int combined_tokens = 0;
   int i = 0;
   int current_shader = 0;
   int current_len;

   for (i = 0; i < num_shaders; ++i) {
      combined_len += strlen(shaders[i]->txt);
      combined_tokens += shaders[i]->num_tokens;
   }
   /* add for the %s->TEMP[0] substitutions */
   combined_len += num_shaders * 7 /*TEMP[0]*/ + 4 /*"END\n"*/;

   combined_txt = (char*)malloc(combined_len);
   combined_txt[0] = '\0';

   create_preamble(combined_txt, shaders, num_shaders);

   while (current_shader < num_shaders) {
      const char temp[] = "TEMP[0]";
      const char out[] = "OUT[0]";
      const char *subst = temp;

      current_len = strlen(combined_txt);

      /* if the last shader then output */
      if (current_shader + 1 == num_shaders)
         subst = out;

      snprintf(combined_txt + current_len,
               combined_len - current_len,
               shaders[current_shader]->txt,
               subst);
      ++current_shader;
   }


   current_len = strlen(combined_txt);
   snprintf(combined_txt + current_len,
            combined_len - current_len,
            "END\n");

   debug_printf("Combined shader is : \n%s\n",
                 combined_txt);

   shader->tokens = tokens_from_assembly(
            combined_txt, combined_tokens);

   free(combined_txt);

   return pipe->create_fs_state(pipe, shader);
}

static void *
create_shader(struct pipe_context *pipe,
              int id,
              struct pipe_shader_state *shader)
{
   int idx = 0;
   const struct shader_asm_info * shaders[SHADER_STAGES];

   /* the shader has to have a fill */
   debug_assert(id & ALL_FILLS);

   /* first stage */
   if (id & VEGA_SOLID_FILL_SHADER) {
      debug_assert(idx == 0);
      shaders[idx] = &shaders_asm[0];
      debug_assert(shaders_asm[0].id == VEGA_SOLID_FILL_SHADER);
      ++idx;
   }
   if ((id & VEGA_LINEAR_GRADIENT_SHADER)) {
      debug_assert(idx == 0);
      shaders[idx] = &shaders_asm[1];
      debug_assert(shaders_asm[1].id == VEGA_LINEAR_GRADIENT_SHADER);
      ++idx;
   }
   if ((id & VEGA_RADIAL_GRADIENT_SHADER)) {
      debug_assert(idx == 0);
      shaders[idx] = &shaders_asm[2];
      debug_assert(shaders_asm[2].id == VEGA_RADIAL_GRADIENT_SHADER);
      ++idx;
   }
   if ((id & VEGA_PATTERN_SHADER)) {
      debug_assert(idx == 0);
      debug_assert(shaders_asm[3].id == VEGA_PATTERN_SHADER);
      shaders[idx] = &shaders_asm[3];
      ++idx;
   }
   if ((id & VEGA_IMAGE_NORMAL_SHADER)) {
      debug_assert(idx == 0);
      debug_assert(shaders_asm[4].id == VEGA_IMAGE_NORMAL_SHADER);
      shaders[idx] = &shaders_asm[4];
      ++idx;
   }

   /* second stage */
   if ((id & VEGA_IMAGE_MULTIPLY_SHADER)) {
      debug_assert(shaders_asm[5].id == VEGA_IMAGE_MULTIPLY_SHADER);
      shaders[idx] = &shaders_asm[5];
      ++idx;
   } else if ((id & VEGA_IMAGE_STENCIL_SHADER)) {
      debug_assert(shaders_asm[6].id == VEGA_IMAGE_STENCIL_SHADER);
      shaders[idx] = &shaders_asm[6];
      ++idx;
   }

   /* third stage */
   if ((id & VEGA_MASK_SHADER)) {
      debug_assert(idx == 1);
      debug_assert(shaders_asm[7].id == VEGA_MASK_SHADER);
      shaders[idx] = &shaders_asm[7];
      ++idx;
   }

   /* fourth stage */
   if ((id & VEGA_BLEND_MULTIPLY_SHADER)) {
      debug_assert(shaders_asm[8].id == VEGA_BLEND_MULTIPLY_SHADER);
      shaders[idx] = &shaders_asm[8];
      ++idx;
   } else if ((id & VEGA_BLEND_SCREEN_SHADER)) {
      debug_assert(shaders_asm[9].id == VEGA_BLEND_SCREEN_SHADER);
      shaders[idx] = &shaders_asm[9];
      ++idx;
   } else if ((id & VEGA_BLEND_DARKEN_SHADER)) {
      debug_assert(shaders_asm[10].id == VEGA_BLEND_DARKEN_SHADER);
      shaders[idx] = &shaders_asm[10];
      ++idx;
   } else if ((id & VEGA_BLEND_LIGHTEN_SHADER)) {
      debug_assert(shaders_asm[11].id == VEGA_BLEND_LIGHTEN_SHADER);
      shaders[idx] = &shaders_asm[11];
      ++idx;
   }

   /* fifth stage */
   if ((id & VEGA_PREMULTIPLY_SHADER)) {
      debug_assert(shaders_asm[12].id == VEGA_PREMULTIPLY_SHADER);
      shaders[idx] = &shaders_asm[12];
      ++idx;
   } else if ((id & VEGA_UNPREMULTIPLY_SHADER)) {
      debug_assert(shaders_asm[13].id == VEGA_UNPREMULTIPLY_SHADER);
      shaders[idx] = &shaders_asm[13];
      ++idx;
   }

   /* sixth stage */
   if ((id & VEGA_BW_SHADER)) {
      debug_assert(shaders_asm[14].id == VEGA_BW_SHADER);
      shaders[idx] = &shaders_asm[14];
      ++idx;
   }

   return combine_shaders(shaders, idx, pipe, shader);
}

/*************************************************/

struct shaders_cache * shaders_cache_create(struct vg_context *vg)
{
   struct shaders_cache *sc = CALLOC_STRUCT(shaders_cache);

   sc->pipe = vg;
   sc->hash = cso_hash_create();

   return sc;
}

void shaders_cache_destroy(struct shaders_cache *sc)
{
   struct cso_hash_iter iter = cso_hash_first_node(sc->hash);

   while (!cso_hash_iter_is_null(iter)) {
      struct cached_shader *cached =
         (struct cached_shader *)cso_hash_iter_data(iter);
      cso_delete_fragment_shader(sc->pipe->cso_context,
                                 cached->driver_shader);
      iter = cso_hash_erase(sc->hash, iter);
   }

   cso_hash_delete(sc->hash);
   free(sc);
}

void * shaders_cache_fill(struct shaders_cache *sc,
                          int shader_key)
{
   VGint key = shader_key;
   struct cached_shader *cached;
   struct cso_hash_iter iter = cso_hash_find(sc->hash, key);

   if (cso_hash_iter_is_null(iter)) {
      cached = CALLOC_STRUCT(cached_shader);
      cached->driver_shader = create_shader(sc->pipe->pipe, key, &cached->state);

      cso_hash_insert(sc->hash, key, cached);

      return cached->driver_shader;
   }

   cached = (struct cached_shader *)cso_hash_iter_data(iter);

   assert(cached->driver_shader);
   return cached->driver_shader;
}

struct vg_shader * shader_create_from_text(struct pipe_context *pipe,
                                           const char *txt, int num_tokens,
                                           int type)
{
   struct vg_shader *shader = (struct vg_shader *)malloc(
      sizeof(struct vg_shader));
   struct tgsi_token *tokens = tokens_from_assembly(txt, num_tokens);
   struct pipe_shader_state state;

   debug_assert(type == PIPE_SHADER_VERTEX ||
                type == PIPE_SHADER_FRAGMENT);

   state.tokens = tokens;
   shader->type = type;
   shader->tokens = tokens;

   if (type == PIPE_SHADER_FRAGMENT)
      shader->driver = pipe->create_fs_state(pipe, &state);
   else
      shader->driver = pipe->create_vs_state(pipe, &state);
   return shader;
}

void vg_shader_destroy(struct vg_context *ctx, struct vg_shader *shader)
{
   if (shader->type == PIPE_SHADER_FRAGMENT)
      cso_delete_fragment_shader(ctx->cso_context, shader->driver);
   else
      cso_delete_vertex_shader(ctx->cso_context, shader->driver);
   free(shader->tokens);
   free(shader);
}
