/* $Id: gl.h,v 1.13 1999/10/17 23:24:49 brianp Exp $ */

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




#ifndef GL_H
#define GL_H

#if defined(USE_MGL_NAMESPACE)
#include "gl_mangle.h"
#endif


#if defined(__BEOS__)
#include <stdlib.h>     /* to get some BeOS-isms */
#endif


#if !defined(OPENSTEP) && (defined(NeXT) || defined(NeXT_PDO))
#define OPENSTEP
#endif


#if defined(_WIN32) && !defined(__WIN32__)
#	define __WIN32__
#endif

#if !defined(OPENSTEP) && (defined(__WIN32__) || defined(__CYGWIN32__))
#	pragma warning( disable : 4244 ) /* '=' : conversion from 'const double ' to 'float ', possible loss of data */
#	pragma warning( disable : 4018 ) /* '<' : signed/unsigned mismatch */
#	pragma warning( disable : 4305 ) /* '=' : truncation from 'const double ' to 'float ' */
#	pragma warning( disable : 4550 ) /* 'function' undefined; assuming extern returning int */
#	pragma warning( disable : 4761 ) /* integral size mismatch in argument; conversion supplied */
#	if defined(_MSC_VER) && defined(BUILD_GL32) /* tag specify we're building mesa as a DLL */
#		define GLAPI __declspec(dllexport)
#     define WGLAPI __declspec(dllexport)
#	elif defined(_MSC_VER) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#		define GLAPI __declspec(dllimport)
#     define WGLAPI __declspec(dllimport)
#	else /* for use with static link lib build of Win32 edition only */
#		define GLAPI extern
#     define WGLAPI __declspec(dllimport)
#	endif /* _STATIC_MESA support */
#	define GLAPIENTRY __stdcall
#	define GLCALLBACK __stdcall
#	define GLWINAPI __stdcall
#	define GLWINAPIV __cdecl
#else
/* non-Windows compilation */
#	define GLAPI extern
#	define GLAPIENTRY
#	define GLCALLBACK
#	define GLWINAPI
#	define GLWINAPIV
#endif /* WIN32 / CYGWIN32 bracket */

/* compatability guard so we don't need to change client code */

#if defined(_WIN32) && !defined(_WINDEF_) && !defined(OPENSTEP)
#	if !defined(MESA_MINWARN)
#		pragma message( "note: WINDOWS.H not included, providing Mesa definition of CALLBACK macro" )
#		pragma message( "----: and PROC typedef. If you receive compiler warnings about either ")
#		pragma message( "----: being multiply defined you should include WINDOWS.H priot to gl/gl.h" )
#	endif
#	define CALLBACK GLCALLBACK
typedef int (GLAPIENTRY *PROC)();
typedef void *HGLRC;
typedef void *HDC;
typedef unsigned long COLORREF;
#endif

#if defined(_WIN32) && !defined(_WINGDI_) && !defined(OPENSTEP)
#	if !defined(MESA_MINWARN)
#		pragma message( "note: WINDOWS.H not included, providing Mesa definition of wgl functions" )
#		pragma message( "----: and macros. If you receive compiler warnings about any being multiply ")
#		pragma message( "----: defined you should include WINDOWS.H priot to gl/gl.h" )
#	endif
#	define WGL_FONT_LINES      0
#	define WGL_FONT_POLYGONS   1
#	ifdef UNICODE
#		define wglUseFontBitmaps  wglUseFontBitmapsW
#		define wglUseFontOutlines  wglUseFontOutlinesW
#	else
#		define wglUseFontBitmaps  wglUseFontBitmapsA
#		define wglUseFontOutlines  wglUseFontOutlinesA
#	endif /* !UNICODE */
typedef struct tagLAYERPLANEDESCRIPTOR LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, *LPLAYERPLANEDESCRIPTOR;
typedef struct _GLYPHMETRICSFLOAT GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, *LPGLYPHMETRICSFLOAT;
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;
#include <gl/mesa_wgl.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


#ifdef macintosh
	#pragma enumsalwaysint on
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import on
	#endif
#endif



/*
 * Apps can test for this symbol to do conditional compilation if needed.
 */
#define MESA

#define MESA_MAJOR_VERSION 3
#define MESA_MINOR_VERSION 1


#define GL_VERSION_1_1   1
#define GL_VERSION_1_2   1


/*
 *
 * Enumerations
 *
 */

