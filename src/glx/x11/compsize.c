/* $XFree86: xc/lib/GL/glx/compsize.c,v 1.6 2004/01/28 18:11:38 alanh Exp $ */
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#include <GL/gl.h>
#include "glxclient.h"
#include "size.h"

/*
** Return the number of elements per group of a specified format
*/
GLint __glElementsPerGroup(GLenum format, GLenum type) 
{
    /*
    ** To make row length computation valid for image extraction,
    ** packed pixel types assume elements per group equals one.
    */
    switch(type) {
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_UNSIGNED_SHORT_8_8_APPLE:
    case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
    case GL_UNSIGNED_SHORT_15_1_MESA:
    case GL_UNSIGNED_SHORT_1_15_REV_MESA:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_24_8_NV:
    case GL_UNSIGNED_INT_24_8_MESA:
    case GL_UNSIGNED_INT_8_24_REV_MESA:
      return 1;
    default:
      break;
    }

    switch(format) {
      case GL_RGB:
      case GL_BGR:
	return 3;
      case GL_422_EXT:
      case GL_422_REV_EXT:
      case GL_422_AVERAGE_EXT:
      case GL_422_REV_AVERAGE_EXT:
      case GL_YCBCR_422_APPLE:
      case GL_LUMINANCE_ALPHA:
	return 2;
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
	return 4;
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_INTENSITY:
	return 1;
      default:
	return 0;
    }
}

/*
** Return the number of bytes per element, based on the element type (other
** than GL_BITMAP).
*/
GLint __glBytesPerElement(GLenum type) 
{
    switch(type) {
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
      case GL_UNSIGNED_SHORT_8_8_APPLE:
      case GL_UNSIGNED_SHORT_8_8_REV_APPLE:
      case GL_UNSIGNED_SHORT_15_1_MESA:
      case GL_UNSIGNED_SHORT_1_15_REV_MESA:
	return 2;
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_BYTE_3_3_2:
      case GL_UNSIGNED_BYTE_2_3_3_REV:
	return 1;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
      case GL_UNSIGNED_INT_24_8_NV:
      case GL_UNSIGNED_INT_24_8_MESA:
      case GL_UNSIGNED_INT_8_24_REV_MESA:
	return 4;
      default:
	return 0;
    }
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
GLint __glImageSize(GLsizei width, GLsizei height, GLsizei depth,
		    GLenum format, GLenum type) 
{
    int bytes_per_row;
    int components;

    if (width < 0 || height < 0 || depth < 0) {
	return 0;
    }
    /*
    ** Zero is returned if either format or type are invalid.
    */
    components = __glElementsPerGroup(format,type);
    if (type == GL_BITMAP) {
	if (format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX) {
	    bytes_per_row = (width + 7) >> 3;
	} else {
	    return 0;
	}
    } else {
	bytes_per_row = __glBytesPerElement(type) * width;
    }
    return bytes_per_row * height * depth * components;
}

GLint __glFogiv_size(GLenum pname)
{
    switch (pname) {
      case GL_FOG_COLOR:	return 4;
      case GL_FOG_DENSITY:	return 1;
      case GL_FOG_END:		return 1;
      case GL_FOG_MODE:		return 1;
      case GL_FOG_INDEX:	return 1;
      case GL_FOG_START:	return 1;
      case GL_FOG_DISTANCE_MODE_NV: return 1;
      case GL_FOG_OFFSET_VALUE_SGIX: return 1;
      default:
	return 0;
    }
}

GLint __glFogfv_size(GLenum pname)
{
    return __glFogiv_size(pname);
}

GLint __glCallLists_size(GLsizei n, GLenum type)
{
    GLint size;

    if (n < 0) return 0;
    switch (type) {
      case GL_BYTE:		size = 1; break;
      case GL_UNSIGNED_BYTE:	size = 1; break;
      case GL_SHORT:		size = 2; break;
      case GL_UNSIGNED_SHORT:	size = 2; break;
      case GL_INT:		size = 4; break;
      case GL_UNSIGNED_INT:	size = 4; break;
      case GL_FLOAT:		size = 4; break;
      case GL_2_BYTES:		size = 2; break;
      case GL_3_BYTES:		size = 3; break;
      case GL_4_BYTES:		size = 4; break;
      default:
	return 0;
    }
    return n * size;
}

GLint __glDrawPixels_size(GLenum format, GLenum type, GLsizei w, GLsizei h)
{
    return __glImageSize( w, h, 1, format, type );
}

GLint __glBitmap_size(GLsizei w, GLsizei h)
{
    return __glDrawPixels_size(GL_COLOR_INDEX, GL_BITMAP, w, h);
}

GLint __glTexGendv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_GEN_MODE:
	return 1;
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
	return 4;
      default:
	return 0;
    }
}

