/* $Id: mtypes.h,v 1.46 2001/06/13 14:56:14 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


#ifndef TYPES_H
#define TYPES_H


#include "glheader.h"
#include "config.h"		/* Hardwired parameters */
#include "glapitable.h"
#include "glthread.h"

#include "math/m_matrix.h"	/* GLmatrix */

#if defined(MESA_TRACE)
#include "Trace/tr_context.h"
#endif


/* Please try to mark derived values with a leading underscore ('_').
 */

/*
 * Color channel data type:
 */
#if CHAN_BITS == 8
   typedef GLubyte GLchan;
#define CHAN_MAX 255
#define CHAN_MAXF 255.0F
#define CHAN_TYPE GL_UNSIGNED_BYTE
#elif CHAN_BITS == 16
   typedef GLushort GLchan;
#define CHAN_MAX 65535
#define CHAN_MAXF 65535.0F
#define CHAN_TYPE GL_UNSIGNED_SHORT
#elif CHAN_BITS == 32
   typedef GLfloat GLchan;
#define CHAN_MAX 1.0
#define CHAN_MAXF 1.0F
#define CHAN_TYPE GL_FLOAT
#else
#error  illegal number of color channel bits
#endif


/*
 * Accumulation buffer data type:
 */
#if ACCUM_BITS==8
   typedef GLbyte GLaccum;
#elif ACCUM_BITS==16
   typedef GLshort GLaccum;
#else
#  error "illegal number of accumulation bits"
#endif


/*
 * Stencil buffer data type:
 */
#if STENCIL_BITS==8
   typedef GLubyte GLstencil;
#  define STENCIL_MAX 0xff
#elif STENCIL_BITS==16
   typedef GLushort GLstencil;
#  define STENCIL_MAX 0xffff
#else
#  error "illegal number of stencil bits"
#endif


/*
 * Depth buffer data type:
 */
typedef GLuint GLdepth;  /* Must be 32-bits! */


/*
 * Fixed point data type:
 */
typedef int GLfixed;



/*
 * Some forward type declarations
 */
struct _mesa_HashTable;
struct gl_texture_image;
struct gl_texture_object;
typedef struct __GLcontextRec GLcontext;
typedef struct __GLcontextModesRec GLvisual;
typedef struct gl_frame_buffer GLframebuffer;



/* Maximum number of temporary vertices required for clipping.  (Used
 * in array_cache and tnl modules).
 */
#define MAX_CLIPPED_VERTICES ((2 * (6 + MAX_CLIP_PLANES))+1)


/* Data structure for color tables */
struct gl_color_table {
   GLenum Format;         /* GL_ALPHA, GL_RGB, GL_RGB, etc */
   GLenum IntFormat;
   GLuint Size;           /* number of entries (rows) in table */
   GLvoid *Table;         /* either GLfloat * or GLchan * */
   GLboolean FloatTable;  /* are entries stored as floats? */
   GLubyte RedSize;
   GLubyte GreenSize;
   GLubyte BlueSize;
   GLubyte AlphaSize;
   GLubyte LuminanceSize;
   GLubyte IntensitySize;
};


/*
 * Bit flags used for updating material values.
 */
#define FRONT_AMBIENT_BIT     0x1
#define BACK_AMBIENT_BIT      0x2
#define FRONT_DIFFUSE_BIT     0x4
#define BACK_DIFFUSE_BIT      0x8
#define FRONT_SPECULAR_BIT   0x10
#define BACK_SPECULAR_BIT    0x20
#define FRONT_EMISSION_BIT   0x40
#define BACK_EMISSION_BIT    0x80
#define FRONT_SHININESS_BIT 0x100
#define BACK_SHININESS_BIT  0x200
#define FRONT_INDEXES_BIT   0x400
#define BACK_INDEXES_BIT    0x800

#define FRONT_MATERIAL_BITS	(FRONT_EMISSION_BIT | FRONT_AMBIENT_BIT | \
				 FRONT_DIFFUSE_BIT | FRONT_SPECULAR_BIT | \
				 FRONT_SHININESS_BIT | FRONT_INDEXES_BIT)

#define BACK_MATERIAL_BITS	(BACK_EMISSION_BIT | BACK_AMBIENT_BIT | \
				 BACK_DIFFUSE_BIT | BACK_SPECULAR_BIT | \
				 BACK_SHININESS_BIT | BACK_INDEXES_BIT)

#define ALL_MATERIAL_BITS	(FRONT_MATERIAL_BITS | BACK_MATERIAL_BITS)



/*
 * Specular exponent and material shininess lookup table sizes:
 */
#define EXP_TABLE_SIZE 512
#define SHINE_TABLE_SIZE 256

struct gl_shine_tab {
   struct gl_shine_tab *next, *prev;
   GLfloat tab[SHINE_TABLE_SIZE+1];
   GLfloat shininess;
   GLuint refcount;
};


struct gl_light {
   struct gl_light *next;	        /* double linked list with sentinel */
   struct gl_light *prev;

   GLfloat Ambient[4];		/* ambient color */
   GLfloat Diffuse[4];		/* diffuse color */
   GLfloat Specular[4];		/* specular color */
   GLfloat EyePosition[4];	/* position in eye coordinates */
   GLfloat EyeDirection[4];	/* spotlight dir in eye coordinates */
   GLfloat SpotExponent;
   GLfloat SpotCutoff;		/* in degress */
   GLfloat _CosCutoff;		/* = MAX(0, cos(SpotCutoff)) */
   GLfloat ConstantAttenuation;
   GLfloat LinearAttenuation;
   GLfloat QuadraticAttenuation;
   GLboolean Enabled;		/* On/off flag */

   /* Derived fields */
   GLuint _Flags;		/* State */

   GLfloat _Position[4];	/* position in eye/obj coordinates */
   GLfloat _VP_inf_norm[3];	/* Norm direction to infinite light */
   GLfloat _h_inf_norm[3];	/* Norm( _VP_inf_norm + <0,0,1> ) */
   GLfloat _NormDirection[4];	/* normalized spotlight direction */
   GLfloat _VP_inf_spot_attenuation;

   GLfloat _SpotExpTable[EXP_TABLE_SIZE][2];  /* to replace a pow() call */
   GLfloat _MatAmbient[2][3];	/* material ambient * light ambient */
   GLfloat _MatDiffuse[2][3];	/* material diffuse * light diffuse */
   GLfloat _MatSpecular[2][3];	/* material spec * light specular */
   GLfloat _dli;		/* CI diffuse light intensity */
   GLfloat _sli;		/* CI specular light intensity */
};


struct gl_lightmodel {
   GLfloat Ambient[4];		/* ambient color */
   GLboolean LocalViewer;	/* Local (or infinite) view point? */
   GLboolean TwoSide;		/* Two (or one) sided lighting? */
   GLenum ColorControl;		/* either GL_SINGLE_COLOR */
				/* or GL_SEPARATE_SPECULAR_COLOR */
};


struct gl_material
{
   GLfloat Ambient[4];
   GLfloat Diffuse[4];
   GLfloat Specular[4];
   GLfloat Emission[4];
   GLfloat Shininess;
   GLfloat AmbientIndex;	/* for color index lighting */
   GLfloat DiffuseIndex;	/* for color index lighting */
   GLfloat SpecularIndex;	/* for color index lighting */
};


/*
 * Attribute structures:
 *    We define a struct for each attribute group to make pushing and
 *    popping attributes easy.  Also it's a good organization.
 */
struct gl_accum_attrib {
   GLfloat ClearColor[4];	/* Accumulation buffer clear color */
};


/*
 * Used in DrawDestMask below
 */
#define FRONT_LEFT_BIT  1
#define FRONT_RIGHT_BIT 2
#define BACK_LEFT_BIT   4
#define BACK_RIGHT_BIT  8


struct gl_colorbuffer_attrib {
   GLuint ClearIndex;			/* Index to use for glClear */
   GLchan ClearColor[4];		/* Color to use for glClear */

   GLuint IndexMask;			/* Color index write mask */
   GLubyte ColorMask[4];		/* Each flag is 0xff or 0x0 */

   GLenum DrawBuffer;			/* Which buffer to draw into */
   GLenum DriverDrawBuffer;		/* Current device driver dest buffer */
   GLboolean MultiDrawBuffer;		/* Drawing to mutliple buffers? */
   GLubyte DrawDestMask;		/* bitwise-OR of bitflags above */

