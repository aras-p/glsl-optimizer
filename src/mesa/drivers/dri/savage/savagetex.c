/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "mm.h"
#include "savagecontext.h"
#include "savagetex.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "simple_list.h"
#include "enums.h"
#include "savage_bci.h"

#include "macros.h"
#include "texformat.h"
#include "texstore.h"
#include "texobj.h"

#include "swrast/swrast.h"

/* declarations of static and inline functions */
__inline GLuint GetTiledCoordinates8(GLuint iBufferWidth, GLint x, GLint y);
static GLuint GetTiledCoordinates16_4( GLint iBufferWidth,GLint x,GLint y );
static GLuint GetTiledCoordinates16_8( GLint iBufferWidth,GLint x,GLint y );
static GLuint GetTiledCoordinates32_4( GLint iBufferWidth, GLint x,   GLint y );
static GLuint GetTiledCoordinates32_8( GLint iBufferWidth, GLint x,   GLint y );
__inline GLuint GetTiledCoordinates( GLint iDepth,GLint iBufferWidth,GLint x,GLint y );
__inline void savageUploadImage(savageTextureObjectPtr t, GLuint level, GLuint startx, GLuint starty, GLuint reloc);
__inline void  savageTileTex(savageTextureObjectPtr tex, GLuint level);

/* tile sizes depending on texel color depth */
GLuint gTileWidth[5] =
{
    64,    /* 4-bit */ 
    64,    /* 8-bit */
    64,    /* 16-bit */
    0,     /* 24-bit */
    32     /* 32-bit */
};

GLuint gTileHeight[5] = 
{
    64,   /* 4-bit */
    32,   /* 8-bit */
    16,   /* 16-bit */
    0,    /* 24-bit */
    16    /* 32-bit */
};

__inline GLuint GetTiledCoordinates8(GLuint iBufferWidth, GLint x, GLint y)
{
    GLint x10, x106, x52;
    GLint y20, y105, y43;
    GLuint uWidthInTiles;

    uWidthInTiles = (iBufferWidth + 63) >> 6;
    x10 = x & 0x3;
    x52 = (x & 0x3c) >> 2;
    x106 = (x & 0x7c0) >> 6;

    y20 = y & 0x7;
    y43 = (y & 0x18) >> 3;
    y105 = (y & 0x7e0) >> 5;
    
    return ( x10         |
            (y20 << 2)   |
	    (x52 << 5)   |
	    (y43 << 9)   |
	    ((y105 * uWidthInTiles) + x106) << 11 );
}

/* 4-pixel wide subtiles */
static GLuint GetTiledCoordinates16_4( GLint iBufferWidth,
                                              GLint x,
                                              GLint y )
{
    GLint  x106;
    GLint  x10;
    GLint  x52;
    GLint  y104;
    GLint  y20;
    GLint  y3;
    GLuint uiWidthInTiles;

    /*
    // calculating tiled address
    */

    uiWidthInTiles = (iBufferWidth + 63) >> 6;

    x10  =  x & 0x3;
    x52  = (x & 0x3c ) >> 2;
    x106 = (x & 0x7c0) >> 6;

    y20  =  y & 0x7;
    y3   = (y & 8    ) >> 3;
    y104 = (y & 0x7f0) >> 4;

    return( (x10 << 1)  |
            (y20 << 3)  |
            (x52 << 6)  |
            (y3  << 10) |
            ((y104 * uiWidthInTiles) + x106) << 11 );
}
/* 8-pixel wide subtiles */
static GLuint GetTiledCoordinates16_8( GLint iBufferWidth,
                                              GLint x,
                                              GLint y )
{
    GLint  x106;
    GLint  x20;
    GLint  x53;
    GLint  y104;
    GLint  y20;
    GLint  y3;
    GLuint uiWidthInTiles;

    /*
    // calculating tiled address
    */

    uiWidthInTiles = (iBufferWidth + 63) >> 6;

    x20  =  x & 0x7;
    x53  = (x & 0x38 ) >> 3;
    x106 = (x & 0x7c0) >> 6;

    y20  =  y & 0x7;
    y3   = (y & 8    ) >> 3;
    y104 = (y & 0x7f0) >> 4;

    return( (x20 << 1)  |
            (y20 << 4)  |
            (x53 << 7)  |
            (y3  << 10) |
            ((y104 * uiWidthInTiles) + x106) << 11 );
}
/* function pointer set to the correct version in savageDDInitTextureFuncs */
GLuint (*GetTiledCoordinates16) (GLint, GLint, GLint);

/* 4-pixel wide subtiles */
static GLuint GetTiledCoordinates32_4( GLint iBufferWidth,
                                              GLint x,
                                              GLint y )
{
    GLint  x10;
    GLint  y20;
    GLuint uiWidthInTiles;
    GLint  x42;
    GLint  x105;
    GLint  y3;
    GLint  y104;

    /*
    // calculating tiled address
    */

    uiWidthInTiles = (iBufferWidth + 31) >> 5;

    x10  =  x & 0x3;
    x42  = (x & 0x1c ) >> 2;
    x105 = (x & 0x7e0) >> 5;

    y20  =  y & 0x7;
    y3   = (y & 8    ) >> 3;
    y104 = (y & 0x7f0) >> 4;

    return( (x10 << 2)  |
            (y20 << 4)  |
            (x42 << 7)  |
            (y3  << 10) |
            ((y104 * uiWidthInTiles) + x105) << 11 );
}
/* 8-pixel wide subtiles */
static GLuint GetTiledCoordinates32_8( GLint iBufferWidth,
                                              GLint x,
                                              GLint y )
{
    GLint  x20;
    GLint  y20;
    GLuint uiWidthInTiles;
    GLint  x43;
    GLint  x105;
    GLint  y3;
    GLint  y104;

    /*
    // calculating tiled address
    */

    uiWidthInTiles = (iBufferWidth + 31) >> 5;

    x20  =  x & 0x7;
    x43  = (x & 0x18 ) >> 3;
    x105 = (x & 0x7e0) >> 5;

    y20  =  y & 0x7;
    y3   = (y & 8    ) >> 3;
    y104 = (y & 0x7f0) >> 4;

    return( (x20 << 2)  |
            (y20 << 5)  |
            (x43 << 8)  |
            (y3  << 10) |
            ((y104 * uiWidthInTiles) + x105) << 11 );
}
/* function pointer set to the correct version in savageDDInitTextureFuncs */
GLuint (*GetTiledCoordinates32) (GLint, GLint, GLint);

__inline GLuint GetTiledCoordinates( GLint iDepth,
                                            GLint iBufferWidth,
                                            GLint x,
                                            GLint y )
{
    /*
    // don't check for 4 since we only have 3 types of fb
    */

    if (iDepth == 16)
    {
        return( GetTiledCoordinates16( iBufferWidth, x, y ) );
    }
    else if (iDepth == 32)
    {
        return( GetTiledCoordinates32( iBufferWidth, x, y ) );
    }
    else
    {
        return( GetTiledCoordinates8( iBufferWidth, x, y ) );
    }
} 

