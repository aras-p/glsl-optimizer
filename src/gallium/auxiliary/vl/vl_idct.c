/**************************************************************************
 *
 * Copyright 2010 Christian König
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

#include "vl_idct.h"
#include "util/u_draw.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_inlines.h>
#include <util/u_sampler.h>
#include <util/u_format.h>
#include <tgsi/tgsi_ureg.h>
#include "vl_types.h"

#define BLOCK_WIDTH 8
#define BLOCK_HEIGHT 8

#define SCALE_FACTOR_16_TO_9 (32768.0f / 256.0f)

#define STAGE1_SCALE 4.0f
#define STAGE2_SCALE (SCALE_FACTOR_16_TO_9 / STAGE1_SCALE)

struct vertex_shader_consts
{
   struct vertex4f norm;
};

enum VS_INPUT
{
   VS_I_RECT,
   VS_I_VPOS,

   NUM_VS_INPUTS
};

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_BLOCK,
   VS_O_TEX,
   VS_O_START
};

static const float const_matrix[8][8] = {
   {  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.3535530f,  0.353553f,  0.3535530f },
   {  0.4903930f,  0.4157350f,  0.2777850f,  0.0975451f, -0.0975452f, -0.2777850f, -0.415735f, -0.4903930f },
   {  0.4619400f,  0.1913420f, -0.1913420f, -0.4619400f, -0.4619400f, -0.1913420f,  0.191342f,  0.4619400f },
   {  0.4157350f, -0.0975452f, -0.4903930f, -0.2777850f,  0.2777850f,  0.4903930f,  0.097545f, -0.4157350f },
   {  0.3535530f, -0.3535530f, -0.3535530f,  0.3535540f,  0.3535530f, -0.3535540f, -0.353553f,  0.3535530f },
   {  0.2777850f, -0.4903930f,  0.0975452f,  0.4157350f, -0.4157350f, -0.0975451f,  0.490393f, -0.2777850f },
   {  0.1913420f, -0.4619400f,  0.4619400f, -0.1913420f, -0.1913410f,  0.4619400f, -0.461940f,  0.1913420f },
   {  0.0975451f, -0.2777850f,  0.4157350f, -0.4903930f,  0.4903930f, -0.4157350f,  0.277786f, -0.0975458f }
};

/* vertices for a quad covering a block */
static const struct vertex2f const_quad[4] = {
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

static void *
create_vert_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_src scale;
   struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos;
   struct ureg_dst o_vpos, o_block, o_tex, o_start;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->destination->width0, 
      (float)BLOCK_HEIGHT / idct->destination->height0);

   t_vpos = ureg_DECL_temporary(shader);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_block = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK);
   o_tex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX);
   o_start = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_START);

   /*
    * t_vpos = vpos + vrect
    * o_vpos.xy = t_vpos * scale
    * o_vpos.zw = vpos
    *
    * o_block = vrect
    * o_tex = t_pos
    * o_start = vpos * scale
    *
    */
   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), vpos);

   ureg_MOV(shader, ureg_writemask(o_block, TGSI_WRITEMASK_XY), vrect);
   ureg_MOV(shader, ureg_writemask(o_tex, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MUL(shader, ureg_writemask(o_start, TGSI_WRITEMASK_XY), vpos, scale);

   ureg_release_temporary(shader, t_vpos);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void
matrix_mul(struct ureg_program *shader, struct ureg_dst dst,
           struct ureg_src tc[2], struct ureg_src sampler[2],
           struct ureg_src start[2], struct ureg_src step[2],
           bool fetch4[2], float scale)
{
   struct ureg_dst t_tc[2], m[2][2], tmp[2];
   unsigned side, i, j;

   for(i = 0; i < 2; ++i) {
      t_tc[i] = ureg_DECL_temporary(shader);
      for(j = 0; j < 2; ++j)
         m[i][j] = ureg_DECL_temporary(shader);
      tmp[i] = ureg_DECL_temporary(shader);
   }

   /*
    * m[0..1][0] = ?
    * tmp[0..1] = dot4(m[0..1][0], m[0..1][1])
    * fragment = tmp[0] + tmp[1]
    */
   ureg_MOV(shader, ureg_writemask(t_tc[0], TGSI_WRITEMASK_X), ureg_scalar(start[0], TGSI_SWIZZLE_X));
   ureg_MOV(shader, ureg_writemask(t_tc[0], TGSI_WRITEMASK_Y), ureg_scalar(tc[0], TGSI_SWIZZLE_Y));

   if(fetch4[1]) {
      ureg_MOV(shader, ureg_writemask(t_tc[1], TGSI_WRITEMASK_X), ureg_scalar(start[1], TGSI_SWIZZLE_Y));
      ureg_MOV(shader, ureg_writemask(t_tc[1], TGSI_WRITEMASK_Y), ureg_scalar(tc[1], TGSI_SWIZZLE_X));
   } else {
      ureg_MOV(shader, ureg_writemask(t_tc[1], TGSI_WRITEMASK_X), ureg_scalar(tc[1], TGSI_SWIZZLE_X));
      ureg_MOV(shader, ureg_writemask(t_tc[1], TGSI_WRITEMASK_Y), ureg_scalar(start[1], TGSI_SWIZZLE_Y));
   }

   for(side = 0; side < 2; ++side) {
      for(i = 0; i < 2; ++i) {
         if(fetch4[side]) {
            ureg_TEX(shader, m[i][side], TGSI_TEXTURE_2D, ureg_src(t_tc[side]), sampler[side]);
            ureg_MOV(shader, ureg_writemask(t_tc[side], TGSI_WRITEMASK_X), step[side]);

         } else for(j = 0; j < 4; ++j) {
            /* Nouveau and r600g can't writemask tex dst regs (yet?), do in two steps */
            ureg_TEX(shader, tmp[side], TGSI_TEXTURE_2D, ureg_src(t_tc[side]), sampler[side]);
            ureg_MOV(shader, ureg_writemask(m[i][side], TGSI_WRITEMASK_X << j), ureg_scalar(ureg_src(tmp[side]), TGSI_SWIZZLE_X));

            ureg_ADD(shader, ureg_writemask(t_tc[side], TGSI_WRITEMASK_X << side), ureg_src(t_tc[side]), step[side]);
         }
      }
   }

   ureg_DP4(shader, ureg_writemask(tmp[0], TGSI_WRITEMASK_X), ureg_src(m[0][0]), ureg_src(m[0][1]));
   ureg_DP4(shader, ureg_writemask(tmp[1], TGSI_WRITEMASK_X), ureg_src(m[1][0]), ureg_src(m[1][1]));
   ureg_ADD(shader, ureg_writemask(tmp[0], TGSI_WRITEMASK_X), ureg_src(tmp[0]), ureg_src(tmp[1]));
   ureg_MUL(shader, dst, ureg_src(tmp[0]), ureg_imm1f(shader, scale));

   for(i = 0; i < 2; ++i) {
      ureg_release_temporary(shader, t_tc[i]);
      for(j = 0; j < 2; ++j)
         ureg_release_temporary(shader, m[i][j]);
      ureg_release_temporary(shader, tmp[i]);
   }
}

static void *
create_transpose_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_src tc[2], sampler[2];
   struct ureg_src start[2], step[2];
   struct ureg_dst fragment;
   bool fetch4[2];

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX, TGSI_INTERPOLATE_LINEAR);

   start[0] = ureg_imm1f(shader, 0.0f);
   start[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_START, TGSI_INTERPOLATE_CONSTANT);

   step[0] = ureg_imm1f(shader, 4.0f / BLOCK_HEIGHT);
   step[1] = ureg_imm1f(shader, 1.0f / idct->destination->height0);

   sampler[0] = ureg_DECL_sampler(shader, 0);
   sampler[1] = ureg_DECL_sampler(shader, 1);

   fetch4[0] = true;
   fetch4[1] = false;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   matrix_mul(shader, fragment, tc, sampler, start, step, fetch4, STAGE1_SCALE);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_matrix_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_src tc[2], sampler[2];
   struct ureg_src start[2], step[2];
   struct ureg_dst fragment;
   bool fetch4[2];

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_TEX, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_BLOCK, TGSI_INTERPOLATE_LINEAR);

   start[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_START, TGSI_INTERPOLATE_CONSTANT);
   start[1] = ureg_imm1f(shader, 0.0f);

   step[0] = ureg_imm1f(shader, 1.0f / idct->destination->width0);
   step[1] = ureg_imm1f(shader, 4.0f / BLOCK_WIDTH);

   sampler[0] = ureg_DECL_sampler(shader, 1);
   sampler[1] = ureg_DECL_sampler(shader, 0);

   fetch4[0] = false;
   fetch4[1] = true;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   matrix_mul(shader, fragment, tc, sampler, start, step, fetch4, STAGE2_SCALE);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_empty_block_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_dst fragment;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   ureg_MOV(shader, fragment, ureg_imm1f(shader, 0.0f));

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void
xfer_buffers_map(struct vl_idct *idct)
{
   struct pipe_box rect =
   {
      0, 0, 0,
      idct->destination->width0,
      idct->destination->height0,
      1
   };

   idct->tex_transfer = idct->pipe->get_transfer
   (
      idct->pipe, idct->textures.individual.source,
      u_subresource(0, 0),
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &rect
   );

   idct->texels = idct->pipe->transfer_map(idct->pipe, idct->tex_transfer);

   idct->vectors = pipe_buffer_map
   (
      idct->pipe,
      idct->vertex_bufs.individual.pos.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &idct->vec_transfer
   );
}

