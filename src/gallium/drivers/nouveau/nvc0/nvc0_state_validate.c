
#include "util/u_math.h"

#include "nvc0/nvc0_context.h"
#include "nv50/nv50_defs.xml.h"

#if 0
static void
nvc0_validate_zcull(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
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

    BEGIN_NVC0(push, NVC0_3D(ZCULL_REGION), 1);
    PUSH_DATA (push, 0);
    BEGIN_NVC0(push, NVC0_3D(ZCULL_ADDRESS_HIGH), 2);
    PUSH_DATAh(push, bo->offset + offset);
    PUSH_DATA (push, bo->offset + offset);
    offset += 1 << 17;
    BEGIN_NVC0(push, NVC0_3D(ZCULL_LIMIT_HIGH), 2);
    PUSH_DATAh(push, bo->offset + offset);
    PUSH_DATA (push, bo->offset + offset);
    BEGIN_NVC0(push, SUBC_3D(0x07e0), 2);
    PUSH_DATA (push, size);
    PUSH_DATA (push, size >> 16);
    BEGIN_NVC0(push, SUBC_3D(0x15c8), 1); /* bits 0x3 */
    PUSH_DATA (push, 2);
    BEGIN_NVC0(push, NVC0_3D(ZCULL_WIDTH), 4);
    PUSH_DATA (push, width);
    PUSH_DATA (push, height);
    PUSH_DATA (push, 1);
    PUSH_DATA (push, 0);
    BEGIN_NVC0(push, NVC0_3D(ZCULL_WINDOW_OFFSET_X), 2);
    PUSH_DATA (push, 0);
    PUSH_DATA (push, 0);
    BEGIN_NVC0(push, NVC0_3D(ZCULL_INVALIDATE), 1);
    PUSH_DATA (push, 0);
}
#endif

static INLINE void
nvc0_fb_set_null_rt(struct nouveau_pushbuf *push, unsigned i)
{
   BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(i)), 6);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 64);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, NV50_SURFACE_FORMAT_NONE);
   PUSH_DATA (push, 0);
}

