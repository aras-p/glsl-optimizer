/*
 * (C) Copyright IBM Corporation 2002, 2004
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
 */

/**
 * \file utils.c
 * Utility functions for DRI drivers.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <string.h>
#include <stdlib.h>
#include "main/mtypes.h"
#include "main/extensions.h"
#include "glapi/dispatch.h"
#include "utils.h"

int driDispatchRemapTable[ driDispatchRemapTable_size ];

#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

#if defined(USE_PPC_ASM)
#include "ppc/common_ppc_features.h"
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



/**
 * Create the \c GL_RENDERER string for DRI drivers.
 * 
 * Almost all DRI drivers use a \c GL_RENDERER string of the form:
 *
 *    "Mesa DRI <chip> <driver date> <AGP speed) <CPU information>"
 *
 * Using the supplied chip name, driver data, and AGP speed, this function
 * creates the string.
 * 
 * \param buffer         Buffer to hold the \c GL_RENDERER string.
 * \param hardware_name  Name of the hardware.
 * \param driver_date    Driver date.
 * \param agp_mode       AGP mode (speed).
 * 
 * \returns
 * The length of the string stored in \c buffer.  This does \b not include
 * the terminating \c NUL character.
 */
unsigned
driGetRendererString( char * buffer, const char * hardware_name,
		      const char * driver_date, GLuint agp_mode )
{
#define MAX_INFO   4
   const char * cpu[MAX_INFO];
   unsigned   next = 0;
   unsigned   i;
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
      cpu[next] = " x86";
      next++;
   }
# ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      cpu[next] = (cpu_has_mmxext) ? "/MMX+" : "/MMX";
      next++;
   }
# endif
# ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      cpu[next] = (cpu_has_3dnowext) ? "/3DNow!+" : "/3DNow!";
      next++;
   }
# endif
# ifdef USE_SSE_ASM
   if ( cpu_has_xmm ) {
      cpu[next] = (cpu_has_xmm2) ? "/SSE2" : "/SSE";
      next++;
   }
# endif

#elif defined(USE_SPARC_ASM)

   cpu[0] = " SPARC";
   next = 1;

#elif defined(USE_PPC_ASM)
   if ( _mesa_ppc_cpu_features ) {
      cpu[next] = (cpu_has_64) ? " PowerPC 64" : " PowerPC";
      next++;
   }

# ifdef USE_VMX_ASM
   if ( cpu_has_vmx ) {
      cpu[next] = "/Altivec";
      next++;
   }
# endif

   if ( ! cpu_has_fpu ) {
      cpu[next] = "/No FPU";
      next++;
   }
#endif

   for ( i = 0 ; i < next ; i++ ) {
      const size_t len = strlen( cpu[i] );

      strncpy( & buffer[ offset ], cpu[i], len );
      offset += len;
   }

   return offset;
}




#define need_GL_ARB_multisample
#define need_GL_ARB_transpose_matrix
#define need_GL_ARB_window_pos
#define need_GL_EXT_compiled_vertex_array
#define need_GL_EXT_polygon_offset
#define need_GL_EXT_texture_object
#define need_GL_EXT_vertex_array
#define need_GL_MESA_window_pos

/* These are needed in *all* drivers because Mesa internally implements
 * certain functionality in terms of functions provided by these extensions.
 * For example, glBlendFunc is implemented by calling glBlendFuncSeparateEXT.
 */
#define need_GL_EXT_blend_func_separate
#define need_GL_NV_vertex_program

#include "extension_helper.h"

static const struct dri_extension all_mesa_extensions[] = {
   { "GL_ARB_multisample",           GL_ARB_multisample_functions },
   { "GL_ARB_transpose_matrix",      GL_ARB_transpose_matrix_functions },
   { "GL_ARB_window_pos",            GL_ARB_window_pos_functions },
   { "GL_EXT_blend_func_separate",   GL_EXT_blend_func_separate_functions },
   { "GL_EXT_compiled_vertex_array", GL_EXT_compiled_vertex_array_functions },
   { "GL_EXT_polygon_offset",        GL_EXT_polygon_offset_functions },
   { "GL_EXT_texture_object",        GL_EXT_texture_object_functions },
   { "GL_EXT_vertex_array",          GL_EXT_vertex_array_functions },
   { "GL_MESA_window_pos",           GL_MESA_window_pos_functions },
   { "GL_NV_vertex_program",         GL_NV_vertex_program_functions },
   { NULL,                           NULL }
};