__inline void savageUploadImage(savageTextureObjectPtr t, GLuint level, GLuint startx, GLuint starty, GLuint reloc)
{
    GLuint uMaxTileWidth = gTileWidth[t->texelBytes]; 
    GLuint x, y, w, row, col;    
    const struct gl_texture_image *image = t->image[level].image;
    GLubyte * dst, * src, * pBuffer;
    GLint xAdd, yAdd;
    GLuint uRowSeparator, uChunk = MIN_TILE_CHUNK, uWrap;

  
    pBuffer = (GLubyte *)(t->BufAddr + t->image[level].offset);
    src = (GLubyte *)image->Data;
    x = startx;
    y = starty;
    w = image->Width;

    if(image->Format ==  GL_COLOR_INDEX)
    {
        if(w < MIN_TILE_CHUNK)
	{
            w = MIN_TILE_CHUNK;
        }
        else
        {
	    if((w > 64 ) && (image->Height <= 16))
            {
	         reloc = GL_TRUE;
                 if(image->Height == 16)
		 {
		     uChunk = MIN_TILE_CHUNK << 1;
		 }
	    }
	}  

	if(!reloc & (w > (64 / 2)))
	{
	    for(row = 0; row < image->Height; row++)
	    {
	        for(col = 0; col < image->Width; col++)    
		{
		    dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, w, x + col, y + row);
		    memcpy (dst, src, t->texelBytes);
		    src += t->texelBytes;
                }
	    }
	}
        else
        {
	    if(reloc & (w > 64))
	    {
	      uWrap = ((w + 63) >> 6) - 1;
	        for(row = 0; row < image->Height; row++)
	        {
	            for(col = 0; col < image->Width; col++)    
		    {
		        xAdd = (col / (4 * 64)) * 64 + col % 64;
                        yAdd = row + ((col / 64) & 3) * uChunk;
		        dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, 64, x + xAdd, y + yAdd);
			memcpy (dst, src, t->texelBytes);
			src += t->texelBytes;
                    }
	        }
	    }
            else
	    {
	        uRowSeparator = 64 * MIN_TILE_CHUNK / w;
	        for(row = 0; row < image->Height; row++)
	        {
	            xAdd = (w * (row / MIN_TILE_CHUNK)) % 64;
                    yAdd = row % MIN_TILE_CHUNK + MIN_TILE_CHUNK * (row / uRowSeparator);
	            for(col = 0; col < image->Width; col++)    
		    {
		        dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, w, x + xAdd, y + yAdd);
			memcpy (dst, src, t->texelBytes);
			src += t->texelBytes;
                    }
	        }
	    }
        }
    }
    else
    {
        if(w < MIN_TILE_CHUNK)
	{
            w = MIN_TILE_CHUNK;
        }
        else
        {
	    if((w > uMaxTileWidth ) && (image->Height <= 8))
            {
	         reloc = GL_TRUE;
	    }
	}
	if(!reloc & (w > uMaxTileWidth / 2))
	{
	    for(row = 0; row < image->Height; row++)
	    {
	        for(col = 0; col < image->Width; col++)    
		{
		    dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, w, x + col, y + row);
		    memcpy (dst, src, t->texelBytes);
		    src += t->texelBytes;
                }
	    }
	}
        else
        {
	    if(reloc & (w > uMaxTileWidth))
	    {
	        for(row = 0; row < image->Height; row++)
	        {
	            for(col = 0; col < image->Width; col++)    
		    {
		        xAdd = (col / (2 * uMaxTileWidth)) * uMaxTileWidth + col % uMaxTileWidth;
                        yAdd = row + ((col / uMaxTileWidth) & 1) * MIN_TILE_CHUNK;
		        dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, w, x + xAdd, y + yAdd);
			memcpy (dst, src, t->texelBytes);
			src += t->texelBytes;
                    }
	        }
	    }
            else
	    {
	        uRowSeparator = uMaxTileWidth * MIN_TILE_CHUNK / w;
	        for(row = 0; row < image->Height; row++)
	        {
                    yAdd = row % MIN_TILE_CHUNK + MIN_TILE_CHUNK * (row / uRowSeparator);
                    xAdd = (w * (row / MIN_TILE_CHUNK)) % uMaxTileWidth;
	            for(col = 0; col < image->Width; col++)    
		    {
		        dst = pBuffer + GetTiledCoordinates(t->texelBytes << 3, w, x + col + xAdd, y + yAdd);
			memcpy (dst, src, t->texelBytes);
			src += t->texelBytes;
                    }
	        }
	    }
        }
    }
}        



__inline void  savageTileTex(savageTextureObjectPtr tex, GLuint level)
{
    GLuint uWidth, uHeight;
    GLint  xOffset, yOffset;
    GLint  xStart=0, yStart=0;
    GLint  minSize;
    GLuint xRepeat, yRepeat;
    GLuint startCol, startRow;
    GLuint reloc;

    const struct gl_texture_image *image = tex->image[level].image;
    
    reloc = GL_FALSE;
    uWidth = image->Width2;
    uHeight = image->Height2;

    if((uWidth > 4) || (uHeight > 4))
        minSize = MIN_TILE_CHUNK;
    else
        minSize = MIPMAP_CHUNK;
 
    if(image->Width >= minSize)
        xRepeat = 1;
    else
        xRepeat = minSize / image->Width;

    if(image->Height >= minSize)
        yRepeat = 1;
    else
    {
        yRepeat = minSize / image->Height;
        if(minSize == MIN_TILE_CHUNK)
            reloc = GL_TRUE;
    }
  
    if(((uWidth < 4) && (uHeight < 4)) && (tex->texelBytes >= 2))
    { 
        if((uWidth == 2) || (uHeight == 2))
        {
            xStart = 4;
            yStart = 0;
        }

        else
        {
            xStart = 4;
            yStart = 4;
        }
    }
    for(xOffset = 0; xOffset < xRepeat; xOffset++)
    {
        for(yOffset = 0; yOffset < yRepeat; yOffset++)
        {
	    startCol = image->Width * xOffset + xStart;
            startRow = image->Height * yOffset + yStart;
            savageUploadImage(tex,level, startCol, startRow, reloc);
	}
    }
}

static void savageSetTexWrapping(savageTextureObjectPtr tex, GLenum s, GLenum t)
{
    tex->texParams.sWrapMode = s;
    tex->texParams.tWrapMode = t;
}

static void savageSetTexFilter(savageTextureObjectPtr t, 
			       GLenum minf, GLenum magf)
{
   t->texParams.minFilter = minf;
   t->texParams.magFilter = magf;
}


/* Need a fallback ?
 */
static void savageSetTexBorderColor(savageTextureObjectPtr t, GLubyte color[4])
{
/*    t->Setup[SAVAGE_TEXREG_TEXBORDERCOL] =  */
      t->texParams.boarderColor = SAVAGEPACKCOLOR8888(color[0],color[1],color[2],color[3]); 
}



static savageTextureObjectPtr
savageAllocTexObj( struct gl_texture_object *texObj ) 
{
   savageTextureObjectPtr t;

   t = (savageTextureObjectPtr) calloc(1,sizeof(*t));
   texObj->DriverData = t;
   if ( t != NULL ) {

      /* Initialize non-image-dependent parts of the state:
       */
      t->globj = texObj;

      /* FIXME Something here to set initial values for other parts of
       * FIXME t->setup?
       */
  
      make_empty_list( t );

      savageSetTexWrapping(t,texObj->WrapS,texObj->WrapT);
      savageSetTexFilter(t,texObj->MinFilter,texObj->MagFilter);
      savageSetTexBorderColor(t,texObj->_BorderChan);
   }

   return t;
}