static void
xfer_buffers_unmap(struct vl_idct *idct)
{
   pipe_buffer_unmap(idct->pipe, idct->vertex_bufs.individual.pos.buffer, idct->vec_transfer);

   idct->pipe->transfer_unmap(idct->pipe, idct->tex_transfer);
   idct->pipe->transfer_destroy(idct->pipe, idct->tex_transfer);
}

static bool
init_shaders(struct vl_idct *idct)
{
   idct->vs = create_vert_shader(idct);
   idct->transpose_fs = create_transpose_frag_shader(idct);
   idct->matrix_fs = create_matrix_frag_shader(idct);
   idct->eb_fs = create_empty_block_frag_shader(idct);

   return 
      idct->vs != NULL &&
      idct->transpose_fs != NULL &&
      idct->matrix_fs != NULL &&
      idct->eb_fs != NULL;
}

static void
cleanup_shaders(struct vl_idct *idct)
{
   idct->pipe->delete_vs_state(idct->pipe, idct->vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->transpose_fs);
   idct->pipe->delete_fs_state(idct->pipe, idct->matrix_fs);
   idct->pipe->delete_fs_state(idct->pipe, idct->eb_fs);
}

static bool
init_buffers(struct vl_idct *idct)
{
   struct pipe_resource template;
   struct pipe_sampler_view sampler_view;
   struct pipe_vertex_element vertex_elems[2];
   unsigned i;

   idct->max_blocks =
      align(idct->destination->width0, BLOCK_WIDTH) / BLOCK_WIDTH *
      align(idct->destination->height0, BLOCK_HEIGHT) / BLOCK_HEIGHT *
      idct->destination->depth0;

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   template.last_level = 0;
   template.width0 = 2;
   template.height0 = 8;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_IMMUTABLE;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   template.format = idct->destination->format;
   template.width0 = idct->destination->width0;
   template.height0 = idct->destination->height0;
   template.depth0 = idct->destination->depth0;
   template.usage = PIPE_USAGE_DYNAMIC;
   idct->textures.individual.source = idct->pipe->screen->resource_create(idct->pipe->screen, &template);

   template.usage = PIPE_USAGE_STATIC;
   idct->textures.individual.intermediate = idct->pipe->screen->resource_create(idct->pipe->screen, &template);

   for (i = 0; i < 4; ++i) {
      if(idct->textures.all[i] == NULL)
         return false; /* a texture failed to allocate */

      u_sampler_view_default_template(&sampler_view, idct->textures.all[i], idct->textures.all[i]->format);
      idct->sampler_views.all[i] = idct->pipe->create_sampler_view(idct->pipe, idct->textures.all[i], &sampler_view);
   }

   idct->vertex_bufs.individual.quad.stride = sizeof(struct vertex2f);
   idct->vertex_bufs.individual.quad.max_index = 4 * idct->max_blocks - 1;
   idct->vertex_bufs.individual.quad.buffer_offset = 0;
   idct->vertex_bufs.individual.quad.buffer = pipe_buffer_create
   (
      idct->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex2f) * 4 * idct->max_blocks
   );

   if(idct->vertex_bufs.individual.quad.buffer == NULL)
      return false;

   idct->vertex_bufs.individual.pos.stride = sizeof(struct vertex2f);
   idct->vertex_bufs.individual.pos.max_index = 4 * idct->max_blocks - 1;
   idct->vertex_bufs.individual.pos.buffer_offset = 0;
   idct->vertex_bufs.individual.pos.buffer = pipe_buffer_create
   (
      idct->pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      sizeof(struct vertex2f) * 4 * idct->max_blocks
   );

   if(idct->vertex_bufs.individual.pos.buffer == NULL)
      return false;

   /* Rect element */
   vertex_elems[0].src_offset = 0;
   vertex_elems[0].instance_divisor = 0;
   vertex_elems[0].vertex_buffer_index = 0;
   vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Pos element */
   vertex_elems[1].src_offset = 0;
   vertex_elems[1].instance_divisor = 0;
   vertex_elems[1].vertex_buffer_index = 1;
   vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   idct->vertex_elems_state = idct->pipe->create_vertex_elements_state(idct->pipe, 2, vertex_elems);

   return true;
}

