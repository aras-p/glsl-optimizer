#include "bufferobj.h"
#include "enums.h"

#include "nouveau_bufferobj.h"
#include "nouveau_buffers.h"
#include "nouveau_context.h"
#include "nouveau_drm.h"
#include "nouveau_object.h"
#include "nouveau_msg.h"

#define DEBUG(fmt,args...) do {                \
	if (NOUVEAU_DEBUG & DEBUG_BUFFEROBJ) { \
		fprintf(stderr, "%s: "fmt, __func__, ##args);  \
	}                                      \
} while(0)

/* Wrapper for nouveau_mem_gpu_offset_get() that marks the bufferobj dirty
 * if the GPU modifies the data.
 */
uint32_t
nouveau_bufferobj_gpu_ref(GLcontext *ctx, GLenum access,
			  struct gl_buffer_object *obj)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	DEBUG("obj=%p, access=%s\n", obj, _mesa_lookup_enum_by_nr(access));

	if (access == GL_WRITE_ONLY_ARB || access == GL_READ_WRITE_ARB)
		nbo->gpu_dirty = GL_TRUE;

	return nouveau_mem_gpu_offset_get(ctx, nbo->gpu_mem);
}

static void
nouveauBindBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)
{
}

static struct gl_buffer_object *
nouveauNewBufferObject(GLcontext *ctx, GLuint buffer, GLenum target)
{
	nouveau_buffer_object *nbo;

	nbo = CALLOC_STRUCT(nouveau_buffer_object_t);
	DEBUG("name=0x%08x, target=%s, obj=%p\n",
			buffer, _mesa_lookup_enum_by_nr(target), nbo);
	_mesa_initialize_buffer_object(&nbo->mesa, buffer, target);
	return &nbo->mesa;
}

static void
nouveauDeleteBuffer(GLcontext *ctx, struct gl_buffer_object *obj)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	DEBUG("obj=%p\n", obj);

	if (nbo->gpu_mem) {
		nouveau_mem_free(ctx, nbo->gpu_mem);
	}
	_mesa_delete_buffer_object(ctx, obj);
}

static void
nouveauBufferData(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		  const GLvoid *data, GLenum usage,
		  struct gl_buffer_object *obj)
{
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	DEBUG("obj=%p, target=%s, usage=%s, size=%d, data=%p\n",
			obj,
			_mesa_lookup_enum_by_nr(target),
			_mesa_lookup_enum_by_nr(usage),
			(unsigned int)size,
			data);

	if (nbo->gpu_mem && nbo->gpu_mem->size != size)
		nouveau_mem_free(ctx, nbo->gpu_mem);

	/* Always have the GPU access the data from VRAM if possible.  For
	 * some "usage" values it may be better from AGP be default?
	 *
	 * TODO: At some point we should drop the NOUVEAU_MEM_MAPPED flag.
	 * TODO: Use the NOUVEAU_MEM_AGP_ACCEPTABLE flag.
	 * TODO: What about PCI-E and shared system memory?
	 */
	if (!nbo->gpu_mem)
		nbo->gpu_mem = nouveau_mem_alloc(ctx,
						 NOUVEAU_MEM_FB |
						 NOUVEAU_MEM_MAPPED,
						 size,
						 0);

	if (!nbo->gpu_mem) {
		MESSAGE("AIII bufferobj malloc failed\n");
		return;
	}

	obj->Usage = usage;
	obj->Size  = size;
	if (!data)
		return;

	ctx->Driver.MapBuffer(ctx, target, GL_WRITE_ONLY_ARB, obj);
	_mesa_memcpy(nbo->cpu_mem->map, data, size);
	ctx->Driver.UnmapBuffer(ctx, target, obj);
}

/*TODO: we don't need to DMA the entire buffer like MapBuffer does.. */
static void
nouveauBufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
		     GLsizeiptrARB size, const GLvoid *data,
		     struct gl_buffer_object *obj)
{
	DEBUG("obj=%p, target=%s, offset=0x%x, size=%d, data=%p\n",
			obj,
			_mesa_lookup_enum_by_nr(target),
			(unsigned int)offset,
			(unsigned int)size,
			data);

	ctx->Driver.MapBuffer(ctx, target, GL_WRITE_ONLY_ARB, obj);
	_mesa_memcpy((GLubyte *)obj->Pointer + offset, data, size);
	ctx->Driver.UnmapBuffer(ctx, target, obj);
}

/*TODO: we don't need to DMA the entire buffer like MapBuffer does.. */
static void
nouveauGetBufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
		     GLsizeiptrARB size, GLvoid *data,
		     struct gl_buffer_object *obj)
{
	DEBUG("obj=%p, target=%s, offset=0x%x, size=%d, data=%p\n",
			obj,
			_mesa_lookup_enum_by_nr(target),
			(unsigned int)offset,
			(unsigned int)size,
			data);

