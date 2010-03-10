#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nouveau/nouveau_util.h"
#include "nv50_context.h"

struct push_context {
   struct nv50_context *nv50;

   unsigned vtx_size;

   void *idxbuf;
   unsigned idxsize;

   float edgeflag;
   int edgeflag_attr;

   struct {
      void *map;
      unsigned stride;
      unsigned divisor;
      unsigned step;
      void (*push)(struct nouveau_channel *, void *);
   } attr[16];
   unsigned attr_nr;
};

static void
emit_b32_1(struct nouveau_channel *chan, void *data)
{
   uint32_t *v = data;

   OUT_RING(chan, v[0]);
}

static void
emit_b32_2(struct nouveau_channel *chan, void *data)
{
   uint32_t *v = data;

   OUT_RING(chan, v[0]);
   OUT_RING(chan, v[1]);
}

static void
emit_b32_3(struct nouveau_channel *chan, void *data)
{
   uint32_t *v = data;

   OUT_RING(chan, v[0]);
   OUT_RING(chan, v[1]);
   OUT_RING(chan, v[2]);
}

static void
emit_b32_4(struct nouveau_channel *chan, void *data)
{
   uint32_t *v = data;

   OUT_RING(chan, v[0]);
   OUT_RING(chan, v[1]);
   OUT_RING(chan, v[2]);
   OUT_RING(chan, v[3]);
}

static void
emit_b16_1(struct nouveau_channel *chan, void *data)
{
   uint16_t *v = data;

   OUT_RING(chan, v[0]);
}

static void
emit_b16_3(struct nouveau_channel *chan, void *data)
{
   uint16_t *v = data;

   OUT_RING(chan, (v[1] << 16) | v[0]);
   OUT_RING(chan, v[2]);
}

static void
emit_b08_1(struct nouveau_channel *chan, void *data)
{
   uint8_t *v = data;

   OUT_RING(chan, v[0]);
}

static void
emit_b08_3(struct nouveau_channel *chan, void *data)
{
   uint8_t *v = data;

   OUT_RING(chan, (v[2] << 16) | (v[1] << 8) | v[0]);
}

