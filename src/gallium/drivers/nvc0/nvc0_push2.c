
#if 0 /* not used, kept for now to compare with util/translate */

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nvc0_3d.xml.h"

struct push_context {
   struct nvc0_context *nvc0;

   uint vertex_size;

   void *idxbuf;
   uint idxsize;

   float edgeflag;
   int edgeflag_input;

   struct {
      void *map;
      void (*push)(struct nouveau_channel *, void *);
      uint32_t stride;
      uint32_t divisor;
      uint32_t step;
   } attr[32];
   int num_attrs;
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

static void
emit_b64_1(struct nouveau_channel *chan, void *data)
{
   double *v = data;

   OUT_RINGf(chan, v[0]);
}

static void
emit_b64_2(struct nouveau_channel *chan, void *data)
{
   double *v = data;

   OUT_RINGf(chan, v[0]);
   OUT_RINGf(chan, v[1]);
}

static void
emit_b64_3(struct nouveau_channel *chan, void *data)
{
   double *v = data;

   OUT_RINGf(chan, v[0]);
   OUT_RINGf(chan, v[1]);
   OUT_RINGf(chan, v[2]);
}

static void
emit_b64_4(struct nouveau_channel *chan, void *data)
{
   double *v = data;

   OUT_RINGf(chan, v[0]);
   OUT_RINGf(chan, v[1]);
   OUT_RINGf(chan, v[2]);
   OUT_RINGf(chan, v[3]);   
}

static INLINE void
emit_vertex(struct push_context *ctx, unsigned n)
{
   struct nouveau_channel *chan = ctx->nvc0->screen->base.channel;
   int i;

   if (ctx->edgeflag_input < 32) {
      /* TODO */
   }

   BEGIN_RING_NI(chan, RING_3D(VERTEX_DATA), ctx->vertex_size);
   for (i = 0; i < ctx->num_attrs; ++i)
      ctx->attr[i].push(chan,
                        (uint8_t *)ctx->attr[i].map + n * ctx->attr[i].stride);
}

static void
emit_edgeflag(struct push_context *ctx, boolean enabled)
{
   struct nouveau_channel *chan = ctx->nvc0->screen->base.channel;
   
   IMMED_RING(chan, RING_3D(EDGEFLAG_ENABLE), enabled);
}

static void
emit_elt08(struct push_context *ctx, unsigned start, unsigned count)
{
   uint8_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_elt16(struct push_context *ctx, unsigned start, unsigned count)
{
   uint16_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_elt32(struct push_context *ctx, unsigned start, unsigned count)
{
   uint32_t *idxbuf = ctx->idxbuf;

   while (count--)
      emit_vertex(ctx, idxbuf[start++]);
}

static void
emit_seq(struct push_context *ctx, unsigned start, unsigned count)
{
   while (count--)
      emit_vertex(ctx, start++);
}

#define NVC0_PRIM_GL_CASE(n) \
   case PIPE_PRIM_##n: return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_##n

static INLINE unsigned
nvc0_prim_gl(unsigned prim)
{
   switch (prim) {
   NVC0_PRIM_GL_CASE(POINTS);
   NVC0_PRIM_GL_CASE(LINES);
   NVC0_PRIM_GL_CASE(LINE_LOOP);
   NVC0_PRIM_GL_CASE(LINE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLES);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLE_FAN);
   NVC0_PRIM_GL_CASE(QUADS);
   NVC0_PRIM_GL_CASE(QUAD_STRIP);
   NVC0_PRIM_GL_CASE(POLYGON);
   NVC0_PRIM_GL_CASE(LINES_ADJACENCY);
   NVC0_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   /*
   NVC0_PRIM_GL_CASE(PATCHES); */
   default:
      return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
      break;
   }
}

void
nvc0_push_vbo2(struct nvc0_context *nvc0, const struct pipe_draw_info *info)
{
   struct push_context ctx;
   unsigned i, n;
   unsigned inst = info->instance_count;
   unsigned prim = nvc0_prim_gl(info->mode);

   ctx.nvc0 = nvc0;
   ctx.vertex_size = nvc0->vertex->vtx_size;
   ctx.idxbuf = NULL;
   ctx.num_attrs = 0;
   ctx.edgeflag = 0.5f;
   ctx.edgeflag_input = 32;

   for (i = 0; i < nvc0->vertex->num_elements; ++i) {
      struct pipe_vertex_element *ve = &nvc0->vertex->element[i].pipe;
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[ve->vertex_buffer_index];
      struct nouveau_bo *bo = nvc0_resource(vb->buffer)->bo;
      unsigned nr_components;

      if (!(nvc0->vbo_fifo & (1 << i)))
         continue;
      n = ctx.num_attrs++;

      if (nouveau_bo_map(bo, NOUVEAU_BO_RD))
         return;
      ctx.attr[n].map = (uint8_t *)bo->map + vb->buffer_offset + ve->src_offset;

      nouveau_bo_unmap(bo);

      ctx.attr[n].stride = vb->stride;
      ctx.attr[n].divisor = ve->instance_divisor;

      nr_components = util_format_get_nr_components(ve->src_format);
      switch (util_format_get_component_bits(ve->src_format,
                                             UTIL_FORMAT_COLORSPACE_RGB, 0)) {
      case 8:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b08_1; break;
         case 2: ctx.attr[n].push = emit_b16_1; break;
         case 3: ctx.attr[n].push = emit_b08_3; break;
         case 4: ctx.attr[n].push = emit_b32_1; break;
         }
         break;
      case 16:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b16_1; break;
         case 2: ctx.attr[n].push = emit_b32_1; break;
         case 3: ctx.attr[n].push = emit_b16_3; break;
         case 4: ctx.attr[n].push = emit_b32_2; break;
         }
         break;
      case 32:
         switch (nr_components) {
         case 1: ctx.attr[n].push = emit_b32_1; break;
         case 2: ctx.attr[n].push = emit_b32_2; break;
         case 3: ctx.attr[n].push = emit_b32_3; break;
         case 4: ctx.attr[n].push = emit_b32_4; break;
         }
         break;
      default:
         assert(0);
         break;
      }
   }

   if (info->indexed) {
      struct nvc0_resource *res = nvc0_resource(nvc0->idxbuf.buffer);
      if (!res || nouveau_bo_map(res->bo, NOUVEAU_BO_RD))
         return;
      ctx.idxbuf = (uint8_t *)res->bo->map + nvc0->idxbuf.offset + res->offset;
      nouveau_bo_unmap(res->bo);
      ctx.idxsize = nvc0->idxbuf.index_size;
   } else {
      ctx.idxsize = 0;
   }

   while (inst--) {
      BEGIN_RING(nvc0->screen->base.channel, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (nvc0->screen->base.channel, prim);
      switch (ctx.idxsize) {
      case 0:
         emit_seq(&ctx, info->start, info->count);
         break;
      case 1:
         emit_elt08(&ctx, info->start, info->count);
         break;
      case 2:
         emit_elt16(&ctx, info->start, info->count);
         break;
      case 4:
         emit_elt32(&ctx, info->start, info->count);
         break;
      }
      IMMED_RING(nvc0->screen->base.channel, RING_3D(VERTEX_END_GL), 0);

      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

#endif
