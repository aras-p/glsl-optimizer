/* $Id: fxdrv.h,v 1.52 2001/09/23 16:50:01 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 */

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */


#ifndef FXDRV_H
#define FXDRV_H

/* If you comment out this define, a variable takes its place, letting
 * you turn debugging on/off from the debugger.
 */

#include "glheader.h"


#if defined(__linux__)
#include <signal.h>
#endif

#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mtypes.h"

#include "GL/fxmesa.h"
#include "fxglidew.h"

#include "math/m_vector.h"


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


extern float gl_ubyte_to_float_255_color_tab[256];
#define UBYTE_COLOR_TO_FLOAT_255_COLOR(c) gl_ubyte_to_float_255_color_tab[c]
#define UBYTE_COLOR_TO_FLOAT_255_COLOR2(f,c) \
    (*(int *)&(f)) = ((int *)gl_ubyte_to_float_255_color_tab)[c]





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



/* fastpath flags first
 */
#define SETUP_TMU0 0x1
#define SETUP_TMU1 0x2
#define SETUP_RGBA 0x4
#define SETUP_SNAP 0x8
#define SETUP_XYZW 0x10
#define SETUP_PTEX 0x20
#define MAX_SETUP  0x40


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
#define FX_UM_E0_ADD		    0x00000010

#define FX_UM_E1_REPLACE            0x00000020
#define FX_UM_E1_MODULATE           0x00000040
#define FX_UM_E1_DECAL              0x00000080
#define FX_UM_E1_BLEND              0x00000100
#define FX_UM_E1_ADD		    0x00000200

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


/*
  Memory range from startAddr to endAddr-1
*/
typedef struct MemRange_t
{
   struct MemRange_t *next;
   FxU32 startAddr, endAddr;
}
MemRange;

typedef struct
{
   GLsizei width, height;	/* image size */
   GLint wScale, hScale;	/* image scale factor */
   GrTextureFormat_t glideFormat;	/* Glide image format */
}
tfxMipMapLevel;

/*
 * TDFX-specific texture object data.  This hangs off of the
 * struct gl_texture_object DriverData pointer.
 */
typedef struct tfxTexInfo_t
{
   struct tfxTexInfo_t *next;
   struct gl_texture_object *tObj;

   GLuint lastTimeUsed;
   FxU32 whichTMU;
   GLboolean isInTM;

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
}
tfxTexInfo;

typedef struct
{
   GLuint swapBuffer;
   GLuint reqTexUpload;
   GLuint texUpload;
   GLuint memTexUpload;
}
tfxStats;



typedef struct
{
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
}
tfxUnitsState;




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


#define FX_CONTEXT(ctx) ((fxMesaContext)((ctx)->DriverCtx))

#define FX_TEXTURE_DATA(texUnit) fxTMGetTexInfo((texUnit)->_Current)

#define fxTMGetTexInfo(o) ((tfxTexInfo*)((o)->DriverData))

#define FX_MIPMAP_DATA(img)  ((tfxMipMapLevel *) (img)->DriverData)

#define BEGIN_BOARD_LOCK()
#define END_BOARD_LOCK()
#define BEGIN_CLIP_LOOP()
#define END_CLIP_LOOP()




/* Covers the state referenced by IsInHardware:
 */
#define _FX_NEW_IS_IN_HARDWARE (_NEW_TEXTURE|		\
			        _NEW_HINT|		\
			        _NEW_STENCIL|		\
			        _NEW_BUFFERS|		\
			        _NEW_COLOR|		\
			        _NEW_LIGHT)

/* Covers the state referenced by fxDDChooseRenderState
 */
#define _FX_NEW_RENDERSTATE (_FX_NEW_IS_IN_HARDWARE |   \
			     _DD_NEW_FLATSHADE |	\
			     _DD_NEW_TRI_LIGHT_TWOSIDE| \
			     _DD_NEW_TRI_OFFSET |	\
			     _DD_NEW_TRI_UNFILLED |	\
			     _DD_NEW_TRI_SMOOTH |	\
			     _DD_NEW_TRI_STIPPLE |	\
			     _DD_NEW_LINE_SMOOTH |	\
			     _DD_NEW_LINE_STIPPLE |	\
			     _DD_NEW_LINE_WIDTH |	\
			     _DD_NEW_POINT_SMOOTH |	\
			     _DD_NEW_POINT_SIZE |	\
			     _NEW_LINE)


