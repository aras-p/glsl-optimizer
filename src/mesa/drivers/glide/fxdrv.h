/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


#ifndef FXDRV_H
#define FXDRV_H

/* If you comment out this define, a variable takes its place, letting
 * you turn debugging on/off from the debugger.
 */

#ifndef XFree86Server
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#else 
#include "GL/xf86glx.h"
#endif


#if defined(__linux__)
#include <signal.h>
#endif

#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "texture.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#include "clip.h"
#include "vbrender.h"

#ifdef XF86DRI
typedef struct tfxMesaContext *fxMesaContext;
#else
#include "GL/fxmesa.h"
#endif
#include "fxglidew.h"
/* use gl/gl.h GLAPI/GLAPIENTRY/GLCALLBACK in place of WINGDIAPI/APIENTRY/CALLBACK, */
/* these are defined in mesa gl/gl.h - tjump@spgs.com */



extern void fx_sanity_triangle( GrVertex *, GrVertex *, GrVertex * );
#if defined(MESA_DEBUG) && 0
#define grDrawTriangle fx_sanity_triangle
#endif


/* Define some shorter names for these things.
 */
#define XCOORD   GR_VERTEX_X_OFFSET
#define YCOORD   GR_VERTEX_Y_OFFSET
#define ZCOORD   GR_VERTEX_OOZ_OFFSET
#define OOWCOORD GR_VERTEX_OOW_OFFSET

#define RCOORD   GR_VERTEX_R_OFFSET
#define GCOORD   GR_VERTEX_G_OFFSET
#define BCOORD   GR_VERTEX_B_OFFSET
#define ACOORD   GR_VERTEX_A_OFFSET

#define S0COORD  GR_VERTEX_SOW_TMU0_OFFSET
#define T0COORD  GR_VERTEX_TOW_TMU0_OFFSET
#define S1COORD  GR_VERTEX_SOW_TMU1_OFFSET
#define T1COORD  GR_VERTEX_TOW_TMU1_OFFSET


#if FX_USE_PARGB

#define CLIP_XCOORD 0		/* normal place */
#define CLIP_YCOROD 1		/* normal place */
#define CLIP_ZCOORD 2		/* normal place */
#define CLIP_WCOORD 3		/* normal place */
#define CLIP_GCOORD 4		/* GR_VERTEX_PARGB_OFFSET */
#define CLIP_BCOORD 5		/* GR_VERTEX_SOW_TMU0_OFFSET */
#define CLIP_RCOORD 6		/* GR_VERTEX_TOW_TMU0_OFFSET */
#define CLIP_ACOORD 7		/* GR_VERTEX_OOW_TMU0_OFFSET */

#else

#define CLIP_XCOORD 0		/* normal place */
#define CLIP_YCOROD 1		/* normal place */
#define CLIP_ZCOORD 2		/* GR_VERTEX_Z_OFFSET */
#define CLIP_WCOORD 3		/* GR_VERTEX_R_OFFSET */
#define CLIP_GCOORD 4		/* normal place */
#define CLIP_BCOORD 5		/* normal place */
#define CLIP_RCOORD 6		/* GR_VERTEX_OOZ_OFFSET */
#define CLIP_ACOORD 7		/* normal place */


#endif

/* Should have size == 16 * sizeof(float).
 */
typedef struct {
   GLfloat f[15];		/* Same layout as GrVertex */
   GLubyte mask;		/* Unsued  */
   GLubyte usermask;		/* Unused */
} fxVertex;




#if defined(FXMESA_USE_ARGB)
#define FXCOLOR4( c ) (      \
  ( ((unsigned int)(c[3]))<<24 ) | \
  ( ((unsigned int)(c[0]))<<16 ) | \
  ( ((unsigned int)(c[1]))<<8 )  | \
  (  (unsigned int)(c[2])) )
  
