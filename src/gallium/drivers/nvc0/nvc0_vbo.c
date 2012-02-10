/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nvc0_3d.xml.h"

void
nvc0_vertex_state_delete(struct pipe_context *pipe,
                         void *hwcso)
{
   struct nvc0_vertex_stateobj *so = hwcso;

   if (so->translate)
      so->translate->release(so->translate);
   FREE(hwcso);
}

void *
nvc0_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements)
{
    struct nvc0_vertex_stateobj *so;
    struct translate_key transkey;
    unsigned i;

    so = MALLOC(sizeof(*so) +
                num_elements * sizeof(struct nvc0_vertex_element));
    if (!so)
        return NULL;
    so->num_elements = num_elements;
    so->instance_elts = 0;
    so->instance_bufs = 0;
    so->need_conversion = FALSE;

    transkey.nr_elements = 0;
    transkey.output_stride = 0;

    for (i = 0; i < num_elements; ++i) {
        const struct pipe_vertex_element *ve = &elements[i];
        const unsigned vbi = ve->vertex_buffer_index;
        enum pipe_format fmt = ve->src_format;

        so->element[i].pipe = elements[i];
        so->element[i].state = nvc0_format_table[fmt].vtx;

        if (!so->element[i].state) {
            switch (util_format_get_nr_components(fmt)) {
            case 1: fmt = PIPE_FORMAT_R32_FLOAT; break;
            case 2: fmt = PIPE_FORMAT_R32G32_FLOAT; break;
            case 3: fmt = PIPE_FORMAT_R32G32B32_FLOAT; break;
            case 4: fmt = PIPE_FORMAT_R32G32B32A32_FLOAT; break;
            default:
                assert(0);
                return NULL;
            }
            so->element[i].state = nvc0_format_table[fmt].vtx;
            so->need_conversion = TRUE;
        }
        so->element[i].state |= i;

        if (1) {
            unsigned j = transkey.nr_elements++;

            transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
            transkey.element[j].input_format = ve->src_format;
            transkey.element[j].input_buffer = vbi;
            transkey.element[j].input_offset = ve->src_offset;
            transkey.element[j].instance_divisor = ve->instance_divisor;

            transkey.element[j].output_format = fmt;
            transkey.element[j].output_offset = transkey.output_stride;
            transkey.output_stride += (util_format_get_stride(fmt, 1) + 3) & ~3;

            if (unlikely(ve->instance_divisor)) {
               so->instance_elts |= 1 << i;
               so->instance_bufs |= 1 << vbi;
            }
        }
    }

    so->translate = translate_create(&transkey);
    so->vtx_size = transkey.output_stride / 4;
    so->vtx_per_packet_max = NV04_PFIFO_MAX_PACKET_LEN / MAX2(so->vtx_size, 1);

    return so;
}

#define NVC0_3D_VERTEX_ATTRIB_INACTIVE                                       \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |                                 \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_32 | NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST

#define VTX_ATTR(a, c, t, s)                            \
   ((NVC0_3D_VTX_ATTR_DEFINE_TYPE_##t) |                \
    (NVC0_3D_VTX_ATTR_DEFINE_SIZE_##s) |                \
    ((a) << NVC0_3D_VTX_ATTR_DEFINE_ATTR__SHIFT) |      \
    ((c) << NVC0_3D_VTX_ATTR_DEFINE_COMP__SHIFT))

static void
nvc0_emit_vtxattr(struct nvc0_context *nvc0, struct pipe_vertex_buffer *vb,
                  struct pipe_vertex_element *ve, unsigned attr)
{
   const void *data;
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nv04_resource *res = nv04_resource(vb->buffer);
   float v[4];
   int i;
   const unsigned nc = util_format_get_nr_components(ve->src_format);

   data = nouveau_resource_map_offset(&nvc0->base, res, vb->buffer_offset +
                                      ve->src_offset, NOUVEAU_BO_RD);

   util_format_read_4f(ve->src_format, v, 0, data, 0, 0, 0, 1, 1);

   BEGIN_RING(chan, RING_3D(VTX_ATTR_DEFINE), nc + 1);
   OUT_RING  (chan, VTX_ATTR(attr, nc, FLOAT, 32));
   for (i = 0; i < nc; ++i)
      OUT_RINGf(chan, v[i]);
}

static INLINE void
nvc0_vbuf_range(struct nvc0_context *nvc0, int vbi,
                uint32_t *base, uint32_t *size)
{
   if (unlikely(nvc0->vertex->instance_bufs & (1 << vbi))) {
      /* TODO: use min and max instance divisor to get a proper range */
      *base = 0;
      *size = nvc0->vtxbuf[vbi].buffer->width0;
   } else {
      assert(nvc0->vbo_max_index != ~0);
      *base = nvc0->vbo_min_index * nvc0->vtxbuf[vbi].stride;
      *size = (nvc0->vbo_max_index -
               nvc0->vbo_min_index + 1) * nvc0->vtxbuf[vbi].stride;
   }
}

static void
nvc0_prevalidate_vbufs(struct nvc0_context *nvc0)
{
   struct pipe_vertex_buffer *vb;
   struct nv04_resource *buf;
   int i;
   uint32_t base, size;

   nvc0->vbo_fifo = nvc0->vbo_user = 0;

   nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_VERTEX);

   for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      vb = &nvc0->vtxbuf[i];
      if (!vb->stride)
         continue;
      buf = nv04_resource(vb->buffer);

      /* NOTE: user buffers with temporary storage count as mapped by GPU */
      if (!nouveau_resource_mapped_by_gpu(vb->buffer)) {
         if (nvc0->vbo_push_hint) {
            nvc0->vbo_fifo = ~0;
            continue;
         } else {
            if (buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY) {
               nvc0->vbo_user |= 1 << i;
               assert(vb->stride > vb->buffer_offset);
               nvc0_vbuf_range(nvc0, i, &base, &size);
               nouveau_user_buffer_upload(buf, base, size);
            } else {
               nouveau_buffer_migrate(&nvc0->base, buf, NOUVEAU_BO_GART);
            }
            nvc0->base.vbo_dirty = TRUE;
         }
      }
      nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_VERTEX, buf, NOUVEAU_BO_RD);
   }
}

static void
nvc0_update_user_vbufs(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   uint32_t base, offset, size;
   int i;
   uint32_t written = 0;

   for (i = 0; i < nvc0->vertex->num_elements; ++i) {
      struct pipe_vertex_element *ve = &nvc0->vertex->element[i].pipe;
      const int b = ve->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[b];
      struct nv04_resource *buf = nv04_resource(vb->buffer);

      if (!(nvc0->vbo_user & (1 << b)))
         continue;

      if (!vb->stride) {
         nvc0_emit_vtxattr(nvc0, vb, ve, i);
         continue;
      }
      nvc0_vbuf_range(nvc0, b, &base, &size);

      if (!(written & (1 << b))) {
         written |= 1 << b;
         nouveau_user_buffer_upload(buf, base, size);
      }
      offset = vb->buffer_offset + ve->src_offset;

      MARK_RING (chan, 6, 4);
      BEGIN_RING_1I(chan, RING_3D(VERTEX_ARRAY_SELECT), 5);
      OUT_RING  (chan, i);
      OUT_RESRCh(chan, buf, base + size - 1, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, buf, base + size - 1, NOUVEAU_BO_RD);
      OUT_RESRCh(chan, buf, offset, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, buf, offset, NOUVEAU_BO_RD);
   }
   nvc0->base.vbo_dirty = TRUE;
}

static INLINE void
nvc0_release_user_vbufs(struct nvc0_context *nvc0)
{
   uint32_t vbo_user = nvc0->vbo_user;

   while (vbo_user) {
      int i = ffs(vbo_user) - 1;
      vbo_user &= ~(1 << i);

      nouveau_buffer_release_gpu_storage(nv04_resource(nvc0->vtxbuf[i].buffer));
   }
}

