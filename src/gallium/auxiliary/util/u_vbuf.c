/**************************************************************************
 *
 * Copyright 2011 Marek Olšák <maraeo@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_vbuf.h"

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"

struct u_vbuf_elements {
   unsigned count;
   struct pipe_vertex_element ve[PIPE_MAX_ATTRIBS];

   unsigned src_format_size[PIPE_MAX_ATTRIBS];

   /* If (velem[i].src_format != native_format[i]), the vertex buffer
    * referenced by the vertex element cannot be used for rendering and
    * its vertex data must be translated to native_format[i]. */
   enum pipe_format native_format[PIPE_MAX_ATTRIBS];
   unsigned native_format_size[PIPE_MAX_ATTRIBS];

   /* This might mean two things:
    * - src_format != native_format, as discussed above.
    * - src_offset % 4 != 0 (if the caps don't allow such an offset). */
   boolean incompatible_layout;
   /* Per-element flags. */
   boolean incompatible_layout_elem[PIPE_MAX_ATTRIBS];
};

struct u_vbuf_priv {
   struct u_vbuf b;
   struct pipe_context *pipe;
   struct translate_cache *translate_cache;

   /* Vertex element state bound by the state tracker. */
   void *saved_ve;
   /* and its associated helper structure for this module. */
   struct u_vbuf_elements *ve;

   /* Vertex elements used for the translate fallback. */
   struct pipe_vertex_element fallback_velems[PIPE_MAX_ATTRIBS];
   /* If non-NULL, this is a vertex element state used for the translate
    * fallback and therefore used for rendering too. */
   void *fallback_ve;
   /* The vertex buffer slot index where translated vertices have been
    * stored in. */
   unsigned fallback_vb_slot;
   /* When binding the fallback vertex element state, we don't want to
    * change saved_ve and ve. This is set to TRUE in such cases. */
   boolean ve_binding_lock;

   /* Whether there is any user buffer. */
   boolean any_user_vbs;
   /* Whether there is a buffer with a non-native layout. */
   boolean incompatible_vb_layout;
   /* Per-buffer flags. */
   boolean incompatible_vb[PIPE_MAX_ATTRIBS];
};

static void u_vbuf_init_format_caps(struct u_vbuf_priv *mgr)
{
   struct pipe_screen *screen = mgr->pipe->screen;

   mgr->b.caps.format_fixed32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_FIXED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_float16 =
      screen->is_format_supported(screen, PIPE_FORMAT_R16_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_float64 =
      screen->is_format_supported(screen, PIPE_FORMAT_R64_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_norm32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_UNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->b.caps.format_scaled32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_USCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SSCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);
}

struct u_vbuf *
u_vbuf_create(struct pipe_context *pipe,
              unsigned upload_buffer_size,
              unsigned upload_buffer_alignment,
              unsigned upload_buffer_bind,
              enum u_fetch_alignment fetch_alignment)
{
   struct u_vbuf_priv *mgr = CALLOC_STRUCT(u_vbuf_priv);

   mgr->pipe = pipe;
   mgr->translate_cache = translate_cache_create();
   mgr->fallback_vb_slot = ~0;

   mgr->b.uploader = u_upload_create(pipe, upload_buffer_size,
                                     upload_buffer_alignment,
                                     upload_buffer_bind);

   mgr->b.caps.fetch_dword_unaligned =
         fetch_alignment == U_VERTEX_FETCH_BYTE_ALIGNED;

   u_vbuf_init_format_caps(mgr);

   return &mgr->b;
}

void u_vbuf_destroy(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;

   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
   }
   for (i = 0; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
   }

   translate_cache_destroy(mgr->translate_cache);
   u_upload_destroy(mgr->b.uploader);
   FREE(mgr);
}


