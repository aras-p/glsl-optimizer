#ifndef __glext_h_
#define __glext_h_


/*
 * XXX Some token values aren't known (grep for ?)
 * XXX This file may be automatically generated in the future.
 * XXX There are some doubly-defined tokens with different values!  Search
 *     for "DUPLICATE".
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
 *   3. Brian Paul, 20 Mar 2000
 *      Added all missing extensions up to number 137
 *   4. Brian Paul, 23 Mar 2000
 *      Now have all extenions up to number 197
 *   5. Brian Paul, 27 Mar 2000
 *      Added GL_ARB_texture_compression
 *   6. Brian Paul, 5 Apr 2000
 *      Added GL_ARB_multisample tokens, added GL_ARB_texture_env_add
 *   7. Brian Paul, 7 Apr 2000
 *      Minor clean-ups, temporary token values for GL_SGIS_pixel_texture
 *   8. Brian Paul, 15 Apr 2000
 *      Added GL_EXT_texture_cube_map, GL_NV_texgen_emboss, adding some
 *      missing tokens values.
 */
#define GL_GLEXT_VERSION_EXT 8


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
 * 4. GL_EXT_texture
 */
#ifndef GL_EXT_texture
#define GL_EXT_texture 1

#define GL_ALPHA4_EXT				0x803B
#define GL_ALPHA8_EXT				0x803C
#define GL_ALPHA12_EXT				0x803D
#define GL_ALPHA16_EXT				0x803E
#define GL_LUMINANCE4_EXT			0x803F
#define GL_LUMINANCE8_EXT			0x8040
#define GL_LUMINANCE12_EXT			0x8041
#define GL_LUMINANCE16_EXT			0x8042
#define GL_LUMINANCE4_ALPHA4_EXT		0x8043
#define GL_LUMINANCE6_ALPHA2_EXT		0x8044
#define GL_LUMINANCE8_ALPHA8_EXT		0x8045
#define GL_LUMINANCE12_ALPHA4_EXT		0x8046
#define GL_LUMINANCE12_ALPHA12_EXT		0x8047
#define GL_LUMINANCE16_ALPHA16_EXT		0x8048
#define GL_INTENSITY_EXT			0x8049
#define GL_INTENSITY4_EXT			0x804A
#define GL_INTENSITY8_EXT			0x804B
#define GL_INTENSITY12_EXT			0x804C
#define GL_INTENSITY16_EXT			0x804D
#define GL_RGB2_EXT				0x804E
#define GL_RGB4_EXT				0x804F
#define GL_RGB5_EXT				0x8050
#define GL_RGB8_EXT				0x8051
#define GL_RGB10_EXT				0x8052
#define GL_RGB12_EXT				0x8053
#define GL_RGB16_EXT				0x8054
#define GL_RGBA2_EXT				0x8055
#define GL_RGBA4_EXT				0x8056
#define GL_RGB5_A1_EXT				0x8057
#define GL_RGBA8_EXT				0x8058
#define GL_RGB10_A2_EXT				0x8059
#define GL_RGBA12_EXT				0x805A
#define GL_RGBA16_EXT				0x805B
#define GL_TEXTURE_RED_SIZE_EXT			0x805C
#define GL_TEXTURE_GREEN_SIZE_EXT		0x805D
#define GL_TEXTURE_BLUE_SIZE_EXT		0x805E
#define GL_TEXTURE_ALPHA_SIZE_EXT		0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_EXT		0x8060
#define GL_TEXTURE_INTENSITY_SIZE_EXT		0x8061
#define GL_REPLACE_EXT				0x8062
#define GL_PROXY_TEXTURE_1D_EXT			0x8063
#define GL_PROXY_TEXTURE_2D_EXT			0x8064

#endif /* GL_EXT_texture */



/*
 * 5. unknown
 */



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

#define GL_FILTER4_SGIS			0x8146
#define GL_TEXTURE_FILTER4_SIZE_SGIS	0x8147

GLAPI void APIENTRY glGetTexFilterFuncSGIS(GLenum target, GLenum filter, GLfloat *weights);
GLAPI void APIENTRY glTexFilterFuncSGIS(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);

typedef void (APIENTRY * PFNGLGETTEXFILTERFUNCSGISPROC) (GLenum target, GLenum filter, GLfloat *weights);
typedef void (APIENTRY * PFNGLTEXFILTERFUNCSGISPROC) (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);

#endif /* GL_SGI_texture_filter4 */



/*
 * 8. unknown
 */



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

#define GL_COLOR_TABLE_SGI				0x80D0 /* DUPLICATE! */
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI		0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI		0x80D2
#define GL_PROXY_COLOR_TABLE_SGI			0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI	0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI	0x80D5
#define GL_COLOR_TABLE_SCALE_SGI			0x80D6
#define GL_COLOR_TABLE_BIAS_SGI				0x80D7
#define GL_COLOR_TABLE_FORMAT_SGI			0x80D8
#define GL_COLOR_TABLE_WIDTH_SGI			0x80D9
#define GL_COLOR_TABLE_RED_SIZE_SGI			0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_SGI			0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_SGI			0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI			0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI		0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI		0x80DF

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

#define GL_PIXEL_TEX_GEN_SGIX               0x8139
#define GL_PIXEL_TEX_GEN_MODE_SGIX          0x832B

GLAPI void APIENTRY glPixelTexGenSGIX(GLenum mode);

typedef void (APIENTRY * PFNGLPIXELTEXGENSGIXPROC) (GLenum mode);

#endif /* GL_SGIX_pixel_texture */



/*
 * 15. GL_SGIS_pixel_texture
 */
#ifndef GL_SGIS_pixel_texture
#define GL_SGIS_pixel_texture 1

#define GL_PIXEL_TEXTURE_SGIS			0x1000  /*?*/
#define GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS	0x1001  /*?*/
#define GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS	0x1002  /*?*/
#define GL_PIXEL_GROUP_COLOR_SGIS		0x1003  /*?*/

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

#define GL_PACK_SKIP_VOLUMES_SGIS		0x8130
#define GL_PACK_IMAGE_DEPTH_SGIS		0x8131
#define GL_UNPACK_SKIP_VOLUMES_SGIS		0x8132
#define GL_UNPACK_IMAGE_DEPTH_SGIS		0x8133
#define GL_TEXTURE_4D_SGIS			0x8134
#define GL_PROXY_TEXTURE_4D_SGIS		0x8135
#define GL_TEXTURE_4DSIZE_SGIS			0x8136
#define GL_TEXTURE_WRAP_Q_SGIS			0x8137
#define GL_MAX_4D_TEXTURE_SIZE_SGIS		0x8138
#define GL_TEXTURE_4D_BINDING_SGIS		0x814F

GLAPI void APIENTRY glTexImage4DSGIS(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels);
GLAPI void APIENTRY glTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels);

typedef void (APIENTRY * PFNGLTEXIMAGE4DSGISPROC) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY * PFNGLTEXSUBIMAGE4DSGISPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels);

#endif /* GL_SGIS_texture4D */



/*
 * 17. GL_SGI_texture_color_table
 */
#ifndef GL_SGI_texture_color_table
#define GL_SGI_texture_color_table 1

#define GL_COLOR_TABLE_SGI_80BC			0x80BC /* DUPLICATE! */
#define GL_PROXY_TEXTURE_COLOR_TABLE_SGI	0x80BD

#endif /* GL_SGI_texture_color_table */



/*
 * 18. GL_EXT_cmyka
 */
#ifndef GL_EXT_cmyka
#define GL_EXT_cmyka 1

#define GL_CMYK_EXT				0x800C
#define GL_CMYKA_EXT				0x800D
#define GL_PACK_CMYK_HINT_EXT			0x800E
#define GL_UNPACK_CMYK_HINT_EXT			0x800F

#endif /* GL_EXT_cmyka */



/*
 * 19. unknown
 */



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
 * 24. GL_SGIS_texture_lod
 */

#ifndef GL_SGIS_texture_lod
#define GL_SGIS_texture_lod 1

#define GL_TEXTURE_MIN_LOD_SGIS			0x813A
#define GL_TEXTURE_MAX_LOD_SGIS			0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS		0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS		0x813D

#endif /* GL_SGIS_texture_lod */



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
 * 26. unknown
 */



/*
 * 27. GL_EXT_rescale_normal
 */
#ifndef GL_EXT_rescale_normal
#define GL_EXT_rescale_normal 1

#define GL_RESCALE_NORMAL_EXT			0x803A

#endif /* GL_EXT_rescale_normal */



/*
 * 28. GLX_EXT_visual_info
 */



/*
 * 29. unknown
 */



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
 * 31. GL_EXT_misc_attribute
 */
#ifndef GL_EXT_misc_attribute
#define GL_EXT_misc_attribute 1

#define GL_MISC_BIT_EXT				?

#endif /* GL_EXT_misc_attribute */



/*
 * 32. GL_SGIS_generate_mipmap
 */
#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap 1

#define GL_GENERATE_MIPMAP_SGIS			0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS		0x8192