/**
 * Enable extensions supported by the driver.
 * 
 * \bug
 * ARB_imaging isn't handled properly.  In Mesa, enabling ARB_imaging also
 * enables all the sub-extensions that are folded into it.  This means that
 * we need to add entry-points (via \c driInitSingleExtension) for those
 * new functions here.
 */
void driInitExtensions( GLcontext * ctx,
			const struct dri_extension * extensions_to_enable,
			GLboolean enable_imaging )
{
   static int first_time = 1;
   unsigned   i;

   if ( first_time ) {
      for ( i = 0 ; i < driDispatchRemapTable_size ; i++ ) {
	 driDispatchRemapTable[i] = -1;
      }

      first_time = 0;
      driInitExtensions( ctx, all_mesa_extensions, GL_FALSE );
   }

   if ( (ctx != NULL) && enable_imaging ) {
      _mesa_enable_imaging_extensions( ctx );
   }

   for ( i = 0 ; extensions_to_enable[i].name != NULL ; i++ ) {
       driInitSingleExtension( ctx, & extensions_to_enable[i] );
   }
}




/**
 * Enable and add dispatch functions for a single extension
 * 
 * \param ctx  Context where extension is to be enabled.
 * \param ext  Extension that is to be enabled.
 * 
 * \sa driInitExtensions, _mesa_enable_extension, _glapi_add_entrypoint
 *
 * \todo
 * Determine if it would be better to use \c strlen instead of the hardcoded
 * for-loops.
 */
void driInitSingleExtension( GLcontext * ctx,
			     const struct dri_extension * ext )
{
    unsigned i;


    if ( ext->functions != NULL ) {
	for ( i = 0 ; ext->functions[i].strings != NULL ; i++ ) {
	    const char * functions[16];
	    const char * parameter_signature;
	    const char * str = ext->functions[i].strings;
	    unsigned j;
	    unsigned offset;


	    /* Separate the parameter signature from the rest of the string.
	     * If the parameter signature is empty (i.e., the string starts
	     * with a NUL character), then the function has a void parameter
	     * list.
	     */
	    parameter_signature = str;
	    while ( str[0] != '\0' ) {
		str++;
	    }
	    str++;


	    /* Divide the string into the substrings that name each
	     * entry-point for the function.
	     */
	    for ( j = 0 ; j < 16 ; j++ ) {
		if ( str[0] == '\0' ) {
		    functions[j] = NULL;
		    break;
		}

		functions[j] = str;

		while ( str[0] != '\0' ) {
		    str++;
		}
		str++;
	    }


	    /* Add each entry-point to the dispatch table.
	     */
	    offset = _glapi_add_dispatch( functions, parameter_signature );
	    if (offset == -1) {
		fprintf(stderr, "DISPATCH ERROR! _glapi_add_dispatch failed "
			"to add %s!\n", functions[0]);
	    }
	    else if (ext->functions[i].remap_index != -1) {
		driDispatchRemapTable[ ext->functions[i].remap_index ] = 
		  offset;
	    }
	    else if (ext->functions[i].offset != offset) {
		fprintf(stderr, "DISPATCH ERROR! %s -> %u != %u\n",
			functions[0], offset, ext->functions[i].offset);
	    }
	}
    }

    if ( ctx != NULL ) {
	_mesa_enable_extension( ctx, ext->name );
    }
}


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
 * \param ddxExpected  Minimum DDX minor and range of DDX major version required by the driver.
 * \param drmActual    Actual DRM version supplied __driCreateNewScreen.
 * \param drmExpected  Minimum DRM version required by the driver.
 * 
 * \returns \c GL_TRUE if all version requirements are met.  Otherwise,
 *          \c GL_FALSE is returned.
 * 
 * \sa __driCreateNewScreen, driCheckDriDdxDrmVersions2, __driUtilMessage
 *
 * \todo
 * Now that the old \c driCheckDriDdxDrmVersions function is gone, this
 * function and \c driCheckDriDdxDrmVersions2 should be renamed.
 */
