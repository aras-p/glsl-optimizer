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

GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const __GLcontextModes * glVisual,
			    __DRIcontextPrivate * driContextPriv,
			    void *sharedContextPrivate);

void radeonCleanupContext(radeonContextPtr radeon);
void radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable);
GLboolean radeonMakeCurrent(__DRIcontextPrivate * driContextPriv,
			    __DRIdrawablePrivate * driDrawPriv,
			    __DRIdrawablePrivate * driReadPriv);

void rcommon_emit_vector(GLcontext * ctx, struct radeon_aos *aos,
			 GLvoid * data, int size, int stride, int count);
void radeon_print_state_atom( struct radeon_state_atom *state );

struct gl_texture_image *radeonNewTextureImage(GLcontext *ctx);
void radeonFreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage);

void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable);
void radeon_teximage_unmap(radeon_texture_image *image);
void radeonMapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeonUnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeon_generate_mipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj);
#endif
