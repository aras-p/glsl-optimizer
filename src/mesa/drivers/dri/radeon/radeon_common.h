#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "radeon_common_context.h"
#include "radeon_dma.h"
#include "radeon_texture.h"

void radeonRecalcScissorRects(radeonContextPtr radeon);
void radeonSetCliprects(radeonContextPtr radeon);
void radeonUpdateScissor( GLcontext *ctx );
void radeonScissor(GLcontext* ctx, GLint x, GLint y, GLsizei w, GLsizei h);

void radeonWaitForIdleLocked(radeonContextPtr radeon);
extern uint32_t radeonGetAge(radeonContextPtr radeon);
void radeonCopyBuffer( __DRIdrawablePrivate *dPriv,
		       const drm_clip_rect_t	  *rect);
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv);
void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h );

void radeonUpdatePageFlipping(radeonContextPtr rmesa);

void radeonFlush(GLcontext *ctx);
void radeonFinish(GLcontext * ctx);
void radeonEmitState(radeonContextPtr radeon);

void radeon_window_moved(radeonContextPtr radeon);
void radeon_draw_buffer(GLcontext *ctx, struct gl_framebuffer *fb);
void radeonDrawBuffer( GLcontext *ctx, GLenum mode );
void radeonReadBuffer( GLcontext *ctx, GLenum mode );
void radeon_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height);
void radeon_get_cliprects(radeonContextPtr radeon,
			  struct drm_clip_rect **cliprects,
			  unsigned int *num_cliprects,
			  int *x_off, int *y_off);

void radeon_fbo_init(struct radeon_context *radeon);
struct gl_renderbuffer *
radeon_create_renderbuffer(GLenum format, __DRIdrawablePrivate *driDrawPriv);
static inline struct radeon_renderbuffer *radeon_renderbuffer(struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = (struct radeon_renderbuffer *)rb;
	if (rrb && rrb->base.ClassID == RADEON_RB_CLASS)
		return rrb;
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_renderbuffer(struct gl_framebuffer *fb, int att_index)
{
	if (att_index >= 0)
		return radeon_renderbuffer(fb->Attachment[att_index].Renderbuffer);
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_depthbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;
	rrb = rmesa->state.depth.rrb;
	if (!rrb)
		return NULL;

	return rrb;
}

static inline struct radeon_renderbuffer *radeon_get_colorbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;

	rrb = rmesa->state.color.rrb;
	if (!rrb)
		return NULL;
	return rrb;
}

#include "radeon_cmdbuf.h"


#endif