/* Called by the _mesa_store_teximage[123]d() functions. */
static const struct gl_texture_format *
savageChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			   GLenum format, GLenum type )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   const GLboolean do32bpt = GL_FALSE;
   const GLboolean force16bpt = GL_FALSE;
   const GLboolean isSavage4 = (imesa->savageScreen->chipset >= S3_SAVAGE4);
   (void) format;
   (void) type;

   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
      switch ( type ) {
      case GL_UNSIGNED_INT_10_10_10_2:
      case GL_UNSIGNED_INT_2_10_10_10_REV:
	 return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
      }

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
      switch ( type ) {
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	 return &_mesa_texformat_argb4444;
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	 return &_mesa_texformat_argb1555;
      case GL_UNSIGNED_SHORT_5_6_5:
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 return &_mesa_texformat_rgb565;
      default:
         return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;
      }

   case GL_RGBA8:
   case GL_RGBA12:
   case GL_RGBA16:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_RGB10_A2:
      return !force16bpt ?
	  &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555;

   case GL_RGBA4:
   case GL_RGBA2:
      return &_mesa_texformat_argb4444;

   case GL_RGB5_A1:
      return &_mesa_texformat_argb1555;

   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_rgb565;

   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
      return &_mesa_texformat_rgb565;

   case GL_ALPHA:
   case GL_COMPRESSED_ALPHA:
      return isSavage4 ? &_mesa_texformat_a8 : (
	 do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);
   case GL_ALPHA4:
      return isSavage4 ? &_mesa_texformat_a8 : &_mesa_texformat_argb4444;
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return isSavage4 ? &_mesa_texformat_a8 : (
	 !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);

   case 1:
   case GL_LUMINANCE:
   case GL_COMPRESSED_LUMINANCE:
      /* no alpha, but use argb1555 in 16bit case to get pure grey values */
      return isSavage4 ? &_mesa_texformat_l8 : (
	 do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555);
   case GL_LUMINANCE4:
      return isSavage4 ? &_mesa_texformat_l8 : &_mesa_texformat_argb1555;
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return isSavage4 ? &_mesa_texformat_l8 : (
	 !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb1555);

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      /* Savage4 has a al44 texture format. But it's not supported by Mesa. */
      return do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
      return &_mesa_texformat_argb4444;
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444;

   case GL_INTENSITY:
   case GL_COMPRESSED_INTENSITY:
      return isSavage4 ? &_mesa_texformat_i8 : (
	 do32bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);
   case GL_INTENSITY4:
      return isSavage4 ? &_mesa_texformat_i8 : &_mesa_texformat_argb4444;
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return isSavage4 ? &_mesa_texformat_i8 : (
	 !force16bpt ? &_mesa_texformat_argb8888 : &_mesa_texformat_argb4444);
/*
   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_ci8;
*/
   default:
      _mesa_problem(ctx, "unexpected texture format in %s", __FUNCTION__);
      return NULL;
   }
}

static void savageSetTexImages( savageContextPtr imesa,
				const struct gl_texture_object *tObj )
{
   savageTextureObjectPtr t = (savageTextureObjectPtr) tObj->DriverData;
   struct gl_texture_image *image = tObj->Image[0][tObj->BaseLevel];
   GLuint offset, width, pitch, i, textureFormat, log_pitch;

   assert(t);
   assert(image);

   switch (image->TexFormat->MesaFormat) {
   case MESA_FORMAT_ARGB8888:
      textureFormat = TFT_ARGB8888;
      t->texelBytes = 4;
      break;
   case MESA_FORMAT_ARGB1555:
      textureFormat = TFT_ARGB1555;
      t->texelBytes = 2;
      break;
   case MESA_FORMAT_ARGB4444:
      textureFormat = TFT_ARGB4444;
      t->texelBytes = 2;
      break;
   case MESA_FORMAT_RGB565:
      textureFormat = TFT_RGB565;
      t->texelBytes = 2;
      break;
   case MESA_FORMAT_L8:
      textureFormat = TFT_L8;
      t->texelBytes = 1;
      break;
   case MESA_FORMAT_I8:
      textureFormat = TFT_I8;
      t->texelBytes = 1;
      break;
   case MESA_FORMAT_A8:
      textureFormat = TFT_A8;
      t->texelBytes = 1;
      break;
   default:
      _mesa_problem(imesa->glCtx, "Bad texture format in %s", __FUNCTION__);
   }

   /* Figure out the size now (and count the levels).  Upload won't be done
    * until later.
    */ 
   width = image->Width * t->texelBytes;
   for (pitch = 2, log_pitch=1 ; pitch < width ; pitch *= 2 )
      log_pitch++;
   
   t->dirty_images = 0;
   t->bound = 0;

   offset = 0;
   for ( i = 0 ; i < SAVAGE_TEX_MAXLEVELS && tObj->Image[0][i] ; i++ ) {
      t->image[i].image = tObj->Image[0][i];
      t->image[i].offset = offset;
      t->image[i].internalFormat = textureFormat;
      t->dirty_images |= (1<<i);
      offset += t->image[i].image->Height * pitch;
      pitch = pitch >> 1;
   }

   t->totalSize = offset;
   t->max_level = i-1;
   t->min_level = 0;
}

void savageDestroyTexObj(savageContextPtr imesa, savageTextureObjectPtr t)
{
   if (!t) return;

   /* This is sad - need to sync *in case* we upload a texture
    * to this newly free memory...
    */
   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (t->age > imesa->dirtyAge)
	 imesa->dirtyAge = t->age;
   }

   if (t->globj)
      t->globj->DriverData = 0;

   if (t->bound)
      imesa->CurrentTexObj[t->bound - 1] = 0; 

   remove_from_list(t);
   free(t);
}


static void savageSwapOutTexObj(savageContextPtr imesa, savageTextureObjectPtr t)
{
   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;      

      if (t->age > imesa->dirtyAge)
	 imesa->dirtyAge = t->age;
   }

   t->dirty_images = ~0;
   move_to_tail(&(imesa->SwappedOut), t);
}



/* Upload an image from mesa's internal copy.
 */
static void savageUploadTexLevel( savageTextureObjectPtr t, int level )
{
   const struct gl_texture_image *image = t->image[level].image;


   /* Need triangle (rather than pixel) fallbacks to simulate this using
    * normal textured triangles.
    *
    * DO THIS IN DRIVER STATE MANAGMENT, not hardware state.
    * 
    */

   if(image->Border != 0) 
       fprintf (stderr, "Not supported texture border %d.\n",
		(int) image->Border);

   savageTileTex(t, level);
}



void savagePrintLocalLRU( savageContextPtr imesa , GLuint heap) 
{
   savageTextureObjectPtr t;
   int sz = 1 << (imesa->savageScreen->logTextureGranularity[heap]);
   
   foreach( t, &imesa->TexObjList[heap] ) {
      if (!t->globj)
	 fprintf(stderr, "Placeholder %d at %x sz %x\n", 
		 t->MemBlock->ofs / sz,
		 t->MemBlock->ofs,
		 t->MemBlock->size);      
      else
	 fprintf(stderr, "Texture (bound %d) at %x sz %x\n", 
		 t->bound,
		 t->MemBlock->ofs,
		 t->MemBlock->size);      

   }
}

void savagePrintGlobalLRU( savageContextPtr imesa , GLuint heap)
{
   int i, j;

   drm_savage_tex_region_t *list = imesa->sarea->texList[heap];
   

   for (i = 0, j = SAVAGE_NR_TEX_REGIONS ; i < SAVAGE_NR_TEX_REGIONS ; i++) {
      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
	      j, list[j].age, list[j].next, list[j].prev);
      j = list[j].next;
      if (j == SAVAGE_NR_TEX_REGIONS) break;
   }
   
   if (j != SAVAGE_NR_TEX_REGIONS)
      fprintf(stderr, "Loop detected in global LRU\n");
       for (i = 0 ; i < SAVAGE_NR_TEX_REGIONS ; i++) 
       {
          fprintf(stderr,"list[%d] age %d next %d prev %d\n",
          i, list[i].age, list[i].next, list[i].prev);
       }
}


