#ifndef __glext_h_
#define __glext_h_


/*
 * XXX Many extensions need to be added yet.
 * XXX Some token values aren't known (grep for ?)
 * XXX This file may be automatically generated in the future.
 */


#ifdef __cplusplus
extern "C" {
#endif


#if defined(_WIN32) && !defined(__CYGWIN32__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef GLAPI
#define GLAPI extern
#endif

#if defined(GLAPIENTRY) && !defined(APIENTRY)
#define APIENTRY GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY
#endif


/*
 * Versions:
 *   1. Brian Paul, 24 Feb 2000
 *      Intial Version
 *   2. Brian Paul, 7 Mar 2000
 *      Added GL_HP_occlusion_test, GL_EXT_texture_lod_bias
 */
#define GL_GLEXT_VERSION_EXT 2


/*
 * 1. GL_EXT_abgr
 */
#ifndef GL_EXT_abgr
#define GL_EXT_abgr 1

#define GL_ABGR_EXT				0x8000

#endif /* GL_EXT_abgr */



/*
 * 2. GL_EXT_blend_color
 */
#ifndef GL_EXT_blend_color
#define GL_EXT_blend_color 1

#define GL_CONSTANT_COLOR_EXT			0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT		0x8002
#define GL_CONSTANT_ALPHA_EXT			0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT		0x8004
#define GL_BLEND_COLOR_EXT			0x8005

GLAPI void APIENTRY glBlendColorEXT(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

typedef void (APIENTRY * PFNGLBLENDCOLOREXTPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

#endif /* GL_EXT_blend_color */



/*
 * 3. GL_EXT_polygon_offset
 */
#ifndef GL_EXT_polygon_offset
#define GL_EXT_polygon_offset 1

#define GL_POLYGON_OFFSET_EXT			0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT		0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT		0x8039

GLAPI void APIENTRY glPolygonOffsetEXT(GLfloat factor, GLfloat bias);

typedef void (APIENTRY * PFNGLPOLYGONOFFSETEXTPROC) (GLfloat factor, GLfloat bias);

#endif /* GL_EXT_polygon_offset */



/*
 * 6. GL_EXT_texture3D
 */
#ifndef GL_EXT_texture3D
#define GL_EXT_texture3D 1

#define GL_PACK_SKIP_IMAGES_EXT			0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT		0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT		0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT		0x806E
#define GL_TEXTURE_3D_EXT			0x806F
#define GL_PROXY_TEXTURE_3D_EXT			0x8070
#define GL_TEXTURE_DEPTH_EXT			0x8071
#define GL_TEXTURE_WRAP_R_EXT			0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT		0x8073
#define GL_TEXTURE_3D_BINDING_EXT		0x806A

GLAPI void APIENTRY glTexImage3DEXT(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLAPI void APIENTRY glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
GLAPI void APIENTRY glCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (APIENTRY * PFNGLTEXIMAGE3DEXTPROC) (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE3DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLCOPYTEXSUBIMAGE3DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

#endif /* GL_EXT_texture3D */



/*
 * 7. GL_SGI_texture_filter4
 */
#ifndef GL_SGI_texture_filter4
#define GL_SGI_texture_filter4 1

#define GL_FILTER4_SGIS			?
#define GL_TEXTURE_FILTER4_SIZE_SGIS	?

GLAPI void APIENTRY glGetTexFilterFuncSGIS(GLenum target, GLenum filter, GLfloat *weights);
GLAPI void APIENTRY glTexFilterFuncSGIS(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);

typedef void (APIENTRY * PFNGLGETTEXFILTERFUNCSGISPROC) (GLenum target, GLenum filter, GLfloat *weights);
typedef void (APIENTRY * PFNGLTEXFILTERFUNCSGISPROC) (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);

#endif /* GL_SGI_texture_filter4 */



/*
 * 9. GL_EXT_subtexture
 */
#ifndef GL_EXT_subtexture
#define GL_EXT_subtexture 1

GLAPI void APIENTRY glCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GLAPI void APIENTRY glTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GLAPI void APIENTRY glTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

typedef void (APIENTRY * PFNGLCOPYTEXSUBIMAGE1DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE1DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE2DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

#endif /* GL_EXT_subtexture */



/*
 * 10. GL_EXT_copy_texture
 */
#ifndef GL_EXT_copy_texture
#define GL_EXT_copy_texture 1

GLAPI void APIENTRY glCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
GLAPI void APIENTRY glCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLAPI void APIENTRY glCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (APIENTRY * PFNGLCOPYTEXIMAGE1DEXTPROC) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
typedef void (APIENTRY * PFNGLCOPYTEXIMAGE2DEXTPROC) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
typedef void (APIENTRY * PFNGLCOPYTEXSUBIMAGE2DEXTPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);

#endif /* GL_EXT_copy_texture */



/*
 * 11. GL_EXT_histogram
 */
#ifndef GL_EXT_histogram
#define GL_EXT_histogram 1

#define GL_HISTOGRAM_EXT			0x8024
#define GL_PROXY_HISTOGRAM_EXT			0x8025
#define GL_HISTOGRAM_WIDTH_EXT			0x8026
#define GL_HISTOGRAM_FORMAT_EXT			0x8027
#define GL_HISTOGRAM_RED_SIZE_EXT		0x8028
#define GL_HISTOGRAM_GREEN_SIZE_EXT		0x8029
#define GL_HISTOGRAM_BLUE_SIZE_EXT		0x802A
#define GL_HISTOGRAM_ALPHA_SIZE_EXT		0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT		0x802C
#define GL_HISTOGRAM_SINK_EXT			0x802D

GLAPI void APIENTRY glGetHistogramEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
GLAPI void APIENTRY glGetHistogramParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetHistogramParameterivEXT(GLenum target, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetMinmaxEXT(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values);
GLAPI void APIENTRY glGetMinmaxParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetMinmaxParameterivEXT(GLenum target, GLenum pname, GLint *params);
GLAPI void APIENTRY glHistogramEXT(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
GLAPI void APIENTRY glMinmaxEXT(GLenum target, GLenum internalformat, GLboolean sink);
GLAPI void APIENTRY glResetHistogramEXT(GLenum target);
GLAPI void APIENTRY glResetMinmaxEXT(GLenum target);

typedef void (APIENTRY * PFNGLGETHISTOGRAMEXTPROC) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
typedef void (APIENTRY * PFNGLGETHISTOGRAMPARAMETERFVEXTPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETHISTOGRAMPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETMINMAXEXTPROC) (GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values);
typedef void (APIENTRY * PFNGLGETMINMAXPARAMETERFVEXTPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETMINMAXPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLHISTOGRAMEXTPROC) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
typedef void (APIENTRY * PFNGLMINMAXEXTPROC) (GLenum target, GLenum internalformat, GLboolean sink);
typedef void (APIENTRY * PFNGLRESETHISTOGRAMEXTPROC) (GLenum target);
typedef void (APIENTRY * PFNGLRESETMINMAXEXTPROC) (GLenum target);

#endif /* GL_EXT_histogram */



/*
 * 12. GL_EXT_convolution
 */
#ifndef GL_EXT_convolution
#define GL_EXT_convolution 1

#define GL_CONVOLUTION_1D_EXT			0x8010
#define GL_CONVOLUTION_2D_EXT			0x8011
#define GL_SEPARABLE_2D_EXT			0x8012
#define GL_CONVOLUTION_BORDER_MODE_EXT		0x8013
#define GL_CONVOLUTION_FILTER_SCALE_EXT		0x8014
#define GL_CONVOLUTION_FILTER_BIAS_EXT		0x8015
#define GL_REDUCE_EXT				0x8016
#define GL_CONVOLUTION_FORMAT_EXT		0x8017
#define GL_CONVOLUTION_WIDTH_EXT		0x8018
#define GL_CONVOLUTION_HEIGHT_EXT		0x8019
#define GL_MAX_CONVOLUTION_WIDTH_EXT		0x801A
#define GL_MAX_CONVOLUTION_HEIGHT_EXT		0x801B
#define GL_POST_CONVOLUTION_RED_SCALE_EXT	0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT	0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT	0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT	0x801F
#define GL_POST_CONVOLUTION_RED_BIAS_EXT	0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT	0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT	0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT	0x8023

GLAPI void APIENTRY glConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
GLAPI void APIENTRY glConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
GLAPI void APIENTRY glConvolutionParameterfEXT(GLenum target, GLenum pname, GLfloat params);
GLAPI void APIENTRY glConvolutionParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params);
GLAPI void APIENTRY glConvolutionParameteriEXT(GLenum target, GLenum pname, GLint params);
GLAPI void APIENTRY glConvolutionParameterivEXT(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glCopyConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
GLAPI void APIENTRY glCopyConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
GLAPI void APIENTRY glGetConvolutionFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *image);
GLAPI void APIENTRY glGetConvolutionParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetConvolutionParameterivEXT(GLenum target, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetSeparableFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
GLAPI void APIENTRY glSeparableFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);

typedef void (APIENTRY * PFNGLCONVOLUTIONFILTER1DEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
typedef void (APIENTRY * PFNGLCONVOLUTIONFILTER2DEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
typedef void (APIENTRY * PFNGLCONVOLUTIONPARAMETERFEXTPROC) (GLenum target, GLenum pname, GLfloat params);
typedef void (APIENTRY * PFNGLCONVOLUTIONPARAMETERFVEXTPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLCONVOLUTIONPARAMETERIEXTPROC) (GLenum target, GLenum pname, GLint params);
typedef void (APIENTRY * PFNGLCONVOLUTIONPARAMETERIVEXTPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLCOPYCONVOLUTIONFILTER1DEXTPROC) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
typedef void (APIENTRY * PFNGLCOPYCONVOLUTIONFILTER2DEXTPROC) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLGETCONVOLUTIONFILTEREXTPROC) (GLenum target, GLenum format, GLenum type, GLvoid *image);
typedef void (APIENTRY * PFNGLGETCONVOLUTIONPARAMETERFVEXTPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCONVOLUTIONPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETSEPARABLEFILTEREXTPROC) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
typedef void (APIENTRY * PFNGLSEPARABLEFILTER2DEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);

#endif /* GL_EXT_convolution */



/*
 * 13. GL_SGI_color_matrix
 */
#ifndef GL_SGI_color_matrix
#define GL_SGI_color_matrix 1

#define GL_COLOR_MATRIX_SGI			0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI		0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI	0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI	0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI	0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI	0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI	0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI	0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI	0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI	0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI	0x80BB

#endif /* GL_SGI_color_matrix */



/*
 * 14. GL_SGI_color_table
 */
#ifndef GL_SGI_color_table
#define GL_SGI_color_table 1

#define COLOR_TABLE_SGI				0x80D0
#define POST_CONVOLUTION_COLOR_TABLE_SGI	0x80D1
#define POST_COLOR_MATRIX_COLOR_TABLE_SGI	0x80D2
#define PROXY_COLOR_TABLE_SGI			0x80D3
#define PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI	0x80D4
#define PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI	0x80D5
#define COLOR_TABLE_SCALE_SGI			0x80D6
#define COLOR_TABLE_BIAS_SGI			0x80D7
#define COLOR_TABLE_FORMAT_SGI			0x80D8
#define COLOR_TABLE_WIDTH_SGI			0x80D9
#define COLOR_TABLE_RED_SIZE_SGI		0x80DA
#define COLOR_TABLE_GREEN_SIZE_SGI		0x80DB
#define COLOR_TABLE_BLUE_SIZE_SGI		0x80DC
#define COLOR_TABLE_ALPHA_SIZE_SGI		0x80DD
#define COLOR_TABLE_LUMINANCE_SIZE_SGI		0x80DE
#define COLOR_TABLE_INTENSITY_SIZE_SGI		0x80DF

GLAPI void APIENTRY glColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params);
GLAPI void APIENTRY glColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glColorTableSGI(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GLAPI void APIENTRY glCopyColorTableSGI(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width);
GLAPI void APIENTRY glGetColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params);
GLAPI void APIENTRY glGetColorTableSGI(GLenum target, GLenum format, GLenum type, GLvoid *table);

typedef void (APIENTRY * PFNGLCOLORTABLEPARAMETERFVSGIPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLCOLORTABLEPARAMETERIVSGIPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLCOLORTABLESGIPROC) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
typedef void (APIENTRY * PFNGLCOPYCOLORTABLESGIPROC) (GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERFVSGIPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERIVSGIPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETCOLORTABLESGIPROC) (GLenum target, GLenum format, GLenum type, GLvoid *table);

#endif /* GL_SGI_color_table */



/*
 * ?. GL_SGIX_pixel_texture
 */
#ifndef GL_SGIX_pixel_texture
#define GL_SGIX_pixel_texture 1

GLAPI void APIENTRY glPixelTexGenSGIX(GLenum mode);

typedef void (APIENTRY * PFNGLPIXELTEXGENSGIXPROC) (GLenum mode);

#endif /* GL_SGIX_pixel_texture */



/*
 * 15. GL_SGIS_pixel_texture
 */
#ifndef GL_SGIS_pixel_texture
#define GL_SGIS_pixel_texture 1

#define GL_PIXEL_TEXTURE_SGIS			?
#define GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS	?
#define GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS	?
#define GL_PIXEL_GROUP_COLOR_SGIS		?

GLAPI void APIENTRY glPixelTexGenParameterfSGIS(GLenum target, GLfloat value);
GLAPI void APIENTRY glPixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value);
GLAPI void APIENTRY glPixelTexGenParameteriSGIS(GLenum target, GLint value);
GLAPI void APIENTRY glPixelTexGenParameterivSGIS(GLenum target, const GLint *value);
GLAPI void APIENTRY glGetPixelTexGenParameterfvSGIS(GLenum target, GLfloat *value);
GLAPI void APIENTRY glGetPixelTexGenParameterivSGIS(GLenum target, GLint *value);

typedef void (APIENTRY * PFNGLPIXELTEXGENPARAMETERFSGISPROC) (GLenum target, GLfloat value);
typedef void (APIENTRY * PFNGLPIXELTEXGENPARAMETERFVSGISPROC) (GLenum target, const GLfloat *value);
typedef void (APIENTRY * PFNGLPIXELTEXGENPARAMETERISGISPROC) (GLenum target, GLint value);
typedef void (APIENTRY * PFNGLPIXELTEXGENPARAMETERIVSGISPROC) (GLenum target, const GLint *value);
typedef void (APIENTRY * PFNGLGETPIXELTEXGENPARAMETERFVSGISPROC) (GLenum target, GLfloat *value);
typedef void (APIENTRY * PFNGLGETPIXELTEXGENPARAMETERIVSGISPROC) (GLenum target, GLint *value);

#endif /* GL_SGIS_pixel_texture */



/*
 * 16. GL_SGIS_texture4D
 */
#ifndef GL_SGIS_texture4D
#define GL_SGIS_texture4D 1

#define GL_PACK_SKIP_VOLUMES_SGIS	?
#define GL_PACK_IMAGE_DEPTH_SGIS	?
#define GL_UNPACK_SKIP_VOLUMES_SGIS	?
#define GL_UNPACK_IMAGE_DEPTH_SGIS	?
#define GL_TEXTURE_4D_SGIS		?
#define GL_PROXY_TEXTURE_4D_SGIS	?
#define GL_TEXTURE_4DSIZE_SGIS		?
#define GL_TEXTURE_WRAP_Q_SGIS		?
#define GL_MAX_4D_TEXTURE_SIZE_SGIS	?
#define GL_TEXTURE_4D_BINDING_SGIS	?

GLAPI void APIENTRY glTexImage4DSGIS(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels);
GLAPI void APIENTRY glTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels);

typedef void (APIENTRY * PFNGLTEXIMAGE4DSGISPROC) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE4DSGISPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels);

#endif /* GL_SGIS_texture4D */



/*
 * 20. GL_EXT_texture_object
 */
#ifndef GL_EXT_texture_object
#define GL_EXT_texture_object 1

#define GL_TEXTURE_PRIORITY_EXT			0x8066
#define GL_TEXTURE_RESIDENT_EXT			0x8067
#define GL_TEXTURE_1D_BINDING_EXT		0x8068
#define GL_TEXTURE_2D_BINDING_EXT		0x8069

GLAPI void APIENTRY glGenTexturesEXT(GLsizei n, GLuint *textures);
GLAPI void APIENTRY glDeleteTexturesEXT(GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glBindTextureEXT(GLenum target, GLuint texture);
GLAPI void APIENTRY glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities);
GLAPI GLboolean APIENTRY glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences);
GLAPI GLboolean APIENTRY glIsTextureEXT(GLuint texture);

typedef void (APIENTRY * PFNGLGENTEXTURESEXTPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRY * PFNGLDELETETEXTURESEXTPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRY * PFNGLBINDTEXTUREEXTPROC) (GLenum target, GLuint texture);
typedef void (APIENTRY * PFNGLPRIORITIZETEXTURESEXTPROC) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef GLboolean (APIENTRY * PFNGLARETEXTURESRESIDENTEXTPROC) (GLsizei n, const GLuint *textures, GLboolean *residences);
typedef GLboolean (APIENTRY * PFNGLISTEXTUREEXTPROC) (GLuint texture);

#endif /* GL_EXT_texture_object */



/*
 * 21. GL_SGIS_detail_texture
 */
#ifndef GL_SGIS_detail_texture
#define GL_SGIS_detail_texture

#define GL_DETAIL_TEXTURE_2D_SGIS		0x8095
#define GL_DETAIL_TEXTURE_2D_BINDING_SGIS	0x8096
#define GL_LINEAR_DETAIL_SGIS			0x8097
#define GL_LINEAR_DETAIL_ALPHA_SGIS		0x8098
#define GL_LINEAR_DETAIL_COLOR_SGIS		0x8099
#define GL_DETAIL_TEXTURE_LEVEL_SGIS		0x809A
#define GL_DETAIL_TEXTURE_MODE_SGIS		0x809B

GLAPI void APIENTRY glDetailTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points);
GLAPI void APIENTRY glGetDetailTexFuncSGIS(GLenum target, GLfloat *points);

typedef void (APIENTRY * PFNGLDETAILTEXFUNCSGISPROC) (GLenum target, GLsizei n, const GLfloat *points);
typedef void (APIENTRY * PFNGLGETDETAILTEXFUNCSGISPROC) (GLenum target, GLfloat *points);

#endif /* GL_SGIS_detail_texture */



/*
 * 22. GL_SGIS_sharpen_texture
 */
#ifndef GL_SGIS_sharpen_texture
#define GL_SGIS_sharpen_texture 1

#define GL_LINEAR_SHARPEN_SGIS			0x80AD
#define GL_LINEAR_SHARPEN_ALPHA_SGIS		0x80AE
#define GL_LINEAR_SHARPEN_COLOR_SGIS		0x80AF
#define GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS	0x80B0

GLAPI void APIENTRY glGetSharpenTexFuncSGIS(GLenum target, GLfloat *points);
GLAPI void APIENTRY glSharpenTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points);

typedef void (APIENTRY * PFNGLGETSHARPENTEXFUNCSGISPROC) (GLenum target, GLfloat *points);
typedef void (APIENTRY * PFNGLSHARPENTEXFUNCSGISPROC) (GLenum target, GLsizei n, const GLfloat *points);

#endif /* GL_SGIS_sharpen_texture */



/*
 * 23. GL_EXT_packed_pixels
 */
#ifndef GL_EXT_packed_pixels
#define GL_EXT_packed_pixels 1

#define GL_UNSIGNED_BYTE_3_3_2_EXT		0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT		0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT		0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT		0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT		0x8036

#endif /* GL_EXT_packed_pixels */



/*
 * 25. GL_SGIS_multisample
 */

#ifndef GL_SGIS_multisample
#define GL_SGIS_multisample 1

#define GL_MULTISAMPLE_SGIS			0x809D
#define GL_SAMPLE_ALPHA_TO_MASK_SGIS		0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_SGIS		0x809F
#define GL_SAMPLE_MASK_SGIS			0x80A0
#define GL_MULTISAMPLE_BIT_EXT			0x20000000
#define GL_1PASS_SGIS				0x80A1
#define GL_2PASS_0_SGIS				0x80A2
#define GL_2PASS_1_SGIS				0x80A3
#define GL_4PASS_0_SGIS				0x80A4
#define GL_4PASS_1_SGIS				0x80A5
#define GL_4PASS_2_SGIS				0x80A6
#define GL_4PASS_3_SGIS				0x80A7
#define GL_SAMPLE_BUFFERS_SGIS			0x80A8
#define GL_SAMPLES_SGIS				0x80A9
#define GL_SAMPLE_MASK_VALUE_SGIS		0x80AA
#define GL_SAMPLE_MASK_INVERT_SGIS		0x80AB
#define GL_SAMPLE_PATTERN_SGIS			0x80AC

GLAPI void APIENTRY glSampleMaskSGIS(GLclampf value, GLboolean invert);
GLAPI void APIENTRY glSamplePatternSGIS(GLenum pattern);

typedef void (APIENTRY * PFNGLSAMPLEMASKSGISPROC) (GLclampf value, GLboolean invert);
typedef void (APIENTRY * PFNGLSAMPLEPATTERNSGISPROC) (GLenum pattern);

#endif /* GL_SGIS_multisample */



/*
 * 27. GL_EXT_rescale_normal
 */
#ifndef GL_EXT_rescale_normal
#define GL_EXT_rescale_normal 1

#define GL_RESCALE_NORMAL_EXT			0x803A

#endif /* GL_EXT_rescale_normal */



/*
 * 30. GL_EXT_vertex_array
 */
#ifndef GL_EXT_vertex_array
#define GL_EXT_vertex_array 1

#define GL_VERTEX_ARRAY_EXT			0x8074
#define GL_NORMAL_ARRAY_EXT			0x8075
#define GL_COLOR_ARRAY_EXT			0x8076
#define GL_INDEX_ARRAY_EXT			0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT		0x8078
#define GL_EDGE_FLAG_ARRAY_EXT			0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT		0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT		0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT		0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT		0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT		0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT		0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT		0x8080
#define GL_COLOR_ARRAY_SIZE_EXT			0x8081
#define GL_COLOR_ARRAY_TYPE_EXT			0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT		0x8083
#define GL_COLOR_ARRAY_COUNT_EXT		0x8084
#define GL_INDEX_ARRAY_TYPE_EXT			0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT		0x8086
#define GL_INDEX_ARRAY_COUNT_EXT		0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT		0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT		0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT	0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT	0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT		0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT		0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT		0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT		0x808F
#define GL_COLOR_ARRAY_POINTER_EXT		0x8090
#define GL_INDEX_ARRAY_POINTER_EXT		0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT	0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT		0x8093

GLAPI void APIENTRY glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
GLAPI void APIENTRY glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
GLAPI void APIENTRY glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
GLAPI void APIENTRY glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
GLAPI void APIENTRY glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *ptr);
GLAPI void APIENTRY glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr);
GLAPI void APIENTRY glGetPointervEXT(GLenum pname, void **params);
GLAPI void APIENTRY glArrayElementEXT(GLint i);
GLAPI void APIENTRY glDrawArraysEXT(GLenum mode, GLint first, GLsizei count);

