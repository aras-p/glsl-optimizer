/* $Id: teximage.c,v 1.8 1999/10/22 10:43:35 brianp Exp $ */

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


/* $XFree86: xc/lib/GL/mesa/src/teximage.c,v 1.3 1999/04/04 00:20:32 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "image.h"
#include "macros.h"
#include "mmath.h"
#include "span.h"
#include "teximage.h"
#include "texstate.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


/*
 * NOTES:
 *
 * The internal texture storage convension is an array of N GLubytes
 * where N = width * height * components.  There is no padding.
 */




/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n<0 return -1.
 */
static int logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n<0) {
      return -1;
   }

   while ( n > i ) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return -1;
   }
   else {
      return log2;
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return -1 if
 * invalid enum.
 */
static GLint decode_internal_format( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return GL_COLOR_INDEX;
      default:
         return -1;  /* error */
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return the
 * number of components for the format.  Return -1 if invalid enum.
 */
static GLint components_in_intformat( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return 1;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return 1;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return 2;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return 1;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return 3;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return 4;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return 1;
      default:
         return -1;  /* error */
   }
}



struct gl_texture_image *gl_alloc_texture_image( void )
{
   return CALLOC_STRUCT(gl_texture_image);
}



void gl_free_texture_image( struct gl_texture_image *teximage )
{
   if (teximage->Data) {
      FREE( teximage->Data );
      teximage->Data = NULL;
   }
   FREE( teximage );
}



/*
 * Examine the texImage->Format field and set the Red, Green, Blue, etc
 * texel component sizes to default values.
 * These fields are set only here by core Mesa but device drivers may
 * overwritting these fields to indicate true texel resolution.
 */
static void set_teximage_component_sizes( struct gl_texture_image *texImage )
{
   switch (texImage->Format) {
      case GL_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8;
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8;
         texImage->IndexBits = 0;
         break;
      case GL_INTENSITY:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 8;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RED:
         texImage->RedBits = 8;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_GREEN:
         texImage->RedBits = 0;
         texImage->GreenBits = 8;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_BLUE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGB:
      case GL_BGR:
         texImage->RedBits = 8;
         texImage->GreenBits = 8;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
         texImage->RedBits = 8;
         texImage->GreenBits = 8;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_COLOR_INDEX:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 8;
         break;
      default:
         gl_problem(NULL, "unexpected format in set_teximage_component_sizes");
   }
}


/* Need this to prevent an out-of-bounds memory access when using
 * X86 optimized code.
 */
#ifdef USE_X86_ASM
#  define EXTRA_BYTE 1
#else
#  define EXTRA_BYTE 0
#endif


/*
 * Given a gl_image, apply the pixel transfer scale, bias, and mapping
 * to produce a gl_texture_image.  Convert image data to GLubytes.
 * Input:  image - the incoming gl_image
 *         internalFormat - desired format of resultant texture
 *         border - texture border width (0 or 1)
 * Return:  pointer to a gl_texture_image or NULL if an error occurs.
 */
static struct gl_texture_image *
image_to_texture( GLcontext *ctx, const struct gl_image *image,
                  GLint internalFormat, GLint border )
{
   GLint components;
   struct gl_texture_image *texImage;
   GLint numPixels, pixel;
   GLboolean scaleOrBias;

   assert(image);
   assert(image->Width>0);
   assert(image->Height>0);
   assert(image->Depth>0);

   /*   internalFormat = decode_internal_format(internalFormat);*/
   components = components_in_intformat(internalFormat);
   numPixels = image->Width * image->Height * image->Depth;

   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = (GLenum) decode_internal_format(internalFormat);
   set_teximage_component_sizes( texImage );
   texImage->IntFormat = (GLenum) internalFormat;
   texImage->Border = border;
   texImage->Width = image->Width;
   texImage->Height = image->Height;
   texImage->Depth = image->Depth;
   texImage->WidthLog2 = logbase2(image->Width - 2*border);
   if (image->Height==1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(image->Height - 2*border);
   if (image->Depth==1)   /* 2-D texture */
      texImage->DepthLog2 = 0;
   else
      texImage->DepthLog2 = logbase2(image->Depth - 2*border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->Depth2 = 1 << texImage->DepthLog2;
   texImage->MaxLog2 = MAX2( texImage->WidthLog2, texImage->HeightLog2 );
   texImage->Data = (GLubyte *) MALLOC( numPixels * components + EXTRA_BYTE );

   if (!texImage->Data) {
      /* out of memory */
      gl_free_texture_image( texImage );
      return NULL;
   }

   /* Determine if scaling and/or biasing is needed */
   if (ctx->Pixel.RedScale!=1.0F   || ctx->Pixel.RedBias!=0.0F ||
       ctx->Pixel.GreenScale!=1.0F || ctx->Pixel.GreenBias!=0.0F ||
       ctx->Pixel.BlueScale!=1.0F  || ctx->Pixel.BlueBias!=0.0F ||
       ctx->Pixel.AlphaScale!=1.0F || ctx->Pixel.AlphaBias!=0.0F) {
      scaleOrBias = GL_TRUE;
   }
   else {
      scaleOrBias = GL_FALSE;
   }

   switch (image->Type) {
      case GL_BITMAP:
         {
            GLint shift = ctx->Pixel.IndexShift;
            GLint offset = ctx->Pixel.IndexOffset;
            /* MapIto[RGBA]Size must be powers of two */
            GLint rMask = ctx->Pixel.MapItoRsize-1;
            GLint gMask = ctx->Pixel.MapItoGsize-1;
            GLint bMask = ctx->Pixel.MapItoBsize-1;
            GLint aMask = ctx->Pixel.MapItoAsize-1;
            GLint i, j;
            GLubyte *srcPtr = (GLubyte *) image->Data;

            assert( image->Format==GL_COLOR_INDEX );

            for (j=0; j<image->Height; j++) {
               GLubyte bitMask = 128;
               for (i=0; i<image->Width; i++) {
                  GLint index;
                  GLubyte red, green, blue, alpha;

                  /* Fetch image color index */
                  index = (*srcPtr & bitMask) ? 1 : 0;
                  bitMask = bitMask >> 1;
                  if (bitMask==0) {
                     bitMask = 128;
                     srcPtr++;
                  }
                  /* apply index shift and offset */
                  if (shift>=0) {
                     index = (index << shift) + offset;
                  }
                  else {
                     index = (index >> -shift) + offset;
                  }
                  /* convert index to RGBA */
                  red   = (GLint) (ctx->Pixel.MapItoR[index & rMask] * 255.0F);
                  green = (GLint) (ctx->Pixel.MapItoG[index & gMask] * 255.0F);
                  blue  = (GLint) (ctx->Pixel.MapItoB[index & bMask] * 255.0F);
                  alpha = (GLint) (ctx->Pixel.MapItoA[index & aMask] * 255.0F);

                  /* store texel (components are GLubytes in [0,255]) */
                  pixel = j * image->Width + i;
                  switch (texImage->Format) {
                     case GL_ALPHA:
                        texImage->Data[pixel] = alpha;
                        break;
                     case GL_LUMINANCE:
                        texImage->Data[pixel] = red;
                        break;
                     case GL_LUMINANCE_ALPHA:
                        texImage->Data[pixel*2+0] = red;
                        texImage->Data[pixel*2+1] = alpha;
                        break;
                     case GL_INTENSITY:
                        texImage->Data[pixel] = red;
                        break;
                     case GL_RGB:
                        texImage->Data[pixel*3+0] = red;
                        texImage->Data[pixel*3+1] = green;
                        texImage->Data[pixel*3+2] = blue;
                        break;
                     case GL_RGBA:
                        texImage->Data[pixel*4+0] = red;
                        texImage->Data[pixel*4+1] = green;
                        texImage->Data[pixel*4+2] = blue;
                        texImage->Data[pixel*4+3] = alpha;
                        break;
                     default:
                        gl_problem(ctx,"Bad format in image_to_texture");
                        return NULL;
                  }
               }
               if (bitMask!=128) {
                  srcPtr++;
               }
            }
         }
         break;

      case GL_UNSIGNED_BYTE:
         if (image->Format == texImage->Format && !scaleOrBias && !ctx->Pixel.MapColorFlag) {
            switch (image->Format) {
               case GL_COLOR_INDEX:
                  if (decode_internal_format(internalFormat)!=GL_COLOR_INDEX) {
                     /* convert color index to RGBA */
                     for (pixel=0; pixel<numPixels; pixel++) {
                        GLint index = ((GLubyte*)image->Data)[pixel];
                        index = (GLint) (255.0F * ctx->Pixel.MapItoR[index]);
                        texImage->Data[pixel] = index;
                     }
                     numPixels = 0;
                     break;
                  }
               case GL_ALPHA:
               case GL_LUMINANCE:
               case GL_INTENSITY:
                  MEMCPY(texImage->Data, image->Data, numPixels * 1);
                  numPixels = 0;
                  break;
               case GL_LUMINANCE_ALPHA:
                  MEMCPY(texImage->Data, image->Data, numPixels * 2);
                  numPixels = 0;
                  break;
               case GL_RGB:
                  MEMCPY(texImage->Data, image->Data, numPixels * 3);
                  numPixels = 0;
                  break;
               case GL_RGBA:
                  MEMCPY(texImage->Data, image->Data, numPixels * 4);
                  numPixels = 0;
                  break;
               default:
                  break;
            }
         }
         for (pixel=0; pixel<numPixels; pixel++) {
            GLubyte red, green, blue, alpha;
            switch (image->Format) {
               case GL_COLOR_INDEX:
                  if (decode_internal_format(internalFormat)==GL_COLOR_INDEX) {
                     /* a paletted texture */
                     GLint index = ((GLubyte*)image->Data)[pixel];
                     red = index;
                     green = blue = alpha = 0;  /* silence compiler warnings */
                  }
                  else {
                     /* convert color index to RGBA */
                     GLint index = ((GLubyte*)image->Data)[pixel];
                     red   = (GLint) (255.0F * ctx->Pixel.MapItoR[index]);
                     green = (GLint) (255.0F * ctx->Pixel.MapItoG[index]);
                     blue  = (GLint) (255.0F * ctx->Pixel.MapItoB[index]);
                     alpha = (GLint) (255.0F * ctx->Pixel.MapItoA[index]);
                  }
                  break;
               case GL_RGB:
                  /* Fetch image RGBA values */
                  red   = ((GLubyte*) image->Data)[pixel*3+0];
                  green = ((GLubyte*) image->Data)[pixel*3+1];
                  blue  = ((GLubyte*) image->Data)[pixel*3+2];
                  alpha = 255;
                  break;
               case GL_RGBA:
                  red   = ((GLubyte*) image->Data)[pixel*4+0];
                  green = ((GLubyte*) image->Data)[pixel*4+1];
                  blue  = ((GLubyte*) image->Data)[pixel*4+2];
                  alpha = ((GLubyte*) image->Data)[pixel*4+3];
                  break;
               case GL_RED:
                  red   = ((GLubyte*) image->Data)[pixel];
                  green = 0;
                  blue  = 0;
                  alpha = 255;
                  break;
               case GL_GREEN:
                  red   = 0;
                  green = ((GLubyte*) image->Data)[pixel];
                  blue  = 0;
                  alpha = 255;
                  break;
               case GL_BLUE:
                  red   = 0;
                  green = 0;
                  blue  = ((GLubyte*) image->Data)[pixel];
                  alpha = 255;
                  break;
               case GL_ALPHA:
                  red   = 0;
                  green = 0;
                  blue  = 0;
                  alpha = ((GLubyte*) image->Data)[pixel];
                  break;
               case GL_LUMINANCE: 
                  red   = ((GLubyte*) image->Data)[pixel];
                  green = red;
                  blue  = red;
                  alpha = 255;
                  break;
              case GL_LUMINANCE_ALPHA:
                  red   = ((GLubyte*) image->Data)[pixel*2+0];
                  green = red;
                  blue  = red;
                  alpha = ((GLubyte*) image->Data)[pixel*2+1];
                  break;
              default:
                 red = green = blue = alpha = 0;
                 gl_problem(ctx,"Bad format (2) in image_to_texture");
                 return NULL;
            }
            
            if (scaleOrBias || ctx->Pixel.MapColorFlag) {
               /* Apply RGBA scale and bias */
               GLfloat r = UBYTE_COLOR_TO_FLOAT_COLOR(red);
               GLfloat g = UBYTE_COLOR_TO_FLOAT_COLOR(green);
               GLfloat b = UBYTE_COLOR_TO_FLOAT_COLOR(blue);
               GLfloat a = UBYTE_COLOR_TO_FLOAT_COLOR(alpha);
               if (scaleOrBias) {
                  /* r,g,b,a now in [0,1] */
                  r = r * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
                  g = g * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
                  b = b * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
                  a = a * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
                  r = CLAMP( r, 0.0F, 1.0F );
                  g = CLAMP( g, 0.0F, 1.0F );
                  b = CLAMP( b, 0.0F, 1.0F );
                  a = CLAMP( a, 0.0F, 1.0F );
               }
               /* Apply pixel maps */
               if (ctx->Pixel.MapColorFlag) {
                  GLint ir = (GLint) (r*ctx->Pixel.MapRtoRsize);
                  GLint ig = (GLint) (g*ctx->Pixel.MapGtoGsize);
                  GLint ib = (GLint) (b*ctx->Pixel.MapBtoBsize);
                  GLint ia = (GLint) (a*ctx->Pixel.MapAtoAsize);
                  r = ctx->Pixel.MapRtoR[ir];
                  g = ctx->Pixel.MapGtoG[ig];
                  b = ctx->Pixel.MapBtoB[ib];
                  a = ctx->Pixel.MapAtoA[ia];
               }
               red   = (GLint) (r * 255.0F);
               green = (GLint) (g * 255.0F);
               blue  = (GLint) (b * 255.0F);
               alpha = (GLint) (a * 255.0F);
            }

            /* store texel (components are GLubytes in [0,255]) */
            switch (texImage->Format) {
               case GL_COLOR_INDEX:
                  texImage->Data[pixel] = red; /* really an index */
                  break;
               case GL_ALPHA:
                  texImage->Data[pixel] = alpha;
                  break;
               case GL_LUMINANCE:
                  texImage->Data[pixel] = red;
                  break;
               case GL_LUMINANCE_ALPHA:
                  texImage->Data[pixel*2+0] = red;
                  texImage->Data[pixel*2+1] = alpha;
                  break;
               case GL_INTENSITY:
                  texImage->Data[pixel] = red;
                  break;
               case GL_RGB:
                  texImage->Data[pixel*3+0] = red;
                  texImage->Data[pixel*3+1] = green;
                  texImage->Data[pixel*3+2] = blue;
                  break;
               case GL_RGBA:
                  texImage->Data[pixel*4+0] = red;
                  texImage->Data[pixel*4+1] = green;
                  texImage->Data[pixel*4+2] = blue;
                  texImage->Data[pixel*4+3] = alpha;
                  break;
               default:
                  gl_problem(ctx,"Bad format (3) in image_to_texture");
                  return NULL;
            }
         }
         break;

      case GL_FLOAT:
         for (pixel=0; pixel<numPixels; pixel++) {
            GLfloat red, green, blue, alpha;
            switch (image->Format) {
               case GL_COLOR_INDEX:
                  if (decode_internal_format(internalFormat)==GL_COLOR_INDEX) {
                     /* a paletted texture */
                     GLint index = (GLint) ((GLfloat*) image->Data)[pixel];
                     red = index;
                     green = blue = alpha = 0;  /* silence compiler warning */
                  }
                  else {
                     GLint shift = ctx->Pixel.IndexShift;
                     GLint offset = ctx->Pixel.IndexOffset;
                     /* MapIto[RGBA]Size must be powers of two */
                     GLint rMask = ctx->Pixel.MapItoRsize-1;
                     GLint gMask = ctx->Pixel.MapItoGsize-1;
                     GLint bMask = ctx->Pixel.MapItoBsize-1;
                     GLint aMask = ctx->Pixel.MapItoAsize-1;
                     /* Fetch image color index */
                     GLint index = (GLint) ((GLfloat*) image->Data)[pixel];
                     /* apply index shift and offset */
                     if (shift>=0) {
                        index = (index << shift) + offset;
                     }
                     else {
                        index = (index >> -shift) + offset;
                     }
                     /* convert index to RGBA */
                     red   = ctx->Pixel.MapItoR[index & rMask];
                     green = ctx->Pixel.MapItoG[index & gMask];
                     blue  = ctx->Pixel.MapItoB[index & bMask];
                     alpha = ctx->Pixel.MapItoA[index & aMask];
                  }
                  break;
               case GL_RGB:
                  /* Fetch image RGBA values */
                  red   = ((GLfloat*) image->Data)[pixel*3+0];
                  green = ((GLfloat*) image->Data)[pixel*3+1];
                  blue  = ((GLfloat*) image->Data)[pixel*3+2];
                  alpha = 1.0;
                  break;
               case GL_RGBA:
                  red   = ((GLfloat*) image->Data)[pixel*4+0];
                  green = ((GLfloat*) image->Data)[pixel*4+1];
                  blue  = ((GLfloat*) image->Data)[pixel*4+2];
                  alpha = ((GLfloat*) image->Data)[pixel*4+3];
                  break;
               case GL_RED:
                  red   = ((GLfloat*) image->Data)[pixel];
                  green = 0.0;
                  blue  = 0.0;
                  alpha = 1.0;
                  break;
               case GL_GREEN:
                  red   = 0.0;
                  green = ((GLfloat*) image->Data)[pixel];
                  blue  = 0.0;
                  alpha = 1.0;
                  break;
               case GL_BLUE:
                  red   = 0.0;
                  green = 0.0;
                  blue  = ((GLfloat*) image->Data)[pixel];
                  alpha = 1.0;
                  break;
               case GL_ALPHA:
                  red   = 0.0;
                  green = 0.0;
                  blue  = 0.0;
                  alpha = ((GLfloat*) image->Data)[pixel];
                  break;
               case GL_LUMINANCE: 
                  red   = ((GLfloat*) image->Data)[pixel];
                  green = red;
                  blue  = red;
                  alpha = 1.0;
                  break;
              case GL_LUMINANCE_ALPHA:
                  red   = ((GLfloat*) image->Data)[pixel*2+0];
                  green = red;
                  blue  = red;
                  alpha = ((GLfloat*) image->Data)[pixel*2+1];
                  break;
               default:
                  gl_problem(ctx,"Bad format (4) in image_to_texture");
                  return NULL;
            }
            
            if (image->Format!=GL_COLOR_INDEX) {
               /* Apply RGBA scale and bias */
               if (scaleOrBias) {
                  red   = red   * ctx->Pixel.RedScale   + ctx->Pixel.RedBias;
                  green = green * ctx->Pixel.GreenScale + ctx->Pixel.GreenBias;
                  blue  = blue  * ctx->Pixel.BlueScale  + ctx->Pixel.BlueBias;
                  alpha = alpha * ctx->Pixel.AlphaScale + ctx->Pixel.AlphaBias;
                  red   = CLAMP( red,    0.0F, 1.0F );
                  green = CLAMP( green,  0.0F, 1.0F );
                  blue  = CLAMP( blue,   0.0F, 1.0F );
                  alpha = CLAMP( alpha,  0.0F, 1.0F );
               }
               /* Apply pixel maps */
               if (ctx->Pixel.MapColorFlag) {
                  GLint ir = (GLint) (red  *ctx->Pixel.MapRtoRsize);
                  GLint ig = (GLint) (green*ctx->Pixel.MapGtoGsize);
                  GLint ib = (GLint) (blue *ctx->Pixel.MapBtoBsize);
                  GLint ia = (GLint) (alpha*ctx->Pixel.MapAtoAsize);
                  red   = ctx->Pixel.MapRtoR[ir];
                  green = ctx->Pixel.MapGtoG[ig];
                  blue  = ctx->Pixel.MapBtoB[ib];
                  alpha = ctx->Pixel.MapAtoA[ia];
               }
            }

            /* store texel (components are GLubytes in [0,255]) */
            switch (texImage->Format) {
               case GL_COLOR_INDEX:
                  /* a paletted texture */
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_ALPHA:
                  texImage->Data[pixel] = (GLint) (alpha * 255.0F);
                  break;
               case GL_LUMINANCE:
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_LUMINANCE_ALPHA:
                  texImage->Data[pixel*2+0] = (GLint) (red * 255.0F);
                  texImage->Data[pixel*2+1] = (GLint) (alpha * 255.0F);
                  break;
               case GL_INTENSITY:
                  texImage->Data[pixel] = (GLint) (red * 255.0F);
                  break;
               case GL_RGB:
                  texImage->Data[pixel*3+0] = (GLint) (red   * 255.0F);
                  texImage->Data[pixel*3+1] = (GLint) (green * 255.0F);
                  texImage->Data[pixel*3+2] = (GLint) (blue  * 255.0F);
                  break;
               case GL_RGBA:
                  texImage->Data[pixel*4+0] = (GLint) (red   * 255.0F);
                  texImage->Data[pixel*4+1] = (GLint) (green * 255.0F);
                  texImage->Data[pixel*4+2] = (GLint) (blue  * 255.0F);
                  texImage->Data[pixel*4+3] = (GLint) (alpha * 255.0F);
                  break;
               default:
                  gl_problem(ctx,"Bad format (5) in image_to_texture");
                  return NULL;
            }
         }
         break;

      default:
         gl_problem(ctx, "Bad image type in image_to_texture");
         return NULL;
   }

   return texImage;
}



/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static struct gl_texture_image *
make_null_texture( GLcontext *ctx, GLenum internalFormat,
                   GLsizei width, GLsizei height, GLsizei depth, GLint border )
{
   GLint components;
   struct gl_texture_image *texImage;
   GLint numPixels;
   (void) ctx;

   /*internalFormat = decode_internal_format(internalFormat);*/
   components = components_in_intformat(internalFormat);
   numPixels = width * height * depth;

   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = (GLenum) decode_internal_format(internalFormat);
   set_teximage_component_sizes( texImage );
   texImage->IntFormat = internalFormat;
   texImage->Border = border;
   texImage->Width = width;
   texImage->Height = height;
   texImage->Depth = depth;
   texImage->WidthLog2 = logbase2(width - 2*border);
   if (height==1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(height - 2*border);
   if (depth==1)   /* 2-D texture */
      texImage->DepthLog2 = 0;
   else
      texImage->DepthLog2 = logbase2(depth - 2*border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->Depth2 = 1 << texImage->DepthLog2;
   texImage->MaxLog2 = MAX2( texImage->WidthLog2, texImage->HeightLog2 );

   /* XXX should we really allocate memory for the image or let it be NULL? */
   /*texImage->Data = NULL;*/

   texImage->Data = (GLubyte *) MALLOC( numPixels * components + EXTRA_BYTE );

   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (texImage->Data) {
      char message[8][32] = {
         "   X   X  XXXXX   XXX     X    ",
         "   XX XX  X      X   X   X X   ",
         "   X X X  X      X      X   X  ",
         "   X   X  XXXX    XXX   XXXXX  ",
         "   X   X  X          X  X   X  ",
         "   X   X  X      X   X  X   X  ",
         "   X   X  XXXXX   XXX   X   X  ",
         "                               "
      };

      GLubyte *imgPtr = texImage->Data;
      GLint i, j, k;
      for (i=0;i<height;i++) {
         GLint srcRow = 7 - i % 8;
         for (j=0;j<width;j++) {
            GLint srcCol = j % 32;
            GLubyte texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
            for (k=0;k<components;k++) {
               *imgPtr++ = texel;
            }
         }
      }
   }

   return texImage;
}



/*
 * Test glTexImage() parameters for errors.
 * Input:
 *         dimensions - must be 1 or 2 or 3
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean texture_error_check( GLcontext *ctx, GLenum target,
                                      GLint level, GLint internalFormat,
                                      GLenum format, GLenum type,
                                      GLint dimensions,
                                      GLint width, GLint height,
                                      GLint depth, GLint border )
{
   GLboolean isProxy;
   GLint iformat;

   if (dimensions == 1) {
      isProxy = (target == GL_PROXY_TEXTURE_1D);
      if (target != GL_TEXTURE_1D && !isProxy) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      isProxy = (target == GL_PROXY_TEXTURE_2D);
      if (target != GL_TEXTURE_2D && !isProxy) {
          gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
          return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      isProxy = (target == GL_PROXY_TEXTURE_3D);
      if (target != GL_TEXTURE_3D && !isProxy) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
         return GL_TRUE;
      }
   }
   else {
      gl_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   /* Border */
   if (border!=0 && border!=1) {
      if (!isProxy) {
         if (dimensions == 1)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(border)" );
         else if (dimensions == 2)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(border)" );
         else if (dimensions == 3)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(border)" );
      }
      return GL_TRUE;
   }

   /* Width */
   if (width < 2 * border || width > 2 + ctx->Const.MaxTextureSize
       || logbase2( width - 2 * border ) < 0) {
      if (!isProxy) {
         if (dimensions == 1)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(width)" );
         else if (dimensions == 2)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(width)" );
         else if (dimensions == 3)
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(width)" );
      }
      return GL_TRUE;
   }

   /* Height */
   if (dimensions >= 2) {
      if (height < 2 * border || height > 2 + ctx->Const.MaxTextureSize
          || logbase2( height - 2 * border ) < 0) {
         if (!isProxy) {
            if (dimensions == 2)
               gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(height)" );
            else if (dimensions == 3)
               gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(height)" );
            return GL_TRUE;
         }
      }
   }

   /* Depth */
   if (dimensions >= 3) {
      if (depth < 2 * border || depth > 2 + ctx->Const.MaxTextureSize
          || logbase2( depth - 2 * border ) < 0) {
         if (!isProxy) {
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(depth)" );
         }
         return GL_TRUE;
      }
   }

   /* Level */
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      if (dimensions == 1)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(level)" );
      else if (dimensions == 2)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(level)" );
      else if (dimensions == 3)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(level)" );
      return GL_TRUE;
   }

   iformat = decode_internal_format( internalFormat );
   if (iformat < 0) {
      if (dimensions == 1)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage1D(internalFormat)" );
      else if (dimensions == 2)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage2D(internalFormat)" );
      else if (dimensions == 3)
         gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(internalFormat)" );
      return GL_TRUE;
   }

   if (!gl_is_legal_format_and_type( format, type )) {
      /* Yes, generate GL_INVALID_OPERATION, not GL_INVALID_ENUM, if there
       * is a type/format mismatch.  See 1.2 spec page 94, sec 3.6.4.
       */
      if (dimensions == 1)
         gl_error( ctx, GL_INVALID_OPERATION, "glTexImage1D(format or type)");
      else if (dimensions == 2)
         gl_error( ctx, GL_INVALID_OPERATION, "glTexImage2D(format or type)");
      else if (dimensions == 3)
         gl_error( ctx, GL_INVALID_OPERATION, "glTexImage3D(format or type)");
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}



/*
 * Called from the API.  Note that width includes the border.
 */
void gl_TexImage1D( GLcontext *ctx,
                    GLenum target, GLint level, GLint internalformat,
		    GLsizei width, GLint border, GLenum format,
		    GLenum type, struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage1D");

   if (target==GL_TEXTURE_1D) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 1, width, 1, 1, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[1]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[1]->Image[level] );
      }

      /* make new texture from source image */
      if (image) {
         teximage = image_to_texture(ctx, image, internalformat, border);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, 1, 1, border);
      }

      /* install new texture image */

      texUnit->CurrentD[1]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );
      ctx->NewState |= NEW_TEXTURING;

      /* free the source image */
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
                                  texUnit->CurrentD[1],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 1, width, 1, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy1D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy1D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy1D->Image[level] );
         ctx->Texture.Proxy1D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy1D->Image[level]->Border = border;
         ctx->Texture.Proxy1D->Image[level]->Width = width;
         ctx->Texture.Proxy1D->Image[level]->Height = 1;
         ctx->Texture.Proxy1D->Image[level]->Depth = 1;
      }
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
      return;
   }
}