void savageResetGlobalLRU( savageContextPtr imesa, GLuint heap )
{
    drm_savage_tex_region_t *list = imesa->sarea->texList[heap];
   int sz = 1 << imesa->savageScreen->logTextureGranularity[heap];
   int i;

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (SAVAGE_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.  
    */
   for (i = 0 ; (i+1) * sz <= imesa->savageScreen->textureSize[heap]; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = SAVAGE_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = SAVAGE_NR_TEX_REGIONS;
   list[SAVAGE_NR_TEX_REGIONS].prev = i;
   list[SAVAGE_NR_TEX_REGIONS].next = 0;
   imesa->sarea->texAge[heap] = 0;
}


static void savageUpdateTexLRU( savageContextPtr imesa, savageTextureObjectPtr t ) 
{
   int i;
   int heap = t->heap;
   int logsz = imesa->savageScreen->logTextureGranularity[heap];
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   drm_savage_tex_region_t *list = imesa->sarea->texList[heap];
   
   imesa->texAge[heap] = ++imesa->sarea->texAge[heap];

   /* Update our local LRU
    */
   move_to_head( &(imesa->TexObjList[heap]), t );

   /* Update the global LRU
    */
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = imesa->texAge[heap];

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;
      
      /* insert_at_head(list, i)
       */
      list[i].prev = SAVAGE_NR_TEX_REGIONS;
      list[i].next = list[SAVAGE_NR_TEX_REGIONS].next;
      list[(unsigned)list[SAVAGE_NR_TEX_REGIONS].next].prev = i;
      list[SAVAGE_NR_TEX_REGIONS].next = i;
   }
}


/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent 
 * the other client's textures.  
 */
void savageTexturesGone( savageContextPtr imesa,
		       GLuint heap,
		       GLuint offset, 
		       GLuint size,
		       GLuint in_use ) 
{
   savageTextureObjectPtr t, tmp;
   
   foreach_s ( t, tmp, &imesa->TexObjList[heap] ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
	 continue;

      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
      if (t->bound)
	 savageSwapOutTexObj( imesa, t );
      else
	 savageDestroyTexObj( imesa, t );
   }

   
   if (in_use) {
      t = (savageTextureObjectPtr) calloc(1,sizeof(*t));
      if (!t) return;

      t->heap = heap;
      t->MemBlock = mmAllocMem( imesa->texHeap[heap], size, 0, offset);      
      if(!t->MemBlock)
      {
          free(t);
          return;
      }
      insert_at_head( &imesa->TexObjList[heap], t );
   }
}





/* This is called with the lock held.  May have to eject our own and/or
 * other client's texture objects to make room for the upload.
 */
int savageUploadTexImages( savageContextPtr imesa, savageTextureObjectPtr t )
{
   int heap;
   int i;
   int ofs;
   
   heap = t->heap = SAVAGE_CARD_HEAP;

   /* Do we need to eject LRU texture objects?
    */
   if (!t->MemBlock) {
      while (1)
      {
	 t->MemBlock = mmAllocMem( imesa->texHeap[heap], t->totalSize, 12, 0 ); 
	 if (t->MemBlock)
	    break;
	 else
	 {
	     heap = t->heap = SAVAGE_AGP_HEAP;
	     t->MemBlock = mmAllocMem( imesa->texHeap[heap], t->totalSize, 12, 0 ); 
	     
	     if (t->MemBlock)
	         break;
	 }

	 if (imesa->TexObjList[heap].prev->bound) {
  	    fprintf(stderr, "Hit bound texture in upload\n"); 
	    savagePrintLocalLRU( imesa,heap );
	    return -1;
	 }

	 if (imesa->TexObjList[heap].prev == &(imesa->TexObjList[heap])) {
 	    fprintf(stderr, "Failed to upload texture, sz %d\n", t->totalSize);
	    mmDumpMemInfo( imesa->texHeap[heap] );
	    return -1;
	 }
	 
	 savageDestroyTexObj( imesa, imesa->TexObjList[heap].prev );
      }
 
      ofs = t->MemBlock->ofs;
      t->texParams.hwPhysAddress = imesa->savageScreen->textureOffset[heap] + ofs;
      t->BufAddr = (char *)((GLuint) imesa->savageScreen->texVirtual[heap] + ofs);
      imesa->dirty |= SAVAGE_UPLOAD_CTX;
   }

   /* Let the world know we've used this memory recently.
    */
   savageUpdateTexLRU( imesa, t );

   if (t->dirty_images) {
      if (SAVAGE_DEBUG & DEBUG_VERBOSE_LRU)
	 fprintf(stderr, "*");

      for (i = t->min_level ; i <= t->max_level ; i++)
	 if (t->dirty_images & (1<<i)) 
	    savageUploadTexLevel( t, i );
   }


   t->dirty_images = 0;
   return 0;
}

static void savageTexSetUnit( savageTextureObjectPtr t, GLuint unit )
{
   if (t->current_unit == unit) return;

   t->current_unit = unit;
}




static void savageUpdateTex0State_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   struct gl_texture_object	*tObj;
   savageTextureObjectPtr t;
   GLuint format;
   Reg_TexCtrl TexCtrl;
   Reg_TexBlendCtrl TexBlendCtrl;
   Reg_TexDescr TexDescr; 

   /* disable */
   
   if (ctx->Texture.Unit[0]._ReallyEnabled == 0) {
      imesa->Registers.TexDescr.s4.tex0En = GL_FALSE;
      imesa->Registers.TexBlendCtrl[0].ui = TBC_NoTexMap;
      imesa->Registers.TexCtrl[0].ui = 0x20f040;
      imesa->Registers.TexAddr[0].ui = 0;
      imesa->Registers.changed.ni.fTex0BlendCtrlChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTex0AddrChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTexDescrChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTex0CtrlChanged = GL_TRUE;
      return;
   }

   tObj = ctx->Texture.Unit[0]._Current;
   if (ctx->Texture.Unit[0]._ReallyEnabled != TEXTURE_2D_BIT ||
       tObj->Image[0][tObj->BaseLevel]->Border > 0) {
      /* 1D or 3D texturing enabled, or texture border - fallback */
      FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
      return;
   }

   /* Do 2D texture setup */

   t = tObj->DriverData;
   if (!t) {
      t = savageAllocTexObj( tObj );
      if (!t)
         return;
   }

   if (t->current_unit != 0)
      savageTexSetUnit( t, 0 );
    
   imesa->CurrentTexObj[0] = t;
   t->bound = 1;

   if (t->dirty_images) {
       savageSetTexImages(imesa, tObj);
       savageUploadTexImages(imesa, imesa->CurrentTexObj[0]); 
   }
   
   if (t->MemBlock)
      savageUpdateTexLRU( imesa, t );
  
   TexDescr.ui     = imesa->Registers.TexDescr.ui & ~0x01000000;
   TexCtrl.ui      = imesa->Registers.TexCtrl[0].ui;
   TexBlendCtrl.ui = imesa->Registers.TexBlendCtrl[0].ui;

   format = tObj->Image[0][tObj->BaseLevel]->Format;

   switch (ctx->Texture.Unit[0].EnvMode) {
   case GL_REPLACE:
      TexCtrl.s4.clrArg1Invert = GL_FALSE;
      switch(format)
      {
          case GL_LUMINANCE:
          case GL_RGB:
               TexBlendCtrl.ui = TBC_Decal;
               break;

          case GL_LUMINANCE_ALPHA:
          case GL_RGBA:
          case GL_INTENSITY:
               TexBlendCtrl.ui = TBC_Copy;
               break;

          case GL_ALPHA:
               TexBlendCtrl.ui = TBC_CopyAlpha;
               break;
      }
       __HWEnvCombineSingleUnitScale(imesa, 0, 0, &TexBlendCtrl);
      break;

    case GL_DECAL:
        TexCtrl.s4.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_RGB:
            case GL_LUMINANCE:
                TexBlendCtrl.ui = TBC_Decal;
                break;

            case GL_RGBA:
            case GL_INTENSITY:
            case GL_LUMINANCE_ALPHA:
                TexBlendCtrl.ui = TBC_DecalAlpha;
                break;

            /*
             GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_INTENSITY
             are undefined with GL_DECAL
            */

            case GL_ALPHA:
                TexBlendCtrl.ui = TBC_CopyAlpha;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 0, &TexBlendCtrl);
        break;

    case GL_MODULATE:
        TexCtrl.s4.clrArg1Invert = GL_FALSE;
        TexBlendCtrl.ui = TBC_ModulAlpha;
        __HWEnvCombineSingleUnitScale(imesa, 0, 0, &TexBlendCtrl);
        break;

    case GL_BLEND:

        switch (format)
        {
            case GL_ALPHA:
                TexBlendCtrl.ui = TBC_ModulAlpha;
                TexCtrl.s4.clrArg1Invert = GL_FALSE;
                break;

            case GL_LUMINANCE:
            case GL_RGB:
                TexBlendCtrl.ui = TBC_Blend0;
                TexDescr.s4.tex1En = GL_TRUE;
                TexDescr.s4.texBLoopEn = GL_TRUE;
                TexDescr.s4.tex1Width  = TexDescr.s4.tex0Width;
                TexDescr.s4.tex1Height = TexDescr.s4.tex0Height;
                TexDescr.s4.tex1Fmt = TexDescr.s4.tex0Fmt;

                if (imesa->Registers.TexAddr[1].ui != imesa->Registers.TexAddr[0].ui)
                {
                    imesa->Registers.TexAddr[1].ui = imesa->Registers.TexAddr[0].ui;
                    imesa->Registers.changed.ni.fTex1AddrChanged = GL_TRUE;
                }

                if (imesa->Registers.TexBlendCtrl[1].ui != TBC_Blend1)
                {
                    imesa->Registers.TexBlendCtrl[1].ui = TBC_Blend1;
                    imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
                }

                TexCtrl.s4.clrArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;

            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
                TexBlendCtrl.ui = TBC_BlendAlpha0;
                TexDescr.s4.tex1En = GL_TRUE;
                TexDescr.s4.texBLoopEn = GL_TRUE;
                TexDescr.s4.tex1Width  = TexDescr.s4.tex0Width;
                TexDescr.s4.tex1Height = TexDescr.s4.tex0Height;
                TexDescr.s4.tex1Fmt = TexDescr.s4.tex0Fmt;

                if (imesa->Registers.TexAddr[1].ui != imesa->Registers.TexAddr[0].ui)
                {
                    imesa->Registers.TexAddr[1].ui = imesa->Registers.TexAddr[0].ui;
                    imesa->Registers.changed.ni.fTex1AddrChanged = GL_TRUE;
                }

                if (imesa->Registers.TexBlendCtrl[1].ui != TBC_BlendAlpha1)
                {
                    imesa->Registers.TexBlendCtrl[1].ui = TBC_BlendAlpha1;
                    imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
                }

                TexCtrl.s4.clrArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;

            case GL_INTENSITY:
                TexBlendCtrl.ui = TBC_BlendInt0;
                TexDescr.s4.tex1En = GL_TRUE;
                TexDescr.s4.texBLoopEn = GL_TRUE;
                TexDescr.s4.tex1Width  = TexDescr.s4.tex0Width;
                TexDescr.s4.tex1Height = TexDescr.s4.tex0Height;
                TexDescr.s4.tex1Fmt = TexDescr.s4.tex0Fmt;

                if (imesa->Registers.TexAddr[1].ui != imesa->Registers.TexAddr[0].ui)
                {
                    imesa->Registers.TexAddr[1].ui = imesa->Registers.TexAddr[0].ui;
                    imesa->Registers.changed.ni.fTex1AddrChanged = GL_TRUE;
                }

                if (imesa->Registers.TexBlendCtrl[1].ui != TBC_BlendInt1)
                {
                    imesa->Registers.TexBlendCtrl[1].ui = TBC_BlendInt1;
                    imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
                }
                TexCtrl.s4.clrArg1Invert = GL_TRUE;
                TexCtrl.s4.alphaArg1Invert = GL_TRUE;
                imesa->bTexEn1 = GL_TRUE;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 0, &TexBlendCtrl);
        break;

        /*
         GL_ADD
        */
    case GL_ADD:
        printf("Add\n");
        TexCtrl.s4.clrArg1Invert = GL_FALSE;
        TexBlendCtrl.ui = TBC_AddAlpha;
        __HWEnvCombineSingleUnitScale(imesa, 0, 0, &TexBlendCtrl);
        break;

#if GL_ARB_texture_env_combine
    case GL_COMBINE_ARB:
        __HWParseTexEnvCombine(imesa, 0, &TexCtrl, &TexBlendCtrl);
        break;
#endif

   default:
      fprintf(stderr, "unknown tex env mode");
      exit(1);
      break;			
   }

    TexCtrl.s4.uMode = !(t->texParams.sWrapMode & 0x01);
    TexCtrl.s4.vMode = !(t->texParams.tWrapMode & 0x01);

    switch (t->texParams.minFilter)
    {
        case GL_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Point;
            TexCtrl.s4.mipmapEnable = GL_FALSE;
            break;

        case GL_LINEAR:
            TexCtrl.s4.filterMode   = TFM_Bilin;
            TexCtrl.s4.mipmapEnable = GL_FALSE;
            break;

        case GL_NEAREST_MIPMAP_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Point;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;

        case GL_LINEAR_MIPMAP_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Bilin;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;

        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_LINEAR:
            TexCtrl.s4.filterMode   = TFM_Trilin;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;
    }

    if((ctx->Texture.Unit[0].LodBias !=0.0F) && (TexCtrl.s4.dBias != 0))
    {
	union {
	    GLfloat f;
	    GLint i;
	} bias;
    	GLuint  ul;
    	
    	bias.f = ctx->Texture.Unit[0].LodBias;
    	
    	/* if the value is >= 15.9375 determine whether >= 16
    	   or <0
    	*/
    	if(((bias.i) & 0x7FFFFFFF) >= 0x417F0000)
    	{
    	    if((bias.i) & 0x80000000)
    	    {
    	        ul=0x101;
    	    }
    	    else
    	    {
    	        ul=0xff;
    	    }
    	}
    	else
    	{
            ul=(GLuint)(bias.f*16.0);
        }
        
        ul &= 0x1FF;
        TexCtrl.s4.dBias = ul;
    }

    TexDescr.s4.tex0En = GL_TRUE;
    TexDescr.s4.tex0Width  = t->image[0].image->WidthLog2;
    TexDescr.s4.tex0Height = t->image[0].image->HeightLog2;
    TexDescr.s4.tex0Fmt = t->image[0].internalFormat;
    TexCtrl.s4.dMax = t->max_level;

    if (TexDescr.s4.tex1En)
        TexDescr.s4.texBLoopEn = GL_TRUE;

    if (imesa->Registers.TexAddr[0].ui != (GLuint)t->texParams.hwPhysAddress)
    {
        imesa->Registers.TexAddr[0].ui = (GLuint) t->texParams.hwPhysAddress | 0x2;
        
        if(t->heap == SAVAGE_AGP_HEAP)
            imesa->Registers.TexAddr[0].ui |= 0x1;
            
        imesa->Registers.changed.ni.fTex0AddrChanged = GL_TRUE;
    }

    if (imesa->Registers.TexCtrl[0].ui != TexCtrl.ui)
    {
        imesa->Registers.TexCtrl[0].ui = TexCtrl.ui;
        imesa->Registers.changed.ni.fTex0CtrlChanged = GL_TRUE;
    }

    if (imesa->Registers.TexBlendCtrl[0].ui != TexBlendCtrl.ui)
    {
        imesa->Registers.TexBlendCtrl[0].ui = TexBlendCtrl.ui;
        imesa->Registers.changed.ni.fTex0BlendCtrlChanged = GL_TRUE;
    }

    if (imesa->Registers.TexDescr.ui != TexDescr.ui)
    {
        imesa->Registers.TexDescr.ui = TexDescr.ui;
        imesa->Registers.changed.ni.fTexDescrChanged = GL_TRUE;
    }

   return;
}
static void savageUpdateTex1State_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   struct gl_texture_object	*tObj;
   savageTextureObjectPtr t;
   GLuint format;
   Reg_TexCtrl           TexCtrl;
   Reg_TexBlendCtrl      TexBlendCtrl;
   Reg_TexDescr          TexDescr;


   /* disable */
   if(imesa->bTexEn1)
   {
       imesa->bTexEn1 = GL_FALSE;
       return;
   }

   if (ctx->Texture.Unit[1]._ReallyEnabled == 0) {
      imesa->Registers.TexDescr.s4.tex1En = GL_FALSE;
      imesa->Registers.TexBlendCtrl[1].ui = TBC_NoTexMap1;
      imesa->Registers.TexCtrl[1].ui = 0x20f040;
      imesa->Registers.TexAddr[1].ui = 0;
      imesa->Registers.TexDescr.s4.texBLoopEn = GL_FALSE;
      imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTexDescrChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
      imesa->Registers.changed.ni.fTex1AddrChanged = GL_TRUE;
      return;
   }

   tObj = ctx->Texture.Unit[1]._Current;

   if (ctx->Texture.Unit[1]._ReallyEnabled != TEXTURE_2D_BIT ||
       tObj->Image[0][tObj->BaseLevel]->Border > 0) {
      /* 1D or 3D texturing enabled, or texture border - fallback */
      FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
      return;
   }

   /* Do 2D texture setup */

   t = tObj->DriverData;
   if (!t) {
      t = savageAllocTexObj( tObj );
      if (!t)
         return;
   }
    
   if (t->current_unit != 1)
      savageTexSetUnit( t, 1 );

   imesa->CurrentTexObj[1] = t;

   t->bound = 2;

   if (t->dirty_images) {
       savageSetTexImages(imesa, tObj);
       savageUploadTexImages(imesa, imesa->CurrentTexObj[1]);
   }
   
   if (t->MemBlock)
      savageUpdateTexLRU( imesa, t );

   TexDescr.ui = imesa->Registers.TexDescr.ui;
   TexCtrl.ui = imesa->Registers.TexCtrl[1].ui;
   TexBlendCtrl.ui = imesa->Registers.TexBlendCtrl[1].ui;

   format = tObj->Image[0][tObj->BaseLevel]->Format;

   switch (ctx->Texture.Unit[1].EnvMode) {
   case GL_REPLACE:
        TexCtrl.s4.clrArg1Invert = GL_FALSE;
        switch (format)
        {
            case GL_LUMINANCE:
            case GL_RGB:
                TexBlendCtrl.ui = TBC_Decal;
                break;

            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
            case GL_RGBA:
                TexBlendCtrl.ui = TBC_Copy;
                break;

            case GL_ALPHA:
                TexBlendCtrl.ui = TBC_CopyAlpha1;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &TexBlendCtrl);
      break;
   case GL_MODULATE:
       TexCtrl.s4.clrArg1Invert = GL_FALSE;
       TexBlendCtrl.ui = TBC_ModulAlpha1;
       __HWEnvCombineSingleUnitScale(imesa, 0, 1, &TexBlendCtrl);
       break;

/*#if GL_EXT_texture_env_add*/
    case GL_ADD:
        TexCtrl.s4.clrArg1Invert = GL_FALSE;
        TexBlendCtrl.ui = TBC_AddAlpha1;
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &TexBlendCtrl);
        break;