void
nvc0_vertex_arrays_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   struct pipe_vertex_buffer *vb;
   struct nvc0_vertex_element *ve;
   unsigned i;

   if (unlikely(vertex->need_conversion) ||
       unlikely(nvc0->vertprog->vp.edgeflag < PIPE_MAX_ATTRIBS)) {
      nvc0->vbo_fifo = ~0;
      nvc0->vbo_user = 0;
   } else {
      nvc0_prevalidate_vbufs(nvc0);
   }

   BEGIN_RING(chan, RING_3D(VERTEX_ATTRIB_FORMAT(0)), vertex->num_elements);
   for (i = 0; i < vertex->num_elements; ++i) {
      ve = &vertex->element[i];
      vb = &nvc0->vtxbuf[ve->pipe.vertex_buffer_index];

      if (likely(vb->stride) || nvc0->vbo_fifo) {
         OUT_RING(chan, ve->state);
      } else {
         OUT_RING(chan, ve->state | NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST);
         nvc0->vbo_fifo &= ~(1 << i);
      }
   }

   for (i = 0; i < vertex->num_elements; ++i) {
      struct nv04_resource *res;
      unsigned size, offset;
      
      ve = &vertex->element[i];
      vb = &nvc0->vtxbuf[ve->pipe.vertex_buffer_index];

      if (unlikely(ve->pipe.instance_divisor)) {
         if (!(nvc0->state.instance_elts & (1 << i))) {
            IMMED_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 1);
         }
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_DIVISOR(i)), 1);
         OUT_RING  (chan, ve->pipe.instance_divisor);
      } else
      if (unlikely(nvc0->state.instance_elts & (1 << i))) {
         IMMED_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 0);
      }

      res = nv04_resource(vb->buffer);

      if (nvc0->vbo_fifo || unlikely(vb->stride == 0)) {
         if (!nvc0->vbo_fifo)
            nvc0_emit_vtxattr(nvc0, vb, &ve->pipe, i);
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
         OUT_RING  (chan, 0);
         continue;
      }

      size = vb->buffer->width0;
      offset = ve->pipe.src_offset + vb->buffer_offset;

      MARK_RING (chan, 8, 4);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, (1 << 12) | vb->stride);
      BEGIN_RING_1I(chan, RING_3D(VERTEX_ARRAY_SELECT), 5);
      OUT_RING  (chan, i);
      OUT_RESRCh(chan, res, size - 1, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, size - 1, NOUVEAU_BO_RD);
      OUT_RESRCh(chan, res, offset, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, offset, NOUVEAU_BO_RD);
   }
   for (; i < nvc0->state.num_vtxelts; ++i) {
      BEGIN_RING(chan, RING_3D(VERTEX_ATTRIB_FORMAT(i)), 1);
      OUT_RING  (chan, NVC0_3D_VERTEX_ATTRIB_INACTIVE);
      if (unlikely(nvc0->state.instance_elts & (1 << i)))
         IMMED_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 0);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, 0);
   }

   nvc0->state.num_vtxelts = vertex->num_elements;
   nvc0->state.instance_elts = vertex->instance_elts;
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

static void
nvc0_draw_vbo_flush_notify(struct nouveau_channel *chan)
{
   struct nvc0_screen *screen = chan->user_private;

   nouveau_fence_update(&screen->base, TRUE);

   nvc0_bufctx_emit_relocs(screen->cur_ctx);
}

