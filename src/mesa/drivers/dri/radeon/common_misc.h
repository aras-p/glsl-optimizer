#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "common_context.h"
void radeonRecalcScissorRects(radeonContextPtr radeon);
void radeonSetCliprects(radeonContextPtr radeon);
void radeonUpdateScissor( GLcontext *ctx );

void radeonWaitForIdleLocked(radeonContextPtr radeon);
extern uint32_t radeonGetAge(radeonContextPtr radeon);
void radeonCopyBuffer( __DRIdrawablePrivate *dPriv,
		       const drm_clip_rect_t	  *rect);
void radeonPageFlip( __DRIdrawablePrivate *dPriv );
void radeon_common_finish(GLcontext * ctx);
void radeonSwapBuffers(__DRIdrawablePrivate * dPriv);
void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h );

void radeonUpdatePageFlipping(radeonContextPtr rmesa);

#endif
