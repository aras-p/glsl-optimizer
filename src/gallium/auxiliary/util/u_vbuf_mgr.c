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

#include "util/u_vbuf_mgr.h"

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "translate/translate.h"
#include "translate/translate_cache.h"

/* Hardware vertex fetcher limitations can be described by this structure. */
struct u_vbuf_caps {
   /* Vertex format CAPs. */
   /* TRUE if hardware supports it. */
   unsigned format_fixed32:1;    /* PIPE_FORMAT_*32*_FIXED */
   unsigned format_float16:1;    /* PIPE_FORMAT_*16*_FLOAT */
   unsigned format_float64:1;    /* PIPE_FORMAT_*64*_FLOAT */
   unsigned format_norm32:1;     /* PIPE_FORMAT_*32*NORM */
   unsigned format_scaled32:1;   /* PIPE_FORMAT_*32*SCALED */

   /* Whether vertex fetches don't have to be dword-aligned. */
   /* TRUE if hardware supports it. */
   unsigned fetch_dword_unaligned:1;
};

struct u_vbuf_mgr_elements {
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
};

struct u_vbuf_mgr_priv {
   struct u_vbuf_mgr b;
   struct u_vbuf_caps caps;
   struct pipe_context *pipe;

   struct translate_cache *translate_cache;
   unsigned translate_vb_slot;

   struct u_vbuf_mgr_elements *ve;
   void *saved_ve, *fallback_ve;
   boolean ve_binding_lock;

   unsigned saved_buffer_offset[PIPE_MAX_ATTRIBS];

   boolean any_user_vbs;
   boolean incompatible_vb_layout;
};

static void u_vbuf_mgr_init_format_caps(struct u_vbuf_mgr_priv *mgr)
{
   struct pipe_screen *screen = mgr->pipe->screen;

   mgr->caps.format_fixed32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_FIXED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->caps.format_float16 =
      screen->is_format_supported(screen, PIPE_FORMAT_R16_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->caps.format_float64 =
      screen->is_format_supported(screen, PIPE_FORMAT_R64_FLOAT, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->caps.format_norm32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_UNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SNORM, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);

   mgr->caps.format_scaled32 =
      screen->is_format_supported(screen, PIPE_FORMAT_R32_USCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER) &&
      screen->is_format_supported(screen, PIPE_FORMAT_R32_SSCALED, PIPE_BUFFER,
                                  0, PIPE_BIND_VERTEX_BUFFER);
}

struct u_vbuf_mgr *
u_vbuf_mgr_create(struct pipe_context *pipe,
                  unsigned upload_buffer_size,
                  unsigned upload_buffer_alignment,
                  unsigned upload_buffer_bind,
                  enum u_fetch_alignment fetch_alignment)
{
   struct u_vbuf_mgr_priv *mgr = CALLOC_STRUCT(u_vbuf_mgr_priv);

   mgr->pipe = pipe;
   mgr->translate_cache = translate_cache_create();

   mgr->b.uploader = u_upload_create(pipe, upload_buffer_size,
                                     upload_buffer_alignment,
                                     upload_buffer_bind);

   mgr->caps.fetch_dword_unaligned =
         fetch_alignment == U_VERTEX_FETCH_BYTE_ALIGNED;

   u_vbuf_mgr_init_format_caps(mgr);

   return &mgr->b;
}

void u_vbuf_mgr_destroy(struct u_vbuf_mgr *mgrb)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;
   unsigned i;

   for (i = 0; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i], NULL);
   }

   translate_cache_destroy(mgr->translate_cache);
   u_upload_destroy(mgr->b.uploader);
   FREE(mgr);
}