typedef void (APIENTRY * PFNGLARRAYELEMENTEXTPROC) (GLint i);
typedef void (APIENTRY * PFNGLCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLDRAWARRAYSEXTPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLEDGEFLAGPOINTEREXTPROC) (GLsizei stride, GLsizei count, const GLboolean *pointer);
typedef void (APIENTRY * PFNGLGETPOINTERVEXTPROC) (GLenum pname, GLvoid* *params);
typedef void (APIENTRY * PFNGLINDEXPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLNORMALPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLTEXCOORDPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLVERTEXPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);

#endif /* GL_EXT_vertex_array */



/*
 * 35. GL_SGIS_texture_edge_clamp
 */
#ifndef GL_SGIS_texture_edge_clamp
#define GL_SGIS_texture_edge_clamp 1

#define GL_CLAMP_TO_EDGE_SGIS			0x812F

#endif /* GL_SGIS_texture_edge_clamp */



/*
 * 37. GL_EXT_blend_minmax
 */
#ifndef GL_EXT_blend_minmax
#define GL_EXT_blend_minmax 1

#define GL_FUNC_ADD_EXT				0x8006
#define GL_MIN_EXT				0x8007
#define GL_MAX_EXT				0x8008
#define GL_BLEND_EQUATION_EXT			0x8009