/* Covers the state referenced by fxDDChooseSetupFunction.
 */
#define _FX_NEW_SETUP_FUNCTION (_NEW_LIGHT|	\
			        _NEW_FOG|	\
			        _NEW_TEXTURE|	\
			        _NEW_COLOR)	\


/* These lookup table are used to extract RGB values in [0,255] from
 * 16-bit pixel values.
 */
extern GLubyte FX_PixelToR[0x10000];
extern GLubyte FX_PixelToG[0x10000];
extern GLubyte FX_PixelToB[0x10000];


typedef void (*fx_tri_func) (fxMesaContext, GrVertex *, GrVertex *, GrVertex *);
typedef void (*fx_line_func) (fxMesaContext, GrVertex *, GrVertex *);
typedef void (*fx_point_func) (fxMesaContext, GrVertex *);

struct tfxMesaContext
{
   GuTexPalette glbPalette;

   GLcontext *glCtx;		/* the core Mesa context */
   GLvisual *glVis;		/* describes the color buffer */
   GLframebuffer *glBuffer;	/* the ancillary buffers */

   GLint board;			/* the board used for this context */
   GLint width, height;		/* size of color buffer */

   GrBuffer_t currentFB;

   GLboolean bgrOrder;
   GrColor_t color;
   GrColor_t clearC;
   GrAlpha_t clearA;
   GLuint constColor;
   GrCullMode_t cullMode;

   tfxUnitsState unitsState;
   tfxUnitsState restoreUnitsState;	/* saved during multipass */


   GLuint new_state;

   /* Texture Memory Manager Data 
    */
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

   /* Vertex building and storage:
    */
   GLuint tmu_source[FX_NUM_TMU];
   GLuint SetupIndex;
   GLuint stw_hint_state;	/* for grHints */
   GrVertex *verts;
   GLboolean snapVertices;      /* needed for older Voodoo hardware */
   struct gl_client_array UbyteColor;

   /* Rasterization:
    */
   GLuint render_index;
   GLuint is_in_hardware;
   GLenum render_primitive;
   GLenum raster_primitive;

   /* Current rasterization functions 
    */
   fx_point_func draw_point;
   fx_line_func draw_line;
   fx_tri_func draw_tri;


   /* Keep texture scales somewhere handy:
    */
   GLfloat s0scale;
   GLfloat s1scale;
   GLfloat t0scale;
   GLfloat t1scale;

   GLfloat inv_s0scale;
   GLfloat inv_s1scale;
   GLfloat inv_t0scale;
   GLfloat inv_t1scale;

   /* Glide stuff
    */
   tfxStats stats;
   void *state;

   /* Options */

   GLboolean verbose;
   GLboolean haveTwoTMUs;	/* True if we really have 2 tmu's  */
   GLboolean haveAlphaBuffer;
   GLboolean haveZBuffer;
   GLboolean haveDoubleBuffer;
   GLboolean haveGlobalPaletteTexture;
   GLint swapInterval;
   GLint maxPendingSwapBuffers;

   FX_GrContext_t glideContext;

   int screen_width;
   int screen_height;
   int initDone;
   int clipMinX;
   int clipMaxX;
   int clipMinY;
   int clipMaxY;
};


extern GrHwConfiguration glbHWConfig;
extern int glbCurrentBoard;

extern void fxSetupFXUnits(GLcontext *);
extern void fxSetupDDPointers(GLcontext *);

/* fxvb.c:
 */
extern void fxAllocVB(GLcontext * ctx);
extern void fxFreeVB(GLcontext * ctx);
extern void fxPrintSetupFlags(char *msg, GLuint flags );
extern void fxCheckTexSizes( GLcontext *ctx );
extern void fxBuildVertices( GLcontext *ctx, GLuint start, GLuint count,
			     GLuint newinputs );
