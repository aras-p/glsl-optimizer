/* $Id: fxglidew.h,v 1.15 2003/08/19 15:52:53 brianp Exp $ */

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
 *    Daniel Borca
 *    Hiroshi Morii
 */


#ifndef __FX_GLIDE_WARPER__
#define __FX_GLIDE_WARPER__


#include <glide.h>
#include <g3ext.h>



#define FX_grGetInteger FX_grGetInteger_NoLock
extern FxI32 FX_grGetInteger_NoLock(FxU32 pname);



#define GR_ASPECT_1x1           GR_ASPECT_LOG2_1x1
#define GR_ASPECT_2x1           GR_ASPECT_LOG2_2x1
#define GR_ASPECT_4x1           GR_ASPECT_LOG2_4x1
#define GR_ASPECT_8x1           GR_ASPECT_LOG2_8x1
#define GR_ASPECT_1x2           GR_ASPECT_LOG2_1x2
#define GR_ASPECT_1x4           GR_ASPECT_LOG2_1x4
#define GR_ASPECT_1x8           GR_ASPECT_LOG2_1x8

#define GR_LOD_2048             GR_LOD_LOG2_2048 /* [koolsmoky] big texture support for napalm */
#define GR_LOD_1024             GR_LOD_LOG2_1024
#define GR_LOD_512              GR_LOD_LOG2_512
#define GR_LOD_256	        GR_LOD_LOG2_256
#define GR_LOD_128	        GR_LOD_LOG2_128
#define GR_LOD_64	        GR_LOD_LOG2_64
#define GR_LOD_32	        GR_LOD_LOG2_32
#define GR_LOD_16	        GR_LOD_LOG2_16
#define GR_LOD_8	        GR_LOD_LOG2_8
#define GR_LOD_4	        GR_LOD_LOG2_4
#define GR_LOD_2	        GR_LOD_LOG2_2
#define GR_LOD_1	        GR_LOD_LOG2_1

#define GR_FOG_WITH_TABLE       GR_FOG_WITH_TABLE_ON_Q



typedef int GrSstType;

#define MAX_NUM_SST             4

enum {
      GR_SSTTYPE_VOODOO  = 0,
      GR_SSTTYPE_SST96   = 1,
      GR_SSTTYPE_AT3D    = 2,
      GR_SSTTYPE_Voodoo2 = 3,
      GR_SSTTYPE_Banshee = 4,
      GR_SSTTYPE_Voodoo3 = 5,
      GR_SSTTYPE_Voodoo4 = 6,
      GR_SSTTYPE_Voodoo5 = 7
};

typedef struct GrTMUConfig_St {
        int tmuRev;		/* Rev of Texelfx chip */
        int tmuRam;		/* 1, 2, or 4 MB */
} GrTMUConfig_t;

typedef struct GrVoodooConfig_St {
        int fbRam;		/* 1, 2, or 4 MB */
        int fbiRev;		/* Rev of Pixelfx chip */
        int nTexelfx;		/* How many texelFX chips are there? */
        GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];	/* Configuration of the Texelfx chips */
        int maxTextureSize;
        int numChips;		/* Number of Voodoo chips [koolsmoky] */
        /* Glide3 extensions */
        GrProc grSstWinOpenExt;
} GrVoodooConfig_t;

typedef struct {
        int num_sst;		/* # of HW units in the system */
        struct SstCard_St {
               GrSstType type;	/* Which hardware is it? */
               GrVoodooConfig_t VoodooConfig;
        }
        SSTs[MAX_NUM_SST];	/* configuration for each board */
} GrHwConfiguration;



typedef FxU32 GrHint_t;
#define GR_HINTTYPE_MIN         0
#define GR_HINT_STWHINT         0

typedef FxU32 GrSTWHint_t;
#define GR_STWHINT_W_DIFF_FBI   FXBIT(0)
#define GR_STWHINT_W_DIFF_TMU0  FXBIT(1)
#define GR_STWHINT_ST_DIFF_TMU0 FXBIT(2)
#define GR_STWHINT_W_DIFF_TMU1  FXBIT(3)
#define GR_STWHINT_ST_DIFF_TMU1 FXBIT(4)
#define GR_STWHINT_W_DIFF_TMU2  FXBIT(5)
#define GR_STWHINT_ST_DIFF_TMU2 FXBIT(6)

#define GR_CONTROL_ACTIVATE     1
#define GR_CONTROL_DEACTIVATE   0