/*#endif*/

#if GL_ARB_texture_env_combine
    case GL_COMBINE_ARB:
        __HWParseTexEnvCombine(imesa, 1, &TexCtrl, &TexBlendCtrl);
        break;
#endif

   case GL_DECAL:
        TexCtrl.s4.clrArg1Invert = GL_FALSE;

        switch (format)
        {
            case GL_LUMINANCE:
            case GL_RGB:
                TexBlendCtrl.ui = TBC_Decal1;
                break;
            case GL_LUMINANCE_ALPHA:
            case GL_INTENSITY:
            case GL_RGBA:
                TexBlendCtrl.ui = TBC_DecalAlpha1;
                break;

                /*
                // GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_ALPHA, GL_INTENSITY
                // are undefined with GL_DECAL
                */
            case GL_ALPHA:
                TexBlendCtrl.ui = TBC_CopyAlpha1;
                break;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &TexBlendCtrl);
        break;

   case GL_BLEND:
        if (format == GL_LUMINANCE)
        {
            /*
            // This is a hack for GLQuake, invert.
            */
            TexCtrl.s4.clrArg1Invert = GL_TRUE;
            TexBlendCtrl.ui = 0;
        }
        __HWEnvCombineSingleUnitScale(imesa, 0, 1, &TexBlendCtrl);
      break;

   default:
      fprintf(stderr, "unkown tex 1 env mode\n");
      exit(1);
      break;			
   }

    TexCtrl.s4.uMode = !(t->texParams.sWrapMode & 0x01);
    TexCtrl.s4.vMode = !(t->texParams.tWrapMode & 0x01);

    switch (t->texParams.minFilter)
    {
        case GL_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Point;
            TexCtrl.s4.mipmapEnable = GL_FALSE;
            break;

        case GL_LINEAR:
            TexCtrl.s4.filterMode   = TFM_Bilin;
            TexCtrl.s4.mipmapEnable = GL_FALSE;
            break;

        case GL_NEAREST_MIPMAP_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Point;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;

        case GL_LINEAR_MIPMAP_NEAREST:
            TexCtrl.s4.filterMode   = TFM_Bilin;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;

        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_LINEAR:
            TexCtrl.s4.filterMode   = TFM_Trilin;
            TexCtrl.s4.mipmapEnable = GL_TRUE;
            break;
    }
    
    if((ctx->Texture.Unit[1].LodBias !=0.0F)&&(TexCtrl.s4.dBias != 0))
    {
	union {
	    GLfloat f;
	    GLint i;
	} bias;
    	GLuint  ul;
    	
    	bias.f = ctx->Texture.Unit[1].LodBias;
    	
    	/* if the value is >= 15.9375 determine whether >= 16
    	   or <0
    	*/
    	if(((bias.i) & 0x7FFFFFFF) >= 0x417F0000)
    	{
    	    if((bias.i) & 0x80000000)
    	    {
    	        ul=0x101;
    	    }
    	    else
    	    {
    	        ul=0xff;
    	    }
    	}
    	else
    	{
            ul=(GLuint)(bias.f*16.0);
        }
        
        ul &= 0x1FF;
        TexCtrl.s4.dBias = ul;
    }

    TexDescr.s4.tex1En = GL_TRUE;
    TexDescr.s4.tex1Width  = t->image[0].image->WidthLog2;
    TexDescr.s4.tex1Height = t->image[0].image->HeightLog2;
    TexDescr.s4.tex1Fmt = t->image[0].internalFormat;
    TexCtrl.s4.dMax = t->max_level;
    TexDescr.s4.texBLoopEn = GL_TRUE;

    if (imesa->Registers.TexAddr[1].ui != (GLuint)t->texParams.hwPhysAddress)
    {
        imesa->Registers.TexAddr[1].ui = (GLuint) t->texParams.hwPhysAddress| 2;
        
        if(t->heap == SAVAGE_AGP_HEAP)
           imesa->Registers.TexAddr[1].ui |= 0x1;
        
        /*imesa->Registers.TexAddr[1].ui = (GLuint) t->texParams.hwPhysAddress| 3;*/
        imesa->Registers.changed.ni.fTex1AddrChanged = GL_TRUE;
    }

    if (imesa->Registers.TexCtrl[1].ui != TexCtrl.ui)
    {
        imesa->Registers.TexCtrl[1].ui = TexCtrl.ui;
        imesa->Registers.changed.ni.fTex1CtrlChanged = GL_TRUE;
    }

    if (imesa->Registers.TexBlendCtrl[1].ui != TexBlendCtrl.ui)
    {
        imesa->Registers.TexBlendCtrl[1].ui = TexBlendCtrl.ui;
        imesa->Registers.changed.ni.fTex1BlendCtrlChanged = GL_TRUE;
    }

    if (imesa->Registers.TexDescr.ui != TexDescr.ui)
    {
        imesa->Registers.TexDescr.ui = TexDescr.ui;
        imesa->Registers.changed.ni.fTexDescrChanged = GL_TRUE;
    }

}
static void savageUpdateTexState_s3d( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    struct gl_texture_object *tObj;
    savageTextureObjectPtr t;
    GLuint format;
    Reg_TexCtrl TexCtrl;
    Reg_DrawCtrl DrawCtrl;
    Reg_TexDescr TexDescr; 

    /* disable */
    if (ctx->Texture.Unit[0]._ReallyEnabled == 0) {
	imesa->Registers.TexCtrl[0].ui = 0;
	imesa->Registers.TexCtrl[0].s3d.texEn = GL_FALSE;
	imesa->Registers.TexCtrl[0].s3d.dBias = 0x08;
	imesa->Registers.TexCtrl[0].s3d.texXprEn = GL_TRUE;
	imesa->Registers.TexAddr[0].ui = 0;
	imesa->Registers.changed.ni.fTex0AddrChanged = GL_TRUE;
	imesa->Registers.changed.ni.fTex0CtrlChanged = GL_TRUE;
	return;
    }

    tObj = ctx->Texture.Unit[0]._Current;
    if (ctx->Texture.Unit[0]._ReallyEnabled != TEXTURE_2D_BIT ||
	tObj->Image[0][tObj->BaseLevel]->Border > 0) {
	/* 1D or 3D texturing enabled, or texture border - fallback */
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
	return;
    }

    /* Do 2D texture setup */
    t = tObj->DriverData;
    if (!t) {
	t = savageAllocTexObj( tObj );
	if (!t)
	    return;
    }

    if (t->current_unit != 0)
	savageTexSetUnit( t, 0 );

    imesa->CurrentTexObj[0] = t;
    t->bound = 1;

    if (t->dirty_images) {
	savageSetTexImages(imesa, tObj);
	savageUploadTexImages(imesa, imesa->CurrentTexObj[0]); 
    }

    if (t->MemBlock)
	savageUpdateTexLRU( imesa, t );

    TexDescr.ui = imesa->Registers.TexDescr.ui;
    TexCtrl.ui = imesa->Registers.TexCtrl[0].ui;
    DrawCtrl.ui = imesa->Registers.DrawCtrl.ui;

    format = tObj->Image[0][tObj->BaseLevel]->Format;

    /* FIXME: copied from utah-glx, probably needs some tuning */
    switch (ctx->Texture.Unit[0].EnvMode) {
    case GL_DECAL:
	DrawCtrl.ni.texBlendCtrl = SAVAGETBC_DECAL_S3D;
	break;
    case GL_REPLACE:
	DrawCtrl.ni.texBlendCtrl = SAVAGETBC_COPY_S3D;
	break;
    case GL_BLEND: /* FIXIT */
    case GL_MODULATE:
	DrawCtrl.ni.texBlendCtrl = SAVAGETBC_MODULATEALPHA_S3D;
	break;
    default:
	fprintf(stderr, "unkown tex env mode\n");
	/*exit(1);*/
	break;			
    }

    DrawCtrl.ni.flushPdDestWrites = GL_TRUE;
    DrawCtrl.ni.flushPdZbufWrites = GL_TRUE;

    /* FIXME: this is how the utah-driver works. I doubt it's the ultimate 
       truth. */
    TexCtrl.s3d.uWrapEn = 0;
    TexCtrl.s3d.vWrapEn = 0;
    if (t->texParams.sWrapMode == GL_CLAMP)
	TexCtrl.s3d.wrapMode = TAM_Clamp;
    else
	TexCtrl.s3d.wrapMode = TAM_Wrap;

    switch (t->texParams.minFilter) {
    case GL_NEAREST:
	TexCtrl.s3d.filterMode    = TFM_Point;
	TexCtrl.s3d.mipmapDisable = GL_TRUE;
	break;

    case GL_LINEAR:
	TexCtrl.s3d.filterMode    = TFM_Bilin;
	TexCtrl.s3d.mipmapDisable = GL_TRUE;
	break;

    case GL_NEAREST_MIPMAP_NEAREST:
	TexCtrl.s3d.filterMode    = TFM_Point;
	TexCtrl.s3d.mipmapDisable = GL_FALSE;
	break;

    case GL_LINEAR_MIPMAP_NEAREST:
	TexCtrl.s3d.filterMode    = TFM_Bilin;
	TexCtrl.s3d.mipmapDisable = GL_FALSE;
	break;

    case GL_NEAREST_MIPMAP_LINEAR:
    case GL_LINEAR_MIPMAP_LINEAR:
	TexCtrl.s3d.filterMode    = TFM_Trilin;
	TexCtrl.s3d.mipmapDisable = GL_FALSE;
	break;
    }

    /* There is no way to specify a maximum mipmap level. We may have to
       disable mipmapping completely. */
    /*
    if (t->max_level < t->image[0].image->WidthLog2 ||
	t->max_level < t->image[0].image->HeightLog2) {
	TexCtrl.s3d.mipmapEnable = GL_TRUE;
	if (TexCtrl.s3d.filterMode == TFM_Trilin)
	    TexCtrl.s3d.filterMode = TFM_Bilin;
	TexCtrl.s3d.filterMode = TFM_Point;
    }
    */

    /* LOD bias makes corruption of small mipmap levels worse on Savage IX
     * but doesn't show the desired effect with the lodbias mesa demo. */
    TexCtrl.s3d.dBias = 0;

    TexCtrl.s3d.texEn = GL_TRUE;
    TexDescr.s3d.texWidth  = t->image[0].image->WidthLog2;
    TexDescr.s3d.texHeight = t->image[0].image->HeightLog2;
    assert (t->image[0].internalFormat <= 7);
    TexDescr.s3d.texFmt = t->image[0].internalFormat;

    if (imesa->Registers.TexAddr[0].ni.addr != (GLuint)t->texParams.hwPhysAddress >> 3)
    {
        imesa->Registers.TexAddr[0].ni.addr = (GLuint) t->texParams.hwPhysAddress >> 3;

        if(t->heap == SAVAGE_AGP_HEAP) {
            imesa->Registers.TexAddr[0].ni.inSysTex = 1;
            imesa->Registers.TexAddr[0].ni.inAGPTex = 1;
	} else {
            imesa->Registers.TexAddr[0].ni.inSysTex = 0;
            imesa->Registers.TexAddr[0].ni.inAGPTex = 1;
	}

        imesa->Registers.changed.ni.fTex0AddrChanged = GL_TRUE;
    }

    if (imesa->Registers.TexCtrl[0].ui != TexCtrl.ui)
    {
        imesa->Registers.TexCtrl[0].ui = TexCtrl.ui;
        imesa->Registers.changed.ni.fTex0CtrlChanged = GL_TRUE;
    }

    if (imesa->Registers.TexDescr.ui != TexDescr.ui)
    {
        imesa->Registers.TexDescr.ui = TexDescr.ui;
        imesa->Registers.changed.ni.fTexDescrChanged = GL_TRUE;
    }

    if (imesa->Registers.DrawCtrl.ui != DrawCtrl.ui)
    {
        imesa->Registers.DrawCtrl.ui = DrawCtrl.ui;
        imesa->Registers.changed.ni.fDrawCtrlChanged = GL_TRUE;
    }
}