static unsigned u_vbuf_get_free_real_vb_slot(struct u_vbuf_priv *mgr)
{
   unsigned i, nr = mgr->ve->count;
   boolean used_vb[PIPE_MAX_ATTRIBS] = {0};

   for (i = 0; i < nr; i++) {
      if (!mgr->ve->incompatible_layout_elem[i]) {
         unsigned index = mgr->ve->ve[i].vertex_buffer_index;

         if (!mgr->incompatible_vb[index]) {
            used_vb[index] = TRUE;
         }
      }
   }

   for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
      if (!used_vb[i]) {
         if (i >= mgr->b.nr_real_vertex_buffers) {
            mgr->b.nr_real_vertex_buffers = i+1;
         }
         return i;
      }
   }
   return ~0;
}

static void
u_vbuf_translate_begin(struct u_vbuf_priv *mgr,
                       int min_index, int max_index)
{
   struct translate_key key;
   struct translate_element *te;
   unsigned tr_elem_index[PIPE_MAX_ATTRIBS];
   struct translate *tr;
   boolean vb_translated[PIPE_MAX_ATTRIBS] = {0};
   uint8_t *out_map;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
   struct pipe_resource *out_buffer = NULL;
   unsigned i, out_offset, num_verts = max_index + 1 - min_index;
   boolean upload_flushed = FALSE;

   memset(&key, 0, sizeof(key));
   memset(tr_elem_index, 0xff, sizeof(tr_elem_index));

   /* Get a new vertex buffer slot. */
   mgr->fallback_vb_slot = u_vbuf_get_free_real_vb_slot(mgr);

   if (mgr->fallback_vb_slot == ~0) {
      return; /* XXX error, not enough attribs */
   }

   /* Initialize the description of how vertices should be translated. */
   for (i = 0; i < mgr->ve->count; i++) {
      enum pipe_format output_format = mgr->ve->native_format[i];
      unsigned output_format_size = mgr->ve->native_format_size[i];

      /* Check for support. */
      if (!mgr->ve->incompatible_layout_elem[i] &&
          !mgr->incompatible_vb[mgr->ve->ve[i].vertex_buffer_index]) {
         continue;
      }

      assert(translate_is_output_format_supported(output_format));

      /* Add this vertex element. */
      te = &key.element[key.nr_elements];
      te->type = TRANSLATE_ELEMENT_NORMAL;
      te->instance_divisor = 0;
      te->input_buffer = mgr->ve->ve[i].vertex_buffer_index;
      te->input_format = mgr->ve->ve[i].src_format;
      te->input_offset = mgr->ve->ve[i].src_offset;
      te->output_format = output_format;
      te->output_offset = key.output_stride;

      key.output_stride += output_format_size;
      vb_translated[mgr->ve->ve[i].vertex_buffer_index] = TRUE;
      tr_elem_index[i] = key.nr_elements;
      key.nr_elements++;
   }

   /* Get a translate object. */
   tr = translate_cache_find(mgr->translate_cache, &key);

   /* Map buffers we want to translate. */
   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      if (vb_translated[i]) {
         struct pipe_vertex_buffer *vb = &mgr->b.vertex_buffer[i];
         unsigned offset = vb->buffer_offset + vb->stride * min_index;
         unsigned size = vb->stride ? num_verts * vb->stride
                                    : vb->buffer->width0 - offset;
         uint8_t *map;

         if (u_vbuf_resource(vb->buffer)->user_ptr) {
            map = u_vbuf_resource(vb->buffer)->user_ptr + offset;
         } else {
            map = pipe_buffer_map_range(mgr->pipe, vb->buffer, offset, size,
                                        PIPE_TRANSFER_READ, &vb_transfer[i]);
         }

         tr->set_buffer(tr, i, map, vb->stride, ~0);
      }
   }

   /* Create and map the output buffer. */
   u_upload_alloc(mgr->b.uploader,
                  key.output_stride * min_index,
                  key.output_stride * num_verts,
                  &out_offset, &out_buffer, &upload_flushed,
                  (void**)&out_map);

   out_offset -= key.output_stride * min_index;

   /* Translate. */
   tr->run(tr, 0, num_verts, 0, out_map);

   /* Unmap all buffers. */
   for (i = 0; i < mgr->b.nr_vertex_buffers; i++) {
      if (vb_transfer[i]) {
         pipe_buffer_unmap(mgr->pipe, vb_transfer[i]);
      }
   }

   /* Setup the new vertex buffer. */
   mgr->b.real_vertex_buffer[mgr->fallback_vb_slot].buffer_offset = out_offset;
   mgr->b.real_vertex_buffer[mgr->fallback_vb_slot].stride = key.output_stride;

   /* Move the buffer reference. */
   pipe_resource_reference(
      &mgr->b.real_vertex_buffer[mgr->fallback_vb_slot].buffer, NULL);
   mgr->b.real_vertex_buffer[mgr->fallback_vb_slot].buffer = out_buffer;
   out_buffer = NULL;

   /* Setup new vertex elements. */
   for (i = 0; i < mgr->ve->count; i++) {
      if (tr_elem_index[i] < key.nr_elements) {
         te = &key.element[tr_elem_index[i]];
         mgr->fallback_velems[i].instance_divisor = mgr->ve->ve[i].instance_divisor;
         mgr->fallback_velems[i].src_format = te->output_format;
         mgr->fallback_velems[i].src_offset = te->output_offset;
         mgr->fallback_velems[i].vertex_buffer_index = mgr->fallback_vb_slot;
      } else {
         memcpy(&mgr->fallback_velems[i], &mgr->ve->ve[i],
                sizeof(struct pipe_vertex_element));
      }
   }


   mgr->fallback_ve =
      mgr->pipe->create_vertex_elements_state(mgr->pipe, mgr->ve->count,
                                              mgr->fallback_velems);

   /* Preserve saved_ve. */
   mgr->ve_binding_lock = TRUE;
   mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->fallback_ve);
   mgr->ve_binding_lock = FALSE;
}