static void
nvc0_validate_fb(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
    struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
    unsigned i, ms;
    unsigned ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS1;
    boolean serialize = FALSE;

    nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_FB);

    BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
    PUSH_DATA (push, (076543210 << 4) | fb->nr_cbufs);
    BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
    PUSH_DATA (push, fb->width << 16);
    PUSH_DATA (push, fb->height << 16);

    for (i = 0; i < fb->nr_cbufs; ++i) {
        struct nv50_surface *sf;
        struct nv04_resource *res;
        struct nouveau_bo *bo;

        if (!fb->cbufs[i]) {
           nvc0_fb_set_null_rt(push, i);
           continue;
        }

        sf = nv50_surface(fb->cbufs[i]);
        res = nv04_resource(sf->base.texture);
        bo = res->bo;

        BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(i)), 9);
        PUSH_DATAh(push, res->address + sf->offset);
        PUSH_DATA (push, res->address + sf->offset);
        if (likely(nouveau_bo_memtype(bo))) {
           struct nv50_miptree *mt = nv50_miptree(sf->base.texture);

           assert(sf->base.texture->target != PIPE_BUFFER);

           PUSH_DATA(push, sf->width);
           PUSH_DATA(push, sf->height);
           PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
           PUSH_DATA(push, (mt->layout_3d << 16) |
                    mt->level[sf->base.u.tex.level].tile_mode);
           PUSH_DATA(push, sf->base.u.tex.first_layer + sf->depth);
           PUSH_DATA(push, mt->layer_stride >> 2);
           PUSH_DATA(push, sf->base.u.tex.first_layer);

           ms_mode = mt->ms_mode;
        } else {
           if (res->base.target == PIPE_BUFFER) {
              PUSH_DATA(push, 262144);
              PUSH_DATA(push, 1);
           } else {
              PUSH_DATA(push, nv50_miptree(sf->base.texture)->level[0].pitch);
              PUSH_DATA(push, sf->height);
           }
           PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
           PUSH_DATA(push, 1 << 12);
           PUSH_DATA(push, 1);
           PUSH_DATA(push, 0);
           PUSH_DATA(push, 0);

           nvc0_resource_fence(res, NOUVEAU_BO_WR);

           assert(!fb->zsbuf);
        }

        if (res->status & NOUVEAU_BUFFER_STATUS_GPU_READING)
           serialize = TRUE;
        res->status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
        res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_READING;

        /* only register for writing, otherwise we'd always serialize here */
        BCTX_REFN(nvc0->bufctx_3d, FB, res, WR);
    }

    if (fb->zsbuf) {
        struct nv50_miptree *mt = nv50_miptree(fb->zsbuf->texture);
        struct nv50_surface *sf = nv50_surface(fb->zsbuf);
        int unk = mt->base.base.target == PIPE_TEXTURE_2D;

        BEGIN_NVC0(push, NVC0_3D(ZETA_ADDRESS_HIGH), 5);
        PUSH_DATAh(push, mt->base.address + sf->offset);
        PUSH_DATA (push, mt->base.address + sf->offset);
        PUSH_DATA (push, nvc0_format_table[fb->zsbuf->format].rt);
        PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
        PUSH_DATA (push, mt->layer_stride >> 2);
        BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
        PUSH_DATA (push, 1);
        BEGIN_NVC0(push, NVC0_3D(ZETA_HORIZ), 3);
        PUSH_DATA (push, sf->width);
        PUSH_DATA (push, sf->height);
        PUSH_DATA (push, (unk << 16) |
                   (sf->base.u.tex.first_layer + sf->depth));
        BEGIN_NVC0(push, NVC0_3D(ZETA_BASE_LAYER), 1);
        PUSH_DATA (push, sf->base.u.tex.first_layer);

        ms_mode = mt->ms_mode;

        if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
           serialize = TRUE;
        mt->base.status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
        mt->base.status &= ~NOUVEAU_BUFFER_STATUS_GPU_READING;

        BCTX_REFN(nvc0->bufctx_3d, FB, &mt->base, WR);
    } else {
        BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
        PUSH_DATA (push, 0);
    }

    IMMED_NVC0(push, NVC0_3D(MULTISAMPLE_MODE), ms_mode);

    ms = 1 << ms_mode;
    BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
    PUSH_DATA (push, 512);
    PUSH_DATAh(push, nvc0->screen->uniform_bo->offset + (5 << 16) + (4 << 9));
    PUSH_DATA (push, nvc0->screen->uniform_bo->offset + (5 << 16) + (4 << 9));
    BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 2 * ms);
    PUSH_DATA (push, 256 + 128);
    for (i = 0; i < ms; i++) {
       float xy[2];
       nvc0->base.pipe.get_sample_position(&nvc0->base.pipe, ms, i, xy);
       PUSH_DATAf(push, xy[0]);
       PUSH_DATAf(push, xy[1]);
    }

    if (serialize)
       IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);

    NOUVEAU_DRV_STAT(&nvc0->screen->base, gpu_serialize_count, serialize);
}

static void
nvc0_validate_blend_colour(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   BEGIN_NVC0(push, NVC0_3D(BLEND_COLOR(0)), 4);
   PUSH_DATAf(push, nvc0->blend_colour.color[0]);
   PUSH_DATAf(push, nvc0->blend_colour.color[1]);
   PUSH_DATAf(push, nvc0->blend_colour.color[2]);
   PUSH_DATAf(push, nvc0->blend_colour.color[3]);
}

static void
nvc0_validate_stencil_ref(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
    const ubyte *ref = &nvc0->stencil_ref.ref_value[0];

    IMMED_NVC0(push, NVC0_3D(STENCIL_FRONT_FUNC_REF), ref[0]);
    IMMED_NVC0(push, NVC0_3D(STENCIL_BACK_FUNC_REF), ref[1]);
}

