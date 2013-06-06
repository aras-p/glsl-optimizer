/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "tgsi/tgsi_parse.h"
#include "intel_winsys.h"

#include "ilo_shader.h"

/**
 * Initialize a shader variant.
 */
void
ilo_shader_variant_init(struct ilo_shader_variant *variant,
                        const struct ilo_shader_info *info,
                        const struct ilo_context *ilo)
{
   int num_views, i;

   memset(variant, 0, sizeof(*variant));

   switch (info->type) {
   case PIPE_SHADER_VERTEX:
      variant->u.vs.rasterizer_discard =
         ilo->rasterizer->state.rasterizer_discard;
      variant->u.vs.num_ucps =
         util_last_bit(ilo->rasterizer->state.clip_plane_enable);
      break;
   case PIPE_SHADER_GEOMETRY:
      variant->u.gs.rasterizer_discard =
         ilo->rasterizer->state.rasterizer_discard;
      variant->u.gs.num_inputs = ilo->vs->shader->out.count;
      for (i = 0; i < ilo->vs->shader->out.count; i++) {
         variant->u.gs.semantic_names[i] =
            ilo->vs->shader->out.semantic_names[i];
         variant->u.gs.semantic_indices[i] =
            ilo->vs->shader->out.semantic_indices[i];
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      variant->u.fs.flatshade =
         (info->has_color_interp && ilo->rasterizer->state.flatshade);
      variant->u.fs.fb_height = (info->has_pos) ?
         ilo->fb.state.height : 1;
      variant->u.fs.num_cbufs = ilo->fb.state.nr_cbufs;
      break;
   default:
      assert(!"unknown shader type");
      break;
   }

   num_views = ilo->view[info->type].count;
   assert(info->num_samplers <= num_views);

   variant->num_sampler_views = info->num_samplers;
   for (i = 0; i < info->num_samplers; i++) {
      const struct pipe_sampler_view *view = ilo->view[info->type].states[i];
      const struct ilo_sampler_cso *sampler = ilo->sampler[info->type].cso[i];

      if (view) {
         variant->sampler_view_swizzles[i].r = view->swizzle_r;
         variant->sampler_view_swizzles[i].g = view->swizzle_g;
         variant->sampler_view_swizzles[i].b = view->swizzle_b;
         variant->sampler_view_swizzles[i].a = view->swizzle_a;
      }
      else if (info->shadow_samplers & (1 << i)) {
         variant->sampler_view_swizzles[i].r = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].g = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].b = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].a = PIPE_SWIZZLE_ONE;
      }
      else {
         variant->sampler_view_swizzles[i].r = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].g = PIPE_SWIZZLE_GREEN;
         variant->sampler_view_swizzles[i].b = PIPE_SWIZZLE_BLUE;
         variant->sampler_view_swizzles[i].a = PIPE_SWIZZLE_ALPHA;
      }

      /*
       * When non-nearest filter and PIPE_TEX_WRAP_CLAMP wrap mode is used,
       * the HW wrap mode is set to BRW_TEXCOORDMODE_CLAMP_BORDER, and we need
       * to manually saturate the texture coordinates.
       */
      if (sampler) {
         variant->saturate_tex_coords[0] |= sampler->saturate_s << i;
         variant->saturate_tex_coords[1] |= sampler->saturate_t << i;
         variant->saturate_tex_coords[2] |= sampler->saturate_r << i;
      }
   }
}

/**
 * Guess the shader variant, knowing that the context may still change.
 */
static void
ilo_shader_variant_guess(struct ilo_shader_variant *variant,
                         const struct ilo_shader_info *info,
                         const struct ilo_context *ilo)
{
   int i;

   memset(variant, 0, sizeof(*variant));

   switch (info->type) {
   case PIPE_SHADER_VERTEX:
      break;
   case PIPE_SHADER_GEOMETRY:
      break;
   case PIPE_SHADER_FRAGMENT:
      variant->u.fs.flatshade = false;
      variant->u.fs.fb_height = (info->has_pos) ?
         ilo->fb.state.height : 1;
      variant->u.fs.num_cbufs = 1;
      break;
   default:
      assert(!"unknown shader type");
      break;
   }

   variant->num_sampler_views = info->num_samplers;
   for (i = 0; i < info->num_samplers; i++) {
      if (info->shadow_samplers & (1 << i)) {
         variant->sampler_view_swizzles[i].r = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].g = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].b = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].a = PIPE_SWIZZLE_ONE;
      }
      else {
         variant->sampler_view_swizzles[i].r = PIPE_SWIZZLE_RED;
         variant->sampler_view_swizzles[i].g = PIPE_SWIZZLE_GREEN;
         variant->sampler_view_swizzles[i].b = PIPE_SWIZZLE_BLUE;
         variant->sampler_view_swizzles[i].a = PIPE_SWIZZLE_ALPHA;
      }
   }
}


