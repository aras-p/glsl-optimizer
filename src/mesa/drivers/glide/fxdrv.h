/* -*- mode: C; tab-width:8;  -*-

             fxdrv.h - 3Dfx VooDoo driver types
*/

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * See the file fxapi.c for more informations about authors
 *
 */

#ifndef FXDRV_H
#define FXDRV_H

/* If you comment out this define, a variable takes its place, letting
 * you turn debugging on/off from the debugger.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#if defined(__linux__)
#include <signal.h>
#endif

#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "texture.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#include "clip.h"
#include "vbrender.h"

#include "GL/fxmesa.h"
#include "fxglidew.h"
/* use gl/gl.h GLAPI/GLAPIENTRY/GLCALLBACK in place of WINGDIAPI/APIENTRY/CALLBACK, */
/* these are defined in mesa gl/gl.h - tjump@spgs.com */



#if defined(MESA_DEBUG) && 0
extern void fx_sanity_triangle( GrVertex *, GrVertex *, GrVertex * );
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

#define CLIP_XCOORD 0		/* normal place */
#define CLIP_YCOROD 1		/* normal place */
#define CLIP_ZCOORD 2		/* GR_VERTEX_Z_OFFSET */
#define CLIP_WCOORD 3		/* GR_VERTEX_R_OFFSET */
#define CLIP_GCOORD 4		/* normal place */
#define CLIP_BCOORD 5		/* normal place */
#define CLIP_RCOORD 6		/* GR_VERTEX_OOZ_OFFSET */
#define CLIP_ACOORD 7		/* normal place */




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

#define FX_VB_COLOR(fxm, color)			\
do {						\
  if (sizeof(GLint) == 4*sizeof(GLubyte)) {	\
     if (fxm->constColor != *(GLuint*)color) {	\
	fxm->constColor = *(GLuint*)color;	\
	grConstantColorValue(FXCOLOR4(color));	\
     }						\
  } else {					\
     grConstantColorValue(FXCOLOR4(color));	\
  }						\
} while (0)

#define GOURAUD(x) {					\
  GLubyte *col = VB->ColorPtr->data[(x)];		\
  gWin[(x)].v.r=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[0]);	\
  gWin[(x)].v.g=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[1]);	\
  gWin[(x)].v.b=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[2]);	\
  gWin[(x)].v.a=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[3]);	\
}

#define GOURAUD2(v, c) {			\
  GLubyte *col = c;  				\
  v->r=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[0]);	\
  v->g=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[1]);	\
  v->b=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[2]);	\
  v->a=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[3]);	\
}


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

#define FX_UM_E1_REPLACE            0x00000010
#define FX_UM_E1_MODULATE           0x00000020
#define FX_UM_E1_DECAL              0x00000040
#define FX_UM_E1_BLEND              0x00000080

#define FX_UM_E_ENVMODE             0x000000ff

#define FX_UM_E0_ALPHA              0x00000100
#define FX_UM_E0_LUMINANCE          0x00000200
#define FX_UM_E0_LUMINANCE_ALPHA    0x00000400
#define FX_UM_E0_INTENSITY          0x00000800
#define FX_UM_E0_RGB                0x00001000
#define FX_UM_E0_RGBA               0x00002000

#define FX_UM_E1_ALPHA              0x00004000
#define FX_UM_E1_LUMINANCE          0x00008000
#define FX_UM_E1_LUMINANCE_ALPHA    0x00010000
#define FX_UM_E1_INTENSITY          0x00020000
#define FX_UM_E1_RGB                0x00040000
#define FX_UM_E1_RGBA               0x00080000

#define FX_UM_E_IFMT                0x000fff00

#define FX_UM_COLOR_ITERATED        0x00100000
#define FX_UM_COLOR_CONSTANT        0x00200000
#define FX_UM_ALPHA_ITERATED        0x00400000
#define FX_UM_ALPHA_CONSTANT        0x00800000

typedef void (*tfxRenderVBFunc)(GLcontext *);

typedef struct tfxTMFreeListNode {
  struct tfxTMFreeListNode *next;
  FxU32 startAddress, endAddress;
} tfxTMFreeNode;

typedef struct tfxTMAllocListNode {
  struct tfxTMAllocListNode *next;
  FxU32 startAddress, endAddress;
  struct gl_texture_object *tObj;
} tfxTMAllocNode;

typedef struct {
  GLsizei width, height;
  GLint glideFormat;

  unsigned short *data;
  GLboolean translated, used;
} tfxMipMapLevel;

