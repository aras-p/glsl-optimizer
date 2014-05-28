/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file brw_meta_stencil_blit.c
 *
 * Implements upsampling, downsampling and scaling of stencil miptrees. The
 * logic can be originally found in brw_blorp_blit.c.
 * Implementation creates a temporary draw framebuffer object and attaches the
 * destination stencil buffer attachment as color attachment. Source attachment
 * is in turn treated as a stencil texture and the glsl program used for the
 * blitting samples it using stencil-indexing.
 *
 * Unfortunately as the data port does not support interleaved msaa-surfaces
 * (stencil is always IMS), the glsl program needs to handle the writing of
 * individual samples manually. Surface is configured as if it were single
 * sampled (with adjusted dimensions) and the glsl program extracts the
 * sample indices from the input coordinates for correct texturing.
 *
 * Target surface is also configured as Y-tiled instead of W-tiled in order
 * to support generations 6-7. Later hardware supports W-tiled as render target
 * and the logic here could be simplified for those.
 */

#include "brw_context.h"
#include "intel_batchbuffer.h"
#include "intel_fbo.h"

#include "main/blit.h"
#include "main/buffers.h"
#include "main/fbobject.h"
#include "main/uniforms.h"
#include "main/texparam.h"
#include "main/texobj.h"
#include "main/viewport.h"
#include "main/enable.h"
#include "main/blend.h"
#include "main/varray.h"
#include "main/shaderapi.h"
#include "util/ralloc.h"

#include "drivers/common/meta.h"
#include "brw_meta_util.h"

#define FILE_DEBUG_FLAG DEBUG_FBO

struct blit_dims {
   int src_x0, src_y0, src_x1, src_y1;
   int dst_x0, dst_y0, dst_x1, dst_y1;
   bool mirror_x, mirror_y;
};

static const char *vs_source =
   "#version 130\n"
   "in vec2 position;\n"
   "out vec2 tex_coords;\n"
   "void main()\n"
   "{\n"
   "   tex_coords = (position + 1.0) / 2.0;\n"
   "   gl_Position = vec4(position, 0.0, 1.0);\n"
   "}\n";

static const struct sampler_and_fetch {
  const char *sampler;
  const char *fetch;
} samplers[] = {
   { "uniform usampler2D texSampler;\n",
     "   out_color = texelFetch(texSampler, txl_coords, 0)" },
   { "#extension GL_ARB_texture_multisample : enable\n"
     "uniform usampler2DMS texSampler;\n",
     "   out_color = texelFetch(texSampler, txl_coords, sample_index)" }
};

/**
 * Translating Y-tiled to W-tiled:
 *
 *  X' = (X & ~0b1011) >> 1 | (Y & 0b1) << 2 | X & 0b1
 *  Y' = (Y & ~0b1) << 1 | (X & 0b1000) >> 2 | (X & 0b10) >> 1
 */