static void
cleanup_buffers(struct vl_idct *idct)
{
   unsigned i;

   assert(idct);

   for (i = 0; i < 4; ++i) {
      pipe_sampler_view_reference(&idct->sampler_views.all[i], NULL);
      pipe_resource_reference(&idct->textures.all[i], NULL);
   }

   idct->pipe->delete_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
   pipe_resource_reference(&idct->vertex_bufs.individual.quad.buffer, NULL);
   pipe_resource_reference(&idct->vertex_bufs.individual.pos.buffer, NULL);
}

static void
init_constants(struct vl_idct *idct)
{
   struct pipe_transfer *buf_transfer;
   struct vertex2f *v;

   unsigned i;

   /* quad vectors */
   v = pipe_buffer_map
   (
      idct->pipe,
      idct->vertex_bufs.individual.quad.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer
   );
   for ( i = 0; i < idct->max_blocks; ++i)
     memcpy(v + i * 4, &const_quad, sizeof(const_quad));
   pipe_buffer_unmap(idct->pipe, idct->vertex_bufs.individual.quad.buffer, buf_transfer);
}

static void
init_state(struct vl_idct *idct)
{
   struct pipe_sampler_state sampler;
   unsigned i;

   idct->num_blocks = 0;
   idct->num_empty_blocks = 0;

   idct->viewport.scale[0] = idct->destination->width0;
   idct->viewport.scale[1] = idct->destination->height0;
   idct->viewport.scale[2] = 1;
   idct->viewport.scale[3] = 1;
   idct->viewport.translate[0] = 0;
   idct->viewport.translate[1] = 0;
   idct->viewport.translate[2] = 0;
   idct->viewport.translate[3] = 0;

   idct->fb_state.width = idct->destination->width0;
   idct->fb_state.height = idct->destination->height0;
   idct->fb_state.nr_cbufs = 1;
   idct->fb_state.zsbuf = NULL;

   for (i = 0; i < 4; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      /*sampler.shadow_ambient = ; */
      /*sampler.lod_bias = ; */
      sampler.min_lod = 0;
      /*sampler.max_lod = ; */
      /*sampler.border_color[0] = ; */
      /*sampler.max_anisotropy = ; */
      idct->samplers.all[i] = idct->pipe->create_sampler_state(idct->pipe, &sampler);
   }
}

