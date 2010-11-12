
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nvc0_3d.xml.h"

struct push_context {
   struct nouveau_channel *chan;

   void *idxbuf;
   int32_t idxbias;

   float edgeflag;
   int edgeflat_attr;

   uint32_t vertex_size;
   uint32_t packet_vertex_limit;

   struct translate *translate;
};

static void
emit_vertices_i08(struct push_context *ctx, unsigned start, unsigned count)
{
   uint8_t *elts = (uint8_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_size * push;

      BEGIN_RING_NI(ctx->chan, RING_3D(VERTEX_DATA), size);

      ctx->translate->run_elts8(ctx->translate, elts, push, 0, ctx->chan->cur);
      ctx->chan->cur += size;
      count -= push;
      elts += push;
   }
}

static void
emit_vertices_i16(struct push_context *ctx, unsigned start, unsigned count)
{
   uint16_t *elts = (uint16_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_size * push;

      BEGIN_RING_NI(ctx->chan, RING_3D(VERTEX_DATA), size);

      ctx->translate->run_elts16(ctx->translate, elts, push, 0, ctx->chan->cur);
      ctx->chan->cur += size;
      count -= push;
      elts += push;
   }
}

static void
emit_vertices_i32(struct push_context *ctx, unsigned start, unsigned count)
{
   uint32_t *elts = (uint32_t *)ctx->idxbuf + start;

   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_size * push;

      BEGIN_RING_NI(ctx->chan, RING_3D(VERTEX_DATA), size);

      ctx->translate->run_elts(ctx->translate, elts, push, 0, ctx->chan->cur);
      ctx->chan->cur += size;
      count -= push;
      elts += push;
   }
}

static void
emit_vertices_seq(struct push_context *ctx, unsigned start, unsigned count)
{
   while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
      unsigned size = ctx->vertex_size * push;

      BEGIN_RING_NI(ctx->chan, RING_3D(VERTEX_DATA), size);

      ctx->translate->run(ctx->translate, start, push, 0, ctx->chan->cur);
      ctx->chan->cur += size;
      count -= push;
      start += push;
   }
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
nvc0_push_vbo(struct nvc0_context *nvc0, const struct pipe_draw_info *info)
{
   struct push_context ctx;
   struct pipe_transfer *transfer = NULL;
   unsigned i, index_size;
   unsigned prim = nvc0_prim_gl(info->mode);
   unsigned inst = info->instance_count;

   ctx.chan = nvc0->screen->base.channel;
   ctx.translate = nvc0->vertex->translate;
   ctx.packet_vertex_limit = nvc0->vertex->vtx_per_packet_max;
   ctx.vertex_size = nvc0->vertex->vtx_size;

   for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      uint8_t *data;
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[i];
      struct nvc0_resource *res = nvc0_resource(vb->buffer);

      if (nouveau_bo_map(res->bo, NOUVEAU_BO_RD))
         return;
      data = (uint8_t *)res->bo->map + vb->buffer_offset;
      if (info->indexed)
         data += info->index_bias * vb->stride;

      ctx.translate->set_buffer(ctx.translate, i, data, vb->stride, ~0);
   }

   if (info->indexed) {
      ctx.idxbuf = pipe_buffer_map(&nvc0->pipe, nvc0->idxbuf.buffer,
                                   PIPE_TRANSFER_READ, &transfer);
      if (!ctx.idxbuf)
         return;
      index_size = nvc0->idxbuf.index_size;
   } else {
      ctx.idxbuf = NULL;
      index_size = 0;
   }

   while (inst--) {
      BEGIN_RING(ctx.chan, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (ctx.chan, prim);
      switch (index_size) {
      case 0:
         emit_vertices_seq(&ctx, info->start, info->count);
         break;
      case 1:
         emit_vertices_i08(&ctx, info->start, info->count);
         break;
      case 2:
         emit_vertices_i16(&ctx, info->start, info->count);
         break;
      case 4:
         emit_vertices_i32(&ctx, info->start, info->count);
         break;
      default:
         assert(0);
         break;
      }
      INLIN_RING(ctx.chan, RING_3D(VERTEX_END_GL), 0);

      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }

   if (info->indexed)
	   pipe_buffer_unmap(&nvc0->pipe, nvc0->idxbuf.buffer, transfer);

   for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      struct nvc0_resource *res = nvc0_resource(nvc0->vtxbuf[i].buffer);

      if (res->bo)
         nouveau_bo_unmap(res->bo);
   }
}