#define GrState                 void



/*
** move the vertex layout defintion to application
*/
typedef struct {
        float sow;		/* s texture ordinate (s over w) */
        float tow;		/* t texture ordinate (t over w) */
        float oow;		/* 1/w (used mipmapping - really 0xfff/w) */
} GrTmuVertex;

typedef struct {
        float x, y, z;		/* X, Y, and Z of scrn space -- Z is ignored */
        float r, g, b;		/* R, G, B, ([0..255.0]) */
        float ooz;		/* 65535/Z (used for Z-buffering) */
        float a;		/* Alpha [0..255.0] */
        float oow;		/* 1/W (used for W-buffering, texturing) */
        GrTmuVertex tmuvtx[GLIDE_NUM_TMU];
} GrVertex;

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



/*
 * Write region: ToDo possible exploit the PixelPipe parameter.
 */
#define FX_grLfbWriteRegion(dst_buffer,dst_x,dst_y,src_format,src_width,src_height,src_stride,src_data)	\
        do { \
            BEGIN_BOARD_LOCK(); \
            grLfbWriteRegion(dst_buffer,\
                             dst_x,     \
                             dst_y,     \
                             src_format,\
                             src_width, \
                             src_height,\
                             FXFALSE,   \
                             src_stride,\
                             src_data);	\
            END_BOARD_LOCK(); \
        } while(0)



/*
 * Draw triangle
 */
#define FX_grDrawTriangle(a,b,c)\
  do {				\
    BEGIN_CLIP_LOOP();		\
    grDrawTriangle(a,b,c);	\
    END_CLIP_LOOP();		\
  } while (0)



/*
 * For Lod/LodLog2 conversion.
 */
#define FX_largeLodLog2(info)		(info).largeLodLog2
#define FX_aspectRatioLog2(info)	(info).aspectRatioLog2
#define FX_smallLodLog2(info)		(info).smallLodLog2
#define FX_lodToValue(val)		((int)(GR_LOD_256-val))
#define FX_largeLodValue(info)		((int)(GR_LOD_256-(info).largeLodLog2))
#define FX_smallLodValue(info)		((int)(GR_LOD_256-(info).smallLodLog2))
#define FX_valueToLod(val)		((GrLOD_t)(GR_LOD_256-val))



/*
 * ScreenWidth/Height stuff.
 */
extern int FX_grSstScreenWidth(void);
extern int FX_grSstScreenHeight(void);



/*
 * Query
 */
extern void FX_grSstPerfStats(GrSstPerfStats_t *st);
extern int FX_grSstQueryHardware(GrHwConfiguration *config);



/*
 * GrHints
 */
#define FX_grHints FX_grHints_NoLock
extern void FX_grHints_NoLock(GrHint_t hintType, FxU32 hintMask);



/*
 * Antialiashed line+point drawing.
 */
extern void FX_grAADrawLine(GrVertex *a, GrVertex *b);
extern void FX_grAADrawPoint(GrVertex *a);



/*
 * Needed for Glide3 only, to set up Glide2 compatible vertex layout.
 */
extern void FX_setupGrVertexLayout(void);



/*
 * grSstControl stuff
 */
extern FxBool FX_grSstControl(FxU32 code);



extern GrContext_t FX_grSstWinOpen(struct SstCard_St *c,
				   FxU32 hWnd,
				   GrScreenResolution_t screen_resolution,
				   GrScreenRefresh_t refresh_rate,
				   GrColorFormat_t color_format,
				   GrPixelFormat_t pixel_format,
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

#define FX_grDrawPolygonVertexList(n, v) \
   do { \
      BEGIN_CLIP_LOOP(); \
      grDrawVertexArrayContiguous(GR_POLYGON, n, v, sizeof(GrVertex)); \
      END_CLIP_LOOP(); \
   } while (0)

#define FX_grBufferClear(c, a, d)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grBufferClear(c, a, d);		\
    END_CLIP_LOOP();			\
  } while (0)


#define FX_grAADrawTriangle(a, b, c, ab, bc, ca)	\
  do {							\
    BEGIN_CLIP_LOOP();					\
    grAADrawTriangle(a, b, c, ab, bc, ca);		\
    END_CLIP_LOOP();					\
  } while (0)



#define FX_grDrawVertexArray(m, c, p)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grDrawVertexArray(m, c, p);		\
    END_CLIP_LOOP();			\
  } while (0)



#endif /* __FX_GLIDE_WARPER__ */