   /* alpha testing */
   GLboolean AlphaEnabled;		/* Alpha test enabled flag */
   GLenum AlphaFunc;			/* Alpha test function */
   GLchan AlphaRef;			/* Alpha ref value as GLchan */

   /* blending */
   GLboolean BlendEnabled;		/* Blending enabled flag */
   GLenum BlendSrcRGB;			/* Blending source operator */
   GLenum BlendDstRGB;			/* Blending destination operator */
   GLenum BlendSrcA;			/* GL_INGR_blend_func_separate */
   GLenum BlendDstA;			/* GL_INGR_blend_func_separate */
   GLenum BlendEquation;
   GLfloat BlendColor[4];

   /* logic op */
   GLenum LogicOp;			/* Logic operator */
   GLboolean IndexLogicOpEnabled;	/* Color index logic op enabled flag */
   GLboolean ColorLogicOpEnabled;	/* RGBA logic op enabled flag */

   GLboolean DitherFlag;		/* Dither enable flag */
};


struct gl_current_attrib {
   /* These values valid only when FLUSH_VERTICES has been called.
    */
   GLfloat Normal[3];				/* Current vertex normal */
   GLfloat Color[4];				/* Current RGBA color */
   GLfloat SecondaryColor[4];			/* Current secondary color */
   GLfloat FogCoord;			        /* Current Fog coord */
   GLuint Index;				/* Current color index */
   GLboolean EdgeFlag;				/* Current edge flag */
   GLfloat Texcoord[MAX_TEXTURE_UNITS][4];	/* Current texture coords */

   /* These values are always valid.
    */
   GLfloat RasterPos[4];			/* Current raster position */
   GLfloat RasterDistance;			/* Current raster distance */
   GLfloat RasterColor[4];			/* Current raster color */
   GLuint  RasterIndex;				/* Current raster index */
   GLfloat *RasterTexCoord;			/* Current raster texcoord*/
   GLfloat RasterMultiTexCoord[MAX_TEXTURE_UNITS][4];
   GLfloat RasterFogCoord;
   GLboolean RasterPosValid;			/* Raster po valid flag */
};


struct gl_depthbuffer_attrib {
   GLenum Func;			/* Function for depth buffer compare */
   GLfloat Clear;		/* Value to clear depth buffer to */
   GLboolean Test;		/* Depth buffering enabled flag */
   GLboolean Mask;		/* Depth buffer writable? */
   GLboolean OcclusionTest;	/* GL_HP_occlusion_test */
};


struct gl_enable_attrib {
   GLboolean AlphaTest;
   GLboolean AutoNormal;
   GLboolean Blend;
   GLboolean ClipPlane[MAX_CLIP_PLANES];
   GLboolean ColorMaterial;
   GLboolean Convolution1D;
   GLboolean Convolution2D;
   GLboolean Separable2D;
   GLboolean CullFace;
   GLboolean DepthTest;
   GLboolean Dither;
   GLboolean Fog;
   GLboolean Histogram;
   GLboolean Light[MAX_LIGHTS];
   GLboolean Lighting;
   GLboolean LineSmooth;
   GLboolean LineStipple;
   GLboolean IndexLogicOp;
   GLboolean ColorLogicOp;
   GLboolean Map1Color4;
   GLboolean Map1Index;
   GLboolean Map1Normal;
   GLboolean Map1TextureCoord1;
   GLboolean Map1TextureCoord2;
   GLboolean Map1TextureCoord3;
   GLboolean Map1TextureCoord4;
   GLboolean Map1Vertex3;
   GLboolean Map1Vertex4;
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;
   GLboolean MinMax;
   GLboolean Normalize;
   GLboolean PixelTexture;
   GLboolean PointSmooth;
   GLboolean PolygonOffsetPoint;
   GLboolean PolygonOffsetLine;
   GLboolean PolygonOffsetFill;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean RescaleNormals;
   GLboolean Scissor;
   GLboolean Stencil;
   GLboolean MultisampleEnabled;     /* GL_ARB_multisample */
   GLboolean SampleAlphaToCoverage;  /* GL_ARB_multisample */
   GLboolean SampleAlphaToOne;       /* GL_ARB_multisample */
   GLboolean SampleCoverage;         /* GL_ARB_multisample */
   GLboolean SampleCoverageInvert;   /* GL_ARB_multisample */
   GLuint Texture[MAX_TEXTURE_UNITS];
   GLuint TexGen[MAX_TEXTURE_UNITS];
};


struct gl_eval_attrib {
   /* Enable bits */
   GLboolean Map1Color4;
   GLboolean Map1Index;
   GLboolean Map1Normal;
   GLboolean Map1TextureCoord1;
   GLboolean Map1TextureCoord2;
   GLboolean Map1TextureCoord3;
   GLboolean Map1TextureCoord4;
   GLboolean Map1Vertex3;
   GLboolean Map1Vertex4;
   GLboolean Map2Color4;
   GLboolean Map2Index;
   GLboolean Map2Normal;
   GLboolean Map2TextureCoord1;
   GLboolean Map2TextureCoord2;
   GLboolean Map2TextureCoord3;
   GLboolean Map2TextureCoord4;
   GLboolean Map2Vertex3;
   GLboolean Map2Vertex4;
   GLboolean AutoNormal;
   /* Map Grid endpoints and divisions and calculated du values */
   GLint MapGrid1un;
   GLfloat MapGrid1u1, MapGrid1u2, MapGrid1du;
   GLint MapGrid2un, MapGrid2vn;
   GLfloat MapGrid2u1, MapGrid2u2, MapGrid2du;
   GLfloat MapGrid2v1, MapGrid2v2, MapGrid2dv;
};


struct gl_fog_attrib {
   GLboolean Enabled;		/* Fog enabled flag */
   GLfloat Color[4];		/* Fog color */
   GLfloat Density;		/* Density >= 0.0 */
   GLfloat Start;		/* Start distance in eye coords */
   GLfloat End;			/* End distance in eye coords */
   GLfloat Index;		/* Fog index */
   GLenum Mode;			/* Fog mode */
   GLboolean ColorSumEnabled;
   GLenum FogCoordinateSource;  /* GL_EXT_fog_coord */
};


struct gl_hint_attrib {
   /* always one of GL_FASTEST, GL_NICEST, or GL_DONT_CARE */
   GLenum PerspectiveCorrection;
   GLenum PointSmooth;
   GLenum LineSmooth;
   GLenum PolygonSmooth;
   GLenum Fog;
   GLenum ClipVolumeClipping;   /* GL_EXT_clip_volume_hint */
   GLenum TextureCompression;   /* GL_ARB_texture_compression */
   GLenum GenerateMipmap;       /* GL_SGIS_generate_mipmap */
};


struct gl_histogram_attrib {
   GLuint Width;				/* number of table entries */
   GLint Format;				/* GL_ALPHA, GL_RGB, etc */
   GLuint Count[HISTOGRAM_TABLE_SIZE][4];	/* the histogram */
   GLboolean Sink;				/* terminate image transfer? */
   GLubyte RedSize;				/* Bits per counter */
   GLubyte GreenSize;
   GLubyte BlueSize;
   GLubyte AlphaSize;
   GLubyte LuminanceSize;
};


struct gl_minmax_attrib {
   GLenum Format;
   GLboolean Sink;
   GLfloat Min[4], Max[4];   /* RGBA */
};


struct gl_convolution_attrib {
   GLenum Format;
   GLenum InternalFormat;
   GLuint Width;
   GLuint Height;
   GLfloat Filter[MAX_CONVOLUTION_WIDTH * MAX_CONVOLUTION_HEIGHT * 4];
};


struct gl_light_attrib {
   struct gl_light Light[MAX_LIGHTS];	/* Array of lights */
   struct gl_lightmodel Model;		/* Lighting model */

   /* Must flush FLUSH_VERTICES before referencing:
    */
   struct gl_material Material[2];	/* Material 0=front, 1=back */

   GLboolean Enabled;			/* Lighting enabled flag */
   GLenum ShadeModel;			/* GL_FLAT or GL_SMOOTH */
   GLenum ColorMaterialFace;		/* GL_FRONT, BACK or FRONT_AND_BACK */
   GLenum ColorMaterialMode;		/* GL_AMBIENT, GL_DIFFUSE, etc */
   GLuint ColorMaterialBitmask;		/* bitmask formed from Face and Mode */
   GLboolean ColorMaterialEnabled;

   struct gl_light EnabledList;         /* List sentinel */

   /* Derived for optimizations: */
   GLboolean _NeedVertices;		/* Use fast shader? */
   GLuint  _Flags;		        /* LIGHT_* flags, see below */
   GLfloat _BaseColor[2][3];
   GLfloat _BaseAlpha[2];
};


