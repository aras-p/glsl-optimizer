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



#ifndef SAVAGECONTEXT_INC
#define SAVAGECONTEXT_INC

typedef struct savage_context_t savageContext;
typedef struct savage_context_t *savageContextPtr;
typedef struct savage_texture_object_t *savageTextureObjectPtr;

#include <X11/Xlibint.h>
#include "dri_util.h"
#include "mtypes.h"
#include "xf86drm.h"
#include "drm.h"
#include "savage_drm.h"
#include "savage_init.h"
#include "mm.h"
#include "tnl/t_vertex.h"

#include "savagetex.h"
#include "savagedma.h"

#include "xmlconfig.h"

/* Reasons to fallback on all primitives.
 */
#define SAVAGE_FALLBACK_TEXTURE        0x1
#define SAVAGE_FALLBACK_DRAW_BUFFER    0x2
#define SAVAGE_FALLBACK_READ_BUFFER    0x4
#define SAVAGE_FALLBACK_COLORMASK      0x8  
#define SAVAGE_FALLBACK_SPECULAR       0x10 
#define SAVAGE_FALLBACK_LOGICOP        0x20
/*frank 2001/11/12 add the stencil fallbak*/
#define SAVAGE_FALLBACK_STENCIL        0x40
#define SAVAGE_FALLBACK_RENDERMODE     0x80
#define SAVAGE_FALLBACK_BLEND_EQ       0x100
#define SAVAGE_FALLBACK_NORAST         0x200
#define SAVAGE_FALLBACK_PROJ_TEXTURE   0x400


#define HW_CULL    1

/* for savagectx.new_state - manage GL->driver state changes
 */
#define SAVAGE_NEW_TEXTURE 0x1
#define SAVAGE_NEW_CULL    0x2


/*define the max numer of vertex in vertex buf*/
#define SAVAGE_MAX_VERTEXS 0x10000

/* Use the templated vertex formats:
 */
#define TAG(x) savage##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*savage_tri_func)( savageContextPtr, savageVertex *,
				 savageVertex *, savageVertex * );
typedef void (*savage_line_func)( savageContextPtr,
				  savageVertex *, savageVertex * );
typedef void (*savage_point_func)( savageContextPtr, savageVertex * );


/**************************************************************
 ****************    enums for chip IDs ************************
 **************************************************************/

#define CHIP_S3GX3MS1NB             0x8A25
#define CHIP_S3GX3MS1NBK            0x8A26
#define CHIP_S3TWISTER              0x8D01
#define CHIP_S3TWISTERK             0x8D02
#define CHIP_S3TWISTER_P4M          0x8D04
#define CHIP_S3PARAMOUNT128         0x8C22              /*SuperSavage 128/MX*/
#define CHIP_S3TRISTAR128SDR        0x8C2A              /*SuperSavage 128/IX*/
#define CHIP_S3TRISTAR64SDRM7       0x8C2C              /*SuperSavage/IX M7 Package*/
#define CHIP_S3TRISTAR64SDR         0x8C2E              /*SuperSavage/IX*/
#define CHIP_S3TRISTAR64CDDR        0x8C2F              /*SuperSavage/IXC DDR*/

#define IS_SAVAGE(imesa) (imesa->savageScreen->deviceID == CHIP_S3GX3MS1NB ||	\
			imesa->savageScreen->deviceID == CHIP_S3GX3MS1NBK || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTER || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTERK || \
                        imesa->savageScreen->deviceID == CHIP_S3TWISTER_P4M || \
                        imesa->savageScreen->deviceID == CHIP_S3PARAMOUNT128 || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR128SDR || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR64SDRM7 || \
                        imesa->savageScreen->deviceID == CHIP_S3TRISTAR64SDR || \
			imesa->savageScreen->deviceID == CHIP_S3TRISTAR64CDDR )




struct savage_context_t {
    GLint refcount;

    GLcontext *glCtx;

    int lastTexHeap;
    savageTextureObjectPtr CurrentTexObj[2];
   
    struct savage_texture_object_t TexObjList[SAVAGE_NR_TEX_HEAPS];
    struct savage_texture_object_t SwappedOut; 
  
    GLuint c_texupload;
    GLuint c_texusage;
    GLuint tex_thrash;
   
    GLuint TextureMode;
   
    
    /* Hardware state
     */

    savageRegisters regs, oldRegs, globalRegMask;

    /* Manage our own state */
    GLuint new_state; 
    GLuint new_gl_state;
    GLboolean ptexHack;