GLint __glTexGenfv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

GLint __glTexGeniv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

GLint __glTexParameterfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_WRAP_R:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_PRIORITY:
      case GL_TEXTURE_RESIDENT:
       
      /* GL_SGIS_texture_lod / GL_EXT_texture_lod / GL 1.2 */
      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
      case GL_TEXTURE_BASE_LEVEL:
      case GL_TEXTURE_MAX_LEVEL:

      /* GL_SGIX_texture_lod_bias */
      case GL_TEXTURE_LOD_BIAS_S_SGIX:
      case GL_TEXTURE_LOD_BIAS_T_SGIX:
      case GL_TEXTURE_LOD_BIAS_R_SGIX:

      /* GL_ARB_shadow / GL 1.4 */
      case GL_TEXTURE_COMPARE_MODE:
      case GL_TEXTURE_COMPARE_FUNC:

      /* GL_SGIS_generate_mipmap / GL 1.4 */
      case GL_GENERATE_MIPMAP:

      /* GL_ARB_depth_texture / GL 1.4 */
      case GL_DEPTH_TEXTURE_MODE:

      /* GL_EXT_texture_lod_bias / GL 1.4 */
      case GL_TEXTURE_LOD_BIAS:

      /* GL_SGIX_shadow_ambient / GL_ARB_shadow_ambient */
      case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:

      /* GL_SGIX_shadow */
      case GL_TEXTURE_COMPARE_SGIX:
      case GL_TEXTURE_COMPARE_OPERATOR_SGIX:

      /* GL_SGIX_texture_coordinate_clamp */
      case GL_TEXTURE_MAX_CLAMP_S_SGIX:
      case GL_TEXTURE_MAX_CLAMP_T_SGIX:
      case GL_TEXTURE_MAX_CLAMP_R_SGIX:

      /* GL_EXT_texture_filter_anisotropic */
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:

      /* GL_NV_texture_expand_normal */
      case GL_TEXTURE_UNSIGNED_REMAP_MODE_NV:
	return 1;

      /* GL_SGIX_clipmap */
      case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
      case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
	return 2;

      /* GL_SGIX_clipmap */
      case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
	return 3;

      case GL_TEXTURE_BORDER_COLOR:

      /* GL_SGIX_texture_scale_bias */
      case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
      case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
	return 4;

      default:
	return 0;
    }
}

GLint __glTexParameteriv_size(GLenum e)
{
    return __glTexParameterfv_size(e);
}

GLint __glTexEnvfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_ENV_MODE:

      /* GL_ARB_texture_env_combine / GL_EXT_texture_env_combine / GL 1.3 */
      case GL_COMBINE_RGB:
      case GL_COMBINE_ALPHA:
      case GL_SOURCE0_RGB:
      case GL_SOURCE1_RGB:
      case GL_SOURCE2_RGB:
      case GL_SOURCE0_ALPHA:
      case GL_SOURCE1_ALPHA:
      case GL_SOURCE2_ALPHA:
      case GL_OPERAND0_RGB:
      case GL_OPERAND1_RGB:
      case GL_OPERAND0_ALPHA:
      case GL_OPERAND1_ALPHA:
      case GL_OPERAND2_RGB:
      case GL_OPERAND2_ALPHA:
      case GL_RGB_SCALE:
      case GL_ALPHA_SCALE:

      /* GL_EXT_texture_lod_bias / GL 1.4 */
      case GL_TEXTURE_LOD_BIAS:

      /* GL_ARB_point_sprite / GL_NV_point_sprite */
      case GL_COORD_REPLACE_ARB:

      /* GL_NV_texture_env_combine4 */
      case GL_SOURCE3_RGB_NV:
      case GL_SOURCE3_ALPHA_NV:
      case GL_OPERAND3_RGB_NV:
      case GL_OPERAND3_ALPHA_NV:
        return 1;

      case GL_TEXTURE_ENV_COLOR:
	return 4;

      default:
	return 0;
    }
}

