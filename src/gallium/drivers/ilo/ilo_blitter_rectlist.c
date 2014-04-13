/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#include "util/u_draw.h"
#include "util/u_pack_color.h"

#include "ilo_blitter.h"
#include "ilo_3d.h"
#include "ilo_3d_pipeline.h"
#include "ilo_blit.h"
#include "ilo_gpe.h"
#include "ilo_gpe_gen6.h" /* for ve_init_cso_with_components and
                             zs_align_surface */

/**
 * Set the states that are invariant between all ops.
 */
static bool
ilo_blitter_set_invariants(struct ilo_blitter *blitter)
{
   struct pipe_screen *screen = blitter->ilo->base.screen;
   struct pipe_resource templ;
   struct pipe_vertex_element velems[2];
   struct pipe_viewport_state vp;

   if (blitter->initialized)
      return true;

   blitter->buffer.size = 4096;

   /* allocate the vertex buffer */
   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_BUFFER;
   templ.width0 = blitter->buffer.size;
   templ.usage = PIPE_USAGE_STREAM;
   templ.bind = PIPE_BIND_VERTEX_BUFFER;
   blitter->buffer.res = screen->resource_create(screen, &templ);
   if (!blitter->buffer.res)
      return false;

   /* do not increase reference count */
   blitter->vb.states[0].buffer = blitter->buffer.res;

   /* only vertex X and Y */
   blitter->vb.states[0].stride = 2 * sizeof(float);
   blitter->vb.enabled_mask = 0x1;
   memset(&velems, 0, sizeof(velems));
   velems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;
   ilo_gpe_init_ve(blitter->ilo->dev, 2, velems, &blitter->ve);

   /* override first VE to be VUE header */
   ve_init_cso_with_components(blitter->ilo->dev,
         GEN6_VFCOMP_STORE_0, /* Reserved */
         GEN6_VFCOMP_STORE_0, /* Render Target Array Index */
         GEN6_VFCOMP_STORE_0, /* Viewport Index */
         GEN6_VFCOMP_STORE_0, /* Point Width */
         &blitter->ve.cso[0]);

   /* a rectangle has 3 vertices in a RECTLIST */
   util_draw_init_info(&blitter->draw);
   blitter->draw.count = 3;

   /**
    * From the Haswell PRM, volume 7, page 615:
    *
    *     "The clear value must be between the min and max depth values
    *     (inclusive) defined in the CC_VIEWPORT."
    *
    * Even though clipping and viewport transformation will be disabled, we
    * still need to set up the viewport states.
    */
   memset(&vp, 0, sizeof(vp));
   vp.scale[0] = 1.0f;
   vp.scale[1] = 1.0f;
   vp.scale[2] = 1.0f;
   vp.scale[3] = 1.0f;
   ilo_gpe_set_viewport_cso(blitter->ilo->dev, &vp, &blitter->viewport);

   blitter->initialized = true;

   return true;
}

static void
ilo_blitter_set_op(struct ilo_blitter *blitter,
                   enum ilo_blitter_rectlist_op op)
{
   blitter->op = op;
}

/**
 * Set the rectangle primitive.
 */
static void
ilo_blitter_set_rectlist(struct ilo_blitter *blitter,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height)
{
   unsigned usage = PIPE_TRANSFER_WRITE | PIPE_TRANSFER_UNSYNCHRONIZED;
   float vertices[3][2];
   struct pipe_box box;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 11:
    *
    *     "(RECTLIST) A list of independent rectangles, where only 3 vertices
    *      are provided per rectangle object, with the fourth vertex implied
    *      by the definition of a rectangle. V0=LowerRight, V1=LowerLeft,
    *      V2=UpperLeft. Implied V3 = V0- V1+V2."
    */
   vertices[0][0] = (float) (x + width);
   vertices[0][1] = (float) (y + height);
   vertices[1][0] = (float) x;
   vertices[1][1] = (float) (y + height);
   vertices[2][0] = (float) x;
   vertices[2][1] = (float) y;

   /* buffer is full */
   if (blitter->buffer.offset + sizeof(vertices) > blitter->buffer.size) {
      if (!ilo_buffer_alloc_bo(ilo_buffer(blitter->buffer.res)))
         usage &= ~PIPE_TRANSFER_UNSYNCHRONIZED;

      blitter->buffer.offset = 0;
   }

   u_box_1d(blitter->buffer.offset, sizeof(vertices), &box);

   blitter->ilo->base.transfer_inline_write(&blitter->ilo->base,
         blitter->buffer.res, 0, usage, &box, vertices, 0, 0);

   blitter->vb.states[0].buffer_offset = blitter->buffer.offset;
   blitter->buffer.offset += sizeof(vertices);
}