static INLINE void
emit_vertex(struct push_context *ctx, unsigned n)
{
   struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
   struct nouveau_channel *chan = tesla->channel;
   int i;

   if (ctx->edgeflag_attr < 16) {
      float *edgeflag = ctx->attr[ctx->edgeflag_attr].map +
                        ctx->attr[ctx->edgeflag_attr].stride * n;

      if (*edgeflag != ctx->edgeflag) {
         BEGIN_RING(chan, tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
         OUT_RING  (chan, *edgeflag ? 1 : 0);
         ctx->edgeflag = *edgeflag;
      }
   }

   BEGIN_RING_NI(chan, tesla, NV50TCL_VERTEX_DATA, ctx->vtx_size);
   for (i = 0; i < ctx->attr_nr; i++)
      ctx->attr[i].push(chan, ctx->attr[i].map + ctx->attr[i].stride * n);
}

static void
emit_edgeflag(void *priv, boolean enabled)
{
   struct push_context *ctx = priv;
   struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
   struct nouveau_channel *chan = tesla->channel;

   BEGIN_RING(chan, tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
   OUT_RING  (chan, enabled ? 1 : 0);
}

static void
emit_elt08(void *priv, unsigned start, unsigned count)
{
   struct push_context *ctx = priv;
   uint8_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_elt16(void *priv, unsigned start, unsigned count)
{
   struct push_context *ctx = priv;
   uint16_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_elt32(void *priv, unsigned start, unsigned count)
{
   struct push_context *ctx = priv;
   uint32_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_verts(void *priv, unsigned start, unsigned count)
{
   while (count--)
      emit_vertex(priv, start++);
}

void
nv50_push_elements_instanced(struct pipe_context *pipe,
                             struct pipe_buffer *idxbuf, unsigned idxsize,
                             unsigned mode, unsigned start, unsigned count,
                             unsigned i_start, unsigned i_count)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   struct nouveau_channel *chan = tesla->channel;
   struct push_context ctx;
   const unsigned p_overhead = 4 + /* begin/end */
                               4; /* potential edgeflag enable/disable */
   const unsigned v_overhead = 1 + /* VERTEX_DATA packet header */
                               2; /* potential edgeflag modification */
   struct u_split_prim s;
   unsigned vtx_size;
   boolean nzi = FALSE;
   int i;

   ctx.nv50 = nv50;
   ctx.attr_nr = 0;
   ctx.idxbuf = NULL;
   ctx.vtx_size = 0;
   ctx.edgeflag = 0.5f;
   ctx.edgeflag_attr = nv50->vertprog->cfg.edgeflag_in;

   /* map vertex buffers, determine vertex size */
   for (i = 0; i < nv50->vtxelt->num_elements; i++) {
      struct pipe_vertex_element *ve = &nv50->vtxelt->pipe[i];
      struct pipe_vertex_buffer *vb = &nv50->vtxbuf[ve->vertex_buffer_index];
      struct nouveau_bo *bo = nouveau_bo(vb->buffer);
      unsigned size, nr_components, n;

      if (!(nv50->vbo_fifo & (1 << i)))
         continue;
      n = ctx.attr_nr++;

      if (nouveau_bo_map(bo, NOUVEAU_BO_RD)) {
         assert(bo->map);
         return;
      }
      ctx.attr[n].map = bo->map + vb->buffer_offset + ve->src_offset;
      nouveau_bo_unmap(bo);

      ctx.attr[n].stride = vb->stride;
      ctx.attr[n].divisor = ve->instance_divisor;
      if (ctx.attr[n].divisor) {
         ctx.attr[n].step = i_start % ve->instance_divisor;
         ctx.attr[n].map += i_start * vb->stride;
      }

      size = util_format_get_component_bits(ve->src_format,
                                            UTIL_FORMAT_COLORSPACE_RGB, 0);
      nr_components = util_format_get_nr_components(ve->src_format);
      switch (size) {
      case 8:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b08_1; break;
         case 2: ctx.attr[n].push = emit_b16_1; break;
         case 3: ctx.attr[n].push = emit_b08_3; break;
         case 4: ctx.attr[n].push = emit_b32_1; break;
         }
         ctx.vtx_size++;
         break;
      case 16:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b16_1; break;
         case 2: ctx.attr[n].push = emit_b32_1; break;
         case 3: ctx.attr[n].push = emit_b16_3; break;
         case 4: ctx.attr[n].push = emit_b32_2; break;
         }
         ctx.vtx_size += (nr_components + 1) >> 1;
         break;
      case 32:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b32_1; break;
         case 2: ctx.attr[n].push = emit_b32_2; break;
         case 3: ctx.attr[n].push = emit_b32_3; break;
         case 4: ctx.attr[n].push = emit_b32_4; break;
         }
         ctx.vtx_size += nr_components;
         break;
      default:
         assert(0);
         return;
      }
   }
   vtx_size = ctx.vtx_size + v_overhead;

   /* map index buffer, if present */
   if (idxbuf) {
      struct nouveau_bo *bo = nouveau_bo(idxbuf);

      if (nouveau_bo_map(bo, NOUVEAU_BO_RD)) {
         assert(bo->map);
         return;
      }
      ctx.idxbuf = bo->map;
      ctx.idxsize = idxsize;
      nouveau_bo_unmap(bo);
   }

   s.priv = &ctx;
   s.edge = emit_edgeflag;
   if (idxbuf) {
      if (idxsize == 1)
         s.emit = emit_elt08;
      else
      if (idxsize == 2)
         s.emit = emit_elt16;
      else
         s.emit = emit_elt32;
   } else
      s.emit = emit_verts;

   /* per-instance loop */
   BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
   OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
   OUT_RING  (chan, i_start);
   while (i_count--) {
      unsigned max_verts;
      boolean done;

      for (i = 0; i < ctx.attr_nr; i++) {
         if (!ctx.attr[i].divisor ||
              ctx.attr[i].divisor != ++ctx.attr[i].step)
            continue;
         ctx.attr[i].step = 0;
         ctx.attr[i].map += ctx.attr[i].stride;
      }

      u_split_prim_init(&s, mode, start, count);
      do {
         if (AVAIL_RING(chan) < p_overhead + (6 * vtx_size)) {
            FIRE_RING(chan);
            if (!nv50_state_validate(nv50, p_overhead + (6 * vtx_size))) {
               assert(0);
               return;
            }
         }

         max_verts  = AVAIL_RING(chan);
         max_verts -= p_overhead;
         max_verts /= vtx_size;

         BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
         OUT_RING  (chan, nv50_prim(s.mode) | (nzi ? (1 << 28) : 0));
         done = u_split_prim_next(&s, max_verts);
         BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
         OUT_RING  (chan, 0);
      } while (!done);

      nzi = TRUE;
   }
}