/**
 * Parse a TGSI instruction for the shader info.
 */
static void
ilo_shader_info_parse_inst(struct ilo_shader_info *info,
                           const struct tgsi_full_instruction *inst)
{
   int i;

   /* look for edgeflag passthrough */
   if (info->edgeflag_out >= 0 &&
       inst->Instruction.Opcode == TGSI_OPCODE_MOV &&
       inst->Dst[0].Register.File == TGSI_FILE_OUTPUT &&
       inst->Dst[0].Register.Index == info->edgeflag_out) {

      assert(inst->Src[0].Register.File == TGSI_FILE_INPUT);
      info->edgeflag_in = inst->Src[0].Register.Index;
   }

   if (inst->Instruction.Texture) {
      bool shadow;

      switch (inst->Texture.Texture) {
      case TGSI_TEXTURE_SHADOW1D:
      case TGSI_TEXTURE_SHADOW2D:
      case TGSI_TEXTURE_SHADOWRECT:
      case TGSI_TEXTURE_SHADOW1D_ARRAY:
      case TGSI_TEXTURE_SHADOW2D_ARRAY:
      case TGSI_TEXTURE_SHADOWCUBE:
      case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
         shadow = true;
         break;
      default:
         shadow = false;
         break;
      }

      for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
         const struct tgsi_full_src_register *src = &inst->Src[i];

         if (src->Register.File == TGSI_FILE_SAMPLER) {
            const int idx = src->Register.Index;

            if (idx >= info->num_samplers)
               info->num_samplers = idx + 1;

            if (shadow)
               info->shadow_samplers |= 1 << idx;
         }
      }
   }
}

/**
 * Parse a TGSI property for the shader info.
 */
static void
ilo_shader_info_parse_prop(struct ilo_shader_info *info,
                           const struct tgsi_full_property *prop)
{
   switch (prop->Property.PropertyName) {
   case TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS:
      info->fs_color0_writes_all_cbufs = prop->u[0].Data;
      break;
   default:
      break;
   }
}

/**
 * Parse a TGSI declaration for the shader info.
 */
static void
ilo_shader_info_parse_decl(struct ilo_shader_info *info,
                           const struct tgsi_full_declaration *decl)
{
   switch (decl->Declaration.File) {
   case TGSI_FILE_INPUT:
      if (decl->Declaration.Interpolate &&
          decl->Interp.Interpolate == TGSI_INTERPOLATE_COLOR)
         info->has_color_interp = true;
      if (decl->Declaration.Semantic &&
          decl->Semantic.Name == TGSI_SEMANTIC_POSITION)
         info->has_pos = true;
      break;
   case TGSI_FILE_OUTPUT:
      if (decl->Declaration.Semantic &&
          decl->Semantic.Name == TGSI_SEMANTIC_EDGEFLAG)
         info->edgeflag_out = decl->Range.First;
      break;
   case TGSI_FILE_SYSTEM_VALUE:
      if (decl->Declaration.Semantic &&
          decl->Semantic.Name == TGSI_SEMANTIC_INSTANCEID)
         info->has_instanceid = true;
      if (decl->Declaration.Semantic &&
          decl->Semantic.Name == TGSI_SEMANTIC_VERTEXID)
         info->has_vertexid = true;
      break;
   default:
      break;
   }
}

static void
ilo_shader_info_parse_tokens(struct ilo_shader_info *info)
{
   struct tgsi_parse_context parse;

   info->edgeflag_in = -1;
   info->edgeflag_out = -1;

   tgsi_parse_init(&parse, info->tokens);
   while (!tgsi_parse_end_of_tokens(&parse)) {
      const union tgsi_full_token *token;

      tgsi_parse_token(&parse);
      token = &parse.FullToken;

      switch (token->Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         ilo_shader_info_parse_decl(info, &token->FullDeclaration);
         break;
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         ilo_shader_info_parse_inst(info, &token->FullInstruction);
         break;
      case TGSI_TOKEN_TYPE_PROPERTY:
         ilo_shader_info_parse_prop(info, &token->FullProperty);
         break;
      default:
         break;
      }
   }
   tgsi_parse_free(&parse);
}