	ctx->Driver.MapBuffer(ctx, target, GL_READ_ONLY_ARB, obj);
	_mesa_memcpy(data, (GLubyte *)obj->Pointer + offset, size);
	ctx->Driver.UnmapBuffer(ctx, target, obj);
}

static void *
nouveauMapBuffer(GLcontext *ctx, GLenum target, GLenum access,
		 struct gl_buffer_object *obj)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	DEBUG("obj=%p, target=%s, access=%s\n",
			obj,
			_mesa_lookup_enum_by_nr(target),
			_mesa_lookup_enum_by_nr(access));

	if (obj->Pointer) {
		DEBUG("already mapped, return NULL\n");
		return NULL;
	}

#ifdef ALLOW_MULTI_SUBCHANNEL
	/* If GPU is accessing the data from VRAM, copy to faster AGP memory
	 * before CPU access to the buffer.
	 */
	if (nbo->gpu_mem->type & NOUVEAU_MEM_FB) {
		DEBUG("Data in VRAM, copying to AGP for CPU access\n");

		/* This can happen if BufferData grows the GPU-access buffer */
		if (nbo->cpu_mem && nbo->cpu_mem->size != nbo->gpu_mem->size) {
			nouveau_mem_free(ctx, nbo->cpu_mem);
			nbo->cpu_mem = NULL;
		}

		if (!nbo->cpu_mem) {
			nbo->cpu_mem = nouveau_mem_alloc(ctx,
							 NOUVEAU_MEM_AGP |
							 NOUVEAU_MEM_MAPPED,
							 nbo->gpu_mem->size,
							 0);

			/* Mark GPU data as modified, so it gets copied to
			 * the new buffer */
			nbo->gpu_dirty = GL_TRUE;
		}

		if (nbo->cpu_mem && nbo->gpu_dirty) {
			nouveau_memformat_flat_emit(ctx, nbo->cpu_mem,
							 nbo->gpu_mem,
							 0, 0,
							 nbo->gpu_mem->size);

			nouveau_notifier_wait_nop(ctx,
						  nmesa->syncNotifier,
						  NvSubMemFormat);
			nbo->gpu_dirty = GL_FALSE;
		}

		/* buffer isn't guaranteed to be up-to-date on the card now */
		nbo->cpu_dirty = GL_TRUE;
	}
#endif

	/* If the copy to AGP failed for some reason, just return a pointer
	 * directly to vram..
	 */
	if (!nbo->cpu_mem) {
		DEBUG("Returning direct pointer to VRAM\n");
		nbo->cpu_mem   = nbo->gpu_mem;
		nbo->cpu_dirty = GL_FALSE;
	}

	obj->Pointer = nbo->cpu_mem->map;
	return obj->Pointer;
}

static GLboolean
nouveauUnmapBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *obj)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_buffer_object *nbo = (nouveau_buffer_object *)obj;

	DEBUG("obj=%p, target=%s\n", obj, _mesa_lookup_enum_by_nr(target));

#ifdef ALLOW_MULTI_SUBCHANNEL
	if (nbo->cpu_dirty && nbo->cpu_mem != nbo->gpu_mem) {
		DEBUG("Copying potentially modified data back to GPU\n");

		/* blit from GPU buffer -> CPU  buffer */
		nouveau_memformat_flat_emit(ctx, nbo->gpu_mem, nbo->cpu_mem,
		      			    0, 0, nbo->cpu_mem->size);

		/* buffer is now up-to-date on the hardware (or rather, will
		 * be by the time any other commands in this channel reference
		 * the data.)
		 */
		nbo->cpu_dirty = GL_FALSE;

		/* we can avoid this wait in some cases.. */
		nouveau_notifier_wait_nop(ctx,
					  nmesa->syncNotifier,
					  NvSubMemFormat);

		/* If it's likely CPU access to the buffer will occur often,
		 * keep the cpu_mem around to avoid repeated allocs.
		 */
		if (obj->Usage != GL_DYNAMIC_DRAW_ARB) {

			nouveau_mem_free(ctx, nbo->cpu_mem);
			nbo->cpu_mem = NULL;
		}
	}
#endif

	obj->Pointer = NULL;
	return GL_TRUE;
}
	  
void
nouveauInitBufferObjects(GLcontext *ctx)
{
	ctx->Driver.BindBuffer		= nouveauBindBuffer;
	ctx->Driver.NewBufferObject	= nouveauNewBufferObject;
	ctx->Driver.DeleteBuffer	= nouveauDeleteBuffer;
	ctx->Driver.BufferData		= nouveauBufferData;
	ctx->Driver.BufferSubData	= nouveauBufferSubData;
	ctx->Driver.GetBufferSubData	= nouveauGetBufferSubData;
	ctx->Driver.MapBuffer		= nouveauMapBuffer;
	ctx->Driver.UnmapBuffer		= nouveauUnmapBuffer;
}