static void u_vbuf_translate_end(struct u_vbuf_priv *mgr)
{
   if (mgr->fallback_ve == NULL) {
      return;
   }

   /* Restore vertex elements. */
   /* Note that saved_ve will be overwritten in bind_vertex_elements_state. */
   mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->saved_ve);
   mgr->pipe->delete_vertex_elements_state(mgr->pipe, mgr->fallback_ve);
   mgr->fallback_ve = NULL;

   /* Delete the now-unused VBO. */
   pipe_resource_reference(&mgr->b.real_vertex_buffer[mgr->fallback_vb_slot].buffer,
                           NULL);
   mgr->fallback_vb_slot = ~0;
   mgr->b.nr_real_vertex_buffers = mgr->b.nr_vertex_buffers;
}

#define FORMAT_REPLACE(what, withwhat) \
    case PIPE_FORMAT_##what: format = PIPE_FORMAT_##withwhat; break

struct u_vbuf_elements *
u_vbuf_create_vertex_elements(struct u_vbuf *mgrb,
                              unsigned count,
                              const struct pipe_vertex_element *attribs,
                              struct pipe_vertex_element *native_attribs)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;
   struct u_vbuf_elements *ve = CALLOC_STRUCT(u_vbuf_elements);

   ve->count = count;

   if (!count) {
      return ve;
   }

   memcpy(ve->ve, attribs, sizeof(struct pipe_vertex_element) * count);
   memcpy(native_attribs, attribs, sizeof(struct pipe_vertex_element) * count);

   /* Set the best native format in case the original format is not
    * supported. */
   for (i = 0; i < count; i++) {
      enum pipe_format format = ve->ve[i].src_format;

      ve->src_format_size[i] = util_format_get_blocksize(format);

      /* Choose a native format.
       * For now we don't care about the alignment, that's going to
       * be sorted out later. */
      if (!mgr->b.caps.format_fixed32) {
         switch (format) {
            FORMAT_REPLACE(R32_FIXED,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_FIXED,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_FIXED,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_FIXED,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_float16) {
         switch (format) {
            FORMAT_REPLACE(R16_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R16G16_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R16G16B16_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R16G16B16A16_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_float64) {
         switch (format) {
            FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_norm32) {
         switch (format) {
            FORMAT_REPLACE(R32_UNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_UNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_UNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_UNORM,  R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SNORM,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_SNORM,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SNORM,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SNORM,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->b.caps.format_scaled32) {
         switch (format) {
            FORMAT_REPLACE(R32_USCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_USCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_USCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_USCALED,R32G32B32A32_FLOAT);
            FORMAT_REPLACE(R32_SSCALED,         R32_FLOAT);
            FORMAT_REPLACE(R32G32_SSCALED,      R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_SSCALED,   R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_SSCALED,R32G32B32A32_FLOAT);
            default:;
         }
      }

      native_attribs[i].src_format = format;
      ve->native_format[i] = format;
      ve->native_format_size[i] =
            util_format_get_blocksize(ve->native_format[i]);

      ve->incompatible_layout_elem[i] =
            ve->ve[i].src_format != ve->native_format[i] ||
            (!mgr->b.caps.fetch_dword_unaligned && ve->ve[i].src_offset % 4 != 0);
      ve->incompatible_layout =
            ve->incompatible_layout ||
            ve->incompatible_layout_elem[i];
   }

   /* Align the formats to the size of DWORD if needed. */
   if (!mgr->b.caps.fetch_dword_unaligned) {
      for (i = 0; i < count; i++) {
         ve->native_format_size[i] = align(ve->native_format_size[i], 4);
      }
   }

   return ve;
}

void u_vbuf_bind_vertex_elements(struct u_vbuf *mgrb,
                                 void *cso,
                                 struct u_vbuf_elements *ve)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;

   if (!cso) {
      return;
   }

   if (!mgr->ve_binding_lock) {
      mgr->saved_ve = cso;
      mgr->ve = ve;
   }
}

void u_vbuf_destroy_vertex_elements(struct u_vbuf *mgr,
                                    struct u_vbuf_elements *ve)
{
   FREE(ve);
}

void u_vbuf_set_vertex_buffers(struct u_vbuf *mgrb,
                               unsigned count,
                               const struct pipe_vertex_buffer *bufs)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i;

   mgr->any_user_vbs = FALSE;
   mgr->incompatible_vb_layout = FALSE;
   memset(mgr->incompatible_vb, 0, sizeof(mgr->incompatible_vb));

   if (!mgr->b.caps.fetch_dword_unaligned) {
      /* Check if the strides and offsets are aligned to the size of DWORD. */
      for (i = 0; i < count; i++) {
         if (bufs[i].buffer) {
            if (bufs[i].stride % 4 != 0 ||
                bufs[i].buffer_offset % 4 != 0) {
               mgr->incompatible_vb_layout = TRUE;
               mgr->incompatible_vb[i] = TRUE;
            }
         }
      }
   }

   for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *vb = &bufs[i];

      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, vb->buffer);

      mgr->b.real_vertex_buffer[i].buffer_offset =
      mgr->b.vertex_buffer[i].buffer_offset = vb->buffer_offset;

      mgr->b.real_vertex_buffer[i].stride =
      mgr->b.vertex_buffer[i].stride = vb->stride;

      if (!vb->buffer ||
          mgr->incompatible_vb[i]) {
         pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
         continue;
      }

      if (u_vbuf_resource(vb->buffer)->user_ptr) {
         pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
         mgr->any_user_vbs = TRUE;
         continue;
      }

      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, vb->buffer);
   }

   for (i = count; i < mgr->b.nr_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
   }
   for (i = count; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i].buffer, NULL);
   }

   mgr->b.nr_vertex_buffers = count;
   mgr->b.nr_real_vertex_buffers = count;
}