#else
#ifdef __i386__
#define FXCOLOR4( c )  (* (int *)c)
#else
#define FXCOLOR4( c ) (      \
  ( ((unsigned int)(c[3]))<<24 ) | \
  ( ((unsigned int)(c[2]))<<16 ) | \
  ( ((unsigned int)(c[1]))<<8 )  | \
  (  (unsigned int)(c[0])) )
#endif
#endif

#define FX_VB_COLOR(fxm, color)				\
  do {							\
    if (sizeof(GLint) == 4*sizeof(GLubyte)) {		\
      if (fxm->constColor != *(GLuint*)color) {		\
	fxm->constColor = *(GLuint*)color;		\
	FX_grConstantColorValue(FXCOLOR4(color));	\
      }							\
    } else {						\
      FX_grConstantColorValue(FXCOLOR4(color));		\
    }							\
  } while (0)

#define GOURAUD(x) {					\
  GLubyte *col = VB->ColorPtr->data[(x)];		\
  gWin[(x)].v.r=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[0]);	\
  gWin[(x)].v.g=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[1]);	\
  gWin[(x)].v.b=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[2]);	\
  gWin[(x)].v.a=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[3]);	\
}

#if FX_USE_PARGB
#define GOURAUD2(v, c) {																\
  GLubyte *col = c;  																	\
  v->argb=MESACOLOR2PARGB(col);															\
}
#else
#define GOURAUD2(v, c) {			\
  GLubyte *col = c;  				\
  v->r=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[0]);	\
  v->g=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[1]);	\
  v->b=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[2]);	\
  v->a=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[3]);	\
}
#endif


/* Mergable items first
 */
#define SETUP_RGBA 0x1
#define SETUP_TMU0 0x2
#define SETUP_TMU1 0x4
#define SETUP_XY   0x8
#define SETUP_Z    0x10
#define SETUP_W    0x20

#define MAX_MERGABLE 0x8


#define FX_NUM_TMU 2

#define FX_TMU0      GR_TMU0
#define FX_TMU1      GR_TMU1
#define FX_TMU_SPLIT 98
#define FX_TMU_BOTH  99
#define FX_TMU_NONE  100

/* Used for fxMesa->lastUnitsMode */

#define FX_UM_NONE                  0x00000000

#define FX_UM_E0_REPLACE            0x00000001
#define FX_UM_E0_MODULATE           0x00000002
#define FX_UM_E0_DECAL              0x00000004
#define FX_UM_E0_BLEND              0x00000008
#define FX_UM_E0_ADD				0x00000010

#define FX_UM_E1_REPLACE            0x00000020
#define FX_UM_E1_MODULATE           0x00000040
#define FX_UM_E1_DECAL              0x00000080
#define FX_UM_E1_BLEND              0x00000100
#define FX_UM_E1_ADD				0x00000200

#define FX_UM_E_ENVMODE             0x000003ff

#define FX_UM_E0_ALPHA              0x00001000
#define FX_UM_E0_LUMINANCE          0x00002000
#define FX_UM_E0_LUMINANCE_ALPHA    0x00004000
#define FX_UM_E0_INTENSITY          0x00008000
#define FX_UM_E0_RGB                0x00010000
#define FX_UM_E0_RGBA               0x00020000

#define FX_UM_E1_ALPHA              0x00040000
#define FX_UM_E1_LUMINANCE          0x00080000
#define FX_UM_E1_LUMINANCE_ALPHA    0x00100000
#define FX_UM_E1_INTENSITY          0x00200000
#define FX_UM_E1_RGB                0x00400000
#define FX_UM_E1_RGBA               0x00800000

#define FX_UM_E_IFMT                0x00fff000

#define FX_UM_COLOR_ITERATED        0x01000000
#define FX_UM_COLOR_CONSTANT        0x02000000
#define FX_UM_ALPHA_ITERATED        0x04000000
#define FX_UM_ALPHA_CONSTANT        0x08000000

