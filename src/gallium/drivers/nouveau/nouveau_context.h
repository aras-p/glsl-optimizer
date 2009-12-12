#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

unsigned int
nouveau_is_texture_referenced(struct pipe_context *, struct pipe_texture *,
			      unsigned face, unsigned level);

unsigned int
nouveau_is_buffer_referenced(struct pipe_context *, struct pipe_buffer *);

#endif
