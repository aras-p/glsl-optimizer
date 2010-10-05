 /**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * SWIG interface definion for Gallium types.
 *
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

%nodefaultctor st_context;
%nodefaultdtor st_context;

struct st_context {
};

%extend st_context {
   
   ~st_context() {
      st_context_destroy($self);
   }
   
   /*
    * State functions (create/bind/destroy state objects)
    */

   void set_blend( const struct pipe_blend_state *state ) {
      cso_set_blend($self->cso, state);
   }

   void set_fragment_sampler( unsigned index, const struct pipe_sampler_state *state ) {
      cso_single_sampler($self->cso, index, state);
      cso_single_sampler_done($self->cso);
   }

   void set_vertex_sampler( unsigned index, const struct pipe_sampler_state *state ) {
      cso_single_vertex_sampler($self->cso, index, state);
      cso_single_vertex_sampler_done($self->cso);
   }

   void set_rasterizer( const struct pipe_rasterizer_state *state ) {
      cso_set_rasterizer($self->cso, state);
   }

   void set_depth_stencil_alpha(const struct pipe_depth_stencil_alpha_state *state) {
      cso_set_depth_stencil_alpha($self->cso, state);
   }

   void set_fragment_shader( const struct pipe_shader_state *state ) {
      void *fs;
      
      if(!state) {
         cso_set_fragment_shader_handle($self->cso, NULL);
         return;
      }
      
      fs = $self->pipe->create_fs_state($self->pipe, state);
      if(!fs)
         return;
      
      if(cso_set_fragment_shader_handle($self->cso, fs) != PIPE_OK)
         return;

      cso_delete_fragment_shader($self->cso, $self->fs);
      $self->fs = fs;
   }

   void set_vertex_shader( const struct pipe_shader_state *state ) {
      void *vs;
      
      if(!state) {
         cso_set_vertex_shader_handle($self->cso, NULL);
         return;
      }
      
      vs = $self->pipe->create_vs_state($self->pipe, state);
      if(!vs)
         return;
      
      if(cso_set_vertex_shader_handle($self->cso, vs) != PIPE_OK)
         return;

      cso_delete_vertex_shader($self->cso, $self->vs);
      $self->vs = vs;
   }

   void set_geometry_shader( const struct pipe_shader_state *state ) {
      void *gs;

      if(!state) {
         cso_set_geometry_shader_handle($self->cso, NULL);
         return;
      }

      gs = $self->pipe->create_gs_state($self->pipe, state);
      if(!gs)
         return;

      if(cso_set_geometry_shader_handle($self->cso, gs) != PIPE_OK)
         return;

      cso_delete_geometry_shader($self->cso, $self->gs);
      $self->gs = gs;
   }

   struct pipe_sampler_view *
   create_sampler_view(struct pipe_resource *texture,
                       enum pipe_format format = PIPE_FORMAT_NONE,
                       unsigned first_level = 0,
                       unsigned last_level = ~0,
                       unsigned swizzle_r = 0,
                       unsigned swizzle_g = 1,
                       unsigned swizzle_b = 2,
                       unsigned swizzle_a = 3)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_sampler_view templat;

      memset(&templat, 0, sizeof templat);
      if (format == PIPE_FORMAT_NONE) {
         templat.format = texture->format;
      } else {
         templat.format = format;
      }
      templat.last_level = MIN2(last_level, texture->last_level);
      templat.first_level = first_level;
      templat.last_level = last_level;
      templat.swizzle_r = swizzle_r;
      templat.swizzle_g = swizzle_g;
      templat.swizzle_b = swizzle_b;
      templat.swizzle_a = swizzle_a;

      return pipe->create_sampler_view(pipe, texture, &templat);
   }

   void
   sampler_view_destroy(struct pipe_context *ctx,
                        struct pipe_sampler_view *view)
   {
      struct pipe_context *pipe = $self->pipe;

      pipe->sampler_view_destroy(pipe, view);
   }

   /*
    * Parameter-like state (or properties)
    */

   void set_blend_color(const struct pipe_blend_color *state ) {
      cso_set_blend_color($self->cso, state);
   }

   void set_stencil_ref(const struct pipe_stencil_ref *state ) {
      cso_set_stencil_ref($self->cso, state);
   }

   void set_clip(const struct pipe_clip_state *state ) {
      $self->pipe->set_clip_state($self->pipe, state);
   }

   void set_constant_buffer(unsigned shader, unsigned index,
                            struct pipe_resource *buffer ) 
   {
      $self->pipe->set_constant_buffer($self->pipe, shader, index, buffer);
   }

   void set_framebuffer(const struct pipe_framebuffer_state *state ) 
   {
      memcpy(&$self->framebuffer, state, sizeof *state);
      cso_set_framebuffer($self->cso, state);
   }

   void set_polygon_stipple(const struct pipe_poly_stipple *state ) {
      $self->pipe->set_polygon_stipple($self->pipe, state);
   }

   void set_scissor(const struct pipe_scissor_state *state ) {
      $self->pipe->set_scissor_state($self->pipe, state);
   }

   void set_viewport(const struct pipe_viewport_state *state) {
      cso_set_viewport($self->cso, state);
   }

   void set_fragment_sampler_view(unsigned index,
                                  struct pipe_sampler_view *view)
   {
      pipe_sampler_view_reference(&$self->fragment_sampler_views[index], view);

      $self->pipe->set_fragment_sampler_views($self->pipe,
                                              PIPE_MAX_SAMPLERS,
                                              $self->fragment_sampler_views);
   }

   void set_vertex_sampler_view(unsigned index,
                                struct pipe_sampler_view *view)
   {
      pipe_sampler_view_reference(&$self->vertex_sampler_views[index], view);

      $self->pipe->set_vertex_sampler_views($self->pipe,
                                            PIPE_MAX_VERTEX_SAMPLERS,
                                            $self->vertex_sampler_views);
   }

   void set_fragment_sampler_texture(unsigned index,
                                     struct pipe_resource *texture) {
      struct pipe_sampler_view templ;

      if(!texture)
         texture = $self->default_texture;
      pipe_sampler_view_reference(&$self->fragment_sampler_views[index], NULL);
      u_sampler_view_default_template(&templ,
                                      texture,
                                      texture->format);
      $self->fragment_sampler_views[index] = $self->pipe->create_sampler_view($self->pipe,
                                                                              texture,
                                                                              &templ);
      $self->pipe->set_fragment_sampler_views($self->pipe,
                                              PIPE_MAX_SAMPLERS,
                                              $self->fragment_sampler_views);
   }

   void set_vertex_sampler_texture(unsigned index,
                                   struct pipe_resource *texture) {
      struct pipe_sampler_view templ;

      if(!texture)
         texture = $self->default_texture;
      pipe_sampler_view_reference(&$self->vertex_sampler_views[index], NULL);
      u_sampler_view_default_template(&templ,
                                      texture,
                                      texture->format);
      $self->vertex_sampler_views[index] = $self->pipe->create_sampler_view($self->pipe,
                                                                            texture,
                                                                            &templ);
      
      $self->pipe->set_vertex_sampler_views($self->pipe,
                                            PIPE_MAX_VERTEX_SAMPLERS,
                                            $self->vertex_sampler_views);
   }

   void set_vertex_buffer(unsigned index,
                          unsigned stride, 
                          unsigned max_index,
                          unsigned buffer_offset,
                          struct pipe_resource *buffer)
   {
      unsigned i;
      struct pipe_vertex_buffer state;
      
      memset(&state, 0, sizeof(state));
      state.stride = stride;
      state.max_index = max_index;
      state.buffer_offset = buffer_offset;
      state.buffer = buffer;

      memcpy(&$self->vertex_buffers[index], &state, sizeof(state));
      
      for(i = 0; i < PIPE_MAX_ATTRIBS; ++i)
         if(self->vertex_buffers[i].buffer)
            $self->num_vertex_buffers = i + 1;
      
      $self->pipe->set_vertex_buffers($self->pipe, 
                                      $self->num_vertex_buffers, 
                                      $self->vertex_buffers);
   }

   void set_index_buffer(unsigned index_size,
                         unsigned offset,
                         struct pipe_resource *buffer)
   {
      struct pipe_index_buffer ib;

      memset(&ib, 0, sizeof(ib));
      ib.index_size = index_size;
      ib.offset = offset;
      ib.buffer = buffer;

      $self->pipe->set_index_buffer($self->pipe, &ib);
   }

   void set_vertex_element(unsigned index,
                           const struct pipe_vertex_element *element) 
   {
      memcpy(&$self->vertex_elements[index], element, sizeof(*element));
   }

   void set_vertex_elements(unsigned num) 
   {
      $self->num_vertex_elements = num;
      cso_set_vertex_elements($self->cso,
                              $self->num_vertex_elements, 
                              $self->vertex_elements);
   }

   /*
    * Draw functions
    */
   
   void draw_arrays(unsigned mode, unsigned start, unsigned count) {
      util_draw_arrays($self->pipe, mode, start, count);
   }

   void draw_vbo(const struct pipe_draw_info *info)
   {
      $self->pipe->draw_vbo($self->pipe, info);
   }

   void draw_vertices(unsigned prim,
                      unsigned num_verts,
                      unsigned num_attribs,
                      const float *vertices) 
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_screen *screen = pipe->screen;
      struct pipe_resource *vbuf;
      struct pipe_transfer *transfer;
      struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
      struct pipe_vertex_buffer vbuffer;
      float *map;
      unsigned size;
      unsigned i;

      size = num_verts * num_attribs * 4 * sizeof(float);

      vbuf = pipe_buffer_create(screen,
                                PIPE_BIND_VERTEX_BUFFER, 
                                size);
      if(!vbuf)
         goto error1;

      map = pipe_buffer_map(pipe, vbuf, PIPE_TRANSFER_WRITE, &transfer);
      if (!map)
         goto error2;
      memcpy(map, vertices, size);
      pipe_buffer_unmap(pipe, vbuf, transfer);

      cso_save_vertex_elements($self->cso);

      /* tell pipe about the vertex attributes */
      for (i = 0; i < num_attribs; i++) {
         velements[i].src_offset = i * 4 * sizeof(float);
         velements[i].instance_divisor = 0;
         velements[i].vertex_buffer_index = 0;
         velements[i].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      }
      cso_set_vertex_elements($self->cso, num_attribs, velements);

      /* tell pipe about the vertex buffer */
      memset(&vbuffer, 0, sizeof(vbuffer));
      vbuffer.buffer = vbuf;
      vbuffer.stride = num_attribs * 4 * sizeof(float);  /* vertex size */
      vbuffer.buffer_offset = 0;
      vbuffer.max_index = num_verts - 1;
      pipe->set_vertex_buffers(pipe, 1, &vbuffer);

      /* draw */
      util_draw_arrays(pipe, prim, 0, num_verts);

      cso_restore_vertex_elements($self->cso);