static const char *fs_tmpl =
   "#version 130\n"
   "%s"
   "uniform float src_x_scale;\n"
   "uniform float src_y_scale;\n"
   "uniform float src_x_off;\n" /* Top right coordinates of the source */
   "uniform float src_y_off;\n" /* rectangle in W-tiled space. */
   "uniform float dst_x_off;\n" /* Top right coordinates of the target */
   "uniform float dst_y_off;\n" /* rectangle in Y-tiled space. */
   "uniform float draw_rect_w;\n" /* This is the unnormalized size of the */
   "uniform float draw_rect_h;\n" /* drawing rectangle in Y-tiled space. */
   "uniform int dst_x0;\n" /* This is the bounding rectangle in the W-tiled */
   "uniform int dst_x1;\n" /* space that will be used to skip pixels lying */
   "uniform int dst_y0;\n" /* outside. In some cases the Y-tiled rectangle */
   "uniform int dst_y1;\n" /* is larger. */
   "uniform int dst_num_samples;\n"
   "in vec2 tex_coords;\n"
   "ivec2 txl_coords;\n"
   "int sample_index;\n"
   "out uvec4 out_color;\n"
   "\n"
   "void get_unorm_target_coords()\n"
   "{\n"
   "   txl_coords.x = int(tex_coords.x * draw_rect_w + dst_x_off);\n"
   "   txl_coords.y = int(tex_coords.y * draw_rect_h + dst_y_off);\n"
   "}\n"
   "\n"
   "void translate_dst_to_src()\n"
   "{\n"
   "   txl_coords.x = int(float(txl_coords.x) * src_x_scale + src_x_off);\n"
   "   txl_coords.y = int(float(txl_coords.y) * src_y_scale + src_y_off);\n"
   "}\n"
   "\n"
   "void translate_y_to_w_tiling()\n"
   "{\n"
   "   int X = txl_coords.x;\n"
   "   int Y = txl_coords.y;\n"
   "   txl_coords.x = (X & int(0xfff4)) >> 1;\n"
   "   txl_coords.x |= ((Y & int(0x1)) << 2);\n"
   "   txl_coords.x |= (X & int(0x1));\n"
   "   txl_coords.y = (Y & int(0xfffe)) << 1;\n"
   "   txl_coords.y |= ((X & int(0x8)) >> 2);\n"
   "   txl_coords.y |= ((X & int(0x2)) >> 1);\n"
   "}\n"
   "\n"
   "void decode_msaa()\n"
   "{\n"
   "   int X = txl_coords.x;\n"
   "   int Y = txl_coords.y;\n"
   "   switch (dst_num_samples) {\n"
   "   case 0:\n"
   "      sample_index = 0;\n"
   "      break;\n"
   "   case 2:\n"
   "      txl_coords.x = ((X & int(0xfffc)) >> 1) | (X & int(0x1));\n"
   "      sample_index = (X & 0x2) >> 1;\n"
   "      break;\n"
   "   case 4:\n"
   "      txl_coords.x = ((X & int(0xfffc)) >> 1) | (X & int(0x1));\n"
   "      txl_coords.y = ((Y & int(0xfffc)) >> 1) | (Y & int(0x1));\n"
   "      sample_index = (Y & 0x2) | ((X & 0x2) >> 1);\n"
   "      break;\n"
   "   case 8:\n"
   "      txl_coords.x = ((X & int(0xfff8)) >> 2) | (X & int(0x1));\n"
   "      txl_coords.y = ((Y & int(0xfffc)) >> 1) | (Y & int(0x1));\n"
   "      sample_index = (X & 0x4) | (Y & 0x2) | ((X & 0x2) >> 1);\n"
   "   }\n"
   "}\n"
   "\n"
   "void discard_outside_bounding_rect()\n"
   "{\n"
   "   int X = txl_coords.x;\n"
   "   int Y = txl_coords.y;\n"
   "   if (X >= dst_x1 || X < dst_x0 || Y >= dst_y1 || Y < dst_y0)\n"
   "      discard;\n"
   "}\n"
   "\n"
   "void main()\n"
   "{\n"
   "   get_unorm_target_coords();\n"
   "   translate_y_to_w_tiling();\n"
   "   decode_msaa();"
   "   discard_outside_bounding_rect();\n"
   "   translate_dst_to_src();\n"
   "   %s;\n"
   "}\n";

/**
 * Setup uniforms telling the coordinates of the destination rectangle in the
 * native w-tiled space. These are needed to ignore pixels that lie outside.
 * The destination is drawn as Y-tiled and in some cases the Y-tiled drawing
 * rectangle is larger than the original (for example 1x4 w-tiled requires
 * 16x2 y-tiled).
 */
static void
setup_bounding_rect(GLuint prog, const struct blit_dims *dims)
{
   _mesa_Uniform1i(_mesa_GetUniformLocation(prog, "dst_x0"), dims->dst_x0);
   _mesa_Uniform1i(_mesa_GetUniformLocation(prog, "dst_x1"), dims->dst_x1);
   _mesa_Uniform1i(_mesa_GetUniformLocation(prog, "dst_y0"), dims->dst_y0);
   _mesa_Uniform1i(_mesa_GetUniformLocation(prog, "dst_y1"), dims->dst_y1);
}

/**
 * Setup uniforms telling the destination width, height and the offset. These
 * are needed to unnoormalize the input coordinates and to correctly translate
 * between destination and source that may have differing offsets.
 */
static void
setup_drawing_rect(GLuint prog, const struct blit_dims *dims)
{
   _mesa_Uniform1f(_mesa_GetUniformLocation(prog, "draw_rect_w"),
                   dims->dst_x1 - dims->dst_x0);
   _mesa_Uniform1f(_mesa_GetUniformLocation(prog, "draw_rect_h"),
                   dims->dst_y1 - dims->dst_y0);
   _mesa_Uniform1f(_mesa_GetUniformLocation(prog, "dst_x_off"), dims->dst_x0);
   _mesa_Uniform1f(_mesa_GetUniformLocation(prog, "dst_y_off"), dims->dst_y0);
}

