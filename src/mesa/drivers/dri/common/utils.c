/*
 * (c) Copyright IBM Corporation 2002
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
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 */
/* $XFree86:$ */

#include <string.h>
#include <stdlib.h>
#include "mtypes.h"
#include "extensions.h"
#include "utils.h"

#if !defined( DRI_NEW_INTERFACE_ONLY ) && !defined( _SOLO )
#include "xf86dri.h"        /* For XF86DRIQueryVersion prototype. */
#endif

#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

unsigned
driParseDebugString( const char * debug, 
		     const struct dri_debug_control * control  )
{
   unsigned   flag;


   flag = 0;
   if ( debug != NULL ) {
      while( control->string != NULL ) {
	 if ( !strcmp( debug, "all" ) ||
	      strstr( debug, control->string ) != NULL ) {
	    flag |= control->flag;
	 }

	 control++;
      }
   }

   return flag;
}




unsigned
driGetRendererString( char * buffer, const char * hardware_name,
		      const char * driver_date, GLuint agp_mode )
{
#ifdef USE_X86_ASM
   char * x86_str = "";
   char * mmx_str = "";
   char * tdnow_str = "";
   char * sse_str = "";
#endif
   unsigned   offset;


   offset = sprintf( buffer, "Mesa DRI %s %s", hardware_name, driver_date );

   /* Append any AGP-specific information.
    */
   switch ( agp_mode ) {
   case 1:
   case 2:
   case 4:
   case 8:
      offset += sprintf( & buffer[ offset ], " AGP %ux", agp_mode );
      break;
	
   default:
      break;
   }

   /* Append any CPU-specific information.
    */
#ifdef USE_X86_ASM
   if ( _mesa_x86_cpu_features ) {
      x86_str = " x86";
   }
# ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      mmx_str = (cpu_has_mmxext) ? "/MMX+" : "/MMX";
   }
# endif
# ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      tdnow_str = (cpu_has_3dnowext) ? "/3DNow!+" : "/3DNow!";
   }
# endif
# ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      sse_str = (cpu_has_xmm2) ? "/SSE2" : "/SSE";
   }
# endif

   offset += sprintf( & buffer[ offset ], "%s%s%s%s", 
		      x86_str, mmx_str, tdnow_str, sse_str );

#elif defined(USE_SPARC_ASM)

   offset += sprintf( & buffer[ offset ], " Sparc" );

#endif

   return offset;
}




void driInitExtensions( GLcontext * ctx,
			const char * const extensions_to_enable[],
			GLboolean  enable_imaging )
{
   unsigned   i;

   if ( enable_imaging ) {
      _mesa_enable_imaging_extensions( ctx );
   }

   for ( i = 0 ; extensions_to_enable[i] != NULL ; i++ ) {
      _mesa_enable_extension( ctx, extensions_to_enable[i] );
   }
}




#ifndef DRI_NEW_INTERFACE_ONLY
/**
 * Utility function used by drivers to test the verions of other components.
 * 
 * \deprecated
 * All drivers using the new interface should use \c driCheckDriDdxVersions2
 * instead.  This function is implemented using a call that is not available
 * to drivers using the new interface.  Furthermore, the information gained
 * by this call (the DRI and DDX version information) is already provided to
 * the driver via the new interface.
 */