GLAPI void APIENTRY glBlendEquationEXT(GLenum mode);

typedef void (APIENTRY * PFNGLBLENDEQUATIONEXTPROC) (GLenum mode);

#endif /* GL_EXT_blend_minmax */



/*
 * 38. GL_EXT_blend_subtract (requires GL_EXT_blend_max)
 */
#ifndef GL_EXT_blend_subtract
#define GL_EXT_blend_subtract 1

#define GL_FUNC_SUBTRACT_EXT			0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT		0x800B

#endif /* GL_EXT_blend_subtract */



/*
 * 39. GL_EXT_blend_logic_op
 */
#ifndef GL_EXT_blend_logic_op
#define GL_EXT_blend_logic_op 1

/* No new tokens or functions */

#endif /* GL_EXT_blend_logic_op */



/*
 * 52. GL_SGIX_sprite
 */
#ifndef GL_SGIX_sprite
#define GL_SGIX_sprite 1

#define GL_SPRITE_SGIX				0x8148
#define GL_SPRITE_MODE_SGIX			0x8149
#define GL_SPRITE_AXIS_SGIX			0x814A
#define GL_SPRITE_TRANSLATION_SGIX		0x814B
#define GL_SPRITE_AXIAL_SGIX			0x814C
#define GL_SPRITE_OBJECT_ALIGNED_SGIX		0x814D
#define GL_SPRITE_EYE_ALIGNED_SGIX		0x814E

