/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef _VIACONTEXT_H
#define _VIACONTEXT_H

typedef struct via_context_t viaContext;
typedef struct via_context_t *viaContextPtr;
typedef struct via_texture_object_t *viaTextureObjectPtr;

#include "dri_util.h"

#include "mtypes.h"
#include "drm.h"
#include "mm.h"
#include "tnl/t_vertex.h"

#include "via_screen.h"
#include "via_tex.h"
#include "via_common.h"

/* Chip tags.  These are used to group the adapters into
 * related families.
 */
enum VIACHIPTAGS {
    VIA_UNKNOWN = 0,
    VIA_CLE266,
    VIA_KM400,
    VIA_K8M800,
    VIA_PM800,
    VIA_LAST
};

#define VIA_FALLBACK_TEXTURE           	0x1
#define VIA_FALLBACK_DRAW_BUFFER       	0x2
#define VIA_FALLBACK_READ_BUFFER       	0x4
#define VIA_FALLBACK_COLORMASK         	0x8
#define VIA_FALLBACK_SPECULAR          	0x20
#define VIA_FALLBACK_LOGICOP           	0x40
#define VIA_FALLBACK_RENDERMODE        	0x80
#define VIA_FALLBACK_STENCIL           	0x100
#define VIA_FALLBACK_BLEND_EQ          	0x200
#define VIA_FALLBACK_BLEND_FUNC        	0x400
#define VIA_FALLBACK_USER_DISABLE      	0x800
#define VIA_FALLBACK_PROJ_TEXTURE      	0x1000
#define VIA_FALLBACK_STIPPLE		0x2000
#define VIA_FALLBACK_ALPHATEST		0x4000

#define VIA_DMA_BUFSIZ                  4096
#define VIA_DMA_HIGHWATER               (VIA_DMA_BUFSIZ - 128)

#define VIA_NO_CLIPRECTS 0x1


/* Use the templated vertex formats:
 */
#define TAG(x) via##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*via_tri_func)(viaContextPtr, viaVertex *, viaVertex *,
                             viaVertex *);
typedef void (*via_line_func)(viaContextPtr, viaVertex *, viaVertex *);
typedef void (*via_point_func)(viaContextPtr, viaVertex *);

typedef struct {
    drm_handle_t handle;
    drmSize size;
    GLuint offset;
    GLuint index;
    GLuint pitch;
    GLuint bpp;
    char *map;
    GLuint orig;		/* The drawing origin, 
				 * at (drawX,drawY) in screen space.
				 */
    char *origMap;
} viaBuffer, *viaBufferPtr;


struct via_context_t {
    GLint refcount;   
    GLcontext *glCtx;
    GLcontext *shareCtx;
    viaBuffer front;
    viaBuffer back;
    viaBuffer depth;
    GLboolean hasBack;
    GLboolean hasDepth;
    GLboolean hasStencil;
    GLboolean hasAccum;
    GLuint    depthBits;
    GLuint    stencilBits;

   GLboolean have_hw_stencil;
   GLuint ClearDepth;
   GLuint depth_clear_mask;
   GLuint stencil_clear_mask;
   GLfloat depth_max;
   GLfloat polygon_offset_scale;

    GLubyte    *dma;
    viaRegion tex;
    
    GLuint isAGP;

    /* Textures
    */
    viaTextureObjectPtr CurrentTexObj[2];
    struct via_texture_object_t TexObjList;
    struct via_texture_object_t SwappedOut;
    memHeap_t *texHeap;

    /* Bit flag to keep 0track of fallbacks.
     */
    GLuint Fallback;

    /* State for via_tris.c.
     */
    GLuint newState;            /* _NEW_* flags */
    GLuint newEmitState;            /* _NEW_* flags */
    GLuint newRenderState;            /* _NEW_* flags */

    struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
    GLuint vertex_attr_count;

    GLuint setupIndex;
    GLuint renderIndex;
    GLmatrix ViewportMatrix;
    GLenum renderPrimitive;
    GLenum hwPrimitive;
    unsigned char *verts;

    /* drmBufPtr dma_buffer;
    */
    GLuint dmaLow;
    GLuint dmaCliprectAddr;
    GLuint dmaLastPrim;
    GLboolean useAgp;
   

    /* Fallback rasterization functions 
     */
    via_point_func drawPoint;
    via_line_func drawLine;
    via_tri_func drawTri;

    /* Hardware register
     */
    GLuint regCmdA;
    GLuint regCmdA_End;
    GLuint regCmdB;

    GLuint regEnable;
    GLuint regHFBBMSKL;
    GLuint regHROP;

    GLuint regHZWTMD;
    GLuint regHSTREF;
    GLuint regHSTMD;

    GLuint regHATMD;
    GLuint regHABLCsat;
    GLuint regHABLCop;
    GLuint regHABLAsat;
    GLuint regHABLAop;
    GLuint regHABLRCa;
    GLuint regHABLRFCa;
    GLuint regHABLRCbias;
    GLuint regHABLRCb;
    GLuint regHABLRFCb;
    GLuint regHABLRAa;
    GLuint regHABLRAb;
    GLuint regHFogLF;
    GLuint regHFogCL;
    GLuint regHFogCH;

    GLuint regHLP;
    GLuint regHLPRF;