#define LIGHT_SPOT         0x1
#define LIGHT_LOCAL_VIEWER 0x2
#define LIGHT_POSITIONAL   0x4

#define LIGHT_NEED_VERTICES (LIGHT_POSITIONAL|LIGHT_LOCAL_VIEWER)

struct gl_line_attrib {
   GLboolean SmoothFlag;	/* GL_LINE_SMOOTH enabled? */
   GLboolean StippleFlag;	/* GL_LINE_STIPPLE enabled? */
   GLushort StipplePattern;	/* Stipple pattern */
   GLint StippleFactor;		/* Stipple repeat factor */
   GLfloat Width;		/* Line width */
   GLfloat _Width;		/* Clamped Line width */
};


struct gl_list_attrib {
   GLuint ListBase;
};

struct gl_list_opcode {
   GLuint size;
   void (*execute)( GLcontext *ctx, void *data );
   void (*destroy)( GLcontext *ctx, void *data );
   void (*print)( GLcontext *ctx, void *data );
};

#define GL_MAX_EXT_OPCODES 16

struct gl_list_extensions {
   struct gl_list_opcode opcode[GL_MAX_EXT_OPCODES];
   GLuint nr_opcodes;
};


struct gl_multisample_attrib {
   GLboolean Enabled;
   GLboolean SampleAlphaToCoverage;
   GLboolean SampleAlphaToOne;
   GLboolean SampleCoverage;
   GLfloat SampleCoverageValue;
   GLboolean SampleCoverageInvert;
};


struct gl_pixel_attrib {
   GLenum ReadBuffer;		/* src buffer for glRead/CopyPixels */
   GLenum DriverReadBuffer;	/* Driver's current source buffer */
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLint IndexShift, IndexOffset;
   GLboolean MapColorFlag;
   GLboolean MapStencilFlag;
   GLfloat ZoomX, ZoomY;
   GLint MapStoSsize;		/* Size of each pixel map */
   GLint MapItoIsize;
   GLint MapItoRsize;
   GLint MapItoGsize;
   GLint MapItoBsize;
   GLint MapItoAsize;
   GLint MapRtoRsize;
   GLint MapGtoGsize;
   GLint MapBtoBsize;
   GLint MapAtoAsize;
   GLint MapStoS[MAX_PIXEL_MAP_TABLE];	/* Pixel map tables */
   GLint MapItoI[MAX_PIXEL_MAP_TABLE];
   GLfloat MapItoR[MAX_PIXEL_MAP_TABLE];
   GLfloat MapItoG[MAX_PIXEL_MAP_TABLE];
   GLfloat MapItoB[MAX_PIXEL_MAP_TABLE];
   GLfloat MapItoA[MAX_PIXEL_MAP_TABLE];
   GLubyte MapItoR8[MAX_PIXEL_MAP_TABLE];  /* converted to 8-bit color */
   GLubyte MapItoG8[MAX_PIXEL_MAP_TABLE];
   GLubyte MapItoB8[MAX_PIXEL_MAP_TABLE];
   GLubyte MapItoA8[MAX_PIXEL_MAP_TABLE];
   GLfloat MapRtoR[MAX_PIXEL_MAP_TABLE];
   GLfloat MapGtoG[MAX_PIXEL_MAP_TABLE];
   GLfloat MapBtoB[MAX_PIXEL_MAP_TABLE];
   GLfloat MapAtoA[MAX_PIXEL_MAP_TABLE];
   /* GL_EXT_histogram */
   GLboolean HistogramEnabled;
   GLboolean MinMaxEnabled;
   /* GL_SGIS_pixel_texture */
   GLboolean PixelTextureEnabled;
   GLenum FragmentRgbSource;
   GLenum FragmentAlphaSource;
   /* GL_SGI_color_matrix */
   GLfloat PostColorMatrixScale[4];  /* RGBA */
   GLfloat PostColorMatrixBias[4];   /* RGBA */
   /* GL_SGI_color_table */
   GLfloat ColorTableScale[4];
   GLfloat ColorTableBias[4];
   GLboolean ColorTableEnabled;
   GLfloat PCCTscale[4];
   GLfloat PCCTbias[4];
   GLboolean PostConvolutionColorTableEnabled;
   GLfloat PCMCTscale[4];
   GLfloat PCMCTbias[4];
   GLboolean PostColorMatrixColorTableEnabled;
   /* Convolution */
   GLboolean Convolution1DEnabled;
   GLboolean Convolution2DEnabled;
   GLboolean Separable2DEnabled;
   GLfloat ConvolutionBorderColor[3][4];
   GLenum ConvolutionBorderMode[3];
   GLfloat ConvolutionFilterScale[3][4];
   GLfloat ConvolutionFilterBias[3][4];
   GLfloat PostConvolutionScale[4];  /* RGBA */
   GLfloat PostConvolutionBias[4];   /* RGBA */
};


struct gl_point_attrib {
   GLboolean SmoothFlag;	/* True if GL_POINT_SMOOTH is enabled */
   GLboolean SpriteMode;	/* GL_MESA_sprite_point extension */
   GLfloat Size;		/* User-specified point size */
   GLfloat _Size;		/* Size clamped to Const.Min/MaxPointSize */
   GLfloat Params[3];		/* GL_EXT_point_parameters */
   GLfloat MinSize, MaxSize;	/* GL_EXT_point_parameters */
   GLfloat Threshold;		/* GL_EXT_point_parameters */
   GLboolean _Attenuated;	/* True if Params != [1, 0, 0] */
};


struct gl_polygon_attrib {
   GLenum FrontFace;		/* Either GL_CW or GL_CCW */
   GLenum FrontMode;		/* Either GL_POINT, GL_LINE or GL_FILL */
   GLenum BackMode;		/* Either GL_POINT, GL_LINE or GL_FILL */
   GLboolean _FrontBit;		/*  */
   GLboolean CullFlag;		/* Culling on/off flag */
   GLboolean SmoothFlag;	/* True if GL_POLYGON_SMOOTH is enabled */
   GLboolean StippleFlag;	/* True if GL_POLYGON_STIPPLE is enabled */
   GLenum CullFaceMode;		/* Culling mode GL_FRONT or GL_BACK */
   GLfloat OffsetFactor;	/* Polygon offset factor, from user */
   GLfloat OffsetUnits;		/* Polygon offset units, from user */
   GLfloat OffsetMRD;		/* = OffsetUnits * visual->MRD */
   GLboolean OffsetPoint;	/* Offset in GL_POINT mode */
   GLboolean OffsetLine;	/* Offset in GL_LINE mode */
   GLboolean OffsetFill;	/* Offset in GL_FILL mode */
   GLboolean _OffsetAny;
};


struct gl_scissor_attrib {
   GLboolean Enabled;		/* Scissor test enabled? */
   GLint X, Y;			/* Lower left corner of box */
   GLsizei Width, Height;	/* Size of box */
};


struct gl_stencil_attrib {
   GLboolean Enabled;		/* Enabled flag */
   GLenum Function;		/* Stencil function */
   GLenum FailFunc;		/* Fail function */
   GLenum ZPassFunc;		/* Depth buffer pass function */
   GLenum ZFailFunc;		/* Depth buffer fail function */
   GLstencil Ref;		/* Reference value */
   GLstencil ValueMask;		/* Value mask */
   GLstencil Clear;		/* Clear value */
   GLstencil WriteMask;		/* Write mask */
};


/* TexGenEnabled flags */
#define S_BIT 1
#define T_BIT 2
#define R_BIT 4
#define Q_BIT 8