void u_vbuf_set_index_buffer(struct u_vbuf *mgr,
                             const struct pipe_index_buffer *ib)
{
   if (ib && ib->buffer) {
      assert(ib->offset % ib->index_size == 0);
      pipe_resource_reference(&mgr->index_buffer.buffer, ib->buffer);
      mgr->index_buffer.offset = ib->offset;
      mgr->index_buffer.index_size = ib->index_size;
   } else {
      pipe_resource_reference(&mgr->index_buffer.buffer, NULL);
   }
}

static void
u_vbuf_upload_buffers(struct u_vbuf_priv *mgr,
                      int min_index, int max_index,
                      unsigned instance_count)
{
   unsigned i;
   unsigned count = max_index + 1 - min_index;
   unsigned nr_velems = mgr->ve->count;
   unsigned nr_vbufs = mgr->b.nr_vertex_buffers;
   struct pipe_vertex_element *velems =
         mgr->fallback_ve ? mgr->fallback_velems : mgr->ve->ve;
   unsigned start_offset[PIPE_MAX_ATTRIBS];
   unsigned end_offset[PIPE_MAX_ATTRIBS] = {0};

   /* Determine how much data needs to be uploaded. */
   for (i = 0; i < nr_velems; i++) {
      struct pipe_vertex_element *velem = &velems[i];
      unsigned index = velem->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &mgr->b.vertex_buffer[index];
      unsigned instance_div, first, size;

      /* Skip the buffer generated by translate. */
      if (index == mgr->fallback_vb_slot) {
         continue;
      }

      assert(vb->buffer);

      if (!u_vbuf_resource(vb->buffer)->user_ptr) {
         continue;
      }

      instance_div = velem->instance_divisor;
      first = vb->buffer_offset + velem->src_offset;

      if (!vb->stride) {
         /* Constant attrib. */
         size = mgr->ve->src_format_size[i];
      } else if (instance_div) {
         /* Per-instance attrib. */
         unsigned count = (instance_count + instance_div - 1) / instance_div;
         size = vb->stride * (count - 1) + mgr->ve->src_format_size[i];
      } else {
         /* Per-vertex attrib. */
         first += vb->stride * min_index;
         size = vb->stride * (count - 1) + mgr->ve->src_format_size[i];
      }

      /* Update offsets. */
      if (!end_offset[index]) {
         start_offset[index] = first;
         end_offset[index] = first + size;
      } else {
         if (first < start_offset[index])
            start_offset[index] = first;
         if (first + size > end_offset[index])
            end_offset[index] = first + size;
      }
   }

   /* Upload buffers. */
   for (i = 0; i < nr_vbufs; i++) {
      unsigned start, end = end_offset[i];
      boolean flushed;
      struct pipe_vertex_buffer *real_vb;
      uint8_t *ptr;

      if (!end) {
         continue;
      }

      start = start_offset[i];
      assert(start < end);

      real_vb = &mgr->b.real_vertex_buffer[i];
      ptr = u_vbuf_resource(mgr->b.vertex_buffer[i].buffer)->user_ptr;

      u_upload_data(mgr->b.uploader, start, end - start, ptr + start,
                    &real_vb->buffer_offset, &real_vb->buffer, &flushed);

      real_vb->buffer_offset -= start;
   }
}

