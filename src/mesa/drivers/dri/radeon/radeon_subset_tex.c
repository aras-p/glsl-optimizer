/**
 * \file radeon_subset_tex.c
 * \brief Texturing.
 *
 * \author Gareth Hughes <gareth@valinux.com>
 * \author Brian Paul <brianp@valinux.com>
 */

/*
 * Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_tex.c,v 1.6 2002/09/16 18:05:20 eich Exp $ */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "image.h"
#include "simple_list.h"
#include "texformat.h"
#include "texstore.h"

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_subset.h"

#include <errno.h>
#include <stdio.h>


/**
 * \brief Destroy hardware state associated with a texture.
 *
 * \param rmesa Radeon context.
 * \param t Radeon texture object to be destroyed.
 *
 * Frees the memory associated with the texture and if the texture is bound to
 * a texture unit cleans the associated hardware state.
 */
void radeonDestroyTexObj( radeonContextPtr rmesa, radeonTexObjPtr t )
{
   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   if ( t->tObj )
      t->tObj->DriverData = NULL;

   if ( rmesa ) {
      if ( t == rmesa->state.texture.unit[0].texobj ) {
         rmesa->state.texture.unit[0].texobj = NULL;
	 remove_from_list( &rmesa->hw.tex[0] );
	 make_empty_list( &rmesa->hw.tex[0] );
      }
   }

   remove_from_list( t );
   FREE( t );
}


/**
 * \brief Keep track of swapped out texture objects.
 * 
 * \param rmesa Radeon context.
 * \param t Radeon texture object.
 *
 * Frees the memory associated with the texture, marks all mipmap images in
 * the texture as dirty and add it to the radeon_texture::swapped list.
 */
static void radeonSwapOutTexObj( radeonContextPtr rmesa, radeonTexObjPtr t )
{
   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   t->dirty_images = ~0;
   move_to_tail( &rmesa->texture.swapped, t );
}


/**
 * Texture space has been invalidated.
 *
 * \param rmesa Radeon context.
 * \param heap texture heap number.
 * 
 * Swaps out every texture in the specified heap.
 */
void radeonAgeTextures( radeonContextPtr rmesa, int heap )
{
   radeonTexObjPtr t, tmp;

   foreach_s ( t, tmp, &rmesa->texture.objects[heap] ) 
      radeonSwapOutTexObj( rmesa, t );
}


/***************************************************************/
/** \name Texture image conversions
 */
/*@{*/

/**
 * \brief Upload texture image.
 *
 * \param rmesa Radeon context.
 * \param t Radeon texture object.
 * \param level level of the image to take the sub-image.
 * \param x sub-image abscissa.
 * \param y sub-image ordinate.
 * \param width sub-image width.
 * \param height sub-image height.
 *
 * Fills in a drmRadeonTexture and drmRadeonTexImage structures and uploads the
 * texture via the DRM_RADEON_TEXTURE ioctl, aborting in case of failure.
 */
static void radeonUploadSubImage( radeonContextPtr rmesa,
				  radeonTexObjPtr t, GLint level,
				  GLint x, GLint y, GLint width, GLint height )
{
   struct gl_texture_image *texImage;
   GLint ret;
   drmRadeonTexture tex;
   drmRadeonTexImage tmp;

   level += t->firstLevel;
   texImage = t->tObj->Image[0][level];

   if ( !texImage || !texImage->Data ) 
      return;

   t->image[level].data = texImage->Data;

   tex.offset = t->bufAddr;
   tex.pitch = (t->image[0].width * texImage->TexFormat->TexelBytes) / 64;
   tex.format = t->pp_txformat & RADEON_TXFORMAT_FORMAT_MASK;
   tex.width = texImage->Width;
   tex.height = texImage->Height;
   tex.image = &tmp;

   memcpy( &tmp, &t->image[level], sizeof(drmRadeonTexImage) );

   do {
      ret = drmCommandWriteRead( rmesa->dri.fd, DRM_RADEON_TEXTURE,
                                 &tex, sizeof(drmRadeonTexture) );
   } while ( ret && errno == EAGAIN );

   if ( ret ) {
      UNLOCK_HARDWARE( rmesa );
      fprintf( stderr, "DRM_RADEON_TEXTURE: return = %d\n", ret );
      exit( 1 );
   }
}