static void savageUpdateTextureState_s4( GLcontext *ctx )
{
   savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
   if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->bound = 0;
   if (imesa->CurrentTexObj[1]) imesa->CurrentTexObj[1]->bound = 0;
   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;   
   FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_FALSE);
   savageUpdateTex0State_s4( ctx );
   savageUpdateTex1State_s4( ctx );
   imesa->dirty |= (SAVAGE_UPLOAD_CTX |
		    SAVAGE_UPLOAD_TEX0 | 
		    SAVAGE_UPLOAD_TEX1);
}
static void savageUpdateTextureState_s3d( GLcontext *ctx )
{
    savageContextPtr imesa = SAVAGE_CONTEXT(ctx);
    if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->bound = 0;
    imesa->CurrentTexObj[0] = 0;
    if (ctx->Texture.Unit[1]._ReallyEnabled) {
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_TRUE);
    } else {
	FALLBACK (ctx, SAVAGE_FALLBACK_TEXTURE, GL_FALSE);
	savageUpdateTexState_s3d( ctx );
	imesa->dirty |= (SAVAGE_UPLOAD_CTX |
			 SAVAGE_UPLOAD_TEX0);
    }
}
static void savageUpdateTextureState_first( GLcontext *ctx)
{
    savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
    if (imesa->savageScreen->chipset <= S3_SAVAGE4) {
	GetTiledCoordinates16 = GetTiledCoordinates16_4;
	GetTiledCoordinates32 = GetTiledCoordinates32_4;
    } else {
	GetTiledCoordinates16 = GetTiledCoordinates16_8;
	GetTiledCoordinates32 = GetTiledCoordinates32_8;
    }
    if (imesa->savageScreen->chipset >= S3_SAVAGE4)
	savageUpdateTextureState = savageUpdateTextureState_s4;
    else
	savageUpdateTextureState = savageUpdateTextureState_s3d;
    savageUpdateTextureState (ctx);
}
void (*savageUpdateTextureState)( GLcontext *ctx ) =
    savageUpdateTextureState_first;