typedef struct {
  GLuint lastTimeUsed;

  FxU32 whichTMU;

  tfxTMAllocNode *tm[FX_NUM_TMU];

  tfxMipMapLevel mipmapLevel[MAX_TEXTURE_LEVELS];
  GLboolean isInTM;
} tfxTMInfo;

typedef struct {
  tfxTMInfo tmi;

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
#define FX_CONTEXT(ctx) ((struct tfxMesaContext *)((ctx)->DriverCtx))
#define FX_TEXTURE_DATA(t) ((tfxTexInfo *) ((t)->Current->DriverData))

struct tfxMesaContext {
  GuTexPalette glbPalette;

  GLcontext *glCtx;              /* the core Mesa context */
  GLvisual *glVis;               /* describes the color buffer */
  GLframebuffer *glBuffer;       /* the ancillary buffers */

  GLint board;                   /* the board used for this context */
  GLint width, height;           /* size of color buffer */

  GrBuffer_t currentFB;

  GrColor_t color;
  GrColor_t clearC;
  GrAlpha_t clearA;
  GLuint constColor;

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
  tfxTMFreeNode *tmFree[FX_NUM_TMU];
  tfxTMAllocNode *tmAlloc[FX_NUM_TMU];

  GLenum fogTableMode;
  GLfloat fogDensity;
  GrFog_t *fogTable;

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
  GLboolean haveDoubleBuffer;
  GLboolean haveGlobalPaletteTexture;
  GLint swapInterval;
  GLint maxPendingSwapBuffers;
  
  FX_GrContext_t glideContext;
};

typedef void (*tfxSetupFunc)(struct vertex_buffer *, GLuint, GLuint);

extern GrHwConfiguration glbHWConfig;
extern int glbCurrentBoard;

extern void fxSetupFXUnits(GLcontext *);
extern void fxSetupDDPointers(GLcontext *);
extern void fxDDSetNearFar(GLcontext *, GLfloat, GLfloat);

extern void fxDDSetupInit();
extern void fxDDCvaInit();
extern void fxDDTrifuncInit();
extern void fxDDFastPathInit();

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
extern void fxDDClipInit();

extern void fxUpdateDDSpanPointers(GLcontext *);
extern void fxSetupDDSpanPointers(GLcontext *);

extern void fxDDBufferSize(GLcontext *, GLuint *, GLuint *);

extern void fxDDTexEnv(GLcontext *, GLenum, const GLfloat *);
extern void fxDDTexImg(GLcontext *, GLenum, struct gl_texture_object *,
		       GLint, GLint, const struct gl_texture_image *);
extern void fxDDTexParam(GLcontext *, GLenum, struct gl_texture_object *,
			 GLenum, const GLfloat *);
extern void fxDDTexBind(GLcontext *, GLenum, struct gl_texture_object *);
extern void fxDDTexDel(GLcontext *, struct gl_texture_object *);
extern void fxDDTexPalette(GLcontext *, struct gl_texture_object *);
extern void fxDDTexuseGlbPalette(GLcontext *, GLboolean);
extern void fxDDTexSubImg(GLcontext *, GLenum, struct gl_texture_object *, GLint,
			  GLint, GLint, GLint, GLint, GLint, const struct gl_texture_image *);
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

extern void fxTMInit(fxMesaContext);
extern void fxTMClose(fxMesaContext);
extern void fxTMMoveInTM(fxMesaContext, struct gl_texture_object *, GLint);
extern void fxTMMoveOutTM(fxMesaContext, struct gl_texture_object *);
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

extern GLuint fxDDDepthTestSpanGeneric(GLcontext *ctx,
                                       GLuint n, GLint x, GLint y, 
				       const GLdepth z[],
                                       GLubyte mask[]);

extern void fxDDDepthTestPixelsGeneric(GLcontext* ctx,
                                       GLuint n, 
				       const GLint x[], const GLint y[],
                                       const GLdepth z[], GLubyte mask[]);

extern void fxDDReadDepthSpanFloat(GLcontext *ctx,
				   GLuint n, GLint x, GLint y, GLfloat depth[]);

extern void fxDDReadDepthSpanInt(GLcontext *ctx,
				 GLuint n, GLint x, GLint y, GLdepth depth[]);


extern void fxDDFastPath( struct vertex_buffer *VB );

extern void fxDDShadeModel(GLcontext *ctx, GLenum mode);

extern void fxDDCullFace(GLcontext *ctx, GLenum mode);
extern void fxDDFrontFace(GLcontext *ctx, GLenum mode);

extern void fxPrintRenderState( const char *msg, GLuint state );
extern void fxPrintHintState( const char *msg, GLuint state );

extern void fxDDDoRenderVB( struct vertex_buffer *VB );


#endif