static void
nvc0_draw_arrays(struct nvc0_context *nvc0,
                 unsigned mode, unsigned start, unsigned count,
                 unsigned instance_count)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   unsigned prim;

   if (nvc0->state.index_bias) {
      IMMED_RING(chan, RING_3D(VB_ELEMENT_BASE), 0);
      nvc0->state.index_bias = 0;
   }

   prim = nvc0_prim_gl(mode);

   while (instance_count--) {
      BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (chan, prim);
      BEGIN_RING(chan, RING_3D(VERTEX_BUFFER_FIRST), 2);
      OUT_RING  (chan, start);
      OUT_RING  (chan, count);
      IMMED_RING(chan, RING_3D(VERTEX_END_GL), 0);

      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

static void
nvc0_draw_elements_inline_u08(struct nouveau_channel *chan, uint8_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 3) {
      unsigned i;
      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U32), count & 3);
      for (i = 0; i < (count & 3); ++i)
         OUT_RING(chan, *map++);
      count &= ~3;
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 4) / 4;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U8), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan,
                  (map[3] << 24) | (map[2] << 16) | (map[1] << 8) | map[0]);
         map += 4;
      }
      count -= nr * 4;
   }
}

static void
nvc0_draw_elements_inline_u16(struct nouveau_channel *chan, uint16_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count &= ~1;
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32), 1);
      OUT_RING  (chan, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nvc0_draw_elements_inline_u32(struct nouveau_channel *chan, uint32_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   while (count) {
      const unsigned nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U32), nr);
      OUT_RINGp    (chan, map, nr);

      map += nr;
      count -= nr;
   }
}

static void
nvc0_draw_elements_inline_u32_short(struct nouveau_channel *chan, uint32_t *map,
                                    unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count--;
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32), 1);
      OUT_RING  (chan, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nvc0_draw_elements(struct nvc0_context *nvc0, boolean shorten,
                   unsigned mode, unsigned start, unsigned count,
                   unsigned instance_count, int32_t index_bias)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   void *data;
   unsigned prim;
   const unsigned index_size = nvc0->idxbuf.index_size;

   prim = nvc0_prim_gl(mode);

   if (index_bias != nvc0->state.index_bias) {
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_BASE), 1);
      OUT_RING  (chan, index_bias);
      nvc0->state.index_bias = index_bias;
   }

   if (nouveau_resource_mapped_by_gpu(nvc0->idxbuf.buffer)) {
      struct nv04_resource *res = nv04_resource(nvc0->idxbuf.buffer);
      unsigned offset = nvc0->idxbuf.offset;
      unsigned limit = nvc0->idxbuf.buffer->width0 - 1;

      while (instance_count--) {
         MARK_RING (chan, 11, 4);
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, mode);
         BEGIN_RING(chan, RING_3D(INDEX_ARRAY_START_HIGH), 7);
         OUT_RESRCh(chan, res, offset, NOUVEAU_BO_RD);
         OUT_RESRCl(chan, res, offset, NOUVEAU_BO_RD);
         OUT_RESRCh(chan, res, limit, NOUVEAU_BO_RD);
         OUT_RESRCl(chan, res, limit, NOUVEAU_BO_RD);
         OUT_RING  (chan, index_size >> 1);
         OUT_RING  (chan, start);
         OUT_RING  (chan, count);
         IMMED_RING(chan, RING_3D(VERTEX_END_GL), 0);

         nvc0_resource_fence(res, NOUVEAU_BO_RD);

         mode |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   } else {
      data = nouveau_resource_map_offset(&nvc0->base,
                                         nv04_resource(nvc0->idxbuf.buffer),
                                         nvc0->idxbuf.offset, NOUVEAU_BO_RD);
      if (!data)
         return;

      while (instance_count--) {
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, prim);
         switch (index_size) {
         case 1:
            nvc0_draw_elements_inline_u08(chan, data, start, count);
            break;
         case 2:
            nvc0_draw_elements_inline_u16(chan, data, start, count);
            break;
         case 4:
            if (shorten)
               nvc0_draw_elements_inline_u32_short(chan, data, start, count);
            else
               nvc0_draw_elements_inline_u32(chan, data, start, count);
            break;
         default:
            assert(0);
            return;
         }
         IMMED_RING(chan, RING_3D(VERTEX_END_GL), 0);

         prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   }
}