GLboolean
driCheckDriDdxDrmVersions(__DRIscreenPrivate *sPriv,
			  const char * driver_name,
			  int dri_major, int dri_minor,
			  int ddx_major, int ddx_minor,
			  int drm_major, int drm_minor)
{
   static const char format[] = "%s DRI driver expected %s version %d.%d.x "
       "but got version %d.%d.%d";
   int major, minor, patch;

#ifndef _SOLO
   /* Check the DRI version */
   if (XF86DRIQueryVersion(sPriv->display, &major, &minor, &patch)) {
      if (major != dri_major || minor < dri_minor) {
	 __driUtilMessage(format, driver_name, "DRI", dri_major, dri_minor,
			  major, minor, patch);
	 return GL_FALSE;
      }
   }

   /* Check that the DDX driver version is compatible */
   if (sPriv->ddxMajor != ddx_major || sPriv->ddxMinor < ddx_minor) {
      __driUtilMessage(format, driver_name, "DDX", ddx_major, ddx_minor,
		       sPriv->ddxMajor, sPriv->ddxMinor, sPriv->ddxPatch);
      return GL_FALSE;
   }
#else
   (void)major;(void)minor;(void)patch;
#endif
   
   /* Check that the DRM driver version is compatible */
   if (sPriv->drmMajor != drm_major || sPriv->drmMinor < drm_minor) {
      __driUtilMessage(format, driver_name, "DRM", drm_major, drm_minor,
		       sPriv->drmMajor, sPriv->drmMinor, sPriv->drmPatch);
      return GL_FALSE;
   }

   return GL_TRUE;
}
#endif /* DRI_NEW_INTERFACE_ONLY */

/**
 * Utility function used by drivers to test the verions of other components.
 *
 * If one of the version requirements is not met, a message is logged using
 * \c __driUtilMessage.
 *
 * \param driver_name  Name of the driver.  Used in error messages.
 * \param driActual    Actual DRI version supplied __driCreateNewScreen.
 * \param driExpected  Minimum DRI version required by the driver.
 * \param ddxActual    Actual DDX version supplied __driCreateNewScreen.
 * \param ddxExpected  Minimum DDX version required by the driver.
 * \param drmActual    Actual DRM version supplied __driCreateNewScreen.
 * \param drmExpected  Minimum DRM version required by the driver.
 * 
 * \returns \c GL_TRUE if all version requirements are met.  Otherwise,
 *          \c GL_FALSE is returned.
 * 
 * \sa __driCreateNewScreen, driCheckDriDdxDrmVersions, __driUtilMessage
 */
GLboolean
driCheckDriDdxDrmVersions2(const char * driver_name,
			   const __DRIversion * driActual,
			   const __DRIversion * driExpected,
			   const __DRIversion * ddxActual,
			   const __DRIversion * ddxExpected,
			   const __DRIversion * drmActual,
			   const __DRIversion * drmExpected)
{
   static const char format[] = "%s DRI driver expected %s version %d.%d.x "
       "but got version %d.%d.%d";


   /* Check the DRI version */
   if ( (driActual->major != driExpected->major)
	|| (driActual->minor < driExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DRI",
		       driExpected->major, driExpected->minor,
		       driActual->major, driActual->minor, driActual->patch);
      return GL_FALSE;
   }

   /* Check that the DDX driver version is compatible */
   if ( (ddxActual->major != ddxExpected->major)
	|| (ddxActual->minor < ddxExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DDX",
		       ddxExpected->major, ddxExpected->minor,
		       ddxActual->major, ddxActual->minor, ddxActual->patch);
      return GL_FALSE;
   }
   
   /* Check that the DRM driver version is compatible */
   if ( (drmActual->major != drmExpected->major)
	|| (drmActual->minor < drmExpected->minor) ) {
      __driUtilMessage(format, driver_name, "DRM",
		       drmExpected->major, drmExpected->minor,
		       drmActual->major, drmActual->minor, drmActual->patch);
      return GL_FALSE;
   }

   return GL_TRUE;
}


GLboolean driClipRectToFramebuffer( const GLframebuffer *buffer,
				    GLint *x, GLint *y,
				    GLsizei *width, GLsizei *height )
{
   /* left clipping */
   if (*x < buffer->_Xmin) {
      *width -= (buffer->_Xmin - *x);
      *x = buffer->_Xmin;
   }

   /* right clipping */
   if (*x + *width > buffer->_Xmax)
      *width -= (*x + *width - buffer->_Xmax - 1);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*y < buffer->_Ymin) {
      *height -= (buffer->_Ymin - *y);
      *y = buffer->_Ymin;
   }

   /* top clipping */
   if (*y + *height > buffer->_Ymax)
      *height -= (*y + *height - buffer->_Ymax - 1);

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}
