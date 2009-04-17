#ifndef ST_INLINES_H
#define ST_INLINES_H

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_state.h"

#include "st_context.h"
#include "st_texture.h"
#include "st_public.h"

static INLINE struct pipe_transfer *
st_cond_flush_get_tex_transfer(struct st_context *st,
			       struct pipe_texture *pt,
			       unsigned int face,
			       unsigned int level,
			       unsigned int zslice,
			       enum pipe_transfer_usage usage,
			       unsigned int x, unsigned int y,
			       unsigned int w, unsigned int h)
{
   struct pipe_screen *screen = st->pipe->screen;

   st_teximage_flush_before_map(st, pt, face, level, usage);
   return screen->get_tex_transfer(screen, pt, face, level, zslice, usage,
				   x, y, w, h);
}

static INLINE struct pipe_transfer *
st_no_flush_get_tex_transfer(struct st_context *st,
			     struct pipe_texture *pt,
			     unsigned int face,
			     unsigned int level,
			     unsigned int zslice,
			     enum pipe_transfer_usage usage,
			     unsigned int x, unsigned int y,
			     unsigned int w, unsigned int h)
{
   struct pipe_screen *screen = st->pipe->screen;

   return screen->get_tex_transfer(screen, pt, face, level,
				   zslice, usage, x, y, w, h);
}

static INLINE void *
st_cond_flush_pipe_buffer_map(struct st_context *st,
			      struct pipe_buffer *buf,
			      unsigned int map_flags)
{
   struct pipe_context *pipe = st->pipe;
   unsigned int referenced = pipe->is_buffer_referenced(pipe, buf);

   if (referenced && ((referenced & PIPE_REFERENCED_FOR_WRITE) ||
		      (map_flags & PIPE_BUFFER_USAGE_CPU_WRITE)))
      st_flush(st, PIPE_FLUSH_RENDER_CACHE, NULL);

   return pipe_buffer_map(pipe->screen, buf, map_flags);
}

static INLINE void *
st_no_flush_pipe_buffer_map(struct st_context *st,
			    struct pipe_buffer *buf,
			    unsigned int map_flags)
{
   return pipe_buffer_map(st->pipe->screen, buf, map_flags);
}


static INLINE void
st_cond_flush_pipe_buffer_write(struct st_context *st,
				struct pipe_buffer *buf,
				unsigned int offset,
				unsigned int size,
				const void * data)
{
   struct pipe_context *pipe = st->pipe;

   if (pipe->is_buffer_referenced(pipe, buf))
      st_flush(st, PIPE_FLUSH_RENDER_CACHE, NULL);

   pipe_buffer_write(pipe->screen, buf, offset, size, data);
}

static INLINE void
st_no_flush_pipe_buffer_write(struct st_context *st,
			      struct pipe_buffer *buf,
			      unsigned int offset,
			      unsigned int size,
			      const void * data)
{
   pipe_buffer_write(st->pipe->screen, buf, offset, size, data);
}

static INLINE void
st_cond_flush_pipe_buffer_read(struct st_context *st,
			       struct pipe_buffer *buf,
			       unsigned int offset,
			       unsigned int size,
			       void * data)
{
   struct pipe_context *pipe = st->pipe;

   if (pipe->is_buffer_referenced(pipe, buf) & PIPE_REFERENCED_FOR_WRITE)
      st_flush(st, PIPE_FLUSH_RENDER_CACHE, NULL);

   pipe_buffer_read(pipe->screen, buf, offset, size, data);
}

static INLINE void
st_no_flush_pipe_buffer_read(struct st_context *st,
			     struct pipe_buffer *buf,
			     unsigned int offset,
			     unsigned int size,
			     void * data)
{
   pipe_buffer_read(st->pipe->screen, buf, offset, size, data);
}

#endif

