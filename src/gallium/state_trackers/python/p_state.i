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

%ignore winsys;
%ignore pipe_vertex_buffer::buffer;

%include "pipe/p_compiler.h";
%include "pipe/p_state.h";


%array_class(struct pipe_stencil_state, StencilArray);


%extend pipe_rt_blend_state
{
   struct pipe_rt_blend_state *
   __getitem__(int index) 
   {
      if(index < 0 || index >= PIPE_MAX_COLOR_BUFS)
         SWIG_exception(SWIG_ValueError, "index out of bounds");
      return $self + index;
   fail:
      return NULL;
   };
};


%extend pipe_blend_state
{
   pipe_blend_state(void)
   {
      return CALLOC_STRUCT(pipe_blend_state);
   }

   %cstring_input_binary(const char *STRING, unsigned LENGTH);
   pipe_blend_state(const char *STRING, unsigned LENGTH)
   {
      struct pipe_blend_state *state;
      state = CALLOC_STRUCT(pipe_blend_state);
      if (state) {
         LENGTH = MIN2(sizeof *state, LENGTH);
         memcpy(state, STRING, LENGTH);
      }
      return state;
   }

   %cstring_output_allocate_size(char **STRING, int *LENGTH, os_free(*$1));
   void __str__(char **STRING, int *LENGTH)
   {
      struct os_stream *stream;

      stream = os_str_stream_create(1);
      util_dump_blend_state(stream, $self);

      *STRING = os_str_stream_get_and_close(stream);
      *LENGTH = strlen(*STRING);
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
   set_cbuf(unsigned index, struct st_surface *surface) 
   {
      struct pipe_surface *_surface = NULL;

      if(index >= PIPE_MAX_COLOR_BUFS)
         SWIG_exception(SWIG_ValueError, "index out of bounds");
      
      if(surface) {
         /* XXX need a context here */
         _surface = st_pipe_surface(NULL, surface, PIPE_BIND_RENDER_TARGET);
         if(!_surface)
            SWIG_exception(SWIG_ValueError, "couldn't acquire surface for writing");
      }

      pipe_surface_reference(&$self->cbufs[index], _surface);
      
   fail:
      return;
   }
   
   void
   set_zsbuf(struct st_surface *surface) 
   {
      struct pipe_surface *_surface = NULL;

      if(surface) {
         /* XXX need a context here */
         _surface = st_pipe_surface(NULL, surface, PIPE_BIND_DEPTH_STENCIL);
         if(!_surface)
            SWIG_exception(SWIG_ValueError, "couldn't acquire surface for writing");
      }

      pipe_surface_reference(&$self->zsbuf, _surface);

   fail:
      return;
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
