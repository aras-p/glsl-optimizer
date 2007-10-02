#ifndef __NOUVEAU_BUFFERS_H__
#define __NOUVEAU_BUFFERS_H__

#include <stdint.h>
#include "mtypes.h"
#include "utils.h"
#include "renderbuffer.h"

#include "nouveau_mem.h"

typedef struct nouveau_renderbuffer {
	struct gl_renderbuffer mesa;	/* must be first! */

	nouveau_mem *mem;
	void *map;

	int cpp;
	uint32_t offset;
	uint32_t pitch;
} nouveau_renderbuffer_t;

extern nouveau_renderbuffer_t *nouveau_renderbuffer_new(GLenum internalFormat);
extern void nouveau_window_moved(GLcontext *);
extern GLboolean nouveau_build_framebuffer(GLcontext *,
					   struct gl_framebuffer *);
extern nouveau_renderbuffer_t *nouveau_current_draw_buffer(GLcontext *);

extern void nouveauInitBufferFuncs(struct dd_function_table *);

#endif
