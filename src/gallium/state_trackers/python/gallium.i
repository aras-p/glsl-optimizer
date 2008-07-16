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

%module gallium;

%{

#include <stdio.h>
#include <Python.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h" 
#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h" 
#include "util/p_tile.h" 
#include "tgsi/util/tgsi_text.h" 
#include "tgsi/util/tgsi_dump.h" 

#include "st_device.h"

%}

%include "carrays.i"
%array_class(unsigned char, ByteArray);
%array_class(int, IntArray);
%array_class(float, FloatArray);


%rename(Device) st_device;
%rename(Context) st_context;
%rename(Texture) pipe_texture;
%rename(Surface) pipe_surface;

%rename(Buffer) pipe_buffer;

%rename(BlendColor) pipe_blend_color;
%rename(Blend) pipe_blend_state;
%rename(Clip) pipe_clip_state;
%rename(ConstantBuffer) pipe_constant_buffer;
%rename(DepthStencilAlpha) pipe_depth_stencil_alpha_state;
%rename(FormatBlock) pipe_format_block;
%rename(Framebuffer) pipe_framebuffer_state;
%rename(PolyStipple) pipe_poly_stipple;
%rename(Rasterizer) pipe_rasterizer_state;
%rename(Sampler) pipe_sampler_state;
%rename(Scissor) pipe_scissor_state;
%rename(Shader) pipe_shader_state;
%rename(VertexBuffer) pipe_vertex_buffer;
%rename(VertexElement) pipe_vertex_element;
%rename(Viewport) pipe_viewport_state;

%nodefaultctor st_device;
%nodefaultctor st_context;
%nodefaultctor pipe_texture;
%nodefaultctor pipe_surface;
%nodefaultctor pipe_buffer;

%nodefaultdtor st_device;
%nodefaultdtor st_context;
%nodefaultdtor pipe_texture;
%nodefaultdtor pipe_surface;
%nodefaultdtor pipe_buffer;

%ignore pipe_texture::screen;

%ignore pipe_surface::winsys;
%immutable pipe_surface::texture;
%immutable pipe_surface::buffer;

%include "p_format.i";
%include "pipe/p_defines.h";
%include "pipe/p_state.h";
%include "pipe/p_shader_tokens.h";


struct st_device {
};

struct st_context {
};


%newobject st_device::texture_create;
%newobject st_device::context_create;
%newobject st_device::buffer_create;

