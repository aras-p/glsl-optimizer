/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgadd.c,v 1.14 2002/10/30 12:51:35 alanh Exp $ */


#include "mtypes.h"


#include <stdio.h>
#include <stdlib.h>

#include "mm.h"
#include "mgacontext.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgaspan.h"
#include "mgatex.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mga_xmesa.h"
#include "extensions.h"
#if defined(USE_X86_ASM)
#include "X86/common_x86_asm.h"
#endif

#define MGA_DATE	"20020221"


/***************************************
 * Mesa's Driver Functions
 ***************************************/


static const GLubyte *mgaDDGetString( GLcontext *ctx, GLenum name )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   static char buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *) "VA Linux Systems Inc.";

   case GL_RENDERER:
      sprintf( buffer, "Mesa DRI %s " MGA_DATE,
	       MGA_IS_G400(mmesa) ? "G400" :
	       MGA_IS_G200(mmesa) ? "G200" : "MGA" );

      /* Append any AGP-specific information.
       */
      switch ( mmesa->mgaScreen->agpMode ) {
      case 1:
	 strncat( buffer, " AGP 1x", 7 );
	 break;
      case 2:
	 strncat( buffer, " AGP 2x", 7 );
	 break;
      case 4:
	 strncat( buffer, " AGP 4x", 7 );
	 break;
      }

      /* Append any CPU-specific information.
       */
#ifdef USE_X86_ASM
      if ( _mesa_x86_cpu_features ) {
	 strncat( buffer, " x86", 4 );
      }
#endif
#ifdef USE_MMX_ASM
      if ( cpu_has_mmx ) {
	 strncat( buffer, "/MMX", 4 );
      }
#endif
#ifdef USE_3DNOW_ASM
      if ( cpu_has_3dnow ) {
	 strncat( buffer, "/3DNow!", 7 );
      }
#endif
#ifdef USE_SSE_ASM
      if ( cpu_has_xmm ) {
	 strncat( buffer, "/SSE", 4 );
      }
#endif
      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}



static void mgaBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   /* Need to lock to make sure the driDrawable is uptodate.  This
    * information is used to resize Mesa's software buffers, so it has
    * to be correct.
    */
   LOCK_HARDWARE( mmesa );
   *width = mmesa->driDrawable->w;
   *height = mmesa->driDrawable->h;
   UNLOCK_HARDWARE( mmesa );
}

void mgaDDExtensionsInit( GLcontext *ctx )
{
   /* paletted_textures currently doesn't work, but we could fix them later */
   /*
   _mesa_enable_extension( ctx, "GL_EXT_shared_texture_palette" );
   _mesa_enable_extension( ctx, "GL_EXT_paletted_texture" );
   */

   _mesa_enable_extension( ctx, "GL_ARB_texture_compression" );
   _mesa_enable_extension( ctx, "GL_ARB_multisample" );

   _mesa_enable_extension( ctx, "GL_SGIS_generate_mipmap" );

   /* Turn on multitexture and texenv_add for the G400.
    */
   if (MGA_IS_G400(MGA_CONTEXT(ctx))) {
      _mesa_enable_extension( ctx, "GL_ARB_multitexture" );
      _mesa_enable_extension( ctx, "GL_ARB_texture_env_add" );

      _mesa_enable_extension( ctx, "GL_EXT_texture_env_add" );

#if defined (MESA_packed_depth_stencil)
      _mesa_enable_extension( ctx, "GL_MESA_packed_depth_stencil" );
#endif

#if defined (MESA_experimetal_agp_allocator)
      if (!getenv("MGA_DISABLE_AGP_ALLOCATOR"))  
	 _mesa_enable_extension( ctx, "GL_MESA_experimental_agp_allocator" );
#endif
   }
}



void mgaDDInitDriverFuncs( GLcontext *ctx )
{
   ctx->Driver.GetBufferSize = mgaBufferSize;
   ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
   ctx->Driver.GetString = mgaDDGetString;
}
