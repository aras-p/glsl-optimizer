/* $Id: fxglidew.h,v 1.13 2001/09/23 16:50:01 brianp Exp $ */

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


#ifndef __FX_GLIDE_WARPER__
#define __FX_GLIDE_WARPER__

#include <glide.h>

/* 
 * General context: 
 */
#if !defined(FX_GLIDE3)
typedef FxU32 FX_GrContext_t;	/* Not used in Glide2 */
#else
typedef GrContext_t FX_GrContext_t;
#endif

/* 
 * Glide3 emulation on Glide2: 
 */
#if !defined(FX_GLIDE3)
	/* Constanst for FX_grGetInteger( ) */
#define FX_FOG_TABLE_ENTRIES            0x0004	/* The number of entries in the hardware fog table. */
#define FX_GLIDE_STATE_SIZE             0x0006	/* Size of buffer, in bytes, needed to save Glide state. */
#define FX_LFB_PIXEL_PIPE               0x0009	/* 1 if LFB writes can go through the 3D pixel pipe. */
#define FX_PENDING_BUFFERSWAPS          0x0014	/* The number of buffer swaps pending. */
#define FX_TEXTURE_ALIGN		0x0024	/* The required alignment for textures */
#else
#define FX_FOG_TABLE_ENTRIES            GR_FOG_TABLE_ENTRIES
#define FX_GLIDE_STATE_SIZE             GR_GLIDE_STATE_SIZE
#define FX_LFB_PIXEL_PIPE               GR_LFB_PIXEL_PIPE
#define FX_PENDING_BUFFERSWAPS          GR_PENDING_BUFFERSWAPS
#define FX_TEXTURE_ALIGN		GR_TEXTURE_ALIGN
#endif

/*
 * Genral warper functions for Glide2/Glide3:
 */
extern FxI32 FX_grGetInteger(FxU32 pname);
extern FxI32 FX_grGetInteger_NoLock(FxU32 pname);

/*
 * Glide2 emulation on Glide3:
 */
#if defined(FX_GLIDE3)

#define GR_ASPECT_1x1 GR_ASPECT_LOG2_1x1
#define GR_ASPECT_2x1 GR_ASPECT_LOG2_2x1
#define GR_ASPECT_4x1 GR_ASPECT_LOG2_4x1
#define GR_ASPECT_8x1 GR_ASPECT_LOG2_8x1
#define GR_ASPECT_1x2 GR_ASPECT_LOG2_1x2
#define GR_ASPECT_1x4 GR_ASPECT_LOG2_1x4
#define GR_ASPECT_1x8 GR_ASPECT_LOG2_1x8

#define GR_LOD_256	GR_LOD_LOG2_256
#define GR_LOD_128	GR_LOD_LOG2_128
#define GR_LOD_64	GR_LOD_LOG2_64
#define GR_LOD_32	GR_LOD_LOG2_32
#define GR_LOD_16	GR_LOD_LOG2_16
#define GR_LOD_8	GR_LOD_LOG2_8
#define GR_LOD_4	GR_LOD_LOG2_4
#define GR_LOD_2	GR_LOD_LOG2_2
#define GR_LOD_1	GR_LOD_LOG2_1

#define GR_FOG_WITH_TABLE GR_FOG_WITH_TABLE_ON_Q

typedef int GrSstType;

#define MAX_NUM_SST            4

#define GR_SSTTYPE_VOODOO    0
#define GR_SSTTYPE_SST96     1
#define GR_SSTTYPE_AT3D      2
#define GR_SSTTYPE_Voodoo2   3

typedef struct GrTMUConfig_St
{
   int tmuRev;			/* Rev of Texelfx chip */
   int tmuRam;			/* 1, 2, or 4 MB */
}
GrTMUConfig_t;

typedef struct GrVoodooConfig_St
{
   int fbRam;			/* 1, 2, or 4 MB */
   int fbiRev;			/* Rev of Pixelfx chip */
   int nTexelfx;		/* How many texelFX chips are there? */
   FxBool sliDetect;		/* Is it a scan-line interleaved board? */
   GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];	/* Configuration of the Texelfx chips */
}
GrVoodooConfig_t;

typedef struct GrSst96Config_St
{
   int fbRam;			/* How much? */
   int nTexelfx;
   GrTMUConfig_t tmuConfig;
}
GrSst96Config_t;

typedef GrVoodooConfig_t GrVoodoo2Config_t;

typedef struct GrAT3DConfig_St
{
   int rev;
}
GrAT3DConfig_t;