static void
ilo_blitter_set_clear_values(struct ilo_blitter *blitter,
                             uint32_t depth, ubyte stencil)
{
   blitter->depth_clear_value = depth;
   blitter->cc.stencil_ref.ref_value[0] = stencil;
}

static void
ilo_blitter_set_dsa(struct ilo_blitter *blitter,
                    const struct pipe_depth_stencil_alpha_state *state)
{
   ilo_gpe_init_dsa(blitter->ilo->dev, state, &blitter->dsa);
}

static void
ilo_blitter_set_fb(struct ilo_blitter *blitter,
                   const struct pipe_resource *res, unsigned level,
                   const struct ilo_surface_cso *cso)
{
   blitter->fb.width = u_minify(res->width0, level);
   blitter->fb.height = u_minify(res->height0, level);

   blitter->fb.num_samples = res->nr_samples;
   if (!blitter->fb.num_samples)
      blitter->fb.num_samples = 1;

   memcpy(&blitter->fb.dst, cso, sizeof(*cso));
}

static void
ilo_blitter_set_fb_from_surface(struct ilo_blitter *blitter,
                                struct pipe_surface *surf)
{
   ilo_blitter_set_fb(blitter, surf->texture, surf->u.tex.level,
         (const struct ilo_surface_cso *) surf);
}

static void
ilo_blitter_set_fb_from_resource(struct ilo_blitter *blitter,
                                 struct pipe_resource *res,
                                 enum pipe_format format,
                                 unsigned level, unsigned slice)
{
   struct pipe_surface templ, *surf;

   memset(&templ, 0, sizeof(templ));
   templ.format = format;
   templ.u.tex.level = level;
   templ.u.tex.first_layer = slice;
   templ.u.tex.last_layer = slice;

   /* if we did not call create_surface(), it would never fail */
   surf = blitter->ilo->base.create_surface(&blitter->ilo->base, res, &templ);
   assert(surf);

   ilo_blitter_set_fb(blitter, res, level,
         (const struct ilo_surface_cso *) surf);

   pipe_surface_reference(&surf, NULL);
}

static void
ilo_blitter_set_uses(struct ilo_blitter *blitter, uint32_t uses)
{
   blitter->uses = uses;
}

