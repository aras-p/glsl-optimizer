/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
#ifndef __FX_GLIDE_WARPER__
#define __FX_GLIDE_WARPER__

#include <glide.h>

/* 
 * General context: 
 */
#if !defined(FX_GLIDE3)
     typedef FxU32 	 FX_GrContext_t;	/* Not used in Glide2 */
#else
     typedef GrContext_t FX_GrContext_t;
#endif

/* 
 * Glide3 emulation on Glide2: 
 */
#if !defined(FX_GLIDE3)
	/* Constanst for FX_grGetInteger( ) */
	#define FX_FOG_TABLE_ENTRIES            0x0004    /* The number of entries in the hardware fog table. */
	#define FX_GLIDE_STATE_SIZE             0x0006	  /* Size of buffer, in bytes, needed to save Glide state. */
	#define FX_LFB_PIXEL_PIPE               0x0009	  /* 1 if LFB writes can go through the 3D pixel pipe. */		
	#define FX_PENDING_BUFFERSWAPS          0x0014    /* The number of buffer swaps pending. */
#else
        #define FX_FOG_TABLE_ENTRIES            GR_FOG_TABLE_ENTRIES  
	#define FX_GLIDE_STATE_SIZE             GR_GLIDE_STATE_SIZE
	#define FX_LFB_PIXEL_PIPE               GR_LFB_PIXEL_PIPE		
	#define FX_PENDING_BUFFERSWAPS          GR_PENDING_BUFFERSWAPS  
#endif

/*
 * Genral warper functions for Glide2/Glide3:
 */ 
extern FxI32 FX_grGetInteger(FxU32 pname);

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

typedef struct GrTMUConfig_St {
  int    tmuRev;                /* Rev of Texelfx chip */
  int    tmuRam;                /* 1, 2, or 4 MB */
} GrTMUConfig_t;

typedef struct GrVoodooConfig_St {
  int    fbRam;                         /* 1, 2, or 4 MB */
  int    fbiRev;                        /* Rev of Pixelfx chip */
  int    nTexelfx;                      /* How many texelFX chips are there? */
  FxBool sliDetect;                     /* Is it a scan-line interleaved board? */
  GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];   /* Configuration of the Texelfx chips */
} GrVoodooConfig_t;

typedef struct GrSst96Config_St {
  int   fbRam;                  /* How much? */
  int   nTexelfx;
  GrTMUConfig_t tmuConfig;
} GrSst96Config_t;

typedef GrVoodooConfig_t GrVoodoo2Config_t;

typedef struct GrAT3DConfig_St {
  int   rev;
} GrAT3DConfig_t;

typedef struct {
  int num_sst;                  /* # of HW units in the system */
  struct {
    GrSstType type;             /* Which hardware is it? */
    union SstBoard_u {
      GrVoodooConfig_t  VoodooConfig;
      GrSst96Config_t   SST96Config;
      GrAT3DConfig_t    AT3DConfig;
      GrVoodoo2Config_t Voodoo2Config;
    } sstBoard;
  } SSTs[MAX_NUM_SST];          /* configuration for each board */
} GrHwConfiguration;

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
typedef struct {
  float  sow;                   /* s texture ordinate (s over w) */
  float  tow;                   /* t texture ordinate (t over w) */  
  float  oow;                   /* 1/w (used mipmapping - really 0xfff/w) */
}  GrTmuVertex;


#if FX_USE_PARGB

typedef struct
{
  float x, y;         /* X and Y in screen space */
  float ooz;          /* 65535/Z (used for Z-buffering) */
  float oow;          /* 1/W (used for W-buffering, texturing) */
  FxU32 argb;         /* R, G, B, A [0..255.0] */
  GrTmuVertex         tmuvtx[GLIDE_NUM_TMU];
  float z;            /* Z is ignored */
} GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_OOZ_OFFSET            2
#define GR_VERTEX_OOW_OFFSET            3
#define GR_VERTEX_PARGB_OFFSET          4
#define GR_VERTEX_SOW_TMU0_OFFSET       5
#define GR_VERTEX_TOW_TMU0_OFFSET       6
#define GR_VERTEX_OOW_TMU0_OFFSET       7
#define GR_VERTEX_SOW_TMU1_OFFSET       8
#define GR_VERTEX_TOW_TMU1_OFFSET       9
#define GR_VERTEX_OOW_TMU1_OFFSET       10
#define GR_VERTEX_Z_OFFSET		11

#define GET_PARGB(v)	((FxU32*)(v))[GR_VERTEX_PARGB_OFFSET]
/* GET_PA: returns the alpha component */
#if GLIDE_ENDIAN == GLIDE_ENDIAN_BIG
   #define GET_PA(v)		((FxU8*)(v))[GR_VERTEX_PARGB_OFFSET*4]
#else 
   #define GET_PA(v)		((FxU8*)(v))[GR_VERTEX_PARGB_OFFSET*4+3]
#endif 
#define MESACOLOR2PARGB(c)	(c[ACOMP] << 24 | c[GCOMP] << 16 | c[GCOMP] << 8 | c[BCOMP])
#define PACK_4F_ARGB(dest,a,r,g,b) { 								\
    					     const GLuint cr = (int)r;				\
    					     const GLuint cg = (int)g;				\
    					     const GLuint ca = (int)a;				\
    					     const GLuint cb = (int)b;				\
    					     dest = ca << 24 | cr << 16 | cg << 8 | cb;		\
    				        }

