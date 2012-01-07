
#include "util/u_math.h"

#include "nvc0_context.h"

static void
nvc0_validate_zcull(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
    struct nv50_surface *sf = nv50_surface(fb->zsbuf);
    struct nv50_miptree *mt = nv50_miptree(sf->base.texture);
    struct nouveau_bo *bo = mt->base.bo;
    uint32_t size;
    uint32_t offset = align(mt->total_size, 1 << 17);
    unsigned width, height;

    assert(mt->base.base.depth0 == 1 && mt->base.base.array_size < 2);

    size = mt->total_size * 2;

    height = align(fb->height, 32);
    width = fb->width % 224;
    if (width)
       width = fb->width + (224 - width);
    else
       width = fb->width;

    MARK_RING (chan, 23, 4);
    BEGIN_RING(chan, RING_3D_(0x1590), 1); /* ZCULL_REGION_INDEX (bits 0x3f) */
    OUT_RING  (chan, 0);
    BEGIN_RING(chan, RING_3D_(0x07e8), 2); /* ZCULL_ADDRESS_A_HIGH */
    OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    offset += 1 << 17;
    BEGIN_RING(chan, RING_3D_(0x07f0), 2); /* ZCULL_ADDRESS_B_HIGH */
    OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    BEGIN_RING(chan, RING_3D_(0x07e0), 2);
    OUT_RING  (chan, size);
    OUT_RING  (chan, size >> 16);
    BEGIN_RING(chan, RING_3D_(0x15c8), 1); /* bits 0x3 */
    OUT_RING  (chan, 2);
    BEGIN_RING(chan, RING_3D_(0x07c0), 4); /* ZCULL dimensions */
    OUT_RING  (chan, width);
    OUT_RING  (chan, height);
    OUT_RING  (chan, 1);
    OUT_RING  (chan, 0);
    BEGIN_RING(chan, RING_3D_(0x15fc), 2);
    OUT_RING  (chan, 0); /* bits 0xffff */
    OUT_RING  (chan, 0); /* bits 0xffff */
    BEGIN_RING(chan, RING_3D_(0x1958), 1);
    OUT_RING  (chan, 0); /* bits ~0 */
}