static void
hiz_align_fb(struct ilo_blitter *blitter)
{
   unsigned align_w, align_h;

   switch (blitter->op) {
   case ILO_BLITTER_RECTLIST_CLEAR_ZS:
   case ILO_BLITTER_RECTLIST_RESOLVE_Z:
      break;
   default:
      return;
      break;
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 313-314:
    *
    *     "A rectangle primitive representing the clear area is delivered. The
    *      primitive must adhere to the following restrictions on size:
    *
    *      - If Number of Multisamples is NUMSAMPLES_1, the rectangle must be
    *        aligned to an 8x4 pixel block relative to the upper left corner
    *        of the depth buffer, and contain an integer number of these pixel
    *        blocks, and all 8x4 pixels must be lit.
    *
    *      - If Number of Multisamples is NUMSAMPLES_4, the rectangle must be
    *        aligned to a 4x2 pixel block (8x4 sample block) relative to the
    *        upper left corner of the depth buffer, and contain an integer
    *        number of these pixel blocks, and all samples of the 4x2 pixels
    *        must be lit
    *
    *      - If Number of Multisamples is NUMSAMPLES_8, the rectangle must be
    *        aligned to a 2x2 pixel block (8x4 sample block) relative to the
    *        upper left corner of the depth buffer, and contain an integer
    *        number of these pixel blocks, and all samples of the 2x2 pixels
    *        must be list."
    *
    *     "The following is required when performing a depth buffer resolve:
    *
    *      - A rectangle primitive of the same size as the previous depth
    *        buffer clear operation must be delivered, and depth buffer state
    *        cannot have changed since the previous depth buffer clear
    *        operation."
    */
   switch (blitter->fb.num_samples) {
   case 1:
      align_w = 8;
      align_h = 4;
      break;
   case 2:
      align_w = 4;
      align_h = 4;
      break;
   case 4:
      align_w = 4;
      align_h = 2;
      break;
   case 8:
   default:
      align_w = 2;
      align_h = 2;
      break;
   }

   if (blitter->fb.width % align_w || blitter->fb.height % align_h) {
      blitter->fb.width = align(blitter->fb.width, align_w);
      blitter->fb.height = align(blitter->fb.width, align_h);

      assert(!blitter->fb.dst.is_rt);
      zs_align_surface(blitter->ilo->dev, align_w, align_h,
            &blitter->fb.dst.u.zs);
   }
}

static void
hiz_emit_rectlist(struct ilo_blitter *blitter)
{
   struct ilo_3d *hw3d = blitter->ilo->hw3d;
   struct ilo_3d_pipeline *p = hw3d->pipeline;

   hiz_align_fb(blitter);

   ilo_blitter_set_rectlist(blitter, 0, 0,
         blitter->fb.width, blitter->fb.height);

   ilo_3d_own_render_ring(hw3d);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 313:
    *
    *     "If other rendering operations have preceded this clear, a
    *      PIPE_CONTROL with write cache flush enabled and Z-inhibit
    *      disabled must be issued before the rectangle primitive used for
    *      the depth buffer clear operation."
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 314:
    *
    *     "Depth buffer clear pass must be followed by a PIPE_CONTROL
    *      command with DEPTH_STALL bit set and Then followed by Depth
    *      FLUSH"
    *
    * But the pipeline has to be flushed both before and after not only
    * because of these workarounds.  We need them for reasons such as
    *
    *  - we may sample from a texture that was rendered to
    *  - we may sample from the fb shortly after
    */
   if (!ilo_cp_empty(p->cp))
      ilo_3d_pipeline_emit_flush(p);

   ilo_3d_pipeline_emit_rectlist(p, blitter);

   ilo_3d_pipeline_emit_flush(p);
}

static bool
hiz_can_clear_zs(const struct ilo_blitter *blitter,
                 const struct ilo_texture *tex)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 314:
    *
    *     "Several cases exist where Depth Buffer Clear cannot be enabled (the
    *      legacy method of clearing must be performed):
    *
    *      - If the depth buffer format is D32_FLOAT_S8X24_UINT or
    *        D24_UNORM_S8_UINT.
    *
    *      - If stencil test is enabled but the separate stencil buffer is
    *        disabled.
    *
    *      - [DevSNB-A{W/A}]: ...
    *
    *      - [DevSNB{W/A}]: When depth buffer format is D16_UNORM and the
    *        width of the map (LOD0) is not multiple of 16, fast clear
    *        optimization must be disabled."
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 313:
    *
    *     "Several cases exist where Depth Buffer Clear cannot be enabled (the
    *      legacy method of clearing must be performed):
    *
    *      - If the depth buffer format is D32_FLOAT_S8X24_UINT or
    *        D24_UNORM_S8_UINT.
    *
    *      - If stencil test is enabled but the separate stencil buffer is
    *        disabled."
    *
    * The truth is when HiZ is enabled, separate stencil is also enabled on
    * all GENs.  The depth buffer format cannot be combined depth/stencil.
    */
   switch (tex->bo_format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (blitter->ilo->dev->gen == ILO_GEN(6) && tex->base.width0 % 16)
         return false;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      assert(!"HiZ with combined depth/stencil");
      return false;
      break;
   default:
      break;
   }

   return true;
}

bool
ilo_blitter_rectlist_clear_zs(struct ilo_blitter *blitter,
                              struct pipe_surface *zs,
                              unsigned clear_flags,
                              double depth, unsigned stencil)
{
   struct ilo_texture *tex = ilo_texture(zs->texture);
   struct pipe_depth_stencil_alpha_state dsa_state;
   uint32_t uses, clear_value;

   if (!ilo_texture_can_enable_hiz(tex,
            zs->u.tex.level, zs->u.tex.first_layer,
            zs->u.tex.last_layer - zs->u.tex.first_layer + 1))
      return false;

   if (!hiz_can_clear_zs(blitter, tex))
      return false;

   clear_value = util_pack_z(tex->bo_format, depth);

   ilo_blit_resolve_surface(blitter->ilo, zs,
         ILO_TEXTURE_RENDER_WRITE | ILO_TEXTURE_CLEAR);
   ilo_texture_set_slice_clear_value(tex, zs->u.tex.level,
         zs->u.tex.first_layer,
         zs->u.tex.last_layer - zs->u.tex.first_layer + 1,
         clear_value);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 313-314:
    *
    *     "- Depth Test Enable must be disabled and Depth Buffer Write Enable
    *        must be enabled (if depth is being cleared).
    *
    *      - Stencil buffer clear can be performed at the same time by
    *        enabling Stencil Buffer Write Enable.  Stencil Test Enable must
    *        be enabled and Stencil Pass Depth Pass Op set to REPLACE, and the
    *        clear value that is placed in the stencil buffer is the Stencil
    *        Reference Value from COLOR_CALC_STATE.
    *
    *      - Note also that stencil buffer clear can be performed without
    *        depth buffer clear. For stencil only clear, Depth Test Enable and
    *        Depth Buffer Write Enable must be disabled.
    *
    *      - [DevSNB] errata: For stencil buffer only clear, the previous
    *        depth clear value must be delivered during the clear."
    */
   memset(&dsa_state, 0, sizeof(dsa_state));

   if (clear_flags & PIPE_CLEAR_DEPTH)
      dsa_state.depth.writemask = true;

   if (clear_flags & PIPE_CLEAR_STENCIL) {
      dsa_state.stencil[0].enabled = true;
      dsa_state.stencil[0].func = PIPE_FUNC_ALWAYS;
      dsa_state.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa_state.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
      dsa_state.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;

      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 277:
       *
       *     "Additionally the following must be set to the correct values.
       *
       *      - DEPTH_STENCIL_STATE::Stencil Write Mask must be 0xFF
       *      - DEPTH_STENCIL_STATE::Stencil Test Mask must be 0xFF
       *      - DEPTH_STENCIL_STATE::Back Face Stencil Write Mask must be 0xFF
       *      - DEPTH_STENCIL_STATE::Back Face Stencil Test Mask must be 0xFF"
       */
      dsa_state.stencil[0].valuemask = 0xff;
      dsa_state.stencil[0].writemask = 0xff;
      dsa_state.stencil[1].valuemask = 0xff;
      dsa_state.stencil[1].writemask = 0xff;
   }

   ilo_blitter_set_invariants(blitter);
   ilo_blitter_set_op(blitter, ILO_BLITTER_RECTLIST_CLEAR_ZS);

   ilo_blitter_set_dsa(blitter, &dsa_state);
   ilo_blitter_set_clear_values(blitter, clear_value, (ubyte) stencil);
   ilo_blitter_set_fb_from_surface(blitter, zs);

   uses = ILO_BLITTER_USE_DSA;
   if (clear_flags & PIPE_CLEAR_DEPTH)
      uses |= ILO_BLITTER_USE_VIEWPORT | ILO_BLITTER_USE_FB_DEPTH;
   if (clear_flags & PIPE_CLEAR_STENCIL)
      uses |= ILO_BLITTER_USE_CC | ILO_BLITTER_USE_FB_STENCIL;
   ilo_blitter_set_uses(blitter, uses);

   hiz_emit_rectlist(blitter);

   return true;
}

