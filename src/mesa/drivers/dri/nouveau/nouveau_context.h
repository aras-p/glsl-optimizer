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

#include "nouveau_screen.h"
#include "nouveau_state_cache.h"

#include "xmlconfig.h"

typedef struct nouveau_fifo_t{
	u_int32_t* buffer;
	u_int32_t* mmio;
	u_int32_t put_base;
	u_int32_t current;
	u_int32_t put;
	u_int32_t free;
	u_int32_t max;
}
nouveau_fifo;

#define TAG(x) nouveau##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

/* Subpixel offsets for window coordinates (triangles): */
#define SUBPIXEL_X  (0.0F)
#define SUBPIXEL_Y  (0.125F)

struct nouveau_context;

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

	/* The read-only regs */
	volatile unsigned char* mmio;

	/* FIXME : do we want to put all state into a separate struct ? */
	/* State for tris */
	GLuint color_offset;
	GLuint specular_offset;

	/* Vertex state */
	GLuint vertex_size;
	GLubyte *verts;
	struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
	GLuint vertex_attr_count;

	/* Depth/stencil clear state */
	uint32_t clear_value;

	/* Light state */
	GLboolean lighting_enabled;
	uint32_t enabled_lights;

	/* Cached state */
	nouveau_state_cache state_cache;

	/* The drawing fallbacks */
	GLuint Fallback;
	nouveau_tri_func draw_tri;
	nouveau_line_func draw_line;
	nouveau_point_func draw_point;

	/* Cliprects information */
	GLuint numClipRects;
	drm_clip_rect_t *pClipRects;

	/* The rendering context information */
	GLenum current_primitive; /* the current primitive enum */
	DECLARE_RENDERINPUTS(render_inputs_bitset); /* the current render inputs */

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

        /* vblank stuff */
        uint32_t vblank_flags;
        uint32_t vblank_seq;

        GLuint new_state;
        GLuint new_render_state;
        GLuint render_index;
        GLmatrix viewport;
        GLfloat depth_scale;

}nouveauContextRec, *nouveauContextPtr;


#define NOUVEAU_CONTEXT(ctx)		((nouveauContextPtr)(ctx->DriverCtx))

/* Flags for what context state needs to be updated: */
#define NOUVEAU_NEW_ALPHA		0x0001
#define NOUVEAU_NEW_DEPTH		0x0002
#define NOUVEAU_NEW_FOG	        	0x0004
#define NOUVEAU_NEW_CLIP		0x0008
#define NOUVEAU_NEW_CULL		0x0010
#define NOUVEAU_NEW_MASKS		0x0020
#define NOUVEAU_NEW_RENDER_NOT	        0x0040
#define NOUVEAU_NEW_WINDOW		0x0080
#define NOUVEAU_NEW_CONTEXT	        0x0100
#define NOUVEAU_NEW_ALL		        0x01ff

/* Flags for software fallback cases: */
#define NOUVEAU_FALLBACK_TEXTURE		0x0001
#define NOUVEAU_FALLBACK_DRAW_BUFFER		0x0002
#define NOUVEAU_FALLBACK_READ_BUFFER		0x0004
#define NOUVEAU_FALLBACK_STENCIL		0x0008
#define NOUVEAU_FALLBACK_RENDER_MODE		0x0010
#define NOUVEAU_FALLBACK_LOGICOP		0x0020
#define NOUVEAU_FALLBACK_SEP_SPECULAR		0x0040
#define NOUVEAU_FALLBACK_BLEND_EQ		0x0080
#define NOUVEAU_FALLBACK_BLEND_FUNC		0x0100
#define NOUVEAU_FALLBACK_PROJTEX		0x0200
#define NOUVEAU_FALLBACK_DISABLE		0x0400


extern GLboolean nouveauCreateContext( const __GLcontextModes *glVisual,
		__DRIcontextPrivate *driContextPriv,
		void *sharedContextPrivate );

extern void nouveauDestroyContext( __DRIcontextPrivate * );

extern GLboolean nouveauMakeCurrent( __DRIcontextPrivate *driContextPriv,
		__DRIdrawablePrivate *driDrawPriv,
		__DRIdrawablePrivate *driReadPriv );

extern GLboolean nouveauUnbindContext( __DRIcontextPrivate *driContextPriv );

extern void nouveauSwapBuffers(__DRIdrawablePrivate *dPriv);

extern void nouveauCopySubBuffer(__DRIdrawablePrivate *dPriv,
				 int x, int y, int w, int h);

#endif /* __NOUVEAU_CONTEXT_H__ */

