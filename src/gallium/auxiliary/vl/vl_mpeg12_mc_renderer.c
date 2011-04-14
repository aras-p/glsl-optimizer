/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <assert.h>

#include <pipe/p_context.h>

#include <util/u_sampler.h>
#include <util/u_draw.h>

#include <tgsi/tgsi_ureg.h>

#include "vl_defines.h"
#include "vl_vertex_buffers.h"
#include "vl_mpeg12_mc_renderer.h"

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_LINE,
   VS_O_TEX_TOP,
   VS_O_TEX_BOTTOM,
   VS_O_MV_TOP,
   VS_O_MV_BOTTOM
};

static void *
create_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src block_scale, mv_scale;
   struct ureg_src vrect, vpos, eb, flags, vmv[2];
   struct ureg_dst t_vpos, t_vtex, t_vmv;
   struct ureg_dst o_vpos, o_line, o_vtex[2], o_vmv[2];
   unsigned i, label;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   t_vpos = ureg_DECL_temporary(shader);
   t_vtex = ureg_DECL_temporary(shader);
   t_vmv = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);
   eb = ureg_DECL_vs_input(shader, VS_I_EB);
   flags = ureg_DECL_vs_input(shader, VS_I_FLAGS);
   vmv[0] = ureg_DECL_vs_input(shader, VS_I_MV_TOP);
   vmv[1] = ureg_DECL_vs_input(shader, VS_I_MV_BOTTOM);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_line = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE);
   o_vtex[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX_TOP);
   o_vtex[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX_BOTTOM);
   o_vmv[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV_TOP);
   o_vmv[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV_BOTTOM);

   /*
    * block_scale = (MACROBLOCK_WIDTH, MACROBLOCK_HEIGHT) / (dst.width, dst.height)
    * mv_scale = 0.5 / (dst.width, dst.height);
    *
    * t_vpos = (vpos + vrect) * block_scale
    * o_vpos.xy = t_vpos
    * o_vpos.zw = vpos
    *
    * o_eb[0..1] = vrect.x ? eb[0..1][1] : eb[0..1][0]
    *
    * o_frame_pred = frame_pred
    * o_info.x = not_intra
    * o_info.y = ref_weight / 2
    *
    * // Apply motion vectors
    * o_vmv[0..3] = t_vpos + vmv[0..3] * mv_scale
    *
    * o_line.xy = vrect * 8
    * o_line.z = interlaced
    *
    * if(eb[0][0].w) { //interlaced
    *    t_vtex.x = vrect.x
    *    t_vtex.y = vrect.y * 0.5
    *    t_vtex += vpos
    *
    *    o_vtex[0].xy = t_vtex * block_scale
    *
    *    t_vtex.y += 0.5
    *    o_vtex[1].xy = t_vtex * block_scale
    * } else {
    *    o_vtex[0..1].xy = t_vpos
    * }
    * o_vtex[2].xy = t_vpos
    *
    */
   block_scale = ureg_imm2f(shader,
      (float)MACROBLOCK_WIDTH / r->buffer_width,
      (float)MACROBLOCK_HEIGHT / r->buffer_height);

   mv_scale = ureg_imm4f(shader,
      0.5f / r->buffer_width,
      0.5f / r->buffer_height,
      1.0f,
      1.0f / 255.0f);

   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), block_scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   for (i = 0; i < 2; ++i) {
      ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), mv_scale, vmv[i], ureg_src(t_vpos));
      ureg_MUL(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_W), mv_scale, vmv[i]);
   }

   ureg_MOV(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_CMP(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_Z),
            ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_X)),
            ureg_scalar(eb, TGSI_SWIZZLE_Y),
            ureg_scalar(eb, TGSI_SWIZZLE_X));

   ureg_MOV(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_CMP(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_Z),
            ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_X)),
            ureg_scalar(eb, TGSI_SWIZZLE_W),
            ureg_scalar(eb, TGSI_SWIZZLE_Z));

   ureg_MOV(shader, ureg_writemask(o_line, TGSI_WRITEMASK_X), ureg_scalar(vrect, TGSI_SWIZZLE_Y));
   ureg_MUL(shader, ureg_writemask(o_line, TGSI_WRITEMASK_Y),
      vrect, ureg_imm1f(shader, MACROBLOCK_HEIGHT / 2));
   ureg_MOV(shader, ureg_writemask(o_line, TGSI_WRITEMASK_Z),
            ureg_scalar(flags, TGSI_SWIZZLE_Y));

   ureg_IF(shader, ureg_scalar(flags, TGSI_SWIZZLE_X), &label);

      ureg_MOV(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_X), vrect);
      ureg_MUL(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), vrect, ureg_imm1f(shader, 0.5f));
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_XY), vpos, ureg_src(t_vtex));
      ureg_MUL(shader, ureg_writemask(o_vtex[0], TGSI_WRITEMASK_XY), ureg_src(t_vtex), block_scale);
      ureg_ADD(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y), ureg_src(t_vtex), ureg_imm1f(shader, 0.5f));
      ureg_MUL(shader, ureg_writemask(o_vtex[1], TGSI_WRITEMASK_XY), ureg_src(t_vtex), block_scale);

      ureg_MUL(shader, ureg_writemask(o_line, TGSI_WRITEMASK_X),
         ureg_scalar(vrect, TGSI_SWIZZLE_Y),
         ureg_imm1f(shader, MACROBLOCK_HEIGHT / 2));

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ENDIF(shader);

   ureg_release_temporary(shader, t_vtex);
   ureg_release_temporary(shader, t_vpos);
   ureg_release_temporary(shader, t_vmv);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static struct ureg_dst