typedef enum {
	/* Boolean values */
	GL_FALSE			= 0,
	GL_TRUE				= 1,

	/* Data types */
	GL_BYTE				= 0x1400,
	GL_UNSIGNED_BYTE		= 0x1401,
	GL_SHORT			= 0x1402,
	GL_UNSIGNED_SHORT		= 0x1403,
	GL_INT				= 0x1404,
	GL_UNSIGNED_INT			= 0x1405,
	GL_FLOAT			= 0x1406,
	GL_DOUBLE			= 0x140A,
	GL_2_BYTES			= 0x1407,
	GL_3_BYTES			= 0x1408,
	GL_4_BYTES			= 0x1409,

	/* Primitives */
	GL_POINTS			= 0x0000,
	GL_LINES			= 0x0001,
	GL_LINE_LOOP			= 0x0002,
	GL_LINE_STRIP			= 0x0003,
	GL_TRIANGLES			= 0x0004,
	GL_TRIANGLE_STRIP		= 0x0005,
	GL_TRIANGLE_FAN			= 0x0006,
	GL_QUADS			= 0x0007,
	GL_QUAD_STRIP			= 0x0008,
	GL_POLYGON			= 0x0009,

	/* Vertex Arrays */
	GL_VERTEX_ARRAY			= 0x8074,
	GL_NORMAL_ARRAY			= 0x8075,
	GL_COLOR_ARRAY			= 0x8076,
	GL_INDEX_ARRAY			= 0x8077,
	GL_TEXTURE_COORD_ARRAY		= 0x8078,
	GL_EDGE_FLAG_ARRAY		= 0x8079,
	GL_VERTEX_ARRAY_SIZE		= 0x807A,
	GL_VERTEX_ARRAY_TYPE		= 0x807B,
	GL_VERTEX_ARRAY_STRIDE		= 0x807C,
	GL_NORMAL_ARRAY_TYPE		= 0x807E,
	GL_NORMAL_ARRAY_STRIDE		= 0x807F,
	GL_COLOR_ARRAY_SIZE		= 0x8081,
	GL_COLOR_ARRAY_TYPE		= 0x8082,
	GL_COLOR_ARRAY_STRIDE		= 0x8083,
	GL_INDEX_ARRAY_TYPE		= 0x8085,
	GL_INDEX_ARRAY_STRIDE		= 0x8086,
	GL_TEXTURE_COORD_ARRAY_SIZE	= 0x8088,
	GL_TEXTURE_COORD_ARRAY_TYPE	= 0x8089,
	GL_TEXTURE_COORD_ARRAY_STRIDE	= 0x808A,
	GL_EDGE_FLAG_ARRAY_STRIDE	= 0x808C,
	GL_VERTEX_ARRAY_POINTER		= 0x808E,
	GL_NORMAL_ARRAY_POINTER		= 0x808F,
	GL_COLOR_ARRAY_POINTER		= 0x8090,
	GL_INDEX_ARRAY_POINTER		= 0x8091,
	GL_TEXTURE_COORD_ARRAY_POINTER	= 0x8092,
	GL_EDGE_FLAG_ARRAY_POINTER	= 0x8093,
	GL_V2F				= 0x2A20,
	GL_V3F				= 0x2A21,
	GL_C4UB_V2F			= 0x2A22,
	GL_C4UB_V3F			= 0x2A23,
	GL_C3F_V3F			= 0x2A24,
	GL_N3F_V3F			= 0x2A25,
	GL_C4F_N3F_V3F			= 0x2A26,
	GL_T2F_V3F			= 0x2A27,
	GL_T4F_V4F			= 0x2A28,
	GL_T2F_C4UB_V3F			= 0x2A29,
	GL_T2F_C3F_V3F			= 0x2A2A,
	GL_T2F_N3F_V3F			= 0x2A2B,
	GL_T2F_C4F_N3F_V3F		= 0x2A2C,
	GL_T4F_C4F_N3F_V4F		= 0x2A2D,

	/* Matrix Mode */
	GL_MATRIX_MODE			= 0x0BA0,
	GL_MODELVIEW			= 0x1700,
	GL_PROJECTION			= 0x1701,
	GL_TEXTURE			= 0x1702,

	/* Points */
	GL_POINT_SMOOTH			= 0x0B10,
	GL_POINT_SIZE			= 0x0B11,
	GL_POINT_SIZE_GRANULARITY 	= 0x0B13,
	GL_POINT_SIZE_RANGE		= 0x0B12,

	/* Lines */
	GL_LINE_SMOOTH			= 0x0B20,
	GL_LINE_STIPPLE			= 0x0B24,
	GL_LINE_STIPPLE_PATTERN		= 0x0B25,
	GL_LINE_STIPPLE_REPEAT		= 0x0B26,
	GL_LINE_WIDTH			= 0x0B21,
	GL_LINE_WIDTH_GRANULARITY	= 0x0B23,
	GL_LINE_WIDTH_RANGE		= 0x0B22,

	/* Polygons */
	GL_POINT			= 0x1B00,
	GL_LINE				= 0x1B01,
	GL_FILL				= 0x1B02,
	GL_CW				= 0x0900,
	GL_CCW				= 0x0901,
	GL_FRONT			= 0x0404,
	GL_BACK				= 0x0405,
	GL_POLYGON_MODE			= 0x0B40,
	GL_POLYGON_SMOOTH		= 0x0B41,
	GL_POLYGON_STIPPLE		= 0x0B42,
	GL_EDGE_FLAG			= 0x0B43,
	GL_CULL_FACE			= 0x0B44,
	GL_CULL_FACE_MODE		= 0x0B45,
	GL_FRONT_FACE			= 0x0B46,
	GL_POLYGON_OFFSET_FACTOR	= 0x8038,
	GL_POLYGON_OFFSET_UNITS		= 0x2A00,
	GL_POLYGON_OFFSET_POINT		= 0x2A01,
	GL_POLYGON_OFFSET_LINE		= 0x2A02,
	GL_POLYGON_OFFSET_FILL		= 0x8037,

	/* Display Lists */
	GL_COMPILE			= 0x1300,
	GL_COMPILE_AND_EXECUTE		= 0x1301,
	GL_LIST_BASE			= 0x0B32,
	GL_LIST_INDEX			= 0x0B33,
	GL_LIST_MODE			= 0x0B30,

	/* Depth buffer */
	GL_NEVER			= 0x0200,
	GL_LESS				= 0x0201,
	GL_GEQUAL			= 0x0206,
	GL_LEQUAL			= 0x0203,
	GL_GREATER			= 0x0204,
	GL_NOTEQUAL			= 0x0205,
	GL_EQUAL			= 0x0202,
	GL_ALWAYS			= 0x0207,
	GL_DEPTH_TEST			= 0x0B71,
	GL_DEPTH_BITS			= 0x0D56,
	GL_DEPTH_CLEAR_VALUE		= 0x0B73,
	GL_DEPTH_FUNC			= 0x0B74,
	GL_DEPTH_RANGE			= 0x0B70,
	GL_DEPTH_WRITEMASK		= 0x0B72,
	GL_DEPTH_COMPONENT		= 0x1902,

	/* Lighting */
	GL_LIGHTING			= 0x0B50,
	GL_LIGHT0			= 0x4000,
	GL_LIGHT1			= 0x4001,
	GL_LIGHT2			= 0x4002,
	GL_LIGHT3			= 0x4003,
	GL_LIGHT4			= 0x4004,
	GL_LIGHT5			= 0x4005,
	GL_LIGHT6			= 0x4006,
	GL_LIGHT7			= 0x4007,
	GL_SPOT_EXPONENT		= 0x1205,
	GL_SPOT_CUTOFF			= 0x1206,
	GL_CONSTANT_ATTENUATION		= 0x1207,
	GL_LINEAR_ATTENUATION		= 0x1208,
	GL_QUADRATIC_ATTENUATION	= 0x1209,
	GL_AMBIENT			= 0x1200,
	GL_DIFFUSE			= 0x1201,
	GL_SPECULAR			= 0x1202,
	GL_SHININESS			= 0x1601,
	GL_EMISSION			= 0x1600,
	GL_POSITION			= 0x1203,
	GL_SPOT_DIRECTION		= 0x1204,
	GL_AMBIENT_AND_DIFFUSE		= 0x1602,
	GL_COLOR_INDEXES		= 0x1603,
	GL_LIGHT_MODEL_TWO_SIDE		= 0x0B52,
	GL_LIGHT_MODEL_LOCAL_VIEWER	= 0x0B51,
	GL_LIGHT_MODEL_AMBIENT		= 0x0B53,
	GL_FRONT_AND_BACK		= 0x0408,
	GL_SHADE_MODEL			= 0x0B54,
	GL_FLAT				= 0x1D00,
	GL_SMOOTH			= 0x1D01,
	GL_COLOR_MATERIAL		= 0x0B57,
	GL_COLOR_MATERIAL_FACE		= 0x0B55,
	GL_COLOR_MATERIAL_PARAMETER	= 0x0B56,
	GL_NORMALIZE			= 0x0BA1,

	/* User clipping planes */
	GL_CLIP_PLANE0			= 0x3000,
	GL_CLIP_PLANE1			= 0x3001,
	GL_CLIP_PLANE2			= 0x3002,
	GL_CLIP_PLANE3			= 0x3003,
	GL_CLIP_PLANE4			= 0x3004,
	GL_CLIP_PLANE5			= 0x3005,

	/* Accumulation buffer */
	GL_ACCUM_RED_BITS		= 0x0D58,
	GL_ACCUM_GREEN_BITS		= 0x0D59,
	GL_ACCUM_BLUE_BITS		= 0x0D5A,
	GL_ACCUM_ALPHA_BITS		= 0x0D5B,
	GL_ACCUM_CLEAR_VALUE		= 0x0B80,
	GL_ACCUM			= 0x0100,
	GL_ADD				= 0x0104,
	GL_LOAD				= 0x0101,
	GL_MULT				= 0x0103,
	GL_RETURN			= 0x0102,

	/* Alpha testing */
	GL_ALPHA_TEST			= 0x0BC0,
	GL_ALPHA_TEST_REF		= 0x0BC2,
	GL_ALPHA_TEST_FUNC		= 0x0BC1,

	/* Blending */
	GL_BLEND			= 0x0BE2,
	GL_BLEND_SRC			= 0x0BE1,
	GL_BLEND_DST			= 0x0BE0,
	GL_ZERO				= 0,
	GL_ONE				= 1,
	GL_SRC_COLOR			= 0x0300,
	GL_ONE_MINUS_SRC_COLOR		= 0x0301,
	GL_DST_COLOR			= 0x0306,
	GL_ONE_MINUS_DST_COLOR		= 0x0307,
	GL_SRC_ALPHA			= 0x0302,
	GL_ONE_MINUS_SRC_ALPHA		= 0x0303,
	GL_DST_ALPHA			= 0x0304,
	GL_ONE_MINUS_DST_ALPHA		= 0x0305,
	GL_SRC_ALPHA_SATURATE		= 0x0308,
	GL_CONSTANT_COLOR		= 0x8001,
	GL_ONE_MINUS_CONSTANT_COLOR	= 0x8002,
	GL_CONSTANT_ALPHA		= 0x8003,
	GL_ONE_MINUS_CONSTANT_ALPHA	= 0x8004,

	/* Render Mode */
	GL_FEEDBACK			= 0x1C01,
	GL_RENDER			= 0x1C00,
	GL_SELECT			= 0x1C02,

	/* Feedback */
	GL_2D				= 0x0600,
	GL_3D				= 0x0601,
	GL_3D_COLOR			= 0x0602,
	GL_3D_COLOR_TEXTURE		= 0x0603,
	GL_4D_COLOR_TEXTURE		= 0x0604,
	GL_POINT_TOKEN			= 0x0701,
	GL_LINE_TOKEN			= 0x0702,
	GL_LINE_RESET_TOKEN		= 0x0707,
	GL_POLYGON_TOKEN		= 0x0703,
	GL_BITMAP_TOKEN			= 0x0704,
	GL_DRAW_PIXEL_TOKEN		= 0x0705,
	GL_COPY_PIXEL_TOKEN		= 0x0706,
	GL_PASS_THROUGH_TOKEN		= 0x0700,
	GL_FEEDBACK_BUFFER_POINTER	= 0x0DF0,
	GL_FEEDBACK_BUFFER_SIZE		= 0x0DF1,
	GL_FEEDBACK_BUFFER_TYPE		= 0x0DF2,

	/* Selection */
	GL_SELECTION_BUFFER_POINTER	= 0x0DF3,
	GL_SELECTION_BUFFER_SIZE	= 0x0DF4,

	/* Fog */
	GL_FOG				= 0x0B60,
	GL_FOG_MODE			= 0x0B65,
	GL_FOG_DENSITY			= 0x0B62,
	GL_FOG_COLOR			= 0x0B66,
	GL_FOG_INDEX			= 0x0B61,
	GL_FOG_START			= 0x0B63,
	GL_FOG_END			= 0x0B64,
	GL_LINEAR			= 0x2601,
	GL_EXP				= 0x0800,
	GL_EXP2				= 0x0801,

	/* Logic Ops */
	GL_LOGIC_OP			= 0x0BF1,
	GL_INDEX_LOGIC_OP		= 0x0BF1,
	GL_COLOR_LOGIC_OP		= 0x0BF2,
	GL_LOGIC_OP_MODE		= 0x0BF0,
	GL_CLEAR			= 0x1500,
	GL_SET				= 0x150F,
	GL_COPY				= 0x1503,
	GL_COPY_INVERTED		= 0x150C,
	GL_NOOP				= 0x1505,
	GL_INVERT			= 0x150A,
	GL_AND				= 0x1501,
	GL_NAND				= 0x150E,
	GL_OR				= 0x1507,
	GL_NOR				= 0x1508,
	GL_XOR				= 0x1506,
	GL_EQUIV			= 0x1509,
	GL_AND_REVERSE			= 0x1502,
	GL_AND_INVERTED			= 0x1504,
	GL_OR_REVERSE			= 0x150B,
	GL_OR_INVERTED			= 0x150D,

	/* Stencil */
	GL_STENCIL_TEST			= 0x0B90,
	GL_STENCIL_WRITEMASK		= 0x0B98,
	GL_STENCIL_BITS			= 0x0D57,
	GL_STENCIL_FUNC			= 0x0B92,
	GL_STENCIL_VALUE_MASK		= 0x0B93,
	GL_STENCIL_REF			= 0x0B97,
	GL_STENCIL_FAIL			= 0x0B94,
	GL_STENCIL_PASS_DEPTH_PASS	= 0x0B96,
	GL_STENCIL_PASS_DEPTH_FAIL	= 0x0B95,
	GL_STENCIL_CLEAR_VALUE		= 0x0B91,
	GL_STENCIL_INDEX		= 0x1901,
	GL_KEEP				= 0x1E00,
	GL_REPLACE			= 0x1E01,
	GL_INCR				= 0x1E02,
	GL_DECR				= 0x1E03,

	/* Buffers, Pixel Drawing/Reading */
	GL_NONE				= 0,
	GL_LEFT				= 0x0406,
	GL_RIGHT			= 0x0407,
	/*GL_FRONT			= 0x0404, */
	/*GL_BACK			= 0x0405, */
	/*GL_FRONT_AND_BACK		= 0x0408, */
	GL_FRONT_LEFT			= 0x0400,
	GL_FRONT_RIGHT			= 0x0401,
	GL_BACK_LEFT			= 0x0402,
	GL_BACK_RIGHT			= 0x0403,
	GL_AUX0				= 0x0409,
	GL_AUX1				= 0x040A,
	GL_AUX2				= 0x040B,
	GL_AUX3				= 0x040C,
	GL_COLOR_INDEX			= 0x1900,
	GL_RED				= 0x1903,
	GL_GREEN			= 0x1904,
	GL_BLUE				= 0x1905,
	GL_ALPHA			= 0x1906,
	GL_LUMINANCE			= 0x1909,
	GL_LUMINANCE_ALPHA		= 0x190A,
	GL_ALPHA_BITS			= 0x0D55,
	GL_RED_BITS			= 0x0D52,
	GL_GREEN_BITS			= 0x0D53,
	GL_BLUE_BITS			= 0x0D54,
	GL_INDEX_BITS			= 0x0D51,
	GL_SUBPIXEL_BITS		= 0x0D50,
	GL_AUX_BUFFERS			= 0x0C00,
	GL_READ_BUFFER			= 0x0C02,
	GL_DRAW_BUFFER			= 0x0C01,
	GL_DOUBLEBUFFER			= 0x0C32,
	GL_STEREO			= 0x0C33,
	GL_BITMAP			= 0x1A00,
	GL_COLOR			= 0x1800,
	GL_DEPTH			= 0x1801,
	GL_STENCIL			= 0x1802,
	GL_DITHER			= 0x0BD0,
	GL_RGB				= 0x1907,
	GL_RGBA				= 0x1908,

	/* Implementation limits */
	GL_MAX_LIST_NESTING		= 0x0B31,
	GL_MAX_ATTRIB_STACK_DEPTH	= 0x0D35,
	GL_MAX_MODELVIEW_STACK_DEPTH	= 0x0D36,
	GL_MAX_NAME_STACK_DEPTH		= 0x0D37,
	GL_MAX_PROJECTION_STACK_DEPTH	= 0x0D38,
	GL_MAX_TEXTURE_STACK_DEPTH	= 0x0D39,
	GL_MAX_EVAL_ORDER		= 0x0D30,
	GL_MAX_LIGHTS			= 0x0D31,
	GL_MAX_CLIP_PLANES		= 0x0D32,
	GL_MAX_TEXTURE_SIZE		= 0x0D33,
	GL_MAX_PIXEL_MAP_TABLE		= 0x0D34,
	GL_MAX_VIEWPORT_DIMS		= 0x0D3A,
	GL_MAX_CLIENT_ATTRIB_STACK_DEPTH= 0x0D3B,

	/* Gets */
	GL_ATTRIB_STACK_DEPTH		= 0x0BB0,
	GL_CLIENT_ATTRIB_STACK_DEPTH	= 0x0BB1,
	GL_COLOR_CLEAR_VALUE		= 0x0C22,
	GL_COLOR_WRITEMASK		= 0x0C23,
	GL_CURRENT_INDEX		= 0x0B01,
	GL_CURRENT_COLOR		= 0x0B00,
	GL_CURRENT_NORMAL		= 0x0B02,
	GL_CURRENT_RASTER_COLOR		= 0x0B04,
	GL_CURRENT_RASTER_DISTANCE	= 0x0B09,
	GL_CURRENT_RASTER_INDEX		= 0x0B05,
	GL_CURRENT_RASTER_POSITION	= 0x0B07,
	GL_CURRENT_RASTER_TEXTURE_COORDS = 0x0B06,
	GL_CURRENT_RASTER_POSITION_VALID = 0x0B08,
	GL_CURRENT_TEXTURE_COORDS	= 0x0B03,
	GL_INDEX_CLEAR_VALUE		= 0x0C20,
	GL_INDEX_MODE			= 0x0C30,
	GL_INDEX_WRITEMASK		= 0x0C21,
	GL_MODELVIEW_MATRIX		= 0x0BA6,
	GL_MODELVIEW_STACK_DEPTH	= 0x0BA3,
	GL_NAME_STACK_DEPTH		= 0x0D70,
	GL_PROJECTION_MATRIX		= 0x0BA7,
	GL_PROJECTION_STACK_DEPTH	= 0x0BA4,
	GL_RENDER_MODE			= 0x0C40,
	GL_RGBA_MODE			= 0x0C31,
	GL_TEXTURE_MATRIX		= 0x0BA8,
	GL_TEXTURE_STACK_DEPTH		= 0x0BA5,
	GL_VIEWPORT			= 0x0BA2,

	/* Evaluators */
	GL_AUTO_NORMAL			= 0x0D80,
	GL_MAP1_COLOR_4			= 0x0D90,
	GL_MAP1_GRID_DOMAIN		= 0x0DD0,
	GL_MAP1_GRID_SEGMENTS		= 0x0DD1,
	GL_MAP1_INDEX			= 0x0D91,
	GL_MAP1_NORMAL			= 0x0D92,
	GL_MAP1_TEXTURE_COORD_1		= 0x0D93,
	GL_MAP1_TEXTURE_COORD_2		= 0x0D94,
	GL_MAP1_TEXTURE_COORD_3		= 0x0D95,
	GL_MAP1_TEXTURE_COORD_4		= 0x0D96,
	GL_MAP1_VERTEX_3		= 0x0D97,
	GL_MAP1_VERTEX_4		= 0x0D98,
	GL_MAP2_COLOR_4			= 0x0DB0,
	GL_MAP2_GRID_DOMAIN		= 0x0DD2,
	GL_MAP2_GRID_SEGMENTS		= 0x0DD3,
	GL_MAP2_INDEX			= 0x0DB1,
	GL_MAP2_NORMAL			= 0x0DB2,
	GL_MAP2_TEXTURE_COORD_1		= 0x0DB3,
	GL_MAP2_TEXTURE_COORD_2		= 0x0DB4,
	GL_MAP2_TEXTURE_COORD_3		= 0x0DB5,
	GL_MAP2_TEXTURE_COORD_4		= 0x0DB6,
	GL_MAP2_VERTEX_3		= 0x0DB7,
	GL_MAP2_VERTEX_4		= 0x0DB8,
	GL_COEFF			= 0x0A00,
	GL_DOMAIN			= 0x0A02,
	GL_ORDER			= 0x0A01,

	/* Hints */
	GL_FOG_HINT			= 0x0C54,
	GL_LINE_SMOOTH_HINT		= 0x0C52,
	GL_PERSPECTIVE_CORRECTION_HINT	= 0x0C50,
	GL_POINT_SMOOTH_HINT		= 0x0C51,
	GL_POLYGON_SMOOTH_HINT		= 0x0C53,
	GL_DONT_CARE			= 0x1100,
	GL_FASTEST			= 0x1101,
	GL_NICEST			= 0x1102,

	/* Scissor box */
	GL_SCISSOR_TEST			= 0x0C11,
	GL_SCISSOR_BOX			= 0x0C10,

	/* Pixel Mode / Transfer */
	GL_MAP_COLOR			= 0x0D10,
	GL_MAP_STENCIL			= 0x0D11,
	GL_INDEX_SHIFT			= 0x0D12,
	GL_INDEX_OFFSET			= 0x0D13,
	GL_RED_SCALE			= 0x0D14,
	GL_RED_BIAS			= 0x0D15,
	GL_GREEN_SCALE			= 0x0D18,
	GL_GREEN_BIAS			= 0x0D19,
	GL_BLUE_SCALE			= 0x0D1A,
	GL_BLUE_BIAS			= 0x0D1B,
	GL_ALPHA_SCALE			= 0x0D1C,
	GL_ALPHA_BIAS			= 0x0D1D,
	GL_DEPTH_SCALE			= 0x0D1E,
	GL_DEPTH_BIAS			= 0x0D1F,
	GL_PIXEL_MAP_S_TO_S_SIZE	= 0x0CB1,
	GL_PIXEL_MAP_I_TO_I_SIZE	= 0x0CB0,
	GL_PIXEL_MAP_I_TO_R_SIZE	= 0x0CB2,
	GL_PIXEL_MAP_I_TO_G_SIZE	= 0x0CB3,
	GL_PIXEL_MAP_I_TO_B_SIZE	= 0x0CB4,
	GL_PIXEL_MAP_I_TO_A_SIZE	= 0x0CB5,
	GL_PIXEL_MAP_R_TO_R_SIZE	= 0x0CB6,
	GL_PIXEL_MAP_G_TO_G_SIZE	= 0x0CB7,
	GL_PIXEL_MAP_B_TO_B_SIZE	= 0x0CB8,
	GL_PIXEL_MAP_A_TO_A_SIZE	= 0x0CB9,
	GL_PIXEL_MAP_S_TO_S		= 0x0C71,
	GL_PIXEL_MAP_I_TO_I		= 0x0C70,
	GL_PIXEL_MAP_I_TO_R		= 0x0C72,
	GL_PIXEL_MAP_I_TO_G		= 0x0C73,
	GL_PIXEL_MAP_I_TO_B		= 0x0C74,
	GL_PIXEL_MAP_I_TO_A		= 0x0C75,
	GL_PIXEL_MAP_R_TO_R		= 0x0C76,
	GL_PIXEL_MAP_G_TO_G		= 0x0C77,
	GL_PIXEL_MAP_B_TO_B		= 0x0C78,
	GL_PIXEL_MAP_A_TO_A		= 0x0C79,
	GL_PACK_ALIGNMENT		= 0x0D05,
	GL_PACK_LSB_FIRST		= 0x0D01,
	GL_PACK_ROW_LENGTH		= 0x0D02,
	GL_PACK_SKIP_PIXELS		= 0x0D04,
	GL_PACK_SKIP_ROWS		= 0x0D03,
	GL_PACK_SWAP_BYTES		= 0x0D00,
	GL_UNPACK_ALIGNMENT		= 0x0CF5,
	GL_UNPACK_LSB_FIRST		= 0x0CF1,
	GL_UNPACK_ROW_LENGTH		= 0x0CF2,
	GL_UNPACK_SKIP_PIXELS		= 0x0CF4,
	GL_UNPACK_SKIP_ROWS		= 0x0CF3,
	GL_UNPACK_SWAP_BYTES		= 0x0CF0,
	GL_ZOOM_X			= 0x0D16,
	GL_ZOOM_Y			= 0x0D17,

	/* Texture mapping */
	GL_TEXTURE_ENV			= 0x2300,
	GL_TEXTURE_ENV_MODE		= 0x2200,
	GL_TEXTURE_1D			= 0x0DE0,
	GL_TEXTURE_2D			= 0x0DE1,
	GL_TEXTURE_WRAP_S		= 0x2802,
	GL_TEXTURE_WRAP_T		= 0x2803,
	GL_TEXTURE_MAG_FILTER		= 0x2800,
	GL_TEXTURE_MIN_FILTER		= 0x2801,
	GL_TEXTURE_ENV_COLOR		= 0x2201,
	GL_TEXTURE_GEN_S		= 0x0C60,
	GL_TEXTURE_GEN_T		= 0x0C61,
	GL_TEXTURE_GEN_MODE		= 0x2500,
	GL_TEXTURE_BORDER_COLOR		= 0x1004,
	GL_TEXTURE_WIDTH		= 0x1000,
	GL_TEXTURE_HEIGHT		= 0x1001,
	GL_TEXTURE_BORDER		= 0x1005,
	GL_TEXTURE_COMPONENTS		= 0x1003,
	GL_TEXTURE_RED_SIZE		= 0x805C,
	GL_TEXTURE_GREEN_SIZE		= 0x805D,
	GL_TEXTURE_BLUE_SIZE		= 0x805E,
	GL_TEXTURE_ALPHA_SIZE		= 0x805F,
	GL_TEXTURE_LUMINANCE_SIZE	= 0x8060,
	GL_TEXTURE_INTENSITY_SIZE	= 0x8061,
	GL_NEAREST_MIPMAP_NEAREST	= 0x2700,
	GL_NEAREST_MIPMAP_LINEAR	= 0x2702,
	GL_LINEAR_MIPMAP_NEAREST	= 0x2701,
	GL_LINEAR_MIPMAP_LINEAR		= 0x2703,
	GL_OBJECT_LINEAR		= 0x2401,
	GL_OBJECT_PLANE			= 0x2501,
	GL_EYE_LINEAR			= 0x2400,
	GL_EYE_PLANE			= 0x2502,
	GL_SPHERE_MAP			= 0x2402,
	GL_DECAL			= 0x2101,
	GL_MODULATE			= 0x2100,
	GL_NEAREST			= 0x2600,
	GL_REPEAT			= 0x2901,
	GL_CLAMP			= 0x2900,
	GL_S				= 0x2000,
	GL_T				= 0x2001,
	GL_R				= 0x2002,
	GL_Q				= 0x2003,
	GL_TEXTURE_GEN_R		= 0x0C62,
	GL_TEXTURE_GEN_Q		= 0x0C63,

	/* GL 1.1 texturing */
	GL_PROXY_TEXTURE_1D		= 0x8063,
	GL_PROXY_TEXTURE_2D		= 0x8064,
	GL_TEXTURE_PRIORITY		= 0x8066,
	GL_TEXTURE_RESIDENT		= 0x8067,
	GL_TEXTURE_BINDING_1D		= 0x8068,
	GL_TEXTURE_BINDING_2D		= 0x8069,
	GL_TEXTURE_INTERNAL_FORMAT	= 0x1003,

	/* GL 1.2 texturing */
	GL_PACK_SKIP_IMAGES		= 0x806B,
	GL_PACK_IMAGE_HEIGHT		= 0x806C,
	GL_UNPACK_SKIP_IMAGES		= 0x806D,
	GL_UNPACK_IMAGE_HEIGHT		= 0x806E,
	GL_TEXTURE_3D			= 0x806F,
	GL_PROXY_TEXTURE_3D		= 0x8070,
	GL_TEXTURE_DEPTH		= 0x8071,
	GL_TEXTURE_WRAP_R		= 0x8072,
	GL_MAX_3D_TEXTURE_SIZE		= 0x8073,
	GL_TEXTURE_BINDING_3D		= 0x806A,

	/* Internal texture formats (GL 1.1) */
	GL_ALPHA4			= 0x803B,
	GL_ALPHA8			= 0x803C,
	GL_ALPHA12			= 0x803D,
	GL_ALPHA16			= 0x803E,
	GL_LUMINANCE4			= 0x803F,
	GL_LUMINANCE8			= 0x8040,
	GL_LUMINANCE12			= 0x8041,
	GL_LUMINANCE16			= 0x8042,
	GL_LUMINANCE4_ALPHA4		= 0x8043,
	GL_LUMINANCE6_ALPHA2		= 0x8044,
	GL_LUMINANCE8_ALPHA8		= 0x8045,
	GL_LUMINANCE12_ALPHA4		= 0x8046,
	GL_LUMINANCE12_ALPHA12		= 0x8047,
	GL_LUMINANCE16_ALPHA16		= 0x8048,
	GL_INTENSITY			= 0x8049,
	GL_INTENSITY4			= 0x804A,
	GL_INTENSITY8			= 0x804B,
	GL_INTENSITY12			= 0x804C,
	GL_INTENSITY16			= 0x804D,
	GL_R3_G3_B2			= 0x2A10,
	GL_RGB4				= 0x804F,
	GL_RGB5				= 0x8050,
	GL_RGB8				= 0x8051,
	GL_RGB10			= 0x8052,
	GL_RGB12			= 0x8053,
	GL_RGB16			= 0x8054,
	GL_RGBA2			= 0x8055,
	GL_RGBA4			= 0x8056,
	GL_RGB5_A1			= 0x8057,
	GL_RGBA8			= 0x8058,
	GL_RGB10_A2			= 0x8059,
	GL_RGBA12			= 0x805A,
	GL_RGBA16			= 0x805B,

	/* Utility */
	GL_VENDOR			= 0x1F00,
	GL_RENDERER			= 0x1F01,
	GL_VERSION			= 0x1F02,
	GL_EXTENSIONS			= 0x1F03,

	/* Errors */
	GL_INVALID_VALUE		= 0x0501,
	GL_INVALID_ENUM			= 0x0500,
	GL_INVALID_OPERATION		= 0x0502,
	GL_STACK_OVERFLOW		= 0x0503,
	GL_STACK_UNDERFLOW		= 0x0504,
	GL_OUT_OF_MEMORY		= 0x0505,

	/*
	 * Extensions
	 */

	/* GL_EXT_blend_minmax and GL_EXT_blend_color */
	GL_CONSTANT_COLOR_EXT			= 0x8001,
	GL_ONE_MINUS_CONSTANT_COLOR_EXT		= 0x8002,
	GL_CONSTANT_ALPHA_EXT			= 0x8003,
	GL_ONE_MINUS_CONSTANT_ALPHA_EXT		= 0x8004,
	GL_BLEND_EQUATION_EXT			= 0x8009,
	GL_MIN_EXT				= 0x8007,
	GL_MAX_EXT				= 0x8008,
	GL_FUNC_ADD_EXT				= 0x8006,
	GL_FUNC_SUBTRACT_EXT			= 0x800A,
	GL_FUNC_REVERSE_SUBTRACT_EXT		= 0x800B,
	GL_BLEND_COLOR_EXT			= 0x8005,

	/* GL_EXT_polygon_offset */
	GL_POLYGON_OFFSET_EXT			= 0x8037,
	GL_POLYGON_OFFSET_FACTOR_EXT		= 0x8038,
	GL_POLYGON_OFFSET_BIAS_EXT		= 0x8039,

	/* GL_EXT_vertex_array */
	GL_VERTEX_ARRAY_EXT			= 0x8074,
	GL_NORMAL_ARRAY_EXT			= 0x8075,
	GL_COLOR_ARRAY_EXT			= 0x8076,
	GL_INDEX_ARRAY_EXT			= 0x8077,
	GL_TEXTURE_COORD_ARRAY_EXT		= 0x8078,
	GL_EDGE_FLAG_ARRAY_EXT			= 0x8079,
	GL_VERTEX_ARRAY_SIZE_EXT		= 0x807A,
	GL_VERTEX_ARRAY_TYPE_EXT		= 0x807B,
	GL_VERTEX_ARRAY_STRIDE_EXT		= 0x807C,
	GL_VERTEX_ARRAY_COUNT_EXT		= 0x807D,
	GL_NORMAL_ARRAY_TYPE_EXT		= 0x807E,
	GL_NORMAL_ARRAY_STRIDE_EXT		= 0x807F,
	GL_NORMAL_ARRAY_COUNT_EXT		= 0x8080,
	GL_COLOR_ARRAY_SIZE_EXT			= 0x8081,
	GL_COLOR_ARRAY_TYPE_EXT			= 0x8082,
	GL_COLOR_ARRAY_STRIDE_EXT		= 0x8083,
	GL_COLOR_ARRAY_COUNT_EXT		= 0x8084,
	GL_INDEX_ARRAY_TYPE_EXT			= 0x8085,
	GL_INDEX_ARRAY_STRIDE_EXT		= 0x8086,
	GL_INDEX_ARRAY_COUNT_EXT		= 0x8087,
	GL_TEXTURE_COORD_ARRAY_SIZE_EXT		= 0x8088,
	GL_TEXTURE_COORD_ARRAY_TYPE_EXT		= 0x8089,
	GL_TEXTURE_COORD_ARRAY_STRIDE_EXT	= 0x808A,
	GL_TEXTURE_COORD_ARRAY_COUNT_EXT	= 0x808B,
	GL_EDGE_FLAG_ARRAY_STRIDE_EXT		= 0x808C,
	GL_EDGE_FLAG_ARRAY_COUNT_EXT		= 0x808D,
	GL_VERTEX_ARRAY_POINTER_EXT		= 0x808E,
	GL_NORMAL_ARRAY_POINTER_EXT		= 0x808F,
	GL_COLOR_ARRAY_POINTER_EXT		= 0x8090,
	GL_INDEX_ARRAY_POINTER_EXT		= 0x8091,
	GL_TEXTURE_COORD_ARRAY_POINTER_EXT	= 0x8092,
	GL_EDGE_FLAG_ARRAY_POINTER_EXT		= 0x8093,

	/* GL_EXT_texture_object */
	GL_TEXTURE_PRIORITY_EXT			= 0x8066,
	GL_TEXTURE_RESIDENT_EXT			= 0x8067,
	GL_TEXTURE_1D_BINDING_EXT		= 0x8068,
	GL_TEXTURE_2D_BINDING_EXT		= 0x8069,

	/* GL_EXT_texture3D */
	GL_PACK_SKIP_IMAGES_EXT			= 0x806B,
	GL_PACK_IMAGE_HEIGHT_EXT		= 0x806C,
	GL_UNPACK_SKIP_IMAGES_EXT		= 0x806D,
	GL_UNPACK_IMAGE_HEIGHT_EXT		= 0x806E,
	GL_TEXTURE_3D_EXT			= 0x806F,
	GL_PROXY_TEXTURE_3D_EXT			= 0x8070,
	GL_TEXTURE_DEPTH_EXT			= 0x8071,
	GL_TEXTURE_WRAP_R_EXT			= 0x8072,
	GL_MAX_3D_TEXTURE_SIZE_EXT		= 0x8073,
	GL_TEXTURE_3D_BINDING_EXT		= 0x806A,

	/* GL_EXT_paletted_texture */
	GL_TABLE_TOO_LARGE_EXT			= 0x8031,
	GL_COLOR_TABLE_FORMAT_EXT		= 0x80D8,
	GL_COLOR_TABLE_WIDTH_EXT		= 0x80D9,
	GL_COLOR_TABLE_RED_SIZE_EXT		= 0x80DA,
	GL_COLOR_TABLE_GREEN_SIZE_EXT		= 0x80DB,
	GL_COLOR_TABLE_BLUE_SIZE_EXT		= 0x80DC,
	GL_COLOR_TABLE_ALPHA_SIZE_EXT	 	= 0x80DD,
	GL_COLOR_TABLE_LUMINANCE_SIZE_EXT	= 0x80DE,
	GL_COLOR_TABLE_INTENSITY_SIZE_EXT	= 0x80DF,
	GL_TEXTURE_INDEX_SIZE_EXT		= 0x80ED,
	GL_COLOR_INDEX1_EXT			= 0x80E2,
	GL_COLOR_INDEX2_EXT			= 0x80E3,
	GL_COLOR_INDEX4_EXT			= 0x80E4,
	GL_COLOR_INDEX8_EXT			= 0x80E5,
	GL_COLOR_INDEX12_EXT			= 0x80E6,
	GL_COLOR_INDEX16_EXT			= 0x80E7,

	/* GL_EXT_shared_texture_palette */
	GL_SHARED_TEXTURE_PALETTE_EXT		= 0x81FB,

	/* GL_EXT_point_parameters */
	GL_POINT_SIZE_MIN_EXT			= 0x8126,
	GL_POINT_SIZE_MAX_EXT			= 0x8127,
	GL_POINT_FADE_THRESHOLD_SIZE_EXT	= 0x8128,
	GL_DISTANCE_ATTENUATION_EXT		= 0x8129,

	/* GL_EXT_rescale_normal */
	GL_RESCALE_NORMAL_EXT			= 0x803A,

	/* GL_EXT_abgr */
	GL_ABGR_EXT				= 0x8000,

	/* GL_EXT_stencil_wrap */
	GL_INCR_WRAP_EXT			= 0x8507,
	GL_DECR_WRAP_EXT			= 0x8508,

	/* GL_SGIS_texture_edge_clamp */
	GL_CLAMP_TO_EDGE_SGIS			= 0x812F,

	/* GL_INGR_blend_func_separate */
	GL_BLEND_DST_RGB_INGR			= 0x80C8,
	GL_BLEND_SRC_RGB_INGR			= 0x80C9,
	GL_BLEND_DST_ALPHA_INGR			= 0x80CA,
	GL_BLEND_SRC_ALPHA_INGR			= 0x80CB,

	/* OpenGL 1.2 */
	GL_RESCALE_NORMAL			= 0x803A,
	GL_CLAMP_TO_EDGE			= 0x812F,
	GL_MAX_ELEMENTS_VERTICES		= 0x80E8,
	GL_MAX_ELEMENTS_INDICES			= 0x80E9,
	GL_BGR					= 0x80E0,
	GL_BGRA					= 0x80E1,
	GL_UNSIGNED_BYTE_3_3_2			= 0x8032,
	GL_UNSIGNED_BYTE_2_3_3_REV		= 0x8362,
	GL_UNSIGNED_SHORT_5_6_5			= 0x8363,
	GL_UNSIGNED_SHORT_5_6_5_REV		= 0x8364,
	GL_UNSIGNED_SHORT_4_4_4_4		= 0x8033,
	GL_UNSIGNED_SHORT_4_4_4_4_REV		= 0x8365,
	GL_UNSIGNED_SHORT_5_5_5_1		= 0x8034,
	GL_UNSIGNED_SHORT_1_5_5_5_REV		= 0x8366,
	GL_UNSIGNED_INT_8_8_8_8			= 0x8035,
	GL_UNSIGNED_INT_8_8_8_8_REV		= 0x8367,
	GL_UNSIGNED_INT_10_10_10_2		= 0x8036,
	GL_UNSIGNED_INT_2_10_10_10_REV		= 0x8368,
	GL_LIGHT_MODEL_COLOR_CONTROL		= 0x81F8,
	GL_SINGLE_COLOR				= 0x81F9,
	GL_SEPARATE_SPECULAR_COLOR		= 0x81FA,
	GL_TEXTURE_MIN_LOD			= 0x813A,
	GL_TEXTURE_MAX_LOD			= 0x813B,
	GL_TEXTURE_BASE_LEVEL			= 0x813C,
	GL_TEXTURE_MAX_LEVEL			= 0x813D,

	/* GL_ARB_multitexture */
	GL_TEXTURE0_ARB				= 0x84C0,
	GL_TEXTURE1_ARB				= 0x84C1,
	GL_TEXTURE2_ARB				= 0x84C2,
	GL_TEXTURE3_ARB				= 0x84C3,
	GL_TEXTURE4_ARB				= 0x84C4,
	GL_TEXTURE5_ARB				= 0x84C5,
	GL_TEXTURE6_ARB				= 0x84C6,
	GL_TEXTURE7_ARB				= 0x84C7,
	GL_TEXTURE8_ARB				= 0x84C8,
	GL_TEXTURE9_ARB				= 0x84C9,
	GL_TEXTURE10_ARB			= 0x84CA,
	GL_TEXTURE11_ARB			= 0x84CB,
	GL_TEXTURE12_ARB			= 0x84CC,
	GL_TEXTURE13_ARB			= 0x84CD,
	GL_TEXTURE14_ARB			= 0x84CE,
	GL_TEXTURE15_ARB			= 0x84CF,
	GL_TEXTURE16_ARB			= 0x84D0,
	GL_TEXTURE17_ARB			= 0x84D1,
	GL_TEXTURE18_ARB			= 0x84D2,
	GL_TEXTURE19_ARB			= 0x84D3,
	GL_TEXTURE20_ARB			= 0x84D4,
	GL_TEXTURE21_ARB			= 0x84D5,
	GL_TEXTURE22_ARB			= 0x84D6,
	GL_TEXTURE23_ARB			= 0x84D7,
	GL_TEXTURE24_ARB			= 0x84D8,
	GL_TEXTURE25_ARB			= 0x84D9,
	GL_TEXTURE26_ARB			= 0x84DA,
	GL_TEXTURE27_ARB			= 0x84DB,
	GL_TEXTURE28_ARB			= 0x84DC,
	GL_TEXTURE29_ARB			= 0x84DD,
	GL_TEXTURE30_ARB			= 0x84DE,
	GL_TEXTURE31_ARB			= 0x84DF,
	GL_ACTIVE_TEXTURE_ARB			= 0x84E0,
	GL_CLIENT_ACTIVE_TEXTURE_ARB		= 0x84E1,
	GL_MAX_TEXTURE_UNITS_ARB		= 0x84E2,

	/*
	 * OpenGL 1.2 imaging subset (NOT IMPLEMENTED BY MESA)
	 */
	/* GL_EXT_color_table */
	GL_COLOR_TABLE				= 0x80D0,
	GL_POST_CONVOLUTION_COLOR_TABLE		= 0x80D1,
	GL_POST_COLOR_MATRIX_COLOR_TABLE	= 0x80D2,
	GL_PROXY_COLOR_TABLE			= 0x80D3,
	GL_PROXY_POST_CONVOLUTION_COLOR_TABLE	= 0x80D4,
	GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE	= 0x80D5,
	GL_COLOR_TABLE_SCALE			= 0x80D6,
	GL_COLOR_TABLE_BIAS			= 0x80D7,
	GL_COLOR_TABLE_FORMAT			= 0x80D8,
	GL_COLOR_TABLE_WIDTH			= 0x80D9,
	GL_COLOR_TABLE_RED_SIZE			= 0x80DA,
	GL_COLOR_TABLE_GREEN_SIZE		= 0x80DB,
	GL_COLOR_TABLE_BLUE_SIZE		= 0x80DC,
	GL_COLOR_TABLE_ALPHA_SIZE		= 0x80DD,
	GL_COLOR_TABLE_LUMINANCE_SIZE		= 0x80DE,
	GL_COLOR_TABLE_INTENSITY_SIZE		= 0x80DF,
	/* GL_EXT_convolution and GL_HP_convolution_border_modes */
	GL_CONVOLUTION_1D			= 0x8010,
	GL_CONVOLUTION_2D			= 0x8011,
	GL_SEPARABLE_2D				= 0x8012,
	GL_CONVOLUTION_BORDER_MODE		= 0x8013,
	GL_CONVOLUTION_FILTER_SCALE		= 0x8014,
	GL_CONVOLUTION_FILTER_BIAS		= 0x8015,
	GL_REDUCE				= 0x8016,
	GL_CONVOLUTION_FORMAT			= 0x8017,
	GL_CONVOLUTION_WIDTH			= 0x8018,
	GL_CONVOLUTION_HEIGHT			= 0x8019,
	GL_MAX_CONVOLUTION_WIDTH		= 0x801A,
	GL_MAX_CONVOLUTION_HEIGHT		= 0x801B,
	GL_POST_CONVOLUTION_RED_SCALE		= 0x801C,
	GL_POST_CONVOLUTION_GREEN_SCALE		= 0x801D,
	GL_POST_CONVOLUTION_BLUE_SCALE		= 0x801E,
	GL_POST_CONVOLUTION_ALPHA_SCALE		= 0x801F,
	GL_POST_CONVOLUTION_RED_BIAS		= 0x8020,
	GL_POST_CONVOLUTION_GREEN_BIAS		= 0x8021,
	GL_POST_CONVOLUTION_BLUE_BIAS		= 0x8022,
	GL_POST_CONVOLUTION_ALPHA_BIAS		= 0x8023,
	GL_CONSTANT_BORDER			= 0x8151,
	GL_REPLICATE_BORDER			= 0x8153,
	GL_CONVOLUTION_BORDER_COLOR		= 0x8154,
	/* GL_SGI_color_matrix */
	GL_COLOR_MATRIX				= 0x80B1,
	GL_COLOR_MATRIX_STACK_DEPTH		= 0x80B2,
	GL_MAX_COLOR_MATRIX_STACK_DEPTH		= 0x80B3,
	GL_POST_COLOR_MATRIX_RED_SCALE		= 0x80B4,
	GL_POST_COLOR_MATRIX_GREEN_SCALE	= 0x80B5,
	GL_POST_COLOR_MATRIX_BLUE_SCALE		= 0x80B6,
	GL_POST_COLOR_MATRIX_ALPHA_SCALE	= 0x80B7,
	GL_POST_COLOR_MATRIX_RED_BIAS		= 0x80B8,
	GL_POST_COLOR_MATRIX_GREEN_BIAS		= 0x80B9,
	GL_POST_COLOR_MATRIX_BLUE_BIAS		= 0x80BA,
	GL_POST_COLOR_MATRIX_ALPHA_BIAS		= 0x80BB,
	/* GL_EXT_histogram */
	GL_HISTOGRAM				= 0x8024,
	GL_PROXY_HISTOGRAM			= 0x8025,
	GL_HISTOGRAM_WIDTH			= 0x8026,
	GL_HISTOGRAM_FORMAT			= 0x8027,
	GL_HISTOGRAM_RED_SIZE			= 0x8028,
	GL_HISTOGRAM_GREEN_SIZE			= 0x8029,
	GL_HISTOGRAM_BLUE_SIZE			= 0x802A,
	GL_HISTOGRAM_ALPHA_SIZE			= 0x802B,
	GL_HISTOGRAM_LUMINANCE_SIZE		= 0x802C,
	GL_HISTOGRAM_SINK			= 0x802D,
	GL_MINMAX				= 0x802E,
	GL_MINMAX_FORMAT			= 0x802F,
	GL_MINMAX_SINK				= 0x8030,
	GL_TABLE_TOO_LARGE			= 0x8031,
	/* GL_EXT_blend_color, GL_EXT_blend_minmax */
	GL_BLEND_EQUATION			= 0x8009,
	GL_MIN					= 0x8007,
	GL_MAX					= 0x8008,
	GL_FUNC_ADD				= 0x8006,
	GL_FUNC_SUBTRACT			= 0x800A,
	GL_FUNC_REVERSE_SUBTRACT		= 0x800B,
	GL_BLEND_COLOR				= 0x8005,

	/* GL_NV_texgen_reflection (nVidia) */
	GL_NORMAL_MAP_NV			= 0x8511,
	GL_REFLECTION_MAP_NV			= 0x8512,

	/* GL_PGI_misc_hints */
	GL_PREFER_DOUBLEBUFFER_HINT_PGI		= 107000,
	GL_STRICT_DEPTHFUNC_HINT_PGI		= 107030,
	GL_STRICT_LIGHTING_HINT_PGI		= 107031,
	GL_STRICT_SCISSOR_HINT_PGI		= 107032,
	GL_FULL_STIPPLE_HINT_PGI		= 107033,
	GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI	= 107011,
	GL_NATIVE_GRAPHICS_END_HINT_PGI		= 107012,
	GL_CONSERVE_MEMORY_HINT_PGI		= 107005,
	GL_RECLAIM_MEMORY_HINT_PGI		= 107006,
	GL_ALWAYS_FAST_HINT_PGI			= 107020,
	GL_ALWAYS_SOFT_HINT_PGI			= 107021,
	GL_ALLOW_DRAW_OBJ_HINT_PGI		= 107022,
	GL_ALLOW_DRAW_WIN_HINT_PGI		= 107023,
	GL_ALLOW_DRAW_FRG_HINT_PGI		= 107024,
	GL_ALLOW_DRAW_SPN_HINT_PGI		= 107024,
	GL_ALLOW_DRAW_MEM_HINT_PGI		= 107025,
	GL_CLIP_NEAR_HINT_PGI			= 107040,
	GL_CLIP_FAR_HINT_PGI			= 107041,
	GL_WIDE_LINE_HINT_PGI		  	= 107042,
	GL_BACK_NORMALS_HINT_PGI		= 107043,
	GL_NATIVE_GRAPHICS_HANDLE_PGI		= 107010,

	/* GL_EXT_compiled_vertex_array */
	GL_ARRAY_ELEMENT_LOCK_FIRST_SGI		= 0x81A8,
	GL_ARRAY_ELEMENT_LOCK_COUNT_SGI		= 0x81A9,

	/* GL_EXT_clip_volume_hint */
	GL_CLIP_VOLUME_CLIPPING_HINT_EXT	= 0x80F0


/* When you add new enums, please make sure you update the strings
 * in enums.c as well...
 */

}
#ifdef CENTERLINE_CLPP
  /* CenterLine C++ workaround: */
  gl_enum;
  typedef int GLenum;