    GLuint BCIBase;  
    GLuint MMIO_BASE;

    /* DMA command buffer */
    DMABuffer_t DMABuf;

    /* aperture base */
    GLuint apertureBase[5];
    GLuint aperturePitch;
    /* Manage hardware state */
    GLuint dirty;
    GLboolean lostContext;
    memHeap_t *texHeap[SAVAGE_NR_TEX_HEAPS];
    GLuint bTexEn1;
    /* One of the few bits of hardware state that can't be calculated
     * completely on the fly:
     */
    GLuint LcsCullMode;

   /* Vertex state 
    */
   GLuint vertex_size;
   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;
   char *verts;			/* points to tnl->clipspace.vertex_buf */

   /* Rasterization state 
    */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   
   GLuint hw_primitive;
   GLenum raster_primitive;
   GLenum render_primitive;

   GLuint DrawPrimitiveCmd;
   GLuint HwVertexSize;

   /* Fallback rasterization functions 
    */
   savage_point_func draw_point;
   savage_line_func draw_line;
   savage_tri_func draw_tri;

    /* Funny mesa mirrors
     */
    GLuint MonoColor;
    GLuint ClearColor;
    GLfloat depth_scale;
    GLfloat hw_viewport[16];
    /* DRI stuff */  
    drmBufPtr  vertex_dma_buffer;

    GLframebuffer *glBuffer;
   
    /* Two flags to keep track of fallbacks. */
    GLuint Fallback;

    GLuint needClip;

    /* These refer to the current draw (front vs. back) buffer:
     */
    char *drawMap;		/* draw buffer address in virtual mem */
    char *readMap;	
    int drawX;   		/* origin of drawable in draw buffer */
    int drawY;
    GLuint numClipRects;		/* cliprects for that buffer */
    GLint currentClip;
    drm_clip_rect_t *pClipRects;

    /*  use this bit to support single/double buffer */
    GLuint IsDouble;
    /*  use this to indicate Fullscreen mode */   
    GLuint IsFullScreen; /* FIXME - open/close fullscreen is gone, is this needed? */
    GLuint backup_frontOffset;
    GLuint backup_backOffset;
    GLuint backup_frontBitmapDesc;
    GLuint toggle;
    GLuint backup_streamFIFO;
    GLuint NotFirstFrame;
   
    GLuint lastSwap;
    GLuint secondLastSwap;
    GLuint ctxAge;
    GLuint dirtyAge;
    GLuint any_contend;		/* throttle me harder */

    GLuint scissor;
    GLboolean scissorChanged;
    drm_clip_rect_t draw_rect;
    drm_clip_rect_t scissor_rect;
    drm_clip_rect_t tmp_boxes[2][SAVAGE_NR_SAREA_CLIPRECTS];
    /*Texture aging and DMA based aging*/
    unsigned int texAge[SAVAGE_NR_TEX_HEAPS]; 

    drm_context_t hHWContext;
    drm_hw_lock_t *driHwLock;
    GLuint driFd;

    __DRIdrawablePrivate *driDrawable;
    __DRIdrawablePrivate *driReadable;

    /**
     * Drawable used by Mesa for software fallbacks for reading and
     * writing.  It is set by Mesa's \c SetBuffer callback, and will always be
     * either \c mga_context_t::driDrawable or \c mga_context_t::driReadable.
     */
    __DRIdrawablePrivate *mesa_drawable;

    __DRIscreenPrivate *driScreen;
    savageScreenPrivate *savageScreen; 
    drm_savage_sarea_t *sarea;

    GLboolean hw_stencil;

    /*shadow pointer*/
    volatile GLuint  *shadowPointer;
    volatile GLuint *eventTag1;
    GLuint shadowCounter;
    GLboolean shadowStatus;

    /* Configuration cache
     */
    driOptionCache optionCache;
    int texture_depth;
};

#define SAVAGE_CONTEXT(ctx) ((savageContextPtr)(ctx->DriverCtx))

/* To remove all debugging, make sure SAVAGE_DEBUG is defined as a
 * preprocessor symbol, and equal to zero.  
 */
#ifndef SAVAGE_DEBUG
extern int SAVAGE_DEBUG;
#endif

#define DEBUG_FALLBACKS      0x001
#define DEBUG_VERBOSE_API    0x002
#define DEBUG_VERBOSE_LRU    0x004

#define TARGET_FRONT    0x0
#define TARGET_BACK     0x1
#define TARGET_DEPTH    0x2

#define SUBPIXEL_X -0.5
#define SUBPIXEL_Y -0.375

#endif