/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void gl_TexImage2D( GLcontext *ctx,
                    GLenum target, GLint level, GLint internalformat,
                    GLsizei width, GLsizei height, GLint border,
                    GLenum format, GLenum type,
                    struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage2D");

   if (target==GL_TEXTURE_2D) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 2, width, height, 1, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[2]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[2]->Image[level] );
      }

      /* make new texture from source image */
      if (image) {
         teximage = image_to_texture(ctx, image, internalformat, border);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, height, 1, border);
      }

      /* install new texture image */
      texUnit->CurrentD[2]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[2] );
      ctx->NewState |= NEW_TEXTURING;

      /* free the source image */
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D,
                                  texUnit->CurrentD[2],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_2D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 2, width, height, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy2D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy2D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy2D->Image[level] );
         ctx->Texture.Proxy2D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy2D->Image[level]->Border = border;
         ctx->Texture.Proxy2D->Image[level]->Width = width;
         ctx->Texture.Proxy2D->Image[level]->Height = height;
         ctx->Texture.Proxy2D->Image[level]->Depth = 1;
      }
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
      return;
   }
}



/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void gl_TexImage3DEXT( GLcontext *ctx,
                       GLenum target, GLint level, GLint internalformat,
                       GLsizei width, GLsizei height, GLsizei depth,
                       GLint border, GLenum format, GLenum type,
                       struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage3DEXT");

   if (target==GL_TEXTURE_3D_EXT) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 3, width, height, depth,
                               border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[3]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[3]->Image[level] );
      }

      /* make new texture from source image */
      if (image) {
         teximage = image_to_texture(ctx, image, internalformat, border);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, height, depth, border);
      }

      /* install new texture image */
      texUnit->CurrentD[3]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[3] );
      ctx->NewState |= NEW_TEXTURING;

      /* free the source image */
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D_EXT,
                                  texUnit->CurrentD[3],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_3D_EXT) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 3, width, height, depth,
                               border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy3D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy3D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy3D->Image[level] );
         ctx->Texture.Proxy3D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy3D->Image[level]->Border = border;
         ctx->Texture.Proxy3D->Image[level]->Width = width;
         ctx->Texture.Proxy3D->Image[level]->Height = height;
         ctx->Texture.Proxy3D->Image[level]->Depth  = depth;
      }
      if (image && image->RefCount==0) {
         /* if RefCount>0 then image must be in a display list */
         gl_free_image(image);
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage3DEXT(target)" );
      return;
   }
}



