/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/



#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "dri_util.h"
#include "drm.h"
#include "nouveau_drm.h"

#include "mtypes.h"
#include "tnl/t_vertex.h"

#include "nouveau_reg.h"
#include "nouveau_screen.h"

#include "xmlconfig.h"

typedef struct nouveau_fifo_t{
	u_int32_t* buffer;
	u_int32_t current;
	u_int32_t put;
	u_int32_t free;
	u_int32_t max;
}
nouveau_fifo;

#define TAG(x) nouveau##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG


typedef void (*nouveau_tri_func)( struct nouveau_context*, 
		nouveauVertex *,
		nouveauVertex *,
		nouveauVertex * );

typedef void (*nouveau_line_func)( struct nouveau_context*, 
		nouveauVertex *,
		nouveauVertex * );

typedef void (*nouveau_point_func)( struct nouveau_context*,
		nouveauVertex * );


typedef struct nouveau_context {
	/* Mesa context */
	GLcontext *glCtx;

	/* The per-context fifo */
	nouveau_fifo fifo;

	/* The fifo control regs */
	volatile unsigned char* fifo_mmio;

	/* The read-only regs */
	volatile unsigned char* mmio;

	/* The drawing fallbacks */
	nouveau_tri_func* draw_tri;
	nouveau_line_func* draw_line;
	nouveau_point_func* draw_point;

	/* Cliprects information */
	GLuint numClipRects;
	drm_clip_rect_t *pClipRects;

	/* The rendering context information */
	GLenum current_primitive; /* the current primitive enum */
	GLuint render_inputs; /* the current render inputs */

	nouveauScreenRec *screen;
	drm_nouveau_sarea_t *sarea;

	__DRIcontextPrivate  *driContext;    /* DRI context */
	__DRIscreenPrivate   *driScreen;     /* DRI screen */
	__DRIdrawablePrivate *driDrawable;   /* DRI drawable bound to this ctx */

	drm_context_t hHWContext;
	drm_hw_lock_t *driHwLock;
	int driFd;

	/* Configuration cache */
	driOptionCache optionCache;
}nouveauContextRec, *nouveauContextPtr;

#define NOUVEAU_CONTEXT(ctx)		((nouveauContextPtr)(ctx->DriverCtx))


extern GLboolean nouveauCreateContext( const __GLcontextModes *glVisual,
		__DRIcontextPrivate *driContextPriv,
		void *sharedContextPrivate );

extern void nouveauDestroyContext( __DRIcontextPrivate * );

extern GLboolean nouveauMakeCurrent( __DRIcontextPrivate *driContextPriv,
		__DRIdrawablePrivate *driDrawPriv,
		__DRIdrawablePrivate *driReadPriv );

extern GLboolean nouveauUnbindContext( __DRIcontextPrivate *driContextPriv );


#endif /* __NOUVEAU_CONTEXT_H__ */