GLAPI void APIENTRY glSpriteParameterfSGIX(GLenum pname, GLfloat param);
GLAPI void APIENTRY glSpriteParameterfvSGIX(GLenum pname, const GLfloat *param);
GLAPI void APIENTRY glSpriteParameteriSGIX(GLenum pname, GLint param);
GLAPI void APIENTRY glSpriteParameterivSGIX(GLenum pname, const GLint *param);

typedef void (APIENTRY * PFNGLSPRITEPARAMETERFSGIXPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLSPRITEPARAMETERFVSGIXPROC) (GLenum pname, const GLfloat *param);
typedef void (APIENTRY * PFNGLSPRITEPARAMETERISGIXPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLSPRITEPARAMETERIVSGIXPROC) (GLenum pname, const GLint *param);

#endif /* GL_SGIX_sprite */



/*
 * 54. GL_EXT_point_parameters
 */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1

#define GL_POINT_SIZE_MIN_EXT			0x8126
#define GL_POINT_SIZE_MAX_EXT			0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT		0x8129

GLAPI void APIENTRY glPointParameterfEXT(GLenum pname, GLfloat param);
GLAPI void APIENTRY glPointParameterfvEXT(GLenum pname, const GLfloat *params);

typedef void (APIENTRY * PFNGLPOINTPARAMETERFEXTPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLPOINTPARAMETERFVEXTPROC) (GLenum pname, const GLfloat *params);

#endif /* GL_EXT_point_parameters */



/*
 * 55. GL_SGIX_instruments
 */
#ifndef GL_SGIX_instruments
#define GL_SGIX_instruments 1

#define GL_INSTRUMENT_BUFFER_POINTER_SGIX	0x8180
#define GL_INSTRUMENT_MEASUREMENTS_SGIX		0x8181

GLAPI GLint APIENTRY glGetInstrumentsSGIX(void);
GLAPI void APIENTRY glInstrumentsBufferSGIX(GLsizei size, GLint *buf);
GLAPI GLint APIENTRY glPollInstrumentsSGIX(GLint *markerp);
GLAPI void APIENTRY glReadInstrumentsSGIX(GLint marker);
GLAPI void APIENTRY glStartInstrumentsSGIX(void);
GLAPI void APIENTRY glStopInstrumentsSGIX(GLint marker);

typedef GLint (APIENTRY * PFNGLGETINSTRUMENTSSGIXPROC) (void);
typedef void (APIENTRY * PFNGLINSTRUMENTSBUFFERSGIXPROC) (GLsizei size, GLint *buf);
typedef GLint (APIENTRY * PFNGLPOLLINSTRUMENTSSGIXPROC) (GLint *markerp);
typedef void (APIENTRY * PFNGLREADINSTRUMENTSSGIXPROC) (GLint marker);
typedef void (APIENTRY * PFNGLSTARTINSTRUMENTSSGIXPROC) (void);
typedef void (APIENTRY * PFNGLSTOPINSTRUMENTSSGIXPROC) (GLint marker);

#endif


/*
 * 57. GL_SGIX_framezoom
 */
#ifndef GL_SGIX_framezoom
#define GL_SGIX_framezoom 1

#define GL_FRAMEZOOM_SGIX			0x818B
#define GL_FRAMEZOOM_FACTOR_SGIX		0x818C
#define GL_MAX_FRAMEZOOM_FACTOR_SGIX		0x818D

GLAPI void APIENTRY glFrameZoomSGIX(GLint factor);

typedef void (APIENTRY * PFNGLFRAMEZOOMSGIXPROC) (GLint factor);

#endif /* GL_SGIX_framezoom */



/*
 * 58. GL_SGIX_tag_sample_buffer
 */
#ifndef GL_SGIX_tag_sample_buffer
#define GL_SGIX_tag_sample_buffer 1

GLAPI void APIENTRY glTagSampleBufferSGIX(void);

typedef void (APIENTRY * PFNGLTAGSAMPLEBUFFERSGIXPROC) (void);

#endif /* GL_SGIX_tag_sample_buffer */



/*
 * 60. GL_SGIX_reference_plane
 */
#ifndef GL_SGIX_reference_plane
#define GL_SGIX_reference_plane 1

#define GL_REFERENCE_PLANE_SGIX			0x817D
#define GL_REFERENCE_PLANE_EQUATION_SGIX	0x817E

GLAPI void APIENTRY glReferencePlaneSGIX(const GLdouble *plane);

typedef void (APIENTRY * PFNGLREFERENCEPLANESGIXPROC) (const GLdouble *plane);

#endif /* GL_SGIX_reference_plane */



/*
 * 61. GL_SGIX_flush_raster
 */
#ifndef GL_SGIX_flush_raster
#define GL_SGIX_flush_raster 1

GLAPI void APIENTRY glFlushRasterSGIX(void);

typedef void (APIENTRY * PFNGLFLUSHRASTERSGIXPROC) (void);

#endif /* GL_SGIX_flush_raster */



/*
 * 74. GL_EXT_color_subtable
 */

#ifndef GL_EXT_color_subtable
#define GL_EXT_color_subtable 1

GLAPI void APIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
GLAPI void APIENTRY glCopyColorSubTableEXT(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
typedef void (APIENTRY * PFNGLCOPYCOLORSUBTABLEEXTPROC) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

#endif


/*
 * 77. GL_PGI_misc_hints
 */
#ifndef GL_PGI_misc_hints
#define GL_PGI_misc_hints 1

#define GL_PREFER_DOUBLEBUFFER_HINT_PGI		107000
#define GL_STRICT_DEPTHFUNC_HINT_PGI		107030
#define GL_STRICT_LIGHTING_HINT_PGI		107031
#define GL_STRICT_SCISSOR_HINT_PGI		107032
#define GL_FULL_STIPPLE_HINT_PGI		107033
#define GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI	107011
#define GL_NATIVE_GRAPHICS_END_HINT_PGI		107012
#define GL_CONSERVE_MEMORY_HINT_PGI		107005
#define GL_RECLAIM_MEMORY_HINT_PGI		107006
#define GL_ALWAYS_FAST_HINT_PGI			107020
#define GL_ALWAYS_SOFT_HINT_PGI			107021
#define GL_ALLOW_DRAW_OBJ_HINT_PGI		107022
#define GL_ALLOW_DRAW_WIN_HINT_PGI		107023
#define GL_ALLOW_DRAW_FRG_HINT_PGI		107024
#define GL_ALLOW_DRAW_SPN_HINT_PGI		107024
#define GL_ALLOW_DRAW_MEM_HINT_PGI		107025
#define GL_CLIP_NEAR_HINT_PGI			107040
#define GL_CLIP_FAR_HINT_PGI			107041
#define GL_WIDE_LINE_HINT_PGI		  	107042
#define GL_BACK_NORMALS_HINT_PGI		107043
#define GL_NATIVE_GRAPHICS_HANDLE_PGI		107010

GLAPI void APIENTRY glHintPGI(GLenum target, GLint mode);

typedef void (APIENTRY * PFNGLHINTPGIPROC) (GLenum target, GLint mode);

#endif /* GL_PGI_misc_hints */



/*
 * 78. GL_EXT_paletted_texture
 */
#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture 1

#define GL_TABLE_TOO_LARGE_EXT			0x8031
#define GL_COLOR_TABLE_FORMAT_EXT		0x80D8
#define GL_COLOR_TABLE_WIDTH_EXT		0x80D9
#define GL_COLOR_TABLE_RED_SIZE_EXT		0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_EXT		0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_EXT		0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_EXT	 	0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_EXT	0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_EXT	0x80DF
#define GL_TEXTURE_INDEX_SIZE_EXT		0x80ED
#define GL_COLOR_INDEX1_EXT			0x80E2
#define GL_COLOR_INDEX2_EXT			0x80E3
#define GL_COLOR_INDEX4_EXT			0x80E4
#define GL_COLOR_INDEX8_EXT			0x80E5
#define GL_COLOR_INDEX12_EXT			0x80E6
#define GL_COLOR_INDEX16_EXT			0x80E7

GLAPI void APIENTRY glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GLAPI void APIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
GLAPI void APIENTRY glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table);
GLAPI void APIENTRY glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params);

typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
typedef void (APIENTRY * PFNGLGETCOLORTABLEEXTPROC) (GLenum target, GLenum format, GLenum type, GLvoid *table);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERFVEXTPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);

#endif /* GL_EXT_paletted_texture */



/*
 * 79. GL_EXT_clip_volume_hint
 */
#ifndef GL_EXT_clip_volume_hint
#define GL_EXT_clip_volume_hint 1

#define GL_CLIP_VOLUME_CLIPPING_HINT_EXT	0x80F

#endif /* GL_EXT_clip_volume_hint */



/*
 * 80. GL_SGIX_list_priority
 */
#ifndef GL_SGIX_list_priority
#define GL_SGIX_list_priority 1

#define GL_LIST_PRIORITY_SGIX			0x8182

GLAPI void APIENTRY glGetListParameterfvSGIX(GLuint list, GLenum name, GLfloat *param);
GLAPI void APIENTRY glGetListParameterivSGIX(GLuint list, GLenum name, GLint *param);
GLAPI void APIENTRY glListParameterfSGIX(GLuint list, GLenum name, GLfloat param);
GLAPI void APIENTRY glListParameterfvSGIX(GLuint list, GLenum name, const GLfloat *param);
GLAPI void APIENTRY glListParameteriSGIX(GLuint list, GLenum name, GLint param);
GLAPI void APIENTRY glListParameterivSGIX(GLuint list, GLenum name, const GLint *param);

typedef void (APIENTRY * PFNGLGETLISTPARAMETERFVSGIXPROC) (GLuint list, GLenum name, GLfloat *param);
typedef void (APIENTRY * PFNGLGETLISTPARAMETERIVSGIXPROC) (GLuint list, GLenum name, GLint *param);
typedef void (APIENTRY * PFNGLLISTPARAMETERFSGIXPROC) (GLuint list, GLenum name, GLfloat param);
typedef void (APIENTRY * PFNGLLISTPARAMETERFVSGIXPROC) (GLuint list, GLenum name, const GLfloat *param);
typedef void (APIENTRY * PFNGLLISTPARAMETERISGIXPROC) (GLuint list, GLenum name, GLint param);
typedef void (APIENTRY * PFNGLLISTPARAMETERIVSGIXPROC) (GLuint list, GLenum name, const GLint *param);

#endif /* GL_SGIX_list_priority */



/*
 * 94. GL_EXT_index_material
 */
#ifndef GL_EXT_index_material
#define GL_EXT_index_material 1

#define GL_INDEX_MATERIAL_EXT			?
#define GL_INDEX_MATERIAL_PARAMETER_EXT		?
#define GL_INDEX_MATERIAL_FACE_EXT		?

GLAPI void APIENTRY glIndexMaterialEXT(GLenum face, GLenum mode);

typedef void (APIENTRY * PFNGLINDEXMATERIALEXTPROC) (GLenum face, GLenum mode);

#endif /* GL_EXT_index_material */



/*
 * 95. GL_EXT_index_func
 */
#ifndef GL_EXT_index_func
#define GL_EXT_index_func 1

#define GL_INDEX_TEST_EXT		?
#define GL_INDEX_TEST_FUNC_EXT		?
#define GL_INDEX_TEST_REF_EXT		?

GLAPI void APIENTRY glIndexFuncEXT(GLenum func, GLfloat ref);

typedef void (APIENTRY * PFNGLINDEXFUNCEXTPROC) (GLenum func, GLfloat ref);

#endif /* GL_EXT_index_func */



/*
 * 97. GL_EXT_compiled_vertex_array
 */
#ifndef GL_EXT_compiled_vertex_array
#define GL_EXT_compiled_vertex_array 1

#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT		0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT		0x81A9

GLAPI void APIENTRY glLockArraysEXT(GLint first, GLsizei count);
GLAPI void APIENTRY glUnlockArraysEXT(void);

typedef void (APIENTRY * PFNGLLOCKARRAYSEXTPROC) (GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLUNLOCKARRAYSEXTPROC) (void);

#endif /* GL_EXT_compiled_vertex_array */



/*
 * 98. GL_EXT_cull_vertex
 */
#ifndef GL_EXT_cull_vertex
#define GL_EXT_cull_vertex 1

#define GL_CULL_VERTEX_EXT			0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT		0x81AB
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT	0x81AC

GLAPI void APIENTRY glCullParameterdvEXT(GLenum pname, const GLdouble *params);
GLAPI void APIENTRY glCullParameterfvEXT(GLenum pname, const GLfloat *params);

typedef void (APIENTRY * PFNGLCULLPARAMETERDVEXTPROC) (GLenum pname, GLdouble* params);
typedef void (APIENTRY * PFNGLCULLPARAMETERFVEXTPROC) (GLenum pname, GLfloat* params);

#endif /* GL_EXT_cull_vertex */



/*
 * 102. GL_SGIX_fragment_lighting
 */
#ifndef GL_SGIX_fragment_lighting
#define GL_SGIX_fragment_lighting 1

GLAPI void APIENTRY glFragmentColorMaterialSGIX(GLenum face, GLenum mode);
GLAPI void APIENTRY glFragmentLightfSGIX(GLenum light, GLenum pname, GLfloat param);
GLAPI void APIENTRY glFragmentLightfvSGIX(GLenum light, GLenum pname, const GLfloat * params);
GLAPI void APIENTRY glFragmentLightiSGIX(GLenum light, GLenum pname, GLint param);
GLAPI void APIENTRY glFragmentLightivSGIX(GLenum light, GLenum pname, const GLint * params);
GLAPI void APIENTRY glFragmentLightModelfSGIX(GLenum pname, GLfloat param);
GLAPI void APIENTRY glFragmentLightModelfvSGIX(GLenum pname, const GLfloat * params);
GLAPI void APIENTRY glFragmentLightModeliSGIX(GLenum pname, GLint param);
GLAPI void APIENTRY glFragmentLightModelivSGIX(GLenum pname, const GLint * params);
GLAPI void APIENTRY glFragmentMaterialfSGIX(GLenum face, GLenum pname, GLfloat param);
GLAPI void APIENTRY glFragmentMaterialfvSGIX(GLenum face, GLenum pname, const GLfloat * params);
GLAPI void APIENTRY glFragmentMaterialiSGIX(GLenum face, GLenum pname, GLint param);
GLAPI void APIENTRY glFragmentMaterialivSGIX(GLenum face, GLenum pname, const GLint * params);
GLAPI void APIENTRY glGetFragmentLightfvSGIX(GLenum light, GLenum pname, GLfloat * params);
GLAPI void APIENTRY glGetFragmentLightivSGIX(GLenum light, GLenum pname, GLint * params);
GLAPI void APIENTRY glGetFragmentMaterialfvSGIX(GLenum face, GLenum pname, GLfloat * params);
GLAPI void APIENTRY glGetFragmentMaterialivSGIX(GLenum face, GLenum pname, GLint * params);
GLAPI void APIENTRY glLightEnviSGIX(GLenum pname, GLint param);