calc_field(struct ureg_program *shader)
{
   struct ureg_dst tmp;
   struct ureg_src line;

   tmp = ureg_DECL_temporary(shader);

   line = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_LINE, TGSI_INTERPOLATE_LINEAR);

   /*
    * line.x going from 0 to 1 if not interlaced
    * line.x going from 0 to 8 in steps of 0.5 if interlaced
    * line.y going from 0 to 8 in steps of 0.5
    * line.z is flag for intra frames
    *
    * tmp.xy = fraction(line)
    * tmp.xy = tmp.xy >= 0.5 ? 1 : 0
    */
   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY), line);
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY), ureg_src(tmp), ureg_imm1f(shader, 0.5f));
   ureg_MOV(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Z), line);

   return tmp;
}

static void *
create_ycbcr_frag_shader(struct vl_mpeg12_mc_renderer *r, float scale)
{
   struct ureg_program *shader;
   struct ureg_src tc[2], sampler;
   struct ureg_dst texel, t_tc, field;
   struct ureg_dst fragment;
   unsigned label;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX_TOP, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX_BOTTOM, TGSI_INTERPOLATE_LINEAR);

   sampler = ureg_DECL_sampler(shader, 0);

   t_tc = ureg_DECL_temporary(shader);
   texel = ureg_DECL_temporary(shader);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   field = calc_field(shader);

   /*
    * texel.y  = tex(field.y ? tc[1] : tc[0], sampler[0])
    * texel.cb = tex(tc[2], sampler[1])
    * texel.cr = tex(tc[2], sampler[2])
    */

   ureg_CMP(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_XYZ),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_X)),
            tc[1], tc[0]);

   ureg_SLT(shader, ureg_writemask(t_tc, TGSI_WRITEMASK_Z), ureg_src(t_tc), ureg_imm1f(shader, 0.5f));

   ureg_MOV(shader, fragment, ureg_imm4f(shader, 0.0f, 0.0f, 0.0f, 1.0f));
   ureg_IF(shader, ureg_scalar(ureg_src(t_tc), TGSI_SWIZZLE_Z), &label);

      ureg_TEX(shader, texel, TGSI_TEXTURE_3D, ureg_src(t_tc), sampler);

      ureg_CMP(shader, t_tc, ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Z)),
               ureg_imm1f(shader, 0.0f), ureg_imm1f(shader, 0.5f));

      if (scale != 1.0f)
         ureg_MAD(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ),
                  ureg_src(texel), ureg_imm1f(shader, scale), ureg_src(t_tc));
      else
         ureg_ADD(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ),
                  ureg_src(texel), ureg_src(t_tc));

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ENDIF(shader);

   ureg_release_temporary(shader, t_tc);
   ureg_release_temporary(shader, texel);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_ref_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   struct ureg_program *shader;
   struct ureg_src tc[2], sampler;
   struct ureg_dst ref, field;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV_TOP, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_MV_BOTTOM, TGSI_INTERPOLATE_LINEAR);

   sampler = ureg_DECL_sampler(shader, 0);
   ref = ureg_DECL_temporary(shader);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   field = calc_field(shader);

   /*
    * if (field.z)
    *    ref[0..1] = tex(tc[0..1], sampler[0..1])
    * else
    *    ref[0..1] = tex(tc[2..3], sampler[0..1])
    * result = LRP(info.y, ref[0..1])
    */
   ureg_CMP(shader, ref, ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Y)), tc[1], tc[0]);

   ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W), ureg_src(ref));
   ureg_TEX(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), TGSI_TEXTURE_2D, ureg_src(ref), sampler);

   ureg_release_temporary(shader, ref);

   ureg_release_temporary(shader, field);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static bool