%extend st_device {
   
   st_device(int hardware = 1) {
      return st_device_create(hardware ? TRUE : FALSE);
   }

   ~st_device() {
      st_device_destroy($self);
   }
   
   const char * get_name( void ) {
      return $self->screen->get_name($self->screen);
   }

   const char * get_vendor( void ) {
      return $self->screen->get_vendor($self->screen);
   }

   /**
    * Query an integer-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   int get_param( int param ) {
      return $self->screen->get_param($self->screen, param);
   }

   /**
    * Query a float-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   float get_paramf( int param ) {
      return $self->screen->get_paramf($self->screen, param);
   }

   /**
    * Check if the given pipe_format is supported as a texture or
    * drawing surface.
    * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
    */
   int is_format_supported( enum pipe_format format, unsigned type ) {
      return $self->screen->is_format_supported( $self->screen, format, type);
   }

   struct st_context *
   context_create(void) {
      return st_context_create($self);
   }

   struct pipe_texture * 
   texture_create(
         enum pipe_format format,
         unsigned width,
         unsigned height,
         unsigned depth = 1,
         unsigned last_level = 0,
         enum pipe_texture_target target = PIPE_TEXTURE_2D,
         unsigned usage = 0
      ) {
      struct pipe_texture templat;
      memset(&templat, 0, sizeof(templat));
      templat.format = format;
      pf_get_block(templat.format, &templat.block);
      templat.width[0] = width;
      templat.height[0] = height;
      templat.depth[0] = depth;
      templat.last_level = last_level;
      templat.target = target;
      templat.tex_usage = usage;
      return $self->screen->texture_create($self->screen, &templat);
   }
   
   struct pipe_buffer *
   buffer_create(unsigned size, unsigned alignment = 0, unsigned usage = 0) {
      return $self->screen->winsys->buffer_create($self->screen->winsys, alignment, usage, size);
   }

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
                            const struct pipe_constant_buffer *buf ) {
      $self->pipe->set_constant_buffer($self->pipe, shader, index, buf);
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
                          const struct pipe_vertex_buffer *buffer) {
      memcpy(&$self->vertex_buffers[index], buffer, sizeof(*buffer));
      $self->pipe->set_vertex_buffers($self->pipe, PIPE_MAX_ATTRIBS, $self->vertex_buffers);
   }

   void set_vertex_element(unsigned index,
                           const struct pipe_vertex_element *element) {
      memcpy(&$self->vertex_elements[index], element, sizeof(*element));
      $self->pipe->set_vertex_elements($self->pipe, PIPE_MAX_ATTRIBS, $self->vertex_elements);
   }

   /*
    * Draw functions
    */
   
   void draw_arrays(unsigned mode, unsigned start, unsigned count) {
      $self->pipe->draw_arrays($self->pipe, mode, start, count);
   }

   void draw_elements( struct pipe_buffer *indexBuffer,
                       unsigned indexSize,
                       unsigned mode, unsigned start, unsigned count) {
      $self->pipe->draw_elements($self->pipe, indexBuffer, indexSize, mode, start, count);
   }

   void draw_vertices(unsigned prim,
                      unsigned num_verts,
                      unsigned num_attribs,
                      const float *vertices) 
   {
      struct pipe_context *pipe = $self->pipe;
      struct pipe_winsys *winsys = pipe->winsys;
      struct pipe_buffer *vbuf;
      float *map;
      unsigned size;

      size = num_verts * num_attribs * 4 * sizeof(float);

      vbuf = winsys->buffer_create(winsys,
                                   32,
                                   PIPE_BUFFER_USAGE_VERTEX, 
                                   size);
      if(!vbuf)
         goto error1;
      
      map = winsys->buffer_map(winsys, vbuf, PIPE_BUFFER_USAGE_CPU_WRITE);
      if (!map)
         goto error2;
      memcpy(map, vertices, size);
      pipe->winsys->buffer_unmap(pipe->winsys, vbuf);
      
      util_draw_vertex_buffer(pipe, vbuf, prim, num_verts, num_attribs);
      
error2:
      pipe_buffer_reference(pipe->winsys, &vbuf, NULL);
error1:
      ;
   }
   
   void
   flush(void) {
      struct pipe_fence_handle *fence = NULL; 
      $self->pipe->flush($self->pipe, PIPE_FLUSH_RENDER_CACHE, &fence);
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


%newobject pipe_texture::get_surface;

%extend pipe_texture {
   
   ~pipe_texture() {
      struct pipe_texture *ptr = $self;
      pipe_texture_reference(&ptr, NULL);
   }
   
   /** Get a surface which is a "view" into a texture */
   struct pipe_surface *
   get_surface(unsigned face=0, unsigned level=0, unsigned zslice=0, unsigned usage=0 )
   {
      struct pipe_screen *screen = $self->screen;
      return screen->get_tex_surface(screen, $self, face, level, zslice, usage);
   }
   
};


%extend pipe_surface {
   
   ~pipe_surface() {
      struct pipe_surface *ptr = $self;
      pipe_surface_reference(&ptr, NULL);
   }
   
   // gets mapped to pipe_surface_map automatically
   void * map( unsigned flags );

   // gets mapped to pipe_surface_unmap automatically
   void unmap( void );

   void
   get_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, unsigned char *p, unsigned stride) {
      pipe_get_tile_raw($self, x, y, w, h, p, stride);
   }

   void
   put_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, const unsigned char *p, unsigned stride) {
      pipe_put_tile_raw($self, x, y, w, h, p, stride);
   }

   void
   get_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, float *p) {
      pipe_get_tile_rgba($self, x, y, w, h, p);
   }

   void
   put_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, const float *p) {
      pipe_put_tile_rgba($self, x, y, w, h, p);
   }

   void
   get_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, unsigned *z) {
      pipe_get_tile_z($self, x, y, w, h, z);
   }

   void
   put_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, const unsigned *z) {
      pipe_put_tile_z($self, x, y, w, h, z);
   }
   
};


%extend pipe_framebuffer_state {
   
   pipe_framebuffer_state(void) {
      return CALLOC_STRUCT(pipe_framebuffer_state);
   }
   
   ~pipe_framebuffer_state() {
      unsigned index;
      for(index = 0; index < PIPE_MAX_COLOR_BUFS; ++index)
         pipe_surface_reference(&$self->cbufs[index], NULL);
      pipe_surface_reference(&$self->zsbuf, NULL);
      FREE($self);
   }
   
   void
   set_cbuf(unsigned index, struct pipe_surface *surface) {
      pipe_surface_reference(&$self->cbufs[index], surface);
   }
   
   void
   set_zsbuf(struct pipe_surface *surface) {
      pipe_surface_reference(&$self->zsbuf, surface);
   }
   
};


%extend pipe_shader_state {
   
   pipe_shader_state(const char *text, unsigned num_tokens = 1024) {
      struct tgsi_token *tokens;
      struct pipe_shader_state *shader;
      
      tokens = MALLOC(num_tokens * sizeof(struct tgsi_token));
      if(!tokens)
         goto error1;
      
      if(tgsi_text_translate(text, tokens, num_tokens ) != TRUE)
         goto error2;
      
      shader = CALLOC_STRUCT(pipe_shader_state);
      if(!shader)
         goto error3;
      
      shader->tokens = tokens;
      
      return shader;
      
error3:
error2:
      FREE(tokens);
error1:      
      return NULL;
   }
   
   ~pipe_shader_state() {
      FREE((void*)$self->tokens);
      FREE($self);
   }

   void dump(unsigned flags = 0) {
      tgsi_dump($self->tokens, flags);
   }
}