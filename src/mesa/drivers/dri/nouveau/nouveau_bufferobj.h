#ifndef __NOUVEAU_BUFFEROBJ_H__
#define __NOUVEAU_BUFFEROBJ_H__

#include "mtypes.h"
#include "nouveau_buffers.h"

typedef struct nouveau_buffer_object_t {
	/* Base class, must be first */
	struct gl_buffer_object mesa;

	/* Memory used for GPU access to the buffer*/
	nouveau_mem *		gpu_mem;
	/* Buffer has been dirtied by the GPU */
	GLboolean		gpu_dirty;

	/* Memory used for CPU access to the buffer */
	nouveau_mem *		cpu_mem;
	/* Buffer has possibly been dirtied by the CPU */
	GLboolean		cpu_dirty;
} nouveau_buffer_object;

extern uint32_t nouveau_bufferobj_gpu_ref(GLcontext *ctx, GLenum access,
      					  struct gl_buffer_object *obj);

extern void nouveauInitBufferObjects(GLcontext *ctx);

#endif