/**
 * When not mirroring a coordinate (say, X), we need:
 *   src_x - src_x0 = (dst_x - dst_x0 + 0.5) * scale
 * Therefore:
 *   src_x = src_x0 + (dst_x - dst_x0 + 0.5) * scale
 *
 * The program uses "round toward zero" to convert the transformed floating
 * point coordinates to integer coordinates, whereas the behaviour we actually
 * want is "round to nearest", so 0.5 provides the necessary correction.
 *
 * When mirroring X we need:
 *   src_x - src_x0 = dst_x1 - dst_x - 0.5
 * Therefore:
 *   src_x = src_x0 + (dst_x1 -dst_x - 0.5) * scale
 */
static void
setup_coord_coeff(GLuint prog, GLuint multiplier, GLuint offset,
                  int src_0, int src_1, int dst_0, int dst_1, bool mirror)
{
   const float scale = ((float)(src_1 - src_0)) / (dst_1 - dst_0);

   if (mirror) {
      _mesa_Uniform1f(multiplier, -scale);
      _mesa_Uniform1f(offset, src_0 + (dst_1 - 0.5) * scale);
   } else {
      _mesa_Uniform1f(multiplier, scale);
      _mesa_Uniform1f(offset, src_0 + (-dst_0 + 0.5) * scale);
   }
}

/**
 * Setup uniforms providing relation between source and destination surfaces.
 * Destination coordinates are in Y-tiling layout while texelFetch() expects
 * W-tiled coordinates. Once the destination coordinates are re-interpreted by
 * the program into the original W-tiled layout, the program needs to know the
 * offset and scaling factors between the destination and source.
 * Note that these are calculated in the original W-tiled space before the
 * destination rectangle is adjusted for possible msaa and Y-tiling.
 */
static void
setup_coord_transform(GLuint prog, const struct blit_dims *dims)
{
   setup_coord_coeff(prog,
                     _mesa_GetUniformLocation(prog, "src_x_scale"),
                     _mesa_GetUniformLocation(prog, "src_x_off"),
                     dims->src_x0, dims->src_x1, dims->dst_x0, dims->dst_x1,
                     dims->mirror_x);

   setup_coord_coeff(prog,
                     _mesa_GetUniformLocation(prog, "src_y_scale"),
                     _mesa_GetUniformLocation(prog, "src_y_off"),
                     dims->src_y0, dims->src_y1, dims->dst_y0, dims->dst_y1,
                     dims->mirror_y);
}

static GLuint
setup_program(struct brw_context *brw, bool msaa_tex)
{
   struct gl_context *ctx = &brw->ctx;
   struct blit_state *blit = &ctx->Meta->Blit;
   char *fs_source;
   const struct sampler_and_fetch *sampler = &samplers[msaa_tex];

   _mesa_meta_setup_vertex_objects(&blit->VAO, &blit->VBO, true, 2, 2, 0);

   GLuint *prog_id = &brw->meta_stencil_blit_programs[msaa_tex];

   if (*prog_id) {
      _mesa_UseProgram(*prog_id);
      return *prog_id;
   }
  
   fs_source = ralloc_asprintf(NULL, fs_tmpl, sampler->sampler,
                               sampler->fetch);
   _mesa_meta_compile_and_link_program(ctx, vs_source, fs_source,
                                       "i965 stencil blit",
                                       prog_id);
   ralloc_free(fs_source);

   return *prog_id;
}

/**
 * Samples in stencil buffer are interleaved, and unfortunately the data port 
 * does not support it as render target. Therefore the surface is set up as
 * single sampled and the program handles the interleaving.
 * In case of single sampled stencil, the render buffer is adjusted with
 * twice the base level height in order for the program to be able to write
 * any mip-level. (Used to set the drawing rectangle for the hw).
 */
static void
adjust_msaa(struct blit_dims *dims, int num_samples)
{
   if (num_samples == 2) {
      dims->dst_x0 *= 2;
      dims->dst_x1 *= 2;
   } else if (num_samples) {
      const int x_num_samples = num_samples / 2;
      dims->dst_x0 = ROUND_DOWN_TO(dims->dst_x0 * x_num_samples, num_samples);
      dims->dst_y0 = ROUND_DOWN_TO(dims->dst_y0 * 2, 4);
      dims->dst_x1 = ALIGN(dims->dst_x1 * x_num_samples, num_samples);
      dims->dst_y1 = ALIGN(dims->dst_y1 * 2, 4);
   }
}