void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level, GLenum format,
                     GLenum type, GLvoid *pixels )
{
   const struct gl_texture_object *texObj;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexImage");

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return;
   }

   if (gl_sizeof_type(type) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return;
   }

   if (gl_components_in_format(format) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return;
   }

   if (!pixels)
      return;  /* XXX generate an error??? */

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[1];
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[2];
         break;
      case GL_TEXTURE_3D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[3];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(target)" );
         return;
   }

   if (texObj->Image[level] && texObj->Image[level]->Data) {
      const struct gl_texture_image *texImage = texObj->Image[level];
      GLint width = texImage->Width;
      GLint height = texImage->Height;
      GLint row;

      for (row = 0; row < height; row++) {
         /* compute destination address in client memory */
         GLvoid *dest = gl_pixel_addr_in_image( &ctx->Unpack, pixels,
                                                width, height,
                                                format, type, 0, row, 0);

         assert(dest);
         if (texImage->Format == GL_RGBA) {
            const GLubyte *src = texImage->Data + row * width * 4 * sizeof(GLubyte);
            gl_pack_rgba_span( ctx, width, (void *) src, format, type, dest,
                               &ctx->Pack, GL_TRUE );
         }
         else {
            /* fetch RGBA row from texture image then pack it in client mem */
            GLubyte rgba[MAX_WIDTH][4];
            GLint i;
            const GLubyte *src;
            switch (texImage->Format) {
               case GL_ALPHA:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = 255;
                     rgba[i][GCOMP] = 255;
                     rgba[i][BCOMP] = 255;
                     rgba[i][ACOMP] = src[i];
                  }
                  break;
               case GL_LUMINANCE:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i];
                     rgba[i][GCOMP] = src[i];
                     rgba[i][BCOMP] = src[i];
                     rgba[i][ACOMP] = 255;
                   }
                  break;
               case GL_LUMINANCE_ALPHA:
                  src = texImage->Data + row * 2 * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i*2+0];
                     rgba[i][GCOMP] = src[i*2+0];
                     rgba[i][BCOMP] = src[i*2+0];
                     rgba[i][ACOMP] = src[i*2+1];
                  }
                  break;
               case GL_INTENSITY:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i];
                     rgba[i][GCOMP] = src[i];
                     rgba[i][BCOMP] = src[i];
                     rgba[i][ACOMP] = 255;
                  }
                  break;
               case GL_RGB:
                  src = texImage->Data + row * 3 * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i*3+0];
                     rgba[i][GCOMP] = src[i*3+1];
                     rgba[i][BCOMP] = src[i*3+2];
                     rgba[i][ACOMP] = 255;
                  }
                  break;
               case GL_RGBA:
                  /* this special case should have been handled above! */
                  gl_problem( ctx, "error 1 in gl_GetTexImage" );
                  break;
               case GL_COLOR_INDEX:
                  gl_problem( ctx, "GL_COLOR_INDEX not implemented in gl_GetTexImage" );
                  break;
               default:
                  gl_problem( ctx, "bad format in gl_GetTexImage" );
            }
            gl_pack_rgba_span( ctx, width, (const GLubyte (*)[4])rgba,
                               format, type, dest, &ctx->Pack, GL_TRUE );
         }
      }
   }
}