GLboolean
driCheckDriDdxDrmVersions3(const char * driver_name,
			   const __DRIversion * driActual,
			   const __DRIversion * driExpected,
			   const __DRIversion * ddxActual,
			   const __DRIutilversion2 * ddxExpected,
			   const __DRIversion * drmActual,
			   const __DRIversion * drmExpected)
{
   static const char format[] = "%s DRI driver expected %s version %d.%d.x "
       "but got version %d.%d.%d\n";
   static const char format2[] = "%s DRI driver expected %s version %d-%d.%d.x "
       "but got version %d.%d.%d\n";


   /* Check the DRI version */
   if ( (driActual->major != driExpected->major)
	|| (driActual->minor < driExpected->minor) ) {
      fprintf(stderr, format, driver_name, "DRI",
		       driExpected->major, driExpected->minor,
		       driActual->major, driActual->minor, driActual->patch);
      return GL_FALSE;
   }

   /* Check that the DDX driver version is compatible */
   /* for miniglx we pass in -1 so we can ignore the DDX version */
   if ( (ddxActual->major != -1) && ((ddxActual->major < ddxExpected->major_min)
	|| (ddxActual->major > ddxExpected->major_max)
	|| (ddxActual->minor < ddxExpected->minor)) ) {
      fprintf(stderr, format2, driver_name, "DDX",
		       ddxExpected->major_min, ddxExpected->major_max, ddxExpected->minor,
		       ddxActual->major, ddxActual->minor, ddxActual->patch);
      return GL_FALSE;
   }
   
   /* Check that the DRM driver version is compatible */
   if ( (drmActual->major != drmExpected->major)
	|| (drmActual->minor < drmExpected->minor) ) {
      fprintf(stderr, format, driver_name, "DRM",
		       drmExpected->major, drmExpected->minor,
		       drmActual->major, drmActual->minor, drmActual->patch);
      return GL_FALSE;
   }

   return GL_TRUE;
}