    GLuint regHTXnTB_0;
    GLuint regHTXnMPMD_0;
    GLuint regHTXnTBLCsat_0;
    GLuint regHTXnTBLCop_0;
    GLuint regHTXnTBLMPfog_0;
    GLuint regHTXnTBLAsat_0;
    GLuint regHTXnTBLRCb_0;
    GLuint regHTXnTBLRAa_0;
    GLuint regHTXnTBLRFog_0;
    /*=* John Sheng [2003.7.18] texture combine *=*/
    GLuint regHTXnTBLRCa_0;
    GLuint regHTXnTBLRCc_0;
    GLuint regHTXnTBLRCbias_0;

    GLuint regHTXnTB_1;
    GLuint regHTXnMPMD_1;
    GLuint regHTXnTBLCsat_1;
    GLuint regHTXnTBLCop_1;
    GLuint regHTXnTBLMPfog_1;
    GLuint regHTXnTBLAsat_1;
    GLuint regHTXnTBLRCb_1;
    GLuint regHTXnTBLRAa_1;
    GLuint regHTXnTBLRFog_1;

    int vertexSize;
    int hwVertexSize;
    GLboolean ptexHack;
    int coloroffset;
    int specoffset;

    GLint lastStamp;

    GLenum TexEnvImageFmt[2];
    GLuint ClearColor;
    GLuint ClearMask;

    /* DRI stuff
     */
    GLuint needClip;
    GLframebuffer *glBuffer;
    GLboolean doPageFlip;
    /*=* John Sheng [2003.5.31] flip *=*/
    GLuint currentPage;

    viaBuffer *drawBuffer;
    viaBuffer *readBuffer;
    int drawX;                   /* origin of drawable in draw buffer */
    int drawY;
    
    int drawW;                  
    int drawH;
    
    int drawXoff;
    GLuint numClipRects;         /* cliprects for that buffer */
    drm_clip_rect_t *pClipRects;

    int lastSwap;
    int texAge;
    int ctxAge;
    int dirtyAge;

    GLboolean scissor;
    drm_clip_rect_t drawRect;
    drm_clip_rect_t scissorRect;

    drm_context_t hHWContext;
    drm_hw_lock_t *driHwLock;
    int driFd;
    __DRInativeDisplay *display;

    __DRIdrawablePrivate *driDrawable;
    __DRIscreenPrivate *driScreen;
    viaScreenPrivate *viaScreen;
    drm_via_sarea_t *sarea;
    volatile GLuint* regMMIOBase;
    volatile GLuint* pnGEMode;
    volatile GLuint* regEngineStatus;
    volatile GLuint* regTranSet;
    volatile GLuint* regTranSpace;
    GLuint* agpBase;
    GLuint drawType;

   GLuint nDoneFirstFlip;
   GLuint agpFullCount;

   GLboolean strictConformance;

   /* Configuration cache
    */
   driOptionCache optionCache;

   GLuint vblank_flags;
   GLuint vbl_seq;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;

   GLuint stipple[32];

   PFNGLXGETUSTPROC get_ust;

};



#define VIA_CONTEXT(ctx)   ((viaContextPtr)(ctx->DriverCtx))

#define GET_DISPATCH_AGE(vmesa) vmesa->sarea->lastDispatch
#define GET_ENQUEUE_AGE(vmesa) vmesa->sarea->lastEnqueue


/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE(vmesa)                                	\
	do {                                                    \
    	    char __ret = 0;                                     \
    	    DRM_CAS(vmesa->driHwLock, vmesa->hHWContext,        \
        	(DRM_LOCK_HELD|vmesa->hHWContext), __ret);      \
    	    if (__ret)                                          \
        	viaGetLock(vmesa, 0);                           \
	} while (0)


/* Release the kernel lock.
 */
#define UNLOCK_HARDWARE(vmesa)                                  	\
	DRM_UNLOCK(vmesa->driFd, vmesa->driHwLock, vmesa->hHWContext);	

#define WAIT_IDLE(vmesa)                                                       	\
    do {                                                            	\
	if ((((GLuint)*vmesa->regEngineStatus) & 0xFFFEFFFF) == 0x00020000)	\
	    break;                                                        	\
    } while (1)
	

#ifdef DEBUG
extern GLuint VIA_DEBUG;
#else
#define VIA_DEBUG 0
#endif


extern void viaGetLock(viaContextPtr vmesa, GLuint flags);
extern void viaLock(viaContextPtr vmesa, GLuint flags);
extern void viaUnLock(viaContextPtr vmesa, GLuint flags);
extern void viaEmitHwStateLocked(viaContextPtr vmesa);
extern void viaEmitScissorValues(viaContextPtr vmesa, int box_nr, int emit);
extern void viaXMesaSetBackClipRects(viaContextPtr vmesa);
extern void viaXMesaSetFrontClipRects(viaContextPtr vmesa);
extern void viaReAllocateBuffers(GLframebuffer *drawbuffer);
extern void viaXMesaWindowMoved(viaContextPtr vmesa);

extern void viaTexCombineState(viaContextPtr vmesa,
    const struct gl_tex_env_combine_state * combine, unsigned unit );

/* Via hw already adjusted for GL pixel centers:
 */
#define SUBPIXEL_X 0
#define SUBPIXEL_Y 0

/* TODO XXX _SOLO temp defines to make code compilable */
#ifndef GLX_PBUFFER_BIT
#define GLX_PBUFFER_BIT        0x00000004
#endif
#ifndef GLX_WINDOW_BIT
#define GLX_WINDOW_BIT 0x00000001
#endif
#ifndef VERT_BIT_CLIP
#define VERT_BIT_CLIP       0x1000000
#endif

#endif