/**
 * \brief Upload texture images.
 *
 * This might require removing our own and/or other client's texture objects to
 * make room for these images.
 *
 * \param rmesa Radeon context.
 * \param tObj texture object to upload.
 *
 * Sets the matching hardware texture format. Calculates which mipmap levels to
 * send, depending of the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL and the
 * Radeon offset rules. Kicks out textures until the requested texture fits,
 * sets the texture hardware state and, while holding the hardware lock,
 * uploads any images that are new.
 */
static void radeonSetTexImages( radeonContextPtr rmesa,
				struct gl_texture_object *tObj )
{
   radeonTexObjPtr t = (radeonTexObjPtr)tObj->DriverData;
   const struct gl_texture_image *baseImage = tObj->Image[0][tObj->BaseLevel];
   GLint totalSize;
   GLint texelsPerDword = 0, blitWidth = 0, blitPitch = 0;
   GLint x, y, width, height;
   GLint i;
   GLint firstLevel, lastLevel, numLevels;
   GLint log2Width, log2Height;
   GLuint txformat = 0;

   /* This code cannot be reached once we have lost focus
    */
   assert(rmesa->radeonScreen->buffers);

   /* Set the hardware texture format
    */
   switch (baseImage->TexFormat->MesaFormat) {
   case MESA_FORMAT_I8:
      txformat = RADEON_TXFORMAT_I8;
      texelsPerDword = 4;
      blitPitch = 64;
      break;
   case MESA_FORMAT_RGBA8888:
      txformat = RADEON_TXFORMAT_RGBA8888 | RADEON_TXFORMAT_ALPHA_IN_MAP;
      texelsPerDword = 1;
      blitPitch = 16;
      break;
   case MESA_FORMAT_RGB565:
      txformat = RADEON_TXFORMAT_RGB565;
      texelsPerDword = 2;
      blitPitch = 32;
      break;
   default:
      _mesa_problem(NULL, "unexpected texture format in radeonTexImage2D");
      return;
   }

   t->pp_txformat &= ~(RADEON_TXFORMAT_FORMAT_MASK |
		       RADEON_TXFORMAT_ALPHA_IN_MAP);
   t->pp_txformat |= txformat;


   /* Select the larger of the two widths for our global texture image
    * coordinate space.  As the Radeon has very strict offset rules, we
    * can't upload mipmaps directly and have to reference their location
    * from the aligned start of the whole image.
    */
   blitWidth = MAX2( baseImage->Width, blitPitch );

   /* Calculate mipmap offsets and dimensions.
    */
   totalSize = 0;
   x = 0;
   y = 0;

   /* Compute which mipmap levels we really want to send to the hardware.
    * This depends on the base image size, GL_TEXTURE_MIN_LOD,
    * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
    * Yes, this looks overly complicated, but it's all needed.
    */
   firstLevel = tObj->BaseLevel + (GLint) (tObj->MinLod + 0.5);
   firstLevel = MAX2(firstLevel, tObj->BaseLevel);
   lastLevel = tObj->BaseLevel + (GLint) (tObj->MaxLod + 0.5);
   lastLevel = MAX2(lastLevel, tObj->BaseLevel);
   lastLevel = MIN2(lastLevel, tObj->BaseLevel + baseImage->MaxLog2);
   lastLevel = MIN2(lastLevel, tObj->MaxLevel);
   lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */

   /* save these values */
   t->firstLevel = firstLevel;
   t->lastLevel = lastLevel;

   numLevels = lastLevel - firstLevel + 1;

   log2Width = tObj->Image[0][firstLevel]->WidthLog2;
   log2Height = tObj->Image[0][firstLevel]->HeightLog2;

