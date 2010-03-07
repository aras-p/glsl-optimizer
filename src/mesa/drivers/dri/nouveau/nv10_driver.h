/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NV10_DRIVER_H__
#define __NV10_DRIVER_H__

#define NV10_TEXTURE_UNITS 2

/* nv10_context.c */
extern const struct nouveau_driver nv10_driver;

/* nv10_render.c */
void
nv10_render_init(GLcontext *ctx);

void
nv10_render_destroy(GLcontext *ctx);

/* nv10_state_fb.c */
void
nv10_emit_framebuffer(GLcontext *ctx, int emit);

void
nv10_emit_render_mode(GLcontext *ctx, int emit);

void
nv10_emit_scissor(GLcontext *ctx, int emit);

void
nv10_emit_viewport(GLcontext *ctx, int emit);

/* nv10_state_polygon.c */
void
nv10_emit_cull_face(GLcontext *ctx, int emit);

void
nv10_emit_front_face(GLcontext *ctx, int emit);

void
nv10_emit_line_mode(GLcontext *ctx, int emit);

void
nv10_emit_line_stipple(GLcontext *ctx, int emit);

void
nv10_emit_point_mode(GLcontext *ctx, int emit);

void
nv10_emit_polygon_mode(GLcontext *ctx, int emit);

void
nv10_emit_polygon_offset(GLcontext *ctx, int emit);

void
nv10_emit_polygon_stipple(GLcontext *ctx, int emit);

/* nv10_state_raster.c */
void
nv10_emit_alpha_func(GLcontext *ctx, int emit);

void
nv10_emit_blend_color(GLcontext *ctx, int emit);

void
nv10_emit_blend_equation(GLcontext *ctx, int emit);

void
nv10_emit_blend_func(GLcontext *ctx, int emit);

void
nv10_emit_color_mask(GLcontext *ctx, int emit);

void
nv10_emit_depth(GLcontext *ctx, int emit);

void
nv10_emit_dither(GLcontext *ctx, int emit);

void
nv10_emit_logic_opcode(GLcontext *ctx, int emit);

void
nv10_emit_shade_model(GLcontext *ctx, int emit);

void
nv10_emit_stencil_func(GLcontext *ctx, int emit);

void
nv10_emit_stencil_mask(GLcontext *ctx, int emit);

void
nv10_emit_stencil_op(GLcontext *ctx, int emit);

/* nv10_state_frag.c */
void
nv10_get_general_combiner(GLcontext *ctx, int i,
			  uint32_t *a_in, uint32_t *a_out,
			  uint32_t *c_in, uint32_t *c_out, uint32_t *k);

void
nv10_get_final_combiner(GLcontext *ctx, uint64_t *in, int *n);

void
nv10_emit_tex_env(GLcontext *ctx, int emit);

void
nv10_emit_frag(GLcontext *ctx, int emit);

/* nv10_state_tex.c */
void
nv10_emit_tex_gen(GLcontext *ctx, int emit);

void
nv10_emit_tex_obj(GLcontext *ctx, int emit);

/* nv10_state_tnl.c */
void
nv10_get_fog_coeff(GLcontext *ctx, float k[3]);

void
nv10_get_spot_coeff(struct gl_light *l, float k[7]);

void
nv10_get_shininess_coeff(float s, float k[6]);

void
nv10_emit_clip_plane(GLcontext *ctx, int emit);

void
nv10_emit_color_material(GLcontext *ctx, int emit);

void
nv10_emit_fog(GLcontext *ctx, int emit);

void
nv10_emit_light_enable(GLcontext *ctx, int emit);

void
nv10_emit_light_model(GLcontext *ctx, int emit);

void
nv10_emit_light_source(GLcontext *ctx, int emit);

void
nv10_emit_material_ambient(GLcontext *ctx, int emit);

void
nv10_emit_material_diffuse(GLcontext *ctx, int emit);

void
nv10_emit_material_specular(GLcontext *ctx, int emit);

void
nv10_emit_material_shininess(GLcontext *ctx, int emit);

void
nv10_emit_modelview(GLcontext *ctx, int emit);

void
nv10_emit_point_parameter(GLcontext *ctx, int emit);

void
nv10_emit_projection(GLcontext *ctx, int emit);

#endif