typedef struct
{
   int num_sst;			/* # of HW units in the system */
   struct
   {
      GrSstType type;		/* Which hardware is it? */
      union SstBoard_u
      {
	 GrVoodooConfig_t VoodooConfig;
	 GrSst96Config_t SST96Config;
	 GrAT3DConfig_t AT3DConfig;
	 GrVoodoo2Config_t Voodoo2Config;
      }
      sstBoard;
   }
   SSTs[MAX_NUM_SST];		/* configuration for each board */
}
GrHwConfiguration;

typedef FxU32 GrHint_t;
#define GR_HINTTYPE_MIN                 0
#define GR_HINT_STWHINT                 0

typedef FxU32 GrSTWHint_t;
#define GR_STWHINT_W_DIFF_FBI   FXBIT(0)
#define GR_STWHINT_W_DIFF_TMU0  FXBIT(1)
#define GR_STWHINT_ST_DIFF_TMU0 FXBIT(2)
#define GR_STWHINT_W_DIFF_TMU1  FXBIT(3)
#define GR_STWHINT_ST_DIFF_TMU1 FXBIT(4)
#define GR_STWHINT_W_DIFF_TMU2  FXBIT(5)
#define GR_STWHINT_ST_DIFF_TMU2 FXBIT(6)

#define GR_CONTROL_ACTIVATE 		1
#define GR_CONTROL_DEACTIVATE		0

#define GrState				void

/*
** move the vertex layout defintion to application
*/
typedef struct
{
   float sow;			/* s texture ordinate (s over w) */
   float tow;			/* t texture ordinate (t over w) */
   float oow;			/* 1/w (used mipmapping - really 0xfff/w) */
}
GrTmuVertex;


typedef struct
{
   float x, y, z;		/* X, Y, and Z of scrn space -- Z is ignored */
   float r, g, b;		/* R, G, B, ([0..255.0]) */
   float ooz;			/* 65535/Z (used for Z-buffering) */
   float a;			/* Alpha [0..255.0] */
   float oow;			/* 1/W (used for W-buffering, texturing) */
   GrTmuVertex tmuvtx[GLIDE_NUM_TMU];
}
GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_Z_OFFSET              2
#define GR_VERTEX_R_OFFSET              3
#define GR_VERTEX_G_OFFSET              4
#define GR_VERTEX_B_OFFSET              5
#define GR_VERTEX_OOZ_OFFSET            6
#define GR_VERTEX_A_OFFSET              7
#define GR_VERTEX_OOW_OFFSET            8
#define GR_VERTEX_SOW_TMU0_OFFSET       9
#define GR_VERTEX_TOW_TMU0_OFFSET       10
#define GR_VERTEX_OOW_TMU0_OFFSET       11
#define GR_VERTEX_SOW_TMU1_OFFSET       12
#define GR_VERTEX_TOW_TMU1_OFFSET       13
#define GR_VERTEX_OOW_TMU1_OFFSET       14

#endif


/*
 * Glide2 functions for Glide3
 */
#if defined(FX_GLIDE3)
#define FX_grTexDownloadTable(TMU,type,data)	\
  do { 						\
    BEGIN_BOARD_LOCK(); 			\
    grTexDownloadTable(type,data); 		\
    END_BOARD_LOCK(); 				\
  } while (0);
#define FX_grTexDownloadTable_NoLock(TMU,type,data) \
  grTexDownloadTable(type, data)
#else
#define FX_grTexDownloadTable(TMU,type,data) 	\
  do {						\
    BEGIN_BOARD_LOCK();				\
    grTexDownloadTable(TMU,type,data);		\
    END_BOARD_LOCK();				\
  } while (0);
#define FX_grTexDownloadTable_NoLock grTexDownloadTable
#endif

/*
 * Flush
 */
#if defined(FX_GLIDE3)
#define FX_grFlush()	\
  do {			\
    BEGIN_BOARD_LOCK(); \
    grFlush();		\
    END_BOARD_LOCK();	\
  } while (0)
#else
#define FX_grFlush()	\
  do {			\
    BEGIN_BOARD_LOCK(); \
    grSstIdle();	\
    END_BOARD_LOCK();	\
  } while (0)
#endif

#define FX_grFinish()	\
  do {			\
    BEGIN_BOARD_LOCK(); \
    grFinish();		\
    END_BOARD_LOCK();	\
  } while (0)

/*
 * Write region: ToDo possible exploit the PixelPipe parameter.
 */
#if defined(FX_GLIDE3)
#define FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,FXFALSE,src_stride,src_data);	\
    END_BOARD_LOCK();		\
  } while(0)
#else
#define FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)		\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data);		\
    END_BOARD_LOCK();		\
  } while (0)
#endif