   for ( i = 0 ; i < numLevels ; i++ ) {
      const struct gl_texture_image *texImage = tObj->Image[0][i + firstLevel];
      if ( !texImage )
	 break;

      width = texImage->Width;
      height = texImage->Height;

      /* Texture images have a minimum pitch of 32 bytes (half of the
       * 64-byte minimum pitch for blits).  For images that have a
       * width smaller than this, we must pad each texture image
       * scanline out to this amount.
       */
      if ( width < blitPitch / 2 ) {
	 width = blitPitch / 2;
      }

      totalSize += width * height * baseImage->TexFormat->TexelBytes;
      ASSERT( (totalSize & 31) == 0 );

      while ( width < blitWidth && height > 1 ) {
	 width *= 2;
	 height /= 2;
      }

      ASSERT(i < RADEON_MAX_TEXTURE_LEVELS);
      t->image[i].x = x;
      t->image[i].y = y;
      t->image[i].width  = width;
      t->image[i].height = height;

      /* While blits must have a pitch of at least 64 bytes, mipmaps
       * must be aligned on a 32-byte boundary (just like each texture
       * image scanline).
       */
      if ( width >= blitWidth ) {
	 y += height;
      } else {
	 x += width;
	 if ( x >= blitWidth ) {
	    x = 0;
	    y++;
	 }
      }
   }

   /* Align the total size of texture memory block.
    */
   t->totalSize = (totalSize + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

   /* Hardware state:
    */
   t->pp_txfilter &= ~RADEON_MAX_MIP_LEVEL_MASK;
   t->pp_txfilter |= (numLevels - 1) << RADEON_MAX_MIP_LEVEL_SHIFT;

   t->pp_txformat &= ~(RADEON_TXFORMAT_WIDTH_MASK |
		       RADEON_TXFORMAT_HEIGHT_MASK);
   t->pp_txformat |= ((log2Width << RADEON_TXFORMAT_WIDTH_SHIFT) |
		      (log2Height << RADEON_TXFORMAT_HEIGHT_SHIFT));
   t->dirty_state = TEX_ALL;

   /* Update the local texture LRU.
    */
   move_to_head( &rmesa->texture.objects[0], t );

   LOCK_HARDWARE( rmesa );

   /* Kick out textures until the requested texture fits */
   while ( !t->memBlock ) {
      t->memBlock = mmAllocMem( rmesa->texture.heap[0], t->totalSize, 12, 0);
	 
      if (!t->memBlock)
	 radeonSwapOutTexObj( rmesa, rmesa->texture.objects[0].prev );
	 
   }

   /* Set the base offset of the texture image */
   t->bufAddr = rmesa->radeonScreen->texOffset[0] + t->memBlock->ofs;
   t->pp_txoffset = t->bufAddr;

   /* Upload any images that are new 
    */
   for ( i = 0 ; i < numLevels ; i++ ) {
      if ( t->dirty_images & (1 << i) ) {
	 radeonUploadSubImage( rmesa, t, i, 0, 0,
			       t->image[i].width, t->image[i].height );
      }
   }

   rmesa->texture.age[0] = ++rmesa->sarea->texAge[0]; 
   UNLOCK_HARDWARE( rmesa );
   t->dirty_images = 0;
}

/*@}*/


/******************************************************************/
/** \name Texture combine functions
 */
/*@{*/

enum {
   RADEON_DISABLE	= 0, /**< \brief disabled */
   RADEON_REPLACE	= 1, /**< \brief replace function */
   RADEON_MODULATE	= 2, /**< \brief modulate function */
   RADEON_DECAL		= 3, /**< \brief decal function */
   RADEON_BLEND		= 4, /**< \brief blend function */
   RADEON_MAX_COMBFUNC	= 5  /**< \brief max number of combine functions */
} ;


/**
 * \brief Color combine function hardware state table.
 */
static GLuint radeon_color_combine[][RADEON_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_CURRENT_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00802800
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T0_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800142
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T0_COLOR |
       RADEON_COLOR_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x008c2d42
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T0_COLOR |
       RADEON_COLOR_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x008c2902
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_TFACTOR_COLOR |
       RADEON_COLOR_ARG_C_T0_COLOR |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   },

};

