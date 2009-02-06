#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "common_context.h"
#include "radeon_buffer.h"
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

void radeonEmitVec8(uint32_t *out, GLvoid * data, int stride, int count);
void radeonEmitVec12(uint32_t *out, GLvoid * data, int stride, int count);

void rcommon_emit_vector(GLcontext * ctx, struct radeon_aos *aos,
			 GLvoid * data, int size, int stride, int count);
void radeon_print_state_atom( struct radeon_state_atom *state );

struct gl_texture_image *radeonNewTextureImage(GLcontext *ctx);
void radeonFreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage);

void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable);
void radeon_teximage_unmap(radeon_texture_image *image);
void radeonMapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeonUnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeonGenerateMipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj);
int radeon_validate_texture_miptree(GLcontext * ctx, struct gl_texture_object *texObj);
GLuint radeon_face_for_target(GLenum target);
const struct gl_texture_format *radeonChooseTextureFormat(GLcontext * ctx,
							  GLint internalFormat,
							  GLenum format,
							  GLenum type);

void radeonTexImage1D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonTexImage2D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonCompressedTexImage2D(GLcontext * ctx, GLenum target,
				GLint level, GLint internalFormat,
				GLint width, GLint height, GLint border,
				GLsizei imageSize, const GLvoid * data,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage);
void radeonTexImage3D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint depth,
		      GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset,
			 GLsizei width,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage);
void radeonTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLenum type,
				const GLvoid * pixels,
				const struct gl_pixelstore_attrib *packing,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage);
void radeonCompressedTexSubImage2D(GLcontext * ctx, GLenum target,
				   GLint level, GLint xoffset,
				   GLint yoffset, GLsizei width,
				   GLsizei height, GLenum format,
				   GLsizei imageSize, const GLvoid * data,
				   struct gl_texture_object *texObj,
				   struct gl_texture_image *texImage);

void radeonTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage);

void radeonSpanRenderStart(GLcontext * ctx);
void radeonSpanRenderFinish(GLcontext * ctx);
GLubyte *radeon_ptr(const struct radeon_renderbuffer * rrb,
		    GLint x, GLint y);
GLubyte *radeon_ptr16(const struct radeon_renderbuffer * rrb,
		    GLint x, GLint y);
GLubyte *radeon_ptr32(const struct radeon_renderbuffer * rrb,
		    GLint x, GLint y);
void radeonRefillCurrentDmaRegion(radeonContextPtr rmesa, int size);
void radeonAllocDmaRegion(radeonContextPtr rmesa,
			  struct radeon_bo **pbo, int *poffset,
			  int bytes, int alignment);
void radeonReleaseDmaRegion(radeonContextPtr rmesa);

void rcommon_flush_last_swtcl_prim(GLcontext *ctx);

void *rcommonAllocDmaLowVerts(radeonContextPtr rmesa, int nverts, int vsize);


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


#endif