/*
 * Unpack the image data given to glTexSubImage[12]D.
 * This function is just a wrapper for gl_unpack_image() but it does
 * some extra error checking.
 */
struct gl_image *
gl_unpack_texsubimage( GLcontext *ctx, GLint width, GLint height,
                       GLenum format, GLenum type, const GLvoid *pixels )
{
   if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
      return NULL;
   }

   if (format==GL_STENCIL_INDEX || format==GL_DEPTH_COMPONENT){
      return NULL;
   }

   if (gl_sizeof_type(type)<=0) {
      return NULL;
   }

   return gl_unpack_image3D( ctx, width, height, 1, format, type, pixels, &ctx->Unpack );
}


/*
 * Unpack the image data given to glTexSubImage3D.
 * This function is just a wrapper for gl_unpack_image() but it does
 * some extra error checking.
 */
struct gl_image *
gl_unpack_texsubimage3D( GLcontext *ctx, GLint width, GLint height,
                         GLint depth, GLenum format, GLenum type,
                         const GLvoid *pixels )
{
   if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
      return NULL;
   }

   if (format==GL_STENCIL_INDEX || format==GL_DEPTH_COMPONENT){
      return NULL;
   }

   if (gl_sizeof_type(type)<=0) {
      return NULL;
   }

   return gl_unpack_image3D( ctx, width, height, depth, format, type, pixels,
                             &ctx->Unpack );
}