GLboolean
driCheckDriDdxDrmVersions2(const char * driver_name,
			   const __DRIversion * driActual,
			   const __DRIversion * driExpected,
			   const __DRIversion * ddxActual,
			   const __DRIversion * ddxExpected,
			   const __DRIversion * drmActual,
			   const __DRIversion * drmExpected)
{
   __DRIutilversion2 ddx_expected;
   ddx_expected.major_min = ddxExpected->major;
   ddx_expected.major_max = ddxExpected->major;
   ddx_expected.minor = ddxExpected->minor;
   ddx_expected.patch = ddxExpected->patch;
   return driCheckDriDdxDrmVersions3(driver_name, driActual,
				driExpected, ddxActual, & ddx_expected,
				drmActual, drmExpected);
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

/**
 * Creates a set of \c __GLcontextModes that a driver will expose.
 * 
 * A set of \c __GLcontextModes will be created based on the supplied
 * parameters.  The number of modes processed will be 2 *
 * \c num_depth_stencil_bits * \c num_db_modes.
 * 
 * For the most part, data is just copied from \c depth_bits, \c stencil_bits,
 * \c db_modes, and \c visType into each \c __GLcontextModes element.
 * However, the meanings of \c fb_format and \c fb_type require further
 * explanation.  The \c fb_format specifies which color components are in
 * each pixel and what the default order is.  For example, \c GL_RGB specifies
 * that red, green, blue are available and red is in the "most significant"
 * position and blue is in the "least significant".  The \c fb_type specifies
 * the bit sizes of each component and the actual ordering.  For example, if
 * \c GL_UNSIGNED_SHORT_5_6_5_REV is specified with \c GL_RGB, bits [15:11]
 * are the blue value, bits [10:5] are the green value, and bits [4:0] are
 * the red value.
 * 
 * One sublte issue is the combination of \c GL_RGB  or \c GL_BGR and either
 * of the \c GL_UNSIGNED_INT_8_8_8_8 modes.  The resulting mask values in the
 * \c __GLcontextModes structure is \b identical to the \c GL_RGBA or
 * \c GL_BGRA case, except the \c alphaMask is zero.  This means that, as
 * far as this routine is concerned, \c GL_RGB with \c GL_UNSIGNED_INT_8_8_8_8
 * still uses 32-bits.
 *
 * If in doubt, look at the tables used in the function.
 * 
 * \param ptr_to_modes  Pointer to a pointer to a linked list of
 *                      \c __GLcontextModes.  Upon completion, a pointer to
 *                      the next element to be process will be stored here.
 *                      If the function fails and returns \c GL_FALSE, this
 *                      value will be unmodified, but some elements in the
 *                      linked list may be modified.
 * \param fb_format     Format of the framebuffer.  Currently only \c GL_RGB,
 *                      \c GL_RGBA, \c GL_BGR, and \c GL_BGRA are supported.
 * \param fb_type       Type of the pixels in the framebuffer.  Currently only
 *                      \c GL_UNSIGNED_SHORT_5_6_5, 
 *                      \c GL_UNSIGNED_SHORT_5_6_5_REV,
 *                      \c GL_UNSIGNED_INT_8_8_8_8, and
 *                      \c GL_UNSIGNED_INT_8_8_8_8_REV are supported.
 * \param depth_bits    Array of depth buffer sizes to be exposed.
 * \param stencil_bits  Array of stencil buffer sizes to be exposed.
 * \param num_depth_stencil_bits  Number of entries in both \c depth_bits and
 *                      \c stencil_bits.
 * \param db_modes      Array of buffer swap modes.  If an element has a
 *                      value of \c GLX_NONE, then it represents a
 *                      single-buffered mode.  Other valid values are
 *                      \c GLX_SWAP_EXCHANGE_OML, \c GLX_SWAP_COPY_OML, and
 *                      \c GLX_SWAP_UNDEFINED_OML.  See the
 *                      GLX_OML_swap_method extension spec for more details.
 * \param num_db_modes  Number of entries in \c db_modes.
 * \param visType       GLX visual type.  Usually either \c GLX_TRUE_COLOR or
 *                      \c GLX_DIRECT_COLOR.
 * 
 * \returns
 * \c GL_TRUE on success or \c GL_FALSE on failure.  Currently the only
 * cause of failure is a bad parameter (i.e., unsupported \c fb_format or
 * \c fb_type).
 * 
 * \todo
 * There is currently no way to support packed RGB modes (i.e., modes with
 * exactly 3 bytes per pixel) or floating-point modes.  This could probably
 * be done by creating some new, private enums with clever names likes
 * \c GL_UNSIGNED_3BYTE_8_8_8, \c GL_4FLOAT_32_32_32_32, 
 * \c GL_4HALF_16_16_16_16, etc.  We can cross that bridge when we come to it.
 */
__DRIconfig **
driCreateConfigs(GLenum fb_format, GLenum fb_type,
		 const uint8_t * depth_bits, const uint8_t * stencil_bits,
		 unsigned num_depth_stencil_bits,
		 const GLenum * db_modes, unsigned num_db_modes)
{
   static const uint8_t bits_table[4][4] = {
     /* R  G  B  A */
      { 3, 3, 2, 0 }, /* Any GL_UNSIGNED_BYTE_3_3_2 */
      { 5, 6, 5, 0 }, /* Any GL_UNSIGNED_SHORT_5_6_5 */
      { 8, 8, 8, 0 }, /* Any RGB with any GL_UNSIGNED_INT_8_8_8_8 */
      { 8, 8, 8, 8 }  /* Any RGBA with any GL_UNSIGNED_INT_8_8_8_8 */
   };

   static const uint32_t masks_table_rgb[6][4] = {
      { 0x000000E0, 0x0000001C, 0x00000003, 0x00000000 }, /* 3_3_2       */
      { 0x00000007, 0x00000038, 0x000000C0, 0x00000000 }, /* 2_3_3_REV   */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5       */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5_REV   */
      { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000 }, /* 8_8_8_8     */
      { 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000 }  /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_rgba[6][4] = {
      { 0x000000E0, 0x0000001C, 0x00000003, 0x00000000 }, /* 3_3_2       */
      { 0x00000007, 0x00000038, 0x000000C0, 0x00000000 }, /* 2_3_3_REV   */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5       */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5_REV   */
      { 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF }, /* 8_8_8_8     */
      { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 }, /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_bgr[6][4] = {
      { 0x00000007, 0x00000038, 0x000000C0, 0x00000000 }, /* 3_3_2       */
      { 0x000000E0, 0x0000001C, 0x00000003, 0x00000000 }, /* 2_3_3_REV   */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5       */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5_REV   */
      { 0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000 }, /* 8_8_8_8     */
      { 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000 }, /* 8_8_8_8_REV */
   };

   static const uint32_t masks_table_bgra[6][4] = {
      { 0x00000007, 0x00000038, 0x000000C0, 0x00000000 }, /* 3_3_2       */
      { 0x000000E0, 0x0000001C, 0x00000003, 0x00000000 }, /* 2_3_3_REV   */
      { 0x0000001F, 0x000007E0, 0x0000F800, 0x00000000 }, /* 5_6_5       */
      { 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000 }, /* 5_6_5_REV   */
      { 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF }, /* 8_8_8_8     */
      { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 }, /* 8_8_8_8_REV */
   };

   static const uint8_t bytes_per_pixel[6] = {
      1, /* 3_3_2       */
      1, /* 2_3_3_REV   */
      2, /* 5_6_5       */
      2, /* 5_6_5_REV   */
      4, /* 8_8_8_8     */
      4  /* 8_8_8_8_REV */
   };

   const uint8_t  * bits;
   const uint32_t * masks;
   int index;
   __DRIconfig **configs, **c;
   __GLcontextModes *modes;
   unsigned i;
   unsigned j;
   unsigned k;
   unsigned num_modes;
   unsigned num_accum_bits = 2;

   switch ( fb_type ) {
      case GL_UNSIGNED_BYTE_3_3_2:
	 index = 0;
	 break;
      case GL_UNSIGNED_BYTE_2_3_3_REV:
	 index = 1;
	 break;
      case GL_UNSIGNED_SHORT_5_6_5:
	 index = 2;
	 break;
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 index = 3;
	 break;
      case GL_UNSIGNED_INT_8_8_8_8:
	 index = 4;
	 break;
      case GL_UNSIGNED_INT_8_8_8_8_REV:
	 index = 5;
	 break;
      default:
	 fprintf( stderr, "[%s:%u] Unknown framebuffer type 0x%04x.\n",
               __FUNCTION__, __LINE__, fb_type );
	 return NULL;
   }


   /* Valid types are GL_UNSIGNED_SHORT_5_6_5 and GL_UNSIGNED_INT_8_8_8_8 and
    * the _REV versions.
    *
    * Valid formats are GL_RGBA, GL_RGB, and GL_BGRA.
    */

   switch ( fb_format ) {
      case GL_RGB:
         masks = masks_table_rgb[ index ];
         break;

      case GL_RGBA:
         masks = masks_table_rgba[ index ];
         break;

      case GL_BGR:
         masks = masks_table_bgr[ index ];
         break;

      case GL_BGRA:
         masks = masks_table_bgra[ index ];
         break;

      default:
         fprintf( stderr, "[%s:%u] Unknown framebuffer format 0x%04x.\n",
               __FUNCTION__, __LINE__, fb_format );
         return NULL;
   }

   switch ( bytes_per_pixel[ index ] ) {
      case 1:
	 bits = bits_table[0];
	 break;
      case 2:
	 bits = bits_table[1];
	 break;
      default:
	 bits = ((fb_format == GL_RGB) || (fb_format == GL_BGR))
	    ? bits_table[2]
	    : bits_table[3];
	 break;
   }

   num_modes = num_depth_stencil_bits * num_db_modes * num_accum_bits;
   configs = _mesa_calloc((num_modes + 1) * sizeof *configs);
   if (configs == NULL)
       return NULL;

    c = configs;
    for ( k = 0 ; k < num_depth_stencil_bits ; k++ ) {
	for ( i = 0 ; i < num_db_modes ; i++ ) {
	    for ( j = 0 ; j < num_accum_bits ; j++ ) {
		*c = _mesa_malloc (sizeof **c);
		modes = &(*c)->modes;
		c++;

		memset(modes, 0, sizeof *modes);
		modes->redBits   = bits[0];
		modes->greenBits = bits[1];
		modes->blueBits  = bits[2];
		modes->alphaBits = bits[3];
		modes->redMask   = masks[0];
		modes->greenMask = masks[1];
		modes->blueMask  = masks[2];
		modes->alphaMask = masks[3];
		modes->rgbBits   = modes->redBits + modes->greenBits
		    + modes->blueBits + modes->alphaBits;

		modes->accumRedBits   = 16 * j;
		modes->accumGreenBits = 16 * j;
		modes->accumBlueBits  = 16 * j;
		modes->accumAlphaBits = (masks[3] != 0) ? 16 * j : 0;
		modes->visualRating = (j == 0) ? GLX_NONE : GLX_SLOW_CONFIG;

		modes->stencilBits = stencil_bits[k];
		modes->depthBits = depth_bits[k];

		modes->transparentPixel = GLX_NONE;
		modes->transparentRed = GLX_DONT_CARE;
		modes->transparentGreen = GLX_DONT_CARE;
		modes->transparentBlue = GLX_DONT_CARE;
		modes->transparentAlpha = GLX_DONT_CARE;
		modes->transparentIndex = GLX_DONT_CARE;
		modes->visualType = GLX_DONT_CARE;
		modes->renderType = GLX_RGBA_BIT;
		modes->drawableType = GLX_WINDOW_BIT;
		modes->rgbMode = GL_TRUE;

		if ( db_modes[i] == GLX_NONE ) {
		    modes->doubleBufferMode = GL_FALSE;
		}
		else {
		    modes->doubleBufferMode = GL_TRUE;
		    modes->swapMethod = db_modes[i];
		}

		modes->haveAccumBuffer = ((modes->accumRedBits +
					   modes->accumGreenBits +
					   modes->accumBlueBits +
					   modes->accumAlphaBits) > 0);
		modes->haveDepthBuffer = (modes->depthBits > 0);
		modes->haveStencilBuffer = (modes->stencilBits > 0);

		modes->bindToTextureRgb = GL_TRUE;
		modes->bindToTextureRgba = GL_TRUE;
		modes->bindToMipmapTexture = GL_FALSE;
		modes->bindToTextureTargets = modes->rgbMode ?
		    __DRI_ATTRIB_TEXTURE_1D_BIT |
		    __DRI_ATTRIB_TEXTURE_2D_BIT |
		    __DRI_ATTRIB_TEXTURE_RECTANGLE_BIT :
		    0;
	    }
	}
    }
    *c = NULL;

    return configs;
}

const __DRIconfig **driConcatConfigs(__DRIconfig **a, __DRIconfig **b)
{
    const __DRIconfig **all;
    int i, j, index;

    i = 0;
    while (a[i] != NULL)
	i++;
    j = 0;
    while (b[j] != NULL)
	j++;
   
    all = _mesa_malloc((i + j + 1) * sizeof *all);
    index = 0;
    for (i = 0; a[i] != NULL; i++)
	all[index++] = a[i];
    for (j = 0; b[j] != NULL; j++)
	all[index++] = b[j];
    all[index++] = NULL;

    _mesa_free(a);
    _mesa_free(b);

    return all;
}

#define __ATTRIB(attrib, field) \
    { attrib, offsetof(__GLcontextModes, field) }

static const struct { unsigned int attrib, offset; } attribMap[] = {
    __ATTRIB(__DRI_ATTRIB_BUFFER_SIZE,			rgbBits),
    __ATTRIB(__DRI_ATTRIB_LEVEL,			level),
    __ATTRIB(__DRI_ATTRIB_RED_SIZE,			redBits),
    __ATTRIB(__DRI_ATTRIB_GREEN_SIZE,			greenBits),
    __ATTRIB(__DRI_ATTRIB_BLUE_SIZE,			blueBits),
    __ATTRIB(__DRI_ATTRIB_ALPHA_SIZE,			alphaBits),
    __ATTRIB(__DRI_ATTRIB_DEPTH_SIZE,			depthBits),
    __ATTRIB(__DRI_ATTRIB_STENCIL_SIZE,			stencilBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_RED_SIZE,		accumRedBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_GREEN_SIZE,		accumGreenBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_BLUE_SIZE,		accumBlueBits),
    __ATTRIB(__DRI_ATTRIB_ACCUM_ALPHA_SIZE,		accumAlphaBits),
    __ATTRIB(__DRI_ATTRIB_SAMPLE_BUFFERS,		sampleBuffers),
    __ATTRIB(__DRI_ATTRIB_SAMPLES,			samples),
    __ATTRIB(__DRI_ATTRIB_DOUBLE_BUFFER,		doubleBufferMode),
    __ATTRIB(__DRI_ATTRIB_STEREO,			stereoMode),
    __ATTRIB(__DRI_ATTRIB_AUX_BUFFERS,			numAuxBuffers),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_TYPE,		transparentPixel),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_INDEX_VALUE,	transparentPixel),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_RED_VALUE,	transparentRed),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_GREEN_VALUE,	transparentGreen),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_BLUE_VALUE,	transparentBlue),
    __ATTRIB(__DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE,	transparentAlpha),
    __ATTRIB(__DRI_ATTRIB_FLOAT_MODE,			floatMode),
    __ATTRIB(__DRI_ATTRIB_RED_MASK,			redMask),
    __ATTRIB(__DRI_ATTRIB_GREEN_MASK,			greenMask),
    __ATTRIB(__DRI_ATTRIB_BLUE_MASK,			blueMask),
    __ATTRIB(__DRI_ATTRIB_ALPHA_MASK,			alphaMask),
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_WIDTH,		maxPbufferWidth),
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_HEIGHT,		maxPbufferHeight),
    __ATTRIB(__DRI_ATTRIB_MAX_PBUFFER_PIXELS,		maxPbufferPixels),
    __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH,	optimalPbufferWidth),
    __ATTRIB(__DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT,	optimalPbufferHeight),
    __ATTRIB(__DRI_ATTRIB_SWAP_METHOD,			swapMethod),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGB,		bindToTextureRgb),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_RGBA,		bindToTextureRgba),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE,	bindToMipmapTexture),
    __ATTRIB(__DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS,	bindToTextureTargets),
    __ATTRIB(__DRI_ATTRIB_YINVERTED,			yInverted),

    /* The struct field doesn't matter here, these are handled by the
     * switch in driGetConfigAttribIndex.  We need them in the array
     * so the iterator includes them though.*/
    __ATTRIB(__DRI_ATTRIB_RENDER_TYPE,			level),
    __ATTRIB(__DRI_ATTRIB_CONFIG_CAVEAT,		level),
    __ATTRIB(__DRI_ATTRIB_SWAP_METHOD,			level)
};

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

