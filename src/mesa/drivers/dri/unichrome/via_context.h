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

#include "via_screen.h"
#include "via_tex.h"
#include "via_common.h"
#include "xf86drmVIA.h"
#ifdef USE_XINERAMA
#include "../../../../../include/extensions/Xinerama.h"
#endif
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

#define VIA_UPLOAD_NONE		  	0x0000
#define VIA_UPLOAD_ALPHATEST		0x0001
#define VIA_UPLOAD_BLEND	  	0x0002
#define VIA_UPLOAD_FOG 		  	0x0004
#define VIA_UPLOAD_MASK_ROP		0x0008
#define VIA_UPLOAD_LINESTIPPLE		0x0010
#define VIA_UPLOAD_POLYGONSTIPPLE	0x0020
#define VIA_UPLOAD_DEPTH		0x0040
#define VIA_UPLOAD_TEXTURE		0x0080
#define VIA_UPLOAD_STENCIL		0x0100
#define VIA_UPLOAD_CLIPPING		0x0200
#define VIA_UPLOAD_DESTBUFFER		0x0400
#define VIA_UPLOAD_DEPTHBUFFER		0x0800
#define VIA_UPLOAD_ENABLE		0x0800
#define VIA_UPLOAD_ALL 		  	0x1000
  		
#define VIA_DMA_BUFSIZ                  500000

/* Use the templated vertex formats:
 */
#define TAG(x) via##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define RightOf 1
#define LeftOf 2
#define Down 4
#define Up 8
#define S0 16
#define S1 32
#define P_MASK 0x0f;
#define S_MASK 0x30;
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
} viaBuffer, *viaBufferPtr;


struct via_context_t {
    GLint refcount;   
    GLcontext *glCtx;
    GLcontext *shareCtx;
    unsigned char* front_base;
    viaBuffer front;
    viaBuffer back;
    viaBuffer depth;
    GLboolean hasBack;
    GLboolean hasDepth;
    GLboolean hasStencil;
    GLboolean hasAccum;
    GLuint    depthBits;
    GLuint    stencilBits;
    GLuint    *dma;
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

    /* Temporaries for translating away float colors:
     */
    struct gl_client_array UbyteColor;
    struct gl_client_array UbyteSecondaryColor;

    /* State for via_vb.c and via_tris.c.
     */
    GLuint newState;            /* _NEW_* flags */
    GLuint setupNewInputs;
    GLuint setupIndex;
    GLuint renderIndex;
    GLmatrix ViewportMatrix;
    GLenum renderPrimitive;
    GLenum reducedPrimitive;
    GLuint hwPrimitive;
    unsigned char *verts;

    /* drmBufPtr dma_buffer;
    */
    unsigned char* dmaAddr;
    GLuint dmaLow;
    GLuint dmaHigh;
    GLuint dmaLastPrim;
    GLboolean useAgp;
   
    GLboolean uploadCliprects;

    GLuint needUploadAllState;
    GLuint primitiveRendered;
    

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

    /* Hardware state 
     */
    GLuint dirty;             
    int vertexSize;
    int vertexStrideShift;
    GLint lastStamp;
    GLboolean stippleInHw;

    GLenum TexEnvImageFmt[2];
    GLuint ClearColor;
    /* DRI stuff
     */
    GLuint needClip;
    GLframebuffer *glBuffer;
    GLboolean doPageFlip;
    /*=* John Sheng [2003.5.31] flip *=*/
    GLuint currentPage;
    char *drawMap;               /* draw buffer address in virtual mem */
    char *readMap;       
    int drawX;                   /* origin of drawable in draw buffer */
    int drawY;
    
    int drawW;                  
    int drawH;
    GLuint saam;
#ifdef USE_XINERAMA
    XineramaScreenInfo *xsi;
#endif
    int drawXoffSaam;
    drm_clip_rect_t *pSaamRects;
    int drawXSaam;
    int drawYSaam;
    GLuint numSaamRects;
    
    int drawPitch;
    int readPitch;
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
};
/*#define DMA_OFFSET 16*/
#define DMA_OFFSET 32
/*#define PERFORMANCE_MEASURE*/

extern GLuint VIA_PERFORMANCE;

#ifdef PERFORMANCE_MEASURE
#define HASH_TABLE_SIZE 1000
#define HASH_TABLE_DEPTH 10
typedef struct {
    char func[50];
    GLuint count;
} hash_element;
extern hash_element hash_table[HASH_TABLE_SIZE][HASH_TABLE_DEPTH];
#define P_M                                                                      \
    do {                                                                         \
	GLuint h_index,h_depth;                                                  \
	h_index = (GLuint)(((GLuint) __FUNCTION__)%HASH_TABLE_SIZE);             \
	for (h_depth = 0; h_depth < HASH_TABLE_DEPTH; h_depth++) {                \
	    if (!strcmp(hash_table[h_index][h_depth].func, "NULL")) {             \
		sprintf(hash_table[h_index][h_depth].func, "%s", __FUNCTION__);  \
		hash_table[h_index][h_depth].count++;                            \
		break;                                                           \
	    }                                                                    \
	    else if (!strcmp(hash_table[h_index][h_depth].func, __FUNCTION__)) {  \
		hash_table[h_index][h_depth].count++;                            \
		break;                                                           \
	    }                                                                    \
	}                                                                        \
    } while (0)