static void
nvc0_validate_fb(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
    unsigned i;
    unsigned ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS1;
    boolean serialize = FALSE;

    nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_FRAME);

    BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
    OUT_RING  (chan, (076543210 << 4) | fb->nr_cbufs);
    BEGIN_RING(chan, RING_3D(SCREEN_SCISSOR_HORIZ), 2);
    OUT_RING  (chan, fb->width << 16);
    OUT_RING  (chan, fb->height << 16);

    MARK_RING(chan, 9 * fb->nr_cbufs, 2 * fb->nr_cbufs);

    for (i = 0; i < fb->nr_cbufs; ++i) {
        struct nv50_surface *sf = nv50_surface(fb->cbufs[i]);
        struct nv04_resource *res = nv04_resource(sf->base.texture);
        struct nouveau_bo *bo = res->bo;
        uint32_t offset = sf->offset + res->offset;

        BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(i)), 9);
        OUT_RELOCh(chan, res->bo, offset, res->domain | NOUVEAU_BO_RDWR);
        OUT_RELOCl(chan, res->bo, offset, res->domain | NOUVEAU_BO_RDWR);
        if (likely(nouveau_bo_tile_layout(bo))) {
           struct nv50_miptree *mt = nv50_miptree(sf->base.texture);

           assert(sf->base.texture->target != PIPE_BUFFER);

           OUT_RING(chan, sf->width);
           OUT_RING(chan, sf->height);
           OUT_RING(chan, nvc0_format_table[sf->base.format].rt);
           OUT_RING(chan, (mt->layout_3d << 16) |
                    mt->level[sf->base.u.tex.level].tile_mode);
           OUT_RING(chan, sf->base.u.tex.first_layer + sf->depth);
           OUT_RING(chan, mt->layer_stride >> 2);
           OUT_RING(chan, sf->base.u.tex.first_layer);

           ms_mode = mt->ms_mode;
        } else {
           if (res->base.target == PIPE_BUFFER) {
              OUT_RING(chan, 262144);
              OUT_RING(chan, 1);
           } else {
              OUT_RING(chan, nv50_miptree(sf->base.texture)->level[0].pitch);
              OUT_RING(chan, sf->height);
           }
           OUT_RING(chan, nvc0_format_table[sf->base.format].rt);
           OUT_RING(chan, 1 << 12);
           OUT_RING(chan, 1);
           OUT_RING(chan, 0);
           OUT_RING(chan, 0);

           nvc0_resource_fence(res, NOUVEAU_BO_WR);

           assert(!fb->zsbuf);
        }

        if (res->status & NOUVEAU_BUFFER_STATUS_GPU_READING)
           serialize = TRUE;
        res->status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
        res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_READING;

        /* only register for writing, otherwise we'd always serialize here */
        nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_FRAME, res,
                                 res->domain | NOUVEAU_BO_WR);
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
        OUT_RING  (chan, nvc0_format_table[fb->zsbuf->format].rt);
        OUT_RING  (chan, mt->level[sf->base.u.tex.level].tile_mode);
        OUT_RING  (chan, mt->layer_stride >> 2);
        BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
        OUT_RING  (chan, 1);
        BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
        OUT_RING  (chan, sf->width);
        OUT_RING  (chan, sf->height);
        OUT_RING  (chan, (unk << 16) |
                   (sf->base.u.tex.first_layer + sf->depth));
        BEGIN_RING(chan, RING_3D(ZETA_BASE_LAYER), 1);
        OUT_RING  (chan, sf->base.u.tex.first_layer);

        ms_mode = mt->ms_mode;

        if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
           serialize = TRUE;
        mt->base.status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
        mt->base.status &= ~NOUVEAU_BUFFER_STATUS_GPU_READING;

        nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_FRAME, &mt->base,
                                 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
    } else {
        BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
        OUT_RING  (chan, 0);
    }

    IMMED_RING(chan, RING_3D(MULTISAMPLE_MODE), ms_mode);

    if (serialize) {
       BEGIN_RING(chan, RING_3D(SERIALIZE), 1);
       OUT_RING  (chan, 0);
    }
}

static void
nvc0_validate_blend_colour(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;

    BEGIN_RING(chan, RING_3D(BLEND_COLOR(0)), 4);
    OUT_RINGf (chan, nvc0->blend_colour.color[0]);
    OUT_RINGf (chan, nvc0->blend_colour.color[1]);
    OUT_RINGf (chan, nvc0->blend_colour.color[2]);
    OUT_RINGf (chan, nvc0->blend_colour.color[3]);    
}

static void
nvc0_validate_stencil_ref(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    const ubyte *ref = &nvc0->stencil_ref.ref_value[0];

    IMMED_RING(chan, RING_3D(STENCIL_FRONT_FUNC_REF), ref[0]);
    IMMED_RING(chan, RING_3D(STENCIL_BACK_FUNC_REF), ref[1]);
}