/**
 * Create a shader state.
 */
struct ilo_shader_state *
ilo_shader_state_create(const struct ilo_context *ilo,
                        int type, const void *templ)
{
   struct ilo_shader_state *state;
   struct ilo_shader_variant variant;

   state = CALLOC_STRUCT(ilo_shader_state);
   if (!state)
      return NULL;

   state->info.dev = ilo->dev;
   state->info.type = type;

   if (type == PIPE_SHADER_COMPUTE) {
      const struct pipe_compute_state *c =
         (const struct pipe_compute_state *) templ;

      state->info.tokens = tgsi_dup_tokens(c->prog);
      state->info.compute.req_local_mem = c->req_local_mem;
      state->info.compute.req_private_mem = c->req_private_mem;
      state->info.compute.req_input_mem = c->req_input_mem;
   }
   else {
      const struct pipe_shader_state *s =
         (const struct pipe_shader_state *) templ;

      state->info.tokens = tgsi_dup_tokens(s->tokens);
      state->info.stream_output = s->stream_output;
   }

   list_inithead(&state->variants);

   ilo_shader_info_parse_tokens(&state->info);

   /* guess and compile now */
   ilo_shader_variant_guess(&variant, &state->info, ilo);
   if (!ilo_shader_state_use_variant(state, &variant)) {
      ilo_shader_state_destroy(state);
      return NULL;
   }

   return state;
}

/**
 * Destroy a shader state.
 */
void
ilo_shader_state_destroy(struct ilo_shader_state *state)
{
   struct ilo_shader *sh, *next;

   LIST_FOR_EACH_ENTRY_SAFE(sh, next, &state->variants, list)
      ilo_shader_destroy(sh);

   FREE((struct tgsi_token *) state->info.tokens);
   FREE(state);
}

/**
 * Add a compiled shader to the shader state.
 */
static void
ilo_shader_state_add_shader(struct ilo_shader_state *state,
                            struct ilo_shader *sh)
{
   list_add(&sh->list, &state->variants);
   state->num_variants++;
   state->total_size += sh->kernel_size;
}

/**
 * Remove a compiled shader from the shader state.
 */
static void
ilo_shader_state_remove_shader(struct ilo_shader_state *state,
                               struct ilo_shader *sh)
{
   list_del(&sh->list);
   state->num_variants--;
   state->total_size -= sh->kernel_size;
}

/**
 * Garbage collect shader variants in the shader state.
 */
static void
ilo_shader_state_gc(struct ilo_shader_state *state)
{
   /* activate when the variants take up more than 4KiB of space */
   const int limit = 4 * 1024;
   struct ilo_shader *sh, *next;

   if (state->total_size < limit)
      return;

   /* remove from the tail as the most recently ones are at the head */
   LIST_FOR_EACH_ENTRY_SAFE_REV(sh, next, &state->variants, list) {
      ilo_shader_state_remove_shader(state, sh);
      ilo_shader_destroy(sh);

      if (state->total_size <= limit / 2)
         break;
   }
}

/**
 * Search for a shader variant.
 */
static struct ilo_shader *
ilo_shader_state_search_variant(struct ilo_shader_state *state,
                                const struct ilo_shader_variant *variant)
{
   struct ilo_shader *sh = NULL, *tmp;

   LIST_FOR_EACH_ENTRY(tmp, &state->variants, list) {
      if (memcmp(&tmp->variant, variant, sizeof(*variant)) == 0) {
         sh = tmp;
         break;
      }
   }

   return sh;
}

/**
 * Add a shader variant to the shader state.
 */