/* Texture Enabled flags */
#define TEXTURE0_1D   0x1     /* Texture unit 0 (default) */
#define TEXTURE0_2D   0x2
#define TEXTURE0_3D   0x4
#define TEXTURE0_CUBE 0x8
#define TEXTURE0_ANY  (TEXTURE0_1D | TEXTURE0_2D | TEXTURE0_3D | TEXTURE0_CUBE)
#define TEXTURE1_1D   (TEXTURE0_1D << 4)    /* Texture unit 1 */
#define TEXTURE1_2D   (TEXTURE0_2D << 4)
#define TEXTURE1_3D   (TEXTURE0_3D << 4)
#define TEXTURE1_CUBE (TEXTURE0_CUBE << 4)
#define TEXTURE1_ANY  (TEXTURE1_1D | TEXTURE1_2D | TEXTURE1_3D | TEXTURE1_CUBE)
#define TEXTURE2_1D   (TEXTURE0_1D << 8)    /* Texture unit 2 */
#define TEXTURE2_2D   (TEXTURE0_2D << 8)
#define TEXTURE2_3D   (TEXTURE0_3D << 8)
#define TEXTURE2_CUBE (TEXTURE0_CUBE << 8)
#define TEXTURE2_ANY  (TEXTURE2_1D | TEXTURE2_2D | TEXTURE2_3D | TEXTURE2_CUBE)
#define TEXTURE3_1D   (TEXTURE0_1D << 12)    /* Texture unit 3 */
#define TEXTURE3_2D   (TEXTURE0_2D << 12)
#define TEXTURE3_3D   (TEXTURE0_3D << 12)
#define TEXTURE3_CUBE (TEXTURE0_CUBE << 12)
#define TEXTURE3_ANY  (TEXTURE3_1D | TEXTURE3_2D | TEXTURE3_3D | TEXTURE3_CUBE)
#define TEXTURE4_1D   (TEXTURE0_1D << 16)    /* Texture unit 4 */
#define TEXTURE4_2D   (TEXTURE0_2D << 16)
#define TEXTURE4_3D   (TEXTURE0_3D << 16)
#define TEXTURE4_CUBE (TEXTURE0_CUBE << 16)
#define TEXTURE5_ANY  (TEXTURE3_1D | TEXTURE3_2D | TEXTURE3_3D | TEXTURE3_CUBE)
#define TEXTURE5_1D   (TEXTURE0_1D << 20)    /* Texture unit 5 */
#define TEXTURE5_2D   (TEXTURE0_2D << 20)
#define TEXTURE5_3D   (TEXTURE0_3D << 20)
#define TEXTURE5_CUBE (TEXTURE0_CUBE << 20)
#define TEXTURE5_ANY  (TEXTURE3_1D | TEXTURE3_2D | TEXTURE3_3D | TEXTURE3_CUBE)
#define TEXTURE6_1D   (TEXTURE0_1D << 24)    /* Texture unit 6 */
#define TEXTURE6_2D   (TEXTURE0_2D << 24)
#define TEXTURE6_3D   (TEXTURE0_3D << 24)
#define TEXTURE6_CUBE (TEXTURE0_CUBE << 24)
#define TEXTURE6_ANY  (TEXTURE3_1D | TEXTURE3_2D | TEXTURE3_3D | TEXTURE3_CUBE)
#define TEXTURE7_1D   (TEXTURE0_1D << 28)    /* Texture unit 7 */
#define TEXTURE7_2D   (TEXTURE0_2D << 28)
#define TEXTURE7_3D   (TEXTURE0_3D << 28)
#define TEXTURE7_CUBE (TEXTURE0_CUBE << 28)
#define TEXTURE7_ANY  (TEXTURE3_1D | TEXTURE3_2D | TEXTURE3_3D | TEXTURE3_CUBE)

/* Bitmap versions of the GL_ constants.
 */
#define TEXGEN_SPHERE_MAP        0x1
#define TEXGEN_OBJ_LINEAR        0x2
#define TEXGEN_EYE_LINEAR        0x4
#define TEXGEN_REFLECTION_MAP_NV 0x8
#define TEXGEN_NORMAL_MAP_NV     0x10

#define TEXGEN_NEED_NORMALS      (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV)
#define TEXGEN_NEED_EYE_COORD    (TEXGEN_SPHERE_MAP        | \
				  TEXGEN_REFLECTION_MAP_NV | \
				  TEXGEN_NORMAL_MAP_NV     | \
				  TEXGEN_EYE_LINEAR)



/* A selection of state flags to make driver and module's lives easier.
 */
#define ENABLE_TEXGEN0        0x1
#define ENABLE_TEXGEN1        0x2
#define ENABLE_TEXGEN2        0x4
#define ENABLE_TEXGEN3        0x8
#define ENABLE_TEXGEN4        0x10
#define ENABLE_TEXGEN5        0x20
#define ENABLE_TEXGEN6        0x40
#define ENABLE_TEXGEN7        0x80

#define ENABLE_TEXMAT0        0x1	/* Ie. not the identity matrix */
#define ENABLE_TEXMAT1        0x2
#define ENABLE_TEXMAT2        0x4
#define ENABLE_TEXMAT3        0x8
#define ENABLE_TEXMAT4        0x10
#define ENABLE_TEXMAT5        0x20
#define ENABLE_TEXMAT6        0x40
#define ENABLE_TEXMAT7        0x80

#define ENABLE_TEXGEN(i) (ENABLE_TEXGEN0 << (i))
#define ENABLE_TEXMAT(i) (ENABLE_TEXMAT0 << (i))

/*
 * If teximage is color-index, texelOut returns GLchan[1].
 * If teximage is depth, texelOut returns GLfloat[1].
 * Otherwise, texelOut returns GLchan[4].
 */
typedef void (*FetchTexelFunc)( const struct gl_texture_image *texImage,
                                GLint col, GLint row, GLint img,
                                GLvoid *texelOut );

/* Texture format record */
struct gl_texture_format {
   GLint MesaFormat;		/* One of the MESA_FORMAT_* values */

   GLenum BaseFormat;		/* Either GL_ALPHA, GL_INTENSITY, GL_LUMINANCE,
                                 * GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA,
                                 * GL_COLOR_INDEX or GL_DEPTH_COMPONENT.
                                 */
   GLenum Type;			/* Internal type as GL enum value */

   GLubyte RedBits;		/* Bits per texel component */
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte LuminanceBits;
   GLubyte IntensityBits;
   GLubyte IndexBits;
   GLubyte DepthBits;

   GLint TexelBytes;

   FetchTexelFunc FetchTexel1D;	/* Texel fetch function pointers */
   FetchTexelFunc FetchTexel2D;
   FetchTexelFunc FetchTexel3D;
};

/* Texture image record */
struct gl_texture_image {
   GLenum Format;		/* GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA,
				 * GL_INTENSITY, GL_RGB, GL_RGBA,
                                 * GL_COLOR_INDEX or GL_DEPTH_COMPONENT only.
				 */
   GLint IntFormat;		/* Internal format as given by the user */
   GLuint Border;		/* 0 or 1 */
   GLuint Width;		/* = 2^WidthLog2 + 2*Border */
   GLuint Height;		/* = 2^HeightLog2 + 2*Border */
   GLuint Depth;		/* = 2^DepthLog2 + 2*Border */
   GLuint Width2;		/* = Width - 2*Border */
   GLuint Height2;		/* = Height - 2*Border */
   GLuint Depth2;		/* = Depth - 2*Border */
   GLuint WidthLog2;		/* = log2(Width2) */
   GLuint HeightLog2;		/* = log2(Height2) */
   GLuint DepthLog2;		/* = log2(Depth2) */
   GLuint MaxLog2;		/* = MAX(WidthLog2, HeightLog2) */
   GLvoid *Data;		/* Image data, accessed via FetchTexel() */

   const struct gl_texture_format *TexFormat;

   FetchTexelFunc FetchTexel;	/* Texel fetch function pointer */

   GLboolean IsCompressed;	/* GL_ARB_texture_compression */
   GLuint CompressedSize;	/* GL_ARB_texture_compression */

   /* For device driver: */
   void *DriverData;		/* Arbitrary device driver data */
};


/* Texture object record */
struct gl_texture_object {
   _glthread_Mutex Mutex;	/* for thread safety */
   GLint RefCount;		/* reference count */
   GLuint Name;			/* an unsigned integer */
   GLuint Dimensions;		/* 1 or 2 or 3 or 6 (cube map) */
   GLfloat Priority;		/* in [0,1] */
   GLchan BorderColor[4];
   GLenum WrapS;		/* Wrap modes are: GL_CLAMP, REPEAT */
   GLenum WrapT;		/*   GL_CLAMP_TO_EDGE, and          */
   GLenum WrapR;		/*   GL_CLAMP_TO_BORDER_ARB         */
   GLenum MinFilter;		/* minification filter */
   GLenum MagFilter;		/* magnification filter */
   GLfloat MinLod;		/* min lambda, OpenGL 1.2 */
   GLfloat MaxLod;		/* max lambda, OpenGL 1.2 */
   GLint BaseLevel;		/* min mipmap level, OpenGL 1.2 */
   GLint MaxLevel;		/* max mipmap level, OpenGL 1.2 */
   GLfloat MaxAnisotropy;	/* GL_EXT_texture_filter_anisotropic */
   GLboolean CompareFlag;	/* GL_SGIX_shadow */
   GLenum CompareOperator;	/* GL_SGIX_shadow */
   GLchan ShadowAmbient;	/* GL_SGIX_shadow_ambient */
   GLint _MaxLevel;		/* actual max mipmap level (q in the spec) */
   GLfloat _MaxLambda;		/* = _MaxLevel - BaseLevel (q - b in spec) */
   GLboolean GenerateMipmap;    /* GL_SGIS_generate_mipmap */