error2:
      pipe_resource_reference(&vbuf, NULL);
error1:
      ;
   }
   
   void
   clear(unsigned buffers, const float *rgba, double depth = 0.0f,
         unsigned stencil = 0)
   {
      $self->pipe->clear($self->pipe, buffers, rgba, depth, stencil);
   }

   void
   flush(unsigned flags = 0) {
      struct pipe_fence_handle *fence = NULL; 
      $self->pipe->flush($self->pipe, flags | PIPE_FLUSH_RENDER_CACHE, &fence);
      if(fence) {
         /* TODO: allow asynchronous operation */ 
         $self->pipe->screen->fence_finish( $self->pipe->screen, fence, 0 );
         $self->pipe->screen->fence_reference( $self->pipe->screen, &fence, NULL );
      }
   }

   /*
    * Surface functions
    */

   void resource_copy_region(struct pipe_resource *dst,
                             struct pipe_subresource subdst,
                             unsigned dstx, unsigned dsty, unsigned dstz,
                             struct pipe_resource *src,
                             struct pipe_subresource subsrc,
                             unsigned srcx, unsigned srcy, unsigned srcz,
                             unsigned width, unsigned height)
   {
      $self->pipe->resource_copy_region($self->pipe,
                                        dst, subdst, dstx, dsty, dstz,
                                        src, subsrc, srcx, srcy, srcz,
                                        width, height);
   }


   void clear_render_target(struct st_surface *dst,
                            float *rgba,
                            unsigned x, unsigned y,
                            unsigned width, unsigned height)
   {
      struct pipe_surface *_dst = NULL;

     _dst = st_pipe_surface(dst, PIPE_BIND_RENDER_TARGET);
      if(!_dst)
         SWIG_exception(SWIG_ValueError, "couldn't acquire destination surface for writing");

      $self->pipe->clear_render_target($self->pipe, _dst, rgba, x, y, width, height);

   fail:
      pipe_surface_reference(&_dst, NULL);
   }

   void clear_depth_stencil(struct st_surface *dst,
                            unsigned clear_flags,
                            double depth,
                            unsigned stencil,
                            unsigned x, unsigned y,
                            unsigned width, unsigned height)
   {
      struct pipe_surface *_dst = NULL;

     _dst = st_pipe_surface(dst, PIPE_BIND_DEPTH_STENCIL);
      if(!_dst)
         SWIG_exception(SWIG_ValueError, "couldn't acquire destination surface for writing");

      $self->pipe->clear_depth_stencil($self->pipe, _dst, clear_flags, depth, stencil,
                                       x, y, width, height);

   fail:
      pipe_surface_reference(&_dst, NULL);
   }

   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void
   surface_read_raw(struct st_surface *surface,
                    unsigned x, unsigned y, unsigned w, unsigned h,
                    char **STRING, int *LENGTH)
   {
      struct pipe_resource *texture = surface->texture;
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      unsigned stride;

      stride = util_format_get_stride(texture->format, w);
      *LENGTH = util_format_get_nblocksy(texture->format, h) * stride;
      *STRING = (char *) malloc(*LENGTH);
      if(!*STRING)
         return;

      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_READ,
                                   x, y, w, h);
      if(transfer) {
         pipe_get_tile_raw(pipe, transfer, 0, 0, w, h, *STRING, stride);
         pipe->transfer_destroy(pipe, transfer);
      }
   }

   %cstring_input_binary(const char *STRING, unsigned LENGTH);
   void
   surface_write_raw(struct st_surface *surface,
                     unsigned x, unsigned y, unsigned w, unsigned h,
                     const char *STRING, unsigned LENGTH, unsigned stride = 0)
   {
      struct pipe_resource *texture = surface->texture;
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;

      if(stride == 0)
         stride = util_format_get_stride(texture->format, w);

      if(LENGTH < util_format_get_nblocksy(texture->format, h) * stride)
         SWIG_exception(SWIG_ValueError, "offset must be smaller than buffer size");

      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_WRITE,
                                   x, y, w, h);
      if(!transfer)
         SWIG_exception(SWIG_MemoryError, "couldn't initiate transfer");

      pipe_put_tile_raw(pipe, transfer, 0, 0, w, h, STRING, stride);
      pipe->transfer_destroy(pipe, transfer);

   fail:
      return;
   }

   void
   surface_read_rgba(struct st_surface *surface,
                     unsigned x, unsigned y, unsigned w, unsigned h,
                     float *rgba)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_READ,
                                   x, y, w, h);
      if(transfer) {
         pipe_get_tile_rgba(pipe, transfer, 0, 0, w, h, rgba);
         pipe->transfer_destroy(pipe, transfer);
      }
   }

   void
   surface_write_rgba(struct st_surface *surface,
                      unsigned x, unsigned y, unsigned w, unsigned h,
                      const float *rgba)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_WRITE,
                                   x, y, w, h);
      if(transfer) {
         pipe_put_tile_rgba(pipe, transfer, 0, 0, w, h, rgba);
         pipe->transfer_destroy(pipe, transfer);
      }
   }

   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void
   surface_read_rgba8(struct st_surface *surface,
                      unsigned x, unsigned y, unsigned w, unsigned h,
                      char **STRING, int *LENGTH)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      float *rgba;
      unsigned char *rgba8;
      unsigned i, j, k;

      *LENGTH = 0;
      *STRING = NULL;

      if (!surface)
         return;

      *LENGTH = h*w*4;
      *STRING = (char *) malloc(*LENGTH);
      if(!*STRING)
         return;

      rgba = malloc(h*w*4*sizeof(float));
      if(!rgba)
         return;

      rgba8 = (unsigned char *) *STRING;

      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_READ,
                                   x, y, w, h);
      if(transfer) {
         pipe_get_tile_rgba(pipe, transfer, 0, 0, w, h, rgba);
         for(j = 0; j < h; ++j) {
            for(i = 0; i < w; ++i)
               for(k = 0; k <4; ++k)
                  rgba8[j*w*4 + i*4 + k] = float_to_ubyte(rgba[j*w*4 + i*4 + k]);
         }
         pipe->transfer_destroy(pipe, transfer);
      }

      free(rgba);
   }

   void
   surface_read_z(struct st_surface *surface,
                  unsigned x, unsigned y, unsigned w, unsigned h,
                  unsigned *z)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_READ,
                                   x, y, w, h);
      if(transfer) {
         pipe_get_tile_z(pipe, transfer, 0, 0, w, h, z);
         pipe->transfer_destroy(pipe, transfer);
      }
   }

   void
   surface_write_z(struct st_surface *surface,
                   unsigned x, unsigned y, unsigned w, unsigned h,
                   const unsigned *z)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_WRITE,
                                   x, y, w, h);
      if(transfer) {
         pipe_put_tile_z(pipe, transfer, 0, 0, w, h, z);
         pipe->transfer_destroy(pipe, transfer);
      }
   }

   void
   surface_sample_rgba(struct st_surface *surface,
                       float *rgba,
                       int norm = 0)
   {
      st_sample_surface($self->pipe, surface, rgba, norm != 0);
   }

   unsigned
   surface_compare_rgba(struct st_surface *surface,
                        unsigned x, unsigned y, unsigned w, unsigned h,
                        const float *rgba, float tol = 0.0)
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_transfer *transfer;
      float *rgba2;
      const float *p1;
      const float *p2;
      unsigned i, j, n;

      rgba2 = MALLOC(h*w*4*sizeof(float));
      if(!rgba2)
         return ~0;

      transfer = pipe_get_transfer(pipe,
                                   surface->texture,
                                   surface->face,
                                   surface->level,
                                   surface->zslice,
                                   PIPE_TRANSFER_READ,
                                   x, y, w, h);
      if(!transfer) {
         FREE(rgba2);
         return ~0;
      }

      pipe_get_tile_rgba(pipe, transfer, 0, 0, w, h, rgba2);
      pipe->transfer_destroy(pipe, transfer);

      p1 = rgba;
      p2 = rgba2;
      n = 0;
      for(i = h*w; i; --i) {
         unsigned differs = 0;
         for(j = 4; j; --j) {
            float delta = *p2++ - *p1++;
            if (delta < -tol || delta > tol)
                differs = 1;
         }
         n += differs;
      }

      FREE(rgba2);

      return n;
   }

   %cstring_input_binary(const char *STRING, unsigned LENGTH);
   void
   transfer_inline_write(struct pipe_resource *resource,
                         struct pipe_subresource *sr,
                         unsigned usage,
                         const struct pipe_box *box,
                         const char *STRING, unsigned LENGTH,
                         unsigned stride,
                         unsigned slice_stride)
   {
      struct pipe_context *pipe = $self->pipe;

      pipe->transfer_inline_write(pipe, resource, *sr, usage, box, STRING, stride, slice_stride);
   }

   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void buffer_read(struct pipe_resource *buffer,
                    char **STRING, int *LENGTH)
   {
      struct pipe_context *pipe = $self->pipe;

      assert(buffer->target == PIPE_BUFFER);

      *LENGTH = buffer->width0;
      *STRING = (char *) malloc(buffer->width0);
      if(!*STRING)
         return;

      pipe_buffer_read(pipe, buffer, 0, buffer->width0, *STRING);
   }

   void buffer_write(struct pipe_resource *buffer,
                     const char *STRING, unsigned LENGTH, unsigned offset = 0)
   {
      struct pipe_context *pipe = $self->pipe;

      assert(buffer->target == PIPE_BUFFER);

      if(offset > buffer->width0)
         SWIG_exception(SWIG_ValueError, "offset must be smaller than buffer size");

      if(offset + LENGTH > buffer->width0)
         SWIG_exception(SWIG_ValueError, "data length must fit inside the buffer");

      pipe_buffer_write(pipe, buffer, offset, LENGTH, STRING);

fail:
      return;
   }

};