static void
nvc0_validate_stipple(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
    unsigned i;

    BEGIN_NVC0(push, NVC0_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
    for (i = 0; i < 32; ++i)
        PUSH_DATA(push, util_bswap32(nvc0->stipple.stipple[i]));
}

static void
nvc0_validate_scissor(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
    struct pipe_scissor_state *s = &nvc0->scissor;

    if (!(nvc0->dirty & NVC0_NEW_SCISSOR) &&
        nvc0->rast->pipe.scissor == nvc0->state.scissor)
       return;
    nvc0->state.scissor = nvc0->rast->pipe.scissor;

    BEGIN_NVC0(push, NVC0_3D(SCISSOR_HORIZ(0)), 2);
    if (nvc0->rast->pipe.scissor) {
       PUSH_DATA(push, (s->maxx << 16) | s->minx);
       PUSH_DATA(push, (s->maxy << 16) | s->miny);
    } else {
       PUSH_DATA(push, (0xffff << 16) | 0);
       PUSH_DATA(push, (0xffff << 16) | 0);
    }
}

static void
nvc0_validate_viewport(struct nvc0_context *nvc0)
{
    struct nouveau_pushbuf *push = nvc0->base.pushbuf;
    struct pipe_viewport_state *vp = &nvc0->viewport;
    int x, y, w, h;
    float zmin, zmax;

    BEGIN_NVC0(push, NVC0_3D(VIEWPORT_TRANSLATE_X(0)), 3);
    PUSH_DATAf(push, vp->translate[0]);
    PUSH_DATAf(push, vp->translate[1]);
    PUSH_DATAf(push, vp->translate[2]);
    BEGIN_NVC0(push, NVC0_3D(VIEWPORT_SCALE_X(0)), 3);
    PUSH_DATAf(push, vp->scale[0]);
    PUSH_DATAf(push, vp->scale[1]);
    PUSH_DATAf(push, vp->scale[2]);

    /* now set the viewport rectangle to viewport dimensions for clipping */

    x = util_iround(MAX2(0.0f, vp->translate[0] - fabsf(vp->scale[0])));
    y = util_iround(MAX2(0.0f, vp->translate[1] - fabsf(vp->scale[1])));
    w = util_iround(vp->translate[0] + fabsf(vp->scale[0])) - x;
    h = util_iround(vp->translate[1] + fabsf(vp->scale[1])) - y;

    zmin = vp->translate[2] - fabsf(vp->scale[2]);
    zmax = vp->translate[2] + fabsf(vp->scale[2]);

    nvc0->vport_int[0] = (w << 16) | x;
    nvc0->vport_int[1] = (h << 16) | y;
    BEGIN_NVC0(push, NVC0_3D(VIEWPORT_HORIZ(0)), 2);
    PUSH_DATA (push, nvc0->vport_int[0]);
    PUSH_DATA (push, nvc0->vport_int[1]);
    BEGIN_NVC0(push, NVC0_3D(DEPTH_RANGE_NEAR(0)), 2);
    PUSH_DATAf(push, zmin);
    PUSH_DATAf(push, zmax);
}

static INLINE void
nvc0_upload_uclip_planes(struct nvc0_context *nvc0, unsigned s)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bo *bo = nvc0->screen->uniform_bo;

   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, 512);
   PUSH_DATAh(push, bo->offset + (5 << 16) + (s << 9));
   PUSH_DATA (push, bo->offset + (5 << 16) + (s << 9));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), PIPE_MAX_CLIP_PLANES * 4 + 1);
   PUSH_DATA (push, 256);
   PUSH_DATAp(push, &nvc0->clip.ucp[0][0], PIPE_MAX_CLIP_PLANES * 4);
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
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *vp;
   unsigned stage;
   uint8_t clip_enable = nvc0->rast->pipe.clip_plane_enable;

   if (nvc0->gmtyprog) {
      stage = 3;
      vp = nvc0->gmtyprog;
   } else
   if (nvc0->tevlprog) {
      stage = 2;
      vp = nvc0->tevlprog;
   } else {
      stage = 0;
      vp = nvc0->vertprog;
   }

   if (clip_enable && vp->vp.num_ucps < PIPE_MAX_CLIP_PLANES)
      nvc0_check_program_ucps(nvc0, vp, clip_enable);

   if (nvc0->dirty & (NVC0_NEW_CLIP | (NVC0_NEW_VERTPROG << stage)))
      if (vp->vp.num_ucps > 0 && vp->vp.num_ucps <= PIPE_MAX_CLIP_PLANES)
         nvc0_upload_uclip_planes(nvc0, stage);

   clip_enable &= vp->vp.clip_enable;

   if (nvc0->state.clip_enable != clip_enable) {
      nvc0->state.clip_enable = clip_enable;
      IMMED_NVC0(push, NVC0_3D(CLIP_DISTANCE_ENABLE), clip_enable);
   }
   if (nvc0->state.clip_mode != vp->vp.clip_mode) {
      nvc0->state.clip_mode = vp->vp.clip_mode;
      BEGIN_NVC0(push, NVC0_3D(CLIP_DISTANCE_MODE), 1);
      PUSH_DATA (push, vp->vp.clip_mode);
   }
}