typedef void (APIENTRY * PFNGLFRAGMENTCOLORMATERIALSGIXPROC) (GLenum face, GLenum mode);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTFSGIXPROC) (GLenum light, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTFVSGIXPROC) (GLenum light, GLenum pname, const GLfloat * params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTISGIXPROC) (GLenum light, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTIVSGIXPROC) (GLenum light, GLenum pname, const GLint * params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELFSGIXPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELFVSGIXPROC) (GLenum pname, const GLfloat * params);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELISGIXPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTLIGHTMODELIVSGIXPROC) (GLenum pname, const GLint * params);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALFSGIXPROC) (GLenum face, GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALFVSGIXPROC) (GLenum face, GLenum pname, const GLfloat * params);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALISGIXPROC) (GLenum face, GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLFRAGMENTMATERIALIVSGIXPROC) (GLenum face, GLenum pname, const GLint * params);
typedef void (APIENTRY * PFNGLGETFRAGMENTLIGHTFVSGIXPROC) (GLenum light, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETFRAGMENTLIGHTIVSGIXPROC) (GLenum light, GLenum pname, GLint * params);
typedef void (APIENTRY * PFNGLGETFRAGMENTMATERIALFVSGIXPROC) (GLenum face, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETFRAGMENTMATERIALIVSGIXPROC) (GLenum face, GLenum pname, GLint * params);
typedef void (APIENTRY * PFNGLLIGHTENVISGIXPROC) (GLenum pname, GLint param);

#endif /* GL_SGIX_fragment_lighting */



/*
 * 129. GL_EXT_bgra
 */
#ifndef GL_EXT_bgra
#define GL_EXT_bgra 1

#define GL_BGR_EXT				0x80E0
#define GL_BGRA_EXT				0x80E1

#endif /* GL_EXT_bgra */



/*
 * 137. GL_HP_occlusion_test
 */
#ifndef GL_HP_occlusion_test
#define GL_HP_occlusion_test 1

#define GL_OCCLUSION_TEST_HP			0x8165
#define GL_OCCLUSION_TEST_RESULT_HP		0x8166

#endif /* GL_HP_occlusion_test */



/*
 * 141. GL_EXT_shared_texture_palette (req's GL_EXT_paletted_texture)
 */
#ifndef GL_EXT_shared_texture_palette
#define GL_EXT_shared_texture_palette 1

#define GL_SHARED_TEXTURE_PALETTE_EXT		0x81FB

#endif /* GL_EXT_shared_texture_palette */



/*
 * 149. GL_EXT_fog_coord
 */
#ifndef GL_EXT_fog_coord
#define GL_EXT_fog_coord 1

#define GL_FOG_COORDINATE_SOURCE_EXT		0x8450
#define GL_FOG_COORDINATE_EXT			0x8451
#define GL_FRAGMENT_DEPTH_EXT			0x8452
#define GL_CURRENT_FOG_COORDINATE_EXT		0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT	0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT	0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT	0x8456
#define GL_FOG_COORDINATE_ARRAY_EXT		0x8457

GLAPI void APIENTRY glFogCoordfEXT(GLfloat coord);
GLAPI void APIENTRY glFogCoordfvEXT(const GLfloat * coord);
GLAPI void APIENTRY glFogCoorddEXT(GLdouble coord);
GLAPI void APIENTRY glFogCoorddvEXT(const GLdouble * coord);
GLAPI void APIENTRY glFogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid * pointer);

typedef void (APIENTRY * PFNGLFOGCOORDFEXTPROC) (GLfloat coord);
typedef void (APIENTRY * PFNGLFOGCOORDFVEXTPROC) (const GLfloat * coord);
typedef void (APIENTRY * PFNGLFOGCOORDDEXTPROC) (GLdouble coord);
typedef void (APIENTRY * PFNGLFOGCOORDDVEXTPROC) (const GLdouble * coord);
typedef void (APIENTRY * PFNGLFOGCOORDPOINTEREXTPROC) (GLenum type, GLsizei stride, const GLvoid * pointer);

#endif



/*
 * 158. GL_EXT_texture_env_combine
 */
#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine 1

#define GL_COMBINE_EXT				0x8570
#define GL_COMBINE_RGB_EXT			0x8571
#define GL_COMBINE_ALPHA_EXT			0x8572
#define GL_RGB_SCALE_EXT			0x8573
#define GL_ADD_SIGNED_EXT			0x8574
#define GL_INTERPOLATE_EXT			0x8575
#define GL_CONSTANT_EXT				0x8576
#define GL_PRIMARY_COLOR_EXT			0x8577
#define GL_PREVIOUS_EXT				0x8578
#define GL_SOURCE0_RGB_EXT			0x8580
#define GL_SOURCE1_RGB_EXT			0x8581
#define GL_SOURCE2_RGB_EXT			0x8582
#define GL_SOURCE0_ALPHA_EXT			0x8588
#define GL_SOURCE1_ALPHA_EXT			0x8589
#define GL_SOURCE2_ALPHA_EXT			0x858A
#define GL_OPERAND0_RGB_EXT			0x8590
#define GL_OPERAND1_RGB_EXT			0x8591
#define GL_OPERAND2_RGB_EXT			0x8592
#define GL_OPERAND0_ALPHA_EXT			0x8598
#define GL_OPERAND1_ALPHA_EXT			0x8599
#define GL_OPERAND2_ALPHA_EXT			0x859A

#endif /* GL_EXT_texture_env_combine */



/*
 * 173. GL_EXT_blend_func_separate
 */
#ifndef GL_EXT_blend_func_separate
#define GL_EXT_blend_func_separate 1

#define GL_BLEND_DST_RGB_EXT			0x80C8
#define GL_BLEND_SRC_RGB_EXT			0x80C9
#define GL_BLEND_DST_ALPHA_EXT			0x80CA
#define GL_BLEND_SRC_ALPHA_EXT			0x80CB

GLAPI void APIENTRY glBlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