#else
  /* all other compilers */
  GLenum;
#endif


/* GL_NO_ERROR must be zero */
#define GL_NO_ERROR 0



enum {
	GL_CURRENT_BIT		= 0x00000001,
	GL_POINT_BIT		= 0x00000002,
	GL_LINE_BIT		= 0x00000004,
	GL_POLYGON_BIT		= 0x00000008,
	GL_POLYGON_STIPPLE_BIT	= 0x00000010,
	GL_PIXEL_MODE_BIT	= 0x00000020,
	GL_LIGHTING_BIT		= 0x00000040,
	GL_FOG_BIT		= 0x00000080,
	GL_DEPTH_BUFFER_BIT	= 0x00000100,
	GL_ACCUM_BUFFER_BIT	= 0x00000200,
	GL_STENCIL_BUFFER_BIT	= 0x00000400,
	GL_VIEWPORT_BIT		= 0x00000800,
	GL_TRANSFORM_BIT	= 0x00001000,
	GL_ENABLE_BIT		= 0x00002000,
	GL_COLOR_BUFFER_BIT	= 0x00004000,
	GL_HINT_BIT		= 0x00008000,
	GL_EVAL_BIT		= 0x00010000,
	GL_LIST_BIT		= 0x00020000,
	GL_TEXTURE_BIT		= 0x00040000,
	GL_SCISSOR_BIT		= 0x00080000,
	GL_ALL_ATTRIB_BITS	= 0x000FFFFF
};