unsigned u_vbuf_draw_max_vertex_count(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   unsigned i, nr = mgr->ve->count;
   struct pipe_vertex_element *velems =
         mgr->fallback_ve ? mgr->fallback_velems : mgr->ve->ve;
   unsigned result = ~0;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb =
            &mgr->b.real_vertex_buffer[velems[i].vertex_buffer_index];
      unsigned size, max_count, value;

      /* We're not interested in constant and per-instance attribs. */
      if (!vb->buffer ||
          !vb->stride ||
          velems[i].instance_divisor) {
         continue;
      }

      size = vb->buffer->width0;

      /* Subtract buffer_offset. */
      value = vb->buffer_offset;
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Subtract src_offset. */
      value = velems[i].src_offset;
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Subtract format_size. */
      value = mgr->ve->native_format_size[i];
      if (value >= size) {
         return 0;
      }
      size -= value;

      /* Compute the max count. */
      max_count = 1 + size / vb->stride;
      result = MIN2(result, max_count);
   }
   return result;
}

static boolean u_vbuf_need_minmax_index(struct u_vbuf_priv *mgr)
{
   unsigned i, nr = mgr->ve->count;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb;
      unsigned index;

      /* Per-instance attribs don't need min/max_index. */
      if (mgr->ve->ve[i].instance_divisor) {
         continue;
      }

      index = mgr->ve->ve[i].vertex_buffer_index;
      vb = &mgr->b.vertex_buffer[index];

      /* Constant attribs don't need min/max_index. */
      if (!vb->stride) {
         continue;
      }

      /* Per-vertex attribs need min/max_index. */
      if (u_vbuf_resource(vb->buffer)->user_ptr ||
          mgr->ve->incompatible_layout_elem[i] ||
          mgr->incompatible_vb[index]) {
         return TRUE;
      }
   }

   return FALSE;
}