/*
 * Read region
 */
#define FX_grLfbReadRegion(src_buffer,src_x,src_y,src_width,src_height,dst_stride,dst_data)			\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grLfbReadRegion(src_buffer,src_x,src_y,src_width,src_height,dst_stride,dst_data);				\
    END_BOARD_LOCK();		\
  } while (0);

/*
 * Draw triangle
 */
#define FX_grDrawTriangle(a,b,c)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grDrawTriangle(a,b,c);		\
    END_CLIP_LOOP();			\
  } while (0)

/*
 * For Lod/LodLog2 conversion.
 */
#if defined(FX_GLIDE3)
#define FX_largeLodLog2(info)		(info).largeLodLog2
#else
#define FX_largeLodLog2(info)		(info).largeLod
#endif

#if defined(FX_GLIDE3)
#define FX_aspectRatioLog2(info)		(info).aspectRatioLog2
#else
#define FX_aspectRatioLog2(info)		(info).aspectRatio
#endif

#if defined(FX_GLIDE3)
#define FX_smallLodLog2(info)		(info).smallLodLog2
#else
#define FX_smallLodLog2(info)		(info).smallLod
#endif

#if defined(FX_GLIDE3)
#define FX_lodToValue(val)		((int)(GR_LOD_256-val))
#else
#define FX_lodToValue(val)		((int)(val))
#endif

#if defined(FX_GLIDE3)
#define FX_largeLodValue(info)		((int)(GR_LOD_256-(info).largeLodLog2))
#else
#define FX_largeLodValue(info)		((int)(info).largeLod)
#endif
#define FX_largeLodValue_NoLock FX_largeLodValue

#if defined(FX_GLIDE3)
#define FX_smallLodValue(info)		((int)(GR_LOD_256-(info).smallLodLog2))
#else
#define FX_smallLodValue(info)		((int)(info).smallLod)
#endif
#define FX_smallLodValue_NoLock FX_smallLodValue

#if defined(FX_GLIDE3)
#define FX_valueToLod(val)		((GrLOD_t)(GR_LOD_256-val))
#else
#define FX_valueToLod(val)		((GrLOD_t)(val))
#endif

/*
 * ScreenWidth/Height stuff.
 */
extern int FX_grSstScreenWidth(void);
extern int FX_grSstScreenHeight(void);



/*
 * Version string.
 */
#if defined(FX_GLIDE3)
extern void FX_grGlideGetVersion(char *buf);
#else
#define FX_grGlideGetVersion(b)	\
	do {				\
	  BEGIN_BOARD_LOCK();		\
	  grGlideGetVersion(b);		\
	  END_BOARD_LOCK();		\
	} while (0)
#endif
/*
 * Performance statistics
 */
#if defined(FX_GLIDE3)
extern void FX_grSstPerfStats(GrSstPerfStats_t * st);
#else
#define FX_grSstPerfStats(s)	\
	do {				\
	  BEGIN_BOARD_LOCK();		\
	  grSstPerfStats(s);		\
	  END_BOARD_LOCK();		\
	} while (0)
#endif

/*
 * Hardware Query
 */
extern int FX_grSstQueryHardware(GrHwConfiguration * config);

/*
 * GrHints
 */
#if defined(FX_GLIDE3)
extern void FX_grHints_NoLock(GrHint_t hintType, FxU32 hintMask);
extern void FX_grHints(GrHint_t hintType, FxU32 hintMask);
#else
#define FX_grHints(t,m)		\
	do {				\
	  BEGIN_BOARD_LOCK();		\
	  grHints(t, m);		\
	  END_BOARD_LOCK();		\
	} while(0)
#define FX_grHints_NoLock grHints
#endif
/*
 * Antialiashed line+point drawing.
 */
#if defined(FX_GLIDE3)
extern void FX_grAADrawLine(GrVertex * a, GrVertex * b);
#else
#define FX_grAADrawLine(a,b)	\
	do {				\
	  BEGIN_CLIP_LOOP();		\
	  grAADrawLine(a,b);		\
	  END_CLIP_LOOP();		\
	} while (0)
#endif

#if defined(FX_GLIDE3)
extern void FX_grAADrawPoint(GrVertex * a);
#else
#define FX_grAADrawPoint(a)	\
	do {				\
	  BEGIN_CLIP_LOOP();		\
	  grAADrawPoint(a);		\
	  END_CLIP_LOOP();		\
	} while (0)
#endif

/*
 * Needed for Glide3 only, to set up Glide2 compatible vertex layout.
 */
#if defined(FX_GLIDE3)
extern void FX_setupGrVertexLayout(void);
#else
#define FX_setupGrVertexLayout()		do {} while (0)
#endif
/*
 * grSstControl stuff
 */