static enum u_vbuf_return_flags
u_vbuf_translate_begin(struct u_vbuf_mgr_priv *mgr,
                       int min_index, int max_index)
{
   struct translate_key key;
   struct translate_element *te;
   unsigned tr_elem_index[PIPE_MAX_ATTRIBS];
   struct translate *tr;
   boolean vb_translated[PIPE_MAX_ATTRIBS] = {0};
   uint8_t *vb_map[PIPE_MAX_ATTRIBS] = {0}, *out_map;
   struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = {0};
   struct pipe_resource *out_buffer = NULL;
   unsigned i, num_verts, out_offset;
   struct pipe_vertex_element new_velems[PIPE_MAX_ATTRIBS];
   boolean upload_flushed = FALSE;

   memset(&key, 0, sizeof(key));
   memset(tr_elem_index, 0xff, sizeof(tr_elem_index));

   /* Initialize the translate key, i.e. the recipe how vertices should be
     * translated. */
   memset(&key, 0, sizeof key);
   for (i = 0; i < mgr->ve->count; i++) {
      struct pipe_vertex_buffer *vb =
            &mgr->b.vertex_buffer[mgr->ve->ve[i].vertex_buffer_index];
      enum pipe_format output_format = mgr->ve->native_format[i];
      unsigned output_format_size = mgr->ve->native_format_size[i];

      /* Check for support. */
      if (mgr->ve->ve[i].src_format == mgr->ve->native_format[i] &&
          (mgr->caps.fetch_dword_unaligned ||
           (vb->buffer_offset % 4 == 0 &&
            vb->stride % 4 == 0 &&
            mgr->ve->ve[i].src_offset % 4 == 0))) {
         continue;
      }

      /* Workaround for translate: output floats instead of halfs. */
      switch (output_format) {
      case PIPE_FORMAT_R16_FLOAT:
         output_format = PIPE_FORMAT_R32_FLOAT;
         output_format_size = 4;
         break;
      case PIPE_FORMAT_R16G16_FLOAT:
         output_format = PIPE_FORMAT_R32G32_FLOAT;
         output_format_size = 8;
         break;
      case PIPE_FORMAT_R16G16B16_FLOAT:
         output_format = PIPE_FORMAT_R32G32B32_FLOAT;
         output_format_size = 12;
         break;
      case PIPE_FORMAT_R16G16B16A16_FLOAT:
         output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         output_format_size = 16;
         break;
      default:;
      }

      /* Add this vertex element. */
      te = &key.element[key.nr_elements];
      /*te->type;
        te->instance_divisor;*/
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

         vb_map[i] = pipe_buffer_map(mgr->pipe, vb->buffer,
                                     PIPE_TRANSFER_READ, &vb_transfer[i]);

         tr->set_buffer(tr, i,
                        vb_map[i] + vb->buffer_offset + vb->stride * min_index,
                        vb->stride, ~0);
      }
   }

   /* Create and map the output buffer. */
   num_verts = max_index + 1 - min_index;

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
      if (vb_translated[i]) {
         pipe_buffer_unmap(mgr->pipe, vb_transfer[i]);
      }
   }

   /* Setup the new vertex buffer in the first free slot. */
   mgr->translate_vb_slot = ~0;
   for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
      if (!mgr->b.vertex_buffer[i].buffer) {
         mgr->translate_vb_slot = i;

         if (i >= mgr->b.nr_vertex_buffers) {
            mgr->b.nr_real_vertex_buffers = i+1;
         }
         break;
      }
   }

   if (mgr->translate_vb_slot != ~0) {
      /* Setup the new vertex buffer. */
      pipe_resource_reference(
            &mgr->b.real_vertex_buffer[mgr->translate_vb_slot], out_buffer);
      mgr->b.vertex_buffer[mgr->translate_vb_slot].buffer_offset = out_offset;
      mgr->b.vertex_buffer[mgr->translate_vb_slot].stride = key.output_stride;

      /* Setup new vertex elements. */
      for (i = 0; i < mgr->ve->count; i++) {
         if (tr_elem_index[i] < key.nr_elements) {
            te = &key.element[tr_elem_index[i]];
            new_velems[i].instance_divisor = mgr->ve->ve[i].instance_divisor;
            new_velems[i].src_format = te->output_format;
            new_velems[i].src_offset = te->output_offset;
            new_velems[i].vertex_buffer_index = mgr->translate_vb_slot;
         } else {
            memcpy(&new_velems[i], &mgr->ve->ve[i],
                   sizeof(struct pipe_vertex_element));
         }
      }

      mgr->fallback_ve =
            mgr->pipe->create_vertex_elements_state(mgr->pipe, mgr->ve->count,
                                                    new_velems);

      /* Preserve saved_ve. */
      mgr->ve_binding_lock = TRUE;
      mgr->pipe->bind_vertex_elements_state(mgr->pipe, mgr->fallback_ve);
      mgr->ve_binding_lock = FALSE;
   }

   pipe_resource_reference(&out_buffer, NULL);

   return upload_flushed ? U_VBUF_UPLOAD_FLUSHED : 0;
}

static void u_vbuf_translate_end(struct u_vbuf_mgr_priv *mgr)
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
   pipe_resource_reference(&mgr->b.real_vertex_buffer[mgr->translate_vb_slot],
                           NULL);
   mgr->b.nr_real_vertex_buffers = mgr->b.nr_vertex_buffers;
}

#define FORMAT_REPLACE(what, withwhat) \
    case PIPE_FORMAT_##what: format = PIPE_FORMAT_##withwhat; break

