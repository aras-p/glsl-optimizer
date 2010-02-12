#ifndef R300_SCREEN_BUFFER_H
#define R300_SCREEN_BUFFER_H
#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "r300_screen.h"

#include "r300_winsys.h"
#include "r300_context.h"

#define R300_BUFFER_MAGIC 0xabcd1234

struct r300_buffer
{
    struct pipe_buffer base;

    uint32_t magic;

    struct r300_winsys_buffer *buf;

    void *user_buffer;
};

static INLINE struct r300_buffer *
r300_buffer(struct pipe_buffer *buffer)
{
    if (buffer) {
	assert(((struct r300_buffer *)buffer)->magic == R300_BUFFER_MAGIC);
	return (struct r300_buffer *)buffer;
    }
    return NULL;
}

static INLINE boolean 
r300_buffer_is_user_buffer(struct pipe_buffer *buffer)
{
    return r300_buffer(buffer)->user_buffer ? true : false;
}

static INLINE boolean r300_add_buffer(struct r300_winsys_screen *rws,
				      struct pipe_buffer *buffer,
				      int rd, int wr)
{
    struct r300_buffer *buf = r300_buffer(buffer);

    if (!buf->buf)
	return true;

    return rws->add_buffer(rws, buf->buf, rd, wr);
}


static INLINE boolean r300_add_texture(struct r300_winsys_screen *rws,
				       struct r300_texture *tex,
				       int rd, int wr)
{
    return rws->add_buffer(rws, tex->buffer, rd, wr);
}

void r300_screen_init_buffer_functions(struct r300_screen *r300screen);

static INLINE void r300_buffer_write_reloc(struct r300_winsys_screen *rws,
				      struct r300_buffer *buf,
				      uint32_t rd, uint32_t wd, uint32_t flags)
{
    if (!buf->buf)
	return;

    rws->write_cs_reloc(rws, buf->buf, rd, wd, flags);
}

static INLINE void r300_texture_write_reloc(struct r300_winsys_screen *rws,
					    struct r300_texture *texture,
					    uint32_t rd, uint32_t wd, uint32_t flags)
{
    rws->write_cs_reloc(rws, texture->buffer, rd, wd, flags);
}

int r300_upload_user_buffers(struct r300_context *r300);

int r300_upload_index_buffer(struct r300_context *r300,
			     struct pipe_buffer **index_buffer,
			     unsigned index_size,
			     unsigned start,
			     unsigned count);
#endif
