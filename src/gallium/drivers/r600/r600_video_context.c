#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pipe/p_defines.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_memory.h>
#include <X11/Xlib.h>

#include <fcntl.h>

#include "softpipe/sp_texture.h"

#include "r600_video_context.h"
#include <softpipe/sp_video_context.h>

#if 0

static void r600_mpeg12_destroy(struct pipe_video_context *vpipe)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;

    assert(vpipe);

    ctx->pipe->bind_vs_state(ctx->pipe, NULL);
    ctx->pipe->bind_fs_state(ctx->pipe, NULL);

    ctx->pipe->delete_blend_state(ctx->pipe, ctx->blend);
    ctx->pipe->delete_rasterizer_state(ctx->pipe, ctx->rast);
    ctx->pipe->delete_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);

    pipe_video_surface_reference(&ctx->decode_target, NULL);
    vl_compositor_cleanup(&ctx->compositor);
    vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
    ctx->pipe->destroy(ctx->pipe);

    FREE(ctx);
}

static void
r600_mpeg12_decode_macroblocks(struct pipe_video_context *vpipe,
                               struct pipe_video_surface *past,
                               struct pipe_video_surface *future,
                               unsigned num_macroblocks,
                               struct pipe_macroblock *macroblocks,
                               struct pipe_fence_handle **fence)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;
    struct pipe_mpeg12_macroblock *mpeg12_macroblocks =
                         (struct pipe_mpeg12_macroblock*)macroblocks;

    assert(vpipe);
    assert(num_macroblocks);
    assert(macroblocks);
    assert(macroblocks->codec == PIPE_VIDEO_CODEC_MPEG12);
    assert(ctx->decode_target);

    vl_mpeg12_mc_renderer_render_macroblocks(
                            &ctx->mc_renderer,
                            r600_video_surface(ctx->decode_target)->tex,
                            past ? r600_video_surface(past)->tex : NULL,
                            future ? r600_video_surface(future)->tex : NULL,
                            num_macroblocks, mpeg12_macroblocks, fence);
}

static void r600_mpeg12_clear_surface(struct pipe_video_context *vpipe,
                                      unsigned x, unsigned y,
                                      unsigned width, unsigned height,
                                      unsigned value,
                                      struct pipe_surface *surface)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;

    assert(vpipe);
    assert(surface);

    if (ctx->pipe->surface_fill)
        ctx->pipe->surface_fill(ctx->pipe, surface, x, y, width, height, value);
    else
        util_surface_fill(ctx->pipe, surface, x, y, width, height, value);
}

static void
r600_mpeg12_render_picture(struct pipe_video_context     *vpipe,
                           struct pipe_video_surface     *src_surface,
                           enum pipe_mpeg12_picture_type picture_type,
                           struct pipe_video_rect        *src_area,
                           struct pipe_surface           *dst_surface,
                           struct pipe_video_rect        *dst_area,
                           struct pipe_fence_handle      **fence)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;

    assert(vpipe);
    assert(src_surface);
    assert(src_area);
    assert(dst_surface);
    assert(dst_area);

    vl_compositor_render(&ctx->compositor,
                         r600_video_surface(src_surface)->tex,
                         picture_type, src_area, dst_surface->texture,
                         dst_area, fence);
}

static void r600_mpeg12_set_decode_target(struct pipe_video_context *vpipe,
                                          struct pipe_video_surface *dt)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;

    assert(vpipe);
    assert(dt);

    pipe_video_surface_reference(&ctx->decode_target, dt);
}

static void r600_mpeg12_set_csc_matrix(struct pipe_video_context *vpipe,
                                       const float *mat)
{
    struct radeon_mpeg12_context *ctx = (struct radeon_mpeg12_context*)vpipe;

    assert(vpipe);

    vl_compositor_set_csc_matrix(&ctx->compositor, mat);
}

