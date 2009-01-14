#ifndef COMMON_MISC_H
#define COMMON_MISC_H

void radeonRecalcScissorRects(radeonContextPtr radeon);
void radeonSetCliprects(radeonContextPtr radeon);
void radeonUpdateScissor( GLcontext *ctx );

void radeonWaitForIdleLocked(radeonContextPtr radeon);
extern uint32_t radeonGetAge(radeonContextPtr radeon);
void radeonCopyBuffer( __DRIdrawablePrivate *dPriv,
		       const drm_clip_rect_t	  *rect);
void radeonPageFlip( __DRIdrawablePrivate *dPriv );
void radeon_common_finish(GLcontext * ctx);

#endif
