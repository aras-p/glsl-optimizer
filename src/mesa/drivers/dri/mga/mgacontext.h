/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgacontext.h,v 1.7 2002/12/16 16:18:52 dawes Exp $*/
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef MGALIB_INC
#define MGALIB_INC

/*#include <X11/Xlibint.h>*/
#include "dri_util.h"
#include "mtypes.h"
#include "xf86drm.h"
#include "mm.h"
/*#include "mem.h"*/
#include "mga_sarea.h"


#define MGA_SET_FIELD(reg,mask,val)  reg = ((reg) & (mask)) | ((val) & ~(mask))
#define MGA_FIELD(field,val) (((val) << (field ## _SHIFT)) & ~(field ## _MASK))
#define MGA_GET_FIELD(field, val) ((val & ~(field ## _MASK)) >> (field ## _SHIFT))

#define MGA_IS_G200(mmesa) (mmesa->mgaScreen->chipset == MGA_CARD_TYPE_G200)
#define MGA_IS_G400(mmesa) (mmesa->mgaScreen->chipset == MGA_CARD_TYPE_G400)


/* SoftwareFallback
 *    - texture env GL_BLEND -- can be fixed
 *    - 1D and 3D textures
 *    - incomplete textures
 *    - GL_DEPTH_FUNC == GL_NEVER not in h/w
 */
#define MGA_FALLBACK_TEXTURE        0x1
#define MGA_FALLBACK_DRAW_BUFFER    0x2
#define MGA_FALLBACK_READ_BUFFER    0x4
#define MGA_FALLBACK_LOGICOP        0x8
#define MGA_FALLBACK_RENDERMODE     0x10
#define MGA_FALLBACK_STENCIL        0x20
#define MGA_FALLBACK_DEPTH          0x40


/* For mgaCtx->new_state.
 */
#define MGA_NEW_DEPTH   0x1
#define MGA_NEW_ALPHA   0x2
#define MGA_NEW_CLIP    0x8
#define MGA_NEW_TEXTURE 0x20
#define MGA_NEW_CULL    0x40
#define MGA_NEW_WARP    0x80
#define MGA_NEW_STENCIL 0x100
#define MGA_NEW_CONTEXT 0x200

/* Use the templated vertex formats:
 */
#define TAG(x) mga##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef struct mga_context_t mgaContext;
typedef struct mga_context_t *mgaContextPtr;

typedef void (*mga_tri_func)( mgaContextPtr, mgaVertex *, mgaVertex *,
			       mgaVertex * );
typedef void (*mga_line_func)( mgaContextPtr, mgaVertex *, mgaVertex * );
typedef void (*mga_point_func)( mgaContextPtr, mgaVertex * );



/* Reasons why the GL_BLEND fallback mightn't work:
 */
#define MGA_BLEND_ENV_COLOR 0x1
#define MGA_BLEND_MULTITEX  0x2

struct mga_texture_object_s;
struct mga_screen_private_s;

#define MGA_TEX_MAXLEVELS 5

typedef struct mga_texture_object_s
{
   struct mga_texture_object_s *next;
   struct mga_texture_object_s *prev;
   struct gl_texture_object *tObj;
   struct mga_context_t *ctx;
   PMemBlock	MemBlock;
   GLuint		offsets[MGA_TEX_MAXLEVELS];
   int             lastLevel;
   GLuint         dirty_images;
   GLuint		totalSize;
   int		texelBytes;
   GLuint 	age;
   int             bound;
   int             heap;	/* agp or card */

   mga_texture_regs_t setup;
} mgaTextureObject_t;

struct mga_context_t {

   GLcontext *glCtx;
   unsigned int lastStamp;	/* fullscreen breaks dpriv->laststamp,
				 * need to shadow it here. */

   /* Bookkeeping for texturing
    */
   int lastTexHeap;
   struct mga_texture_object_s TexObjList[MGA_NR_TEX_HEAPS];
   struct mga_texture_object_s SwappedOut;
   struct mga_texture_object_s *CurrentTexObj[2];
   memHeap_t *texHeap[MGA_NR_TEX_HEAPS];
   int c_texupload;
   int c_texusage;
   int tex_thrash;


   /* Map GL texture units onto hardware.
    */
   GLuint tmu_source[2];
   
   GLboolean default32BitTextures;

   /* Manage fallbacks
    */
   GLuint Fallback;  


   /* Temporaries for translating away float colors:
    */
   struct gl_client_array UbyteColor;
   struct gl_client_array UbyteSecondaryColor;

