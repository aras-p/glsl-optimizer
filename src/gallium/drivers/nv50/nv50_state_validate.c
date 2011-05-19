
#include "nv50_context.h"
#include "os/os_time.h"

static void
nv50_validate_fb(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i;
   boolean serialize = FALSE;

   nv50_bufctx_reset(nv50, NV50_BUFCTX_FRAME);

   BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
   OUT_RING  (chan, (076543210 << 4) | fb->nr_cbufs);
   BEGIN_RING(chan, RING_3D(SCREEN_SCISSOR_HORIZ), 2);
   OUT_RING  (chan, fb->width << 16);
   OUT_RING  (chan, fb->height << 16);

   MARK_RING(chan, 9 * fb->nr_cbufs, 2 * fb->nr_cbufs);

   for (i = 0; i < fb->nr_cbufs; ++i) {
      struct nv50_miptree *mt = nv50_miptree(fb->cbufs[i]->texture);
      struct nv50_surface *sf = nv50_surface(fb->cbufs[i]);
      struct nouveau_bo *bo = mt->base.bo;
      uint32_t offset = sf->offset;

      BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(i)), 5);
      OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
      OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
      OUT_RING  (chan, nv50_format_table[sf->base.format].rt);
      OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode << 4);
      OUT_RING  (chan, mt->layer_stride >> 2);
      BEGIN_RING(chan, RING_3D(RT_HORIZ(i)), 2);
      OUT_RING  (chan, sf->width);
      OUT_RING  (chan, sf->height);
      BEGIN_RING(chan, RING_3D(RT_ARRAY_MODE), 1);
      OUT_RING  (chan, sf->depth);

      if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         serialize = TRUE;
      mt->base.status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      mt->base.status &= NOUVEAU_BUFFER_STATUS_GPU_READING;

      /* only register for writing, otherwise we'd always serialize here */
      nv50_bufctx_add_resident(nv50, NV50_BUFCTX_FRAME, &mt->base,
                               NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   }

   if (fb->zsbuf) {
      struct nv50_miptree *mt = nv50_miptree(fb->zsbuf->texture);
      struct nv50_surface *sf = nv50_surface(fb->zsbuf);
      struct nouveau_bo *bo = mt->base.bo;
      int unk = mt->base.base.target == PIPE_TEXTURE_2D;
      uint32_t offset = sf->offset;

      MARK_RING (chan, 12, 2);
      BEGIN_RING(chan, RING_3D(ZETA_ADDRESS_HIGH), 5);
      OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
      OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
      OUT_RING  (chan, nv50_format_table[fb->zsbuf->format].rt);
      OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode << 4);
      OUT_RING  (chan, mt->layer_stride >> 2);
      BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
      OUT_RING  (chan, sf->width);
      OUT_RING  (chan, sf->height);
      OUT_RING  (chan, (unk << 16) | sf->depth);

      if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         serialize = TRUE;
      mt->base.status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      mt->base.status &= NOUVEAU_BUFFER_STATUS_GPU_READING;

      nv50_bufctx_add_resident(nv50, NV50_BUFCTX_FRAME, &mt->base,
                               NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
   } else {
      BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
      OUT_RING  (chan, 0);
   }

   BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
   OUT_RING  (chan, fb->width << 16);
   OUT_RING  (chan, fb->height << 16);

   if (serialize) {
      BEGIN_RING(chan, RING_3D(SERIALIZE), 1);
      OUT_RING  (chan, 0);
   }
}

static void
nv50_validate_blend_colour(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;

   BEGIN_RING(chan, RING_3D(BLEND_COLOR(0)), 4);
   OUT_RINGf (chan, nv50->blend_colour.color[0]);
   OUT_RINGf (chan, nv50->blend_colour.color[1]);
   OUT_RINGf (chan, nv50->blend_colour.color[2]);
   OUT_RINGf (chan, nv50->blend_colour.color[3]);    
}

static void
nv50_validate_stencil_ref(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;

   BEGIN_RING(chan, RING_3D(STENCIL_FRONT_FUNC_REF), 1);
   OUT_RING  (chan, nv50->stencil_ref.ref_value[0]);
   BEGIN_RING(chan, RING_3D(STENCIL_BACK_FUNC_REF), 1);
   OUT_RING  (chan, nv50->stencil_ref.ref_value[1]);
}