static bool r600_mpeg12_init_pipe_state(struct radeon_mpeg12_context *ctx)
{
    struct pipe_rasterizer_state rast;
    struct pipe_blend_state blend;
    struct pipe_depth_stencil_alpha_state dsa;
    unsigned i;

    assert(ctx);

    rast.flatshade = 1;
    rast.flatshade_first = 0;
    rast.light_twoside = 0;
    rast.front_winding = PIPE_WINDING_CCW;
    rast.cull_mode = PIPE_WINDING_CW;
    rast.fill_cw = PIPE_POLYGON_MODE_FILL;
    rast.fill_ccw = PIPE_POLYGON_MODE_FILL;
    rast.offset_cw = 0;
    rast.offset_ccw = 0;
    rast.scissor = 0;
    rast.poly_smooth = 0;
    rast.poly_stipple_enable = 0;
    rast.point_sprite = 0;
    rast.point_size_per_vertex = 0;
    rast.multisample = 0;
    rast.line_smooth = 0;
    rast.line_stipple_enable = 0;
    rast.line_stipple_factor = 0;
    rast.line_stipple_pattern = 0;
    rast.line_last_pixel = 0;
    rast.bypass_vs_clip_and_viewport = 0;
    rast.line_width = 1;
    rast.point_smooth = 0;
    rast.point_size = 1;
    rast.offset_units = 1;
    rast.offset_scale = 1;
    /*rast.sprite_coord_mode[i] = ;*/
    ctx->rast = ctx->pipe->create_rasterizer_state(ctx->pipe, &rast);
    ctx->pipe->bind_rasterizer_state(ctx->pipe, ctx->rast);

    blend.blend_enable = 0;
    blend.rgb_func = PIPE_BLEND_ADD;
    blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
    blend.rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
    blend.alpha_func = PIPE_BLEND_ADD;
    blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
    blend.alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
    blend.logicop_enable = 0;
    blend.logicop_func = PIPE_LOGICOP_CLEAR;
    /* Needed to allow color writes to FB, even if blending disabled */
    blend.colormask = PIPE_MASK_RGBA;
    blend.dither = 0;
    ctx->blend = ctx->pipe->create_blend_state(ctx->pipe, &blend);
    ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend);

    dsa.depth.enabled = 0;
    dsa.depth.writemask = 0;
    dsa.depth.func = PIPE_FUNC_ALWAYS;
    for (i = 0; i < 2; ++i)
    {
        dsa.stencil[i].enabled = 0;
        dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
        dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
        dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
        dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
        dsa.stencil[i].ref_value = 0;
        dsa.stencil[i].valuemask = 0;
        dsa.stencil[i].writemask = 0;
    }
    dsa.alpha.enabled = 0;
    dsa.alpha.func = PIPE_FUNC_ALWAYS;
    dsa.alpha.ref_value = 0;
    ctx->dsa = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &dsa);
    ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);
}

static struct pipe_video_context *
r600_mpeg12_context_create(struct pipe_screen *screen,
                           enum pipe_video_profile profile,
                           enum pipe_video_chroma_format chroma_format,
                           unsigned int width,
                           unsigned int height)
{
    struct radeon_mpeg12_context *ctx;
    ctx = CALLOC_STRUCT(radeon_mpeg12_context);
    if (!ctx)
        return NULL;

    ctx->base.profile       = profile;
    ctx->base.chroma_format = chroma_format;
    ctx->base.width         = width;
    ctx->base.height        = height;
    ctx->base.screen        = screen;

    ctx->base.destroy               = radeon_mpeg12_destroy;
    ctx->base.decode_macroblocks    = radeon_mpeg12_decode_macroblocks;
    ctx->base.clear_surface         = radeon_mpeg12_clear_surface;
    ctx->base.render_picture        = radeon_mpeg12_render_picture;
    ctx->base.set_decode_target     = radeon_mpeg12_set_decode_target;
    ctx->base.set_csc_matrix        = radeon_mpeg12_set_csc_matrix;

    ctx->pipe = r600_create_context(screen,(struct r600_winsys*)screen->winsys);
    if (!ctx->pipe)
    {
        FREE(ctx);
        return NULL;
    }

    if (!vl_mpeg12_mc_renderer_init(&ctx->mc_renderer, ctx->pipe,
                                   width, height, chroma_format,
                                   VL_MPEG12_MC_RENDERER_BUFFER_PICTURE,
                                   VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE,
                                   true))
    {
        ctx->pipe->destroy(ctx->pipe);
        FREE(ctx);
        return NULL;
    }

    if (!vl_compositor_init(&ctx->compositor, ctx->pipe))
    {
        vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
        ctx->pipe->destroy(ctx->pipe);
        FREE(ctx);
        return NULL;
    }

    if (!radeon_mpeg12_init_pipe_state(ctx))
    {
        vl_compositor_cleanup(&ctx->compositor);
        vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
        ctx->pipe->destroy(ctx->pipe);
        FREE(ctx);
        return NULL;
    }

    return &ctx->base;
}

#endif

struct pipe_video_context *
r600_video_create(struct pipe_screen *screen, enum pipe_video_profile profile,
                  enum pipe_video_chroma_format chroma_format,
                  unsigned width, unsigned height, void *priv)
{
   struct pipe_context *pipe;

   assert(screen);

   pipe = screen->context_create(screen, priv);
   if (!pipe)
      return NULL;

   return sp_video_create_ex(pipe, profile, chroma_format, width, height,
                             VL_MPEG12_MC_RENDERER_BUFFER_PICTURE,
                             VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE,
                             true,
                             PIPE_FORMAT_VUYX);

#if 0
    struct pipe_video_context *vpipe;
    struct radeon_vl_context *rvl_ctx;

    assert(p_screen);
    assert(width && height);

    /* create radeon pipe_context */
    switch(u_reduce_video_profile(profile))
    {
        case PIPE_VIDEO_CODEC_MPEG12:
            vpipe = radeon_mpeg12_context_create(p_screen, profile, chr_f,
                                                 width, height);
            break;
        default:
            return NULL;
    }

    /* create radeon_vl_context */
    rvl_ctx = calloc(1, sizeof(struct radeon_vl_context));
    rvl_ctx->display = display;
    rvl_ctx->screen = screen;

    vpipe->priv = rvl_ctx;

    return vpipe;
#endif
}