   struct gl_texture_image *Image[MAX_TEXTURE_LEVELS];

   /* Texture cube faces */
   /* Image[] is alias for *PosX[MAX_TEXTURE_LEVELS]; */
   struct gl_texture_image *NegX[MAX_TEXTURE_LEVELS];
   struct gl_texture_image *PosY[MAX_TEXTURE_LEVELS];
   struct gl_texture_image *NegY[MAX_TEXTURE_LEVELS];
   struct gl_texture_image *PosZ[MAX_TEXTURE_LEVELS];
   struct gl_texture_image *NegZ[MAX_TEXTURE_LEVELS];

   /* GL_EXT_paletted_texture */
   struct gl_color_table Palette;

   GLboolean Complete;			/* Is texture object complete? */
   struct gl_texture_object *Next;	/* Next in linked list */

   /* For device driver: */
   void *DriverData;	/* Arbitrary device driver data */
};



/*
 * Texture units are new with the multitexture extension.
 */
struct gl_texture_unit {
   GLuint Enabled;              /* bitmask of TEXTURE0_1D, _2D, _3D, _CUBE */
   GLuint _ReallyEnabled;       /* 0 or one of TEXTURE0_1D, _2D, _3D, _CUBE */

   GLenum EnvMode;              /* GL_MODULATE, GL_DECAL, GL_BLEND, etc. */
   GLfloat EnvColor[4];
   GLuint TexGenEnabled;	/* Bitwise-OR of [STRQ]_BIT values */
   GLenum GenModeS;		/* Tex coord generation mode, either */
   GLenum GenModeT;		/*	GL_OBJECT_LINEAR, or */
   GLenum GenModeR;		/*	GL_EYE_LINEAR, or    */
   GLenum GenModeQ;		/*      GL_SPHERE_MAP        */
   GLuint _GenBitS;
   GLuint _GenBitT;
   GLuint _GenBitR;
   GLuint _GenBitQ;
   GLuint _GenFlags;		/* bitwise or of GenBit[STRQ] */
   GLfloat ObjectPlaneS[4];
   GLfloat ObjectPlaneT[4];
   GLfloat ObjectPlaneR[4];
   GLfloat ObjectPlaneQ[4];
   GLfloat EyePlaneS[4];
   GLfloat EyePlaneT[4];
   GLfloat EyePlaneR[4];
   GLfloat EyePlaneQ[4];
   GLfloat LodBias;		/* for biasing mipmap levels */

   /* GL_EXT_texture_env_combine */
   GLenum CombineModeRGB;       /* GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum CombineModeA;         /* GL_REPLACE, GL_DECAL, GL_ADD, etc. */
   GLenum CombineSourceRGB[3];  /* GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
   GLenum CombineSourceA[3];    /* GL_PRIMARY_COLOR, GL_TEXTURE, etc. */
   GLenum CombineOperandRGB[3]; /* SRC_COLOR, ONE_MINUS_SRC_COLOR, etc */
   GLenum CombineOperandA[3];   /* SRC_ALPHA, ONE_MINUS_SRC_ALPHA, etc */
   GLuint CombineScaleShiftRGB; /* 0, 1 or 2 */
   GLuint CombineScaleShiftA;   /* 0, 1 or 2 */

   struct gl_texture_object *Current1D;
   struct gl_texture_object *Current2D;
   struct gl_texture_object *Current3D;
   struct gl_texture_object *CurrentCubeMap; /* GL_ARB_texture_cube_map */

   struct gl_texture_object *_Current; /* Points to really enabled tex obj */

   struct gl_texture_object Saved1D;  /* only used by glPush/PopAttrib */
   struct gl_texture_object Saved2D;
   struct gl_texture_object Saved3D;
   struct gl_texture_object SavedCubeMap;
};


struct gl_texture_attrib {
   /* multitexture */
   GLuint CurrentUnit;	          /* Active texture unit */

   GLuint _ReallyEnabled;     /* enables for all texture units: */
                             /* = (Unit[0]._ReallyEnabled << 0) | */
                             /*   (Unit[1]._ReallyEnabled << 4) | */
	                     /*   (Unit[2]._ReallyEnabled << 8) | etc... */

   GLuint _GenFlags;  /* for texgen */
   GLuint _TexGenEnabled;	
   GLuint _TexMatEnabled;

   struct gl_texture_unit Unit[MAX_TEXTURE_UNITS];

   struct gl_texture_object *Proxy1D;
   struct gl_texture_object *Proxy2D;
   struct gl_texture_object *Proxy3D;
   struct gl_texture_object *ProxyCubeMap;

   /* GL_EXT_shared_texture_palette */
   GLboolean SharedPalette;
   struct gl_color_table Palette;
};


struct gl_transform_attrib {
   GLenum MatrixMode;				/* Matrix mode */
   GLfloat EyeUserPlane[MAX_CLIP_PLANES][4];
   GLfloat _ClipUserPlane[MAX_CLIP_PLANES][4];	/* derived */
   GLboolean ClipEnabled[MAX_CLIP_PLANES];
   GLubyte   _AnyClip;				/* How many ClipEnabled? */
   GLboolean Normalize;				/* Normalize all normals? */
   GLboolean RescaleNormals;			/* GL_EXT_rescale_normal */
};


struct gl_viewport_attrib {
   GLint X, Y;			/* position */
   GLsizei Width, Height;	/* size */
   GLfloat Near, Far;		/* Depth buffer range */
   GLmatrix _WindowMap;		/* Mapping transformation as a matrix. */
};


/* For the attribute stack: */
struct gl_attrib_node {
   GLbitfield kind;
   void *data;
   struct gl_attrib_node *next;
};


/*
 * Client pixel packing/unpacking attributes
 */
struct gl_pixelstore_attrib {
   GLint Alignment;
   GLint RowLength;
   GLint SkipPixels;
   GLint SkipRows;
   GLint ImageHeight;     /* for GL_EXT_texture3D */
   GLint SkipImages;      /* for GL_EXT_texture3D */
   GLboolean SwapBytes;
   GLboolean LsbFirst;
};


#define CA_CLIENT_DATA     0x1	/* Data not alloced by mesa */


/*
 * Client vertex array attributes
 */
struct gl_client_array {
   GLint Size;
   GLenum Type;
   GLsizei Stride;		/* user-specified stride */
   GLsizei StrideB;		/* actual stride in bytes */
   void *Ptr;
   GLuint Flags;
   GLboolean Enabled;
};


struct gl_array_attrib {
   struct gl_client_array Vertex;	     /* client data descriptors */
   struct gl_client_array Normal;
   struct gl_client_array Color;
   struct gl_client_array SecondaryColor;
   struct gl_client_array FogCoord;
   struct gl_client_array Index;
   struct gl_client_array TexCoord[MAX_TEXTURE_UNITS];
   struct gl_client_array EdgeFlag;

   GLint TexCoordInterleaveFactor;
   GLint ActiveTexture;		/* Client Active Texture */
   GLuint LockFirst;
   GLuint LockCount;

   GLuint _Enabled;		/* _NEW_ARRAY_* - bit set if array enabled */
   GLuint NewState;		/* _NEW_ARRAY_* */
};


struct gl_feedback {
   GLenum Type;
   GLuint _Mask;		/* FB_* bits */
   GLfloat *Buffer;
   GLuint BufferSize;
   GLuint Count;
};


struct gl_selection {
   GLuint *Buffer;
   GLuint BufferSize;	/* size of SelectBuffer */
   GLuint BufferCount;	/* number of values in SelectBuffer */
   GLuint Hits;		/* number of records in SelectBuffer */
   GLuint NameStackDepth;
   GLuint NameStack[MAX_NAME_STACK_DEPTH];
   GLboolean HitFlag;
   GLfloat HitMinZ, HitMaxZ;
};


/*
 * 1-D Evaluator control points
 */
struct gl_1d_map {
   GLuint Order;	/* Number of control points */
   GLfloat u1, u2, du;	/* u1, u2, 1.0/(u2-u1) */
   GLfloat *Points;	/* Points to contiguous control points */
};