struct u_vbuf_mgr_elements *
u_vbuf_mgr_create_vertex_elements(struct u_vbuf_mgr *mgrb,
                                  unsigned count,
                                  const struct pipe_vertex_element *attribs,
                                  struct pipe_vertex_element *native_attribs)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;
   unsigned i;
   struct u_vbuf_mgr_elements *ve = CALLOC_STRUCT(u_vbuf_mgr_elements);

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
      if (!mgr->caps.format_fixed32) {
         switch (format) {
            FORMAT_REPLACE(R32_FIXED,           R32_FLOAT);
            FORMAT_REPLACE(R32G32_FIXED,        R32G32_FLOAT);
            FORMAT_REPLACE(R32G32B32_FIXED,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R32G32B32A32_FIXED,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_float16) {
         switch (format) {
            FORMAT_REPLACE(R16_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R16G16_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R16G16B16_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R16G16B16A16_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_float64) {
         switch (format) {
            FORMAT_REPLACE(R64_FLOAT,           R32_FLOAT);
            FORMAT_REPLACE(R64G64_FLOAT,        R32G32_FLOAT);
            FORMAT_REPLACE(R64G64B64_FLOAT,     R32G32B32_FLOAT);
            FORMAT_REPLACE(R64G64B64A64_FLOAT,  R32G32B32A32_FLOAT);
            default:;
         }
      }
      if (!mgr->caps.format_norm32) {
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
      if (!mgr->caps.format_scaled32) {
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

      ve->incompatible_layout =
            ve->incompatible_layout ||
            ve->ve[i].src_format != ve->native_format[i] ||
            (!mgr->caps.fetch_dword_unaligned && ve->ve[i].src_offset % 4 != 0);
   }

   /* Align the formats to the size of DWORD if needed. */
   if (!mgr->caps.fetch_dword_unaligned) {
      for (i = 0; i < count; i++) {
         ve->native_format_size[i] = align(ve->native_format_size[i], 4);
      }
   }

   return ve;
}

void u_vbuf_mgr_bind_vertex_elements(struct u_vbuf_mgr *mgrb,
                                     void *cso,
                                     struct u_vbuf_mgr_elements *ve)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;

   if (!cso) {
      return;
   }

   if (!mgr->ve_binding_lock) {
      mgr->saved_ve = cso;
      mgr->ve = ve;
   }
}

void u_vbuf_mgr_destroy_vertex_elements(struct u_vbuf_mgr *mgr,
                                        struct u_vbuf_mgr_elements *ve)
{
   FREE(ve);
}

void u_vbuf_mgr_set_vertex_buffers(struct u_vbuf_mgr *mgrb,
                                   unsigned count,
                                   const struct pipe_vertex_buffer *bufs)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;
   unsigned i;

   mgr->any_user_vbs = FALSE;
   mgr->incompatible_vb_layout = FALSE;

   if (!mgr->caps.fetch_dword_unaligned) {
      /* Check if the strides and offsets are aligned to the size of DWORD. */
      for (i = 0; i < count; i++) {
         if (bufs[i].buffer) {
            if (bufs[i].stride % 4 != 0 ||
                bufs[i].buffer_offset % 4 != 0) {
               mgr->incompatible_vb_layout = TRUE;
               break;
            }
         }
      }
   }

   for (i = 0; i < count; i++) {
      const struct pipe_vertex_buffer *vb = &bufs[i];

      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, vb->buffer);
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i], NULL);
      mgr->saved_buffer_offset[i] = vb->buffer_offset;

      if (!vb->buffer) {
         continue;
      }

      if (u_vbuf_resource(vb->buffer)->user_ptr) {
         mgr->any_user_vbs = TRUE;
         continue;
      }

      pipe_resource_reference(&mgr->b.real_vertex_buffer[i], vb->buffer);
   }

   for (; i < mgr->b.nr_real_vertex_buffers; i++) {
      pipe_resource_reference(&mgr->b.vertex_buffer[i].buffer, NULL);
      pipe_resource_reference(&mgr->b.real_vertex_buffer[i], NULL);
   }

   memcpy(mgr->b.vertex_buffer, bufs,
          sizeof(struct pipe_vertex_buffer) * count);

   mgr->b.nr_vertex_buffers = count;
   mgr->b.nr_real_vertex_buffers = count;
}