/*****************************************
 * DRIVER functions
 *****************************************/

static void savageTexEnv( GLcontext *ctx, GLenum target, 
			GLenum pname, const GLfloat *param )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (pname == GL_TEXTURE_ENV_MODE) {

      imesa->new_state |= SAVAGE_NEW_TEXTURE;

   } else if (pname == GL_TEXTURE_ENV_COLOR) {

      struct gl_texture_unit *texUnit = 
	 &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      const GLfloat *fc = texUnit->EnvColor;
      GLuint r, g, b, a, col;
      CLAMPED_FLOAT_TO_UBYTE(r, fc[0]);
      CLAMPED_FLOAT_TO_UBYTE(g, fc[1]);
      CLAMPED_FLOAT_TO_UBYTE(b, fc[2]);
      CLAMPED_FLOAT_TO_UBYTE(a, fc[3]);

      col = ((a << 24) | 
	     (r << 16) | 
	     (g <<  8) | 
	     (b <<  0));
    

   } 
}

static void savageTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			      GLint internalFormat,
			      GLint width, GLint height, GLint border,
			      GLenum format, GLenum type, const GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage )
{
   savageTextureObjectPtr t = (savageTextureObjectPtr) texObj->DriverData;
   if (t) {
      savageSwapOutTexObj( SAVAGE_CONTEXT(ctx), t );
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   _mesa_store_teximage2d( ctx, target, level, internalFormat,
			   width, height, border, format, type,
			   pixels, packing, texObj, texImage );
   t->dirty_images |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexSubImage2D( GLcontext *ctx, 
				 GLenum target,
				 GLint level,	
				 GLint xoffset, GLint yoffset,
				 GLsizei width, GLsizei height,
				 GLenum format, GLenum type,
				 const GLvoid *pixels,
				 const struct gl_pixelstore_attrib *packing,
				 struct gl_texture_object *texObj,
				 struct gl_texture_image *texImage )
{
   savageTextureObjectPtr t = (savageTextureObjectPtr) texObj->DriverData;
   assert( t ); /* this _should_ be true */
   if (t) {
      savageSwapOutTexObj( SAVAGE_CONTEXT(ctx), t );
   } else {
      t = savageAllocTexObj(texObj);
      if (!t) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
         return;
      }
   }
   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width, 
			     height, format, type, pixels, packing, texObj,
			     texImage);
   t->dirty_images |= (1 << level);
   SAVAGE_CONTEXT(ctx)->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageTexParameter( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj,
			      GLenum pname, const GLfloat *params )
{
   savageTextureObjectPtr t = (savageTextureObjectPtr) tObj->DriverData;
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (!t || target != GL_TEXTURE_2D)
      return;

   switch (pname) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      savageSetTexFilter(t,tObj->MinFilter,tObj->MagFilter);
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      savageSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
      break;
  
   case GL_TEXTURE_BORDER_COLOR:
      savageSetTexBorderColor(t,tObj->_BorderChan);
      break;

   default:
      return;
   }

   imesa->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageBindTexture( GLcontext *ctx, GLenum target,
			       struct gl_texture_object *tObj )
{
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );
   

   if (imesa->CurrentTexObj[ctx->Texture.CurrentUnit]) {
      imesa->CurrentTexObj[ctx->Texture.CurrentUnit]->bound = 0;
      imesa->CurrentTexObj[ctx->Texture.CurrentUnit] = 0;  
   }

   assert( (target != GL_TEXTURE_2D) || (tObj->DriverData != NULL) );

   imesa->new_state |= SAVAGE_NEW_TEXTURE;
}