/*
 * 2-D Evaluator control points
 */
struct gl_2d_map {
   GLuint Uorder;		/* Number of control points in U dimension */
   GLuint Vorder;		/* Number of control points in V dimension */
   GLfloat u1, u2, du;
   GLfloat v1, v2, dv;
   GLfloat *Points;		/* Points to contiguous control points */
};


/*
 * All evalutator control points
 */
struct gl_evaluators {
   /* 1-D maps */
   struct gl_1d_map Map1Vertex3;
   struct gl_1d_map Map1Vertex4;
   struct gl_1d_map Map1Index;
   struct gl_1d_map Map1Color4;
   struct gl_1d_map Map1Normal;
   struct gl_1d_map Map1Texture1;
   struct gl_1d_map Map1Texture2;
   struct gl_1d_map Map1Texture3;
   struct gl_1d_map Map1Texture4;

   /* 2-D maps */
   struct gl_2d_map Map2Vertex3;
   struct gl_2d_map Map2Vertex4;
   struct gl_2d_map Map2Index;
   struct gl_2d_map Map2Color4;
   struct gl_2d_map Map2Normal;
   struct gl_2d_map Map2Texture1;
   struct gl_2d_map Map2Texture2;
   struct gl_2d_map Map2Texture3;
   struct gl_2d_map Map2Texture4;
};


/*
 * State which can be shared by multiple contexts:
 */
struct gl_shared_state {
   _glthread_Mutex Mutex;		   /* for thread safety */
   GLint RefCount;			   /* Reference count */
   struct _mesa_HashTable *DisplayList;	   /* Display lists hash table */
   struct _mesa_HashTable *TexObjects;	   /* Texture objects hash table */
   struct gl_texture_object *TexObjectList;/* Linked list of texture objects */

   /* Default texture objects (shared by all multi-texture units) */
   struct gl_texture_object *Default1D;
   struct gl_texture_object *Default2D;
   struct gl_texture_object *Default3D;
   struct gl_texture_object *DefaultCubeMap;

   void *DriverData;  /* Device driver shared state */
};


/*
 * A "frame buffer" is a color buffer and its optional ancillary buffers:
 * depth, accum, stencil, and software-simulated alpha buffers.
 * In C++ terms, think of this as a base class from which device drivers
 * will make derived classes.
 */
struct gl_frame_buffer {
   GLvisual Visual;		/* The corresponding visual */

   GLint Width, Height;		/* size of frame buffer in pixels */

   GLboolean UseSoftwareDepthBuffer;
   GLboolean UseSoftwareAccumBuffer;
   GLboolean UseSoftwareStencilBuffer;
   GLboolean UseSoftwareAlphaBuffers;

   /* Software depth (aka Z) buffer */
   GLvoid *DepthBuffer;		/* array [Width*Height] of GLushort or GLuint*/

   /* Software stencil buffer */
   GLstencil *Stencil;		/* array [Width*Height] of GLstencil values */

   /* Software accumulation buffer */
   GLaccum *Accum;		/* array [4*Width*Height] of GLaccum values */

   /* Software alpha planes */
   GLchan *FrontLeftAlpha;	/* array [Width*Height] of GLubyte */
   GLchan *BackLeftAlpha;	/* array [Width*Height] of GLubyte */
   GLchan *FrontRightAlpha;	/* array [Width*Height] of GLubyte */
   GLchan *BackRightAlpha;	/* array [Width*Height] of GLubyte */
   GLchan *Alpha;		/* Points to current alpha buffer */

   /* Drawing bounds: intersection of window size and scissor box */
   GLint _Xmin, _Ymin;  /* inclusive */
   GLint _Xmax, _Ymax;  /* exclusive */
};


/*
 * Constants which may be overriden by device driver during context creation
 * but are never changed after that.
 */
struct gl_constants {
   GLint MaxTextureLevels;
   GLint Max3DTextureLevels;
   GLint MaxCubeTextureLevels;
   GLuint MaxTextureUnits;
   GLfloat MaxTextureMaxAnisotropy;	/* GL_EXT_texture_filter_anisotropic */
   GLuint MaxArrayLockSize;
   GLint SubPixelBits;
   GLfloat MinPointSize, MaxPointSize;		/* aliased */
   GLfloat MinPointSizeAA, MaxPointSizeAA;	/* antialiased */
   GLfloat PointSizeGranularity;
   GLfloat MinLineWidth, MaxLineWidth;		/* aliased */
   GLfloat MinLineWidthAA, MaxLineWidthAA;	/* antialiased */
   GLfloat LineWidthGranularity;
   GLuint NumAuxBuffers;
   GLuint MaxColorTableSize;
   GLuint MaxConvolutionWidth;
   GLuint MaxConvolutionHeight;
   GLuint NumCompressedTextureFormats;	/* GL_ARB_texture_compression */
   GLenum CompressedTextureFormats[MAX_COMPRESSED_TEXTURE_FORMATS];
   GLuint MaxClipPlanes;
   GLuint MaxLights;
};


/*
 * List of extensions.
 */
struct extension;
struct gl_extensions {
   char *ext_string;
   struct extension *ext_list;
   /* Flags to quickly test if certain extensions are available.
    * Not every extension needs to have such a flag, but it's encouraged.
    */
   GLboolean ARB_imaging;
   GLboolean ARB_multisample;
   GLboolean ARB_multitexture;
   GLboolean ARB_texture_border_clamp;
   GLboolean ARB_texture_compression;
   GLboolean ARB_texture_cube_map;
   GLboolean ARB_texture_env_combine;
   GLboolean ARB_texture_env_dot3;
   GLboolean EXT_blend_color;
   GLboolean EXT_blend_func_separate;
   GLboolean EXT_blend_logic_op;
   GLboolean EXT_blend_minmax;
   GLboolean EXT_blend_subtract;
   GLboolean EXT_clip_volume_hint;
   GLboolean EXT_convolution;
   GLboolean EXT_compiled_vertex_array;
   GLboolean EXT_fog_coord;
   GLboolean EXT_histogram;
   GLboolean EXT_packed_pixels;
   GLboolean EXT_paletted_texture;
   GLboolean EXT_point_parameters;
   GLboolean EXT_polygon_offset;
   GLboolean EXT_rescale_normal;
   GLboolean EXT_secondary_color;
   GLboolean EXT_shared_texture_palette;
   GLboolean EXT_stencil_wrap;
   GLboolean EXT_texture3D;
   GLboolean EXT_texture_compression_s3tc;
   GLboolean EXT_texture_env_add;
   GLboolean EXT_texture_env_combine;
   GLboolean EXT_texture_env_dot3;
   GLboolean EXT_texture_filter_anisotropic;
   GLboolean EXT_texture_object;
   GLboolean EXT_texture_lod_bias;
   GLboolean EXT_vertex_array_set;
   GLboolean HP_occlusion_test;
   GLboolean INGR_blend_func_separate;
   GLboolean MESA_window_pos;
   GLboolean MESA_resize_buffers;
   GLboolean MESA_sprite_point;
   GLboolean NV_blend_square;
   GLboolean NV_texgen_reflection;
   GLboolean SGI_color_matrix;
   GLboolean SGI_color_table;
   GLboolean SGIS_generate_mipmap;
   GLboolean SGIS_pixel_texture;
   GLboolean SGIS_texture_edge_clamp;
   GLboolean SGIX_depth_texture;
   GLboolean SGIX_pixel_texture;
   GLboolean SGIX_shadow;
   GLboolean SGIX_shadow_ambient;
   GLboolean _3DFX_texture_compression_FXT1;
};



/*
 * Bits for image transfer operations (ctx->ImageTransferState).
 */
#define IMAGE_SCALE_BIAS_BIT                      0x1
#define IMAGE_SHIFT_OFFSET_BIT                    0x2
#define IMAGE_MAP_COLOR_BIT                       0x4
#define IMAGE_COLOR_TABLE_BIT                     0x8
#define IMAGE_CONVOLUTION_BIT                     0x10
#define IMAGE_POST_CONVOLUTION_SCALE_BIAS         0x20
#define IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT    0x40
#define IMAGE_COLOR_MATRIX_BIT                    0x80
#define IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT   0x100
#define IMAGE_HISTOGRAM_BIT                       0x200
#define IMAGE_MIN_MAX_BIT                         0x400

/* transfer ops up to convolution: */
#define IMAGE_PRE_CONVOLUTION_BITS (IMAGE_SCALE_BIAS_BIT |     \
                                    IMAGE_SHIFT_OFFSET_BIT |   \
                                    IMAGE_MAP_COLOR_BIT |      \
                                    IMAGE_COLOR_TABLE_BIT)

