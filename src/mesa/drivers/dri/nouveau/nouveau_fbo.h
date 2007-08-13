#ifndef __NOUVEAU_BUFFERS_H__
#define __NOUVEAU_BUFFERS_H__

#include <stdint.h>
#include "mtypes.h"
#include "utils.h"
#include "renderbuffer.h"

#include "nouveau_mem.h"

typedef struct nouveau_renderbuffer_t {
	struct gl_renderbuffer mesa;	/* must be first! */
	__DRIdrawablePrivate *dPriv;

	nouveau_mem *mem;
	void *map;

	int cpp;
	uint32_t offset;
	uint32_t pitch;
} nouveau_renderbuffer;

extern nouveau_renderbuffer *nouveau_renderbuffer_new(GLenum internalFormat,
						      GLvoid *map,
						      GLuint offset,
						      GLuint pitch,
						      __DRIdrawablePrivate *);
extern void nouveau_window_moved(GLcontext *);
extern GLboolean nouveau_build_framebuffer(GLcontext *,
					   struct gl_framebuffer *);
extern nouveau_renderbuffer *nouveau_current_draw_buffer(GLcontext *);

extern void nouveauInitBufferFuncs(struct dd_function_table *);

#endif
