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

void rcommonEnsureCmdBufSpace(radeonContextPtr rmesa, int dwords, const char *caller);
int rcommonFlushCmdBuf(radeonContextPtr rmesa, const char *caller);
int rcommonFlushCmdBufLocked(radeonContextPtr rmesa, const char *caller);
void rcommonInitCmdBuf(radeonContextPtr rmesa, int max_state_size);
void rcommonDestroyCmdBuf(radeonContextPtr rmesa);
#endif