/* transfer ops after convolution: */
#define IMAGE_POST_CONVOLUTION_BITS (IMAGE_POST_CONVOLUTION_SCALE_BIAS |      \
                                     IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT | \
                                     IMAGE_COLOR_MATRIX_BIT |                 \
                                     IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT |\
                                     IMAGE_HISTOGRAM_BIT |                    \
                                     IMAGE_MIN_MAX_BIT)


/*
 * Bits to indicate what state has changed.  6 unused flags.
 */
#define _NEW_MODELVIEW		0x1        /* ctx->ModelView */
#define _NEW_PROJECTION		0x2        /* ctx->Projection */
#define _NEW_TEXTURE_MATRIX	0x4        /* ctx->TextureMatrix */
#define _NEW_COLOR_MATRIX	0x8        /* ctx->ColorMatrix */
#define _NEW_ACCUM		0x10       /* ctx->Accum */
#define _NEW_COLOR		0x20       /* ctx->Color */
#define _NEW_DEPTH		0x40       /* ctx->Depth */
#define _NEW_EVAL		0x80       /* ctx->Eval, ctx->EvalMap */
#define _NEW_FOG		0x100      /* ctx->Fog */
#define _NEW_HINT		0x200      /* ctx->Hint */
#define _NEW_LIGHT		0x400      /* ctx->Light */
#define _NEW_LINE		0x800      /* ctx->Line */
#define _NEW_PIXEL		0x1000     /* ctx->Pixel */
#define _NEW_POINT		0x2000     /* ctx->Point */
#define _NEW_POLYGON		0x4000     /* ctx->Polygon */
#define _NEW_POLYGONSTIPPLE	0x8000     /* ctx->PolygonStipple */
#define _NEW_SCISSOR		0x10000    /* ctx->Scissor */
#define _NEW_STENCIL		0x20000    /* ctx->Stencil */
#define _NEW_TEXTURE		0x40000    /* ctx->Texture */
#define _NEW_TRANSFORM		0x80000    /* ctx->Transform */
#define _NEW_VIEWPORT		0x100000   /* ctx->Viewport */
#define _NEW_PACKUNPACK		0x200000   /* ctx->Pack, ctx->Unpack */
#define _NEW_ARRAY	        0x400000   /* ctx->Array */
#define _NEW_RENDERMODE		0x800000   /* RenderMode, Feedback, Select */
#define _NEW_BUFFERS            0x1000000  /* ctx->Visual, ctx->DrawBuffer, */
#define _NEW_MULTISAMPLE        0x2000000  /* ctx->Multisample */
#define _NEW_ALL ~0



/* Bits to track array state changes (also used to summarize array enabled)
 */
#define _NEW_ARRAY_VERTEX           0x1
#define _NEW_ARRAY_COLOR            0x2
#define _NEW_ARRAY_NORMAL           0x4
#define _NEW_ARRAY_INDEX            0x8
#define _NEW_ARRAY_EDGEFLAG         0x10
#define _NEW_ARRAY_SECONDARYCOLOR   0x20
#define _NEW_ARRAY_FOGCOORD         0x40
#define _NEW_ARRAY_TEXCOORD_0       0x80
#define _NEW_ARRAY_TEXCOORD_1       0x100
#define _NEW_ARRAY_TEXCOORD_2       0x200
#define _NEW_ARRAY_TEXCOORD_3       0x400
#define _NEW_ARRAY_TEXCOORD_4       0x800
#define _NEW_ARRAY_TEXCOORD_5       0x1000
#define _NEW_ARRAY_TEXCOORD_6       0x2000
#define _NEW_ARRAY_TEXCOORD_7       0x4000
#define _NEW_ARRAY_ALL              0x7fff

#define _NEW_ARRAY_TEXCOORD(i) (_NEW_ARRAY_TEXCOORD_0 << (i))

/* A bunch of flags that we think might be useful to drivers.
 */
#define DD_FLATSHADE                0x1
#define DD_SEPARATE_SPECULAR        0x2
#define DD_TRI_CULL_FRONT_BACK      0x4 /* special case on some hw */
#define DD_TRI_LIGHT_TWOSIDE        0x8
#define DD_TRI_UNFILLED             0x10
#define DD_TRI_SMOOTH               0x20
#define DD_TRI_STIPPLE              0x40
#define DD_TRI_OFFSET               0x80
#define DD_LINE_SMOOTH              0x100
#define DD_LINE_STIPPLE             0x200
#define DD_LINE_WIDTH               0x400
#define DD_POINT_SMOOTH             0x800
#define DD_POINT_SIZE               0x1000
#define DD_POINT_ATTEN              0x2000

/* Define the state changes under which each of these bits might change
 */
#define _DD_NEW_FLATSHADE                _NEW_LIGHT
#define _DD_NEW_SEPARATE_SPECULAR        (_NEW_LIGHT | _NEW_FOG)
#define _DD_NEW_TRI_CULL_FRONT_BACK      _NEW_POLYGON
#define _DD_NEW_TRI_LIGHT_TWOSIDE        _NEW_LIGHT
#define _DD_NEW_TRI_UNFILLED             _NEW_POLYGON
#define _DD_NEW_TRI_SMOOTH               _NEW_POLYGON
#define _DD_NEW_TRI_STIPPLE              _NEW_POLYGON
#define _DD_NEW_TRI_OFFSET               _NEW_POLYGON
#define _DD_NEW_LINE_SMOOTH              _NEW_LINE
#define _DD_NEW_LINE_STIPPLE             _NEW_LINE
#define _DD_NEW_LINE_WIDTH               _NEW_LINE
#define _DD_NEW_POINT_SMOOTH             _NEW_POINT
#define _DD_NEW_POINT_SIZE               _NEW_POINT
#define _DD_NEW_POINT_ATTEN              _NEW_POINT

#define _MESA_NEW_NEED_EYE_COORDS         (_NEW_LIGHT |		\
                                           _NEW_TEXTURE |	\
                                           _NEW_POINT |		\
                                           _NEW_MODELVIEW)

#define _MESA_NEW_NEED_NORMALS            (_NEW_LIGHT |		\
                                           _NEW_TEXTURE)

#define _IMAGE_NEW_TRANSFER_STATE         (_NEW_PIXEL | _NEW_COLOR_MATRIX)


#define NEED_NORMALS_TEXGEN      0x1
#define NEED_NORMALS_LIGHT       0x2

#define NEED_EYE_TEXGEN          0x1
#define NEED_EYE_LIGHT           0x2
#define NEED_EYE_LIGHT_MODELVIEW 0x4
#define NEED_EYE_POINT_ATTEN     0x8


/*
 * Forward declaration of display list datatypes:
 */
union node;
typedef union node Node;


/* This has to be included here. */
#include "dd.h"


/*
 * Core Mesa's support for tnl modules:
 */
#define NUM_VERTEX_FORMAT_ENTRIES (sizeof(GLvertexformat) / sizeof(void *))

struct gl_tnl_module {
   /* Vertex format to be lazily swapped into current dispatch.
    */
   GLvertexformat *Current;

   /* Record of functions swapped out.  On restore, only need to swap
    * these functions back in.
    */
   void *Swapped[NUM_VERTEX_FORMAT_ENTRIES][2];
   GLuint SwapCount;
};


/*
 * The library context:
 */
struct __GLcontextRec {
   /*
   ** Os related interfaces; these *must* be the first members of this
   ** structure, because they are exposed to the outside world (i.e. GLX
   ** extension).
   */
   __GLimports imports;
   __GLexports exports;

   /* State possibly shared with other contexts in the address space */
   struct gl_shared_state *Shared;

   /* API function pointer tables */
   struct _glapi_table *Save;	/* Display list save funcs */
   struct _glapi_table *Exec;	/* Execute funcs */
   struct _glapi_table *CurrentDispatch;  /* == Save or Exec !! */

   GLboolean ExecPrefersFloat;	/* What preference for color conversion? */
   GLboolean SavePrefersFloat;

   GLvisual Visual;
   GLframebuffer *DrawBuffer;	/* buffer for writing */
   GLframebuffer *ReadBuffer;	/* buffer for reading */

   /* Driver function pointer table */
   struct dd_function_table Driver;

   void *DriverCtx;	/* Points to device driver context/state */
   void *DriverMgrCtx;	/* Points to device driver manager (optional)*/

   /* Core/Driver constants */
   struct gl_constants Const;