void gl_TexSubImage1D( GLcontext *ctx,
                       GLenum target, GLint level, GLint xoffset,
                       GLsizei width, GLenum format, GLenum type,
                       struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(level)" );
      return;
   }

   destTex = texUnit->CurrentD[1]->Image[level];
   if (!destTex) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexSubImage1D" );
      return;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(xoffset)" );
      return;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(xoffset+width)" );
      return;
   }

   if (image) {
      /* unpacking must have been error-free */
      const GLint texcomponents = components_in_intformat(destTex->Format);
      const GLint xoffsetb = xoffset + destTex->Border;

      if (image->Type==GL_UNSIGNED_BYTE && texcomponents==image->Components) {
         /* Simple case, just byte copy image data into texture image */
         /* row by row. */
         GLubyte *dst = destTex->Data + texcomponents * xoffsetb;
         GLubyte *src = (GLubyte *) image->Data;
         MEMCPY( dst, src, width * texcomponents );
      }
      else {
         /* General case, convert image pixels into texels, scale, bias, etc */
         struct gl_texture_image *subTexImg = image_to_texture(ctx, image,
                                        destTex->IntFormat, destTex->Border);
         GLubyte *dst = destTex->Data + texcomponents * xoffsetb;
         GLubyte *src = subTexImg->Data;
         MEMCPY( dst, src, width * texcomponents );
         gl_free_texture_image(subTexImg);
      }

      /* if the image's reference count is zero, delete it now */
      if (image->RefCount==0) {
         gl_free_image(image);
      }

      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );

      /* tell driver about change */
      if (ctx->Driver.TexSubImage) {
	(*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_1D,
				    texUnit->CurrentD[1], level,
				    xoffset,0,width,1,
				    texUnit->CurrentD[1]->Image[level]->IntFormat,
				    destTex );
      }
      else {
	if (ctx->Driver.TexImage) {
	  (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D, texUnit->CurrentD[1], level,
				   texUnit->CurrentD[1]->Image[level]->IntFormat,
				   destTex );
	}
      }
   }
   else {
      /* if no image, an error must have occured, do more testing now */
      GLint components, size;

      if (width<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage1D(width)" );
         return;
      }
      if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      components = components_in_intformat( format );
      if (components<0 || format==GL_STENCIL_INDEX
          || format==GL_DEPTH_COMPONENT){
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      size = gl_sizeof_type( type );
      if (size<=0) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(type)" );
         return;
      }
      /* if we get here, probably ran out of memory during unpacking */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glTexSubImage1D" );
   }
}