typedef void (*tfxRenderVBFunc)(GLcontext *);

/*
  Memory range from startAddr to endAddr-1
*/
typedef struct MemRange_t {
  struct MemRange_t *next;
  FxU32 startAddr, endAddr;
} MemRange;

typedef struct {
  GLsizei width, height;              /* image size */
  GrTextureFormat_t glideFormat;      /* Glide image format */
  unsigned short *data;               /* Glide-formated texture image */
} tfxMipMapLevel;

typedef struct tfxTexInfo_t {
  struct tfxTexInfo *next;
  struct gl_texture_object *tObj;

  GLuint lastTimeUsed;
  FxU32 whichTMU;
  GLboolean isInTM;

  tfxMipMapLevel mipmapLevel[MAX_TEXTURE_LEVELS];

  MemRange *tm[FX_NUM_TMU];

  GLint minLevel, maxLevel;
  GLint baseLevelInternalFormat;

  GrTexInfo info;

  GrTextureFilterMode_t minFilt;
  GrTextureFilterMode_t maxFilt;
  FxBool LODblend;

  GrTextureClampMode_t sClamp;
  GrTextureClampMode_t tClamp;

  GrMipMapMode_t mmMode;

  GLfloat sScale, tScale;
  GLint int_sScale, int_tScale;	/* x86 floating point trick for
				 * multiplication by powers of 2.  
				 * Used in fxfasttmp.h
				 */

  GuTexPalette palette;

  GLboolean fixedPalette;
  GLboolean validated;
} tfxTexInfo;

typedef struct {
  GLuint swapBuffer;
  GLuint reqTexUpload;
  GLuint texUpload;
  GLuint memTexUpload;
} tfxStats;


typedef void (*tfxTriViewClipFunc)( struct vertex_buffer *VB, 
				    GLuint v[],
				    GLubyte mask );

typedef void (*tfxTriClipFunc)( struct vertex_buffer *VB, 
				GLuint v[],
				GLuint mask );


typedef void (*tfxLineClipFunc)( struct vertex_buffer *VB, 
				 GLuint v1, GLuint v2,
				 GLubyte mask );


extern tfxTriViewClipFunc fxTriViewClipTab[0x8];
extern tfxTriClipFunc fxTriClipStrideTab[0x8];
extern tfxLineClipFunc fxLineClipTab[0x8];

typedef struct {
  /* Alpha test */

  GLboolean alphaTestEnabled;
  GrCmpFnc_t alphaTestFunc;
  GrAlpha_t alphaTestRefValue;

  /* Blend function */

  GLboolean blendEnabled;
  GrAlphaBlendFnc_t blendSrcFuncRGB;
  GrAlphaBlendFnc_t blendDstFuncRGB;
  GrAlphaBlendFnc_t blendSrcFuncAlpha;
  GrAlphaBlendFnc_t blendDstFuncAlpha;

  /* Depth test */

  GLboolean depthTestEnabled;
  GLboolean depthMask;
  GrCmpFnc_t depthTestFunc;
} tfxUnitsState;


/* Flags for render_index.
 */
#define FX_OFFSET             0x1
#define FX_TWOSIDE            0x2
#define FX_FRONT_BACK         0x4 
#define FX_FLAT               0x8
#define FX_ANTIALIAS          0x10 
#define FX_FALLBACK           0x20 


/* Flags for fxMesa->new_state
 */
#define FX_NEW_TEXTURING      0x1
#define FX_NEW_BLEND          0x2
#define FX_NEW_ALPHA          0x4
#define FX_NEW_DEPTH          0x8
#define FX_NEW_FOG            0x10
#define FX_NEW_SCISSOR        0x20
#define FX_NEW_COLOR_MASK     0x40
#define FX_NEW_CULL           0x80

/* FX struct stored in VB->driver_data.
 */
struct tfxMesaVertexBuffer {
   GLvector1ui clipped_elements;