static void
nvc0_validate_blend(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   PUSH_SPACE(push, nvc0->blend->size);
   PUSH_DATAp(push, nvc0->blend->state, nvc0->blend->size);
}

static void
nvc0_validate_zsa(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   PUSH_SPACE(push, nvc0->zsa->size);
   PUSH_DATAp(push, nvc0->zsa->state, nvc0->zsa->size);
}

static void
nvc0_validate_rasterizer(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   PUSH_SPACE(push, nvc0->rast->size);
   PUSH_DATAp(push, nvc0->rast->state, nvc0->rast->size);
}

static void
nvc0_constbufs_validate(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned s;

   for (s = 0; s < 5; ++s) {
      while (nvc0->constbuf_dirty[s]) {
         int i = ffs(nvc0->constbuf_dirty[s]) - 1;
         nvc0->constbuf_dirty[s] &= ~(1 << i);

         if (nvc0->constbuf[s][i].user) {
            struct nouveau_bo *bo = nvc0->screen->uniform_bo;
            const unsigned base = s << 16;
            const unsigned size = nvc0->constbuf[s][0].size;
            assert(i == 0); /* we really only want OpenGL uniforms here */
            assert(nvc0->constbuf[s][0].u.data);

            if (nvc0->state.uniform_buffer_bound[s] < size) {
               nvc0->state.uniform_buffer_bound[s] = align(size, 0x100);

               BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
               PUSH_DATA (push, nvc0->state.uniform_buffer_bound[s]);
               PUSH_DATAh(push, bo->offset + base);
               PUSH_DATA (push, bo->offset + base);
               BEGIN_NVC0(push, NVC0_3D(CB_BIND(s)), 1);
               PUSH_DATA (push, (0 << 4) | 1);
            }
            nvc0_cb_push(&nvc0->base, bo, NOUVEAU_BO_VRAM,
                         base, nvc0->state.uniform_buffer_bound[s],
                         0, (size + 3) / 4,
                         nvc0->constbuf[s][0].u.data);
         } else {
            struct nv04_resource *res =
               nv04_resource(nvc0->constbuf[s][i].u.buf);
            if (res) {
               BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
               PUSH_DATA (push, nvc0->constbuf[s][i].size);
               PUSH_DATAh(push, res->address + nvc0->constbuf[s][i].offset);
               PUSH_DATA (push, res->address + nvc0->constbuf[s][i].offset);
               BEGIN_NVC0(push, NVC0_3D(CB_BIND(s)), 1);
               PUSH_DATA (push, (i << 4) | 1);

               BCTX_REFN(nvc0->bufctx_3d, CB(s, i), res, RD);
            } else {
               BEGIN_NVC0(push, NVC0_3D(CB_BIND(s)), 1);
               PUSH_DATA (push, (i << 4) | 0);
            }
            if (i == 0)
               nvc0->state.uniform_buffer_bound[s] = 0;
         }
      }
   }
}