static void
nvc0_validate_stipple(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    unsigned i;

    BEGIN_RING(chan, RING_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
    for (i = 0; i < 32; ++i)
        OUT_RING(chan, util_bswap32(nvc0->stipple.stipple[i]));
}

static void
nvc0_validate_scissor(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    struct pipe_scissor_state *s = &nvc0->scissor;

    if (!(nvc0->dirty & NVC0_NEW_SCISSOR) &&
        nvc0->rast->pipe.scissor == nvc0->state.scissor)
       return;
    nvc0->state.scissor = nvc0->rast->pipe.scissor;

    BEGIN_RING(chan, RING_3D(SCISSOR_HORIZ(0)), 2);
    if (nvc0->rast->pipe.scissor) {
       OUT_RING(chan, (s->maxx << 16) | s->minx);
       OUT_RING(chan, (s->maxy << 16) | s->miny);
    } else {
       OUT_RING(chan, (0xffff << 16) | 0);
       OUT_RING(chan, (0xffff << 16) | 0);
    }
}

static void
nvc0_validate_viewport(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    struct pipe_viewport_state *vp = &nvc0->viewport;
    int x, y, w, h;
    float zmin, zmax;

    BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSLATE_X(0)), 3);
    OUT_RINGf (chan, vp->translate[0]);
    OUT_RINGf (chan, vp->translate[1]);
    OUT_RINGf (chan, vp->translate[2]);
    BEGIN_RING(chan, RING_3D(VIEWPORT_SCALE_X(0)), 3);
    OUT_RINGf (chan, vp->scale[0]);
    OUT_RINGf (chan, vp->scale[1]);
    OUT_RINGf (chan, vp->scale[2]);

    /* now set the viewport rectangle to viewport dimensions for clipping */

    x = util_iround(MAX2(0.0f, vp->translate[0] - fabsf(vp->scale[0])));
    y = util_iround(MAX2(0.0f, vp->translate[1] - fabsf(vp->scale[1])));
    w = util_iround(vp->translate[0] + fabsf(vp->scale[0])) - x;
    h = util_iround(vp->translate[1] + fabsf(vp->scale[1])) - y;

    zmin = vp->translate[2] - fabsf(vp->scale[2]);
    zmax = vp->translate[2] + fabsf(vp->scale[2]);

    BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
    OUT_RING  (chan, (w << 16) | x);
    OUT_RING  (chan, (h << 16) | y);
    BEGIN_RING(chan, RING_3D(DEPTH_RANGE_NEAR(0)), 2);
    OUT_RINGf (chan, zmin);
    OUT_RINGf (chan, zmax);
}