/**
 * \brief Alpha combine function hardware state table.
 */
static GLuint radeon_alpha_combine[][RADEON_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00800500
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800051
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_T0_ALPHA |
       RADEON_ALPHA_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x00800100
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x00800051
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_TFACTOR_ALPHA |
       RADEON_ALPHA_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

   },

};

/*@}*/


/******************************************************************/
/** \name Texture unit state management
 */
/*@{*/

/**
 * \brief Update the texture environment.
 *
 * \param ctx GL context
 * \param unit texture unit to update.
 *
 * Sets the state of the RADEON_TEX_PP_TXCBLEND and RADEON_TEX_PP_TXABLEND
 * registers using the ::radeon_color_combine and ::radeon_alpha_combine tables,
 * and informs of the state change.
 */
static void radeonUpdateTextureEnv( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   const struct gl_texture_object *tObj = texUnit->_Current;
   const GLenum format = tObj->Image[0][tObj->BaseLevel]->Format;
   GLuint color_combine = radeon_color_combine[unit][RADEON_DISABLE];
   GLuint alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];


   /* Set the texture environment state.  Isn't this nice and clean?
    * The Radeon will automagically set the texture alpha to 0xff when
    * the texture format does not include an alpha component.  This
    * reduces the amount of special-casing we have to do, alpha-only
    * textures being a notable exception.
    */
   switch ( texUnit->EnvMode ) {
   case GL_REPLACE:
      switch ( format ) {
      case GL_RGBA:
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_REPLACE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_REPLACE];
	 break;
      case GL_RGB:
	 color_combine = radeon_color_combine[unit][RADEON_REPLACE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
         break;
      default:
	 break;
      }
      break;

   case GL_MODULATE:
      switch ( format ) {
      case GL_RGBA:
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_MODULATE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_RGB:
	 color_combine = radeon_color_combine[unit][RADEON_MODULATE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
	 break;
      default:
	 break;
      }
      break;

   case GL_DECAL:
      switch ( format ) {
      case GL_RGBA:
      case GL_RGB:
	 color_combine = radeon_color_combine[unit][RADEON_DECAL];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
	 break;
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
	 break;
      default:
	 break;
      }
      break;

   case GL_BLEND:
      switch ( format ) {
      case GL_RGBA:
      case GL_RGB:
	 color_combine = radeon_color_combine[unit][RADEON_BLEND];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_BLEND];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_BLEND];
	 break;
      default:
	 break;
      }
      break;

   default:
      break;
   }

   if ( rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] != color_combine ||
	rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] != alpha_combine ) {
      RADEON_STATECHANGE( rmesa, tex[unit] );
      rmesa->hw.tex[unit].cmd[TEX_PP_TXCBLEND] = color_combine;
      rmesa->hw.tex[unit].cmd[TEX_PP_TXABLEND] = alpha_combine;
   }
}


#define TEXOBJ_TXFILTER_MASK (RADEON_MAX_MIP_LEVEL_MASK |	\
			      RADEON_MIN_FILTER_MASK | 		\
			      RADEON_MAG_FILTER_MASK |		\
			      RADEON_MAX_ANISO_MASK |		\
			      RADEON_CLAMP_S_MASK | 		\
			      RADEON_CLAMP_T_MASK)

#define TEXOBJ_TXFORMAT_MASK (RADEON_TXFORMAT_WIDTH_MASK |	\
			      RADEON_TXFORMAT_HEIGHT_MASK |	\
			      RADEON_TXFORMAT_FORMAT_MASK |	\
			      RADEON_TXFORMAT_ALPHA_IN_MAP)