/**
 * Stencil is mapped as Y-tiled render target and the dimensions need to be
 * adjusted in order for the Y-tiled rectangle to cover the entire linear
 * memory space of the original W-tiled rectangle.
 */
static void
adjust_tiling(struct blit_dims *dims, int num_samples)
{
   const unsigned x_align = 8, y_align = num_samples > 2 ? 8 : 4;

   dims->dst_x0 = ROUND_DOWN_TO(dims->dst_x0, x_align) * 2;
   dims->dst_y0 = ROUND_DOWN_TO(dims->dst_y0, y_align) / 2;
   dims->dst_x1 = ALIGN(dims->dst_x1, x_align) * 2;
   dims->dst_y1 = ALIGN(dims->dst_y1, y_align) / 2;
}

/**
 * When stencil is mapped as Y-tiled render target the mip-level offsets
 * calculated for the Y-tiling do not always match the offsets in W-tiling.
 * Therefore the sampling engine cannot be used for individual mip-level
 * access but the program needs to do it internally. This can be achieved
 * by shifting the coordinates of the blit rectangle here.
 */
static void
adjust_mip_level(const struct intel_mipmap_tree *mt,
                 unsigned level, unsigned layer, struct blit_dims *dims)
{
   unsigned x_offset;
   unsigned y_offset;

   intel_miptree_get_image_offset(mt, level, layer, &x_offset, &y_offset);

   dims->dst_x0 += x_offset;
   dims->dst_y0 += y_offset;
   dims->dst_x1 += x_offset;
   dims->dst_y1 += y_offset;
}

static void
prepare_vertex_data(void)
{
   static const struct vertex verts[] = {
      { .x = -1.0f, .y = -1.0f },
      { .x =  1.0f, .y = -1.0f },
      { .x =  1.0f, .y =  1.0f },
      { .x = -1.0f, .y =  1.0f } };

   _mesa_BufferSubData(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
}

static bool
set_read_rb_tex_image(struct gl_context *ctx, struct fb_tex_blit_state *blit,
                      GLenum *target)
{
   const struct gl_renderbuffer_attachment *att =
      &ctx->ReadBuffer->Attachment[BUFFER_STENCIL];
   struct gl_renderbuffer *rb = att->Renderbuffer;
   struct gl_texture_object *tex_obj;
   unsigned level = 0;

   /* If the renderbuffer is already backed by an tex image, use it. */
   if (att->Texture) {
      tex_obj = att->Texture;
      *target = tex_obj->Target;
      level = att->TextureLevel;
   } else {
      if (!_mesa_meta_bind_rb_as_tex_image(ctx, rb, &blit->tempTex, &tex_obj,
                                          target)) {
         return false;
      }
   }

   blit->baseLevelSave = tex_obj->BaseLevel;
   blit->maxLevelSave = tex_obj->MaxLevel;
   blit->stencilSamplingSave = tex_obj->StencilSampling;
   blit->sampler = _mesa_meta_setup_sampler(ctx, tex_obj, *target,
                                            GL_NEAREST, level);
   return true;
}

static void
brw_meta_stencil_blit(struct brw_context *brw,
                      struct intel_mipmap_tree *dst_mt,
                      unsigned dst_level, unsigned dst_layer,
                      const struct blit_dims *orig_dims)
{
   struct gl_context *ctx = &brw->ctx;
   struct blit_dims dims = *orig_dims;
   struct fb_tex_blit_state blit;
   GLuint prog, fbo, rbo;
   GLenum target;

   _mesa_meta_fb_tex_blit_begin(ctx, &blit);

   _mesa_GenFramebuffers(1, &fbo);
   /* Force the surface to be configured for level zero. */
   rbo = brw_get_rb_for_slice(brw, dst_mt, 0, dst_layer, true);
   adjust_msaa(&dims, dst_mt->num_samples);
   adjust_tiling(&dims, dst_mt->num_samples);

   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
   _mesa_FramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_RENDERBUFFER, rbo);
   _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0);
   ctx->DrawBuffer->_Status = GL_FRAMEBUFFER_COMPLETE;

   if (!set_read_rb_tex_image(ctx, &blit, &target)) {
      goto error;
   }

   _mesa_TexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE,
                       GL_STENCIL_INDEX);

   prog = setup_program(brw, target != GL_TEXTURE_2D);
   setup_bounding_rect(prog, orig_dims);
   setup_drawing_rect(prog, &dims);
   setup_coord_transform(prog, orig_dims);

   _mesa_Uniform1i(_mesa_GetUniformLocation(prog, "dst_num_samples"),
                   dst_mt->num_samples);

   prepare_vertex_data();
   _mesa_set_viewport(ctx, 0, dims.dst_x0, dims.dst_y0,
                      dims.dst_x1 - dims.dst_x0, dims.dst_y1 - dims.dst_y0);
   _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   _mesa_set_enable(ctx, GL_DEPTH_TEST, false);

   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