   /* Modelview matrix and stack */
   GLmatrix ModelView;           /* current matrix, not stored on stack */
   GLuint ModelViewStackDepth;
   GLmatrix ModelViewStack[MAX_MODELVIEW_STACK_DEPTH - 1];

   /* Projection matrix and stack */
   GLmatrix ProjectionMatrix;    /* current matrix, not stored on stack */
   GLuint ProjectionStackDepth;
   GLmatrix ProjectionStack[MAX_PROJECTION_STACK_DEPTH - 1];

   /* Combined modelview and projection matrix */
   GLmatrix _ModelProjectMatrix;

   /* Texture matrix and stack */
   GLmatrix TextureMatrix[MAX_TEXTURE_UNITS];
   GLuint TextureStackDepth[MAX_TEXTURE_UNITS];
   GLmatrix TextureStack[MAX_TEXTURE_UNITS][MAX_TEXTURE_STACK_DEPTH - 1];

   /* Color matrix and stack */
   GLmatrix ColorMatrix;
   GLuint ColorStackDepth;
   GLmatrix ColorStack[MAX_COLOR_STACK_DEPTH - 1];

   /* Display lists */
   GLuint CallDepth;		/* Current recursion calling depth */
   GLboolean ExecuteFlag;	/* Execute GL commands? */
   GLboolean CompileFlag;	/* Compile GL commands into display list? */
   Node *CurrentListPtr;	/* Head of list being compiled */
   GLuint CurrentListNum;	/* Number of the list being compiled */
   Node *CurrentBlock;		/* Pointer to current block of nodes */
   GLuint CurrentPos;		/* Index into current block of nodes */

   /* Extensions */
   struct gl_extensions Extensions;

   /* Renderer attribute stack */
   GLuint AttribStackDepth;
   struct gl_attrib_node *AttribStack[MAX_ATTRIB_STACK_DEPTH];

   /* Renderer attribute groups */
   struct gl_accum_attrib	Accum;
   struct gl_colorbuffer_attrib	Color;
   struct gl_current_attrib	Current;
   struct gl_depthbuffer_attrib	Depth;
   struct gl_eval_attrib	Eval;
   struct gl_fog_attrib		Fog;
   struct gl_hint_attrib	Hint;
   struct gl_light_attrib	Light;
   struct gl_line_attrib	Line;
   struct gl_list_attrib	List;
   struct gl_multisample_attrib Multisample;
   struct gl_pixel_attrib	Pixel;
   struct gl_point_attrib	Point;
   struct gl_polygon_attrib	Polygon;
   GLuint PolygonStipple[32];
   struct gl_scissor_attrib	Scissor;
   struct gl_stencil_attrib	Stencil;
   struct gl_texture_attrib	Texture;
   struct gl_transform_attrib	Transform;
   struct gl_viewport_attrib	Viewport;

   /* Other attribute groups */
   struct gl_histogram_attrib	Histogram;
   struct gl_minmax_attrib	MinMax;
   struct gl_convolution_attrib Convolution1D;
   struct gl_convolution_attrib Convolution2D;
   struct gl_convolution_attrib Separable2D;

   /* Client attribute stack */
   GLuint ClientAttribStackDepth;
   struct gl_attrib_node *ClientAttribStack[MAX_CLIENT_ATTRIB_STACK_DEPTH];

   /* Client attribute groups */
   struct gl_array_attrib	Array;	/* Vertex arrays */
   struct gl_pixelstore_attrib	Pack;	/* Pixel packing */
   struct gl_pixelstore_attrib	Unpack;	/* Pixel unpacking */

   struct gl_evaluators EvalMap;   /* All evaluators */
   struct gl_feedback   Feedback;  /* Feedback */
   struct gl_selection  Select;    /* Selection */

   struct gl_color_table ColorTable;       /* Pre-convolution */
   struct gl_color_table ProxyColorTable;  /* Pre-convolution */
   struct gl_color_table PostConvolutionColorTable;
   struct gl_color_table ProxyPostConvolutionColorTable;
   struct gl_color_table PostColorMatrixColorTable;
   struct gl_color_table ProxyPostColorMatrixColorTable;

   GLenum ErrorValue;        /* Last error code */
   GLenum RenderMode;        /* either GL_RENDER, GL_SELECT, GL_FEEDBACK */
   GLuint NewState;          /* bitwise-or of _NEW_* flags */

   /* Derived */
   GLuint _TriangleCaps;      /* bitwise-or of DD_* flags */
   GLuint _ImageTransferState;/* bitwise-or of IMAGE_*_BIT flags */
   GLfloat _EyeZDir[3];
   GLfloat _ModelViewInvScale;
   GLuint _NeedEyeCoords;
   GLuint _NeedNormals;    /* Are vertex normal vectors needed? */

   struct gl_shine_tab *_ShineTable[2];  /* Active shine tables */
   struct gl_shine_tab *_ShineTabList;   /* Mru list of inactive shine tables */

   struct gl_list_extensions listext; /* driver dlist extensions */


   GLboolean OcclusionResult;  /* GL_HP_occlusion_test */
   GLboolean OcclusionResultSaved;  /* GL_HP_occlusion_test */

   /* Z buffer stuff */
   GLuint DepthMax;		/* Max depth buffer value */
   GLfloat DepthMaxF;		/* Float max depth buffer value */
   GLfloat MRD;			/* minimum resolvable difference in Z values */

   /* Should 3Dfx Glide driver catch signals? */
   GLboolean CatchSignals;

   /* For debugging/development only */
   GLboolean NoRaster;
   GLboolean FirstTimeCurrent;

   /* Dither disable via MESA_NO_DITHER env var */
   GLboolean NoDither;

   GLboolean Rendering;

#if defined(MESA_TRACE)
   struct _glapi_table *TraceDispatch;
   trace_context_t     *TraceCtx;
#else
   void *TraceDispatch;
   void *TraceCtx;
#endif

   /* Core tnl module support */
   struct gl_tnl_module TnlModule;

   /* Hooks for module contexts.  These will eventually live
    * in the driver or elsewhere.
    */
   void *swrast_context;
   void *swsetup_context;
   void *swtnl_context;
   void *swtnl_im;
   void *acache_context;
   void *aelt_context;
};


/* The string names for GL_POINT, GL_LINE_LOOP, etc */
extern const char *_mesa_prim_name[GL_POLYGON+4];


#ifdef MESA_DEBUG
extern int MESA_VERBOSE;
extern int MESA_DEBUG_FLAGS;
#else
# define MESA_VERBOSE 0
# define MESA_DEBUG_FLAGS 0
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif


enum _verbose {
   VERBOSE_VARRAY		= 0x0001,
   VERBOSE_TEXTURE		= 0x0002,
   VERBOSE_IMMEDIATE		= 0x0004,
   VERBOSE_PIPELINE		= 0x0008,
   VERBOSE_DRIVER		= 0x0010,
   VERBOSE_STATE		= 0x0020,
   VERBOSE_API			= 0x0040,
   VERBOSE_DISPLAY_LIST		= 0x0200,
   VERBOSE_LIGHTING		= 0x0400
};


enum _debug {
   DEBUG_ALWAYS_FLUSH		= 0x1
};



#define Elements(x) sizeof(x)/sizeof(*(x))



/* Eventually let the driver specify what statechanges require a flush:
 */
#define FLUSH_VERTICES(ctx, newstate)					\
do {									\
   if (ctx->Driver.NeedFlush & FLUSH_STORED_VERTICES)			\
      ctx->Driver.FlushVertices(ctx, FLUSH_STORED_VERTICES);		\
   ctx->NewState |= newstate;						\
} while (0)

#define FLUSH_CURRENT(ctx, newstate)					\
do {									\
   if (ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT)			\
      ctx->Driver.FlushVertices(ctx, FLUSH_UPDATE_CURRENT);		\
   ctx->NewState |= newstate;						\
} while (0)

#define ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, retval)		\
do {									\
   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {	\
      _mesa_error( ctx, GL_INVALID_OPERATION, "begin/end" );		\
      return retval;							\
   }									\
} while (0)

#define ASSERT_OUTSIDE_BEGIN_END(ctx)					\
do {									\
   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END) {	\
      _mesa_error( ctx, GL_INVALID_OPERATION, "begin/end" );		\
      return;								\
   }									\
} while (0)

#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx)				\
do {									\
   ASSERT_OUTSIDE_BEGIN_END(ctx);					\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)

#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval)	\
do {									\
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, retval);			\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)




#endif /* TYPES_H */