static void
nv50_validate_stipple(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   unsigned i;

   BEGIN_RING(chan, RING_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
   for (i = 0; i < 32; ++i)
      OUT_RING(chan, util_bswap32(nv50->stipple.stipple[i]));
}

static void
nv50_validate_scissor(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct pipe_scissor_state *s = &nv50->scissor;
#ifdef NV50_SCISSORS_CLIPPING
   struct pipe_viewport_state *vp = &nv50->viewport;
   int minx, maxx, miny, maxy;

   if (!(nv50->dirty &
         (NV50_NEW_SCISSOR | NV50_NEW_VIEWPORT | NV50_NEW_FRAMEBUFFER)) &&
       nv50->state.scissor == nv50->rast->pipe.scissor)
      return;
   nv50->state.scissor = nv50->rast->pipe.scissor;

   if (nv50->state.scissor) {
      minx = s->minx;
      maxx = s->maxx;
      miny = s->miny;
      maxy = s->maxy;
   } else {
      minx = 0;
      maxx = nv50->framebuffer.width;
      miny = 0;
      maxy = nv50->framebuffer.height;
   }

   minx = MAX2(minx, (int)(vp->translate[0] - fabsf(vp->scale[0])));
   maxx = MIN2(maxx, (int)(vp->translate[0] + fabsf(vp->scale[0])));
   miny = MAX2(miny, (int)(vp->translate[1] - fabsf(vp->scale[1])));
   maxy = MIN2(maxy, (int)(vp->translate[1] + fabsf(vp->scale[1])));

   BEGIN_RING(chan, RING_3D(SCISSOR_HORIZ(0)), 2);
   OUT_RING  (chan, (maxx << 16) | minx);
   OUT_RING  (chan, (maxy << 16) | miny);
#else
   BEGIN_RING(chan, RING_3D(SCISSOR_HORIZ(0)), 2);
   OUT_RING  (chan, (s->maxx << 16) | s->minx);
   OUT_RING  (chan, (s->maxy << 16) | s->miny);
#endif
}

static void
nv50_validate_viewport(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   float zmin, zmax;

   BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSLATE_X(0)), 3);
   OUT_RINGf (chan, nv50->viewport.translate[0]);
   OUT_RINGf (chan, nv50->viewport.translate[1]);
   OUT_RINGf (chan, nv50->viewport.translate[2]);
   BEGIN_RING(chan, RING_3D(VIEWPORT_SCALE_X(0)), 3);
   OUT_RINGf (chan, nv50->viewport.scale[0]);
   OUT_RINGf (chan, nv50->viewport.scale[1]);
   OUT_RINGf (chan, nv50->viewport.scale[2]);

   zmin = nv50->viewport.translate[2] - fabsf(nv50->viewport.scale[2]);
   zmax = nv50->viewport.translate[2] + fabsf(nv50->viewport.scale[2]);

#ifdef NV50_SCISSORS_CLIPPING
   BEGIN_RING(chan, RING_3D(DEPTH_RANGE_NEAR(0)), 2);
   OUT_RINGf (chan, zmin);
   OUT_RINGf (chan, zmax);
#endif
}

static void
nv50_validate_clip(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   uint32_t clip;

   if (nv50->clip.depth_clamp) {
      clip =
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_NEAR |
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_FAR |
         NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK12_UNK1;
   } else {
      clip = 0;
   }

#ifndef NV50_SCISSORS_CLIPPING
   clip |=
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK7 |
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK12_UNK1;
#endif

   BEGIN_RING(chan, RING_3D(VIEW_VOLUME_CLIP_CTRL), 1);
   OUT_RING  (chan, clip);

   if (nv50->clip.nr) {
      BEGIN_RING(chan, RING_3D(CB_ADDR), 1);
      OUT_RING  (chan, (0 << 8) | NV50_CB_AUX);
      BEGIN_RING_NI(chan, RING_3D(CB_DATA(0)), nv50->clip.nr * 4);
      OUT_RINGp (chan, &nv50->clip.ucp[0][0], nv50->clip.nr * 4);
   }

   BEGIN_RING(chan, RING_3D(VP_CLIP_DISTANCE_ENABLE), 1);
   OUT_RING  (chan, (1 << nv50->clip.nr) - 1);

   if (nv50->vertprog && nv50->clip.nr > nv50->vertprog->vp.clpd_nr)
      nv50->dirty |= NV50_NEW_VERTPROG;
}

