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
#include "indirect_size.h"
#include "glxclient.h"

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
		    GLenum format, GLenum type, GLenum target)
{
    int bytes_per_row;
    int components;

    switch( target ) {
    case GL_PROXY_TEXTURE_1D:
    case GL_PROXY_TEXTURE_2D:
    case GL_PROXY_TEXTURE_3D:
    case GL_PROXY_TEXTURE_4D_SGIS:
    case GL_PROXY_TEXTURE_CUBE_MAP:
    case GL_PROXY_TEXTURE_RECTANGLE_ARB:
    case GL_PROXY_HISTOGRAM:
    case GL_PROXY_COLOR_TABLE:
    case GL_PROXY_TEXTURE_COLOR_TABLE_SGI:
    case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
    case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
    case GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP:
	return 0;
    }

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