extern FxBool FX_grSstControl(FxU32 code);

/*
 * grGammaCorrectionValue
 */
#if defined(FX_GLIDE3)
extern void FX_grGammaCorrectionValue(float val);
#else
#define FX_grGammaCorrectionValue(v)	\
      do {					\
        BEGIN_BOARD_LOCK();			\
	grGammaCorrectionValue(v)		\
        END_BOARD_LOCK();			\
      } while (0)
#endif

#if defined(FX_GLIDE3)
#define FX_grSstWinClose(w)	\
  do { 				\
    BEGIN_BOARD_LOCK();		\
    grSstWinClose(w);		\
    END_BOARD_LOCK();		\
  } while (0)
#else
#define FX_grSstWinClose(w)	\
  do { 				\
    BEGIN_BOARD_LOCK();		\
    grSstWinClose();		\
    END_BOARD_LOCK();		\
  } while (0)
#endif


extern FX_GrContext_t FX_grSstWinOpen(FxU32 hWnd,
				      GrScreenResolution_t screen_resolution,
				      GrScreenRefresh_t refresh_rate,
				      GrColorFormat_t color_format,
				      GrOriginLocation_t origin_location,
				      int nColBuffers, int nAuxBuffers);


#define FX_grDrawLine(v1, v2)	\
  do {				\
    BEGIN_CLIP_LOOP();		\
    grDrawLine(v1, v2);		\
    END_CLIP_LOOP();		\
  } while (0)

#define FX_grDrawPoint(p)	\
  do {				\
    BEGIN_CLIP_LOOP();		\
    grDrawPoint(p);		\
    END_CLIP_LOOP();		\
  } while (0)

#if defined(FX_GLIDE3)
extern void FX_grDrawPolygonVertexList(int n, GrVertex * v);
#else
#define FX_grDrawPolygonVertexList(n, v)	\
  do {						\
    BEGIN_CLIP_LOOP();				\
    grDrawPolygonVertexList(n, v);		\
    END_CLIP_LOOP();				\
  } while (0)
#endif

#define FX_grDitherMode(m)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grDitherMode(m);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grRenderBuffer(b)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grRenderBuffer(b);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grBufferClear(c, a, d)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grBufferClear(c, a, d);		\
    END_CLIP_LOOP();			\
  } while (0)

#define FX_grDepthMask(m)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grDepthMask(m);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grColorMask(c, a)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grColorMask(c, a);		\
    END_BOARD_LOCK();		\
  } while (0)

extern FxBool FX_grLfbLock(GrLock_t type, GrBuffer_t buffer,
			   GrLfbWriteMode_t writeMode,
			   GrOriginLocation_t origin, FxBool pixelPipeline,
			   GrLfbInfo_t * info);

#define FX_grLfbUnlock(t, b)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grLfbUnlock(t, b);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grConstantColorValue(v)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grConstantColorValue(v);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grConstantColorValue_NoLock grConstantColorValue

#define FX_grAADrawTriangle(a, b, c, ab, bc, ca)	\
  do {							\
    BEGIN_CLIP_LOOP();					\
    grAADrawTriangle(a, b, c, ab, bc, ca);		\
    END_CLIP_LOOP();					\
  } while (0)

#define FX_grAlphaBlendFunction(rs, rd, as, ad)	\
  do {						\
    BEGIN_BOARD_LOCK();				\
    grAlphaBlendFunction(rs, rd, as, ad);	\
    END_BOARD_LOCK();				\
  } while (0)

#define FX_grAlphaCombine(func, fact, loc, oth, inv)	\
  do {							\
    BEGIN_BOARD_LOCK();					\
    grAlphaCombine(func, fact, loc, oth, inv);		\
    END_BOARD_LOCK();					\
  } while (0)

#define FX_grAlphaCombine_NoLock grAlphaCombine

#define FX_grAlphaTestFunction(f)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grAlphaTestFunction(f);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grAlphaTestReferenceValue(v)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grAlphaTestReferenceValue(v);	\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grClipWindow(minx, miny, maxx, maxy)	\
  do {						\
    BEGIN_BOARD_LOCK();				\
    grClipWindow(minx, miny, maxx, maxy);	\
    END_BOARD_LOCK();				\
  } while (0)

#define FX_grClipWindow_NoLock grClipWindow

#define FX_grColorCombine(func, fact, loc, oth, inv)	\
  do {							\
    BEGIN_BOARD_LOCK();					\
    grColorCombine(func, fact, loc, oth, inv);		\
    END_BOARD_LOCK();					\
  } while (0)