enum {
	GL_CLIENT_PIXEL_STORE_BIT	= 0x00000001,
	GL_CLIENT_VERTEX_ARRAY_BIT	= 0x00000002
};
#define GL_CLIENT_ALL_ATTRIB_BITS 0xFFFFFFFF


typedef unsigned int GLbitfield;


#ifdef CENTERLINE_CLPP
#define signed
#endif


/*
 *
 * Data types (may be architecture dependent in some cases)
 *
 */

/*  C type		GL type		storage                            */
/*-------------------------------------------------------------------------*/
typedef void		GLvoid;
typedef unsigned char	GLboolean;
typedef signed char	GLbyte;		/* 1-byte signed */
typedef short		GLshort;	/* 2-byte signed */
typedef int		GLint;		/* 4-byte signed */
typedef unsigned char	GLubyte;	/* 1-byte unsigned */
typedef unsigned short	GLushort;	/* 2-byte unsigned */
typedef unsigned int	GLuint;		/* 4-byte unsigned */
typedef int		GLsizei;	/* 4-byte signed */
typedef float		GLfloat;	/* single precision float */
typedef float		GLclampf;	/* single precision float in [0,1] */
typedef double		GLdouble;	/* double precision float */
typedef double		GLclampd;	/* double precision float in [0,1] */