static enum u_vbuf_return_flags
u_vbuf_upload_buffers(struct u_vbuf_mgr_priv *mgr,
                      int min_index, int max_index,
                      unsigned instance_count)
{
   unsigned i, nr = mgr->ve->count;
   unsigned count = max_index + 1 - min_index;
   boolean uploaded[PIPE_MAX_ATTRIBS] = {0};
   enum u_vbuf_return_flags retval = 0;

   for (i = 0; i < nr; i++) {
      unsigned index = mgr->ve->ve[i].vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &mgr->b.vertex_buffer[index];

      if (vb->buffer &&
          u_vbuf_resource(vb->buffer)->user_ptr &&
          !uploaded[index]) {
         unsigned first, size;
         boolean flushed;
         unsigned instance_div = mgr->ve->ve[i].instance_divisor;

         if (instance_div) {
            first = 0;
            size = vb->stride *
                   ((instance_count + instance_div - 1) / instance_div);
         } else if (vb->stride) {
            first = vb->stride * min_index;
            size = vb->stride * count;

            /* Unusual case when stride is smaller than the format size.
             * XXX This won't work with interleaved arrays. */
            if (mgr->ve->native_format_size[i] > vb->stride)
               size += mgr->ve->native_format_size[i] - vb->stride;
         } else {
            first = 0;
            size = mgr->ve->native_format_size[i];
         }

         u_upload_data(mgr->b.uploader, first, size,
                       u_vbuf_resource(vb->buffer)->user_ptr + first,
                       &vb->buffer_offset,
                       &mgr->b.real_vertex_buffer[index],
                       &flushed);

         vb->buffer_offset -= first;

         uploaded[index] = TRUE;
         if (flushed)
            retval |= U_VBUF_UPLOAD_FLUSHED;
      } else {
         assert(mgr->b.real_vertex_buffer[index]);
      }
   }

   return retval;
}

static void u_vbuf_mgr_compute_max_index(struct u_vbuf_mgr_priv *mgr)
{
   unsigned i, nr = mgr->ve->count;

   mgr->b.max_index = ~0;

   for (i = 0; i < nr; i++) {
      struct pipe_vertex_buffer *vb =
            &mgr->b.vertex_buffer[mgr->ve->ve[i].vertex_buffer_index];
      int unused;
      unsigned max_index;

      if (!vb->buffer ||
          !vb->stride ||
          u_vbuf_resource(vb->buffer)->user_ptr) {
         continue;
      }

      /* How many bytes is unused after the last vertex.
       * width0 may be "count*stride - unused" and we have to compensate
       * for that when dividing by stride. */
      unused = vb->stride -
               (mgr->ve->ve[i].src_offset + mgr->ve->src_format_size[i]);

      /* If src_offset is greater than stride (which means it's a buffer
       * offset rather than a vertex offset)... */
      if (unused < 0) {
         unused = 0;
      }

      /* Compute the maximum index for this vertex element. */
      max_index =
         (vb->buffer->width0 - vb->buffer_offset + (unsigned)unused) /
         vb->stride - 1;

      mgr->b.max_index = MIN2(mgr->b.max_index, max_index);
   }
}

enum u_vbuf_return_flags
u_vbuf_mgr_draw_begin(struct u_vbuf_mgr *mgrb,
                      const struct pipe_draw_info *info)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;
   int min_index, max_index;
   enum u_vbuf_return_flags retval = 0;

   u_vbuf_mgr_compute_max_index(mgr);

   min_index = info->min_index - info->index_bias;
   if (info->max_index == ~0) {
      max_index = mgr->b.max_index;
   } else {
      max_index = MIN2(info->max_index - info->index_bias, mgr->b.max_index);
   }

   /* Translate vertices with non-native layouts or formats. */
   if (mgr->incompatible_vb_layout || mgr->ve->incompatible_layout) {
      retval |= u_vbuf_translate_begin(mgr, min_index, max_index);

      if (mgr->fallback_ve) {
         retval |= U_VBUF_BUFFERS_UPDATED;
      }
   }

   /* Upload user buffers. */
   if (mgr->any_user_vbs) {
      retval |= u_vbuf_upload_buffers(mgr, min_index, max_index,
                                      info->instance_count);
      retval |= U_VBUF_BUFFERS_UPDATED;
   }
   return retval;
}

void u_vbuf_mgr_draw_end(struct u_vbuf_mgr *mgrb)
{
   struct u_vbuf_mgr_priv *mgr = (struct u_vbuf_mgr_priv*)mgrb;
   unsigned i;

   /* buffer offsets were modified in u_vbuf_upload_buffers */
   if (mgr->any_user_vbs) {
      for (i = 0; i < mgr->b.nr_vertex_buffers; i++)
         mgr->b.vertex_buffer[i].buffer_offset = mgr->saved_buffer_offset[i];
   }

   if (mgr->fallback_ve) {
      u_vbuf_translate_end(mgr);
   }
}