extern void fxChooseVertexState( GLcontext *ctx );






/* fxtrifuncs:
 */
extern void fxDDInitTriFuncs(GLcontext *);
extern void fxDDChooseRenderState(GLcontext * ctx);


extern void fxUpdateDDSpanPointers(GLcontext *);
extern void fxSetupDDSpanPointers(GLcontext *);

extern void fxPrintTextureData(tfxTexInfo * ti);

extern const struct gl_texture_format *
fxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                         GLenum srcFormat, GLenum srcType );
extern void fxDDTexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat, GLint width, GLint height,
			   GLint border, GLenum format, GLenum type,
			   const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage);

extern void fxDDTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint yoffset,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
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
extern void fxDDAlphaFunc(GLcontext *, GLenum, GLchan);
extern void fxDDBlendFunc(GLcontext *, GLenum, GLenum);
extern void fxDDDepthMask(GLcontext *, GLboolean);
extern void fxDDDepthFunc(GLcontext *, GLenum);

extern void fxDDInitExtensions(GLcontext * ctx);

extern void fxTMInit(fxMesaContext ctx);
extern void fxTMClose(fxMesaContext ctx);
extern void fxTMRestoreTextures_NoLock(fxMesaContext ctx);
extern void fxTMMoveInTM(fxMesaContext, struct gl_texture_object *, GLint);
extern void fxTMMoveOutTM(fxMesaContext, struct gl_texture_object *);
#define fxTMMoveOutTM_NoLock fxTMMoveOutTM
extern void fxTMFreeTexture(fxMesaContext, struct gl_texture_object *);
extern void fxTMReloadMipMapLevel(fxMesaContext, struct gl_texture_object *,
				  GLint);
extern void fxTMReloadSubMipMapLevel(fxMesaContext,
				     struct gl_texture_object *, GLint, GLint,
				     GLint);

extern void fxTexGetFormat(GLenum, GrTextureFormat_t *, GLint *);
extern int fxTexGetInfo(int, int, GrLOD_t *, GrAspectRatio_t *,
			float *, float *, int *, int *, int *, int *);

extern void fxDDScissor(GLcontext * ctx,
			GLint x, GLint y, GLsizei w, GLsizei h);
extern void fxDDFogfv(GLcontext * ctx, GLenum pname, const GLfloat * params);
extern void fxDDColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a);

extern void fxDDWriteDepthSpan(GLcontext * ctx, GLuint n, GLint x, GLint y,
			       const GLdepth depth[], const GLubyte mask[]);

extern void fxDDReadDepthSpan(GLcontext * ctx, GLuint n, GLint x, GLint y,
			      GLdepth depth[]);

extern void fxDDWriteDepthPixels(GLcontext * ctx, GLuint n,
				 const GLint x[], const GLint y[],
				 const GLdepth depth[], const GLubyte mask[]);

extern void fxDDReadDepthPixels(GLcontext * ctx, GLuint n,
				const GLint x[], const GLint y[],
				GLdepth depth[]);

extern void fxDDShadeModel(GLcontext * ctx, GLenum mode);

extern void fxDDCullFace(GLcontext * ctx, GLenum mode);
extern void fxDDFrontFace(GLcontext * ctx, GLenum mode);

extern void fxPrintRenderState(const char *msg, GLuint state);
extern void fxPrintHintState(const char *msg, GLuint state);

extern int fxDDInitFxMesaContext(fxMesaContext fxMesa);
extern void fxDDDestroyFxMesaContext(fxMesaContext fxMesa);




extern void fxSetScissorValues(GLcontext * ctx);
extern void fxTMMoveInTM_NoLock(fxMesaContext fxMesa,
				struct gl_texture_object *tObj, GLint where);
extern void fxInitPixelTables(fxMesaContext fxMesa, GLboolean bgrOrder);

extern void fxCheckIsInHardware(GLcontext *ctx);

extern GLboolean fx_check_IsInHardware(GLcontext *ctx);

#endif
