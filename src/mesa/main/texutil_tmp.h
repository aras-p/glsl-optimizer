/*
 * NOTE: All 3D code is untested and most definitely broken...
 */

#define DST_TEXEL_BYTES		(4 / DST_TEXELS_PER_DWORD)
#define DST_ROW_WIDTH		(convert->width * DST_TEXEL_BYTES)
#define DST_ROW_STRIDE		(convert->imageWidth * DST_TEXEL_BYTES)
#define DST_IMG_STRIDE		(convert->imageWidth *			\
				 convert->imageHeight * DST_TEXEL_BYTES)


/* ================================================================
 * PRE: No pixelstore attribs, width == imageWidth.
 */
static GLboolean
TAG(texsubimage2d)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   GLuint *dst = (GLuint *)((GLubyte *)convert->dstImage +
			    (convert->yoffset * convert->imageWidth +
			     convert->xoffset) * DST_TEXEL_BYTES);
   GLint dwords, i;
   (void) dwords; (void) i;

   if ( DBG )
      fprintf( stderr, __FUNCTION__ "\n" );

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->height * DST_ROW_WIDTH );
#else
   dwords = (convert->width * convert->height +
	     DST_TEXELS_PER_DWORD - 1) / DST_TEXELS_PER_DWORD;

   for ( i = 0 ; i < dwords ; i++ ) {
      *dst++ = CONVERT_TEXEL_DWORD( src );
      src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
   }
#endif

   return GL_TRUE;
}

/* PRE: As above, height == imageHeight also.
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

   if ( DBG )
      fprintf( stderr, __FUNCTION__ "\n" );

#ifdef CONVERT_DIRECT
   MEMCPY( dst, src, convert->depth * convert->height * DST_ROW_WIDTH );
#else
   dwords = (convert->width * convert->height * convert->depth +
	     DST_TEXELS_PER_DWORD - 1) / DST_TEXELS_PER_DWORD;

   for ( i = 0 ; i < dwords ; i++ ) {
      *dst++ = CONVERT_TEXEL_DWORD( src );
      src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
   }
#endif

   return GL_TRUE;
}



/* ================================================================
 * PRE: No pixelstore attribs, width != imageWidth.
 */
static GLboolean
TAG(texsubimage2d_stride)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				(convert->yoffset * convert->imageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col;

   adjust = convert->imageWidth - convert->width;

   if ( DBG ) {
      fprintf( stderr, __FUNCTION__ ":\n" );
      fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
	       convert->xoffset, convert->yoffset, convert->width,
	       convert->height, convert->imageWidth );
      fprintf( stderr, "   adjust=%d\n", adjust );
   }

   for ( row = 0 ; row < convert->height ; row++ ) {
      for ( col = 0 ; col < convert->width ; col++ ) {
	 *dst++ = CONVERT_TEXEL( src );
	 src += SRC_TEXEL_BYTES;
      }
      dst += adjust;
   }

   return GL_TRUE;
}

/* PRE: As above, or height != imageHeight also.
 */
static GLboolean
TAG(texsubimage3d_stride)( struct gl_texture_convert *convert )
{
   const GLubyte *src = (const GLubyte *)convert->srcImage;
   DST_TYPE *dst = (DST_TYPE *)((GLubyte *)convert->dstImage +
				((convert->zoffset * convert->imageHeight +
				  convert->yoffset) * convert->imageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col, img;

   adjust = convert->imageWidth - convert->width;

   if ( DBG ) {
      fprintf( stderr, __FUNCTION__ ":\n" );
      fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
	       convert->xoffset, convert->yoffset, convert->width,
	       convert->height, convert->imageWidth );
      fprintf( stderr, "   adjust=%d\n", adjust );
   }

   for ( img = 0 ; img < convert->depth ; img++ ) {
      for ( row = 0 ; row < convert->height ; row++ ) {
	 for ( col = 0 ; col < convert->width ; col++ ) {
	    *dst++ = CONVERT_TEXEL( src );
	    src += SRC_TEXEL_BYTES;
	 }
	 dst += adjust;
      }
      /* FIXME: ... */
   }

   return GL_TRUE;
}



/* ================================================================
 * PRE: Require pixelstore attribs, width == imageWidth.
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

   if ( DBG )
      fprintf( stderr, __FUNCTION__ "\n" );

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
	 *dst++ = CONVERT_TEXEL_DWORD( src );
	 src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
      }
      src = srcRow + srcRowStride;
#endif
   }

   return GL_TRUE;
}

/* PRE: as above, height == imageHeight also.
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

   if ( DBG )
      fprintf( stderr, __FUNCTION__ "\n" );

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
	    *dst++ = CONVERT_TEXEL_DWORD( src );
	    src += SRC_TEXEL_BYTES * DST_TEXELS_PER_DWORD;
	 }
	 src = srcRow + srcRowStride;
#endif
      }
   }

   return GL_TRUE;
}



/* ================================================================
 * PRE: Require pixelstore attribs, width != imageWidth.
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
				(convert->yoffset * convert->imageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col;
   (void) col;

   adjust = convert->imageWidth - convert->width;

   if ( DBG ) {
      fprintf( stderr, __FUNCTION__ ":\n" );
      fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
	       convert->xoffset, convert->yoffset, convert->width,
	       convert->height, convert->imageWidth );
      fprintf( stderr, "   adjust=%d\n", adjust );
   }

   for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
      MEMCPY( dst, src, DST_ROW_WIDTH );
      src += srcRowStride;
      dst += convert->imageWidth;
#else
      const GLubyte *srcRow = src;
      for ( col = 0 ; col < convert->width ; col++ ) {
	 *dst++ = CONVERT_TEXEL( src );
	 src += SRC_TEXEL_BYTES;
      }
      src = srcRow + srcRowStride;
      dst += adjust;
#endif
   }

   return GL_TRUE;
}

/* PRE: As above, or height != imageHeight also.
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
				((convert->zoffset * convert->imageHeight +
				  convert->yoffset) * convert->imageWidth +
				 convert->xoffset) * DST_TEXEL_BYTES);
   GLint adjust;
   GLint row, col, img;
   (void) col;

   adjust = convert->imageWidth - convert->width;

   if ( DBG ) {
      fprintf( stderr, __FUNCTION__ ":\n" );
      fprintf( stderr, "   x=%d y=%d w=%d h=%d s=%d\n",
	       convert->xoffset, convert->yoffset, convert->width,
	       convert->height, convert->imageWidth );
      fprintf( stderr, "   adjust=%d\n", adjust );
   }

   for ( img = 0 ; img < convert->depth ; img++ ) {
      for ( row = 0 ; row < convert->height ; row++ ) {
#ifdef CONVERT_DIRECT
	 MEMCPY( dst, src, DST_ROW_WIDTH );
	 src += srcRowStride;
	 dst += convert->imageWidth;
#else
	 const GLubyte *srcRow = src;
	 for ( col = 0 ; col < convert->width ; col++ ) {
	    *dst++ = CONVERT_TEXEL( src );
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
