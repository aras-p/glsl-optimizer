
#include "nvc0_context.h"
#include "os/os_time.h"

static void
nvc0_validate_zcull(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;
    struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
    struct nvc0_miptree *mt = nvc0_miptree(fb->zsbuf->texture);
    struct nouveau_bo *bo = mt->base.bo;
    uint32_t size;
    uint32_t offset = align(mt->total_size, 1 << 17);
    unsigned width, height;

    size = mt->total_size * 2;

    height = align(fb->height, 32);
    width = fb->width % 224;
    if (width)
       width = fb->width + (224 - width);
    else
       width = fb->width;

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

    nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_FRAME);

    BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
    OUT_RING  (chan, (076543210 << 4) | fb->nr_cbufs);
    BEGIN_RING(chan, RING_3D(SCREEN_SCISSOR_HORIZ), 2);
    OUT_RING  (chan, fb->width << 16);
    OUT_RING  (chan, fb->height << 16);

    for (i = 0; i < fb->nr_cbufs; ++i) {
        struct nvc0_miptree *mt = nvc0_miptree(fb->cbufs[i]->texture);
        struct nouveau_bo *bo = mt->base.bo;
        unsigned offset = fb->cbufs[i]->offset;
        
        BEGIN_RING(chan, RING_3D(RT_ADDRESS_HIGH(i)), 8);
        OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
        OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
        OUT_RING  (chan, fb->cbufs[i]->width);
        OUT_RING  (chan, fb->cbufs[i]->height);
        OUT_RING  (chan, nvc0_format_table[fb->cbufs[i]->format].rt);
        OUT_RING  (chan, mt->level[fb->cbufs[i]->level].tile_mode);
        OUT_RING  (chan, 1);
        OUT_RING  (chan, 0);

        nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_FRAME, &mt->base,
                                 NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    }

    if (fb->zsbuf) {
        struct nvc0_miptree *mt = nvc0_miptree(fb->zsbuf->texture);
        struct nouveau_bo *bo = mt->base.bo;
        unsigned offset = fb->zsbuf->offset;
        
        BEGIN_RING(chan, RING_3D(ZETA_ADDRESS_HIGH), 5);
        OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
        OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
        OUT_RING  (chan, nvc0_format_table[fb->zsbuf->format].rt);
        OUT_RING  (chan, mt->level[fb->zsbuf->level].tile_mode);
        OUT_RING  (chan, 0);
        BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
        OUT_RING  (chan, 1);
        BEGIN_RING(chan, RING_3D(ZETA_HORIZ), 3);
        OUT_RING  (chan, fb->zsbuf->width);
        OUT_RING  (chan, fb->zsbuf->height);
        OUT_RING  (chan, (1 << 16) | 1);

        nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_FRAME, &mt->base,
                                 NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
    } else {
        BEGIN_RING(chan, RING_3D(ZETA_ENABLE), 1);
        OUT_RING  (chan, 0);
    }

    BEGIN_RING(chan, RING_3D(VIEWPORT_HORIZ(0)), 2);
    OUT_RING  (chan, fb->width << 16);
    OUT_RING  (chan, fb->height << 16);
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

    BEGIN_RING(chan, RING_3D(STENCIL_FRONT_FUNC_REF), 1);
    OUT_RING  (chan, nvc0->stencil_ref.ref_value[0]);
    BEGIN_RING(chan, RING_3D(STENCIL_BACK_FUNC_REF), 1);
    OUT_RING  (chan, nvc0->stencil_ref.ref_value[1]);
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
	nvc0->state.scissor == nvc0->rast->pipe.scissor)
       return;
    nvc0->state.scissor = nvc0->rast->pipe.scissor;

    if (nvc0->state.scissor) {
       BEGIN_RING(chan, RING_3D(SCISSOR_HORIZ(0)), 2);
       OUT_RING  (chan, (s->maxx << 16) | s->minx);
       OUT_RING  (chan, (s->maxy << 16) | s->miny);
    } else {
       BEGIN_RING(chan, RING_3D(SCISSOR_HORIZ(0)), 2);
       OUT_RING  (chan, nvc0->framebuffer.width << 16);
       OUT_RING  (chan, nvc0->framebuffer.height << 16);
    }
}

static void
nvc0_validate_viewport(struct nvc0_context *nvc0)
{
    struct nouveau_channel *chan = nvc0->screen->base.channel;

    BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSLATE_X(0)), 3);
    OUT_RINGf (chan, nvc0->viewport.translate[0]);
    OUT_RINGf (chan, nvc0->viewport.translate[1]);
    OUT_RINGf (chan, nvc0->viewport.translate[2]);
    BEGIN_RING(chan, RING_3D(VIEWPORT_SCALE_X(0)), 3);
    OUT_RINGf (chan, nvc0->viewport.scale[0]);
    OUT_RINGf (chan, nvc0->viewport.scale[1]);
    OUT_RINGf (chan, nvc0->viewport.scale[2]);
}