GLint __glTexEnviv_size(GLenum e)
{
    return __glTexEnvfv_size(e);
}

GLint __glTexImage1D_size(GLenum format, GLenum type, GLsizei w)
{
    return __glImageSize( w, 1, 1, format, type );
}

GLint __glTexImage2D_size(GLenum format, GLenum type, GLsizei w, GLsizei h)
{
    return __glImageSize( w, h, 1, format, type );
}

GLint __glTexImage3D_size(GLenum format, GLenum type, GLsizei w, GLsizei h,
			  GLsizei d)
{
    return __glImageSize( w, h, d, format, type );
}

GLint __glLightfv_size(GLenum pname)
{
    switch (pname) {
      case GL_SPOT_EXPONENT:		return 1;
      case GL_SPOT_CUTOFF:		return 1;
      case GL_AMBIENT:			return 4;
      case GL_DIFFUSE:			return 4;
      case GL_SPECULAR:			return 4;
      case GL_POSITION:			return 4;
      case GL_SPOT_DIRECTION:		return 3;
      case GL_CONSTANT_ATTENUATION:	return 1;
      case GL_LINEAR_ATTENUATION:	return 1;
      case GL_QUADRATIC_ATTENUATION:	return 1;
      default:
	return 0;
    }
}

GLint __glLightiv_size(GLenum pname)
{
    return __glLightfv_size(pname);
}

GLint __glLightModelfv_size(GLenum pname)
{
    switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:		return 4;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:		return 1;
      case GL_LIGHT_MODEL_TWO_SIDE:		return 1;
      case GL_LIGHT_MODEL_COLOR_CONTROL:	return 1;
      default:
	return 0;
    }
}

GLint __glLightModeliv_size(GLenum pname)
{
    return __glLightModelfv_size(pname);
}

GLint __glMaterialfv_size(GLenum pname)
{
    switch (pname) {
      case GL_SHININESS:		return 1;
      case GL_EMISSION:			return 4;
      case GL_AMBIENT:			return 4;
      case GL_DIFFUSE:			return 4;
      case GL_SPECULAR:			return 4;
      case GL_AMBIENT_AND_DIFFUSE:	return 4;
      case GL_COLOR_INDEXES:		return 3;
      default:
	return 0;
    }
}

GLint __glMaterialiv_size(GLenum pname)
{
    return __glMaterialfv_size(pname);
}

GLint __glColorTableParameterfv_size(GLenum pname)
{
    switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
      case GL_COLOR_TABLE_WIDTH:
      case GL_COLOR_TABLE_RED_SIZE:
      case GL_COLOR_TABLE_GREEN_SIZE:
      case GL_COLOR_TABLE_BLUE_SIZE:
      case GL_COLOR_TABLE_ALPHA_SIZE:
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
      case GL_COLOR_TABLE_INTENSITY_SIZE:
	return 1;
      case GL_COLOR_TABLE_SCALE:
      case GL_COLOR_TABLE_BIAS:
	return 4;
      default:
	return -1;
    }
}

GLint __glColorTableParameteriv_size(GLenum pname)
{
    return __glColorTableParameterfv_size(pname);
}

GLint __glConvolutionParameterfv_size(GLenum pname)
{
    switch(pname) {
      case GL_CONVOLUTION_BORDER_MODE:
	return 1;
      case GL_CONVOLUTION_BORDER_COLOR:
      case GL_CONVOLUTION_FILTER_SCALE:
      case GL_CONVOLUTION_FILTER_BIAS:
	return 4;
      default: /* error: bad enum value */
	return -1;
    }
}

GLint __glConvolutionParameteriv_size(GLenum pname)
{
    return __glConvolutionParameterfv_size(pname);
}

GLint __glPointParameterfvARB_size(GLenum e)
{
    switch (e) {
      case GL_POINT_SIZE_MIN:
      case GL_POINT_SIZE_MAX:
      case GL_POINT_FADE_THRESHOLD_SIZE:

      /* GL_NV_point_sprite */
      case GL_POINT_SPRITE_R_MODE_NV:
        return 1;

      case GL_POINT_DISTANCE_ATTENUATION:
	return 3;

      default:
        return -1;
    }
}

GLint __glPointParameteriv_size(GLenum e)
{
    return __glPointParameterfvARB_size(e);
}