static void
cleanup_state(struct vl_idct *idct)
{
   unsigned i;

   for (i = 0; i < 4; ++i)
      idct->pipe->delete_sampler_state(idct->pipe, idct->samplers.all[i]);
}

struct pipe_resource *
vl_idct_upload_matrix(struct pipe_context *pipe)
{
   struct pipe_resource template, *matrix;
   struct pipe_transfer *buf_transfer;
   unsigned i, j, pitch;
   float *f;

   struct pipe_box rect =
   {
      0, 0, 0,
      BLOCK_WIDTH,
      BLOCK_HEIGHT,
      1
   };

   memset(&template, 0, sizeof(struct pipe_resource));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   template.last_level = 0;
   template.width0 = 2;
   template.height0 = 8;
   template.depth0 = 1;
   template.usage = PIPE_USAGE_IMMUTABLE;
   template.bind = PIPE_BIND_SAMPLER_VIEW;
   template.flags = 0;

   matrix = pipe->screen->resource_create(pipe->screen, &template);

   /* matrix */
   buf_transfer = pipe->get_transfer
   (
      pipe, matrix,
      u_subresource(0, 0),
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &rect
   );
   pitch = buf_transfer->stride / util_format_get_blocksize(buf_transfer->resource->format);

   f = pipe->transfer_map(pipe, buf_transfer);
   for(i = 0; i < BLOCK_HEIGHT; ++i)
      for(j = 0; j < BLOCK_WIDTH; ++j)
         f[i * pitch * 4 + j] = const_matrix[j][i]; // transpose

   pipe->transfer_unmap(pipe, buf_transfer);
   pipe->transfer_destroy(pipe, buf_transfer);

   return matrix;
}