#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export on
#endif


/*
 * Miscellaneous
 */

GLAPI void GLAPIENTRY glClearIndex( GLfloat c );

GLAPI void GLAPIENTRY glClearColor( GLclampf red,
			  GLclampf green,
			  GLclampf blue,
			  GLclampf alpha );

GLAPI void GLAPIENTRY glClear( GLbitfield mask );

GLAPI void GLAPIENTRY glIndexMask( GLuint mask );

GLAPI void GLAPIENTRY glColorMask( GLboolean red, GLboolean green,
			 GLboolean blue, GLboolean alpha );

GLAPI void GLAPIENTRY glAlphaFunc( GLenum func, GLclampf ref );

GLAPI void GLAPIENTRY glBlendFunc( GLenum sfactor, GLenum dfactor );

GLAPI void GLAPIENTRY glLogicOp( GLenum opcode );

GLAPI void GLAPIENTRY glCullFace( GLenum mode );

GLAPI void GLAPIENTRY glFrontFace( GLenum mode );

GLAPI void GLAPIENTRY glPointSize( GLfloat size );

GLAPI void GLAPIENTRY glLineWidth( GLfloat width );

GLAPI void GLAPIENTRY glLineStipple( GLint factor, GLushort pattern );

GLAPI void GLAPIENTRY glPolygonMode( GLenum face, GLenum mode );

GLAPI void GLAPIENTRY glPolygonOffset( GLfloat factor, GLfloat units );

GLAPI void GLAPIENTRY glPolygonStipple( const GLubyte *mask );

GLAPI void GLAPIENTRY glGetPolygonStipple( GLubyte *mask );

GLAPI void GLAPIENTRY glEdgeFlag( GLboolean flag );

GLAPI void GLAPIENTRY glEdgeFlagv( const GLboolean *flag );

GLAPI void GLAPIENTRY glScissor( GLint x, GLint y,
                                   GLsizei width, GLsizei height);

GLAPI void GLAPIENTRY glClipPlane( GLenum plane, const GLdouble *equation );

GLAPI void GLAPIENTRY glGetClipPlane( GLenum plane, GLdouble *equation );

GLAPI void GLAPIENTRY glDrawBuffer( GLenum mode );

GLAPI void GLAPIENTRY glReadBuffer( GLenum mode );

GLAPI void GLAPIENTRY glEnable( GLenum cap );

GLAPI void GLAPIENTRY glDisable( GLenum cap );

GLAPI GLboolean GLAPIENTRY glIsEnabled( GLenum cap );


GLAPI void GLAPIENTRY glEnableClientState( GLenum cap );  /* 1.1 */

GLAPI void GLAPIENTRY glDisableClientState( GLenum cap );  /* 1.1 */


GLAPI void GLAPIENTRY glGetBooleanv( GLenum pname, GLboolean *params );

GLAPI void GLAPIENTRY glGetDoublev( GLenum pname, GLdouble *params );

GLAPI void GLAPIENTRY glGetFloatv( GLenum pname, GLfloat *params );

GLAPI void GLAPIENTRY glGetIntegerv( GLenum pname, GLint *params );


GLAPI void GLAPIENTRY glPushAttrib( GLbitfield mask );

GLAPI void GLAPIENTRY glPopAttrib( void );


GLAPI void GLAPIENTRY glPushClientAttrib( GLbitfield mask );  /* 1.1 */

GLAPI void GLAPIENTRY glPopClientAttrib( void );  /* 1.1 */


GLAPI GLint GLAPIENTRY glRenderMode( GLenum mode );

GLAPI GLenum GLAPIENTRY glGetError( void );

GLAPI const GLubyte* GLAPIENTRY glGetString( GLenum name );

GLAPI void GLAPIENTRY glFinish( void );

GLAPI void GLAPIENTRY glFlush( void );

GLAPI void GLAPIENTRY glHint( GLenum target, GLenum mode );



/*
 * Depth Buffer
 */

GLAPI void GLAPIENTRY glClearDepth( GLclampd depth );

GLAPI void GLAPIENTRY glDepthFunc( GLenum func );

GLAPI void GLAPIENTRY glDepthMask( GLboolean flag );

GLAPI void GLAPIENTRY glDepthRange( GLclampd near_val, GLclampd far_val );


/*
 * Accumulation Buffer
 */

GLAPI void GLAPIENTRY glClearAccum( GLfloat red, GLfloat green,
                                      GLfloat blue, GLfloat alpha );

GLAPI void GLAPIENTRY glAccum( GLenum op, GLfloat value );



/*
 * Transformation
 */

GLAPI void GLAPIENTRY glMatrixMode( GLenum mode );

GLAPI void GLAPIENTRY glOrtho( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val );

GLAPI void GLAPIENTRY glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val );

GLAPI void GLAPIENTRY glViewport( GLint x, GLint y,
                                    GLsizei width, GLsizei height );

GLAPI void GLAPIENTRY glPushMatrix( void );

GLAPI void GLAPIENTRY glPopMatrix( void );

GLAPI void GLAPIENTRY glLoadIdentity( void );

GLAPI void GLAPIENTRY glLoadMatrixd( const GLdouble *m );
GLAPI void GLAPIENTRY glLoadMatrixf( const GLfloat *m );

GLAPI void GLAPIENTRY glMultMatrixd( const GLdouble *m );
GLAPI void GLAPIENTRY glMultMatrixf( const GLfloat *m );

GLAPI void GLAPIENTRY glRotated( GLdouble angle,
                                   GLdouble x, GLdouble y, GLdouble z );
GLAPI void GLAPIENTRY glRotatef( GLfloat angle,
                                   GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY glScaled( GLdouble x, GLdouble y, GLdouble z );
GLAPI void GLAPIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z );

GLAPI void GLAPIENTRY glTranslated( GLdouble x, GLdouble y, GLdouble z );
GLAPI void GLAPIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z );



/*
 * Display Lists
 */

GLAPI GLboolean GLAPIENTRY glIsList( GLuint list );

GLAPI void GLAPIENTRY glDeleteLists( GLuint list, GLsizei range );

GLAPI GLuint GLAPIENTRY glGenLists( GLsizei range );

GLAPI void GLAPIENTRY glNewList( GLuint list, GLenum mode );

GLAPI void GLAPIENTRY glEndList( void );

GLAPI void GLAPIENTRY glCallList( GLuint list );

GLAPI void GLAPIENTRY glCallLists( GLsizei n, GLenum type,
                                     const GLvoid *lists );

GLAPI void GLAPIENTRY glListBase( GLuint base );



/*
 * Drawing Functions
 */

GLAPI void GLAPIENTRY glBegin( GLenum mode );

GLAPI void GLAPIENTRY glEnd( void );


GLAPI void GLAPIENTRY glVertex2d( GLdouble x, GLdouble y );
GLAPI void GLAPIENTRY glVertex2f( GLfloat x, GLfloat y );
GLAPI void GLAPIENTRY glVertex2i( GLint x, GLint y );
GLAPI void GLAPIENTRY glVertex2s( GLshort x, GLshort y );

GLAPI void GLAPIENTRY glVertex3d( GLdouble x, GLdouble y, GLdouble z );
GLAPI void GLAPIENTRY glVertex3f( GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glVertex3i( GLint x, GLint y, GLint z );
GLAPI void GLAPIENTRY glVertex3s( GLshort x, GLshort y, GLshort z );

GLAPI void GLAPIENTRY glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
GLAPI void GLAPIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
GLAPI void GLAPIENTRY glVertex4i( GLint x, GLint y, GLint z, GLint w );
GLAPI void GLAPIENTRY glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w );

GLAPI void GLAPIENTRY glVertex2dv( const GLdouble *v );
GLAPI void GLAPIENTRY glVertex2fv( const GLfloat *v );
GLAPI void GLAPIENTRY glVertex2iv( const GLint *v );
GLAPI void GLAPIENTRY glVertex2sv( const GLshort *v );