static void savageDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj )
{
   savageTextureObjectPtr t = (savageTextureObjectPtr)tObj->DriverData;
   savageContextPtr imesa = SAVAGE_CONTEXT( ctx );

   if (t) {

      if (t->bound) {
	 imesa->CurrentTexObj[t->bound-1] = 0;
	 imesa->new_state |= SAVAGE_NEW_TEXTURE;
      }

      savageDestroyTexObj(imesa,t);
      tObj->DriverData=0;
   }
   /* Free mipmap images and the texture object itself */
   _mesa_delete_texture_object(ctx, tObj);
}


static GLboolean savageIsTextureResident( GLcontext *ctx, 
					struct gl_texture_object *t )
{
   savageTextureObjectPtr mt;

/*     LOCK_HARDWARE; */
   mt = (savageTextureObjectPtr)t->DriverData;
/*     UNLOCK_HARDWARE; */

   return mt && mt->MemBlock;
}


static struct gl_texture_object *
savageNewTextureObject( GLcontext *ctx, GLuint name, GLenum target )
{
    struct gl_texture_object *obj;
    obj = _mesa_new_texture_object(ctx, name, target);
    savageAllocTexObj( obj );

    return obj;
}

void savageDDInitTextureFuncs( struct dd_function_table *functions )
{
   functions->TexEnv = savageTexEnv;
   functions->ChooseTextureFormat = savageChooseTextureFormat;
   functions->TexImage2D = savageTexImage2D;
   functions->TexSubImage2D = savageTexSubImage2D;
   functions->BindTexture = savageBindTexture;
   functions->NewTextureObject = savageNewTextureObject;
   functions->DeleteTexture = savageDeleteTexture;
   functions->IsTextureResident = savageIsTextureResident;
   functions->TexParameter = savageTexParameter;
}
