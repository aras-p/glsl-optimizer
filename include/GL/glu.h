/* $Id: glu.h,v 1.19 1999/11/24 13:06:48 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1995-1999  Brian Paul
 *
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
 */


#ifndef __glu_h__
#define __glu_h__


#if defined(USE_MGL_NAMESPACE)
#include "glu_mangle.h"
#endif

#include "GL/gl.h"


#ifdef __cplusplus
extern "C" {
#endif


	/* to facilitate clean DLL building ... */
#if !defined(OPENSTEP) && (defined(__WIN32__) || defined(__CYGWIN32__))
#	if defined(_MSC_VER) && defined(BUILD_GLU32) /* tag specify we're building mesa as a DLL */
#		define GLUAPI __declspec(dllexport)
#	elif defined(_MSC_VER) && defined(_DLL) /* tag specifying we're building for DLL runtime support */
#		define GLUAPI __declspec(dllimport)
#	else /* for use with static link lib build of Win32 edition only */
#		define GLUAPI extern
#	endif /* _STATIC_MESA support */
#else
#	define GLUAPI extern
#endif /* WIN32 / CYGWIN32 bracket */

#ifdef macintosh
	#pragma enumsalwaysint on
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import on
	#endif
#endif

#ifndef GLUAPI
#define GLUAPI
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef GLCALLBACK
#define GLCALLBACK
#endif


#define GLU_VERSION_1_1			1
#define GLU_VERSION_1_2			1


#define GLU_TRUE			1
#define GLU_FALSE			0


/* Normal vectors */
#define GLU_SMOOTH			100000
#define GLU_FLAT			100001
#define GLU_NONE			100002

/* Quadric draw styles */
#define GLU_POINT			100010
#define GLU_LINE			100011
#define GLU_FILL			100012
#define GLU_SILHOUETTE			100013

/* Quadric orientation */
#define GLU_OUTSIDE			100020
#define GLU_INSIDE			100021

/* Tessellator */
#define GLU_TESS_BEGIN			100100
#define GLU_TESS_VERTEX			100101
#define GLU_TESS_END			100102
#define GLU_TESS_ERROR			100103
#define GLU_TESS_EDGE_FLAG		100104
#define GLU_TESS_COMBINE		100105

#define GLU_TESS_BEGIN_DATA		100106
#define GLU_TESS_VERTEX_DATA		100107
#define GLU_TESS_END_DATA		100108
#define GLU_TESS_ERROR_DATA		100109
#define GLU_TESS_EDGE_FLAG_DATA		100110
#define GLU_TESS_COMBINE_DATA		100111

/* Winding rules */
#define GLU_TESS_WINDING_ODD		100130
#define GLU_TESS_WINDING_NONZERO	100131
#define GLU_TESS_WINDING_POSITIVE	100132
#define GLU_TESS_WINDING_NEGATIVE	100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO	100134

/* Tessellation properties */
#define GLU_TESS_WINDING_RULE		100140
#define GLU_TESS_BOUNDARY_ONLY		100141
#define GLU_TESS_TOLERANCE		100142

/* Tessellation errors */
#define GLU_TESS_ERROR1			100151  /* Missing gluBeginPolygon */
#define GLU_TESS_ERROR2			100152  /* Missing gluBeginContour */
#define GLU_TESS_ERROR3			100153  /* Missing gluEndPolygon */
#define GLU_TESS_ERROR4			100154  /* Missing gluEndContour */
#define GLU_TESS_ERROR5			100155  /* */
#define GLU_TESS_ERROR6			100156  /* */
#define GLU_TESS_ERROR7			100157  /* */
#define GLU_TESS_ERROR8			100158  /* */

/* NURBS */
#define GLU_AUTO_LOAD_MATRIX		100200
#define GLU_CULLING			100201
#define GLU_PARAMETRIC_TOLERANCE	100202
#define GLU_SAMPLING_TOLERANCE		100203
#define GLU_DISPLAY_MODE		100204
#define GLU_SAMPLING_METHOD		100205
#define GLU_U_STEP			100206
#define GLU_V_STEP			100207

#define GLU_PATH_LENGTH			100215
#define GLU_PARAMETRIC_ERROR		100216
#define GLU_DOMAIN_DISTANCE		100217

#define GLU_MAP1_TRIM_2			100210
#define GLU_MAP1_TRIM_3			100211

#define GLU_OUTLINE_POLYGON		100240
#define GLU_OUTLINE_PATCH		100241

#define GLU_NURBS_ERROR1	100251   /* spline order un-supported */
#define GLU_NURBS_ERROR2	100252   /* too few knots */
#define GLU_NURBS_ERROR3	100253   /* valid knot range is empty */
#define GLU_NURBS_ERROR4	100254   /* decreasing knot sequence */
#define GLU_NURBS_ERROR5	100255   /* knot multiplicity > spline order */
#define GLU_NURBS_ERROR6	100256   /* endcurve() must follow bgncurve() */
#define GLU_NURBS_ERROR7	100257   /* bgncurve() must precede endcurve() */
#define GLU_NURBS_ERROR8	100258   /* ctrlarray or knot vector is NULL */
#define GLU_NURBS_ERROR9 	100259   /* can't draw pwlcurves */
#define GLU_NURBS_ERROR10	100260   /* missing gluNurbsCurve() */
#define GLU_NURBS_ERROR11	100261   /* missing gluNurbsSurface() */
#define GLU_NURBS_ERROR12	100262   /* endtrim() must precede endsurface() */
#define GLU_NURBS_ERROR13	100263   /* bgnsurface() must precede endsurface() */
#define GLU_NURBS_ERROR14	100264   /* curve of improper type passed as trim curve */
#define GLU_NURBS_ERROR15	100265   /* bgnsurface() must precede bgntrim() */
#define GLU_NURBS_ERROR16	100266   /* endtrim() must follow bgntrim() */
#define GLU_NURBS_ERROR17	100267   /* bgntrim() must precede endtrim()*/
#define GLU_NURBS_ERROR18	100268   /* invalid or missing trim curve*/
#define GLU_NURBS_ERROR19	100269   /* bgntrim() must precede pwlcurve() */
#define GLU_NURBS_ERROR20	100270   /* pwlcurve referenced twice*/
#define GLU_NURBS_ERROR21	100271   /* pwlcurve and nurbscurve mixed */
#define GLU_NURBS_ERROR22	100272   /* improper usage of trim data type */
#define GLU_NURBS_ERROR23	100273   /* nurbscurve referenced twice */
#define GLU_NURBS_ERROR24	100274   /* nurbscurve and pwlcurve mixed */
#define GLU_NURBS_ERROR25	100275   /* nurbssurface referenced twice */
#define GLU_NURBS_ERROR26	100276   /* invalid property */
#define GLU_NURBS_ERROR27	100277   /* endsurface() must follow bgnsurface() */
#define GLU_NURBS_ERROR28	100278   /* intersecting or misoriented trim curves */
#define GLU_NURBS_ERROR29	100279   /* intersecting trim curves */
#define GLU_NURBS_ERROR30	100280   /* UNUSED */
#define GLU_NURBS_ERROR31	100281   /* unconnected trim curves */
#define GLU_NURBS_ERROR32	100282   /* unknown knot error */
#define GLU_NURBS_ERROR33	100283   /* negative vertex count encountered */
#define GLU_NURBS_ERROR34	100284   /* negative byte-stride */
#define GLU_NURBS_ERROR35	100285   /* unknown type descriptor */
#define GLU_NURBS_ERROR36	100286   /* null control point reference */
#define GLU_NURBS_ERROR37	100287   /* duplicate point on pwlcurve */

/* GLU 1.3 and later */
#define GLU_NURBS_MODE ?


/* Errors */
#define GLU_INVALID_ENUM		100900
#define GLU_INVALID_VALUE		100901
#define GLU_OUT_OF_MEMORY		100902
#define GLU_INCOMPATIBLE_GL_VERSION	100903

/* GLU 1.1 and later */
#define GLU_VERSION			100800
#define GLU_EXTENSIONS			100801



/*** GLU 1.0 tessellation - obsolete! ***/

/* Contour types */
#define GLU_CW				100120
#define GLU_CCW				100121
#define GLU_INTERIOR			100122
#define GLU_EXTERIOR			100123
#define GLU_UNKNOWN			100124

/* Tessellator */
#define GLU_BEGIN			GLU_TESS_BEGIN
#define GLU_VERTEX			GLU_TESS_VERTEX
#define GLU_END				GLU_TESS_END
#define GLU_ERROR			GLU_TESS_ERROR
#define GLU_EDGE_FLAG			GLU_TESS_EDGE_FLAG


/*
 * These are the GLU 1.1 typedefs.  GLU 1.3 has different ones!
 */
#if defined(__BEOS__)
    /* The BeOS does something funky and makes these typedefs in one
     * of its system headers.
     */
#else
    typedef struct GLUquadric GLUquadricObj;
    typedef struct GLUnurbs GLUnurbsObj;

    /* FIXME: We need to implement the other 1.3 typedefs - GH */
    typedef struct GLUtesselator GLUtesselator;
    typedef GLUtesselator GLUtriangulatorObj;
#endif



#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export on
#endif


/*
 *
 * Miscellaneous functions
 *
 */

GLUAPI void GLAPIENTRY gluLookAt( GLdouble eyex, GLdouble eyey, GLdouble eyez,
                                  GLdouble centerx, GLdouble centery,
                                  GLdouble centerz,
                                  GLdouble upx, GLdouble upy, GLdouble upz );


GLUAPI void GLAPIENTRY gluOrtho2D( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top );


GLUAPI void GLAPIENTRY gluPerspective( GLdouble fovy, GLdouble aspect,
                                       GLdouble zNear, GLdouble zFar );


GLUAPI void GLAPIENTRY gluPickMatrix( GLdouble x, GLdouble y,
                                      GLdouble width, GLdouble height,
                                      const GLint viewport[4] );

GLUAPI GLint GLAPIENTRY gluProject( GLdouble objx, GLdouble objy, GLdouble objz,
                                    const GLdouble modelMatrix[16],
                                    const GLdouble projMatrix[16],
                                    const GLint viewport[4],
                                    GLdouble *winx, GLdouble *winy,
                                    GLdouble *winz );

GLUAPI GLint GLAPIENTRY gluUnProject( GLdouble winx, GLdouble winy,
                                      GLdouble winz,
                                      const GLdouble modelMatrix[16],
                                      const GLdouble projMatrix[16],
                                      const GLint viewport[4],
                                      GLdouble *objx, GLdouble *objy,
                                      GLdouble *objz );

GLUAPI const GLubyte* GLAPIENTRY gluErrorString( GLenum errorCode );



/*
 *
 * Mipmapping and image scaling
 *
 */

GLUAPI GLint GLAPIENTRY gluScaleImage( GLenum format,
                                       GLint widthin, GLint heightin,
                                       GLenum typein, const void *datain,
                                       GLint widthout, GLint heightout,
                                       GLenum typeout, void *dataout );

GLUAPI GLint GLAPIENTRY gluBuild1DMipmaps( GLenum target, GLint components,
                                           GLint width, GLenum format,
                                           GLenum type, const void *data );

GLUAPI GLint GLAPIENTRY gluBuild2DMipmaps( GLenum target, GLint components,
                                           GLint width, GLint height,
                                           GLenum format,
                                           GLenum type, const void *data );



/*
 *
 * Quadrics
 *
 */

GLUAPI GLUquadricObj* GLAPIENTRY gluNewQuadric( void );

GLUAPI void GLAPIENTRY gluDeleteQuadric( GLUquadricObj *state );

GLUAPI void GLAPIENTRY gluQuadricDrawStyle( GLUquadricObj *quadObject,
                                            GLenum drawStyle );

GLUAPI void GLAPIENTRY gluQuadricOrientation( GLUquadricObj *quadObject,
                                              GLenum orientation );

GLUAPI void GLAPIENTRY gluQuadricNormals( GLUquadricObj *quadObject,
                                          GLenum normals );

GLUAPI void GLAPIENTRY gluQuadricTexture( GLUquadricObj *quadObject,
                                          GLboolean textureCoords );

GLUAPI void GLAPIENTRY gluQuadricCallback( GLUquadricObj *qobj,
                                           GLenum which,
                                           void (GLCALLBACK *fn)() );

GLUAPI void GLAPIENTRY gluCylinder( GLUquadricObj *qobj,
                                    GLdouble baseRadius,
                                    GLdouble topRadius,
                                    GLdouble height,
                                    GLint slices, GLint stacks );

GLUAPI void GLAPIENTRY gluSphere( GLUquadricObj *qobj,
                                  GLdouble radius, GLint slices,
                                  GLint stacks );

GLUAPI void GLAPIENTRY gluDisk( GLUquadricObj *qobj,
                                GLdouble innerRadius, GLdouble outerRadius,
                                GLint slices, GLint loops );

GLUAPI void GLAPIENTRY gluPartialDisk( GLUquadricObj *qobj, GLdouble innerRadius,
                                       GLdouble outerRadius, GLint slices,
                                       GLint loops, GLdouble startAngle,
                                       GLdouble sweepAngle );



/*
 *
 * Nurbs
 *
 */

GLUAPI GLUnurbsObj* GLAPIENTRY gluNewNurbsRenderer( void );

GLUAPI void GLAPIENTRY gluDeleteNurbsRenderer( GLUnurbsObj *nobj );

GLUAPI void GLAPIENTRY gluLoadSamplingMatrices( GLUnurbsObj *nobj,
                                                const GLfloat modelMatrix[16],
                                                const GLfloat projMatrix[16],
                                                const GLint viewport[4] );

GLUAPI void GLAPIENTRY gluNurbsProperty( GLUnurbsObj *nobj, GLenum property,
                                         GLfloat value );

GLUAPI void GLAPIENTRY gluGetNurbsProperty( GLUnurbsObj *nobj, GLenum property,
                                            GLfloat *value );

GLUAPI void GLAPIENTRY gluBeginCurve( GLUnurbsObj *nobj );

GLUAPI void GLAPIENTRY gluEndCurve( GLUnurbsObj * nobj );

GLUAPI void GLAPIENTRY gluNurbsCurve( GLUnurbsObj *nobj, GLint nknots,
                                      GLfloat *knot, GLint stride,
                                      GLfloat *ctlarray, GLint order,
                                      GLenum type );

GLUAPI void GLAPIENTRY gluBeginSurface( GLUnurbsObj *nobj );

GLUAPI void GLAPIENTRY gluEndSurface( GLUnurbsObj * nobj );

GLUAPI void GLAPIENTRY gluNurbsSurface( GLUnurbsObj *nobj,
                                        GLint sknot_count, GLfloat *sknot,
                                        GLint tknot_count, GLfloat *tknot,
                                        GLint s_stride, GLint t_stride,
                                        GLfloat *ctlarray,
                                        GLint sorder, GLint torder,
                                        GLenum type );

GLUAPI void GLAPIENTRY gluBeginTrim( GLUnurbsObj *nobj );

GLUAPI void GLAPIENTRY gluEndTrim( GLUnurbsObj *nobj );

GLUAPI void GLAPIENTRY gluPwlCurve( GLUnurbsObj *nobj, GLint count,
                                    GLfloat *array, GLint stride,
                                    GLenum type );

GLUAPI void GLAPIENTRY gluNurbsCallback( GLUnurbsObj *nobj, GLenum which,
                                         void (GLCALLBACK *fn)() );



/*
 *
 * Polygon tessellation
 *
 */

GLUAPI GLUtesselator* GLAPIENTRY gluNewTess( void );

GLUAPI void GLAPIENTRY gluDeleteTess( GLUtesselator *tobj );

GLUAPI void GLAPIENTRY gluTessBeginPolygon( GLUtesselator *tobj,
					    void *polygon_data );

GLUAPI void GLAPIENTRY gluTessBeginContour( GLUtesselator *tobj );

GLUAPI void GLAPIENTRY gluTessVertex( GLUtesselator *tobj, GLdouble coords[3],
				      void *vertex_data );

GLUAPI void GLAPIENTRY gluTessEndContour( GLUtesselator *tobj );

GLUAPI void GLAPIENTRY gluTessEndPolygon( GLUtesselator *tobj );

GLUAPI void GLAPIENTRY gluTessProperty( GLUtesselator *tobj, GLenum which,
					GLdouble value );

GLUAPI void GLAPIENTRY gluTessNormal( GLUtesselator *tobj, GLdouble x,
				      GLdouble y, GLdouble z );

GLUAPI void GLAPIENTRY gluTessCallback( GLUtesselator *tobj, GLenum which,
					void (GLCALLBACK *fn)() );

GLUAPI void GLAPIENTRY gluGetTessProperty( GLUtesselator *tobj, GLenum which,
					   GLdouble *value );

/*
 *
 * Obsolete 1.0 tessellation functions
 *
 */

GLUAPI void GLAPIENTRY gluBeginPolygon( GLUtesselator *tobj );

GLUAPI void GLAPIENTRY gluNextContour( GLUtesselator *tobj, GLenum type );

GLUAPI void GLAPIENTRY gluEndPolygon( GLUtesselator *tobj );



/*
 *
 * New functions in GLU 1.1
 *
 */

GLUAPI const GLubyte* GLAPIENTRY gluGetString( GLenum name );



/*
 *
 * GLU 1.3 functions
 *
 */

GLUAPI GLboolean GLAPIENTRY
gluCheckExtension(const char *extName, const GLubyte *extString);


GLUAPI GLint GLAPIENTRY
gluBuild3DMipmaps( GLenum target, GLint internalFormat, GLsizei width,
                   GLsizei height, GLsizei depth, GLenum format,
                   GLenum type, const void *data );

GLUAPI GLint GLAPIENTRY
gluBuild1DMipmapLevels( GLenum target, GLint internalFormat, GLsizei width,
                        GLenum format, GLenum type, GLint level, GLint base,
                        GLint max, const void *data );

GLUAPI GLint GLAPIENTRY
gluBuild2DMipmapLevels( GLenum target, GLint internalFormat, GLsizei width,
                        GLsizei height, GLenum format, GLenum type,
                        GLint level, GLint base, GLint max,
                        const void *data );

GLUAPI GLint GLAPIENTRY
gluBuild3DMipmapLevels( GLenum target, GLint internalFormat, GLsizei width,
                        GLsizei height, GLsizei depth, GLenum format,
                        GLenum type, GLint level, GLint base, GLint max,
                        const void *data );

GLUAPI GLint GLAPIENTRY
gluUnProject4( GLdouble winx, GLdouble winy, GLdouble winz, GLdouble clipw,
               const GLdouble modelMatrix[16], const GLdouble projMatrix[16],
               const GLint viewport[4], GLclampd zNear, GLclampd zFar,
               GLdouble *objx, GLdouble *objy, GLdouble *objz,
               GLdouble *objw );



#if defined(__BEOS__) || defined(__QUICKDRAW__)
#pragma export off
#endif


#ifdef macintosh
	#pragma enumsalwaysint reset
	#if PRAGMA_IMPORT_SUPPORTED
	#pragma import off
	#endif
#endif


#ifdef __cplusplus
}
#endif


#endif /* __glu_h__ */