   fxVertex *verts;
   fxVertex *last_vert;
   void *vert_store;
#if defined(FX_GLIDE3)
   GrVertex **triangle_b;	/* Triangle buffer */
   GrVertex **strips_b;		/* Strips buffer */
#endif

   GLuint size;
};

#define FX_DRIVER_DATA(vb) ((struct tfxMesaVertexBuffer *)((vb)->driver_data))
#define FX_CONTEXT(ctx) ((fxMesaContext)((ctx)->DriverCtx))
#define FX_TEXTURE_DATA(t) fxTMGetTexInfo((t)->Current)

#if defined(XFree86Server) || defined(GLX_DIRECT_RENDERING)
#include "tdfx_init.h"
#else
#define DRI_FX_CONTEXT
#define BEGIN_BOARD_LOCK()
#define END_BOARD_LOCK()
#define BEGIN_CLIP_LOOP()
#define END_CLIP_LOOP()
#endif


/* These lookup table are used to extract RGB values in [0,255] from
 * 16-bit pixel values.
 */
extern GLubyte FX_PixelToR[0x10000];
extern GLubyte FX_PixelToG[0x10000];
extern GLubyte FX_PixelToB[0x10000];


struct tfxMesaContext {
  GuTexPalette glbPalette;

  GLcontext *glCtx;              /* the core Mesa context */
  GLvisual *glVis;               /* describes the color buffer */
  GLframebuffer *glBuffer;       /* the ancillary buffers */

  GLint board;                   /* the board used for this context */
  GLint width, height;           /* size of color buffer */

  GrBuffer_t currentFB;

  GLboolean bgrOrder;
  GrColor_t color;
  GrColor_t clearC;
  GrAlpha_t clearA;
  GLuint constColor;
  GrCullMode_t cullMode;

  tfxUnitsState unitsState;
  tfxUnitsState restoreUnitsState; /* saved during multipass */

  GLuint tmu_source[FX_NUM_TMU];
  GLuint tex_dest[MAX_TEXTURE_UNITS];
  GLuint setupindex;
  GLuint partial_setup_index;
  GLuint setupdone;
  GLuint mergeindex;
  GLuint mergeinputs;
  GLuint render_index;
  GLuint last_tri_caps;
  GLuint stw_hint_state;		/* for grHints */
  GLuint is_in_hardware;
  GLuint new_state;   
  GLuint using_fast_path, passes, multipass;

  tfxLineClipFunc clip_line;
  tfxTriClipFunc clip_tri_stride;
  tfxTriViewClipFunc view_clip_tri;


  /* Texture Memory Manager Data */

  GLuint texBindNumber;
  GLint tmuSrc;
  GLuint lastUnitsMode;
  GLuint freeTexMem[FX_NUM_TMU];
  MemRange *tmPool;
  MemRange *tmFree[FX_NUM_TMU];

  GLenum fogTableMode;
  GLfloat fogDensity;
  GLfloat fogStart, fogEnd;
  GrFog_t *fogTable;
  GLint textureAlign;

  /* Acc. functions */

  points_func PointsFunc;
  line_func LineFunc;
  triangle_func TriangleFunc;
  quad_func QuadFunc;

  render_func **RenderVBTables;

  render_func *RenderVBClippedTab;
  render_func *RenderVBCulledTab;
  render_func *RenderVBRawTab;


  tfxStats stats;

  void *state;

  /* Options */

  GLboolean verbose;
  GLboolean haveTwoTMUs;	/* True if we really have 2 tmu's  */
  GLboolean emulateTwoTMUs;	/* True if we present 2 tmu's to mesa.  */
  GLboolean haveAlphaBuffer;
  GLboolean haveZBuffer;
  GLboolean haveDoubleBuffer;
  GLboolean haveGlobalPaletteTexture;
  GLint swapInterval;
  GLint maxPendingSwapBuffers;
  
  FX_GrContext_t glideContext;

