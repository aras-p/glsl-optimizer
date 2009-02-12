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
void radeonPageFlip( __DRIdrawablePrivate *dPriv );
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv);
void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h );

void radeonUpdatePageFlipping(radeonContextPtr rmesa);

void radeonFlush(GLcontext *ctx);
void radeonFinish(GLcontext * ctx);
void radeonEmitState(radeonContextPtr radeon);

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
	GLframebuffer *fb = rmesa->dri.drawable->driverPrivate;

	rrb = rmesa->state.color.rrb;
	if (rmesa->radeonScreen->driScreen->dri2.enabled) {
		rrb = (struct radeon_renderbuffer *)fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
	}
	if (!rrb)
		return NULL;
	return rrb;
}

#include "radeon_cmdbuf.h"


#endif