void gl_TexSubImage2D( GLcontext *ctx,
                       GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(level)" );
      return;
   }

   destTex = texUnit->CurrentD[2]->Image[level];
   if (!destTex) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexSubImage2D" );
      return;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(xoffset)" );
      return;
   }
   if (yoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(yoffset)" );
      return;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(xoffset+width)" );
      return;
   }
   if (yoffset + height > (GLint) (destTex->Height + destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(yoffset+height)" );
      return;
   }

   if (image) {
      /* unpacking must have been error-free */
      const GLint texcomponents = components_in_intformat(destTex->Format);
      const GLint xoffsetb = xoffset + destTex->Border;
      const GLint yoffsetb = yoffset + destTex->Border;

      if (image->Type==GL_UNSIGNED_BYTE && texcomponents==image->Components) {
         /* Simple case, just byte copy image data into texture image */
         /* row by row. */
         GLubyte *dst = destTex->Data 
                      + (yoffsetb * destTex->Width + xoffsetb) * texcomponents;
         const GLubyte *src = (const GLubyte *) image->Data;
         GLint  j;
         for (j=0;j<height;j++) {
            MEMCPY( dst, src, width * texcomponents );
            dst += destTex->Width * texcomponents * sizeof(GLubyte);
            src += width * texcomponents * sizeof(GLubyte);
         }
      }
      else if (image->Type==GL_UNSIGNED_BYTE
               && texcomponents==3 && image->Components == 4 ) {
         /* 32 bit (padded) to 24 bit case, used heavily by quake */
         GLubyte *dst = destTex->Data 
                      + (yoffsetb * destTex->Width + xoffsetb) * texcomponents;
         const GLubyte *src = (const GLubyte *) image->Data;
         GLint j;
         for (j=0;j<height;j++) {
            const GLubyte *stop = src + (width << 2);
            for ( ; src != stop ; ) {
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = src[2];
               dst += 3;
               src += 4;
            }
            dst += (destTex->Width - width) * texcomponents * sizeof(GLubyte);
         }
      }
      else {
         /* General case, convert image pixels into texels, scale, bias, etc */
         struct gl_texture_image *subTexImg = image_to_texture(ctx, image,
                                        destTex->IntFormat, destTex->Border);
         GLubyte *dst = destTex->Data
                  + (yoffsetb * destTex->Width + xoffsetb) * texcomponents;
         const GLubyte *src = subTexImg->Data;
         GLint j;
         for (j=0;j<height;j++) {
            MEMCPY( dst, src, width * texcomponents );
            dst += destTex->Width * texcomponents * sizeof(GLubyte);
            src += width * texcomponents * sizeof(GLubyte);
         }
         gl_free_texture_image(subTexImg);
      }

      /* if the image's reference count is zero, delete it now */
      if (image->RefCount==0) {
         gl_free_image(image);
      }

      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[2] );

      /* tell driver about change */
      if (ctx->Driver.TexSubImage) {
	(*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_2D, texUnit->CurrentD[2], level,
				    xoffset, yoffset, width, height,
				    texUnit->CurrentD[2]->Image[level]->IntFormat,
				    destTex );
      }
      else {
	if (ctx->Driver.TexImage) {
	  (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D, texUnit->CurrentD[2], level,
				   texUnit->CurrentD[2]->Image[level]->IntFormat,
				   destTex );
	}
      }
   }
   else {
      /* if no image, an error must have occured, do more testing now */
      GLint components, size;

      if (width<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(width)" );
         return;
      }
      if (height<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage2D(height)" );
         return;
      }
      if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(format)" );
         return;
      }
      components = gl_components_in_format( format );
      if (components<0 || format==GL_STENCIL_INDEX
          || format==GL_DEPTH_COMPONENT){
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(format)" );
         return;
      }
      size = gl_sizeof_packed_type( type );
      if (size<=0) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(type)" );
         return;
      }
      /* if we get here, probably ran out of memory during unpacking */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glTexSubImage2D" );
   }
}