#endif /* GL_SGIS_generate_mipmap */



/*
 * 33. GL_SGIX_clipmap
 */
#ifndef GL_SGIX_clipmap
#define GL_SGIX_clipmap 1

#define GL_LINEAR_CLIPMAP_LINEAR_SGIX		0x8170
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX		0x8171
#define GL_TEXTURE_CLIPMAP_FRAME_SGIX		0x8172
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX		0x8173
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX	0x8174
#define GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX	0x8175
#define GL_TEXTURE_CLIPMAP_DEPTH_SGIX		0x8176
#define GL_MAX_CLIPMAP_DEPTH_SGIX		0x8177
#define GL_MAX_CLIPMAP_VIRTUAL_DEPTH_SGIX	0x8178

#endif /* GL_SGIX_clipmap */



/*
 * 34. GL_SGIX_shadow
 */
#ifndef GL_SGIX_shadow
#define GL_SGIX_shadow 1

#define GL_TEXTURE_COMPARE_SGIX			0x819A
#define GL_TEXTURE_COMPARE_OPERATOR_SGIX	0x819B
#define GL_TEXTURE_LEQUAL_R_SGIX		0x819C
#define GL_TEXTURE_GEQUAL_R_SGIX		0x819D

#endif /* GL_SGIX_shadow */



/*
 * 35. GL_SGIS_texture_edge_clamp
 */
#ifndef GL_SGIS_texture_edge_clamp
#define GL_SGIS_texture_edge_clamp 1

#define GL_CLAMP_TO_EDGE_SGIS			0x812F

#endif /* GL_SGIS_texture_edge_clamp */



/*
 * 36. GL_SGIS_texture_border_clamp
 */
#ifndef GL_SGIS_texture_border_clamp
#define GL_SGIS_texture_border_clamp 1

#define GL_CLAMP_TO_BORDER_SGIS			0x812D

#endif /* GL_SGIS_texture_border_clamp */



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
 * 40. GLX_SGI_swap_control
 * 41. GLX_SGI_video_sync 
 * 42. GLX_SGI_make_current_read
 * 43. GLX_SGIX_video_source 
 * 44. GLX_EXT_visual_rating 
 */



/*
 * 45. GL_SGIX_interlace
 */
#ifndef GL_SGIX_interlace
#define GL_SGIX_interlace 1

#define GL_INTERLACE_SGIX			0x8094

#endif /* GL_SGIX_interlace */



/*
 * 46. unknown
 * 47. GLX_EXT_import_context 
 * 49. GLX_SGIX_fbconfig 
 * 50. GLX_SGIX_pbuffer
 */



/*
 * 51. GL_SGIS_texture_select
 */
#ifndef GL_SGIS_texture_select
#define GL_SGIS_texture_select 1

#define GL_DUAL_ALPHA4_SGIS			0x8110
#define GL_DUAL_ALPHA8_SGIS			0x8111
#define GL_DUAL_ALPHA12_SGIS			0x8112
#define GL_DUAL_ALPHA16_SGIS			0x8113
#define GL_DUAL_LUMINANCE4_SGIS			0x8114
#define GL_DUAL_LUMINANCE8_SGIS			0x8115
#define GL_DUAL_LUMINANCE12_SGIS		0x8116
#define GL_DUAL_LUMINANCE16_SGIS		0x8117
#define GL_DUAL_INTENSITY4_SGIS			0x8118
#define GL_DUAL_INTENSITY8_SGIS			0x8119
#define GL_DUAL_INTENSITY12_SGIS		0x811A
#define GL_DUAL_INTENSITY16_SGIS		0x811B
#define GL_DUAL_LUMINANCE_ALPHA4_SGIS  		0x811C
#define GL_DUAL_LUMINANCE_ALPHA8_SGIS   	0x811D
#define GL_DUAL_TEXTURE_SELECT_SGIS		0x8124
#define GL_QUAD_ALPHA4_SGIS			0x811E
#define GL_QUAD_ALPHA8_SGIS			0x811F
#define GL_QUAD_LUMINANCE4_SGIS0		x8120
#define GL_QUAD_LUMINANCE8_SGIS			0x8121
#define GL_QUAD_INTENSITY4_SGIS			0x8122
#define GL_QUAD_INTENSITY8_SGIS			0x8123
#define GL_QUAD_TEXTURE_SELECT_SGIS		0x8125

#endif /* GL_SGIS_texture_select */



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
 * 53. unknown
 */



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

#endif /* GL_SGIX_instruments */



/*
 * 56. GL_SGIX_texture_scale_bias
 */
#ifndef GL_SGIX_texture_scale_bias
#define GL_SGIX_texture_scale_bias 1

#define GL_POST_TEXTURE_FILTER_BIAS_SGIX	0x8179
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX	0x817A
#define GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX	0x817B
#define GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX	0x817C

#endif /* GL_SGIX_texture_scale_bias */



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
 * 59. unknown
 */



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
 * 62. GLX_SGI_cushion
 */



/*
 * 63. GL_SGIX_depth_texture
 */
#ifndef GL_SGIX_depth_texture
#define GL_SGIX_depth_texture 1

#define GL_DEPTH_COMPONENT16_SGIX 		0x81A5
#define GL_DEPTH_COMPONENT24_SGIX		0x81A6
#define GL_DEPTH_COMPONENT32_SGIX		0x81A7

#endif /* GL_SGIX_depth_texture */



/*
 * 64. GL_SGIS_fog_function
 */
#ifndef GL_SGIS_fog_function
#define GL_SGIS_fog_function 1

#define GL_FOG_FUNC_SGIS			0x812A
#define GL_FOG_FUNC_POINTS_SGIS			0x812B
#define GL_MAX_FOG_FUNC_POINTS_SGIS		0x812C

#endif /* GL_SGIS_fog_function */



/*
 * 65. GL_SGIX_fog_offset
 */
#ifndef GL_SGIX_fog_offset
#define GL_SGIX_fog_offset 1

#define GL_FOG_OFFSET_SGIX			0x8198
#define GL_FOG_OFFSET_VALUE_SGIX		0x8199

#endif /* GL_SGIX_fog_offset */



/*
 * 66. GL_HP_image_transform
 */
#ifndef GL_HP_image_transform
#define GL_HP_image_transform 1

#define GL_IMAGE_SCALE_X_HP				?
#define GL_IMAGE_SCALE_Y_HP				?
#define GL_IMAGE_TRANSLATE_X_HP				?
#define GL_IMAGE_TRANSLATE_Y_HP				?
#define GL_IMAGE_ROTATE_ANGLE_HP			?
#define GL_IMAGE_ROTATE_ORIGIN_X_HP			?
#define GL_IMAGE_ROTATE_ORIGIN_Y_HP			?
#define GL_IMAGE_MAG_FILTER_HP				?
#define GL_IMAGE_MIN_FILTER_HP				?
#define GL_IMAGE_CUBIC_WEIGHT_HP			?
#define GL_CUBIC_HP					?
#define GL_AVERAGE_HP					?
#define GL_IMAGE_TRANSFORM_2D_HP			?
#define GL_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP		?
#define GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP	?