static void u_vbuf_get_minmax_index(struct pipe_context *pipe,
                                    struct pipe_index_buffer *ib,
                                    const struct pipe_draw_info *info,
                                    int *out_min_index,
                                    int *out_max_index)
{
   struct pipe_transfer *transfer = NULL;
   const void *indices;
   unsigned i;
   unsigned restart_index = info->restart_index;

   if (u_vbuf_resource(ib->buffer)->user_ptr) {
      indices = u_vbuf_resource(ib->buffer)->user_ptr +
                ib->offset + info->start * ib->index_size;
   } else {
      indices = pipe_buffer_map_range(pipe, ib->buffer,
                                      ib->offset + info->start * ib->index_size,
                                      info->count * ib->index_size,
                                      PIPE_TRANSFER_READ, &transfer);
   }

   switch (ib->index_size) {
   case 4: {
      const unsigned *ui_indices = (const unsigned*)indices;
      unsigned max_ui = 0;
      unsigned min_ui = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] != restart_index) {
               if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
               if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
         }
      }
      *out_min_index = min_ui;
      *out_max_index = max_ui;
      break;
   }
   case 2: {
      const unsigned short *us_indices = (const unsigned short*)indices;
      unsigned max_us = 0;
      unsigned min_us = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] != restart_index) {
               if (us_indices[i] > max_us) max_us = us_indices[i];
               if (us_indices[i] < min_us) min_us = us_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (us_indices[i] > max_us) max_us = us_indices[i];
            if (us_indices[i] < min_us) min_us = us_indices[i];
         }
      }
      *out_min_index = min_us;
      *out_max_index = max_us;
      break;
   }
   case 1: {
      const unsigned char *ub_indices = (const unsigned char*)indices;
      unsigned max_ub = 0;
      unsigned min_ub = ~0U;
      if (info->primitive_restart) {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] != restart_index) {
               if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
               if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < info->count; i++) {
            if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
         }
      }
      *out_min_index = min_ub;
      *out_max_index = max_ub;
      break;
   }
   default:
      assert(0);
      *out_min_index = 0;
      *out_max_index = 0;
   }

   if (transfer) {
      pipe_buffer_unmap(pipe, transfer);
   }
}

enum u_vbuf_return_flags
u_vbuf_draw_begin(struct u_vbuf *mgrb,
                  const struct pipe_draw_info *info)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;
   int min_index, max_index;

   if (!mgr->incompatible_vb_layout &&
       !mgr->ve->incompatible_layout &&
       !mgr->any_user_vbs) {
      return 0;
   }

   if (info->indexed) {
      if (info->max_index != ~0) {
         min_index = info->min_index + info->index_bias;
         max_index = info->max_index + info->index_bias;
      } else if (u_vbuf_need_minmax_index(mgr)) {
         u_vbuf_get_minmax_index(mgr->pipe, &mgr->b.index_buffer, info,
                                 &min_index, &max_index);
         min_index += info->index_bias;
         max_index += info->index_bias;
      } else {
         min_index = 0;
         max_index = 0;
      }
   } else {
      min_index = info->start;
      max_index = info->start + info->count - 1;
   }

   /* Translate vertices with non-native layouts or formats. */
   if (mgr->incompatible_vb_layout || mgr->ve->incompatible_layout) {
      u_vbuf_translate_begin(mgr, min_index, max_index);
   }

   /* Upload user buffers. */
   if (mgr->any_user_vbs) {
      u_vbuf_upload_buffers(mgr, min_index, max_index, info->instance_count);
   }
   return U_VBUF_BUFFERS_UPDATED;
}

void u_vbuf_draw_end(struct u_vbuf *mgrb)
{
   struct u_vbuf_priv *mgr = (struct u_vbuf_priv*)mgrb;

   if (mgr->fallback_ve) {
      u_vbuf_translate_end(mgr);
   }
}