GLAPI void GLAPIENTRY glVertex3dv( const GLdouble *v );
GLAPI void GLAPIENTRY glVertex3fv( const GLfloat *v );
GLAPI void GLAPIENTRY glVertex3iv( const GLint *v );
GLAPI void GLAPIENTRY glVertex3sv( const GLshort *v );

GLAPI void GLAPIENTRY glVertex4dv( const GLdouble *v );
GLAPI void GLAPIENTRY glVertex4fv( const GLfloat *v );
GLAPI void GLAPIENTRY glVertex4iv( const GLint *v );
GLAPI void GLAPIENTRY glVertex4sv( const GLshort *v );


GLAPI void GLAPIENTRY glNormal3b( GLbyte nx, GLbyte ny, GLbyte nz );
GLAPI void GLAPIENTRY glNormal3d( GLdouble nx, GLdouble ny, GLdouble nz );
GLAPI void GLAPIENTRY glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz );
GLAPI void GLAPIENTRY glNormal3i( GLint nx, GLint ny, GLint nz );
GLAPI void GLAPIENTRY glNormal3s( GLshort nx, GLshort ny, GLshort nz );

GLAPI void GLAPIENTRY glNormal3bv( const GLbyte *v );
GLAPI void GLAPIENTRY glNormal3dv( const GLdouble *v );
GLAPI void GLAPIENTRY glNormal3fv( const GLfloat *v );
GLAPI void GLAPIENTRY glNormal3iv( const GLint *v );
GLAPI void GLAPIENTRY glNormal3sv( const GLshort *v );


GLAPI void GLAPIENTRY glIndexd( GLdouble c );
GLAPI void GLAPIENTRY glIndexf( GLfloat c );
GLAPI void GLAPIENTRY glIndexi( GLint c );
GLAPI void GLAPIENTRY glIndexs( GLshort c );
GLAPI void GLAPIENTRY glIndexub( GLubyte c );  /* 1.1 */

GLAPI void GLAPIENTRY glIndexdv( const GLdouble *c );
GLAPI void GLAPIENTRY glIndexfv( const GLfloat *c );
GLAPI void GLAPIENTRY glIndexiv( const GLint *c );
GLAPI void GLAPIENTRY glIndexsv( const GLshort *c );
GLAPI void GLAPIENTRY glIndexubv( const GLubyte *c );  /* 1.1 */

GLAPI void GLAPIENTRY glColor3b( GLbyte red, GLbyte green, GLbyte blue );
GLAPI void GLAPIENTRY glColor3d( GLdouble red, GLdouble green, GLdouble blue );
GLAPI void GLAPIENTRY glColor3f( GLfloat red, GLfloat green, GLfloat blue );
GLAPI void GLAPIENTRY glColor3i( GLint red, GLint green, GLint blue );
GLAPI void GLAPIENTRY glColor3s( GLshort red, GLshort green, GLshort blue );
GLAPI void GLAPIENTRY glColor3ub( GLubyte red, GLubyte green, GLubyte blue );
GLAPI void GLAPIENTRY glColor3ui( GLuint red, GLuint green, GLuint blue );
GLAPI void GLAPIENTRY glColor3us( GLushort red, GLushort green, GLushort blue );

GLAPI void GLAPIENTRY glColor4b( GLbyte red, GLbyte green,
                                   GLbyte blue, GLbyte alpha );
GLAPI void GLAPIENTRY glColor4d( GLdouble red, GLdouble green,
                                   GLdouble blue, GLdouble alpha );
GLAPI void GLAPIENTRY glColor4f( GLfloat red, GLfloat green,
                                   GLfloat blue, GLfloat alpha );
GLAPI void GLAPIENTRY glColor4i( GLint red, GLint green,
                                   GLint blue, GLint alpha );
GLAPI void GLAPIENTRY glColor4s( GLshort red, GLshort green,
                                   GLshort blue, GLshort alpha );
GLAPI void GLAPIENTRY glColor4ub( GLubyte red, GLubyte green,
                                    GLubyte blue, GLubyte alpha );
GLAPI void GLAPIENTRY glColor4ui( GLuint red, GLuint green,
                                    GLuint blue, GLuint alpha );
GLAPI void GLAPIENTRY glColor4us( GLushort red, GLushort green,
                                    GLushort blue, GLushort alpha );


GLAPI void GLAPIENTRY glColor3bv( const GLbyte *v );
GLAPI void GLAPIENTRY glColor3dv( const GLdouble *v );
GLAPI void GLAPIENTRY glColor3fv( const GLfloat *v );
GLAPI void GLAPIENTRY glColor3iv( const GLint *v );
GLAPI void GLAPIENTRY glColor3sv( const GLshort *v );
GLAPI void GLAPIENTRY glColor3ubv( const GLubyte *v );
GLAPI void GLAPIENTRY glColor3uiv( const GLuint *v );
GLAPI void GLAPIENTRY glColor3usv( const GLushort *v );

GLAPI void GLAPIENTRY glColor4bv( const GLbyte *v );
GLAPI void GLAPIENTRY glColor4dv( const GLdouble *v );
GLAPI void GLAPIENTRY glColor4fv( const GLfloat *v );
GLAPI void GLAPIENTRY glColor4iv( const GLint *v );
GLAPI void GLAPIENTRY glColor4sv( const GLshort *v );
GLAPI void GLAPIENTRY glColor4ubv( const GLubyte *v );
GLAPI void GLAPIENTRY glColor4uiv( const GLuint *v );
GLAPI void GLAPIENTRY glColor4usv( const GLushort *v );


GLAPI void GLAPIENTRY glTexCoord1d( GLdouble s );
GLAPI void GLAPIENTRY glTexCoord1f( GLfloat s );
GLAPI void GLAPIENTRY glTexCoord1i( GLint s );
GLAPI void GLAPIENTRY glTexCoord1s( GLshort s );

GLAPI void GLAPIENTRY glTexCoord2d( GLdouble s, GLdouble t );
GLAPI void GLAPIENTRY glTexCoord2f( GLfloat s, GLfloat t );
GLAPI void GLAPIENTRY glTexCoord2i( GLint s, GLint t );
GLAPI void GLAPIENTRY glTexCoord2s( GLshort s, GLshort t );

GLAPI void GLAPIENTRY glTexCoord3d( GLdouble s, GLdouble t, GLdouble r );
GLAPI void GLAPIENTRY glTexCoord3f( GLfloat s, GLfloat t, GLfloat r );
GLAPI void GLAPIENTRY glTexCoord3i( GLint s, GLint t, GLint r );
GLAPI void GLAPIENTRY glTexCoord3s( GLshort s, GLshort t, GLshort r );

GLAPI void GLAPIENTRY glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
GLAPI void GLAPIENTRY glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
GLAPI void GLAPIENTRY glTexCoord4i( GLint s, GLint t, GLint r, GLint q );
GLAPI void GLAPIENTRY glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q );

GLAPI void GLAPIENTRY glTexCoord1dv( const GLdouble *v );
GLAPI void GLAPIENTRY glTexCoord1fv( const GLfloat *v );
GLAPI void GLAPIENTRY glTexCoord1iv( const GLint *v );
GLAPI void GLAPIENTRY glTexCoord1sv( const GLshort *v );

GLAPI void GLAPIENTRY glTexCoord2dv( const GLdouble *v );
GLAPI void GLAPIENTRY glTexCoord2fv( const GLfloat *v );
GLAPI void GLAPIENTRY glTexCoord2iv( const GLint *v );
GLAPI void GLAPIENTRY glTexCoord2sv( const GLshort *v );

GLAPI void GLAPIENTRY glTexCoord3dv( const GLdouble *v );
GLAPI void GLAPIENTRY glTexCoord3fv( const GLfloat *v );
GLAPI void GLAPIENTRY glTexCoord3iv( const GLint *v );
GLAPI void GLAPIENTRY glTexCoord3sv( const GLshort *v );

GLAPI void GLAPIENTRY glTexCoord4dv( const GLdouble *v );
GLAPI void GLAPIENTRY glTexCoord4fv( const GLfloat *v );
GLAPI void GLAPIENTRY glTexCoord4iv( const GLint *v );
GLAPI void GLAPIENTRY glTexCoord4sv( const GLshort *v );


GLAPI void GLAPIENTRY glRasterPos2d( GLdouble x, GLdouble y );
GLAPI void GLAPIENTRY glRasterPos2f( GLfloat x, GLfloat y );
GLAPI void GLAPIENTRY glRasterPos2i( GLint x, GLint y );
GLAPI void GLAPIENTRY glRasterPos2s( GLshort x, GLshort y );

GLAPI void GLAPIENTRY glRasterPos3d( GLdouble x, GLdouble y, GLdouble z );
GLAPI void GLAPIENTRY glRasterPos3f( GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glRasterPos3i( GLint x, GLint y, GLint z );
GLAPI void GLAPIENTRY glRasterPos3s( GLshort x, GLshort y, GLshort z );

GLAPI void GLAPIENTRY glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
GLAPI void GLAPIENTRY glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
GLAPI void GLAPIENTRY glRasterPos4i( GLint x, GLint y, GLint z, GLint w );
GLAPI void GLAPIENTRY glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w );

GLAPI void GLAPIENTRY glRasterPos2dv( const GLdouble *v );
GLAPI void GLAPIENTRY glRasterPos2fv( const GLfloat *v );
GLAPI void GLAPIENTRY glRasterPos2iv( const GLint *v );
GLAPI void GLAPIENTRY glRasterPos2sv( const GLshort *v );

GLAPI void GLAPIENTRY glRasterPos3dv( const GLdouble *v );
GLAPI void GLAPIENTRY glRasterPos3fv( const GLfloat *v );
GLAPI void GLAPIENTRY glRasterPos3iv( const GLint *v );
GLAPI void GLAPIENTRY glRasterPos3sv( const GLshort *v );

GLAPI void GLAPIENTRY glRasterPos4dv( const GLdouble *v );
GLAPI void GLAPIENTRY glRasterPos4fv( const GLfloat *v );
GLAPI void GLAPIENTRY glRasterPos4iv( const GLint *v );
GLAPI void GLAPIENTRY glRasterPos4sv( const GLshort *v );


GLAPI void GLAPIENTRY glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
GLAPI void GLAPIENTRY glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
GLAPI void GLAPIENTRY glRecti( GLint x1, GLint y1, GLint x2, GLint y2 );
GLAPI void GLAPIENTRY glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );


GLAPI void GLAPIENTRY glRectdv( const GLdouble *v1, const GLdouble *v2 );
GLAPI void GLAPIENTRY glRectfv( const GLfloat *v1, const GLfloat *v2 );
GLAPI void GLAPIENTRY glRectiv( const GLint *v1, const GLint *v2 );
GLAPI void GLAPIENTRY glRectsv( const GLshort *v1, const GLshort *v2 );



/*
 * Vertex Arrays  (1.1)
 */