static void
nv50_validate_blend(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;

   WAIT_RING(chan, nv50->blend->size);
   OUT_RINGp(chan, nv50->blend->state, nv50->blend->size);
}

static void
nv50_validate_zsa(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;

   WAIT_RING(chan, nv50->zsa->size);
   OUT_RINGp(chan, nv50->zsa->state, nv50->zsa->size);
}

static void
nv50_validate_rasterizer(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;

   WAIT_RING(chan, nv50->rast->size);
   OUT_RINGp(chan, nv50->rast->state, nv50->rast->size);
}

static void
nv50_switch_pipe_context(struct nv50_context *ctx_to)
{
   struct nv50_context *ctx_from = ctx_to->screen->cur_ctx;

   if (ctx_from)
      ctx_to->state = ctx_from->state;

   ctx_to->dirty = ~0;

   if (!ctx_to->vertex)
      ctx_to->dirty &= ~(NV50_NEW_VERTEX | NV50_NEW_ARRAYS);

   if (!ctx_to->vertprog)
      ctx_to->dirty &= ~NV50_NEW_VERTPROG;
   if (!ctx_to->fragprog)
      ctx_to->dirty &= ~NV50_NEW_FRAGPROG;

   if (!ctx_to->blend)
      ctx_to->dirty &= ~NV50_NEW_BLEND;
   if (!ctx_to->rast)
      ctx_to->dirty &= ~NV50_NEW_RASTERIZER;
   if (!ctx_to->zsa)
      ctx_to->dirty &= ~NV50_NEW_ZSA;

   ctx_to->screen->base.channel->user_private = ctx_to->screen->cur_ctx =
      ctx_to;
}

static struct state_validate {
    void (*func)(struct nv50_context *);
    uint32_t states;
} validate_list[] = {
    { nv50_validate_fb,            NV50_NEW_FRAMEBUFFER },
    { nv50_validate_blend,         NV50_NEW_BLEND },
    { nv50_validate_zsa,           NV50_NEW_ZSA },
    { nv50_validate_rasterizer,    NV50_NEW_RASTERIZER },
    { nv50_validate_blend_colour,  NV50_NEW_BLEND_COLOUR },
    { nv50_validate_stencil_ref,   NV50_NEW_STENCIL_REF },
    { nv50_validate_stipple,       NV50_NEW_STIPPLE },
#ifdef NV50_SCISSORS_CLIPPING
    { nv50_validate_scissor,       NV50_NEW_SCISSOR | NV50_NEW_VIEWPORT |
                                   NV50_NEW_RASTERIZER |
                                   NV50_NEW_FRAMEBUFFER },
#else
    { nv50_validate_scissor,       NV50_NEW_SCISSOR },
#endif
    { nv50_validate_viewport,      NV50_NEW_VIEWPORT },
    { nv50_validate_clip,          NV50_NEW_CLIP },
    { nv50_vertprog_validate,      NV50_NEW_VERTPROG },
    { nv50_gmtyprog_validate,      NV50_NEW_GMTYPROG },
    { nv50_fragprog_validate,      NV50_NEW_FRAGPROG },
    { nv50_fp_linkage_validate,    NV50_NEW_FRAGPROG | NV50_NEW_VERTPROG |
                                   NV50_NEW_GMTYPROG },
    { nv50_gp_linkage_validate,    NV50_NEW_GMTYPROG | NV50_NEW_VERTPROG },
    { nv50_validate_derived_rs,    NV50_NEW_FRAGPROG | NV50_NEW_RASTERIZER |
                                   NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG },
    { nv50_constbufs_validate,     NV50_NEW_CONSTBUF },
    { nv50_validate_textures,      NV50_NEW_TEXTURES },
    { nv50_validate_samplers,      NV50_NEW_SAMPLERS },
    { nv50_vertex_arrays_validate, NV50_NEW_VERTEX | NV50_NEW_ARRAYS }
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nv50_state_validate(struct nv50_context *nv50)
{
   unsigned i;

   if (nv50->screen->cur_ctx != nv50)
      nv50_switch_pipe_context(nv50);

   if (nv50->dirty) {
      for (i = 0; i < validate_list_len; ++i) {
         struct state_validate *validate = &validate_list[i];

         if (nv50->dirty & validate->states)
            validate->func(nv50);
      }
      nv50->dirty = 0;
   }

   nv50_bufctx_emit_relocs(nv50);

   return TRUE;
}
