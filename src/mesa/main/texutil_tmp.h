
/*
 * Mesa 3-D graphics library
 * Version:  4.0.2
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 *
 * Author:
 *    Gareth Hughes
 */


/*
 * For 2D and 3D texture images, we generate functions for
 *  - conversion without pixel unpacking and standard stride
 *  - conversion without pixel unpacking and non-standard stride
 *  - conversion with pixel unpacking and standard stride
 *  - conversion with pixel unpacking and non-standard stride
 *
 *
 * Macros which need to be defined before including this file:
 *  TAG(x)  - the function name wrapper
 *  DST_TYPE - the destination texel datatype (GLuint, GLushort, etc)
 *  DST_TEXELS_PER_DWORD - number of dest texels that'll fit in 4 bytes
 *  CONVERT_TEXEL - code to convert from source to dest texel
 *  CONVER_TEXEL_DWORD - if multiple texels fit in 4 bytes, this macros
 *                       will convert/store multiple texels at once
 *  CONVERT_DIRECT - if defined, just memcpy texels from src to dest
 *  SRC_TEXEL_BYTES - bytes per source texel
 *  PRESERVE_DST_TYPE - if defined, don't undefined these macros at end
 */


#define DST_TEXEL_BYTES		(4 / DST_TEXELS_PER_DWORD)
#define DST_ROW_BYTES		(convert->width * DST_TEXEL_BYTES)
#define DST_ROW_STRIDE		(convert->dstImageWidth * DST_TEXEL_BYTES)
#define DST_IMG_STRIDE		(convert->dstImageWidth *		\
				 convert->dstImageHeight * DST_TEXEL_BYTES)


/* =============================================================
 * PRE: No pixelstore attribs, width == dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    (convert->yoffset * convert->dstImageWidth +
			     convert->xoffset) * DST_TEXEL_BYTES);

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ "\n" );
#endif

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->height * DST_ROW_BYTES );
#else
   {
      const GLint texels = convert->width * convert->height;
      const GLint dwords = texels / DST_TEXELS_PER_DWORD;
      const GLint leftover = texels - dwords * DST_TEXELS_PER_DWORD;
      GLint i;
      for ( i = 0 ; i < dwords ; i++ ) {
         CONVERT_TEXEL_DWORD( *dst++, src );
         src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
      }
      for ( i = 0; i < leftover; i++ ) {
         CONVERT_TEXEL( *dst++, src );
         src += SRC_TEXEL_BYTES;
      }
   }
#endif

   return GL_TRUE;
}

/* PRE: As above, height == dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    ((convert->zoffset * convert->height +
			      convert->yoffset) * convert->width +
			     convert->xoffset) * DST_TEXEL_BYTES);
#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ "\n" );
#endif

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->depth * convert->height * DST_ROW_BYTES );
#else
   {
      const GLint texels = convert->width * convert->height * convert->depth;
      const GLint dwords = texels / DST_TEXELS_PER_DWORD;
      const GLint leftover = texels - dwords * DST_TEXELS_PER_DWORD;
      GLint i;
      for ( i = 0 ; i < dwords ; i++ ) {
         CONVERT_TEXEL_DWORD( *dst++, src );
         src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
      }
      for ( i = 0; i < leftover; i++ ) {
         CONVERT_TEXEL( *dst++, src );
         src += SRC_TEXEL_BYTES;
      }
   }
#endif

   return GL_TRUE;
}



/* =============================================================
 * PRE: No pixelstore attribs, width != dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d_stride)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				(convert->yoffset * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ ":\n" );
   _mesa_debug( NULL, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   _mesa_debug( NULL, "   adjust=%d\n", adjust );
#endif

   for ( row = 0 ; row < convert->height ; row++ ) {
      for ( col = 0 ; col < convert->width ; col++ ) {
	 CONVERT_TEXEL( *dst++, src );
	 src += SRC_TEXEL_BYTES;
      }
      dst += adjust;
   }

   return GL_TRUE;
}

/* PRE: As above, or height != dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d_stride)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				((convert->zoffset * convert->dstImageHeight +
				  convert->yoffset) * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col, img;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ ":\n" );
   _mesa_debug( NULL, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   _mesa_debug( NULL, "   adjust=%d\n", adjust );
#endif

   for ( img = 0 ; img < convert->depth ; img++ ) {
      for ( row = 0 ; row < convert->height ; row++ ) {
	 for ( col = 0 ; col < convert->width ; col++ ) {
	    CONVERT_TEXEL( *dst++, src );
	    src += SRC_TEXEL_BYTES;
	 }
	 dst += adjust;
      }
      /* FIXME: ... */
   }

   return GL_TRUE;
}



/* =============================================================
 * PRE: Require pixelstore attribs, width == dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d_unpack)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->unpacking, convert->width,
			      convert->format, convert->type );
   GLint row, col;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ "\n" );
#endif

   if (convert->width & (DST_TEXELS_PER_DWORD - 1)) {
      /* Can't use dword conversion (i.e. when width = 1 and texels/dword = 2
       * or width = 2 and texels/dword = 4).
       */
      DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
                                   (convert->yoffset * convert->width +
                                    convert->xoffset) * DST_TEXEL_BYTES);
      for ( row = 0 ; row < convert->height ; row++ ) {
         const GLubyte *srcRow = src;
         for ( col = 0; col < convert->width; col++ ) {
            CONVERT_TEXEL(*dst, src);
            src += SRC_TEXEL_BYTES;
         }
         src = srcRow + srcRowStride;
      }
   }
   else {
      /* the common case */
      GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
                               (convert->yoffset * convert->width +
                                convert->xoffset) * DST_TEXEL_BYTES);
      for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
         MEMCPY( dst, src, DST_ROW_STRIDE );
         src += srcRowStride;
         dst = (GLuint *)((GLubyte *)dst + DST_ROW_STRIDE);
