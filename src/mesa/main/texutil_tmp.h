/* $Id: texutil_tmp.h,v 1.8 2001/04/20 19:21:41 brianp Exp $ */

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
 *
 * Author:
 *    Gareth Hughes <gareth@valinux.com>
 */

/*
 * NOTE: All 3D teximage code is untested and most definitely broken...
 */


#define DST_TEXEL_BYTES		(4 / DST_TEXELS_PER_DWORD)
#define DST_ROW_WIDTH		(convert->width * DST_TEXEL_BYTES)
#define DST_ROW_STRIDE		(convert->dstImageWidth * DST_TEXEL_BYTES)
#define DST_IMG_STRIDE		(convert->dstImageWidth *		\
				 convert->dstImageHeight * DST_TEXEL_BYTES)


/* =============================================================
 * PRE: No pixelstore attribs, width == dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    (convert->yoffset * convert->dstImageWidth +
			     convert->xoffset) * DST_TEXEL_BYTES);
   GLint dwords, i;
   (void) dwords; (void) i;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ "\n" );
#endif

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->height * DST_ROW_WIDTH );
#else
   dwords = (convert->width * convert->height +
	     DST_TEXELS_PER_DWORD - 1) / DST_TEXELS_PER_DWORD;

   for ( i = 0 ; i < dwords ; i++ ) {
      CONVERT_TEXEL_DWORD( *dst++, src );
      src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
   }
#endif

   return GL_TRUE;
}

/* PRE: As above, height == dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    ((convert->zoffset * convert->height +
			      convert->yoffset) * convert->width +
			     convert->xoffset) * DST_TEXEL_BYTES);
   GLint dwords, i;
   (void) dwords; (void) i;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ "\n" );
#endif

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->depth * convert->height * DST_ROW_WIDTH );
#else
   dwords = (convert->width * convert->height * convert->depth +
	     DST_TEXELS_PER_DWORD - 1) / DST_TEXELS_PER_DWORD;

   for ( i = 0 ; i < dwords ; i++ ) {
      CONVERT_TEXEL_DWORD( *dst++, src );
      src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
   }
#endif

   return GL_TRUE;
}



/* =============================================================
 * PRE: No pixelstore attribs, width != dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d_stride)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				(convert->yoffset * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ ":\n" );
   fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   fprintf( stderr, "   adjust=%d\n", adjust );
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
TAG(texsubimage3d_stride)( struct gl_texture_convert *convert )
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
   fprintf( stderr, __FUNCTION__ ":\n" );
   fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   fprintf( stderr, "   adjust=%d\n", adjust );
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
TAG(texsubimage2d_pack)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->packing, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->packing, convert->width,
			      convert->format, convert->type );
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    (convert->yoffset * convert->width +
			     convert->xoffset) * DST_TEXEL_BYTES);
   GLint width;
   GLint row, col;
   (void) col;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ "\n" );
#endif

   width = ((convert->width + DST_TEXELS_PER_DWORD - 1)
	    & ~(DST_TEXELS_PER_DWORD - 1));

   for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
      MEMCPY( dst, src, DST_ROW_STRIDE );
      src += srcRowStride;
      dst = (GLuint *)((GLubyte *)dst + DST_ROW_STRIDE);
#else
      const GLubyte *srcRow = src;
      for ( col = width / DST_TEXELS_PER_DWORD ; col ; col-- ) {
	 CONVERT_TEXEL_DWORD( *dst++, src );
	 src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
      }
      src = srcRow + srcRowStride;
#endif
   }

   return GL_TRUE;
}

/* PRE: as above, height == dstImageHeight also.
 */
static GLboolean
TAG(texsubimage3d_pack)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->packing, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->packing, convert->width,
			      convert->format, convert->type );
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    ((convert->zoffset * convert->height +
			      convert->yoffset) * convert->width +
			     convert->xoffset) * DST_TEXEL_BYTES);
   GLint width;
   GLint row, col, img;
   (void) col;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ "\n" );
#endif

   width = ((convert->width + DST_TEXELS_PER_DWORD - 1)
	    & ~(DST_TEXELS_PER_DWORD - 1));

   for ( img = 0 ; img < convert->depth ; img++ ) {
      for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
	 MEMCPY( dst, src, DST_ROW_STRIDE );
	 src += srcRowStride;
	 dst = (GLuint *)((GLubyte *)dst + DST_ROW_STRIDE);
#else
	 const GLubyte *srcRow = src;
	 for ( col = width / DST_TEXELS_PER_DWORD ; col ; col-- ) {
	    CONVERT_TEXEL_DWORD( *dst++, src );
	    src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
	 }
	 src = srcRow + srcRowStride;
#endif
      }
   }

   return GL_TRUE;
}



/* =============================================================
 * PRE: Require pixelstore attribs, width != dstImageWidth.
 */
static GLboolean
TAG(texsubimage2d_stride_pack)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->packing, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->packing, convert->width,
			      convert->format, convert->type );
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				(convert->yoffset * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col;
   (void) col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ ":\n" );
   fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   fprintf( stderr, "   adjust=%d\n", adjust );
#endif

   for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
      MEMCPY( dst, src, DST_ROW_WIDTH );
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
TAG(texsubimage3d_stride_pack)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)
      _mesa_image_address( convert->packing, convert->srcImage,
			   convert->width, convert->height,
			   convert->format, convert->type, 0, 0, 0 );
   const GLint srcRowStride =
      _mesa_image_row_stride( convert->packing, convert->width,
			      convert->format, convert->type );
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				((convert->zoffset * convert->dstImageHeight +
				  convert->yoffset) * convert->dstImageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col, img;
   (void) col;

   adjust = convert->dstImageWidth - convert->width;

#if DEBUG_TEXUTIL
   fprintf( stderr, __FUNCTION__ ":\n" );
   fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
            convert->xoffset, convert->yoffset, convert->width,
            convert->height, convert->dstImageWidth );
   fprintf( stderr, "   adjust=%d\n", adjust );
#endif

   for ( img = 0 ; img < convert->depth ; img++ ) {
      for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
	 MEMCPY( dst, src, DST_ROW_WIDTH );
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
      /* FIXME: ... */
   }

   return GL_TRUE;
}



static convert_func TAG(texsubimage2d_tab)[] = {
   TAG(texsubimage2d),
   TAG(texsubimage2d_stride),
   TAG(texsubimage2d_pack),
   TAG(texsubimage2d_stride_pack),
};

static convert_func TAG(texsubimage3d_tab)[] = {
   TAG(texsubimage3d),
   TAG(texsubimage3d_stride),
   TAG(texsubimage3d_pack),
   TAG(texsubimage3d_stride_pack),
};


#ifndef PRESERVE_DST_TYPE
#undef DST_TYPE
#undef DST_TEXELS_PER_DWORD
#endif

#undef SRC_TEXEL_BYTES
#undef DST_TEXEL_BYTES
#undef DST_ROW_WIDTH
#undef DST_ROW_STRIDE

#undef CONVERT_TEXEL
#undef CONVERT_TEXEL_DWORD
#undef CONVERT_DIRECT

#undef TAG

#undef PRESERVE_DST_TYPE