  int x_offset;
  int y_offset;
  int y_delta;
  int screen_width;
  int screen_height;
  int initDone;
  int clipMinX;
  int clipMaxX;
  int clipMinY;
  int clipMaxY;
  int needClip;

  DRI_FX_CONTEXT
};

typedef void (*tfxSetupFunc)(struct vertex_buffer *, GLuint, GLuint);

extern GrHwConfiguration glbHWConfig;
extern int glbCurrentBoard;

extern void fxPrintSetupFlags( const char *msg, GLuint flags );
extern void fxSetupFXUnits(GLcontext *);
extern void fxSetupDDPointers(GLcontext *);
extern void fxDDSetNearFar(GLcontext *, GLfloat, GLfloat);

extern void fxDDSetupInit(void);
extern void fxDDCvaInit(void);
extern void fxDDTrifuncInit(void);
extern void fxDDFastPathInit(void);

extern void fxDDChooseRenderState( GLcontext *ctx );

extern void fxRenderClippedLine( struct vertex_buffer *VB, 
				 GLuint v1, GLuint v2 );

extern void fxRenderClippedTriangle( struct vertex_buffer *VB,
				     GLuint n, GLuint vlist[] );


extern tfxSetupFunc fxDDChooseSetupFunction(GLcontext *);

extern points_func fxDDChoosePointsFunction(GLcontext *);
extern line_func fxDDChooseLineFunction(GLcontext *);
extern triangle_func fxDDChooseTriangleFunction(GLcontext *);
extern quad_func fxDDChooseQuadFunction(GLcontext *);
extern render_func **fxDDChooseRenderVBTables(GLcontext *);

extern void fxDDRenderInit(GLcontext *);
extern void fxDDClipInit(void);

extern void fxUpdateDDSpanPointers(GLcontext *);
extern void fxSetupDDSpanPointers(GLcontext *);

extern void fxPrintTextureData(tfxTexInfo *ti);
extern void fxDDTexImg(GLcontext *, GLenum, struct gl_texture_object *,
		       GLint, GLint, const struct gl_texture_image *);
extern void fxDDTexSubImg(GLcontext *, GLenum, struct gl_texture_object *,
                          GLint, GLint, GLint, GLint, GLint, GLint,
                          const struct gl_texture_image *);
extern GLboolean fxDDTexImage2D(GLcontext *ctx, GLenum target, GLint level,
                              GLenum format, GLenum type, const GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage,
                              GLboolean *retainInternalCopy);
extern GLboolean fxDDTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type, const GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage);
extern void fxDDTexEnv(GLcontext *, GLenum, GLenum, const GLfloat *);
extern void fxDDTexParam(GLcontext *, GLenum, struct gl_texture_object *,
			 GLenum, const GLfloat *);
extern void fxDDTexBind(GLcontext *, GLenum, struct gl_texture_object *);
extern void fxDDTexDel(GLcontext *, struct gl_texture_object *);
extern void fxDDTexPalette(GLcontext *, struct gl_texture_object *);
extern void fxDDTexUseGlbPalette(GLcontext *, GLboolean);

extern void fxDDEnable(GLcontext *, GLenum, GLboolean);
extern void fxDDAlphaFunc(GLcontext *, GLenum, GLclampf);
extern void fxDDBlendFunc(GLcontext *, GLenum, GLenum);
extern void fxDDDepthMask(GLcontext *, GLboolean);
extern void fxDDDepthFunc(GLcontext *, GLenum);

extern void fxDDRegisterVB( struct vertex_buffer *VB );
extern void fxDDUnregisterVB( struct vertex_buffer *VB );
extern void fxDDResizeVB( struct vertex_buffer *VB, GLuint size );

extern void fxDDCheckMergeAndRender( GLcontext *ctx, 
				     struct gl_pipeline_stage *d );

extern void fxDDMergeAndRender( struct vertex_buffer *VB );