#define P_M_X                                                                    	\
    do {                                                                         	\
	GLuint h_index,h_depth;                                                  	\
	char str[80];                                                            	\
	strcpy(str, __FUNCTION__);                                               	\
	h_index = (GLuint)(((GLuint) __FUNCTION__)%HASH_TABLE_SIZE);             	\
	for (h_depth = 0; h_depth < HASH_TABLE_DEPTH; h_depth++) {                	\
	    if (!strcmp(hash_table[h_index][h_depth].func, "NULL")) {             	\
		sprintf(hash_table[h_index][h_depth].func, "%s_X", __FUNCTION__);  	\
		hash_table[h_index][h_depth].count++;                              	\
		break;                                                           	\
	    }                                                                    	\
	    else if (!strcmp(hash_table[h_index][h_depth].func, strcat(str, "_X"))) {  	\
		hash_table[h_index][h_depth].count++;                            	\
		break;                                                           	\
	    }                                                                    	\
	}                                                                        	\
    } while (0)

#define P_M_R                                                                                                                 	\
    do {                                                                                                                      	\
	GLuint h_size, h_depth;                                                                                               	\
	for (h_size = 0; h_size < HASH_TABLE_SIZE; h_size++) {                                                                 	\
	    for (h_depth = 0; h_depth < HASH_TABLE_DEPTH; h_depth++) {                                                         	\
		if (hash_table[h_size][h_depth].count) {                                                                       	\
		    fprintf(stderr, "func:%s count:%d\n", hash_table[h_size][h_depth].func, hash_table[h_size][h_depth].count); \
		}                                                                                                             	\
	    }                                                                                                                 	\
	}                                                                                                                     	\
    } while (0)
#else /* PERFORMANCE_MEASURE */
#define P_M {}
#define P_M_X {}
#define P_M_R {}
#endif

#define VIA_CONTEXT(ctx)   ((viaContextPtr)(ctx->DriverCtx))

#define GET_DISPATCH_AGE(vmesa) vmesa->sarea->lastDispatch
#define GET_ENQUEUE_AGE(vmesa) vmesa->sarea->lastEnqueue


/* Lock the hardware and validate our state.  
 */
/*
#define LOCK_HARDWARE(vmesa)                                \
    do {                                                    \
        char __ret = 0;                                     \
        DRM_CAS(vmesa->driHwLock, vmesa->hHWContext,        \
            (DRM_LOCK_HELD|vmesa->hHWContext), __ret);      \
        if (__ret)                                          \
            viaGetLock(vmesa, 0);                           \
    } while (0)
*/
/*=* John Sheng [2003.6.20] fix pci *=*/
/*=* John Sheng [2003.7.25] fix viewperf black shadow *=*/
#define LOCK_HARDWARE(vmesa)                                	\
    if(1)                                         	\
	do {                                                    \
    	    char __ret = 0;                                     \
    	    DRM_CAS(vmesa->driHwLock, vmesa->hHWContext,        \
        	(DRM_LOCK_HELD|vmesa->hHWContext), __ret);      \
    	    if (__ret)                                          \
        	viaGetLock(vmesa, 0);                           \
	} while (0);                                            \
    else                                                        \
	viaLock(vmesa, 0)
	

/*
#define LOCK_HARDWARE(vmesa)                                	\
    viaLock(vmesa, 0);                           
*/

/* Release the kernel lock.
 */
/*
#define UNLOCK_HARDWARE(vmesa)                                  \
    DRM_UNLOCK(vmesa->driFd, vmesa->driHwLock, vmesa->hHWContext);
*/
/*=* John Sheng [2003.6.20] fix pci *=*/
/*=* John Sheng [2003.7.25] fix viewperf black shadow *=*/
#define UNLOCK_HARDWARE(vmesa)                                  	\
    if(1)                                         		\
	DRM_UNLOCK(vmesa->driFd, vmesa->driHwLock, vmesa->hHWContext);	\
    else								\
	viaUnLock(vmesa, 0);				
/*	
#define UNLOCK_HARDWARE(vmesa)                             	\
    viaUnLock(vmesa, 0);
*/
#define WAIT_IDLE                                                             	\
    while (1) {                                                            	\
	if ((((GLuint)*vmesa->regEngineStatus) & 0xFFFEFFFF) == 0x00020000)	\
	    break;                                                        	\
    }
	
#define LOCK_HARDWARE_QUIESCENT(vmesa)          \
    do {                                        \
        LOCK_HARDWARE(vmesa);                   \
        viaRegetLockQuiescent(vmesa);           \
    } while (0)


#ifdef DEBUG
extern GLuint VIA_DEBUG;
#else
#define VIA_DEBUG 0
#endif


extern GLuint DRAW_FRONT;
extern void viaGetLock(viaContextPtr vmesa, GLuint flags);
extern void viaLock(viaContextPtr vmesa, GLuint flags);
extern void viaUnLock(viaContextPtr vmesa, GLuint flags);
extern void viaEmitHwStateLocked(viaContextPtr vmesa);
extern void viaEmitScissorValues(viaContextPtr vmesa, int box_nr, int emit);
extern void viaEmitDrawingRectangle(viaContextPtr vmesa);
extern void viaXMesaSetBackClipRects(viaContextPtr vmesa);
extern void viaXMesaSetFrontClipRects(viaContextPtr vmesa);
extern void viaReAllocateBuffers(GLframebuffer *drawbuffer);
extern void viaXMesaWindowMoved(viaContextPtr vmesa);

extern void viaTexCombineState(viaContextPtr vmesa,
    const struct gl_tex_env_combine_state * combine, unsigned unit );

#define SUBPIXEL_X -.5
#define SUBPIXEL_Y -.5

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