typedef void (APIENTRY * PFNGLBLENDFUNCSEPARATEEXTPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

#endif /* GL_EXT_blend_func_separate */



/*
 * 173. GL_INGR_blend_func_separate
 */
#ifndef GL_INGR_blend_func_separate
#define GL_INGR_blend_func_separate 1

#define GL_BLEND_DST_RGB_INGR			0x80C8
#define GL_BLEND_SRC_RGB_INGR			0x80C9
#define GL_BLEND_DST_ALPHA_INGR			0x80CA
#define GL_BLEND_SRC_ALPHA_INGR			0x80CB

GLAPI void APIENTRY glBlendFuncSeparateINGR(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

typedef void (APIENTRY * PFNGLBLENDFUNCSEPARATEINGRPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

#endif /* GL_INGR_blend_func_separate */



/*
 * 176. GL_EXT_stencil_wrap
 */
#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1

#define GL_INCR_WRAP_EXT			0x8507
#define GL_DECR_WRAP_EXT			0x8508

#endif /* GL_EXT_stencil_wrap */



/*
 * 179. GL_NV_texgen_reflection
 */
#ifndef GL_NV_texgen_reflection
#define GL_NV_texgen_reflection 1

#define GL_NORMAL_MAP_NV			0x8511
#define GL_REFLECTION_MAP_NV			0x8512

#endif /* GL_NV_texgen_reflection */



/*
 * 185. GL_EXT_texture_env_add
 */
#ifndef GL_EXT_texture_env_add
#define GL_EXT_texture_env_add 1

/* No new tokens or functions */

#endif /* GL_EXT_texture_env_add */



/*
 * 186. GL_EXT_texture_lod_bias
 */
#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias 1

#define GL_TEXTURE_FILTER_CONTROL_EXT		0x8500
#define GL_TEXTURE_LOD_BIAS_EXT			0x8501

#endif /* GL_EXT_texture_lod_bias */



/*
 * ??. GL_WIN_swap_hint
 */
#ifndef GL_WIN_swap_hint
#define GL_WIN_swap_hint 1

GLAPI void APIENTRY glAddSwapHintRectWIN(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (APIENTRY * PFNGLADDSWAPHINTRECTWINPROC) (GLint x, GLint y, GLsizei width, GLsizei height);

#endif /* GL_WIN_swap_hint */



/*
 * 188. GL_EXT_vertex_weighting
 */
#ifndef GL_EXT_vertex_weighting
#define GL_EXT_vertex_weighting 1

#define GL_VERTEX_WEIGHTING_EXT			0x8509
#define GL_MODELVIEW0_EXT			0x1700
#define GL_MODELVIEW1_EXT			0x850a
#define GL_CURRENT_VERTEX_WEIGHT_EXT		0x850b
#define GL_VERTEX_WEIGHT_ARRAY_EXT		0x850c
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT		0x850d
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT		0x850e
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT	0x850f
#define GL_MODELVIEW0_STACK_DEPTH_EXT		0x0BA3
#define GL_MODELVIEW1_STACK_DEPTH_EXT		0x8502
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT	0x8510

GLAPI void APIENTRY glVertexWeightfEXT(GLfloat weight);
GLAPI void APIENTRY glVertexWeightfvEXT(const GLfloat *weight);
GLAPI void APIENTRY glVertexWeightPointerEXT(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

typedef void (APIENTRY * PFNGLVERTEXWEIGHTFEXTPROC) (GLfloat weight);
typedef void (APIENTRY * PFNGLVERTEXWEIGHTFVEXTPROC) (const GLfloat *weight);
typedef void (APIENTRY * PFNGLVERTEXWEIGHTPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

#endif



/*
 * 189. GL_NV_light_max_exponent
 */
#ifndef GL_NV_light_max_exponent
#define GL_NV_light_max_exponent 1

#define GL_MAX_SHININESS_NV			0x8507
#define GL_MAX_SPOT_EXPONENT_NV			0x8508

#endif



/*
 * 190. GL_NV_vertex_array_range
 */
#ifndef GL_NV_vertex_array_range
#define GL_NV_vertex_array_range 1

/* TOKENS? */

GLAPI void APIENTRY glFlushVertexArrayRangeNV(void);
GLAPI void APIENTRY glVertexArrayRangeNV(GLsizei size, const GLvoid * pointer);

typedef void (APIENTRY * PFNGLFLUSHVERTEXARRAYRANGENVPROC) (void);

#endif



/*
 * 191. GL_NV_register_combiners
 */
#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners 1

/* TOKENS? */

#ifdef VMS
/*VMS only allows externals of maximal 31 characters! */
#define glGetCombinerOutputParameterfvNV glGetCombinerOutputParameterfvN
#define glGetCombinerOutputParameterivNV glGetCombinerOutputParameterivN
#define glGetFinalCombinerInputParameterfvNV glGetFnlCmbinerInpParameterfvNV
#define glGetFinalCombinerInputParameterivNV glGetFnlCmbinerInpParameterivNV
#endif

GLAPI void APIENTRY glCombinerParameterfvNV(GLenum pname, const GLfloat * params);
GLAPI void APIENTRY glCombinerParameterfNV(GLenum pname, GLfloat param);
GLAPI void APIENTRY glCombinerParameterivNV(GLenum pname, const GLint * params);
GLAPI void APIENTRY glCombinerParameteriNV(GLenum pname, GLint param);
GLAPI void APIENTRY glCombinerInputNV(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
GLAPI void APIENTRY glCombinerOutputNV(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
GLAPI void APIENTRY glFinalCombinerInputNV(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
GLAPI void APIENTRY glGetCombinerInputParameterfvNV(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params);
GLAPI void APIENTRY glGetCombinerInputParameterivNV(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params);
GLAPI void APIENTRY glGetCombinerOutputParameterfvNV(GLenum stage, GLenum portion, GLenum pname, GLfloat * params);
GLAPI void APIENTRY glGetCombinerOutputParameterivNV(GLenum stage, GLenum portion, GLenum pname, GLint * params);
GLAPI void APIENTRY glGetFinalCombinerInputParameterfvNV(GLenum variable, GLenum pname, GLfloat * params);
GLAPI void APIENTRY glGetFinalCombinerInputParameterivNV(GLenum variable, GLenum pname, GLint * params);

typedef void (APIENTRY * PFNGLVERTEXARRAYRANGENVPROC) (GLsizei size, const GLvoid * pointer);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFVNVPROC) (GLenum pname, const GLfloat * params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFNVPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERIVNVPROC) (GLenum pname, const GLint * params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERINVPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLCOMBINERINPUTNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLCOMBINEROUTPUTNVPROC) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
typedef void (APIENTRY * PFNGLFINALCOMBINERINPUTNVPROC) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLint * params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) (GLenum variable, GLenum pname, GLfloat * params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) (GLenum variable, GLenum pname, GLint * params);

#endif /* GL_NV_register_combiners */



/*
 * 192. GL_NV_fog_distance
 */
#ifndef GL_NV_fog_distance
#define GL_NV_fog_distance 1

#define GL_FOG_DISTANCE_MODE_NV			0x855a
#define GL_EYE_RADIAL_NV			0x855b
#define GL_EYE_PLANE_ABSOLUTE_NV		0x855c

#endif /* GL_NV_fog_distance*/



/*
 * 194. GL_NV_blend_square
 */
#ifndef GL_NV_blend_square
#define GL_NV_blend_square 1

/* no tokens or functions */

#endif /* GL_NV_blend_square */



/*
 * 195. GL_NV_texture_env_combine4
 */
#ifndef GL_NV_texture_env_combine4
#define GL_NV_texture_env_combine4 1

#define GL_COMBINE4_NV				0x8503
#define GL_SOURCE3_RGB_NV			0x8583
#define GL_SOURCE3_ALPHA_NV			0x858B
#define GL_OPERAND3_RGB_NV			0x8593
#define GL_OPERAND3_ALPHA_NV			0x859B

#endif /* GL_NV_texture_env_combine4 */



/*
 * 196. GL_MESA_resize_bufffers
 */
#ifndef GL_MESA_resize_buffers
#define GL_MESA_resize_buffers 1

GLAPI void APIENTRY glResizeBuffersMESA(void);

typedef void (APIENTRY * PFNGLRESIZEBUFFERSMESAPROC) (void);

#endif /* GL_MESA_resize_bufffers */



/*
 * 197. GL_MESA_window_pos
 */
#ifndef GL_MESA_window_pos
#define GL_MESA_window_pos 1

GLAPI void APIENTRY glWindowPos2iMESA(GLint x, GLint y);
GLAPI void APIENTRY glWindowPos2sMESA(GLshort x, GLshort y);
GLAPI void APIENTRY glWindowPos2fMESA(GLfloat x, GLfloat y);
GLAPI void APIENTRY glWindowPos2dMESA(GLdouble x, GLdouble y);
GLAPI void APIENTRY glWindowPos2ivMESA(const GLint *p);
GLAPI void APIENTRY glWindowPos2svMESA(const GLshort *p);
GLAPI void APIENTRY glWindowPos2fvMESA(const GLfloat *p);
GLAPI void APIENTRY glWindowPos2dvMESA(const GLdouble *p);
GLAPI void APIENTRY glWindowPos3iMESA(GLint x, GLint y, GLint z);
GLAPI void APIENTRY glWindowPos3sMESA(GLshort x, GLshort y, GLshort z);
GLAPI void APIENTRY glWindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glWindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glWindowPos3ivMESA(const GLint *p);
GLAPI void APIENTRY glWindowPos3svMESA(const GLshort *p);
GLAPI void APIENTRY glWindowPos3fvMESA(const GLfloat *p);
GLAPI void APIENTRY glWindowPos3dvMESA(const GLdouble *p);
GLAPI void APIENTRY glWindowPos4iMESA(GLint x, GLint y, GLint z, GLint w);
GLAPI void APIENTRY glWindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w);
GLAPI void APIENTRY glWindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLAPI void APIENTRY glWindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLAPI void APIENTRY glWindowPos4ivMESA(const GLint *p);
GLAPI void APIENTRY glWindowPos4svMESA(const GLshort *p);
GLAPI void APIENTRY glWindowPos4fvMESA(const GLfloat *p);
GLAPI void APIENTRY glWindowPos4dvMESA(const GLdouble *p);

typedef void (APIENTRY * PFNGLWINDOWPOS2IMESAPROC) (GLint x, GLint y);
typedef void (APIENTRY * PFNGLWINDOWPOS2SMESAPROC) (GLshort x, GLshort y);
typedef void (APIENTRY * PFNGLWINDOWPOS2FMESAPROC) (GLfloat x, GLfloat y);
typedef void (APIENTRY * PFNGLWINDOWPOS2DMESAPROC) (GLdouble x, GLdouble y);
typedef void (APIENTRY * PFNGLWINDOWPOS2IVMESAPROC) (const GLint *p);
typedef void (APIENTRY * PFNGLWINDOWPOS2SVMESAPROC) (const GLshort *p);
typedef void (APIENTRY * PFNGLWINDOWPOS2FVMESAPROC) (const GLfloat *p);
typedef void (APIENTRY * PFNGLWINDOWPOS2DVMESAPROC) (const GLdouble *p);
typedef void (APIENTRY * PFNGLWINDOWPOS3IMESAPROC) (GLint x, GLint y, GLint z);
typedef void (APIENTRY * PFNGLWINDOWPOS3SMESAPROC) (GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLWINDOWPOS3FMESAPROC) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLWINDOWPOS3DMESAPROC) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLWINDOWPOS3IVMESAPROC) (const GLint *p);
typedef void (APIENTRY * PFNGLWINDOWPOS3SVMESAPROC) (const GLshort *p);
typedef void (APIENTRY * PFNGLWINDOWPOS3FVMESAPROC) (const GLfloat *p);
typedef void (APIENTRY * PFNGLWINDOWPOS3DVMESAPROC) (const GLdouble *p);
typedef void (APIENTRY * PFNGLWINDOWPOS4SMESAPROC) (GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRY * PFNGLWINDOWPOS4FMESAPROC) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLWINDOWPOS4DMESAPROC) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLWINDOWPOS4IVMESAPROC) (const GLint *p);
typedef void (APIENTRY * PFNGLWINDOWPOS4SVMESAPROC) (const GLshort *p);
typedef void (APIENTRY * PFNGLWINDOWPOS4FVMESAPROC) (const GLfloat *p);
typedef void (APIENTRY * PFNGLWINDOWPOS4DVMESAPROC) (const GLdouble *p);

#endif /* GL_MESA_window_pos */



/*
 * ARB 0. GL_ARB_multitexture
 */
#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1

#define GL_TEXTURE0_ARB				0x84C0
#define GL_TEXTURE1_ARB				0x84C1
#define GL_TEXTURE2_ARB				0x84C2
#define GL_TEXTURE3_ARB				0x84C3
#define GL_TEXTURE4_ARB				0x84C4
#define GL_TEXTURE5_ARB				0x84C5
#define GL_TEXTURE6_ARB				0x84C6
#define GL_TEXTURE7_ARB				0x84C7
#define GL_TEXTURE8_ARB				0x84C8
#define GL_TEXTURE9_ARB				0x84C9
#define GL_TEXTURE10_ARB			0x84CA
#define GL_TEXTURE11_ARB			0x84CB
#define GL_TEXTURE12_ARB			0x84CC
#define GL_TEXTURE13_ARB			0x84CD
#define GL_TEXTURE14_ARB			0x84CE
#define GL_TEXTURE15_ARB			0x84CF
#define GL_TEXTURE16_ARB			0x84D0
#define GL_TEXTURE17_ARB			0x84D1
#define GL_TEXTURE18_ARB			0x84D2
#define GL_TEXTURE19_ARB			0x84D3
#define GL_TEXTURE20_ARB			0x84D4
#define GL_TEXTURE21_ARB			0x84D5
#define GL_TEXTURE22_ARB			0x84D6
#define GL_TEXTURE23_ARB			0x84D7
#define GL_TEXTURE24_ARB			0x84D8
#define GL_TEXTURE25_ARB			0x84D9
#define GL_TEXTURE26_ARB			0x84DA
#define GL_TEXTURE27_ARB			0x84DB
#define GL_TEXTURE28_ARB			0x84DC
#define GL_TEXTURE29_ARB			0x84DD
#define GL_TEXTURE30_ARB			0x84DE
#define GL_TEXTURE31_ARB			0x84DF
#define GL_ACTIVE_TEXTURE_ARB			0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB		0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB		0x84E2

GLAPI void APIENTRY glActiveTextureARB(GLenum texture);
GLAPI void APIENTRY glClientActiveTextureARB(GLenum texture);
GLAPI void APIENTRY glMultiTexCoord1dARB(GLenum target, GLdouble s);
GLAPI void APIENTRY glMultiTexCoord1dvARB(GLenum target, const GLdouble *v);
GLAPI void APIENTRY glMultiTexCoord1fARB(GLenum target, GLfloat s);
GLAPI void APIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat *v);
GLAPI void APIENTRY glMultiTexCoord1iARB(GLenum target, GLint s);
GLAPI void APIENTRY glMultiTexCoord1ivARB(GLenum target, const GLint *v);
GLAPI void APIENTRY glMultiTexCoord1sARB(GLenum target, GLshort s);
GLAPI void APIENTRY glMultiTexCoord1svARB(GLenum target, const GLshort *v);
GLAPI void APIENTRY glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t);
GLAPI void APIENTRY glMultiTexCoord2dvARB(GLenum target, const GLdouble *v);
GLAPI void APIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t);
GLAPI void APIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat *v);
GLAPI void APIENTRY glMultiTexCoord2iARB(GLenum target, GLint s, GLint t);
GLAPI void APIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v);
GLAPI void APIENTRY glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t);
GLAPI void APIENTRY glMultiTexCoord2svARB(GLenum target, const GLshort *v);
GLAPI void APIENTRY glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r);
GLAPI void APIENTRY glMultiTexCoord3dvARB(GLenum target, const GLdouble *v);
GLAPI void APIENTRY glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r);
GLAPI void APIENTRY glMultiTexCoord3fvARB(GLenum target, const GLfloat *v);
GLAPI void APIENTRY glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r);
GLAPI void APIENTRY glMultiTexCoord3ivARB(GLenum target, const GLint *v);
GLAPI void APIENTRY glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r);
GLAPI void APIENTRY glMultiTexCoord3svARB(GLenum target, const GLshort *v);
GLAPI void APIENTRY glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLAPI void APIENTRY glMultiTexCoord4dvARB(GLenum target, const GLdouble *v);
GLAPI void APIENTRY glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLAPI void APIENTRY glMultiTexCoord4fvARB(GLenum target, const GLfloat *v);
GLAPI void APIENTRY glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
GLAPI void APIENTRY glMultiTexCoord4ivARB(GLenum target, const GLint *v);
GLAPI void APIENTRY glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GLAPI void APIENTRY glMultiTexCoord4svARB(GLenum target, const GLshort *v);

typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DARBPROC) (GLenum target, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FARBPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IARBPROC) (GLenum target, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SARBPROC) (GLenum target, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DARBPROC) (GLenum target, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FARBPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IARBPROC) (GLenum target, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SARBPROC) (GLenum target, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IARBPROC) (GLenum target, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IARBPROC) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVARBPROC) (GLenum target, const GLshort *v);

#endif /* GL_ARB_multitexture */



/*
 * ARB 2. GL_ARB_tranpose_matrix
 */
#ifndef GL_ARB_transpose_matrix
#define GL_ARB_transpose_matrix 1

#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB		0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB		0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB			0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX_ARB			0x84E6

GLAPI void APIENTRY glLoadTransposeMatrixdARB( const GLdouble m[16] );
GLAPI void APIENTRY glLoadTransposeMatrixfARB( const GLfloat m[16] );
GLAPI void APIENTRY glMultTransposeMatrixdARB( const GLdouble m[16] );
GLAPI void APIENTRY glMultTransposeMatrixfARB( const GLfloat m[16] );

typedef void (APIENTRY * PFNGLLOADTRANSPOSEMATRIXDARBPROC) ( const GLdouble m[16] );
typedef void (APIENTRY * PFNGLLOADTRANSPOSEMATRIXFARBPROC) ( const GLfloat m[16] );
typedef void (APIENTRY * PFNGLMULTTRANSPOSEMATRIXDARBPROC) ( const GLdouble m[16] );
typedef void (APIENTRY * PFNGLMULTTRANSPOSEMATRIXFARBPROC) ( const GLfloat m[16] );

#endif /* GL_ARB_tranpose_matrix */



/*
 * ARB 4. GL_ARB_multisample
 */
#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1

GLAPI void APIENTRY glSamplePassARB(GLenum pass);
GLAPI void APIENTRY glSampleCoverageARB(GLclampf value, GLboolean invert);

typedef void (APIENTRY * PFNGLSAMPLEPASSARBPROC) (GLenum pass);
typedef void (APIENTRY * PFNGLSAMPLECOVERAGEARBPROC) (GLclampf value, GLboolean invert);

#endif /* GL_ARB_multisample */



#ifdef __cplusplus
}
#endif


#endif /* __glext_h_ */