void radeonUpdateTextureState( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[0];

   if ( texUnit->_ReallyEnabled & (TEXTURE_1D_BIT | TEXTURE_2D_BIT) ) {
      struct gl_texture_object *tObj = texUnit->_Current;
      radeonTexObjPtr t = (radeonTexObjPtr) tObj->DriverData;

      /* Upload teximages (not pipelined)
       */
      if ( t->dirty_images ) {
	 RADEON_FIREVERTICES( rmesa );
	 radeonSetTexImages( rmesa, tObj );
      }

      /* Update state if this is a different texture object to last
       * time.
       */
      if ( rmesa->state.texture.unit[0].texobj != t ) {
	 rmesa->state.texture.unit[0].texobj = t;
	 t->dirty_state |= 1<<0;
	 move_to_head( &rmesa->texture.objects[0], t );
      }

      if (t->dirty_state) {
	 GLuint *cmd = RADEON_DB_STATE( tex[0] );

	 cmd[TEX_PP_TXFILTER] &= ~TEXOBJ_TXFILTER_MASK;
	 cmd[TEX_PP_TXFORMAT] &= ~TEXOBJ_TXFORMAT_MASK;
	 cmd[TEX_PP_TXFILTER] |= t->pp_txfilter & TEXOBJ_TXFILTER_MASK;
	 cmd[TEX_PP_TXFORMAT] |= t->pp_txformat & TEXOBJ_TXFORMAT_MASK;
	 cmd[TEX_PP_TXOFFSET] = t->pp_txoffset;
	 cmd[TEX_PP_BORDER_COLOR] = t->pp_border_color;
	 
	 RADEON_DB_STATECHANGE( rmesa, &rmesa->hw.tex[0] );
	 t->dirty_state = 0;
      }

      /* Newly enabled?
       */
      if (!(rmesa->hw.ctx.cmd[CTX_PP_CNTL] & RADEON_TEX_0_ENABLE)) {
	 RADEON_STATECHANGE( rmesa, ctx );
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= (RADEON_TEX_0_ENABLE | 
					    RADEON_TEX_BLEND_0_ENABLE);

	 RADEON_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] |= RADEON_TCL_VTX_ST0;
      }

      radeonUpdateTextureEnv( ctx, 0 );
   }
   else if (rmesa->hw.ctx.cmd[CTX_PP_CNTL] & (RADEON_TEX_0_ENABLE<<0)) {
      /* Texture unit disabled */
      rmesa->state.texture.unit[0].texobj = 0;
      RADEON_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= 
	 ~((RADEON_TEX_0_ENABLE | RADEON_TEX_BLEND_0_ENABLE) << 0);

      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &= ~(RADEON_TCL_VTX_ST0 |
						RADEON_TCL_VTX_Q0);
   }
}



/**
 * \brief Choose texture format.
 *
 * \param ctx GL context.
 * \param internalFormat texture internal format.
 * \param format pixel format. Not used.
 * \param type pixel data type. Not used.
 *
 * \return pointer to chosen texture format.
 *
 * Returns a pointer to one of the Mesa texture formats which is supported by
 * Radeon and matches the internal format.
 */
static const struct gl_texture_format *
radeonChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                           GLenum format, GLenum type )
{
   switch ( internalFormat ) {
   case GL_RGBA:
   case GL_RGBA8:
      return &_mesa_texformat_rgba8888;

   case GL_RGB:
   case GL_RGB5:
      return &_mesa_texformat_rgb565;

   case GL_INTENSITY:
   case GL_INTENSITY8:
      return &_mesa_texformat_i8;

   default:
      _mesa_problem(ctx, "unexpected texture format in radeonChoosTexFormat");
      return NULL;
   }
}

/**
 * \brief Allocate a Radeon texture object.
 *
 * \param texObj texture object.
 *
 * \return pointer to the device specific texture object on success, or NULL on failure.
 *
 * Allocates and initializes a radeon_tex_obj structure to connect it to the
 * driver private data pointer in \p texObj.
 */