extern void fxDDCheckPartialRasterSetup( GLcontext *ctx, 
					 struct gl_pipeline_stage *d );

extern void fxDDPartialRasterSetup( struct vertex_buffer *VB );

extern void fxDDDoRasterSetup( struct vertex_buffer *VB );

extern GLuint fxDDRegisterPipelineStages( struct gl_pipeline_stage *out,
					  const struct gl_pipeline_stage *in,
					  GLuint nr );

extern GLboolean fxDDBuildPrecalcPipeline( GLcontext *ctx );

extern void fxDDOptimizePrecalcPipeline( GLcontext *ctx, 
					 struct gl_pipeline *pipe );

extern void fxDDRenderElementsDirect( struct vertex_buffer *VB );
extern void fxDDRenderVBIndirectDirect( struct vertex_buffer *VB );

extern void fxDDInitExtensions( GLcontext *ctx );

#define fxTMGetTexInfo(o) ((tfxTexInfo*)((o)->DriverData))
extern void fxTMInit(fxMesaContext ctx);
extern void fxTMClose(fxMesaContext ctx);
extern void fxTMRestoreTextures_NoLock(fxMesaContext ctx);
extern void fxTMMoveInTM(fxMesaContext, struct gl_texture_object *, GLint);
extern void fxTMMoveOutTM(fxMesaContext, struct gl_texture_object *);
#define fxTMMoveOutTM_NoLock fxTMMoveOutTM
extern void fxTMFreeTexture(fxMesaContext, struct gl_texture_object *);
extern void fxTMReloadMipMapLevel(fxMesaContext, struct gl_texture_object *, GLint);
extern void fxTMReloadSubMipMapLevel(fxMesaContext, struct gl_texture_object *,
				     GLint, GLint, GLint);

extern void fxTexGetFormat(GLenum, GrTextureFormat_t *, GLint *);
extern int fxTexGetInfo(int, int, GrLOD_t *, GrAspectRatio_t *,
			float *, float *, int *, int *, int *, int *);

extern void fxDDScissor( GLcontext *ctx,
			      GLint x, GLint y, GLsizei w, GLsizei h );
extern void fxDDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *params );
extern GLboolean fxDDColorMask(GLcontext *ctx, 
			       GLboolean r, GLboolean g, 
			       GLboolean b, GLboolean a );

extern void fxDDWriteDepthSpan(GLcontext *ctx, GLuint n, GLint x, GLint y,
                               const GLdepth depth[], const GLubyte mask[]);

extern void fxDDReadDepthSpan(GLcontext *ctx, GLuint n, GLint x, GLint y,
                              GLdepth depth[]);

extern void fxDDWriteDepthPixels(GLcontext *ctx, GLuint n,
                                 const GLint x[], const GLint y[],
                                 const GLdepth depth[], const GLubyte mask[]);

extern void fxDDReadDepthPixels(GLcontext *ctx, GLuint n,
                                const GLint x[], const GLint y[],
                                GLdepth depth[]);

extern void fxDDFastPath( struct vertex_buffer *VB );

extern void fxDDShadeModel(GLcontext *ctx, GLenum mode);

extern void fxDDCullFace(GLcontext *ctx, GLenum mode);
extern void fxDDFrontFace(GLcontext *ctx, GLenum mode);

extern void fxPrintRenderState( const char *msg, GLuint state );
extern void fxPrintHintState( const char *msg, GLuint state );

extern void fxDDDoRenderVB( struct vertex_buffer *VB );

extern int fxDDInitFxMesaContext( fxMesaContext fxMesa );


extern void fxSetScissorValues(GLcontext *ctx);
extern void fxTMMoveInTM_NoLock(fxMesaContext fxMesa, 
				struct gl_texture_object *tObj, 
				GLint where);
extern void fxInitPixelTables(fxMesaContext fxMesa, GLboolean bgrOrder);

#endif