void
ilo_blitter_rectlist_resolve_z(struct ilo_blitter *blitter,
                               struct pipe_resource *res,
                               unsigned level, unsigned slice)
{
   struct ilo_texture *tex = ilo_texture(res);
   struct pipe_depth_stencil_alpha_state dsa_state;
   const struct ilo_texture_slice *s =
      ilo_texture_get_slice(tex, level, slice);

   if (!ilo_texture_can_enable_hiz(tex, level, slice, 1))
      return;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 314:
    *
    *     "Depth Test Enable must be enabled with the Depth Test Function set
    *      to NEVER. Depth Buffer Write Enable must be enabled. Stencil Test
    *      Enable and Stencil Buffer Write Enable must be disabled."
    */
   memset(&dsa_state, 0, sizeof(dsa_state));
   dsa_state.depth.writemask = true;
   dsa_state.depth.enabled = true;
   dsa_state.depth.func = PIPE_FUNC_NEVER;

   ilo_blitter_set_invariants(blitter);
   ilo_blitter_set_op(blitter, ILO_BLITTER_RECTLIST_RESOLVE_Z);

   ilo_blitter_set_dsa(blitter, &dsa_state);
   ilo_blitter_set_clear_values(blitter, s->clear_value, 0);
   ilo_blitter_set_fb_from_resource(blitter, res, res->format, level, slice);
   ilo_blitter_set_uses(blitter,
         ILO_BLITTER_USE_DSA | ILO_BLITTER_USE_FB_DEPTH);

   hiz_emit_rectlist(blitter);
}

void
ilo_blitter_rectlist_resolve_hiz(struct ilo_blitter *blitter,
                                 struct pipe_resource *res,
                                 unsigned level, unsigned slice)
{
   struct ilo_texture *tex = ilo_texture(res);
   struct pipe_depth_stencil_alpha_state dsa_state;

   if (!ilo_texture_can_enable_hiz(tex, level, slice, 1))
      return;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 315:
    *
    *     "(Hierarchical Depth Buffer Resolve) Depth Test Enable must be
    *      disabled. Depth Buffer Write Enable must be enabled. Stencil Test
    *      Enable and Stencil Buffer Write Enable must be disabled."
    */
   memset(&dsa_state, 0, sizeof(dsa_state));
   dsa_state.depth.writemask = true;

   ilo_blitter_set_invariants(blitter);
   ilo_blitter_set_op(blitter, ILO_BLITTER_RECTLIST_RESOLVE_HIZ);

   ilo_blitter_set_dsa(blitter, &dsa_state);
   ilo_blitter_set_fb_from_resource(blitter, res, res->format, level, slice);
   ilo_blitter_set_uses(blitter,
         ILO_BLITTER_USE_DSA | ILO_BLITTER_USE_FB_DEPTH);

   hiz_emit_rectlist(blitter);
}