bool
vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe, struct pipe_resource *dst, struct pipe_resource *matrix)
{
   assert(idct && pipe && dst);

   idct->pipe = pipe;
   pipe_resource_reference(&idct->textures.individual.matrix, matrix);
   pipe_resource_reference(&idct->textures.individual.transpose, matrix);
   pipe_resource_reference(&idct->destination, dst);

   init_state(idct);

   if(!init_shaders(idct))
      return false;

   if(!init_buffers(idct)) {
      cleanup_shaders(idct);
      return false;
   }

   idct->surfaces.intermediate = idct->pipe->screen->get_tex_surface(
      idct->pipe->screen, idct->textures.individual.intermediate, 0, 0, 0,
      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET);

   idct->surfaces.destination = idct->pipe->screen->get_tex_surface(
      idct->pipe->screen, idct->destination, 0, 0, 0,
      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET);

   init_constants(idct);
   xfer_buffers_map(idct);

   return true;
}

void
vl_idct_cleanup(struct vl_idct *idct)
{
   idct->pipe->screen->tex_surface_destroy(idct->surfaces.destination);
   idct->pipe->screen->tex_surface_destroy(idct->surfaces.intermediate);

   cleanup_shaders(idct);
   cleanup_buffers(idct);

   cleanup_state(idct);

   pipe_resource_reference(&idct->destination, NULL);
}