#else /* FX_USE_PARGB */

typedef struct
{
  float x, y;         /* X and Y in screen space */
  float ooz;          /* 65535/Z (used for Z-buffering) */
  float oow;          /* 1/W (used for W-buffering, texturing) */
  float r, g, b, a;   /* R, G, B, A [0..255.0] */
  float z;            /* Z is ignored */
  GrTmuVertex  tmuvtx[GLIDE_NUM_TMU];
} GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_OOZ_OFFSET            2
#define GR_VERTEX_OOW_OFFSET            3
#define GR_VERTEX_R_OFFSET              4
#define GR_VERTEX_G_OFFSET              5
#define GR_VERTEX_B_OFFSET              6
#define GR_VERTEX_A_OFFSET              7
#define GR_VERTEX_Z_OFFSET              8
#define GR_VERTEX_SOW_TMU0_OFFSET       9
#define GR_VERTEX_TOW_TMU0_OFFSET       10
#define GR_VERTEX_OOW_TMU0_OFFSET       11
#define GR_VERTEX_SOW_TMU1_OFFSET       12
#define GR_VERTEX_TOW_TMU1_OFFSET       13
#define GR_VERTEX_OOW_TMU1_OFFSET       14
#endif /* FX_USE_PARGB */

#endif


/*
 * Glide2 functions for Glide3
 */
#if defined(FX_GLIDE3)
#define FX_grTexDownloadTable(TMU,type,data)		grTexDownloadTable(type,data)
#else
#define FX_grTexDownloadTable(TMU,type,data) 		grTexDownloadTable(TMU,type,data)
#endif

/*
 * Flush
 */
#if defined(FX_GLIDE3)
#define FX_grFlush		grFlush
#else
#define FX_grFlush		grSstIdle
#endif	
/*
 * Write region: ToDo possible exploit the PixelPipe parameter.
 */
#if defined(FX_GLIDE3)
#define FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)	\
	grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,FXFALSE,src_stride,src_data)
#else
#define FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)	\
	grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)
#endif
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

#if defined(FX_GLIDE3)
	#define FX_smallLodValue(info)		((int)(GR_LOD_256-(info).smallLodLog2))
#else
	#define FX_smallLodValue(info)		((int)(info).smallLod)
#endif

#if defined(FX_GLIDE3)
	#define FX_valueToLod(val)		((GrLOD_t)(GR_LOD_256-val))
#else
	#define FX_valueToLod(val)		((GrLOD_t)(val))
#endif

/*
 * ScreenWidth/Height stuff.
 */
#if defined(FX_GLIDE3)
	extern int FX_grSstScreenWidth();
	extern int FX_grSstScreenHeight();
#else
	#define FX_grSstScreenWidth()		grSstScreenWidth()
	#define FX_grSstScreenHeight()		grSstScreenHeight()
#endif


/*
 * Version string.
 */
#if defined(FX_GLIDE3)
	extern void FX_grGlideGetVersion(char *buf);
#else
	#define FX_grGlideGetVersion		grGlideGetVersion	
#endif
/*
 * Performance statistics
 */
#if defined(FX_GLIDE3)
        extern void FX_grSstPerfStats(GrSstPerfStats_t *st);
#else
	#define FX_grSstPerfStats		grSstPerfStats
#endif

/*
 * Hardware Query
 */
#if defined(FX_GLIDE3)
       extern int FX_grSstQueryHardware(GrHwConfiguration *config);
#else
       #define FX_grSstQueryHardware		grSstQueryHardware		
#endif

/*
 * GrHints
 */
#if defined(FX_GLIDE3)
	extern void FX_grHints(GrHint_t hintType, FxU32 hintMask);
#else
	#define FX_grHints			grHints
#endif
/*
 * Antialiashed line+point drawing.
 */
#if defined(FX_GLIDE3)
	extern void FX_grAADrawLine(GrVertex *a,GrVertex *b);
#else
	#define FX_grAADrawLine			grAADrawLine
#endif

#if defined(FX_GLIDE3)
	extern void FX_grAADrawPoint(GrVertex *a);
#else
	#define FX_grAADrawPoint		grAADrawPoint
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
#if defined(FX_GLIDE3)
	extern void FX_grSstControl(int par);
#else
	#define FX_grSstControl				grSstControl
#endif
/*
 * grGammaCorrectionValue
 */
#if defined(FX_GLIDE3)
      extern void FX_grGammaCorrectionValue(float val);
#else
      #define FX_grGammaCorrectionValue			grGammaCorrectionValue
#endif

/*
 * WinOpen/Close.
 */
#if defined(FX_GLIDE3)
       #define FX_grSstWinOpen(hWnd,screen_resolution,refresh_rate,color_format,origin_location,nColBuffers,nAuxBuffers) \
      		  grSstWinOpen(-1,screen_resolution,refresh_rate,color_format,origin_location,nColBuffers,nAuxBuffers)
       #define FX_grSstWinClose		grSstWinClose
#else
       #define FX_grSstWinOpen		grSstWinOpen
       #define FX_grSstWinClose(win)	grSstWinClose()
#endif


#endif /* __FX_GLIDE_WARPER__ */