static int
driGetConfigAttribIndex(const __DRIconfig *config,
			unsigned int index, unsigned int *value)
{
    switch (attribMap[index].attrib) {
    case __DRI_ATTRIB_RENDER_TYPE:
	if (config->modes.rgbMode)
	    *value = __DRI_ATTRIB_RGBA_BIT;
	else
	    *value = __DRI_ATTRIB_COLOR_INDEX_BIT;
	break;
    case __DRI_ATTRIB_CONFIG_CAVEAT:
	if (config->modes.visualRating == GLX_NON_CONFORMANT_CONFIG)
	    *value = __DRI_ATTRIB_NON_CONFORMANT_CONFIG;
	else if (config->modes.visualRating == GLX_SLOW_CONFIG)
	    *value = __DRI_ATTRIB_SLOW_BIT;
	else
	    *value = 0;
	break;
    case __DRI_ATTRIB_SWAP_METHOD:
	break;

    case __DRI_ATTRIB_FLOAT_MODE:
        *value = config->modes.floatMode;
        break;

    default:
	*value = *(unsigned int *)
	    ((char *) &config->modes + attribMap[index].offset);
	
	break;
    }

    return GL_TRUE;
}

int
driGetConfigAttrib(const __DRIconfig *config,
		   unsigned int attrib, unsigned int *value)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(attribMap); i++)
	if (attribMap[i].attrib == attrib)
	    return driGetConfigAttribIndex(config, i, value);

    return GL_FALSE;
}

int
driIndexConfigAttrib(const __DRIconfig *config, int index,
		     unsigned int *attrib, unsigned int *value)
{
    if (index >= 0 && index < ARRAY_SIZE(attribMap)) {
	*attrib = attribMap[index].attrib;
	return driGetConfigAttribIndex(config, index, value);
    }

    return GL_FALSE;
}