void gl_TexSubImage3DEXT( GLcontext *ctx,
                          GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLsizei width, GLsizei height, GLsizei depth,
                          GLenum format, GLenum type,
                          struct gl_image *image )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (target!=GL_TEXTURE_3D_EXT) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3DEXT(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3DEXT(level)" );
      return;
   }

   destTex = texUnit->CurrentD[3]->Image[level];
   if (!destTex) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexSubImage3DEXT" );
      return;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(xoffset)" );
      return;
   }
   if (yoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(yoffset)" );
      return;
   }
   if (zoffset < -((GLint)destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(zoffset)" );
      return;
   }
   if (xoffset + width > (GLint) (destTex->Width+destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(xoffset+width)" );
      return;
   }
   if (yoffset + height > (GLint) (destTex->Height+destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(yoffset+height)" );
      return;
   }
   if (zoffset + depth  > (GLint) (destTex->Depth+destTex->Border)) {
      gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(zoffset+depth)" );
      return;
   }

   if (image) {
      /* unpacking must have been error-free */
      GLint texcomponents = components_in_intformat(destTex->Format);
      GLint dstRectArea = destTex->Width * destTex->Height;
      GLint srcRectArea = width * height;
      const GLint xoffsetb = xoffset + destTex->Border;
      const GLint yoffsetb = yoffset + destTex->Border;
      const GLint zoffsetb = zoffset + destTex->Border;

      if (image->Type==GL_UNSIGNED_BYTE && texcomponents==image->Components) {
         /* Simple case, just byte copy image data into texture image */
         /* row by row. */
         GLubyte *dst = destTex->Data 
               + (zoffsetb * dstRectArea +  yoffsetb * destTex->Width + xoffsetb)
               * texcomponents;
         GLubyte *src = (GLubyte *) image->Data;
         GLint j, k;
         for(k=0;k<depth; k++) {
           for (j=0;j<height;j++) {
              MEMCPY( dst, src, width * texcomponents );
              dst += destTex->Width * texcomponents;
              src += width * texcomponents;
           }
           dst += dstRectArea * texcomponents * sizeof(GLubyte);
           src += srcRectArea * texcomponents * sizeof(GLubyte);
         }
      }
      else {
         /* General case, convert image pixels into texels, scale, bias, etc */
         struct gl_texture_image *subTexImg = image_to_texture(ctx, image,
                                        destTex->IntFormat, destTex->Border);
         GLubyte *dst = destTex->Data 
               + (zoffsetb * dstRectArea +  yoffsetb * destTex->Width + xoffsetb)
               * texcomponents;
         GLubyte *src = subTexImg->Data;
         GLint j, k;
         for(k=0;k<depth; k++) {
           for (j=0;j<height;j++) {
              MEMCPY( dst, src, width * texcomponents );
              dst += destTex->Width * texcomponents;
              src += width * texcomponents;
           }
           dst += dstRectArea * texcomponents * sizeof(GLubyte);
           src += srcRectArea * texcomponents * sizeof(GLubyte);
         }
         gl_free_texture_image(subTexImg);
      }
      /* if the image's reference count is zero, delete it now */
      if (image->RefCount==0) {
         gl_free_image(image);
      }

      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[3] );

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D_EXT, texUnit->CurrentD[3],
                                  level, texUnit->CurrentD[3]->Image[level]->IntFormat,
				  destTex );
      }
   }
   else {
      /* if no image, an error must have occured, do more testing now */
      GLint components, size;

      if (width<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(width)" );
         return;
      }
      if (height<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(height)" );
         return;
      }
      if (depth<0) {
         gl_error( ctx, GL_INVALID_VALUE, "glTexSubImage3DEXT(depth)" );
         return;
      }
      if (type==GL_BITMAP && format!=GL_COLOR_INDEX) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3DEXT(format)" );
         return;
      }
      components = gl_components_in_format( format );
      if (components<0 || format==GL_STENCIL_INDEX
          || format==GL_DEPTH_COMPONENT){
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3DEXT(format)" );
         return;
      }
      size = gl_sizeof_packed_type( type );
      if (size<=0) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3DEXT(type)" );
         return;
      }
      /* if we get here, probably ran out of memory during unpacking */
      gl_error( ctx, GL_OUT_OF_MEMORY, "glTexSubImage3DEXT" );
   }
}



/*
 * Read an RGBA image from the frame buffer.
 * Input:  ctx - the context
 *         x, y - lower left corner
 *         width, height - size of region to read
 *         format - one of GL_RED, GL_RGB, GL_LUMINANCE, etc.
 * Return: gl_image pointer or NULL if out of memory
 */
static struct gl_image *read_color_image( GLcontext *ctx, GLint x, GLint y,
                                          GLsizei width, GLsizei height,
                                          GLenum format )
{
   struct gl_image *image;
   GLubyte *imgptr;
   GLint components;
   GLint i, j;

   components = components_in_intformat( format );

   /*
    * Allocate image struct and image data buffer
    */
   image = MALLOC_STRUCT( gl_image );
   if (image) {
      image->Width = width;
      image->Height = height;
      image->Depth = 1;
      image->Components = components;
      image->Format = format;
      image->Type = GL_UNSIGNED_BYTE;
      image->RefCount = 0;
      image->Data = (GLubyte *) MALLOC( width * height * components );
      if (!image->Data) {
         FREE(image);
         return NULL;
      }
   }
   else {
      return NULL;
   }

   imgptr = (GLubyte *) image->Data;

   /* Select buffer to read from */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.DriverReadBuffer );

   for (j=0;j<height;j++) {
      GLubyte rgba[MAX_WIDTH][4];
      gl_read_rgba_span( ctx, width, x, y+j, rgba );

      switch (format) {
         case GL_ALPHA:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][ACOMP];
            }
            break;
         case GL_LUMINANCE:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][RCOMP];
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][RCOMP];
               *imgptr++ = rgba[i][ACOMP];
            }
            break;
         case GL_INTENSITY:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][RCOMP];
            }
            break;
         case GL_RGB:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][RCOMP];
               *imgptr++ = rgba[i][GCOMP];
               *imgptr++ = rgba[i][BCOMP];
            }
            break;
         case GL_RGBA:
            for (i=0;i<width;i++) {
               *imgptr++ = rgba[i][RCOMP];
               *imgptr++ = rgba[i][GCOMP];
               *imgptr++ = rgba[i][BCOMP];
               *imgptr++ = rgba[i][ACOMP];
            }
            break;
         default:
            gl_problem(ctx, "Bad format in read_color_image");
            break;
      } /*switch*/

   } /*for*/         

   /* Restore drawing buffer */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DriverDrawBuffer );

   return image;
}




void gl_CopyTexImage1D( GLcontext *ctx,
                        GLenum target, GLint level,
                        GLenum internalformat,
                        GLint x, GLint y,
                        GLsizei width, GLint border )
{
   GLint format;
   struct gl_image *teximage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage1D");
   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(level)" );
      return;
   }
   if (border!=0 && border!=1) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(border)" );
      return;
   }
   if (width < 2*border || width > 2 + ctx->Const.MaxTextureSize || width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(width)" );
      return;
   }
   format = decode_internal_format( internalformat );
   if (format<0 || (internalformat>=1 && internalformat<=4)) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage1D(format)" );
      return;
   }

   teximage = read_color_image( ctx, x, y, width, 1, (GLenum) format );
   if (!teximage) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D" );
      return;
   }

   gl_TexImage1D( ctx, target, level, internalformat, width,
                  border, GL_RGBA, GL_UNSIGNED_BYTE, teximage );

   /* teximage was freed in gl_TexImage1D */
}