GLAPI void GLAPIENTRY glVertexPointer( GLint size, GLenum type,
                                       GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY glNormalPointer( GLenum type, GLsizei stride,
                                       const GLvoid *ptr );

GLAPI void GLAPIENTRY glColorPointer( GLint size, GLenum type,
                                      GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY glIndexPointer( GLenum type, GLsizei stride,
                                      const GLvoid *ptr );

GLAPI void GLAPIENTRY glTexCoordPointer( GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY glEdgeFlagPointer( GLsizei stride, const GLvoid *ptr );

GLAPI void GLAPIENTRY glGetPointerv( GLenum pname, void **params );

GLAPI void GLAPIENTRY glArrayElement( GLint i );

GLAPI void GLAPIENTRY glDrawArrays( GLenum mode, GLint first, GLsizei count );

GLAPI void GLAPIENTRY glDrawElements( GLenum mode, GLsizei count,
                                      GLenum type, const GLvoid *indices );

GLAPI void GLAPIENTRY glInterleavedArrays( GLenum format, GLsizei stride,
                                           const GLvoid *pointer );


/*
 * Lighting
 */

GLAPI void GLAPIENTRY glShadeModel( GLenum mode );

GLAPI void GLAPIENTRY glLightf( GLenum light, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glLighti( GLenum light, GLenum pname, GLint param );
GLAPI void GLAPIENTRY glLightfv( GLenum light, GLenum pname,
                                 const GLfloat *params );
GLAPI void GLAPIENTRY glLightiv( GLenum light, GLenum pname,
                                 const GLint *params );

GLAPI void GLAPIENTRY glGetLightfv( GLenum light, GLenum pname,
                                    GLfloat *params );
GLAPI void GLAPIENTRY glGetLightiv( GLenum light, GLenum pname,
                                    GLint *params );

GLAPI void GLAPIENTRY glLightModelf( GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glLightModeli( GLenum pname, GLint param );
GLAPI void GLAPIENTRY glLightModelfv( GLenum pname, const GLfloat *params );
GLAPI void GLAPIENTRY glLightModeliv( GLenum pname, const GLint *params );

GLAPI void GLAPIENTRY glMaterialf( GLenum face, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glMateriali( GLenum face, GLenum pname, GLint param );
GLAPI void GLAPIENTRY glMaterialfv( GLenum face, GLenum pname, const GLfloat *params );
GLAPI void GLAPIENTRY glMaterialiv( GLenum face, GLenum pname, const GLint *params );

GLAPI void GLAPIENTRY glGetMaterialfv( GLenum face, GLenum pname, GLfloat *params );
GLAPI void GLAPIENTRY glGetMaterialiv( GLenum face, GLenum pname, GLint *params );

GLAPI void GLAPIENTRY glColorMaterial( GLenum face, GLenum mode );




/*
 * Raster functions
 */

GLAPI void GLAPIENTRY glPixelZoom( GLfloat xfactor, GLfloat yfactor );

GLAPI void GLAPIENTRY glPixelStoref( GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glPixelStorei( GLenum pname, GLint param );

GLAPI void GLAPIENTRY glPixelTransferf( GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glPixelTransferi( GLenum pname, GLint param );

GLAPI void GLAPIENTRY glPixelMapfv( GLenum map, GLint mapsize,
                                    const GLfloat *values );
GLAPI void GLAPIENTRY glPixelMapuiv( GLenum map, GLint mapsize,
                                     const GLuint *values );
GLAPI void GLAPIENTRY glPixelMapusv( GLenum map, GLint mapsize,
                                     const GLushort *values );

GLAPI void GLAPIENTRY glGetPixelMapfv( GLenum map, GLfloat *values );
GLAPI void GLAPIENTRY glGetPixelMapuiv( GLenum map, GLuint *values );
GLAPI void GLAPIENTRY glGetPixelMapusv( GLenum map, GLushort *values );

GLAPI void GLAPIENTRY glBitmap( GLsizei width, GLsizei height,
                                GLfloat xorig, GLfloat yorig,
                                GLfloat xmove, GLfloat ymove,
                                const GLubyte *bitmap );

GLAPI void GLAPIENTRY glReadPixels( GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    GLvoid *pixels );

GLAPI void GLAPIENTRY glDrawPixels( GLsizei width, GLsizei height,
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels );

GLAPI void GLAPIENTRY glCopyPixels( GLint x, GLint y,
                                    GLsizei width, GLsizei height,
                                    GLenum type );



/*
 * Stenciling
 */

GLAPI void GLAPIENTRY glStencilFunc( GLenum func, GLint ref, GLuint mask );

GLAPI void GLAPIENTRY glStencilMask( GLuint mask );

GLAPI void GLAPIENTRY glStencilOp( GLenum fail, GLenum zfail, GLenum zpass );

GLAPI void GLAPIENTRY glClearStencil( GLint s );



/*
 * Texture mapping
 */

GLAPI void GLAPIENTRY glTexGend( GLenum coord, GLenum pname, GLdouble param );
GLAPI void GLAPIENTRY glTexGenf( GLenum coord, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexGeni( GLenum coord, GLenum pname, GLint param );

GLAPI void GLAPIENTRY glTexGendv( GLenum coord, GLenum pname, const GLdouble *params );
GLAPI void GLAPIENTRY glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params );
GLAPI void GLAPIENTRY glTexGeniv( GLenum coord, GLenum pname, const GLint *params );

GLAPI void GLAPIENTRY glGetTexGendv( GLenum coord, GLenum pname, GLdouble *params );
GLAPI void GLAPIENTRY glGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params );
GLAPI void GLAPIENTRY glGetTexGeniv( GLenum coord, GLenum pname, GLint *params );


GLAPI void GLAPIENTRY glTexEnvf( GLenum target, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexEnvi( GLenum target, GLenum pname, GLint param );

GLAPI void GLAPIENTRY glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params );
GLAPI void GLAPIENTRY glTexEnviv( GLenum target, GLenum pname, const GLint *params );

GLAPI void GLAPIENTRY glGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params );
GLAPI void GLAPIENTRY glGetTexEnviv( GLenum target, GLenum pname, GLint *params );


GLAPI void GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param );

GLAPI void GLAPIENTRY glTexParameterfv( GLenum target, GLenum pname,
                                          const GLfloat *params );
GLAPI void GLAPIENTRY glTexParameteriv( GLenum target, GLenum pname,
                                          const GLint *params );

GLAPI void GLAPIENTRY glGetTexParameterfv( GLenum target,
                                           GLenum pname, GLfloat *params);
GLAPI void GLAPIENTRY glGetTexParameteriv( GLenum target,
                                           GLenum pname, GLint *params );

GLAPI void GLAPIENTRY glGetTexLevelParameterfv( GLenum target, GLint level,
                                                GLenum pname, GLfloat *params );
GLAPI void GLAPIENTRY glGetTexLevelParameteriv( GLenum target, GLint level,
                                                GLenum pname, GLint *params );


GLAPI void GLAPIENTRY glTexImage1D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLint border,
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels );

GLAPI void GLAPIENTRY glTexImage2D( GLenum target, GLint level,
                                    GLint internalFormat,
                                    GLsizei width, GLsizei height,
                                    GLint border, GLenum format, GLenum type,
                                    const GLvoid *pixels );

GLAPI void GLAPIENTRY glGetTexImage( GLenum target, GLint level,
                                     GLenum format, GLenum type,
                                     GLvoid *pixels );



/* 1.1 functions */

GLAPI void GLAPIENTRY glGenTextures( GLsizei n, GLuint *textures );

GLAPI void GLAPIENTRY glDeleteTextures( GLsizei n, const GLuint *textures);

GLAPI void GLAPIENTRY glBindTexture( GLenum target, GLuint texture );

GLAPI void GLAPIENTRY glPrioritizeTextures( GLsizei n,
                                            const GLuint *textures,
                                            const GLclampf *priorities );

GLAPI GLboolean GLAPIENTRY glAreTexturesResident( GLsizei n,
                                                  const GLuint *textures,
                                                  GLboolean *residences );

GLAPI GLboolean GLAPIENTRY glIsTexture( GLuint texture );


GLAPI void GLAPIENTRY glTexSubImage1D( GLenum target, GLint level,
                                       GLint xoffset,
                                       GLsizei width, GLenum format,
                                       GLenum type, const GLvoid *pixels );


GLAPI void GLAPIENTRY glTexSubImage2D( GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels );


GLAPI void GLAPIENTRY glCopyTexImage1D( GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLint border );


GLAPI void GLAPIENTRY glCopyTexImage2D( GLenum target, GLint level,
                                        GLenum internalformat,
                                        GLint x, GLint y,
                                        GLsizei width, GLsizei height,
                                        GLint border );


GLAPI void GLAPIENTRY glCopyTexSubImage1D( GLenum target, GLint level,
                                           GLint xoffset, GLint x, GLint y,
                                           GLsizei width );


GLAPI void GLAPIENTRY glCopyTexSubImage2D( GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height );




/*
 * Evaluators
 */

GLAPI void GLAPIENTRY glMap1d( GLenum target, GLdouble u1, GLdouble u2,
                               GLint stride,
                               GLint order, const GLdouble *points );
GLAPI void GLAPIENTRY glMap1f( GLenum target, GLfloat u1, GLfloat u2,
                               GLint stride,
                               GLint order, const GLfloat *points );

GLAPI void GLAPIENTRY glMap2d( GLenum target,
		     GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
		     GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
		     const GLdouble *points );
GLAPI void GLAPIENTRY glMap2f( GLenum target,
		     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		     const GLfloat *points );

GLAPI void GLAPIENTRY glGetMapdv( GLenum target, GLenum query, GLdouble *v );
GLAPI void GLAPIENTRY glGetMapfv( GLenum target, GLenum query, GLfloat *v );
GLAPI void GLAPIENTRY glGetMapiv( GLenum target, GLenum query, GLint *v );

GLAPI void GLAPIENTRY glEvalCoord1d( GLdouble u );
GLAPI void GLAPIENTRY glEvalCoord1f( GLfloat u );

GLAPI void GLAPIENTRY glEvalCoord1dv( const GLdouble *u );
GLAPI void GLAPIENTRY glEvalCoord1fv( const GLfloat *u );

GLAPI void GLAPIENTRY glEvalCoord2d( GLdouble u, GLdouble v );
GLAPI void GLAPIENTRY glEvalCoord2f( GLfloat u, GLfloat v );

GLAPI void GLAPIENTRY glEvalCoord2dv( const GLdouble *u );
GLAPI void GLAPIENTRY glEvalCoord2fv( const GLfloat *u );

GLAPI void GLAPIENTRY glMapGrid1d( GLint un, GLdouble u1, GLdouble u2 );
GLAPI void GLAPIENTRY glMapGrid1f( GLint un, GLfloat u1, GLfloat u2 );

GLAPI void GLAPIENTRY glMapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                                   GLint vn, GLdouble v1, GLdouble v2 );
GLAPI void GLAPIENTRY glMapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                                   GLint vn, GLfloat v1, GLfloat v2 );

GLAPI void GLAPIENTRY glEvalPoint1( GLint i );

GLAPI void GLAPIENTRY glEvalPoint2( GLint i, GLint j );

GLAPI void GLAPIENTRY glEvalMesh1( GLenum mode, GLint i1, GLint i2 );

GLAPI void GLAPIENTRY glEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );



/*
 * Fog
 */

GLAPI void GLAPIENTRY glFogf( GLenum pname, GLfloat param );

GLAPI void GLAPIENTRY glFogi( GLenum pname, GLint param );

GLAPI void GLAPIENTRY glFogfv( GLenum pname, const GLfloat *params );

GLAPI void GLAPIENTRY glFogiv( GLenum pname, const GLint *params );



/*
 * Selection and Feedback
 */

GLAPI void GLAPIENTRY glFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer );

GLAPI void GLAPIENTRY glPassThrough( GLfloat token );

GLAPI void GLAPIENTRY glSelectBuffer( GLsizei size, GLuint *buffer );

GLAPI void GLAPIENTRY glInitNames( void );

GLAPI void GLAPIENTRY glLoadName( GLuint name );

GLAPI void GLAPIENTRY glPushName( GLuint name );

GLAPI void GLAPIENTRY glPopName( void );



/*
 * Extensions
 */

/* GL_EXT_blend_minmax */
GLAPI void GLAPIENTRY glBlendEquationEXT( GLenum mode );



/* GL_EXT_blend_color */
GLAPI void GLAPIENTRY glBlendColorEXT( GLclampf red, GLclampf green,
                                       GLclampf blue, GLclampf alpha );



/* GL_EXT_polygon_offset */
GLAPI void GLAPIENTRY glPolygonOffsetEXT( GLfloat factor, GLfloat bias );



/* GL_EXT_vertex_array */

GLAPI void GLAPIENTRY glVertexPointerEXT( GLint size, GLenum type,
                                          GLsizei stride,
                                          GLsizei count, const GLvoid *ptr );

GLAPI void GLAPIENTRY glNormalPointerEXT( GLenum type, GLsizei stride,
                                          GLsizei count, const GLvoid *ptr );

GLAPI void GLAPIENTRY glColorPointerEXT( GLint size, GLenum type,
                                         GLsizei stride,
                                         GLsizei count, const GLvoid *ptr );

GLAPI void GLAPIENTRY glIndexPointerEXT( GLenum type, GLsizei stride,
                                         GLsizei count, const GLvoid *ptr );

GLAPI void GLAPIENTRY glTexCoordPointerEXT( GLint size, GLenum type,
                                            GLsizei stride, GLsizei count,
                                            const GLvoid *ptr );

GLAPI void GLAPIENTRY glEdgeFlagPointerEXT( GLsizei stride, GLsizei count,
                                            const GLboolean *ptr );

GLAPI void GLAPIENTRY glGetPointervEXT( GLenum pname, void **params );

GLAPI void GLAPIENTRY glArrayElementEXT( GLint i );

GLAPI void GLAPIENTRY glDrawArraysEXT( GLenum mode, GLint first,
                                       GLsizei count );



/* GL_EXT_texture_object */

GLAPI void GLAPIENTRY glGenTexturesEXT( GLsizei n, GLuint *textures );

GLAPI void GLAPIENTRY glDeleteTexturesEXT( GLsizei n, const GLuint *textures);

GLAPI void GLAPIENTRY glBindTextureEXT( GLenum target, GLuint texture );

GLAPI void GLAPIENTRY glPrioritizeTexturesEXT( GLsizei n,
                                               const GLuint *textures,
                                               const GLclampf *priorities );

GLAPI GLboolean GLAPIENTRY glAreTexturesResidentEXT( GLsizei n,
                                                     const GLuint *textures,
                                                     GLboolean *residences );

GLAPI GLboolean GLAPIENTRY glIsTextureEXT( GLuint texture );



/* GL_EXT_texture3D */

GLAPI void GLAPIENTRY glTexImage3DEXT( GLenum target, GLint level,
                                       GLenum internalFormat,
                                       GLsizei width, GLsizei height,
                                       GLsizei depth, GLint border,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels );

GLAPI void GLAPIENTRY glTexSubImage3DEXT( GLenum target, GLint level,
                                          GLint xoffset, GLint yoffset,
                                          GLint zoffset, GLsizei width,
                                          GLsizei height, GLsizei depth,
                                          GLenum format,
                                          GLenum type, const GLvoid *pixels);

GLAPI void GLAPIENTRY glCopyTexSubImage3DEXT( GLenum target, GLint level,
                                              GLint xoffset, GLint yoffset,
                                              GLint zoffset, GLint x,
                                              GLint y, GLsizei width,
                                              GLsizei height );



/* GL_EXT_color_table */

GLAPI void GLAPIENTRY glColorTableEXT( GLenum target, GLenum internalformat,
                                       GLsizei width, GLenum format,
                                       GLenum type, const GLvoid *table );