static void
nvc0_validate_sample_mask(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;

   unsigned mask[4] =
   {
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff,
      nvc0->sample_mask & 0xffff
   };

   BEGIN_NVC0(push, NVC0_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, mask[0]);
   PUSH_DATA (push, mask[1]);
   PUSH_DATA (push, mask[2]);
   PUSH_DATA (push, mask[3]);
}

static void
nvc0_validate_min_samples(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   int samples;

   samples = util_next_power_of_two(nvc0->min_samples);
   if (samples > 1)
      samples |= NVC0_3D_SAMPLE_SHADING_ENABLE;

   IMMED_NVC0(push, NVC0_3D(SAMPLE_SHADING), samples);
}

void
nvc0_validate_global_residents(struct nvc0_context *nvc0,
                               struct nouveau_bufctx *bctx, int bin)
{
   unsigned i;

   for (i = 0; i < nvc0->global_residents.size / sizeof(struct pipe_resource *);
        ++i) {
      struct pipe_resource *res = *util_dynarray_element(
         &nvc0->global_residents, struct pipe_resource *, i);
      if (res)
         nvc0_add_resident(bctx, bin, nv04_resource(res), NOUVEAU_BO_RDWR);
   }
}

static void
nvc0_validate_derived_1(struct nvc0_context *nvc0)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   boolean rasterizer_discard;

   if (nvc0->rast && nvc0->rast->pipe.rasterizer_discard) {
      rasterizer_discard = TRUE;
   } else {
      boolean zs = nvc0->zsa &&
         (nvc0->zsa->pipe.depth.enabled || nvc0->zsa->pipe.stencil[0].enabled);
      rasterizer_discard = !zs &&
         (!nvc0->fragprog || !nvc0->fragprog->hdr[18]);
   }

   if (rasterizer_discard != nvc0->state.rasterizer_discard) {
      nvc0->state.rasterizer_discard = rasterizer_discard;
      IMMED_NVC0(push, NVC0_3D(RASTERIZE_ENABLE), !rasterizer_discard);
   }
}

static void
nvc0_switch_pipe_context(struct nvc0_context *ctx_to)
{
   struct nvc0_context *ctx_from = ctx_to->screen->cur_ctx;
   unsigned s;

   if (ctx_from)
      ctx_to->state = ctx_from->state;

   ctx_to->dirty = ~0;

   for (s = 0; s < 5; ++s) {
      ctx_to->samplers_dirty[s] = ~0;
      ctx_to->textures_dirty[s] = ~0;
      ctx_to->constbuf_dirty[s] = (1 << NVC0_MAX_PIPE_CONSTBUFS) - 1;
   }

   if (!ctx_to->vertex)
      ctx_to->dirty &= ~(NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS);
   if (!ctx_to->idxbuf.buffer)
      ctx_to->dirty &= ~NVC0_NEW_IDXBUF;

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
    { nve4_set_tex_handles,        NVC0_NEW_TEXTURES | NVC0_NEW_SAMPLERS },
    { nvc0_vertex_arrays_validate, NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS },
    { nvc0_validate_surfaces,      NVC0_NEW_SURFACES },
    { nvc0_idxbuf_validate,        NVC0_NEW_IDXBUF },
    { nvc0_tfb_validate,           NVC0_NEW_TFB_TARGETS | NVC0_NEW_GMTYPROG },
    { nvc0_validate_min_samples,   NVC0_NEW_MIN_SAMPLES },
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nvc0_state_validate(struct nvc0_context *nvc0, uint32_t mask, unsigned words)
{
   uint32_t state_mask;
   int ret;
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

      nvc0_bufctx_fence(nvc0, nvc0->bufctx_3d, FALSE);
   }

   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx_3d);
   ret = nouveau_pushbuf_validate(nvc0->base.pushbuf);

   if (unlikely(nvc0->state.flushed)) {
      nvc0->state.flushed = FALSE;
      nvc0_bufctx_fence(nvc0, nvc0->bufctx_3d, TRUE);
   }
   return !ret;
}
