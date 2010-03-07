#ifndef COMMON_MISC_H
#define COMMON_MISC_H

#include "radeon_common_context.h"
#include "radeon_dma.h"
#include "radeon_texture.h"

void radeonUserClear(GLcontext *ctx, GLuint mask);
void radeonRecalcScissorRects(radeonContextPtr radeon);
void radeonSetCliprects(radeonContextPtr radeon);
void radeonUpdateScissor( GLcontext *ctx );
void radeonScissor(GLcontext* ctx, GLint x, GLint y, GLsizei w, GLsizei h);

void radeonWaitForIdleLocked(radeonContextPtr radeon);
extern uint32_t radeonGetAge(radeonContextPtr radeon);
void radeonCopyBuffer( __DRIdrawable *dPriv,
		       const drm_clip_rect_t	  *rect);
void radeonSwapBuffers(__DRIdrawable * dPriv);
void radeonCopySubBuffer(__DRIdrawable * dPriv,
			 int x, int y, int w, int h );

void radeonUpdatePageFlipping(radeonContextPtr rmesa);

void radeonFlush(GLcontext *ctx);
void radeonFinish(GLcontext * ctx);
void radeonEmitState(radeonContextPtr radeon);
GLuint radeonCountStateEmitSize(radeonContextPtr radeon);

void radeon_clear_tris(GLcontext *ctx, GLbitfield mask);

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
void
radeon_renderbuffer_set_bo(struct radeon_renderbuffer *rb,
			   struct radeon_bo *bo);
struct radeon_renderbuffer *
radeon_create_renderbuffer(gl_format format, __DRIdrawable *driDrawPriv);

void radeon_check_front_buffer_rendering(GLcontext *ctx);
static inline struct radeon_renderbuffer *radeon_renderbuffer(struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = (struct radeon_renderbuffer *)rb;
	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s(rb %p)\n",
		__func__, rb);
	if (rrb && rrb->base.ClassID == RADEON_RB_CLASS)
		return rrb;
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_renderbuffer(struct gl_framebuffer *fb, int att_index)
{
	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s(fb %p, index %d)\n",
		__func__, fb, att_index);

	if (att_index >= 0)
		return radeon_renderbuffer(fb->Attachment[att_index].Renderbuffer);
	else
		return NULL;
}

static inline struct radeon_renderbuffer *radeon_get_depthbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;
	rrb = radeon_renderbuffer(rmesa->state.depth.rb);
	if (!rrb)
		return NULL;

	return rrb;
}

static inline struct radeon_renderbuffer *radeon_get_colorbuffer(radeonContextPtr rmesa)
{
	struct radeon_renderbuffer *rrb;

	rrb = radeon_renderbuffer(rmesa->state.color.rb);
	if (!rrb)
		return NULL;
	return rrb;
}

#include "radeon_cmdbuf.h"


#endif