void gl_CopyTexImage2D( GLcontext *ctx,
                        GLenum target, GLint level, GLenum internalformat,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        GLint border )
{
   GLint format;
   struct gl_image *teximage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage2D");
   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(level)" );
      return;
   }
   if (border!=0 && border!=1) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(border)" );
      return;
   }
   if (width<2*border || width>2+ctx->Const.MaxTextureSize || width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(width)" );
      return;
   }
   if (height<2*border || height>2+ctx->Const.MaxTextureSize || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(height)" );
      return;
   }
   format = decode_internal_format( internalformat );
   if (format<0 || (internalformat>=1 && internalformat<=4)) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexImage2D(format)" );
      return;
   }

   teximage = read_color_image( ctx, x, y, width, height, (GLenum) format );
   if (!teximage) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D" );
      return;
   }

   gl_TexImage2D( ctx, target, level, internalformat, width, height,
                  border, GL_RGBA, GL_UNSIGNED_BYTE, teximage );

   /* teximage was freed in gl_TexImage2D */
}




/*
 * Do the work of glCopyTexSubImage[123]D.
 * TODO: apply pixel bias scale and mapping.
 */
static void copy_tex_sub_image( GLcontext *ctx, struct gl_texture_image *dest,
                                GLint width, GLint height,
                                GLint srcx, GLint srcy,
                                GLint dstx, GLint dsty, GLint dstz )
{
   GLint i, j;
   GLint format, components, rectarea;
   GLint texwidth, texheight, zoffset;

   /* dst[xyz] may be negative if we have a texture border! */
   dstx += dest->Border;
   dsty += dest->Border;
   dstz += dest->Border;
   texwidth = dest->Width;
   texheight = dest->Height;
   rectarea = texwidth * texheight;
   zoffset = dstz * rectarea; 
   format = dest->Format;
   components = components_in_intformat( format );

   /* Select buffer to read from */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.DriverReadBuffer );

   for (j=0;j<height;j++) {
      GLubyte rgba[MAX_WIDTH][4];
      GLubyte *texptr;

      gl_read_rgba_span( ctx, width, srcx, srcy+j, rgba );

      texptr = dest->Data + ( zoffset + (dsty+j) * texwidth + dstx) * components;

      switch (format) {
         case GL_ALPHA:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][ACOMP];
            }
            break;
         case GL_LUMINANCE:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][RCOMP];
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][RCOMP];
               *texptr++ = rgba[i][ACOMP];
            }
            break;
         case GL_INTENSITY:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][RCOMP];
            }
            break;
         case GL_RGB:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][RCOMP];
               *texptr++ = rgba[i][GCOMP];
               *texptr++ = rgba[i][BCOMP];
            }
            break;
         case GL_RGBA:
            for (i=0;i<width;i++) {
               *texptr++ = rgba[i][RCOMP];
               *texptr++ = rgba[i][GCOMP];
               *texptr++ = rgba[i][BCOMP];
               *texptr++ = rgba[i][ACOMP];
            }
            break;
      } /*switch*/
   } /*for*/         


   /* Restore drawing buffer */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DriverDrawBuffer );
}




void gl_CopyTexSubImage1D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint x, GLint y, GLsizei width )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *teximage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage1D");
   if (target!=GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage1D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(level)" );
      return;
   }
   if (width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(width)" );
      return;
   }

   teximage = texUnit->CurrentD[1]->Image[level];

   if (teximage) {
      if (xoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage1D(xoffset)" );
         return;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (xoffset+width > (GLint) (teximage->Width+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage1D(xoffset+width)" );
         return;
      }
      if (teximage->Data) {
         copy_tex_sub_image( ctx, teximage, width, 1, x, y, xoffset, 0, 0 );

	 /* tell driver about change */
	 if (ctx->Driver.TexSubImage) {
	   (*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_1D,
				       texUnit->CurrentD[1], level,
				       xoffset,0,width,1,
				       teximage->IntFormat,
				       teximage );
	 }
	 else {
	   if (ctx->Driver.TexImage) {
	     (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D, texUnit->CurrentD[1], level,
				      teximage->IntFormat,
				      teximage );
	   }
	 }
      }
   }
   else {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage1D" );
   }
}



void gl_CopyTexSubImage2D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *teximage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage2D");
   if (target!=GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(level)" );
      return;
   }
   if (width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(width)" );
      return;
   }
   if (height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(height)" );
      return;
   }

   teximage = texUnit->CurrentD[2]->Image[level];

   if (teximage) {
      if (xoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(xoffset)" );
         return;
      }
      if (yoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage2D(yoffset)" );
         return;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (xoffset+width > (GLint) (teximage->Width+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage2D(xoffset+width)" );
         return;
      }
      if (yoffset+height > (GLint) (teximage->Height+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage2D(yoffset+height)" );
         return;
      }

      if (teximage->Data) {
         copy_tex_sub_image( ctx, teximage, width, height,
                             x, y, xoffset, yoffset, 0 );
	 /* tell driver about change */
	 if (ctx->Driver.TexSubImage) {
	   (*ctx->Driver.TexSubImage)( ctx, GL_TEXTURE_2D, texUnit->CurrentD[2], level,
				       xoffset, yoffset, width, height,
				       teximage->IntFormat,
				       teximage );
	 }
	 else {
	   if (ctx->Driver.TexImage) {
	     (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D, texUnit->CurrentD[2], level,
				      teximage->IntFormat,
				      teximage );
	   }
	 }
      }
   }
   else {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D" );
   }
}



void gl_CopyTexSubImage3DEXT( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *teximage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage3DEXT");
   if (target!=GL_TEXTURE_3D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage3DEXT(target)" );
      return;
   }
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(level)" );
      return;
   }
   if (width<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(width)" );
      return;
   }
   if (height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(height)" );
      return;
   }

   teximage = texUnit->CurrentD[3]->Image[level];
   if (teximage) {
      if (xoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(xoffset)" );
         return;
      }
      if (yoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(yoffset)" );
         return;
      }
      if (zoffset < -((GLint)teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE, "glCopyTexSubImage3DEXT(zoffset)" );
         return;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (xoffset+width > (GLint) (teximage->Width+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage3DEXT(xoffset+width)" );
         return;
      }
      if (yoffset+height > (GLint) (teximage->Height+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage3DEXT(yoffset+height)" );
         return;
      }
      if (zoffset > (GLint) (teximage->Depth+teximage->Border)) {
         gl_error( ctx, GL_INVALID_VALUE,
                   "glCopyTexSubImage3DEXT(zoffset+depth)" );
         return;
      }

      if (teximage->Data) {
         copy_tex_sub_image( ctx, teximage, width, height, 
                             x, y, xoffset, yoffset, zoffset);

	 /* tell driver about change */
	 if (ctx->Driver.TexImage) {
	   (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D_EXT, texUnit->CurrentD[3],
				    level, teximage->IntFormat,
				    teximage );
	 }
      }
   }
   else {
      gl_error( ctx, GL_INVALID_OPERATION, "glCopyTexSubImage3DEXT" );
   }
}

