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
   
   void set_sampler( unsigned index, const struct pipe_sampler_state *state ) {
      cso_single_sampler($self->cso, index, state);
      cso_single_sampler_done($self->cso);
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

   /*
    * Parameter-like state (or properties)
    */
   
   void set_blend_color(const struct pipe_blend_color *state ) {
      cso_set_blend_color($self->cso, state);
   }

   void set_clip(const struct pipe_clip_state *state ) {
      $self->pipe->set_clip_state($self->pipe, state);
   }

   void set_constant_buffer(unsigned shader, unsigned index,
                            struct st_buffer *buffer ) 
   {
      struct pipe_constant_buffer state;
      memset(&state, 0, sizeof(state));
      state.buffer = buffer ? buffer->buffer : NULL;
      state.size = buffer->buffer->size;
      $self->pipe->set_constant_buffer($self->pipe, shader, index, &state);
   }

   void set_framebuffer(const struct pipe_framebuffer_state *state ) {
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

   void set_sampler_texture(unsigned index,
                            struct pipe_texture *texture) {
      if(!texture)
         texture = $self->default_texture;
      pipe_texture_reference(&$self->sampler_textures[index], texture);
      $self->pipe->set_sampler_textures($self->pipe, 
                                        PIPE_MAX_SAMPLERS,
                                        $self->sampler_textures);
   }

   void set_vertex_buffer(unsigned index,
                          unsigned pitch, 
                          unsigned max_index,
                          unsigned buffer_offset,
                          struct st_buffer *buffer)
   {
      unsigned i;
      struct pipe_vertex_buffer state;
      
      memset(&state, 0, sizeof(state));
      state.pitch = pitch;
      state.max_index = max_index;
      state.buffer_offset = buffer_offset;
      state.buffer = buffer ? buffer->buffer : NULL;

      memcpy(&$self->vertex_buffers[index], &state, sizeof(state));
      
      for(i = 0; i < PIPE_MAX_ATTRIBS; ++i)
         if(self->vertex_buffers[i].buffer)
            $self->num_vertex_buffers = i + 1;
      
      $self->pipe->set_vertex_buffers($self->pipe, 
                                      $self->num_vertex_buffers, 
                                      $self->vertex_buffers);
   }

   void set_vertex_element(unsigned index,
                           const struct pipe_vertex_element *element) 
   {
      memcpy(&$self->vertex_elements[index], element, sizeof(*element));
   }

   void set_vertex_elements(unsigned num) 
   {
      $self->num_vertex_elements = num;
      $self->pipe->set_vertex_elements($self->pipe, 
                                       $self->num_vertex_elements, 
                                       $self->vertex_elements);
   }

   /*
    * Draw functions
    */
   
   void draw_arrays(unsigned mode, unsigned start, unsigned count) {
      $self->pipe->draw_arrays($self->pipe, mode, start, count);
   }

   void draw_elements( struct st_buffer *indexBuffer,
                       unsigned indexSize,
                       unsigned mode, unsigned start, unsigned count) 
   {
      $self->pipe->draw_elements($self->pipe, 
                                 indexBuffer->buffer, 
                                 indexSize, 
                                 mode, start, count);
   }

   void draw_range_elements( struct st_buffer *indexBuffer,
                             unsigned indexSize, unsigned minIndex, unsigned maxIndex,
                             unsigned mode, unsigned start, unsigned count)
   {
      $self->pipe->draw_range_elements($self->pipe, 
                                       indexBuffer->buffer, 
                                       indexSize, minIndex, maxIndex,
                                       mode, start, count);
   }

   void draw_vertices(unsigned prim,
                      unsigned num_verts,
                      unsigned num_attribs,
                      const float *vertices) 
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_screen *screen = pipe->screen;
      struct pipe_buffer *vbuf;
      float *map;
      unsigned size;

      size = num_verts * num_attribs * 4 * sizeof(float);

      vbuf = pipe_buffer_create(screen,
                                32,
                                PIPE_BUFFER_USAGE_VERTEX, 
                                size);
      if(!vbuf)
         goto error1;
      
      map = pipe_buffer_map(screen, vbuf, PIPE_BUFFER_USAGE_CPU_WRITE);
      if (!map)
         goto error2;
      memcpy(map, vertices, size);
      pipe_buffer_unmap(screen, vbuf);
      
      util_draw_vertex_buffer(pipe, vbuf, prim, num_verts, num_attribs);
      
error2:
      pipe_buffer_reference(screen, &vbuf, NULL);
error1:
      ;
   }
   
   void
   flush(unsigned flags = 0) {
      struct pipe_fence_handle *fence = NULL; 
      $self->pipe->flush($self->pipe, flags | PIPE_FLUSH_RENDER_CACHE, &fence);
      /* TODO: allow asynchronous operation */ 
      $self->pipe->winsys->fence_finish( $self->pipe->winsys, fence, 0 );
      $self->pipe->winsys->fence_reference( $self->pipe->winsys, &fence, NULL );
   }

   /*
    * Surface functions
    */
   
   void surface_copy(int do_flip,
                     struct pipe_surface *dest,
                     unsigned destx, unsigned desty,
                     struct pipe_surface *src,
                     unsigned srcx, unsigned srcy,
                     unsigned width, unsigned height) {
      $self->pipe->surface_copy($self->pipe, do_flip, dest, destx, desty, src, srcx, srcy, width, height);
   }

   void surface_fill(struct pipe_surface *dst,
                     unsigned x, unsigned y,
                     unsigned width, unsigned height,
                     unsigned value) {
      $self->pipe->surface_fill($self->pipe, dst, x, y, width, height, value);
   }

   void surface_clear(struct pipe_surface *surface, unsigned value = 0) {
      $self->pipe->clear($self->pipe, surface, value);
   }

};