GLAPI void GLAPIENTRY glColorSubTableEXT( GLenum target,
                                          GLsizei start, GLsizei count,
                                          GLenum format, GLenum type,
                                          const GLvoid *data );

GLAPI void GLAPIENTRY glGetColorTableEXT( GLenum target, GLenum format,
                                          GLenum type, GLvoid *table );

GLAPI void GLAPIENTRY glGetColorTableParameterfvEXT( GLenum target,
                                                     GLenum pname,
                                                     GLfloat *params );

GLAPI void GLAPIENTRY glGetColorTableParameterivEXT( GLenum target,
                                                     GLenum pname,
                                                     GLint *params );


/* GL_ARB_multitexture */

GLAPI void GLAPIENTRY glActiveTextureARB(GLenum texture);
GLAPI void GLAPIENTRY glClientActiveTextureARB(GLenum texture);
GLAPI void GLAPIENTRY glMultiTexCoord1dARB(GLenum target, GLdouble s);
GLAPI void GLAPIENTRY glMultiTexCoord1dvARB(GLenum target, const GLdouble *v);
GLAPI void GLAPIENTRY glMultiTexCoord1fARB(GLenum target, GLfloat s);
GLAPI void GLAPIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat *v);
GLAPI void GLAPIENTRY glMultiTexCoord1iARB(GLenum target, GLint s);
GLAPI void GLAPIENTRY glMultiTexCoord1ivARB(GLenum target, const GLint *v);
GLAPI void GLAPIENTRY glMultiTexCoord1sARB(GLenum target, GLshort s);
GLAPI void GLAPIENTRY glMultiTexCoord1svARB(GLenum target, const GLshort *v);
GLAPI void GLAPIENTRY glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t);
GLAPI void GLAPIENTRY glMultiTexCoord2dvARB(GLenum target, const GLdouble *v);
GLAPI void GLAPIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t);
GLAPI void GLAPIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat *v);
GLAPI void GLAPIENTRY glMultiTexCoord2iARB(GLenum target, GLint s, GLint t);
GLAPI void GLAPIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v);
GLAPI void GLAPIENTRY glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t);
GLAPI void GLAPIENTRY glMultiTexCoord2svARB(GLenum target, const GLshort *v);
GLAPI void GLAPIENTRY glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r);
GLAPI void GLAPIENTRY glMultiTexCoord3dvARB(GLenum target, const GLdouble *v);
GLAPI void GLAPIENTRY glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r);
GLAPI void GLAPIENTRY glMultiTexCoord3fvARB(GLenum target, const GLfloat *v);
GLAPI void GLAPIENTRY glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r);
GLAPI void GLAPIENTRY glMultiTexCoord3ivARB(GLenum target, const GLint *v);
GLAPI void GLAPIENTRY glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r);
GLAPI void GLAPIENTRY glMultiTexCoord3svARB(GLenum target, const GLshort *v);
GLAPI void GLAPIENTRY glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLAPI void GLAPIENTRY glMultiTexCoord4dvARB(GLenum target, const GLdouble *v);
GLAPI void GLAPIENTRY glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLAPI void GLAPIENTRY glMultiTexCoord4fvARB(GLenum target, const GLfloat *v);
GLAPI void GLAPIENTRY glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
GLAPI void GLAPIENTRY glMultiTexCoord4ivARB(GLenum target, const GLint *v);
GLAPI void GLAPIENTRY glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GLAPI void GLAPIENTRY glMultiTexCoord4svARB(GLenum target, const GLshort *v);



/* GL_EXT_point_parameters */
GLAPI void GLAPIENTRY glPointParameterfEXT( GLenum pname, GLfloat param );
GLAPI void GLAPIENTRY glPointParameterfvEXT( GLenum pname,
                                               const GLfloat *params );



/* GL_INGR_blend_func_separate */
GLAPI void GLAPIENTRY glBlendFuncSeparateINGR( GLenum sfactorRGB,
                                               GLenum dfactorRGB,
                                               GLenum sfactorAlpha,
                                               GLenum dfactorAlpha );



/* GL_MESA_window_pos */

GLAPI void GLAPIENTRY glWindowPos2iMESA( GLint x, GLint y );
GLAPI void GLAPIENTRY glWindowPos2sMESA( GLshort x, GLshort y );
GLAPI void GLAPIENTRY glWindowPos2fMESA( GLfloat x, GLfloat y );
GLAPI void GLAPIENTRY glWindowPos2dMESA( GLdouble x, GLdouble y );

GLAPI void GLAPIENTRY glWindowPos2ivMESA( const GLint *p );
GLAPI void GLAPIENTRY glWindowPos2svMESA( const GLshort *p );
GLAPI void GLAPIENTRY glWindowPos2fvMESA( const GLfloat *p );
GLAPI void GLAPIENTRY glWindowPos2dvMESA( const GLdouble *p );

GLAPI void GLAPIENTRY glWindowPos3iMESA( GLint x, GLint y, GLint z );
GLAPI void GLAPIENTRY glWindowPos3sMESA( GLshort x, GLshort y, GLshort z );
GLAPI void GLAPIENTRY glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z );
GLAPI void GLAPIENTRY glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z );

GLAPI void GLAPIENTRY glWindowPos3ivMESA( const GLint *p );
GLAPI void GLAPIENTRY glWindowPos3svMESA( const GLshort *p );
GLAPI void GLAPIENTRY glWindowPos3fvMESA( const GLfloat *p );
GLAPI void GLAPIENTRY glWindowPos3dvMESA( const GLdouble *p );

GLAPI void GLAPIENTRY glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w );
GLAPI void GLAPIENTRY glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w );
GLAPI void GLAPIENTRY glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
GLAPI void GLAPIENTRY glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w);

GLAPI void GLAPIENTRY glWindowPos4ivMESA( const GLint *p );
GLAPI void GLAPIENTRY glWindowPos4svMESA( const GLshort *p );
GLAPI void GLAPIENTRY glWindowPos4fvMESA( const GLfloat *p );
GLAPI void GLAPIENTRY glWindowPos4dvMESA( const GLdouble *p );


/* GL_MESA_resize_buffers */

GLAPI void GLAPIENTRY glResizeBuffersMESA( void );


/* 1.2 functions */
GLAPI void GLAPIENTRY glDrawRangeElements( GLenum mode, GLuint start,
	GLuint end, GLsizei count, GLenum type, const GLvoid *indices );

GLAPI void GLAPIENTRY glTexImage3D( GLenum target, GLint level,
                                      GLint internalFormat,
                                      GLsizei width, GLsizei height,
                                      GLsizei depth, GLint border,
                                      GLenum format, GLenum type,
                                      const GLvoid *pixels );

GLAPI void GLAPIENTRY glTexSubImage3D( GLenum target, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLint zoffset, GLsizei width,
                                         GLsizei height, GLsizei depth,
                                         GLenum format,
                                         GLenum type, const GLvoid *pixels);

GLAPI void GLAPIENTRY glCopyTexSubImage3D( GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLint zoffset, GLint x,
                                             GLint y, GLsizei width,
                                             GLsizei height );


/* 1.2 imaging extension functions */

GLAPI void GLAPIENTRY glColorTable( GLenum target, GLenum internalformat,
                                    GLsizei width, GLenum format,
                                    GLenum type, const GLvoid *table );

GLAPI void GLAPIENTRY glColorSubTable( GLenum target,
                                       GLsizei start, GLsizei count,
                                       GLenum format, GLenum type,
                                       const GLvoid *data );

GLAPI void GLAPIENTRY glColorTableParameteriv(GLenum target, GLenum pname,
                                              const GLint *params);

GLAPI void GLAPIENTRY glColorTableParameterfv(GLenum target, GLenum pname,
                                              const GLfloat *params);

GLAPI void GLAPIENTRY glCopyColorSubTable( GLenum target, GLsizei start,
                                           GLint x, GLint y, GLsizei width );

GLAPI void GLAPIENTRY glCopyColorTable( GLenum target, GLenum internalformat,
                                        GLint x, GLint y, GLsizei width );

GLAPI void GLAPIENTRY glGetColorTable( GLenum target, GLenum format,
                                       GLenum type, GLvoid *table );

GLAPI void GLAPIENTRY glGetColorTableParameterfv( GLenum target, GLenum pname,
                                                  GLfloat *params );

GLAPI void GLAPIENTRY glGetColorTableParameteriv( GLenum target, GLenum pname,
                                                  GLint *params );

GLAPI void GLAPIENTRY glBlendEquation( GLenum mode );

GLAPI void GLAPIENTRY glBlendColor( GLclampf red, GLclampf green,
                                    GLclampf blue, GLclampf alpha );

GLAPI void GLAPIENTRY glHistogram( GLenum target, GLsizei width,
				   GLenum internalformat, GLboolean sink );

GLAPI void GLAPIENTRY glResetHistogram( GLenum target );

GLAPI void GLAPIENTRY glGetHistogram( GLenum target, GLboolean reset,
				      GLenum format, GLenum type,
				      GLvoid *values );

GLAPI void GLAPIENTRY glGetHistogramParameterfv( GLenum target, GLenum pname,
						 GLfloat *params );

GLAPI void GLAPIENTRY glGetHistogramParameteriv( GLenum target, GLenum pname,
						 GLint *params );

GLAPI void GLAPIENTRY glMinmax( GLenum target, GLenum internalformat,
				GLboolean sink );

GLAPI void GLAPIENTRY glResetMinmax( GLenum target );

GLAPI void GLAPIENTRY glGetMinmax( GLenum target, GLboolean reset,
                                   GLenum format, GLenum types,
                                   GLvoid *values );

GLAPI void GLAPIENTRY glGetMinmaxParameterfv( GLenum target, GLenum pname,
					      GLfloat *params );

GLAPI void GLAPIENTRY glGetMinmaxParameteriv( GLenum target, GLenum pname,
					      GLint *params );

GLAPI void GLAPIENTRY glConvolutionFilter1D( GLenum target,
	GLenum internalformat, GLsizei width, GLenum format, GLenum type,
	const GLvoid *image );

GLAPI void GLAPIENTRY glConvolutionFilter2D( GLenum target,
	GLenum internalformat, GLsizei width, GLsizei height, GLenum format,
	GLenum type, const GLvoid *image );

GLAPI void GLAPIENTRY glConvolutionParameterf( GLenum target, GLenum pname,
	GLfloat params );

GLAPI void GLAPIENTRY glConvolutionParameterfv( GLenum target, GLenum pname,
	const GLfloat *params );

GLAPI void GLAPIENTRY glConvolutionParameteri( GLenum target, GLenum pname,
	GLint params );

GLAPI void GLAPIENTRY glConvolutionParameteriv( GLenum target, GLenum pname,
	const GLint *params );

GLAPI void GLAPIENTRY glCopyConvolutionFilter1D( GLenum target,
	GLenum internalformat, GLint x, GLint y, GLsizei width );

GLAPI void GLAPIENTRY glCopyConvolutionFilter2D( GLenum target,
	GLenum internalformat, GLint x, GLint y, GLsizei width,
	GLsizei height);

GLAPI void GLAPIENTRY glGetConvolutionFilter( GLenum target, GLenum format,
	GLenum type, GLvoid *image );

GLAPI void GLAPIENTRY glGetConvolutionParameterfv( GLenum target, GLenum pname,
	GLfloat *params );

GLAPI void GLAPIENTRY glGetConvolutionParameteriv( GLenum target, GLenum pname,
	GLint *params );

GLAPI void GLAPIENTRY glSeparableFilter2D( GLenum target,
	GLenum internalformat, GLsizei width, GLsizei height, GLenum format,
	GLenum type, const GLvoid *row, const GLvoid *column );

GLAPI void GLAPIENTRY glGetSeparableFilter( GLenum target, GLenum format,
	GLenum type, GLvoid *row, GLvoid *column, GLvoid *span );



/* GL_EXT_compiled_vertex_array */
GLAPI void GLAPIENTRY glLockArraysEXT( GLint first, GLsizei count );
GLAPI void GLAPIENTRY glUnlockArraysEXT( void );



#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export off
#endif


/*
 * Compile-time tests for extensions:
 */
#define GL_EXT_blend_color		1
#define GL_EXT_blend_logic_op		1
#define GL_EXT_blend_minmax		1
#define GL_EXT_blend_subtract		1
#define GL_EXT_polygon_offset		1
#define GL_EXT_vertex_array		1
#define GL_EXT_texture_object		1
#define GL_EXT_texture3D		1
#define GL_EXT_paletted_texture		1
#define GL_EXT_shared_texture_palette	1
#define GL_EXT_point_parameters		1
#define GL_EXT_rescale_normal		1
#define GL_EXT_abgr			1
#define GL_EXT_stencil_wrap		1
#define GL_MESA_window_pos		1
#define GL_MESA_resize_buffers		1
#define GL_SGIS_texture_edge_clamp	1
#define GL_INGR_blend_func_separate	1
#define GL_ARB_multitexture		1
#define GL_NV_texgen_reflection		1
#define GL_PGI_misc_hints               1
#define GL_EXT_compiled_vertex_array    1
#define GL_EXT_clip_volume_hint         1


#ifdef macintosh
	#pragma enumsalwaysint reset
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import off
	#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