   /* Support for limited GL_BLEND fallback
    */
   unsigned int blend_flags;
   unsigned int envcolor;

   /* Rasterization state 
    */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   
   GLuint hw_primitive;
   GLenum raster_primitive;
   GLenum render_primitive;

   char *verts;
   GLint vertex_stride_shift;
   GLuint vertex_format;		
   GLuint vertex_size;

   /* Fallback rasterization functions 
    */
   mga_point_func draw_point;
   mga_line_func draw_line;
   mga_tri_func draw_tri;


   /* Manage driver and hardware state
    */
   GLuint        new_gl_state; 
   GLuint        new_state; 
   GLuint        dirty;

   mga_context_regs_t setup;

   GLuint        ClearColor;
   GLuint        ClearDepth;
   GLuint        poly_stipple;
   GLfloat       depth_scale;

   GLuint        depth_clear_mask;
   GLuint        stencil_clear_mask;
   GLuint        hw_stencil;
   GLuint        haveHwStipple;
   GLfloat       hw_viewport[16];

   /* Dma buffers
    */
   drmBufPtr  vertex_dma_buffer;
   drmBufPtr  iload_buffer;

   /* VBI
    */
   GLuint vbl_seq;

   /* Drawable, cliprect and scissor information
    */
   int dirty_cliprects;		/* which sets of cliprects are uptodate? */
   int draw_buffer;		/* which buffer are we rendering to */
   unsigned int drawOffset;		/* draw buffer address in  space */
   int read_buffer;
   int readOffset;
   int drawX, drawY;		/* origin of drawable in draw buffer */
   int lastX, lastY;		/* detect DSTORG bug */
   GLuint numClipRects;		/* cliprects for the draw buffer */
   XF86DRIClipRectPtr pClipRects;
   XF86DRIClipRectRec draw_rect;
   XF86DRIClipRectRec scissor_rect;
   int scissor;

   XF86DRIClipRectRec tmp_boxes[2][MGA_NR_SAREA_CLIPRECTS];


   /* Texture aging and DMA based aging.
    */
   unsigned int texAge[MGA_NR_TEX_HEAPS];/* texture LRU age  */
   unsigned int dirtyAge;		/* buffer age for synchronization */

   GLuint primary_offset;

   /* Mirrors of some DRI state.
    */
   GLframebuffer *glBuffer;
   drmContext hHWContext;
   drmLock *driHwLock;
   int driFd;
   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   struct mga_screen_private_s *mgaScreen;
   MGASAREAPrivPtr sarea;
};

#define MGA_CONTEXT(ctx) ((mgaContextPtr)(ctx->DriverCtx))

#define MGAPACKCOLOR555(r,g,b,a) \
  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
    ((a) ? 0x8000 : 0))

#define MGAPACKCOLOR565(r,g,b) \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define MGAPACKCOLOR88(l, a) \
  (((l) << 8) | (a))

#define MGAPACKCOLOR888(r,g,b) \
  (((r) << 16) | ((g) << 8) | (b))

#define MGAPACKCOLOR8888(r,g,b,a) \
  (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define MGAPACKCOLOR4444(r,g,b,a) \
  ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))


#define MGA_DEBUG 0
#ifndef MGA_DEBUG
extern int MGA_DEBUG;
#endif

#define DEBUG_ALWAYS_SYNC	0x1
#define DEBUG_VERBOSE_MSG	0x2
#define DEBUG_VERBOSE_LRU	0x4
#define DEBUG_VERBOSE_DRI	0x8
#define DEBUG_VERBOSE_IOCTL	0x10
#define DEBUG_VERBOSE_2D	0x20
#define DEBUG_VERBOSE_FALLBACK	0x40

static __inline__ GLuint mgaPackColor(GLuint cpp,
				      GLubyte r, GLubyte g,
				      GLubyte b, GLubyte a)
{
  switch (cpp) {
  case 2:
    return MGAPACKCOLOR565(r,g,b);
  case 4:
    return MGAPACKCOLOR8888(r,g,b,a);
  default:
    return 0;
  }
}


/*
 * Subpixel offsets for window coordinates:
 */
#define SUBPIXEL_X (-0.5F)
#define SUBPIXEL_Y (-0.5F + 0.125)


#define MGA_WA_TRIANGLES     0x18000000
#define MGA_WA_TRISTRIP_T0   0x02010200
#define MGA_WA_TRIFAN_T0     0x01000408
#define MGA_WA_TRISTRIP_T0T1 0x02010400
#define MGA_WA_TRIFAN_T0T1   0x01000810

#endif