static INLINE void
nvc0_upload_uclip_planes(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nouveau_bo *bo = nvc0->screen->uniforms;

   MARK_RING (chan, 6 + PIPE_MAX_CLIP_PLANES * 4, 2);
   BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
   OUT_RING  (chan, 256);
   OUT_RELOCh(chan, bo, 5 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, bo, 5 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   BEGIN_RING_1I(chan, RING_3D(CB_POS), PIPE_MAX_CLIP_PLANES * 4 + 1);
   OUT_RING  (chan, 0);
   OUT_RINGp (chan, &nvc0->clip.ucp[0][0], PIPE_MAX_CLIP_PLANES * 4);
}

static INLINE void
nvc0_check_program_ucps(struct nvc0_context *nvc0,
                        struct nvc0_program *vp, uint8_t mask)
{
   const unsigned n = util_logbase2(mask) + 1;

   if (vp->vp.num_ucps >= n)
      return;
   nvc0_program_destroy(nvc0, vp);

   vp->vp.num_ucps = n;
   if (likely(vp == nvc0->vertprog))
      nvc0_vertprog_validate(nvc0);
   else
   if (likely(vp == nvc0->gmtyprog))
      nvc0_vertprog_validate(nvc0);
   else
      nvc0_tevlprog_validate(nvc0);
}

static void
nvc0_validate_clip(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *vp;
   uint8_t clip_enable = nvc0->rast->pipe.clip_plane_enable;

   if (nvc0->dirty & NVC0_NEW_CLIP)
      nvc0_upload_uclip_planes(nvc0);

   vp = nvc0->gmtyprog;
   if (!vp) {
      vp = nvc0->tevlprog;
      if (!vp)
         vp = nvc0->vertprog;
   }

   if (clip_enable && vp->vp.num_ucps < PIPE_MAX_CLIP_PLANES)
      nvc0_check_program_ucps(nvc0, vp, clip_enable);

   clip_enable &= vp->vp.clip_enable;

   if (nvc0->state.clip_enable != clip_enable) {
      nvc0->state.clip_enable = clip_enable;
      IMMED_RING(chan, RING_3D(CLIP_DISTANCE_ENABLE), clip_enable);
   }
   if (nvc0->state.clip_mode != vp->vp.clip_mode) {
      nvc0->state.clip_mode = vp->vp.clip_mode;
      BEGIN_RING(chan, RING_3D(CLIP_DISTANCE_MODE), 1);
      OUT_RING  (chan, vp->vp.clip_mode);
   }
}

static void
nvc0_validate_blend(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   WAIT_RING(chan, nvc0->blend->size);
   OUT_RINGp(chan, nvc0->blend->state, nvc0->blend->size);
}

static void
nvc0_validate_zsa(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   WAIT_RING(chan, nvc0->zsa->size);
   OUT_RINGp(chan, nvc0->zsa->state, nvc0->zsa->size);
}

static void
nvc0_validate_rasterizer(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   WAIT_RING(chan, nvc0->rast->size);
   OUT_RINGp(chan, nvc0->rast->state, nvc0->rast->size);
}

static void
nvc0_constbufs_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nouveau_bo *bo;
   unsigned s;

   for (s = 0; s < 5; ++s) {
      struct nv04_resource *res;
      int i;

      while (nvc0->constbuf_dirty[s]) {
         unsigned base = 0;
         unsigned words = 0;
         boolean rebind = TRUE;

         i = ffs(nvc0->constbuf_dirty[s]) - 1;
         nvc0->constbuf_dirty[s] &= ~(1 << i);

         res = nv04_resource(nvc0->constbuf[s][i]);
         if (!res) {
            BEGIN_RING(chan, RING_3D(CB_BIND(s)), 1);
            OUT_RING  (chan, (i << 4) | 0);
            if (i == 0)
               nvc0->state.uniform_buffer_bound[s] = 0;
            continue;
         }

         if (!nouveau_resource_mapped_by_gpu(&res->base)) {
            if (i == 0 && (res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY)) {
               base = s << 16;
               bo = nvc0->screen->uniforms;

               if (nvc0->state.uniform_buffer_bound[s] >= res->base.width0)
                  rebind = FALSE;
               else
                  nvc0->state.uniform_buffer_bound[s] =
                     align(res->base.width0, 0x100);

               words = res->base.width0 / 4;
            } else {
               nouveau_buffer_migrate(&nvc0->base, res, NOUVEAU_BO_VRAM);
               bo = res->bo;
               base = res->offset;
            }
         } else {
            bo = res->bo;
            base = res->offset;
            if (i == 0)
               nvc0->state.uniform_buffer_bound[s] = 0;
         }

         if (bo != nvc0->screen->uniforms)
            nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_CONSTANT, res,
                                     NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

         if (rebind) {
            MARK_RING (chan, 4, 2);
            BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
            OUT_RING  (chan, align(res->base.width0, 0x100));
            OUT_RELOCh(chan, bo, base, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
            OUT_RELOCl(chan, bo, base, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
            BEGIN_RING(chan, RING_3D(CB_BIND(s)), 1);
            OUT_RING  (chan, (i << 4) | 1);
         }

         if (words)
            nvc0_cb_push(&nvc0->base,
                         bo, NOUVEAU_BO_VRAM, base, res->base.width0,
                         0, words, (const uint32_t *)res->data);
      }
   }
}

static void
nvc0_validate_sample_mask(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   unsigned mask[4] =
   {
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff
   };

   BEGIN_RING(chan, RING_3D(MSAA_MASK(0)), 4);
   OUT_RING  (chan, mask[0]);
   OUT_RING  (chan, mask[1]);
   OUT_RING  (chan, mask[2]);
   OUT_RING  (chan, mask[3]);
   BEGIN_RING(chan, RING_3D(SAMPLE_SHADING), 1);
   OUT_RING  (chan, 0x01);
}

static void
nvc0_validate_derived_1(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   boolean early_z;
   boolean rasterizer_discard;

   early_z = nvc0->fragprog->fp.early_z && !nvc0->zsa->pipe.alpha.enabled;

   if (early_z != nvc0->state.early_z) {
      nvc0->state.early_z = early_z;
      IMMED_RING(chan, RING_3D(EARLY_FRAGMENT_TESTS), early_z);
   }

   rasterizer_discard = (!nvc0->fragprog || !nvc0->fragprog->hdr[18]) &&
      !nvc0->zsa->pipe.depth.enabled && !nvc0->zsa->pipe.stencil[0].enabled;
   rasterizer_discard = rasterizer_discard ||
      nvc0->rast->pipe.rasterizer_discard;

   if (rasterizer_discard != nvc0->state.rasterizer_discard) {
      nvc0->state.rasterizer_discard = rasterizer_discard;
      IMMED_RING(chan, RING_3D(RASTERIZE_ENABLE), !rasterizer_discard);
   }
}

static void
nvc0_switch_pipe_context(struct nvc0_context *ctx_to)
{
   struct nvc0_context *ctx_from = ctx_to->screen->cur_ctx;

   if (ctx_from)
      ctx_to->state = ctx_from->state;

   ctx_to->dirty = ~0;

   if (!ctx_to->vertex)
      ctx_to->dirty &= ~(NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS);

   if (!ctx_to->vertprog)
      ctx_to->dirty &= ~NVC0_NEW_VERTPROG;
   if (!ctx_to->fragprog)
      ctx_to->dirty &= ~NVC0_NEW_FRAGPROG;

   if (!ctx_to->blend)
      ctx_to->dirty &= ~NVC0_NEW_BLEND;
   if (!ctx_to->rast)
      ctx_to->dirty &= ~(NVC0_NEW_RASTERIZER | NVC0_NEW_SCISSOR);
   if (!ctx_to->zsa)
      ctx_to->dirty &= ~NVC0_NEW_ZSA;

   ctx_to->screen->cur_ctx = ctx_to;
}

static struct state_validate {
    void (*func)(struct nvc0_context *);
    uint32_t states;
} validate_list[] = {
    { nvc0_validate_fb,            NVC0_NEW_FRAMEBUFFER },
    { nvc0_validate_blend,         NVC0_NEW_BLEND },
    { nvc0_validate_zsa,           NVC0_NEW_ZSA },
    { nvc0_validate_sample_mask,   NVC0_NEW_SAMPLE_MASK },
    { nvc0_validate_rasterizer,    NVC0_NEW_RASTERIZER },
    { nvc0_validate_blend_colour,  NVC0_NEW_BLEND_COLOUR },
    { nvc0_validate_stencil_ref,   NVC0_NEW_STENCIL_REF },
    { nvc0_validate_stipple,       NVC0_NEW_STIPPLE },
    { nvc0_validate_scissor,       NVC0_NEW_SCISSOR | NVC0_NEW_RASTERIZER },
    { nvc0_validate_viewport,      NVC0_NEW_VIEWPORT },
    { nvc0_vertprog_validate,      NVC0_NEW_VERTPROG },
    { nvc0_tctlprog_validate,      NVC0_NEW_TCTLPROG },
    { nvc0_tevlprog_validate,      NVC0_NEW_TEVLPROG },
    { nvc0_gmtyprog_validate,      NVC0_NEW_GMTYPROG },
    { nvc0_fragprog_validate,      NVC0_NEW_FRAGPROG },
    { nvc0_validate_derived_1,     NVC0_NEW_FRAGPROG | NVC0_NEW_ZSA |
                                   NVC0_NEW_RASTERIZER },
    { nvc0_validate_clip,          NVC0_NEW_CLIP | NVC0_NEW_RASTERIZER |
                                   NVC0_NEW_VERTPROG |
                                   NVC0_NEW_TEVLPROG |
                                   NVC0_NEW_GMTYPROG },
    { nvc0_constbufs_validate,     NVC0_NEW_CONSTBUF },
    { nvc0_validate_textures,      NVC0_NEW_TEXTURES },
    { nvc0_validate_samplers,      NVC0_NEW_SAMPLERS },
    { nvc0_vertex_arrays_validate, NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS },
    { nvc0_tfb_validate,           NVC0_NEW_TFB_TARGETS | NVC0_NEW_GMTYPROG }
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nvc0_state_validate(struct nvc0_context *nvc0, uint32_t mask, unsigned words)
{
   uint32_t state_mask;
   unsigned i;

   if (nvc0->screen->cur_ctx != nvc0)
      nvc0_switch_pipe_context(nvc0);

   state_mask = nvc0->dirty & mask;

   if (state_mask) {
      for (i = 0; i < validate_list_len; ++i) {
         struct state_validate *validate = &validate_list[i];

         if (state_mask & validate->states)
            validate->func(nvc0);
      }
      nvc0->dirty &= ~state_mask;
   }

   MARK_RING(nvc0->screen->base.channel, words, 0);

   nvc0_bufctx_emit_relocs(nvc0);

   return TRUE;
}