static radeonTexObjPtr radeonAllocTexObj( struct gl_texture_object *texObj )
{
   radeonTexObjPtr t;

   t = CALLOC_STRUCT( radeon_tex_obj );
   if (!t)
      return NULL;

   t->tObj = texObj;
   texObj->DriverData = t;
   make_empty_list( t );
   t->dirty_images = ~0;
   return t;
}


/**
 * \brief Load a texture image.
 *
 * \param ctx GL context.
 * \param texObj texture object
 * \param target target texture.
 * \param level level of detail number.
 * \param internalFormat internal format.
 * \param width texture image width.
 * \param height texture image height.
 * \param border border width.
 * \param format pixel format.
 * \param type pixel data type.
 * \param pixels image data.
 * \param packing passed to _mesa_store_teximage2d() unchanged.
 * \param texImage passed to _mesa_store_teximage2d() unchanged.
 * 
 * If there is a device specific texture object associated with the given
 * texture object then swaps that texture out. Calls _mesa_store_teximage2d()
 * with all other parameters unchanged.
 */
static void radeonTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                              GLint internalFormat,
                              GLint width, GLint height, GLint border,
                              GLenum format, GLenum type, const GLvoid *pixels,
                              const struct gl_pixelstore_attrib *packing,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t = (radeonTexObjPtr)texObj->DriverData;

   if ( t ) 
      radeonSwapOutTexObj( rmesa, t );

   /* Note, this will call radeonChooseTextureFormat */
   _mesa_store_teximage2d(ctx, target, level, internalFormat,
                          width, height, border, format, type, pixels,
                          &ctx->Unpack, texObj, texImage);
}

/**
 * \brief Set texture environment parameters.
 *
 * \param ctx GL context.
 * \param target texture environment.
 * \param pname texture parameter. Accepted value is GL_TEXTURE_ENV_COLOR.
 * \param param parameter value.
 *
 * Updates the current unit's RADEON_TEX_PP_TFACTOR register and informs of the
 * state change.
 */
static void radeonTexEnv( GLcontext *ctx, GLenum target,
			  GLenum pname, const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   switch ( pname ) {
   case GL_TEXTURE_ENV_COLOR: {
      GLubyte c[4];
      GLuint envColor;
      UNCLAMPED_FLOAT_TO_RGBA_CHAN( c, texUnit->EnvColor );
      envColor = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] != envColor ) {
	 RADEON_STATECHANGE( rmesa, tex[unit] );
	 rmesa->hw.tex[unit].cmd[TEX_PP_TFACTOR] = envColor;
      }
      break;
   }

   default:
      return;
   }
}

/**
 * \brief Set texture parameter.
 *
 * \param ctx GL context.
 * \param target target texture.
 * \param texObj texture object.
 * \param pname texture parameter.
 * \param params parameter value.
 * 
 * Allocates the device specific texture object data if it doesn't exist
 * already.
 * 
 * Updates the texture object radeon_tex_obj::pp_txfilter register and marks
 * the texture state (radeon_tex_obj::dirty_state) as dirty.
 */