error:
   _mesa_meta_fb_tex_blit_end(ctx, target, &blit);
   _mesa_meta_end(ctx);

   _mesa_DeleteRenderbuffers(1, &rbo);
   _mesa_DeleteFramebuffers(1, &fbo);
}

void
brw_meta_fbo_stencil_blit(struct brw_context *brw,
                          GLfloat src_x0, GLfloat src_y0,
                          GLfloat src_x1, GLfloat src_y1,
                          GLfloat dst_x0, GLfloat dst_y0,
                          GLfloat dst_x1, GLfloat dst_y1)
{
   struct gl_context *ctx = &brw->ctx;
   struct gl_renderbuffer *draw_fb =
      ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   const struct intel_renderbuffer *dst_irb = intel_renderbuffer(draw_fb);
   struct intel_mipmap_tree *dst_mt = dst_irb->mt;

   if (!dst_mt)
      return;

   if (dst_mt->stencil_mt)
      dst_mt = dst_mt->stencil_mt;

   bool mirror_x, mirror_y;
   if (brw_meta_mirror_clip_and_scissor(ctx,
                                        &src_x0, &src_y0, &src_x1, &src_y1,
                                        &dst_x0, &dst_y0, &dst_x1, &dst_y1,
                                        &mirror_x, &mirror_y))
      return;

   struct blit_dims dims = { .src_x0 = src_x0, .src_y0 = src_y0,
                             .src_x1 = src_x1, .src_y1 = src_y1,
                             .dst_x0 = dst_x0, .dst_y0 = dst_y0,
                             .dst_x1 = dst_x1, .dst_y1 = dst_y1,
                             .mirror_x = mirror_x, .mirror_y = mirror_y };
   adjust_mip_level(dst_mt, dst_irb->mt_level, dst_irb->mt_layer, &dims);

   intel_batchbuffer_emit_mi_flush(brw);
   _mesa_meta_begin(ctx, MESA_META_ALL);
   brw_meta_stencil_blit(brw,
                         dst_mt, dst_irb->mt_level, dst_irb->mt_layer, &dims);
   intel_batchbuffer_emit_mi_flush(brw);
}

void
brw_meta_stencil_updownsample(struct brw_context *brw,
                              struct intel_mipmap_tree *src,
                              struct intel_mipmap_tree *dst)
{
   struct gl_context *ctx = &brw->ctx;
   struct blit_dims dims = {
      .src_x0 = 0, .src_y0 = 0,
      .src_x1 = src->logical_width0, .src_y1 = src->logical_height0,
      .dst_x0 = 0, .dst_y0 = 0,
      .dst_x1 = dst->logical_width0, .dst_y1 = dst->logical_height0,
      .mirror_x = 0, .mirror_y = 0 };
   GLuint fbo, rbo;

   if (dst->stencil_mt)
      dst = dst->stencil_mt;

   intel_batchbuffer_emit_mi_flush(brw);
   _mesa_meta_begin(ctx, MESA_META_ALL);

   _mesa_GenFramebuffers(1, &fbo);
   rbo = brw_get_rb_for_slice(brw, src, 0, 0, false);

   _mesa_BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
   _mesa_FramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                 GL_RENDERBUFFER, rbo);

   brw_meta_stencil_blit(brw, dst, 0, 0, &dims);
   intel_batchbuffer_emit_mi_flush(brw);

   _mesa_DeleteRenderbuffers(1, &rbo);
   _mesa_DeleteFramebuffers(1, &fbo);
}