static void
nvc0_draw_stream_output(struct nvc0_context *nvc0,
                        const struct pipe_draw_info *info)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_so_target *so = nvc0_so_target(info->count_from_stream_output);
   struct nv04_resource *res = nv04_resource(so->pipe.buffer);
   unsigned mode = nvc0_prim_gl(info->mode);
   unsigned num_instances = info->instance_count;

   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
      IMMED_RING(chan, RING_3D(SERIALIZE), 0);
      nvc0_query_fifo_wait(chan, so->pq);
      IMMED_RING(chan, RING_3D(VERTEX_ARRAY_FLUSH), 0);
   }

   while (num_instances--) {
      BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (chan, mode);
      BEGIN_RING(chan, RING_3D(DRAW_TFB_BASE), 1);
      OUT_RING  (chan, 0);
      BEGIN_RING(chan, RING_3D(DRAW_TFB_STRIDE), 1);
      OUT_RING  (chan, so->stride);
      BEGIN_RING(chan, RING_3D(DRAW_TFB_BYTES), 1);
      nvc0_query_pushbuf_submit(chan, so->pq, 0x4);
      IMMED_RING(chan, RING_3D(VERTEX_END_GL), 0);

      mode |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

void
nvc0_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   /* For picking only a few vertices from a large user buffer, push is better,
    * if index count is larger and we expect repeated vertices, suggest upload.
    */
   nvc0->vbo_push_hint = /* the 64 is heuristic */
      !(info->indexed &&
        ((info->max_index - info->min_index + 64) < info->count));

   nvc0->vbo_min_index = info->min_index;
   nvc0->vbo_max_index = info->max_index;

   if (nvc0->vbo_push_hint != !!nvc0->vbo_fifo)
      nvc0->dirty |= NVC0_NEW_ARRAYS;

   if (nvc0->vbo_user && !(nvc0->dirty & (NVC0_NEW_VERTEX | NVC0_NEW_ARRAYS)))
      nvc0_update_user_vbufs(nvc0);

   /* 8 as minimum to avoid immediate double validation of new buffers */
   nvc0_state_validate(nvc0, ~0, 8);

   chan->flush_notify = nvc0_draw_vbo_flush_notify;

   if (nvc0->vbo_fifo) {
      nvc0_push_vbo(nvc0, info);
      chan->flush_notify = nvc0_default_flush_notify;
      return;
   }

   if (nvc0->state.instance_base != info->start_instance) {
      nvc0->state.instance_base = info->start_instance;
      /* NOTE: this does not affect the shader input, should it ? */
      BEGIN_RING(chan, RING_3D(VB_INSTANCE_BASE), 1);
      OUT_RING  (chan, info->start_instance);
   }

   if (nvc0->base.vbo_dirty) {
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FLUSH), 1);
      OUT_RING  (chan, 0);
      nvc0->base.vbo_dirty = FALSE;
   }

   if (unlikely(info->count_from_stream_output)) {
      nvc0_draw_stream_output(nvc0, info);
   } else
   if (!info->indexed) {
      nvc0_draw_arrays(nvc0,
                       info->mode, info->start, info->count,
                       info->instance_count);
   } else {
      boolean shorten = info->max_index <= 65535;

      assert(nvc0->idxbuf.buffer);

      if (info->primitive_restart != nvc0->state.prim_restart) {
         if (info->primitive_restart) {
            BEGIN_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 2);
            OUT_RING  (chan, 1);
            OUT_RING  (chan, info->restart_index);

            if (info->restart_index > 65535)
               shorten = FALSE;
         } else {
            IMMED_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 0);
         }
         nvc0->state.prim_restart = info->primitive_restart;
      } else
      if (info->primitive_restart) {
         BEGIN_RING(chan, RING_3D(PRIM_RESTART_INDEX), 1);
         OUT_RING  (chan, info->restart_index);

         if (info->restart_index > 65535)
            shorten = FALSE;
      }

      nvc0_draw_elements(nvc0, shorten,
                         info->mode, info->start, info->count,
                         info->instance_count, info->index_bias);
   }
   chan->flush_notify = nvc0_default_flush_notify;

   nvc0_release_user_vbufs(nvc0);
}