static void radeonTexParameter( GLcontext *ctx, GLenum target,
				struct gl_texture_object *texObj,
				GLenum pname, const GLfloat *params )
{
   radeonTexObjPtr t = (radeonTexObjPtr) texObj->DriverData;
   
   if (!t)
      t = radeonAllocTexObj( texObj );

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
      t->pp_txfilter &= ~RADEON_MIN_FILTER_MASK;
      switch ( texObj->MinFilter ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
	 break;
      case GL_NEAREST_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_NEAREST;
	 break;
      case GL_NEAREST_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_NEAREST;
	 break;
      case GL_LINEAR_MIPMAP_NEAREST:
	 t->pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_LINEAR;
	 break;
      case GL_LINEAR_MIPMAP_LINEAR:
	 t->pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_LINEAR;
	 break;
      }
      break;

   case GL_TEXTURE_MAG_FILTER:
      t->pp_txfilter &= ~RADEON_MAG_FILTER_MASK;
      switch ( texObj->MagFilter ) {
      case GL_NEAREST:
	 t->pp_txfilter |= RADEON_MAG_FILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 t->pp_txfilter |= RADEON_MAG_FILTER_LINEAR;
	 break;
      }
      break;

   case GL_TEXTURE_WRAP_S:
      t->pp_txfilter &= ~RADEON_CLAMP_S_MASK;
      switch ( texObj->WrapS ) {
      case GL_REPEAT:
	 t->pp_txfilter |= RADEON_CLAMP_S_WRAP;
	 break;
      case GL_CLAMP_TO_EDGE:
	 t->pp_txfilter |= RADEON_CLAMP_S_CLAMP_LAST;
	 break;
      }
      break;

   case GL_TEXTURE_WRAP_T:
      t->pp_txfilter &= ~RADEON_CLAMP_T_MASK;
      switch ( texObj->WrapT ) {
      case GL_REPEAT:
	 t->pp_txfilter |= RADEON_CLAMP_T_WRAP;
	 break;
      case GL_CLAMP_TO_EDGE:
	 t->pp_txfilter |= RADEON_CLAMP_T_CLAMP_LAST;
	 break;
      }
      break;

   default:
      return;
   }

   /* Mark this texobj as dirty (one bit per tex unit)
    */
   t->dirty_state = TEX_ALL;
}

/**
 * \brief Bind texture.
 *
 * \param ctx GL context.
 * \param target not used.
 * \param texObj texture object.
 * 
 * Allocates the device specific texture data if it doesn't exist already.
 */
static void radeonBindTexture( GLcontext *ctx, GLenum target,
			       struct gl_texture_object *texObj )
{
   if ( !texObj->DriverData ) 
      radeonAllocTexObj( texObj );
}

/**
 * \brief Delete texture.
 *
 * \param ctx GL context.
 * \param texObj texture object.
 *
 * Fires any outstanding vertices and destroy the device specific texture
 * object.
 */ 
static void radeonDeleteTexture( GLcontext *ctx,
				 struct gl_texture_object *texObj )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t = (radeonTexObjPtr) texObj->DriverData;

   if ( t ) {
      if ( rmesa ) 
         RADEON_FIREVERTICES( rmesa );
      radeonDestroyTexObj( rmesa, t );
   }
}

/**
 * \brief Initialize context texture object data.
 * 
 * \param ctx GL context.
 *
 * Called by radeonInitTextureFuncs() to setup the context initial texture
 * objects.
 */
static void radeonInitTextureObjects( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_object *texObj;
   GLuint tmp = ctx->Texture.CurrentUnit;

   ctx->Texture.CurrentUnit = 0;

   texObj = ctx->Texture.Unit[0].Current2D;
   radeonBindTexture( ctx, GL_TEXTURE_2D, texObj );
   move_to_tail( &rmesa->texture.swapped,
		 (radeonTexObjPtr)texObj->DriverData );


   ctx->Texture.CurrentUnit = tmp;
}

/**
 * \brief Setup the GL context driver callbacks.
 *
 * \param ctx GL context.
 *
 * \sa Called by radeonCreateContext().
 */
void radeonInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.ChooseTextureFormat	= radeonChooseTextureFormat;
   ctx->Driver.TexImage2D		= radeonTexImage2D;

   ctx->Driver.BindTexture		= radeonBindTexture;
   ctx->Driver.CreateTexture		= NULL; /* FIXME: Is this used??? */
   ctx->Driver.DeleteTexture		= radeonDeleteTexture;
   ctx->Driver.PrioritizeTexture	= NULL;
   ctx->Driver.ActiveTexture		= NULL;
   ctx->Driver.UpdateTexturePalette	= NULL;

   ctx->Driver.TexEnv			= radeonTexEnv;
   ctx->Driver.TexParameter		= radeonTexParameter;

   radeonInitTextureObjects( ctx );
}

/*@}*/