init_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_sampler_state sampler;
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;

   assert(r);

   r->viewport.scale[2] = 1;
   r->viewport.scale[3] = 1;
   r->viewport.translate[0] = 0;
   r->viewport.translate[1] = 0;
   r->viewport.translate[2] = 0;
   r->viewport.translate[3] = 0;

   r->fb_state.nr_cbufs = 1;
   r->fb_state.zsbuf = NULL;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   r->sampler_ref = r->pipe->create_sampler_state(r->pipe, &sampler);
   if (!r->sampler_ref)
      goto error_sampler_ref;

   r->sampler_ycbcr = r->pipe->create_sampler_state(r->pipe, &sampler);
   if (!r->sampler_ycbcr)
      goto error_sampler_ycbcr;

   memset(&blend, 0, sizeof blend);
   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   r->blend_clear = r->pipe->create_blend_state(r->pipe, &blend);
   if (!r->blend_clear)
      goto error_blend_clear;

   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   r->blend_add = r->pipe->create_blend_state(r->pipe, &blend);
   if (!r->blend_add)
      goto error_blend_add;

   memset(&rs_state, 0, sizeof(rs_state));
   /*rs_state.sprite_coord_enable */
   rs_state.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
   rs_state.point_quad_rasterization = true;
   rs_state.point_size = BLOCK_WIDTH;
   rs_state.gl_rasterization_rules = true;
   r->rs_state = r->pipe->create_rasterizer_state(r->pipe, &rs_state);
   if (!r->rs_state)
      goto error_rs_state;

   return true;

error_rs_state:
   r->pipe->delete_blend_state(r->pipe, r->blend_add);

error_blend_add:
   r->pipe->delete_blend_state(r->pipe, r->blend_clear);

error_blend_clear:
   r->pipe->delete_sampler_state(r->pipe, r->sampler_ref);

error_sampler_ref:
   r->pipe->delete_sampler_state(r->pipe, r->sampler_ycbcr);

error_sampler_ycbcr:
   return false;
}

static void
cleanup_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   assert(r);

   r->pipe->delete_sampler_state(r->pipe, r->sampler_ref);
   r->pipe->delete_sampler_state(r->pipe, r->sampler_ycbcr);
   r->pipe->delete_blend_state(r->pipe, r->blend_clear);
   r->pipe->delete_blend_state(r->pipe, r->blend_add);
   r->pipe->delete_rasterizer_state(r->pipe, r->rs_state);
}

bool
vl_mc_init(struct vl_mpeg12_mc_renderer *renderer, struct pipe_context *pipe,
           unsigned buffer_width, unsigned buffer_height, float scale)
{
   struct pipe_resource tex_templ, *tex_dummy;
   struct pipe_sampler_view sampler_view;

   assert(renderer);
   assert(pipe);

   memset(renderer, 0, sizeof(struct vl_mpeg12_mc_renderer));

   renderer->pipe = pipe;
   renderer->buffer_width = buffer_width;
   renderer->buffer_height = buffer_height;

   if (!init_pipe_state(renderer))
      goto error_pipe_state;

   renderer->vs = create_vert_shader(renderer);
   if (!renderer->vs)
      goto error_vs_shaders;

   renderer->fs_ref = create_ref_frag_shader(renderer);
   if (!renderer->fs_ref)
      goto error_fs_ref_shaders;

   renderer->fs_ycbcr = create_ycbcr_frag_shader(renderer, scale);
   if (!renderer->fs_ycbcr)
      goto error_fs_ycbcr_shaders;

   /* create a dummy sampler */
   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   tex_templ.flags = 0;

   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = PIPE_FORMAT_R8_SNORM;
   tex_templ.width0 = 1;
   tex_templ.height0 = 1;
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.last_level = 0;
   tex_templ.usage = PIPE_USAGE_STATIC;
   tex_dummy = pipe->screen->resource_create(pipe->screen, &tex_templ);
   if (!tex_dummy)
      goto error_dummy;

   memset(&sampler_view, 0, sizeof(sampler_view));
   u_sampler_view_default_template(&sampler_view, tex_dummy, tex_dummy->format);
   renderer->dummy = pipe->create_sampler_view(pipe, tex_dummy, &sampler_view);
   pipe_resource_reference(&tex_dummy, NULL);
   if (!renderer->dummy)
      goto error_dummy;

   return true;

error_dummy:
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr);