void
vl_idct_add_block(struct vl_idct *idct, unsigned x, unsigned y, short *block)
{
   struct vertex2f v, *v_dst;

   unsigned tex_pitch;
   short *texels;

   unsigned i;

   assert(idct);

   if(block) {
      tex_pitch = idct->tex_transfer->stride / util_format_get_blocksize(idct->tex_transfer->resource->format);
      texels = idct->texels + y * tex_pitch * BLOCK_HEIGHT + x * BLOCK_WIDTH;

      for (i = 0; i < BLOCK_HEIGHT; ++i)
         memcpy(texels + i * tex_pitch, block + i * BLOCK_WIDTH, BLOCK_WIDTH * 2);

      /* non empty blocks fills the vector buffer from left to right */
      v_dst = idct->vectors + idct->num_blocks * 4;

      idct->num_blocks++;

   } else {

      /* while empty blocks fills the vector buffer from right to left */
      v_dst = idct->vectors + (idct->max_blocks - idct->num_empty_blocks) * 4 - 4;

      idct->num_empty_blocks++;
   }

   v.x = x;
   v.y = y;

   for (i = 0; i < 4; ++i) {
      v_dst[i] = v;
   }
}

void
vl_idct_flush(struct vl_idct *idct)
{
   xfer_buffers_unmap(idct);

   if(idct->num_blocks > 0) {

      /* first stage */
      idct->fb_state.cbufs[0] = idct->surfaces.intermediate;
      idct->pipe->set_framebuffer_state(idct->pipe, &idct->fb_state);
      idct->pipe->set_viewport_state(idct->pipe, &idct->viewport);

      idct->pipe->set_vertex_buffers(idct->pipe, 2, idct->vertex_bufs.all);
      idct->pipe->bind_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, idct->sampler_views.stage[0]);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers.stage[0]);
      idct->pipe->bind_vs_state(idct->pipe, idct->vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->transpose_fs);

      util_draw_arrays(idct->pipe, PIPE_PRIM_QUADS, 0, idct->num_blocks * 4);

      /* second stage */
      idct->fb_state.cbufs[0] = idct->surfaces.destination;
      idct->pipe->set_framebuffer_state(idct->pipe, &idct->fb_state);
      idct->pipe->set_viewport_state(idct->pipe, &idct->viewport);

      idct->pipe->set_vertex_buffers(idct->pipe, 2, idct->vertex_bufs.all);
      idct->pipe->bind_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 2, idct->sampler_views.stage[1]);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers.stage[1]);
      idct->pipe->bind_vs_state(idct->pipe, idct->vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->matrix_fs);

      util_draw_arrays(idct->pipe, PIPE_PRIM_QUADS, 0, idct->num_blocks * 4);
   }

   if(idct->num_empty_blocks > 0) {

      /* empty block handling */
      idct->fb_state.cbufs[0] = idct->surfaces.destination;
      idct->pipe->set_framebuffer_state(idct->pipe, &idct->fb_state);
      idct->pipe->set_viewport_state(idct->pipe, &idct->viewport);

      idct->pipe->set_vertex_buffers(idct->pipe, 2, idct->vertex_bufs.all);
      idct->pipe->bind_vertex_elements_state(idct->pipe, idct->vertex_elems_state);
      idct->pipe->set_fragment_sampler_views(idct->pipe, 4, idct->sampler_views.all);
      idct->pipe->bind_fragment_sampler_states(idct->pipe, 4, idct->samplers.all);
      idct->pipe->bind_vs_state(idct->pipe, idct->vs);
      idct->pipe->bind_fs_state(idct->pipe, idct->eb_fs);

      util_draw_arrays(idct->pipe, PIPE_PRIM_QUADS,
         (idct->max_blocks - idct->num_empty_blocks) * 4,
         idct->num_empty_blocks * 4);
   }

   idct->num_blocks = 0;
   idct->num_empty_blocks = 0;
   xfer_buffers_map(idct);
}