static void
nvc0_validate_clip(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   uint32_t clip;

   clip = nvc0->clip.depth_clamp ? 0x201a : 0x0002;

   BEGIN_RING(chan, RING_3D(VIEW_VOLUME_CLIP_CTRL), 1);
   OUT_RING  (chan, clip);
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
      struct nvc0_resource *res;
      int i, j;

      while (nvc0->constbuf_dirty[s]) {
         unsigned offset = 0;
         i = ffs(nvc0->constbuf_dirty[s]) - 1;
         nvc0->constbuf_dirty[s] &= ~(1 << i);

         res = nvc0_resource(nvc0->constbuf[s][i]);
         if (!res) {
            BEGIN_RING(chan, RING_3D(CB_BIND(s)), 1);
            OUT_RING  (chan, (i << 4) | 0);
            continue;
         }

         if (i == 0 && !nvc0_resource_mapped_by_gpu(&res->base)) {
            offset = s << 16;
            bo = nvc0->screen->uniforms;
         } else {
            bo = res->bo;
            nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_CONSTANT, res,
                                     NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         }

         BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
         OUT_RING  (chan, align(res->base.width0, 0x100));
         OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         BEGIN_RING(chan, RING_3D(CB_BIND(s)), 1);
         OUT_RING  (chan, (i << 4) | 1);

         BEGIN_RING(chan, RING_3D(CB_SIZE), 4);
         OUT_RING  (chan, align(res->base.width0, 0x100));
         OUT_RELOCh(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         OUT_RELOCl(chan, bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
         OUT_RING  (chan, 0);
	 BEGIN_RING_NI(chan, RING_3D(CB_DATA(0)), res->base.width0 / 4);
         for (j = 0; j < res->base.width0 / 4; ++j)
            OUT_RING(chan, ((uint32_t *)res->data)[j]);
      }
   }
}

static struct state_validate {
    void (*func)(struct nvc0_context *);
    uint32_t states;
} validate_list[] = {
    { nvc0_validate_fb,            NVC0_NEW_FRAMEBUFFER },
    { nvc0_validate_blend,         NVC0_NEW_BLEND },
    { nvc0_validate_zsa,           NVC0_NEW_ZSA },
    { nvc0_validate_rasterizer,    NVC0_NEW_RASTERIZER },
    { nvc0_validate_blend_colour,  NVC0_NEW_BLEND_COLOUR },
    { nvc0_validate_stencil_ref,   NVC0_NEW_STENCIL_REF },
    { nvc0_validate_stipple,       NVC0_NEW_STIPPLE },
    { nvc0_validate_scissor,       NVC0_NEW_SCISSOR | NVC0_NEW_FRAMEBUFFER |
                                   NVC0_NEW_RASTERIZER },
    { nvc0_validate_viewport,      NVC0_NEW_VIEWPORT },
    { nvc0_validate_clip,          NVC0_NEW_CLIP },
    { nvc0_vertprog_validate,      NVC0_NEW_VERTPROG },
    { nvc0_tctlprog_validate,      NVC0_NEW_TCTLPROG },
    { nvc0_tevlprog_validate,      NVC0_NEW_TEVLPROG },
    { nvc0_gmtyprog_validate,      NVC0_NEW_GMTYPROG },
    { nvc0_fragprog_validate,      NVC0_NEW_FRAGPROG },
    { nvc0_vertex_arrays_validate, NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS },
    { nvc0_validate_textures,      NVC0_NEW_TEXTURES },
    { nvc0_validate_samplers,      NVC0_NEW_SAMPLERS },
    { nvc0_constbufs_validate,     NVC0_NEW_CONSTBUF }
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nvc0_state_validate(struct nvc0_context *nvc0)
{
   unsigned i;
#if 0
   if (nvc0->screen->cur_ctx != nvc0) /* FIXME: not everything is valid */
      nvc0->dirty = 0xffffffff;
#endif
   nvc0->screen->cur_ctx = nvc0;

   if (nvc0->dirty) {
      FIRE_RING(nvc0->screen->base.channel);

      for (i = 0; i < validate_list_len; ++i) {
         struct state_validate *validate = &validate_list[i];

         if (nvc0->dirty & validate->states)
            validate->func(nvc0);
      }
      nvc0->dirty = 0;
   }

   nvc0_bufctx_emit_relocs(nvc0);

   return TRUE;
}