error_fs_ycbcr_shaders:
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ref);

error_fs_ref_shaders:
   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs);

error_vs_shaders:
   cleanup_pipe_state(renderer);

error_pipe_state:
   return false;
}

void
vl_mc_cleanup(struct vl_mpeg12_mc_renderer *renderer)
{
   assert(renderer);

   pipe_sampler_view_reference(&renderer->dummy, NULL);

   cleanup_pipe_state(renderer);

   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ref);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr);
}

bool
vl_mc_init_buffer(struct vl_mpeg12_mc_renderer *renderer, struct vl_mpeg12_mc_buffer *buffer,
                  struct pipe_sampler_view *source)
{
   assert(renderer && buffer);
   assert(source);

   buffer->renderer = renderer;

   pipe_sampler_view_reference(&buffer->source, source);

   return true;
}

void
vl_mc_cleanup_buffer(struct vl_mpeg12_mc_buffer *buffer)
{
   assert(buffer);

   pipe_sampler_view_reference(&buffer->source, NULL);
}

void
vl_mc_set_surface(struct vl_mpeg12_mc_renderer *renderer, struct pipe_surface *surface)
{
   assert(renderer && surface);

   renderer->viewport.scale[0] = surface->width;
   renderer->viewport.scale[1] = surface->height;

   renderer->fb_state.width = surface->width;
   renderer->fb_state.height = surface->height;
   renderer->fb_state.cbufs[0] = surface;
}

void
vl_mc_render_ref(struct vl_mpeg12_mc_buffer *buffer,
                 struct pipe_sampler_view *ref, bool first,
                 unsigned not_empty_start_instance, unsigned not_empty_num_instances,
                 unsigned empty_start_instance, unsigned empty_num_instances)
{
   struct vl_mpeg12_mc_renderer *renderer;

   assert(buffer && ref);

   if (not_empty_num_instances == 0 && empty_num_instances == 0)
      return;

   renderer = buffer->renderer;
   renderer->pipe->bind_rasterizer_state(renderer->pipe, renderer->rs_state);
   renderer->pipe->set_framebuffer_state(renderer->pipe, &renderer->fb_state);
   renderer->pipe->set_viewport_state(renderer->pipe, &renderer->viewport);
   renderer->pipe->bind_blend_state(renderer->pipe, first ? renderer->blend_clear : renderer->blend_add);

   renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ref);

   renderer->pipe->set_fragment_sampler_views(renderer->pipe, 1, &ref);
   renderer->pipe->bind_fragment_sampler_states(renderer->pipe, 1, &renderer->sampler_ref);

   if (not_empty_num_instances > 0)
      util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4,
                                 not_empty_start_instance, not_empty_num_instances);

   if (empty_num_instances > 0)
      util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4,
                                 empty_start_instance, empty_num_instances);
}

void
vl_mc_render_ycbcr(struct vl_mpeg12_mc_buffer *buffer, bool first,
                   unsigned not_empty_start_instance, unsigned not_empty_num_instances)
{
   struct vl_mpeg12_mc_renderer *renderer;

   assert(buffer);

   if (not_empty_num_instances == 0)
      return;

   renderer = buffer->renderer;
   renderer->pipe->bind_rasterizer_state(renderer->pipe, renderer->rs_state);
   renderer->pipe->set_framebuffer_state(renderer->pipe, &renderer->fb_state);
   renderer->pipe->set_viewport_state(renderer->pipe, &renderer->viewport);
   renderer->pipe->bind_blend_state(renderer->pipe, first ? renderer->blend_clear : renderer->blend_add);

   renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ycbcr);

   renderer->pipe->set_fragment_sampler_views(renderer->pipe, 1, &buffer->source);
   renderer->pipe->bind_fragment_sampler_states(renderer->pipe, 1, &renderer->sampler_ycbcr);

   util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4,
                              not_empty_start_instance, not_empty_num_instances);
}