GLAPI void APIENTRY glImageTransformParameteriHP(GLenum target, GLenum pname, const GLint param);
GLAPI void APIENTRY glImageTransformParameterfHP(GLenum target, GLenum pname, const GLfloat param);
GLAPI void APIENTRY glImageTransformParameterivHP(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glImageTransformParameterfvHP(GLenum target, GLenum pname, const GLfloat *params);
GLAPI void APIENTRY GetImageTransformParameterivHP(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glGetImageTransformParameterfvHP(GLenum target, GLenum pname,const GLfloat* params);

typedef void (APIENTRY * PFNGLIMAGETRANSFORMPARAMETERIHPPROC) (GLenum target, GLenum pname, const GLint param);
typedef void (APIENTRY * PFNGLIMAGETRANSFORMPARAMETERFHPPROC) (GLenum target, GLenum pname, const GLfloat param);
typedef void (APIENTRY * PFNGLIMAGETRANSFORMPARAMETERIVHPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLIMAGETRANSFORMPARAMETERFVHPPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGETIMAGETRANSFORMPARAMETERIVHPPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLGETIMAGETRANSFORMPARAMETERFVHPPROC) (GLenum target, GLenum pname,const GLfloat* params);

#endif /* GL_HP_image_transform */



/*
 * 67. GL_HP_convolution_border_modes 
 */
#ifndef GL_HP_convolution_border_modes
#define GL_HP_convolution_border_modes 1

#define GL_IGNORE_BORDER_HP			?
#define GL_CONSTANT_BORDER_HP			?
#define GL_REPLICATE_BORDER_HP			?
#define GL_CONVOLUTION_BORDER_COLOR_HP		?

#endif /* GL_HP_convolution_border_modes */



/*
 * 68. unknown
 */



/*
 * 69. GL_SGIX_texture_add_env
 */
#ifndef GL_SGIX_texture_add_env
#define GL_SGIX_texture_add_env 1

#define GL_TEXTURE_ENV_BIAS_SGIX		0x80BE

#endif /* GL_SGIX_texture_add_env */



/*
 * 70. unknown
 * 71. unknown
 * 72. unknown
 * 73. unknown
 */



/*
 * 74. GL_EXT_color_subtable
 */
#ifndef GL_EXT_color_subtable
#define GL_EXT_color_subtable 1

GLAPI void APIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
GLAPI void APIENTRY glCopyColorSubTableEXT(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
typedef void (APIENTRY * PFNGLCOPYCOLORSUBTABLEEXTPROC) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

#endif /* GL_EXT_color_subtable */



/*
 * 75. GLU_EXT_object_space_tess
 */



/*
 * 76. GL_PGI_vertex_hints
 */
#ifndef GL_PGI_vertex_hints
#define GL_PGI_vertex_hints 1

#define GL_VERTEX_DATA_HINT_PGI			107050
#define GL_VERTEX_CONSISTENT_HINT_PGI		107051
#define GL_VATERIAL_SIDE_HINT_PGI		107052
#define GL_VAX_VERTEX_HINT_PGI			107053
#define GL_VOLOR3_BIT_PGI			0x00010000
#define GL_VOLOR4_BIT_PGI			0x00020000
#define GL_VDGEFLAG_BIT_PGI			0x00040000
#define GL_VNDEX_BIT_PGI			0x00080000
#define GL_VAT_AMBIENT_BIT_PGI			0x00100000
#define GL_VAT_AMBIENT_AND_DIFFUSE_BIT_PGI	0x00200000
#define GL_VAT_DIFFUSE_BIT_PGI			0x00400000
#define GL_VAT_EMISSION_BIT_PGI			0x00800000
#define GL_VAT_COLOR_INDEXES_BIT_PGI		0x01000000
#define GL_VAT_SHININESS_BIT_PGI		0x02000000
#define GL_VAT_SPECULAR_BIT_PGI			0x04000000
#define GL_VORMAL_BIT_PGI			0x08000000
#define GL_VEXCOORD1_BIT_PGI			0x10000000
#define GL_VEXCOORD2_BIT_PGI			0x20000000
#define GL_VEXCOORD3_BIT_PGI			0x40000000
#define GL_VEXCOORD4_BIT_PGI			0x80000000
#define GL_VERTEX23_BIT_PGI			0x00000004
#define GL_VERTEX4_BIT_PGI			0x00000008

#endif /* GL_PGI_vertex_hints */



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
 * 81. GL_SGIX_ir_instrument1
 */
#ifndef GL_SGIX_ir_instrument1
#define GL_SGIX_ir_instrument1 1

#define GL_IR_INSTRUMENT1_SGIX			0x817F

#endif /* GL_SGIX_ir_instrument1 */



/*
 * 82. unknown
 * 83. GLX_SGIX_video_resize
 */



/*
 * 84. GL_SGIX_texture_lod_bias
 */
#ifndef GL_SGIX_texture_lod_bias
#define GL_SGIX_texture_lod_bias 1

#define GL_TEXTURE_LOD_BIAS_S_SGIX		0x818E
#define GL_TEXTURE_LOD_BIAS_T_SGIX		0x818F
#define GL_TEXTURE_LOD_BIAS_R_SGIX		0x8190

#endif /* GL_SGIX_texture_lod_bias */



/*
 * 85. GLU_SGI_filter4_parameters
 * 86. GLX_SGIX_dm_buffer
 * 87. unknown
 * 88. unknown
 * 89. unknown
 * 90. unknown
 * 91. GLX_SGIX_swap_group
 * 92. GLX_SGIX_swap_barrier
 */



/*
 *  93. GL_EXT_index_texture
 */
#ifndef GL_EXT_index_texture
#define GL_EXT_index_texture 1

/* No new tokens or functions */

#endif /* GL_EXT_index_texture */



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

#define GL_INDEX_TEST_EXT			?
#define GL_INDEX_TEST_FUNC_EXT			?
#define GL_INDEX_TEST_REF_EXT			?

GLAPI void APIENTRY glIndexFuncEXT(GLenum func, GLfloat ref);

typedef void (APIENTRY * PFNGLINDEXFUNCEXTPROC) (GLenum func, GLfloat ref);

#endif /* GL_EXT_index_func */



/*
 * 96. GL_EXT_index_array_formats
 */
#ifndef GL_EXT_index_array_formats
#define GL_EXT_index_array_formats 1

#define GL_IUI_V2F_EXT				?
#define GL_IUI_V3F_EXT				?
#define GL_IUI_N3F_V2F_EXT			?
#define GL_IUI_N3F_V3F_EXT			?
#define GL_T2F_IUI_V2F_EXT			?
#define GL_T2F_IUI_V3F_EXT			?
#define GL_T2F_IUI_N3F_V2F_EXT			?
#define GL_T2F_IUI_N3F_V3F_EXT			?

#endif /* GL_EXT_index_array_formats */



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
 * 99. unknown
 * 100. GLU_EXT_nurbs_tessellator
 */



/*
 * 101. GL_SGIX_ycrcb
 */
#ifndef GL_SGIX_ycrcb
#define GL_SGIX_ycrcb 1

#define GL_YCRCB_422_SGIX			0x81BB
#define GL_YCRCB_444_SGIX			0x81BC

#endif /* GL_SGIX_ycrcb */



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
 * 103. unknown
 * 104. unknown
 * 105. unknown
 * 106. unknown
 * 107. unknown
 * 108. unknown
 * 109. unknown
 */



/*
 * 110. GL_IBM_rasterpos_clip
 */
#ifndef GL_IBM_rasterpos_clip
#define GL_IBM_rasterpos_clip 1

#endif /* GL_IBM_rasterpos_clip */



/*
 * 111. GL_HP_texture_lighting
 */
#ifndef GL_HP_texture_lighting
#define GL_HP_texture_lighting 1

#define GL_TEXTURE_LIGHTING_MODE_HP		?
#define GL_TEXTURE_POST_SPECULAR_HP		?
#define GL_TEXTURE_PRE_SPECULAR_HP		?

#endif /* GL_HP_texture_lighting */



/*
 * 112. GL_EXT_draw_range_elements
 */
#ifndef GL_EXT_draw_range_elements
#define GL_EXT_draw_range_elements 1

#define GL_MAX_ELEMENTS_VERTICES_EXT		0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT		0x80E9

GLAPI void APIENTRY glDrawRangeElementsEXT(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

typedef void (APIENTRY * PFNGLDRAWRANGEELEMENTSEXTPROC) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

#endif /* GL_EXT_draw_range_elements */



/*
 * 113. GL_WIN_phong_shading
 */
#ifndef GL_WIN_phong_shading
#define GL_WIN_phong_shading 1

#define GL_PHONG_WIN				0x80EA
#define GL_PHONG_HINT_WIN			0x80EB

#endif /* GL_WIN_phong_shading */



/*
 * 114. GL_WIN_specular_fog
 */
#ifndef GL_WIN_specular_fog
#define GL_WIN_specular_fog 1

#define GL_FOG_SPECULAR_TEXTURE_WIN		0x80EC
#define GL_FOG_SPECULAR_TEXTURE_WIN		0x80EC

#endif /* GL_WIN_specular_fog */



/*
 * 115. unknown
 * 116. unknown
 */



/*
 * 117. GL_EXT_light_texture
 */
#ifndef GL_EXT_light_texture
#define GL_EXT_light_texture 1

#define GL_FRAGMENT_MATERIAL_EXT		0x8349
#define GL_FRAGMENT_NORMAL_EXT			0x834A

#define GL_FRAGMENT_DEPTH_EXT_834B		0x834B /* DUPLICATE! */
#define GL_FRAGMENT_COLOR_EXT			0x834C
#define GL_ATTENUATION_EXT			0x834D
#define GL_SHADOW_ATTENUATION_EXT	        0x834E
#define GL_TEXTURE_APPLICATION_MODE_EXT		0x834F
#define GL_TEXTURE_LIGHT_EXT			0x8350
#define GL_TEXTURE_MATERIAL_FACE_EXT		0x8351
#define GL_TEXTURE_MATERIAL_PARAMETER_EXT	0x8352

GLAPI void APIENTRY glApplyTextureEXT(GLenum mode);
GLAPI void APIENTRY glTextureLightEXT(GLenum pname);
GLAPI void APIENTRY glTextureMaterialEXT(GLenum face, GLenum mode);

typedef void (APIENTRY * PFNGLAPPLYTEXTUREEXTPROC) (GLenum mode);
typedef void (APIENTRY * PFNGLTEXTURELIGHTEXTPROC) (GLenum pname);
typedef void (APIENTRY * PFNGLTEXTUREMATERIALEXTPROC) (GLenum face, GLenum mode);

#endif /* GL_EXT_light_texture */



/*
 * 118. unknown
 */



/*
 * 119. GL_SGIX_blend_alpha_minmax
 */
#ifndef GL_SGIX_blend_alpha_minmax
#define GL_SGIX_blend_alpha_minmax 1

#endif /* GL_SGIX_blend_alpha_minmax */



/*
 * 120. GL_EXT_scene_marker
 */
#ifndef GL_EXT_scene_marker
#define GL_EXT_scene_marker 1

#endif /* GL_EXT_scene_marker */



/*
 * 121. unknown
 * 122. unknown
 * 123. unknown
 * 124. unknown
 * 125. unknown
 * 126. unknown
 * 127. unknown
 * 128. unknown
 */



/*
 * 129. GL_EXT_bgra
 */
#ifndef GL_EXT_bgra
#define GL_EXT_bgra 1

#define GL_BGR_EXT				0x80E0
#define GL_BGRA_EXT				0x80E1

#endif /* GL_EXT_bgra */



/*
 * 130. unknown
 * 131. unknown
 * 132. unknown
 * 133. unknown
 * 134. unknown
 */



/*
 * 135. GL_INTEL_texture_scissor
 */
#ifndef GL_INTEL_texture_scissor
#define GL_INTEL_texture_scissor 1

#define GL_TEXTURE_SCISSOR_INTEL			?
#define GL_TEXTURE_SCISSOR_S_INTEL			?
#define GL_TEXTURE_SCISSOR_T_INTEL			?
#define GL_TEXTURE_SCISSOR_R_INTEL			?

GLAPI void APIENTRY glTexScissorINTEL(GLenum target, GLclampf tlow, GLclampf thigh);
GLAPI void APIENTRY glTexScissorFuncINTEL(GLenum target, GLenum lfunc, GLenum hfunc);

typedef void (APIENTRY * PFNGLTEXSCISSORINTELPROC) (GLenum target, GLclampf tlow, GLclampf thigh);
typedef void (APIENTRY * PFNGLTEXSCISSORFUNCINTELPROC) (GLenum target, GLenum lfunc, GLenum hfunc);

#endif /* GL_INTEL_texture_scissor */



/*
 * 136. GL_INTEL_parallel_arrays
 */
#ifndef GL_INTEL_parallel_arrays
#define GL_INTEL_parallel_arrays 1

#define GL_PARALLEL_ARRAYS_INTEL			?
#define GL_VERTEX_ARRAY_PARALLEL_POINTERS_INTEL		?
#define GL_NORMAL_ARRAY_PARALLEL_POINTERS_INTEL		?
#define GL_COLOR_ARRAY_PARALLEL_POINTERS_INTEL		?
#define GL_TEXTURE_COORD_ARRAY_PARALLEL_POINTERS_INTEL	?

GLAPI void APIENTRY glVertexPointervINTEL(GLint size, GLenum type, const void ** pointer);
GLAPI void APIENTRY glNormalPointervINTEL(GLenum type, const void** pointer);
GLAPI void APIENTRY glColorPointervINTEL(GLint size, GLenum type, const void** pointer);
GLAPI void APIENTRY glTexCoordPointervINTEL(GLint size, GLenum type, const void** pointer);

typedef void (APIENTRY * PFNGLVERTEXPOINTERVINTELPROC) (GLint size, GLenum type, const void ** pointer);
typedef void (APIENTRY * PFNGLNORMALPOINTERVINTELPROC) (GLenum type, const void** pointer);
typedef void (APIENTRY * PFNGLCOLORPOINTERVINTELPROC) (GLint size, GLenum type, const void** pointer);
typedef void (APIENTRY * PFNGLTEXCOORDPOINTERVINTELPROC) (GLint size, GLenum type, const void** pointer);

#endif /* GL_INTEL_parallel_arrays */



/*
 * 137. GL_HP_occlusion_test
 */
#ifndef GL_HP_occlusion_test
#define GL_HP_occlusion_test 1

#define GL_OCCLUSION_TEST_HP			0x8165
#define GL_OCCLUSION_TEST_RESULT_HP		0x8166

#endif /* GL_HP_occlusion_test */



/*
 * 138. GL_EXT_pixel_transform
 */
#ifndef GL_EXT_pixel_transform
#define GL_EXT_pixel_transform 1

#define GL_PIXEL_MAG_FILTER_EXT				0x8331
#define GL_PIXEL_MIN_FILTER_EXT				0x8332
#define GL_PIXEL_CUBIC_WEIGHT_EXT			0x8333
#define GL_CUBIC_EXT					0x8334
#define GL_AVERAGE_EXT					0x8335
#define GL_PIXEL_TRANSFORM_2D_EXT			0x8330
#define GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT		0x8336
#define GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT	0x8337
#define GL_PIXEL_TRANSFORM_2D_MATRIX_EXT	        0x8338

GLAPI void APIENTRY glPixelTransformParameteriEXT(GLenum target, GLenum pname, const GLint param);
GLAPI void APIENTRY glPixelTransformParameterfEXT(GLenum target, GLenum pname, const GLfloat param);
GLAPI void APIENTRY glPixelTransformParameterivEXT(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glPixelTransformParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params);
GLAPI void APIENTRY glGetPixelTransformParameterivEXT(GLenum target, GLenum pname, const GLint *params);
GLAPI void APIENTRY glGetPixelTransformParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params);

#endif /* #define GL_EXT_pixel_transform */



/*
 * 139. GL_EXT_pixel_transform_color_table
 */
#ifndef GL_EXT_pixel_transform_color_table
#define GL_EXT_pixel_transform_color_table 1

#define GL_PIXEL_TRANSFORM_COLOR_TABLE_EXT		?
#define GL_PROXY_PIXEL_TRANSFORM_COLOR_TABLE_EXT	?

#endif /* GL_EXT_pixel_transform_color_table */



/*
 * 140. unknown
 */



/*
 * 141. GL_EXT_shared_texture_palette (req's GL_EXT_paletted_texture)
 */
#ifndef GL_EXT_shared_texture_palette
#define GL_EXT_shared_texture_palette 1

#define GL_SHARED_TEXTURE_PALETTE_EXT		0x81FB

#endif /* GL_EXT_shared_texture_palette */



/*
 * 142. unknown
 * 143. unknown
 */



/*
 * 144. GL_EXT_separate_specular_color
 */
#ifndef GL_EXT_separate_specular_color
#define GL_EXT_separate_specular_color 1

#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT	0x81F8
#define GL_SINGLE_COLOR_EXT			0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_EXT		0x81FA

#endif /* GL_EXT_separate_specular_color */



/*
 * 145. GL_EXT_secondary_color 
 */
#ifndef GL_EXT_secondary_color
#define GL_EXT_secondary_color 1

#define GL_COLOR_SUM_EXT			0x8458
#define GL_CURRENT_SECONDARY_COLOR_EXT		0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT	0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT	0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT	0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT	0x845D
#define GL_SECONDARY_COLOR_ARRAY_EXT		0x845E

GLAPI void APIENTRY glSecondaryColor3bEXT(GLbyte red, GLbyte green, GLbyte blue);
GLAPI void APIENTRY glSecondaryColor3dEXT(GLdouble red, GLdouble green, GLdouble blue);
GLAPI void APIENTRY glSecondaryColor3fEXT(GLfloat red, GLfloat green, GLfloat blue);
GLAPI void APIENTRY glSecondaryColor3iEXT(GLint red, GLint green, GLint blue);
GLAPI void APIENTRY glSecondaryColor3sEXT(GLshort red, GLshort green, GLshort blue);
GLAPI void APIENTRY glSecondaryColor3ubEXT(GLubyte red, GLubyte green, GLubyte blue);
GLAPI void APIENTRY glSecondaryColor3uiEXT(GLuint red, GLuint green, GLuint blue);
GLAPI void APIENTRY glSecondaryColor3usEXT(GLushort red, GLushort green, GLushort blue);
GLAPI void APIENTRY glSecondaryColor4bEXT(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
GLAPI void APIENTRY glSecondaryColor4dEXT(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GLAPI void APIENTRY glSecondaryColor4fEXT(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void APIENTRY glSecondaryColor4iEXT(GLint red, GLint green, GLint blue, GLint alpha);
GLAPI void APIENTRY glSecondaryColor4sEXT(GLshort red, GLshort green, GLshort blue, GLshort alpha);
GLAPI void APIENTRY glSecondaryColor4ubEXT(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GLAPI void APIENTRY glSecondaryColor4uiEXT(GLuint red, GLuint green, GLuint blue, GLuint alpha);
GLAPI void APIENTRY glSecondaryColor4usEXT(GLushort red, GLushort green, GLushort blue, GLushort alpha);
GLAPI void APIENTRY glSecondaryColor3bvEXT(const GLbyte *v);
GLAPI void APIENTRY glSecondaryColor3dvEXT(const GLdouble *v);
GLAPI void APIENTRY glSecondaryColor3fvEXT(const GLfloat *v);
GLAPI void APIENTRY glSecondaryColor3ivEXT(const GLint *v);
GLAPI void APIENTRY glSecondaryColor3svEXT(const GLshort *v);
GLAPI void APIENTRY glSecondaryColor3ubvEXT(const GLubyte *v);
GLAPI void APIENTRY glSecondaryColor3uivEXT(const GLuint *v);
GLAPI void APIENTRY glSecondaryColor3usvEXT(const GLushort *v);
GLAPI void APIENTRY glSecondaryColor4bvEXT(const GLbyte *v);
GLAPI void APIENTRY glSecondaryColor4dvEXT(const GLdouble *v);
GLAPI void APIENTRY glSecondaryColor4fvEXT(const GLfloat *v);
GLAPI void APIENTRY glSecondaryColor4ivEXT(const GLint *v);
GLAPI void APIENTRY glSecondaryColor4svEXT(const GLshort *v);
GLAPI void APIENTRY glSecondaryColor4ubvEXT(const GLubyte *v);
GLAPI void APIENTRY glSecondaryColor4uivEXT(const GLuint *v);
GLAPI void APIENTRY glSecondaryColor4usvEXT(const GLushort *v);
GLAPI void APIENTRY glSecondaryColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLvoid *pointer);

#endif /* GL_EXT_secondary_color */



/*
 * 146. GL_EXT_texture_env
 */
#ifndef GL_EXT_texture_env
#define GL_EXT_texture_env 1

#define GL_TEXTURE_ENV0_EXT		0x?
#define GL_TEXTURE_ENV1_EXT		(GL_TEXTURE_ENV0_EXT+1)
#define GL_TEXTURE_ENV2_EXT		(GL_TEXTURE_ENV0_EXT+2)
#define GL_TEXTURE_ENV3_EXT		(GL_TEXTURE_ENV0_EXT+3)
#define GL_TEXTURE_ENV4_EXT		(GL_TEXTURE_ENV0_EXT+4)
#define GL_TEXTURE_ENV5_EXT		(GL_TEXTURE_ENV0_EXT+5)
#define GL_TEXTURE_ENV6_EXT		(GL_TEXTURE_ENV0_EXT+6)
#define GL_TEXTURE_ENV7_EXT		(GL_TEXTURE_ENV0_EXT+7)
#define GL_TEXTURE_ENV8_EXT		(GL_TEXTURE_ENV0_EXT+8)
#define GL_TEXTURE_ENV9_EXT		(GL_TEXTURE_ENV0_EXT+9)
#define GL_TEXTURE_ENV10_EXT		(GL_TEXTURE_ENV0_EXT+10)
#define GL_TEXTURE_ENV11_EXT		(GL_TEXTURE_ENV0_EXT+11)
#define GL_TEXTURE_ENV12_EXT		(GL_TEXTURE_ENV0_EXT+12)
#define GL_TEXTURE_ENV13_EXT		(GL_TEXTURE_ENV0_EXT+13)
#define GL_TEXTURE_ENV14_EXT		(GL_TEXTURE_ENV0_EXT+14)
#define GL_TEXTURE_ENV15_EXT		(GL_TEXTURE_ENV0_EXT+15)
#define GL_TEXTURE_ENV_MODE_ALPHA_EXT	0x?
#define GL_ENV_COPY_EXT			0x?
#define GL_ENV_REPLACE_EXT		0x?
#define GL_ENV_MODULATE_EXT		0x?
#define GL_ENV_ADD_EXT			0x?
#define GL_ENV_SUBTRACT_EXT		0x?
#define GL_ENV_REVERSE_SUBTRACT_EXT	0x?
#define GL_ENV_BLEND_EXT		0x?
#define GL_ENV_REVERSE_BLEND_EXT	0x?
#define GL_TEXTURE_ENV_SHIFT_EXT	0x?

#endif /* GL_EXT_texture_env */



/*
 * 147. GL_EXT_texture_perturb_normal
 */
#ifndef GL_EXT_texture_perturb_normal
#define GL_EXT_texture_perturb_normal 1

#define GL_PERTURB_EXT			0x85AE
#define GL_TEXTURE_NORMAL_EXT		0x85AF

GLAPI void APIENTRY glTextureNormalEXT(GLenum mode);

typedef void (APIENTRY * PFNGLTEXTURENORMALEXT) (GLenum mode);

#endif /* GL_EXT_texture_perturb_normal */



/*
 * 148. GL_EXT_multi_draw_arrays
 */
#ifndef GL_EXT_multi_draw_arrays
#define GL_EXT_multi_draw_arrays 1

GLAPI void APIENTRY glMultiDrawArraysEXT(GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);

typedef void (APIENTRY * PFNGLMULTIDRAWARRAYSEXT) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);

#endif /* GL_EXT_multi_draw_arrays */



/*
 * 149. GL_EXT_fog_coord
 */
#ifndef GL_EXT_fog_coord
#define GL_EXT_fog_coord 1

#define GL_FOG_COORDINATE_SOURCE_EXT		0x8450
#define GL_FOG_COORDINATE_EXT			0x8451
#define GL_FRAGMENT_DEPTH_EXT			0x8452 /* DUPLICATE! */
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
 * 150. unknown
 * 151. unknown
 * 152. unknown
 * 153. unknown
 * 154. unknown
 */



/*
 * 155. GL_REND_screen_coordinates
 */
#ifndef GL_REND_screen_coordinates
#define GL_REND_screen_coordinates 1

#define GL_SCREEN_COORDINATES_REND		0x8490
#define GL_INVERTED_SCREEN_W_REND		0x8491

#endif /* GL_REND_screen_coordinates */



/*
 * 156. GL_EXT_coordinate_frame 
 */
#ifndef GL_EXT_coordinate_frame
#define GL_EXT_coordinate_frame 1

#define GL_TANGENT_ARRAY_EXT			0x8439
#define GL_BINORMAL_ARRAY_EXT			0x843A
#define GL_CURRENT_TANGENT_EXT			0x843B
#define GL_CURRENT_BINORMAL_EXT			0x843C
#define GL_TANGENT_ARRAY_TYPE_EXT		0x843E
#define GL_TANGENT_ARRAY_STRIDE_EXT		0x843F
#define GL_BINORMAL_ARRAY_TYPE_EXT		0x8440
#define GL_BINORMAL_ARRAY_STRIDE_EXT		0x8441
#define GL_TANGENT_ARRAY_POINTER_EXT		0x8442
#define GL_BINORMAL_ARRAY_POINTER_EXT		0x8443
#define GL_MAP1_TANGENT_EXT			0x8444
#define GL_MAP2_TANGENT_EXT			0x8445
#define GL_MAP1_BINORMAL_EXT			0x8446
#define GL_MAP2_BINORMAL_EXT			0x8447

GLAPI void APIENTRY glTangent3bEXT(GLbyte x, GLbyte y, GLbyte z);
GLAPI void APIENTRY glTangent3dEXT(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glTangent3fEXT(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTangent3iEXT(GLint x, GLint y, GLint z);
GLAPI void APIENTRY glTangent3sEXT(GLshort x, GLshort y, GLshort z);
GLAPI void APIENTRY glTangent3bvEXT(const GLbyte *v);
GLAPI void APIENTRY glTangent3dvEXT(const GLdouble *v);
GLAPI void APIENTRY glTangent3fvEXT(const GLfloat *v);
GLAPI void APIENTRY glTangent3ivEXT(const GLint *v);
GLAPI void APIENTRY glTangent3svEXT(const GLshort *v);
GLAPI void APIENTRY glBinormal3bEXT(GLbyte x, GLbyte y, GLbyte z);
GLAPI void APIENTRY glBinormal3dEXT(GLdouble x, GLdouble y, GLdouble z);
GLAPI void APIENTRY glBinormal3fEXT(GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glBinormal3iEXT(GLint x, GLint y, GLint z);
GLAPI void APIENTRY glBinormal3sEXT(GLshort x, GLshort y, GLshort z);
GLAPI void APIENTRY glBinormal3bvEXT(const GLbyte *v);
GLAPI void APIENTRY glBinormal3dvEXT(const GLdouble *v);
GLAPI void APIENTRY glBinormal3fvEXT(const GLfloat *v);
GLAPI void APIENTRY glBinormal3ivEXT(const GLint *v);
GLAPI void APIENTRY glBinormal3svEXT(const GLshort *v);
GLAPI void APIENTRY glTangentPointerEXT(GLenum type, GLsizei stride, GLvoid *pointer);
GLAPI void APIENTRY glBinormalPointerEXT(GLenum type, GLsizei stride, GLvoid *pointer);

typedef void (APIENTRY * PFNGLTANGENT3BEXT) (GLbyte x, GLbyte y, GLbyte z);
typedef void (APIENTRY * PFNGLTANGENT3DEXT) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLTANGENT3FEXT) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTANGENT3IEXT) (GLint x, GLint y, GLint z);
typedef void (APIENTRY * PFNGLTANGENT3SEXT) (GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLTANGENT3BVEXT) (const GLbyte *v);
typedef void (APIENTRY * PFNGLTANGENT3DVEXT) (const GLdouble *v);
typedef void (APIENTRY * PFNGLTANGENT3FVEXT) (const GLfloat *v);
typedef void (APIENTRY * PFNGLTANGENT3IVEXT) (const GLint *v);
typedef void (APIENTRY * PFNGLTANGENT3SVEXT) (const GLshort *v);
typedef void (APIENTRY * PFNGLBINORMAL3BEXT) (GLbyte x, GLbyte y, GLbyte z);
typedef void (APIENTRY * PFNGLBINORMAL3DEXT) (GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRY * PFNGLBINORMAL3FEXT) (GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLBINORMAL3IEXT) (GLint x, GLint y, GLint z);
typedef void (APIENTRY * PFNGLBINORMAL3SEXT) (GLshort x, GLshort y, GLshort z);
typedef void (APIENTRY * PFNGLBINORMAL3BVEXT) (const GLbyte *v);
typedef void (APIENTRY * PFNGLBINORMAL3DVEXT) (const GLdouble *v);
typedef void (APIENTRY * PFNGLBINORMAL3FVEXT) (const GLfloat *v);
typedef void (APIENTRY * PFNGLBINORMAL3IVEXT) (const GLint *v);
typedef void (APIENTRY * PFNGLBINORMAL3SVEXT) (const GLshort *v);
typedef void (APIENTRY * PFNGLTANGENTPOINTEREXT) (GLenum type, GLsizei stride, GLvoid *pointer);
typedef void (APIENTRY * PFNGLBINORMALPOINTEREXT) (GLenum type, GLsizei stride, GLvoid *pointer);

#endif /* GL_EXT_coordinate_frame */



/*
 * 157. unknown
 */



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
 * 159. GL_APPLE_specular_vector 
 */
#ifndef GL_APPLE_specular_vector
#define GL_APPLE_specular_vector 1

#define GL_LIGHT_MODEL_SPECULAR_VECTOR_APPLE	0x85B0

#endif /* GL_APPLE_specular_vector */



/*
 * 160. GL_APPLE_transform_hint
 */
#ifndef GL_APPLE_transform_hint
#define GL_APPLE_transform_hint 1

#define GL_TRANSFORM_HINT_APPLE			0x85B1

#endif /* GL_APPLE_transform_hint */



/*
 * 161. unknown
 * 162. unknown
 */



/*
 * 163. GL_SUNX_constant_data
 */
#ifndef GL_SUNX_constant_data
#define GL_SUNX_constant_data 1

#define GL_UNPACK_CONSTANT_DATA_SUNX		0x81D5
#define GL_TEXTURE_CONSTANT_DATA_SUNX		0x81D6

GLAPI void APIENTRY glFinishTextureSUNX(void);

typedef void (APIENTRY * PFNGLFINISHTEXTURESUNX) (void);

#endif /* GL_SUNX_constant_data */



/*
 * 164. GL_SUN_global_alpha
 */
#ifndef GL_SUN_global_alpha
#define GL_SUN_global_alpha 1

#define GL_GLOBAL_ALPHA_SUN			0x81D9
#define GL_GLOBAL_ALPHA_FACTOR_SUN		0x81DA

GLAPI void APIENTRY glGlobalAlphaFactorbSUN(GLbyte factor);
GLAPI void APIENTRY glGlobalAlphaFactorsSUN(GLshort factor);
GLAPI void APIENTRY glGlobalAlphaFactoriSUN(GLint factor);
GLAPI void APIENTRY glGlobalAlphaFactorfSUN(GLfloat factor);
GLAPI void APIENTRY glGlobalAlphaFactordSUN(GLdouble factor);
GLAPI void APIENTRY glGlobalAlphaFactorubSUN(GLubyte factor);
GLAPI void APIENTRY glGlobalAlphaFactorusSUN(GLushort factor);
GLAPI void APIENTRY glGlobalAlphaFactoruiSUN(GLuint factor);

typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORBSUN) (GLbyte factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORSSUN) (GLshort factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORISUN) (GLint factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORFSUN) (GLfloat factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORDSUN) (GLdouble factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORUBSUN) (GLubyte factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORUSSUN) (GLushort factor);
typedef void (APIENTRY * PFNGLGLOBALALPHAFACTORUISUN) (GLuint factor);

#endif /* GL_SUN_global_alpha */



/*
 * 165. GL_SUN_triangle_list
 */
#ifndef GL_SUN_triangle_list
#define GL_SUN_triangle_list 1

#define GL_TRIANGLE_LIST_SUN                       0x81D7
#define GL_REPLACEMENT_CODE_SUN                    0x81D8
#define GL_RESTART_SUN                             0x01
#define GL_REPLACE_MIDDLE_SUN                      0x02
#define GL_REPLACE_OLDEST_SUN                      0x03
#define GL_REPLACEMENT_CODE_ARRAY_SUN              0x85C0
#define GL_REPLACEMENT_CODE_ARRAY_TYPE_SUN         0x85C1
#define GL_REPLACEMENT_CODE_ARRAY_STRIDE_SUN       0x85C2
#define GL_REPLACEMENT_CODE_ARRAY_POINTER_SUN      0x85C3
#define GL_R1UI_V3F_SUN                            0x85C4
#define GL_R1UI_C4UB_V3F_SUN                       0x85C5
#define GL_R1UI_C3F_V3F_SUN                        0x85C6
#define GL_R1UI_N3F_V3F_SUN                        0x85C7
#define GL_R1UI_C4F_N3F_V3F_SUN                    0x85C8
#define GL_R1UI_T2F_V3F_SUN                        0x85C9
#define GL_R1UI_T2F_N3F_V3F_SUN                    0x85CA
#define GL_R1UI_T2F_C4F_N3F_V3F_SUN                0x85CB

GLAPI void APIENTRY glReplacementCodeuiSUN(GLuint code);
GLAPI void APIENTRY glReplacementCodeusSUN(GLushort code);
GLAPI void APIENTRY glReplacementCodeubSUN(GLubyte code);
GLAPI void APIENTRY glReplacementCodeuivSUN(const GLuint *code);
GLAPI void APIENTRY glReplacementCodeusvSUN(const GLushort *code);
GLAPI void APIENTRY glReplacementCodeubvSUN(const GLubyte *code);
GLAPI void APIENTRY glReplacementCodePointerSUN(GLenum type, GLsizei stride, const GLvoid *pointer);

typedef void (APIENTRY * PFNGLREPLACEMENTCODEUISUN) (GLuint code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUSSUN) (GLushort code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUBSUN) (GLubyte code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUIVSUN) (const GLuint *code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUSVSUN) (const GLushort *code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUBVSUN) (const GLubyte *code);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEPOINTERSUN) (GLenum type, GLsizei stride, const GLvoid *pointer);

#endif /* GL_SUN_triangle_list */



/*
 * 166. GL_SUN_vertex
 */
#ifndef GL_SUN_vertex
#define GL_SUN_vertex 1

GLAPI void APIENTRY glColor4ubVertex2fSUN(GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y);
GLAPI void APIENTRY glColor4ubVertex2fvSUN(const GLubyte *c, const GLfloat *v);
GLAPI void APIENTRY glColor4ubVertex3fSUN(GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glColor4ubVertex3fvSUN(const GLubyte *c, const GLfloat *v);
GLAPI void APIENTRY glColor3fVertex3fSUN(GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glColor3fVertex3fvSUN(const GLfloat *c, const GLfloat *v);
GLAPI void APIENTRY glNormal3fVertex3fSUN(GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glNormal3fVertex3fvSUN(const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glColor4fNormal3fVertex3fSUN(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glColor4fNormal3fVertex3fvSUN(const GLfloat *c, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glTexCoord2fVertex3fSUN(GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTexCoord2fVertex3fvSUN(const GLfloat *tc, const GLfloat *v);
GLAPI void APIENTRY glTexCoord4fVertex4fSUN(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLAPI void APIENTRY glTexCoord4fVertex4fvSUN(const GLfloat *tc, const GLfloat *v);
GLAPI void APIENTRY glTexCoord2fColor4ubVertex3fSUN(GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTexCoord2fColor4ubVertex3fvSUN(const GLfloat *tc, const GLubyte *c, const GLfloat *v);
GLAPI void APIENTRY glTexCoord2fColor3fVertex3fSUN(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTexCoord2fColor3fVertex3fvSUN(const GLfloat *tc, const GLfloat *c, const GLfloat *v);
GLAPI void APIENTRY glTexCoord2fNormal3fVertex3fSUN(GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTexCoord2fNormal3fVertex3fvSUN(const GLfloat *tc, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glTexCoord2fColor4fNormal3fVertex3fSUN(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glTexCoord2fColor4fNormal3fVertex3fvSUN(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glTexCoord4fColor4fNormal3fVertex4fSUN(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLAPI void APIENTRY glTexCoord4fColor4fNormal3fVertex4fvSUN(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiVertex3fSUN(GLuint rc, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiVertex3fvSUN(const GLuint *rc, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiColor4ubVertex3fSUN(GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiColor4ubVertex3fvSUN(const GLuint *rc, const GLubyte *c, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiColor3fVertex3fSUN(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiColor3fVertex3fvSUN(const GLuint *rc, const GLfloat *c, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiNormal3fVertex3fSUN(GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiNormal3fVertex3fvSUN(const GLuint *rc, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiColor4fNormal3fVertex3fSUN(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiColor4fNormal3fVertex3fvSUN(const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fVertex3fSUN(GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fVertex3fvSUN(const GLuint *rc, const GLfloat *tc, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN(GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN(const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN(GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GLAPI void APIENTRY glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN(const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);

typedef void (APIENTRY * PFNGLCOLOR4UBVERTEX2FSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y);
typedef void (APIENTRY * PFNGLCOLOR4UBVERTEX2FVSUN) (const GLubyte *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLCOLOR4UBVERTEX3FSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLCOLOR4UBVERTEX3FVSUN) (const GLubyte *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLCOLOR3FVERTEX3FSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLCOLOR3FVERTEX3FVSUN) (const GLfloat *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLNORMAL3FVERTEX3FSUN) (GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLNORMAL3FVERTEX3FVSUN) (const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLCOLOR4FNORMAL3FVERTEX3FSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLCOLOR4FNORMAL3FVERTEX3FVSUN) (const GLfloat *c, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD2FVERTEX3FSUN) (GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTEXCOORD2FVERTEX3FVSUN) (const GLfloat *tc, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD4FVERTEX4FSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLTEXCOORD4FVERTEX4FVSUN) (const GLfloat *tc, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR4UBVERTEX3FSUN) (GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR4UBVERTEX3FVSUN) (const GLfloat *tc, const GLubyte *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR3FVERTEX3FSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR3FVERTEX3FVSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD2FNORMAL3FVERTEX3FSUN) (GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTEXCOORD2FNORMAL3FVERTEX3FVSUN) (const GLfloat *tc, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLTEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLTEXCOORD4FCOLOR4FNORMAL3FVERTEX4FVSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUIVERTEX3FSUN) (GLuint rc, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUIVERTEX3FVSUN) (const GLuint *rc, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FSUN) (GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR4UBVERTEX3FVSUN) (const GLuint *rc, const GLubyte *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR3FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FSUN) (GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUINORMAL3FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUICOLOR4FNORMAL3FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FNORMAL3FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY * PFNGLREPLACEMENTCODEUITEXCOORD2FCOLOR4FNORMAL3FVERTEX3FVSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);

#endif /* GL_SUN_vertex */



/*
 * 167. WGL_EXT_display_color_table
 * 168. WGL_EXT_extensions_string
 * 169. WGL_EXT_make_current_read
 * 170. WGL_EXT_pixel_format
 * 171. WGL_EXT_pbuffer
 * 172. WGL_EXT_swap_control
 */



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
 * 174. GL_INGR_color_clamp
 */
#ifndef GL_INGR_color_clamp
#define GL_INGR_color_clamp 1

#define GL_RED_MIN_CLAMP_INGR			0x8560
#define GL_GREEN_MIN_CLAMP_INGR			0x8561
#define GL_BLUE_MIN_CLAMP_INGR			0x8562
#define GL_ALPHA_MIN_CLAMP_INGR			0x8563
#define GL_RED_MAX_CLAMP_INGR			0x8564
#define GL_GREEN_MAX_CLAMP_INGR			0x8565
#define GL_BLUE_MAX_CLAMP_INGR			0x8566
#define GL_ALPHA_MAX_CLAMP_INGR			0x8567
#define GL_RED_MIN_CLAMP_INGR			0x8560
#define GL_GREEN_MIN_CLAMP_INGR			0x8561
#define GL_BLUE_MIN_CLAMP_INGR			0x8562
#define GL_ALPHA_MIN_CLAMP_INGR			0x8563
#define GL_RED_MAX_CLAMP_INGR			0x8564
#define GL_GREEN_MAX_CLAMP_INGR			0x8565
#define GL_BLUE_MAX_CLAMP_INGR			0x8566
#define GL_ALPHA_MAX_CLAMP_INGR		        0x8567

#endif /* GL_INGR_color_clamp */



/*
 * 175. GL_INGR_interlace_read
 */
#ifndef GL_INGR_interlace_read
#define GL_INGR_interlace_read 1

#define GL_INTERLACE_READ_INGR			0x8568

#endif /* GL_INGR_interlace_read */



/*
 * 176. GL_EXT_stencil_wrap
 */
#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1

#define GL_INCR_WRAP_EXT			0x8507
#define GL_DECR_WRAP_EXT			0x8508

#endif /* GL_EXT_stencil_wrap */



/*
 * 177. WGL_EXT_depth_float
 */



/*
 * 178. GL_EXT_422_pixels
 */
#ifndef GL_EXT_422_pixels
#define GL_EXT_422_pixels 1

#define GL_422_EXT				0x80CC
#define GL_422_REV_EXT				0x80CD
#define GL_422_AVERAGE_EXT			0x80CE
#define GL_422_REV_AVERAGE_EXT	        	0x80CF

#endif /* GL_EXT_422_pixels */



/*
 * 179. GL_NV_texgen_reflection
 */
#ifndef GL_NV_texgen_reflection
#define GL_NV_texgen_reflection 1

#define GL_NORMAL_MAP_NV			0x8511
#define GL_REFLECTION_MAP_NV			0x8512

#endif /* GL_NV_texgen_reflection */



/*
 * 180. unknown
 * 181. unknown
 */



/*
 * 182. GL_SUN_convolution_border_modes
 */
#ifndef GL_SUN_convolution_border_modes
#define GL_SUN_convolution_border_modes 1

#define GL_WRAP_BORDER_SUN			0x81D4

#endif /* GL_SUN_convolution_border_modes */



/*
 * 183. GLX_SUN_transparent_index
 * 184. unknown
 */



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
 * 187. GL_EXT_texture_filter_anisotropic
 */
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1

#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF

#endif /* GL_EXT_texture_filter_anisotropic */



/*
 * 188. GL_EXT_vertex_weighting
 */
#ifndef GL_EXT_vertex_weighting
#define GL_EXT_vertex_weighting 1

#define GL_VERTEX_WEIGHTING_EXT			0x8509
#define GL_MODELVIEW0_EXT			0x1700
#define GL_MODELVIEW1_EXT			0x850A
#define GL_CURRENT_VERTEX_WEIGHT_EXT		0x850B
#define GL_VERTEX_WEIGHT_ARRAY_EXT		0x850C
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT		0x850D
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT		0x850E
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT	0x850F
#define GL_MODELVIEW0_STACK_DEPTH_EXT		0x0BA3
#define GL_MODELVIEW1_STACK_DEPTH_EXT		0x8502
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT	0x8510

GLAPI void APIENTRY glVertexWeightfEXT(GLfloat weight);
GLAPI void APIENTRY glVertexWeightfvEXT(const GLfloat *weight);
GLAPI void APIENTRY glVertexWeightPointerEXT(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

typedef void (APIENTRY * PFNGLVERTEXWEIGHTFEXTPROC) (GLfloat weight);
typedef void (APIENTRY * PFNGLVERTEXWEIGHTFVEXTPROC) (const GLfloat *weight);
typedef void (APIENTRY * PFNGLVERTEXWEIGHTPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

#endif /* GL_EXT_vertex_weighting */



/*
 * 189. GL_NV_light_max_exponent
 */
#ifndef GL_NV_light_max_exponent
#define GL_NV_light_max_exponent 1

#define GL_MAX_SHININESS_NV			0x8507
#define GL_MAX_SPOT_EXPONENT_NV			0x8508

#endif /* GL_NV_light_max_exponent */



/*
 * 190. GL_NV_vertex_array_range
 */
#ifndef GL_NV_vertex_array_range
#define GL_NV_vertex_array_range 1

#define GL_VERTEX_ARRAY_RANGE_NV		0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV		0x851E
#define GL_VERTEX_ARRAY_RANGE_VALID_NV		0x851F
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV	0x8520
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV	0x8521

GLAPI void APIENTRY glFlushVertexArrayRangeNV(void);
GLAPI void APIENTRY glVertexArrayRangeNV(GLsizei size, const GLvoid * pointer);

typedef void (APIENTRY * PFNGLFLUSHVERTEXARRAYRANGENVPROC) (void);
typedef void (APIENTRY * PFNGLVERTEXARRAYRANGENV) (GLsizei size, const GLvoid * pointer);

#endif /* GL_NV_vertex_array_range */



/*
 * 191. GL_NV_register_combiners
 */
#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners 1

#define GL_REGISTER_COMBINERS_NV		0x8522
#define GL_COMBINER0_NV				0x8550
#define GL_COMBINER1_NV				0x8551
#define GL_COMBINER2_NV				0x8552
#define GL_COMBINER3_NV				0x8553
#define GL_COMBINER4_NV				0x8554
#define GL_COMBINER5_NV				0x8555
#define GL_COMBINER6_NV				0x8556
#define GL_COMBINER7_NV				0x8557
#define GL_VARIABLE_A_NV			0x8523
#define GL_VARIABLE_B_NV			0x8524
#define GL_VARIABLE_C_NV			0x8525
#define GL_VARIABLE_D_NV			0x8526
#define GL_VARIABLE_E_NV			0x8527
#define GL_VARIABLE_F_NV			0x8528
#define GL_VARIABLE_G_NV			0x8529
#define GL_CONSTANT_COLOR0_NV			0x852A
#define GL_CONSTANT_COLOR1_NV			0x852B
#define GL_PRIMARY_COLOR_NV			0x852C
#define GL_SECONDARY_COLOR_NV			0x852D
#define GL_SPARE0_NV				0x852E
#define GL_SPARE1_NV				0x852F
#define GL_UNSIGNED_IDENTITY_NV			0x8536
#define GL_UNSIGNED_INVERT_NV			0x8537
#define GL_EXPAND_NORMAL_NV			0x8538
#define GL_EXPAND_NEGATE_NV			0x8539
#define GL_HALF_BIAS_NORMAL_NV			0x853A
#define GL_HALF_BIAS_NEGATE_NV			0x853B
#define GL_SIGNED_IDENTITY_NV			0x853C
#define GL_SIGNED_NEGATE_NV			0x853D
#define GL_E_TIMES_F_NV				0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV	0x8532
#define GL_SCALE_BY_TWO_NV			0x853E
#define GL_SCALE_BY_FOUR_NV			0x853F
#define GL_SCALE_BY_ONE_HALF_NV			0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV		0x8541
#define GL_DISCARD_NV				0x8530
#define GL_COMBINER_INPUT_NV			0x8542
#define GL_COMBINER_MAPPING_NV			0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV		0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV		0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV		0x8546
#define GL_COMBINER_MUX_SUM_NV			0x8547
#define GL_COMBINER_SCALE_NV			0x8548
#define GL_COMBINER_BIAS_NV			0x8549
#define GL_COMBINER_AB_OUTPUT_NV		0x854A
#define GL_COMBINER_CD_OUTPUT_NV		0x854B
#define GL_COMBINER_SUM_OUTPUT_NV		0x854C
#define GL_NUM_GENERAL_COMBINERS_NV		0x854E
#define GL_COLOR_SUM_CLAMP_NV			0x854F
#define GL_MAX_GENERAL_COMBINERS_NV		0x854D

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

#define GL_FOG_DISTANCE_MODE_NV			0x855A
#define GL_EYE_RADIAL_NV			0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV		0x855C

#endif /* GL_NV_fog_distance*/



/*
 * 193. unknown
 */



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
 * ARB 1. GL_ARB_multitexture
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
 * ARB 2. GLX_ARB_get_proc_address
 */


/*
 * ARB 3. GL_ARB_tranpose_matrix
 */
#ifndef GL_ARB_transpose_matrix
#define GL_ARB_transpose_matrix 1

#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB		0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB		0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB			0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX_ARB			0x84E6

GLAPI void APIENTRY glLoadTransposeMatrixdARB(const GLdouble m[16]);
GLAPI void APIENTRY glLoadTransposeMatrixfARB(const GLfloat m[16]);
GLAPI void APIENTRY glMultTransposeMatrixdARB(const GLdouble m[16]);
GLAPI void APIENTRY glMultTransposeMatrixfARB(const GLfloat m[16]);

typedef void (APIENTRY * PFNGLLOADTRANSPOSEMATRIXDARBPROC) ( const GLdouble m[16] );
typedef void (APIENTRY * PFNGLLOADTRANSPOSEMATRIXFARBPROC) ( const GLfloat m[16] );
typedef void (APIENTRY * PFNGLMULTTRANSPOSEMATRIXDARBPROC) ( const GLdouble m[16] );
typedef void (APIENTRY * PFNGLMULTTRANSPOSEMATRIXFARBPROC) ( const GLfloat m[16] );

#endif /* GL_ARB_tranpose_matrix */



/*
 * ARB 4. WGL_ARB_buffer_region
 */



/*
 * ARB 5. GL_ARB_multisample
 */
#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1

#define GL_MULTISAMPLE_ARBfunda			0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB		0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB		0x809F
#define GL_SAMPLE_COVERAGE_ARB			0x80A0
#define GL_MULTISAMPLE_BIT_ARB			0x20000000
#define GL_1PASS_ARB				0x80A1
#define GL_2PASS_0_ARB				0x80A2
#define GL_2PASS_1_ARB				0x80A3
#define GL_4PASS_0_ARB				0x80A4
#define GL_4PASS_1_ARB				0x80A5
#define GL_4PASS_2_ARB				0x80A6
#define GL_4PASS_3_ARB				0x80A7
#define GL_SAMPLE_BUFFERS_ARB			0x80A8
#define GL_SAMPLES_ARB				0x80A9
#define GL_SAMPLE_MAX_PASSES_ARB		0x84E7
#define GL_SAMPLE_PASS_ARB			0x84E8
#define GL_SAMPLE_COVERAGE_VALUE_ARB		0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB		0x80AB
/* Note: there are more tokens for GLX and WGL */

GLAPI void APIENTRY glSamplePassARB(GLenum pass);
GLAPI void APIENTRY glSampleCoverageARB(GLclampf value, GLboolean invert);

typedef void (APIENTRY * PFNGLSAMPLEPASSARBPROC) (GLenum pass);
typedef void (APIENTRY * PFNGLSAMPLECOVERAGEARBPROC) (GLclampf value, GLboolean invert);

#endif /* GL_ARB_multisample */



/*
 * ARB 6. GL_ARB_texture_env_add
 */
#ifndef GL_ARB_texture_env_add
#define GL_ARB_texture_env_add 1

/* No new tokens or functions */

#endif /* GL_ARB_texture_env_add */



/*
 * ARB ?. GL_ARB_texture_compression
 */
#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression 1

#define GL_COMPRESSED_ALPHA_ARB				0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB			0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB		0x84EB
#define GL_COMPRESSED_INTENSITY_ARB			0x84EC
#define GL_COMPRESSED_RGB_ARB				0x84ED
#define GL_COMPRESSED_RGBA_ARB				0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB			0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB		0x86A0
#define GL_TEXTURE_COMPRESSED_ARB			0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB		0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB		0x86A3

GLAPI void APIENTRY glCompressedTexImage3DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glCompressedTexImage2DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glCompressedTexImage1DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid *data);
GLAPI void APIENTRY glGetCompressedTexImageARB(GLenum target, GLint lod, GLvoid *img);

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE3DARBPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE1DARBPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, GLvoid *data);
typedef void (APIENTRY * PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) (GLenum target, GLint lod, GLvoid *img);

#endif /* GL_ARB_texture_compression */



/*
 * ?. GL_EXT_texture_cube_map
 */
#ifndef GL_EXT_texture_cube_map
#define GL_EXT_texture_cube_map 1

#define GL_NORMAL_MAP_EXT			0x8511
#define GL_REFLECTION_MAP_EXT			0x8512
#define GL_TEXTURE_CUBE_MAP_EXT			0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT		0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT	0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT	0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT	0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT	0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT	0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT	0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT		0x851B 
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT	0x851C

#endif /* GL_EXT_texture_cube_map */



/*
 * ? GL_NV_texgen_emboss
 */

#ifndef GL_NV_texgen_emboss
#define GL_NV_texgen_emboss 1

#define GL_EMBOSS_MAP_NV			0x855F
#define GL_EMBOSS_LIGHT_NV			0x855D
#define GL_EMBOSS_CONSTANT_NV			0x855E

#endif /* GL_NV_texgen_emboss */



/*
 * ??. GL_WIN_swap_hint
 */
#ifndef GL_WIN_swap_hint
#define GL_WIN_swap_hint 1

GLAPI void APIENTRY glAddSwapHintRectWIN(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (APIENTRY * PFNGLADDSWAPHINTRECTWINPROC) (GLint x, GLint y, GLsizei width, GLsizei height);

#endif /* GL_WIN_swap_hint */



/*
 * ?. GL_IBM_cull_vertex
 */
#ifndef GL_IBM_cull_vertex
#define GL_IBM_cull_vertex 1

#define GL_CULL_VERTEX_IBM			0x1928A

#endif /* GL_IBM_cull_vertex */



/*
 * ?. GL_IBM_static_data
 */
#ifndef GL_IBM_static_data
#define GL_IBM_static_data 1

#define GL_ALL_STATIC_DATA_IBM			0x19294
#define GL_STATIC_VERTEX_ARRAY_IBM		0x19295

GLAPI void APIENTRY glFlushStaticDataIBM(GLenum target);

typedef void (APIENTRY * PFNGLFLUSHSTATICDATAIBM) (GLenum target);

#endif /* GL_IBM_static_data */



/*
 * ?. GL_IBM_texture_mirrored_repeat
 */
#ifndef GL_IBM_texture_mirrored_repeat
#define GL_IBM_texture_mirrored_repeat 1

#define GL_MIRRORED_REPEAT_IBM			0x8370

#endif /* GL_IBM_texture_mirrored_repeat */



#ifdef __cplusplus
}
#endif


#endif /* __glext_h_ */