#define FX_grColorCombine_NoLock grColorCombine

#define FX_grCullMode(m)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grCullMode(m);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grDepthBiasLevel(lev)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grDepthBiasLevel(lev);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grDepthBufferFunction(func)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grDepthBufferFunction(func);	\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grFogColorValue(c)		\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grFogColorValue(c);			\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grFogMode(m)	\
  do {			\
    BEGIN_BOARD_LOCK(); \
    grFogMode(m);	\
    END_BOARD_LOCK();	\
  } while (0)

#define FX_grFogTable(t)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grFogTable(t);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grTexClampMode(t, sc, tc)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grTexClampMode(t, sc, tc);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grTexClampMode_NoLock grTexClampMode

#define FX_grTexCombine(t, rfunc, rfact, afunc, afact, rinv, ainv)	\
  do {									\
    BEGIN_BOARD_LOCK();							\
    grTexCombine(t, rfunc, rfact, afunc, afact, rinv, ainv);		\
    END_BOARD_LOCK();							\
  } while (0)

#define FX_grTexCombine_NoLock grTexCombine

#define FX_grTexDownloadMipMapLevel(t, sa, tlod, llod, ar, f, eo, d)	\
  do {									\
    BEGIN_BOARD_LOCK();							\
    grTexDownloadMipMapLevel(t, sa, tlod, llod, ar, f, eo, d);		\
    END_BOARD_LOCK();							\
  } while (0)

#define FX_grTexDownloadMipMapLevel_NoLock grTexDownloadMipMapLevel

#define FX_grTexDownloadMipMapLevelPartial(t, sa, tlod, llod, ar, f, eo, d, s, e);	\
  do {									    \
    BEGIN_BOARD_LOCK();							    \
    grTexDownloadMipMapLevelPartial(t, sa, tlod, llod, ar, f, eo, d, s, e); \
    END_BOARD_LOCK();							    \
  } while (0)

#define FX_grTexFilterMode(t, minf, magf)	\
  do {						\
    BEGIN_BOARD_LOCK();				\
    grTexFilterMode(t, minf, magf);		\
    END_BOARD_LOCK();				\
  } while (0)

#define FX_grTexFilterMode_NoLock grTexFilterMode

extern FxU32 FX_grTexMinAddress(GrChipID_t tmu);
extern FxU32 FX_grTexMaxAddress(GrChipID_t tmu);

#define FX_grTexMipMapMode(t, m, lod)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grTexMipMapMode(t, m, lod);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grTexMipMapMode_NoLock grTexMipMapMode

#define FX_grTexSource(t, sa, eo, i)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grTexSource(t, sa, eo, i);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grTexSource_NoLock grTexSource

extern FxU32 FX_grTexTextureMemRequired(FxU32 evenOdd, GrTexInfo * info);
#define FX_grTexTextureMemRequired_NoLock grTexTextureMemRequired

#define FX_grGlideGetState(s)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grGlideGetState(s);		\
    END_BOARD_LOCK();		\
  } while (0)
#define FX_grGlideGetState_NoLock(s) grGlideGetState(s);

#define FX_grDRIBufferSwap(i)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grDRIBufferSwap(i);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grSstSelect(b)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grSstSelect(b);		\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grSstSelect_NoLock grSstSelect

#define FX_grGlideSetState(s)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grGlideSetState(s);		\
    END_BOARD_LOCK();		\
  } while (0)
#define FX_grGlideSetState_NoLock(s) grGlideSetState(s);

#define FX_grDepthBufferMode(m)	\
  do {				\
    BEGIN_BOARD_LOCK();		\
    grDepthBufferMode(m);	\
    END_BOARD_LOCK();		\
  } while (0)

#define FX_grLfbWriteColorFormat(f)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grLfbWriteColorFormat(f);		\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grDrawVertexArray(m, c, p)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grDrawVertexArray(m, c, p);		\
    END_CLIP_LOOP();			\
  } while (0)

#define FX_grGlideShutdown()		\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grGlideShutdown();			\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grTexLodBiasValue_NoLock(t, v) grTexLodBiasValue(t, v)

#define FX_grTexLodBiasValue(t, v)	\
  do {					\
    BEGIN_BOARD_LOCK();			\
    grTexLodBiasValue(t, v);	\
    END_BOARD_LOCK();			\
  } while (0)

#define FX_grGlideInit_NoLock grGlideInit
#define FX_grSstWinOpen_NoLock grSstWinOpen

extern int FX_getFogTableSize(void);
extern int FX_getGrStateSize(void);

#endif /* __FX_GLIDE_WARPER__ */