struct ilo_shader *
ilo_shader_state_add_variant(struct ilo_shader_state *state,
                             const struct ilo_shader_variant *variant)
{
   struct ilo_shader *sh;

   sh = ilo_shader_state_search_variant(state, variant);
   if (sh)
      return sh;

   ilo_shader_state_gc(state);

   switch (state->info.type) {
   case PIPE_SHADER_VERTEX:
      sh = ilo_shader_compile_vs(state, variant);
      break;
   case PIPE_SHADER_FRAGMENT:
      sh = ilo_shader_compile_fs(state, variant);
      break;
   case PIPE_SHADER_GEOMETRY:
      sh = ilo_shader_compile_gs(state, variant);
      break;
   case PIPE_SHADER_COMPUTE:
      sh = ilo_shader_compile_cs(state, variant);
      break;
   default:
      sh = NULL;
      break;
   }
   if (!sh) {
      assert(!"failed to compile shader");
      return NULL;
   }

   sh->variant = *variant;

   ilo_shader_state_add_shader(state, sh);

   return sh;
}

/**
 * Update state->shader to point to a variant.  If the variant does not exist,
 * it will be added first.
 */
bool
ilo_shader_state_use_variant(struct ilo_shader_state *state,
                             const struct ilo_shader_variant *variant)
{
   struct ilo_shader *sh;

   sh = ilo_shader_state_add_variant(state, variant);
   if (!sh)
      return false;

   /* move to head */
   if (state->variants.next != &sh->list) {
      list_del(&sh->list);
      list_add(&sh->list, &state->variants);
   }

   state->shader = sh;

   return true;
}

/**
 * Reset the shader cache.
 */
static void
ilo_shader_cache_reset(struct ilo_shader_cache *shc)
{
   if (shc->bo)
      shc->bo->unreference(shc->bo);

   shc->bo = shc->winsys->alloc_buffer(shc->winsys,
         "shader cache", shc->size, 0);
   shc->busy = false;
   shc->cur = 0;
   shc->seqno++;
   if (!shc->seqno)
      shc->seqno = 1;
}

/**
 * Create a shader cache.  A shader cache is a bo holding all compiled shaders.
 * When the bo is full, a larger bo is allocated and all cached shaders are
 * invalidated.  This is how outdated shaders get dropped.  Active shaders
 * will be added to the new bo when used.
 */
struct ilo_shader_cache *
ilo_shader_cache_create(struct intel_winsys *winsys)
{
   struct ilo_shader_cache *shc;

   shc = CALLOC_STRUCT(ilo_shader_cache);
   if (!shc)
      return NULL;

   shc->winsys = winsys;
   /* initial cache size */
   shc->size = 4096;

   ilo_shader_cache_reset(shc);

   return shc;
}

/**
 * Destroy a shader cache.
 */
void
ilo_shader_cache_destroy(struct ilo_shader_cache *shc)
{
   if (shc->bo)
      shc->bo->unreference(shc->bo);

   FREE(shc);
}

/**
 * Add shaders to the cache.  This may invalidate all other shaders in the
 * cache.
 */
void
ilo_shader_cache_set(struct ilo_shader_cache *shc,
                     struct ilo_shader **shaders,
                     int num_shaders)
{
   int new_cur, i;

   /* calculate the space needed */
   new_cur = shc->cur;
   for (i = 0; i < num_shaders; i++) {
      if (shaders[i]->cache_seqno != shc->seqno)
         new_cur = align(new_cur, 64) + shaders[i]->kernel_size;
   }

   /* all shaders are already in the cache */
   if (new_cur == shc->cur)
      return;

   /*
    * From the Sandy Bridge PRM, volume 4 part 2, page 112:
    *
    *     "Due to prefetch of the instruction stream, the EUs may attempt to
    *      access up to 8 instructions (128 bytes) beyond the end of the kernel
    *      program - possibly into the next memory page.  Although these
    *      instructions will not be executed, software must account for the
    *      prefetch in order to avoid invalid page access faults."
    */
   new_cur += 128;

   /*
    * we should be able to append data without being blocked even the bo
    * is busy...
    */

   /* reallocate when the cache is full or busy */
   if (new_cur > shc->size || shc->busy) {
      while (new_cur > shc->size)
         shc->size <<= 1;

      ilo_shader_cache_reset(shc);
   }

   /* upload now */
   for (i = 0; i < num_shaders; i++) {
      if (shaders[i]->cache_seqno != shc->seqno) {
         /* kernels must be aligned to 64-byte */
         shc->cur = align(shc->cur, 64);
         shc->bo->pwrite(shc->bo, shc->cur,
               shaders[i]->kernel_size, shaders[i]->kernel);

         shaders[i]->cache_seqno = shc->seqno;
         shaders[i]->cache_offset = shc->cur;

         shc->cur += shaders[i]->kernel_size;
      }
   }
}