#else
         const GLubyte *srcRow = src;
         for ( col = convert->width / DST_TEXELS_PER_DWORD ; col ; col-- ) {
            CONVERT_TEXEL_DWORD( *dst++, src );
            src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
         }
         src = srcRow + srcRowStride;
#endif
      }
   }

   return GL_TRUE;
}

/* PRE: as above, height == dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d_unpack)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcImgStride = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 1, 0, 0 ) - src;
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->unpacking, convert->width,
			      convert->format, convert->type );
   GLint row, col, img;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ "\n" );
#endif

   if (convert->width & (DST_TEXELS_PER_DWORD - 1)) {
      /* Can't use dword conversion (i.e. when width = 1 and texels/dword = 2
       * or width = 2 and texels/dword = 4).
       */
      DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
                                   ((convert->zoffset * convert->height +
                                     convert->yoffset) * convert->width +
                                    convert->xoffset) * DST_TEXEL_BYTES);
      for ( img = 0 ; img < convert->depth ; img++ ) {
         const GLubyte *srcImage = src;
         for ( row = 0 ; row < convert->height ; row++ ) {
            const GLubyte *srcRow = src;
            for ( col = 0; col < convert->width; col++ ) {
               CONVERT_TEXEL(*dst, src);
               src += SRC_TEXEL_BYTES;
            }
            src = srcRow + srcRowStride;
         }
         src = srcImage + srcImgStride;
      }
   }
   else {
      /* the common case */
      GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
                               ((convert->zoffset * convert->height +
                                 convert->yoffset) * convert->width +
                                convert->xoffset) * DST_TEXEL_BYTES);
      for ( img = 0 ; img < convert->depth ; img++ ) {
         const GLubyte *srcImage = src;
         for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
            MEMCPY( dst, src, DST_ROW_STRIDE );
            src += srcRowStride;
            dst = (GLuint *)((GLubyte *)dst + DST_ROW_STRIDE);
#else
            const GLubyte *srcRow = src;
            for ( col = convert->width / DST_TEXELS_PER_DWORD ; col ; col-- ) {
               CONVERT_TEXEL_DWORD( *dst++, src );
               src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
            }
            src = srcRow + srcRowStride;
#endif
         }
         src = srcImage + srcImgStride;
      }
   }

   return GL_TRUE;
}



/* =============================================================
 * PRE: Require pixelstore attribs, width != dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d_stride_unpack)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->unpacking, convert->width,
			      convert->format, convert->type );
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				(convert->yoffset * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col = 0;
   (void) col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ ":\n" );
   _mesa_debug( NULL, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   _mesa_debug( NULL, "   adjust=%d\n", adjust );
#endif

   for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
      MEMCPY( dst, src, DST_ROW_BYTES );
      src += srcRowStride;
      dst += convert->dstImageWidth;
#else
      const GLubyte *srcRow = src;
      for ( col = 0 ; col < convert->width ; col++ ) {
	 CONVERT_TEXEL( *dst++, src );
	 src += SRC_TEXEL_BYTES;
      }
      src = srcRow + srcRowStride;
      dst += adjust;
#endif
   }

   return GL_TRUE;
}

/* PRE: As above, or height != dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d_stride_unpack)( const struct convert_info *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcImgStride = (const GLubyte *)
      _mesa_image_address( convert->unpacking, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 1, 0, 0 ) - src;
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->unpacking, convert->width,
			      convert->format, convert->type );
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				((convert->zoffset * convert->dstImageHeight +
				  convert->yoffset) * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col = 0, img;
   (void) col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   _mesa_debug( NULL, __FUNCTION__ ":\n" );
   _mesa_debug( NULL, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   _mesa_debug( NULL, "   adjust=%d\n", adjust );
#endif

   for ( img = 0 ; img < convert->depth ; img++ ) {
      const GLubyte *srcImage = src;
      for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
	 MEMCPY( dst, src, DST_ROW_BYTES );
	 src += srcRowStride;
	 dst += convert->dstImageWidth;
#else
	 const GLubyte *srcRow = src;
	 for ( col = 0 ; col < convert->width ; col++ ) {
	    CONVERT_TEXEL( *dst++, src );
	    src += SRC_TEXEL_BYTES;
	 }
	 src = srcRow + srcRowStride;
	 dst += adjust;
#endif
      }
      src = srcImage + srcImgStride;
   }

   return GL_TRUE;
}



static convert_func TAG(texsubimage2d_tab)[] = {
   TAG(texsubimage2d),
   TAG(texsubimage2d_stride),
   TAG(texsubimage2d_unpack),
   TAG(texsubimage2d_stride_unpack),
};

static convert_func TAG(texsubimage3d_tab)[] = {
   TAG(texsubimage3d),
   TAG(texsubimage3d_stride),
   TAG(texsubimage3d_unpack),
   TAG(texsubimage3d_stride_unpack),
};


#ifndef PRESERVE_DST_TYPE
#undef DST_TYPE
#undef DST_TEXELS_PER_DWORD
#endif

#undef SRC_TEXEL_BYTES
#undef DST_TEXEL_BYTES
#undef DST_ROW_BYTES
#undef DST_ROW_STRIDE

#undef CONVERT_TEXEL
#undef CONVERT_TEXEL_DWORD
#undef CONVERT_DIRECT

#undef TAG

#undef PRESERVE_DST_TYPE
