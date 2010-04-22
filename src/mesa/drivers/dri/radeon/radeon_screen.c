/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file radeon_screen.c
 * Screen initialization functions for the Radeon driver.
 *
 * \author Kevin E. Martin <martin@valinux.com>
 * \author  Gareth Hughes <gareth@valinux.com>
 */

#include <errno.h>
#include "main/glheader.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"

#define STANDALONE_MMIO
#include "radeon_chipset.h"
#include "radeon_macros.h"
#include "radeon_screen.h"
#include "radeon_common.h"
#if defined(RADEON_R100)
#include "radeon_context.h"
#include "radeon_tex.h"
#elif defined(RADEON_R200)
#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_tex.h"
#elif defined(RADEON_R300)
#include "r300_context.h"
#include "r300_tex.h"
#elif defined(RADEON_R600)
#include "r600_context.h"
#include "r700_driconf.h" /* +r6/r7 */
#include "r600_tex.h"     /* +r6/r7 */
#endif

#include "utils.h"
#include "vblank.h"

#include "radeon_bocs_wrapper.h"

#include "GL/internal/dri_interface.h"

/* Radeon configuration
 */
#include "xmlpool.h"

#define DRI_CONF_COMMAND_BUFFER_SIZE(def,min,max) \
DRI_CONF_OPT_BEGIN_V(command_buffer_size,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Size of command buffer (in KB)") \
        DRI_CONF_DESC(de,"Grösse des Befehlspuffers (in KB)") \
DRI_CONF_OPT_END

#if defined(RADEON_R100)	/* R100 */
PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
        DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
        DRI_CONF_MAX_TEXTURE_UNITS(3,2,3)
        DRI_CONF_HYPERZ(false)
        DRI_CONF_COMMAND_BUFFER_SIZE(8, 8, 32)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_DEF_MAX_ANISOTROPY(1.0,"1.0,2.0,4.0,8.0,16.0")
        DRI_CONF_NO_NEG_LOD_BIAS(false)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
        DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
        DRI_CONF_ALLOW_LARGE_TEXTURES(2)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 15;

#elif defined(RADEON_R200)

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
        DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
        DRI_CONF_MAX_TEXTURE_UNITS(6,2,6)
        DRI_CONF_HYPERZ(false)
        DRI_CONF_COMMAND_BUFFER_SIZE(8, 8, 32)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_DEF_MAX_ANISOTROPY(1.0,"1.0,2.0,4.0,8.0,16.0")
        DRI_CONF_NO_NEG_LOD_BIAS(false)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
        DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
        DRI_CONF_ALLOW_LARGE_TEXTURES(2)
        DRI_CONF_TEXTURE_BLEND_QUALITY(1.0,"0.0:1.0")
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_SOFTWARE
        DRI_CONF_NV_VERTEX_PROGRAM(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 17;

#elif defined(RADEON_R300) || defined(RADEON_R600)

#define DRI_CONF_FP_OPTIMIZATION_SPEED   0
#define DRI_CONF_FP_OPTIMIZATION_QUALITY 1

/* TODO: integrate these into xmlpool.h! */
#define DRI_CONF_MAX_TEXTURE_IMAGE_UNITS(def,min,max) \
DRI_CONF_OPT_BEGIN_V(texture_image_units,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Number of texture image units") \
        DRI_CONF_DESC(de,"Anzahl der Textureinheiten") \
DRI_CONF_OPT_END

#define DRI_CONF_MAX_TEXTURE_COORD_UNITS(def,min,max) \
DRI_CONF_OPT_BEGIN_V(texture_coord_units,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Number of texture coordinate units") \
        DRI_CONF_DESC(de,"Anzahl der Texturkoordinateneinheiten") \
DRI_CONF_OPT_END



#define DRI_CONF_DISABLE_S3TC(def) \
DRI_CONF_OPT_BEGIN(disable_s3tc,bool,def) \
        DRI_CONF_DESC(en,"Disable S3TC compression") \
DRI_CONF_OPT_END

#define DRI_CONF_DISABLE_FALLBACK(def) \
DRI_CONF_OPT_BEGIN(disable_lowimpact_fallback,bool,def) \
        DRI_CONF_DESC(en,"Disable Low-impact fallback") \
DRI_CONF_OPT_END

#define DRI_CONF_DISABLE_DOUBLE_SIDE_STENCIL(def) \
DRI_CONF_OPT_BEGIN(disable_stencil_two_side,bool,def) \
        DRI_CONF_DESC(en,"Disable GL_EXT_stencil_two_side") \
DRI_CONF_OPT_END

#define DRI_CONF_FP_OPTIMIZATION(def) \
DRI_CONF_OPT_BEGIN_V(fp_optimization,enum,def,"0:1") \
	DRI_CONF_DESC_BEGIN(en,"Fragment Program optimization") \
                DRI_CONF_ENUM(0,"Optimize for Speed") \
                DRI_CONF_ENUM(1,"Optimize for Quality") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
	DRI_CONF_SECTION_PERFORMANCE
		DRI_CONF_TCL_MODE(DRI_CONF_TCL_CODEGEN)
		DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
		DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
		DRI_CONF_MAX_TEXTURE_IMAGE_UNITS(8, 2, 8)
		DRI_CONF_MAX_TEXTURE_COORD_UNITS(8, 2, 8)
		DRI_CONF_COMMAND_BUFFER_SIZE(8, 8, 32)
		DRI_CONF_DISABLE_FALLBACK(true)
		DRI_CONF_DISABLE_DOUBLE_SIDE_STENCIL(false)
	DRI_CONF_SECTION_END
	DRI_CONF_SECTION_QUALITY
		DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
		DRI_CONF_DEF_MAX_ANISOTROPY(1.0, "1.0,2.0,4.0,8.0,16.0")
		DRI_CONF_FORCE_S3TC_ENABLE(false)
		DRI_CONF_DISABLE_S3TC(false)
		DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
		DRI_CONF_ROUND_MODE(DRI_CONF_ROUND_TRUNC)
		DRI_CONF_DITHER_MODE(DRI_CONF_DITHER_XERRORDIFF)
		DRI_CONF_FP_OPTIMIZATION(DRI_CONF_FP_OPTIMIZATION_SPEED)
	DRI_CONF_SECTION_END
	DRI_CONF_SECTION_DEBUG
		DRI_CONF_NO_RAST(false)
	DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 17;

#endif

static int getSwapInfo( __DRIdrawable *dPriv, __DRIswapInfo * sInfo );

static int
radeonGetParam(__DRIscreen *sPriv, int param, void *value)
{
  int ret;
  drm_radeon_getparam_t gp = { 0 };
  struct drm_radeon_info info = { 0 };

  if (sPriv->drm_version.major >= 2) {
      info.value = (uint64_t)(uintptr_t)value;
      switch (param) {
      case RADEON_PARAM_DEVICE_ID:
          info.request = RADEON_INFO_DEVICE_ID;
          break;
      case RADEON_PARAM_NUM_GB_PIPES:
          info.request = RADEON_INFO_NUM_GB_PIPES;
          break;
      case RADEON_PARAM_NUM_Z_PIPES:
          info.request = RADEON_INFO_NUM_Z_PIPES;
          break;
      default:
          return -EINVAL;
      }
      ret = drmCommandWriteRead(sPriv->fd, DRM_RADEON_INFO, &info, sizeof(info));
  } else {
      gp.param = param;
      gp.value = value;

      ret = drmCommandWriteRead(sPriv->fd, DRM_RADEON_GETPARAM, &gp, sizeof(gp));
  }
  return ret;
}

static const __DRIconfig **
radeonFillInModes( __DRIscreen *psp,
		   unsigned pixel_bits, unsigned depth_bits,
		   unsigned stencil_bits, GLboolean have_back_buffer )
{
    __DRIconfig **configs;
    __GLcontextModes *m;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    int i;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    uint8_t depth_bits_array[2];
    uint8_t stencil_bits_array[2];
    uint8_t msaa_samples_array[1];

    depth_bits_array[0] = depth_bits;
    depth_bits_array[1] = depth_bits;

    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    stencil_bits_array[0] = stencil_bits;
    stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

    msaa_samples_array[0] = 0;

    depth_buffer_factor = (stencil_bits == 0) ? 2 : 1;
    back_buffer_factor  = (have_back_buffer) ? 2 : 1;

    if (pixel_bits == 16) {
	__DRIconfig **configs_a8r8g8b8;
	__DRIconfig **configs_r5g6b5;

	configs_r5g6b5 = driCreateConfigs(GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
					  depth_bits_array, stencil_bits_array,
					  depth_buffer_factor, back_buffer_modes,
					  back_buffer_factor, msaa_samples_array,
					  1, GL_TRUE);
	configs_a8r8g8b8 = driCreateConfigs(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
					    depth_bits_array, stencil_bits_array,
					    1, back_buffer_modes, 1,
					    msaa_samples_array, 1, GL_TRUE);
	configs = driConcatConfigs(configs_r5g6b5, configs_a8r8g8b8);
   } else
	configs = driCreateConfigs(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
				   depth_bits_array, stencil_bits_array,
				   depth_buffer_factor,
				   back_buffer_modes, back_buffer_factor,
				   msaa_samples_array, 1, GL_TRUE);

    if (configs == NULL) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    /* Mark the visual as slow if there are "fake" stencil bits.
     */
    for (i = 0; configs[i]; i++) {
	m = &configs[i]->modes;
	if ((m->stencilBits != 0) && (m->stencilBits != stencil_bits)) {
	    m->visualRating = GLX_SLOW_CONFIG;
	}
    }

    return (const __DRIconfig **) configs;
}

#if defined(RADEON_R100)
static const __DRItexOffsetExtension radeonTexOffsetExtension = {
    { __DRI_TEX_OFFSET, __DRI_TEX_OFFSET_VERSION },
    radeonSetTexOffset,
};

static const __DRItexBufferExtension radeonTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   radeonSetTexBuffer,
   radeonSetTexBuffer2,
};
#endif

#if defined(RADEON_R200)
static const __DRIallocateExtension r200AllocateExtension = {
    { __DRI_ALLOCATE, __DRI_ALLOCATE_VERSION },
    r200AllocateMemoryMESA,
    r200FreeMemoryMESA,
    r200GetMemoryOffsetMESA
};

static const __DRItexOffsetExtension r200texOffsetExtension = {
    { __DRI_TEX_OFFSET, __DRI_TEX_OFFSET_VERSION },
   r200SetTexOffset,
};

static const __DRItexBufferExtension r200TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   r200SetTexBuffer,
   r200SetTexBuffer2,
};
#endif

#if defined(RADEON_R300)
static const __DRItexOffsetExtension r300texOffsetExtension = {
    { __DRI_TEX_OFFSET, __DRI_TEX_OFFSET_VERSION },
   r300SetTexOffset,
};

static const __DRItexBufferExtension r300TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   r300SetTexBuffer,
   r300SetTexBuffer2,
};
#endif

#if defined(RADEON_R600)
static const __DRItexOffsetExtension r600texOffsetExtension = {
    { __DRI_TEX_OFFSET, __DRI_TEX_OFFSET_VERSION },
   r600SetTexOffset, /* +r6/r7 */
};

static const __DRItexBufferExtension r600TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   r600SetTexBuffer,  /* +r6/r7 */
   r600SetTexBuffer2, /* +r6/r7 */
};
#endif

static int radeon_set_screen_flags(radeonScreenPtr screen, int device_id)
{
   screen->device_id = device_id;
   screen->chip_flags = 0;
   switch ( device_id ) {
   case PCI_CHIP_RN50_515E:
   case PCI_CHIP_RN50_5969:
	return -1;

   case PCI_CHIP_RADEON_LY:
   case PCI_CHIP_RADEON_LZ:
   case PCI_CHIP_RADEON_QY:
   case PCI_CHIP_RADEON_QZ:
      screen->chip_family = CHIP_FAMILY_RV100;
      break;

   case PCI_CHIP_RS100_4136:
   case PCI_CHIP_RS100_4336:
      screen->chip_family = CHIP_FAMILY_RS100;
      break;

   case PCI_CHIP_RS200_4137:
   case PCI_CHIP_RS200_4337:
   case PCI_CHIP_RS250_4237:
   case PCI_CHIP_RS250_4437:
      screen->chip_family = CHIP_FAMILY_RS200;
      break;

   case PCI_CHIP_RADEON_QD:
   case PCI_CHIP_RADEON_QE:
   case PCI_CHIP_RADEON_QF:
   case PCI_CHIP_RADEON_QG:
      /* all original radeons (7200) presumably have a stencil op bug */
      screen->chip_family = CHIP_FAMILY_R100;
      screen->chip_flags = RADEON_CHIPSET_TCL | RADEON_CHIPSET_BROKEN_STENCIL;
      break;

   case PCI_CHIP_RV200_QW:
   case PCI_CHIP_RV200_QX:
   case PCI_CHIP_RADEON_LW:
   case PCI_CHIP_RADEON_LX:
      screen->chip_family = CHIP_FAMILY_RV200;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_R200_BB:
   case PCI_CHIP_R200_BC:
   case PCI_CHIP_R200_QH:
   case PCI_CHIP_R200_QL:
   case PCI_CHIP_R200_QM:
      screen->chip_family = CHIP_FAMILY_R200;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV250_If:
   case PCI_CHIP_RV250_Ig:
   case PCI_CHIP_RV250_Ld:
   case PCI_CHIP_RV250_Lf:
   case PCI_CHIP_RV250_Lg:
      screen->chip_family = CHIP_FAMILY_RV250;
      screen->chip_flags = R200_CHIPSET_YCBCR_BROKEN | RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV280_5960:
   case PCI_CHIP_RV280_5961:
   case PCI_CHIP_RV280_5962:
   case PCI_CHIP_RV280_5964:
   case PCI_CHIP_RV280_5965:
   case PCI_CHIP_RV280_5C61:
   case PCI_CHIP_RV280_5C63:
      screen->chip_family = CHIP_FAMILY_RV280;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RS300_5834:
   case PCI_CHIP_RS300_5835:
   case PCI_CHIP_RS350_7834:
   case PCI_CHIP_RS350_7835:
      screen->chip_family = CHIP_FAMILY_RS300;
      break;

   case PCI_CHIP_R300_AD:
   case PCI_CHIP_R300_AE:
   case PCI_CHIP_R300_AF:
   case PCI_CHIP_R300_AG:
   case PCI_CHIP_R300_ND:
   case PCI_CHIP_R300_NE:
   case PCI_CHIP_R300_NF:
   case PCI_CHIP_R300_NG:
      screen->chip_family = CHIP_FAMILY_R300;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV350_AP:
   case PCI_CHIP_RV350_AQ:
   case PCI_CHIP_RV350_AR:
   case PCI_CHIP_RV350_AS:
   case PCI_CHIP_RV350_AT:
   case PCI_CHIP_RV350_AV:
   case PCI_CHIP_RV350_AU:
   case PCI_CHIP_RV350_NP:
   case PCI_CHIP_RV350_NQ:
   case PCI_CHIP_RV350_NR:
   case PCI_CHIP_RV350_NS:
   case PCI_CHIP_RV350_NT:
   case PCI_CHIP_RV350_NV:
      screen->chip_family = CHIP_FAMILY_RV350;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_R350_AH:
   case PCI_CHIP_R350_AI:
   case PCI_CHIP_R350_AJ:
   case PCI_CHIP_R350_AK:
   case PCI_CHIP_R350_NH:
   case PCI_CHIP_R350_NI:
   case PCI_CHIP_R360_NJ:
   case PCI_CHIP_R350_NK:
      screen->chip_family = CHIP_FAMILY_R350;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV370_5460:
   case PCI_CHIP_RV370_5462:
   case PCI_CHIP_RV370_5464:
   case PCI_CHIP_RV370_5B60:
   case PCI_CHIP_RV370_5B62:
   case PCI_CHIP_RV370_5B63:
   case PCI_CHIP_RV370_5B64:
   case PCI_CHIP_RV370_5B65:
   case PCI_CHIP_RV380_3150:
   case PCI_CHIP_RV380_3152:
   case PCI_CHIP_RV380_3154:
   case PCI_CHIP_RV380_3E50:
   case PCI_CHIP_RV380_3E54:
      screen->chip_family = CHIP_FAMILY_RV380;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_R420_JN:
   case PCI_CHIP_R420_JH:
   case PCI_CHIP_R420_JI:
   case PCI_CHIP_R420_JJ:
   case PCI_CHIP_R420_JK:
   case PCI_CHIP_R420_JL:
   case PCI_CHIP_R420_JM:
   case PCI_CHIP_R420_JO:
   case PCI_CHIP_R420_JP:
   case PCI_CHIP_R420_JT:
   case PCI_CHIP_R481_4B49:
   case PCI_CHIP_R481_4B4A:
   case PCI_CHIP_R481_4B4B:
   case PCI_CHIP_R481_4B4C:
   case PCI_CHIP_R423_UH:
   case PCI_CHIP_R423_UI:
   case PCI_CHIP_R423_UJ:
   case PCI_CHIP_R423_UK:
   case PCI_CHIP_R430_554C:
   case PCI_CHIP_R430_554D:
   case PCI_CHIP_R430_554E:
   case PCI_CHIP_R430_554F:
   case PCI_CHIP_R423_5550:
   case PCI_CHIP_R423_UQ:
   case PCI_CHIP_R423_UR:
   case PCI_CHIP_R423_UT:
   case PCI_CHIP_R430_5D48:
   case PCI_CHIP_R430_5D49:
   case PCI_CHIP_R430_5D4A:
   case PCI_CHIP_R480_5D4C:
   case PCI_CHIP_R480_5D4D:
   case PCI_CHIP_R480_5D4E:
   case PCI_CHIP_R480_5D4F:
   case PCI_CHIP_R480_5D50:
   case PCI_CHIP_R480_5D52:
   case PCI_CHIP_R423_5D57:
      screen->chip_family = CHIP_FAMILY_R420;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV410_5E4C:
   case PCI_CHIP_RV410_5E4F:
   case PCI_CHIP_RV410_564A:
   case PCI_CHIP_RV410_564B:
   case PCI_CHIP_RV410_564F:
   case PCI_CHIP_RV410_5652:
   case PCI_CHIP_RV410_5653:
   case PCI_CHIP_RV410_5657:
   case PCI_CHIP_RV410_5E48:
   case PCI_CHIP_RV410_5E4A:
   case PCI_CHIP_RV410_5E4B:
   case PCI_CHIP_RV410_5E4D:
      screen->chip_family = CHIP_FAMILY_RV410;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RS480_5954:
   case PCI_CHIP_RS480_5955:
   case PCI_CHIP_RS482_5974:
   case PCI_CHIP_RS482_5975:
   case PCI_CHIP_RS400_5A41:
   case PCI_CHIP_RS400_5A42:
   case PCI_CHIP_RC410_5A61:
   case PCI_CHIP_RC410_5A62:
      screen->chip_family = CHIP_FAMILY_RS400;
      break;

   case PCI_CHIP_RS600_793F:
   case PCI_CHIP_RS600_7941:
   case PCI_CHIP_RS600_7942:
      screen->chip_family = CHIP_FAMILY_RS600;
      break;

   case PCI_CHIP_RS690_791E:
   case PCI_CHIP_RS690_791F:
      screen->chip_family = CHIP_FAMILY_RS690;
      break;
   case PCI_CHIP_RS740_796C:
   case PCI_CHIP_RS740_796D:
   case PCI_CHIP_RS740_796E:
   case PCI_CHIP_RS740_796F:
      screen->chip_family = CHIP_FAMILY_RS740;
      break;

   case PCI_CHIP_R520_7100:
   case PCI_CHIP_R520_7101:
   case PCI_CHIP_R520_7102:
   case PCI_CHIP_R520_7103:
   case PCI_CHIP_R520_7104:
   case PCI_CHIP_R520_7105:
   case PCI_CHIP_R520_7106:
   case PCI_CHIP_R520_7108:
   case PCI_CHIP_R520_7109:
   case PCI_CHIP_R520_710A:
   case PCI_CHIP_R520_710B:
   case PCI_CHIP_R520_710C:
   case PCI_CHIP_R520_710E:
   case PCI_CHIP_R520_710F:
      screen->chip_family = CHIP_FAMILY_R520;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV515_7140:
   case PCI_CHIP_RV515_7141:
   case PCI_CHIP_RV515_7142:
   case PCI_CHIP_RV515_7143:
   case PCI_CHIP_RV515_7144:
   case PCI_CHIP_RV515_7145:
   case PCI_CHIP_RV515_7146:
   case PCI_CHIP_RV515_7147:
   case PCI_CHIP_RV515_7149:
   case PCI_CHIP_RV515_714A:
   case PCI_CHIP_RV515_714B:
   case PCI_CHIP_RV515_714C:
   case PCI_CHIP_RV515_714D:
   case PCI_CHIP_RV515_714E:
   case PCI_CHIP_RV515_714F:
   case PCI_CHIP_RV515_7151:
   case PCI_CHIP_RV515_7152:
   case PCI_CHIP_RV515_7153:
   case PCI_CHIP_RV515_715E:
   case PCI_CHIP_RV515_715F:
   case PCI_CHIP_RV515_7180:
   case PCI_CHIP_RV515_7181:
   case PCI_CHIP_RV515_7183:
   case PCI_CHIP_RV515_7186:
   case PCI_CHIP_RV515_7187:
   case PCI_CHIP_RV515_7188:
   case PCI_CHIP_RV515_718A:
   case PCI_CHIP_RV515_718B:
   case PCI_CHIP_RV515_718C:
   case PCI_CHIP_RV515_718D:
   case PCI_CHIP_RV515_718F:
   case PCI_CHIP_RV515_7193:
   case PCI_CHIP_RV515_7196:
   case PCI_CHIP_RV515_719B:
   case PCI_CHIP_RV515_719F:
   case PCI_CHIP_RV515_7200:
   case PCI_CHIP_RV515_7210:
   case PCI_CHIP_RV515_7211:
      screen->chip_family = CHIP_FAMILY_RV515;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV530_71C0:
   case PCI_CHIP_RV530_71C1:
   case PCI_CHIP_RV530_71C2:
   case PCI_CHIP_RV530_71C3:
   case PCI_CHIP_RV530_71C4:
   case PCI_CHIP_RV530_71C5:
   case PCI_CHIP_RV530_71C6:
   case PCI_CHIP_RV530_71C7:
   case PCI_CHIP_RV530_71CD:
   case PCI_CHIP_RV530_71CE:
   case PCI_CHIP_RV530_71D2:
   case PCI_CHIP_RV530_71D4:
   case PCI_CHIP_RV530_71D5:
   case PCI_CHIP_RV530_71D6:
   case PCI_CHIP_RV530_71DA:
   case PCI_CHIP_RV530_71DE:
      screen->chip_family = CHIP_FAMILY_RV530;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_R580_7240:
   case PCI_CHIP_R580_7243:
   case PCI_CHIP_R580_7244:
   case PCI_CHIP_R580_7245:
   case PCI_CHIP_R580_7246:
   case PCI_CHIP_R580_7247:
   case PCI_CHIP_R580_7248:
   case PCI_CHIP_R580_7249:
   case PCI_CHIP_R580_724A:
   case PCI_CHIP_R580_724B:
   case PCI_CHIP_R580_724C:
   case PCI_CHIP_R580_724D:
   case PCI_CHIP_R580_724E:
   case PCI_CHIP_R580_724F:
   case PCI_CHIP_R580_7284:
      screen->chip_family = CHIP_FAMILY_R580;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV570_7280:
   case PCI_CHIP_RV560_7281:
   case PCI_CHIP_RV560_7283:
   case PCI_CHIP_RV560_7287:
   case PCI_CHIP_RV570_7288:
   case PCI_CHIP_RV570_7289:
   case PCI_CHIP_RV570_728B:
   case PCI_CHIP_RV570_728C:
   case PCI_CHIP_RV560_7290:
   case PCI_CHIP_RV560_7291:
   case PCI_CHIP_RV560_7293:
   case PCI_CHIP_RV560_7297:
      screen->chip_family = CHIP_FAMILY_RV560;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_R600_9400:
   case PCI_CHIP_R600_9401:
   case PCI_CHIP_R600_9402:
   case PCI_CHIP_R600_9403:
   case PCI_CHIP_R600_9405:
   case PCI_CHIP_R600_940A:
   case PCI_CHIP_R600_940B:
   case PCI_CHIP_R600_940F:
      screen->chip_family = CHIP_FAMILY_R600;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV610_94C0:
   case PCI_CHIP_RV610_94C1:
   case PCI_CHIP_RV610_94C3:
   case PCI_CHIP_RV610_94C4:
   case PCI_CHIP_RV610_94C5:
   case PCI_CHIP_RV610_94C6:
   case PCI_CHIP_RV610_94C7:
   case PCI_CHIP_RV610_94C8:
   case PCI_CHIP_RV610_94C9:
   case PCI_CHIP_RV610_94CB:
   case PCI_CHIP_RV610_94CC:
   case PCI_CHIP_RV610_94CD:
      screen->chip_family = CHIP_FAMILY_RV610;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV630_9580:
   case PCI_CHIP_RV630_9581:
   case PCI_CHIP_RV630_9583:
   case PCI_CHIP_RV630_9586:
   case PCI_CHIP_RV630_9587:
   case PCI_CHIP_RV630_9588:
   case PCI_CHIP_RV630_9589:
   case PCI_CHIP_RV630_958A:
   case PCI_CHIP_RV630_958B:
   case PCI_CHIP_RV630_958C:
   case PCI_CHIP_RV630_958D:
   case PCI_CHIP_RV630_958E:
   case PCI_CHIP_RV630_958F:
      screen->chip_family = CHIP_FAMILY_RV630;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV670_9500:
   case PCI_CHIP_RV670_9501:
   case PCI_CHIP_RV670_9504:
   case PCI_CHIP_RV670_9505:
   case PCI_CHIP_RV670_9506:
   case PCI_CHIP_RV670_9507:
   case PCI_CHIP_RV670_9508:
   case PCI_CHIP_RV670_9509:
   case PCI_CHIP_RV670_950F:
   case PCI_CHIP_RV670_9511:
   case PCI_CHIP_RV670_9515:
   case PCI_CHIP_RV670_9517:
   case PCI_CHIP_RV670_9519:
      screen->chip_family = CHIP_FAMILY_RV670;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV620_95C0:
   case PCI_CHIP_RV620_95C2:
   case PCI_CHIP_RV620_95C4:
   case PCI_CHIP_RV620_95C5:
   case PCI_CHIP_RV620_95C6:
   case PCI_CHIP_RV620_95C7:
   case PCI_CHIP_RV620_95C9:
   case PCI_CHIP_RV620_95CC:
   case PCI_CHIP_RV620_95CD:
   case PCI_CHIP_RV620_95CE:
   case PCI_CHIP_RV620_95CF:
      screen->chip_family = CHIP_FAMILY_RV620;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV635_9590:
   case PCI_CHIP_RV635_9591:
   case PCI_CHIP_RV635_9593:
   case PCI_CHIP_RV635_9595:
   case PCI_CHIP_RV635_9596:
   case PCI_CHIP_RV635_9597:
   case PCI_CHIP_RV635_9598:
   case PCI_CHIP_RV635_9599:
   case PCI_CHIP_RV635_959B:
      screen->chip_family = CHIP_FAMILY_RV635;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RS780_9610:
   case PCI_CHIP_RS780_9611:
   case PCI_CHIP_RS780_9612:
   case PCI_CHIP_RS780_9613:
   case PCI_CHIP_RS780_9614:
   case PCI_CHIP_RS780_9615:
   case PCI_CHIP_RS780_9616:
      screen->chip_family = CHIP_FAMILY_RS780;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;
   case PCI_CHIP_RS880_9710:
   case PCI_CHIP_RS880_9711:
   case PCI_CHIP_RS880_9712:
   case PCI_CHIP_RS880_9713:
   case PCI_CHIP_RS880_9714:
   case PCI_CHIP_RS880_9715:
      screen->chip_family = CHIP_FAMILY_RS880;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV770_9440:
   case PCI_CHIP_RV770_9441:
   case PCI_CHIP_RV770_9442:
   case PCI_CHIP_RV770_9443:
   case PCI_CHIP_RV770_9444:
   case PCI_CHIP_RV770_9446:
   case PCI_CHIP_RV770_944A:
   case PCI_CHIP_RV770_944B:
   case PCI_CHIP_RV770_944C:
   case PCI_CHIP_RV770_944E:
   case PCI_CHIP_RV770_9450:
   case PCI_CHIP_RV770_9452:
   case PCI_CHIP_RV770_9456:
   case PCI_CHIP_RV770_945A:
   case PCI_CHIP_RV770_945B:
   case PCI_CHIP_RV770_945E:
   case PCI_CHIP_RV790_9460:
   case PCI_CHIP_RV790_9462:
   case PCI_CHIP_RV770_946A:
   case PCI_CHIP_RV770_946B:
   case PCI_CHIP_RV770_947A:
   case PCI_CHIP_RV770_947B:
      screen->chip_family = CHIP_FAMILY_RV770;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV730_9480:
   case PCI_CHIP_RV730_9487:
   case PCI_CHIP_RV730_9488:
   case PCI_CHIP_RV730_9489:
   case PCI_CHIP_RV730_948A:
   case PCI_CHIP_RV730_948F:
   case PCI_CHIP_RV730_9490:
   case PCI_CHIP_RV730_9491:
   case PCI_CHIP_RV730_9495:
   case PCI_CHIP_RV730_9498:
   case PCI_CHIP_RV730_949C:
   case PCI_CHIP_RV730_949E:
   case PCI_CHIP_RV730_949F:
      screen->chip_family = CHIP_FAMILY_RV730;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV710_9540:
   case PCI_CHIP_RV710_9541:
   case PCI_CHIP_RV710_9542:
   case PCI_CHIP_RV710_954E:
   case PCI_CHIP_RV710_954F:
   case PCI_CHIP_RV710_9552:
   case PCI_CHIP_RV710_9553:
   case PCI_CHIP_RV710_9555:
   case PCI_CHIP_RV710_9557:
   case PCI_CHIP_RV710_955F:
      screen->chip_family = CHIP_FAMILY_RV710;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   case PCI_CHIP_RV740_94A0:
   case PCI_CHIP_RV740_94A1:
   case PCI_CHIP_RV740_94A3:
   case PCI_CHIP_RV740_94B1:
   case PCI_CHIP_RV740_94B3:
   case PCI_CHIP_RV740_94B4:
   case PCI_CHIP_RV740_94B5:
   case PCI_CHIP_RV740_94B9:
      screen->chip_family = CHIP_FAMILY_RV740;
      screen->chip_flags = RADEON_CHIPSET_TCL;
      break;

   default:
      fprintf(stderr, "unknown chip id 0x%x, can't guess.\n",
	      device_id);
      return -1;
   }

   return 0;
}


/* Create the device specific screen private data struct.
 */
static radeonScreenPtr
radeonCreateScreen( __DRIscreen *sPriv )
{
   radeonScreenPtr screen;
   RADEONDRIPtr dri_priv = (RADEONDRIPtr)sPriv->pDevPriv;
   unsigned char *RADEONMMIO = NULL;
   int i;
   int ret;
   uint32_t temp = 0;

   if (sPriv->devPrivSize != sizeof(RADEONDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(RADEONDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   screen = (radeonScreenPtr) CALLOC( sizeof(*screen) );
   if ( !screen ) {
      __driUtilMessage("%s: Could not allocate memory for screen structure",
		       __FUNCTION__);
      return NULL;
   }

   radeon_init_debug();

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   /* This is first since which regions we map depends on whether or
    * not we are using a PCI card.
    */
   screen->card_type = (dri_priv->IsPCI ? RADEON_CARD_PCI : RADEON_CARD_AGP);
   {
      int ret;

      ret = radeonGetParam(sPriv, RADEON_PARAM_GART_BUFFER_OFFSET,
			    &screen->gart_buffer_offset);

      if (ret) {
	 FREE( screen );
	 fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_GART_BUFFER_OFFSET): %d\n", ret);
	 return NULL;
      }

      ret = radeonGetParam(sPriv, RADEON_PARAM_GART_BASE,
			    &screen->gart_base);
      if (ret) {
	 FREE( screen );
	 fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_GART_BASE): %d\n", ret);
	 return NULL;
      }

      ret = radeonGetParam(sPriv, RADEON_PARAM_IRQ_NR,
			    &screen->irq);
      if (ret) {
	 FREE( screen );
	 fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_IRQ_NR): %d\n", ret);
	 return NULL;
      }
      screen->drmSupportsCubeMapsR200 = (sPriv->drm_version.minor >= 7);
      screen->drmSupportsBlendColor = (sPriv->drm_version.minor >= 11);
      screen->drmSupportsTriPerf = (sPriv->drm_version.minor >= 16);
      screen->drmSupportsFragShader = (sPriv->drm_version.minor >= 18);
      screen->drmSupportsPointSprites = (sPriv->drm_version.minor >= 13);
      screen->drmSupportsCubeMapsR100 = (sPriv->drm_version.minor >= 15);
      screen->drmSupportsVertexProgram = (sPriv->drm_version.minor >= 25);
      screen->drmSupportsOcclusionQueries = (sPriv->drm_version.minor >= 30);
   }

   ret = radeon_set_screen_flags(screen, dri_priv->deviceID);
   if (ret == -1)
     return NULL;

   screen->mmio.handle = dri_priv->registerHandle;
   screen->mmio.size   = dri_priv->registerSize;
   if ( drmMap( sPriv->fd,
		screen->mmio.handle,
		screen->mmio.size,
		&screen->mmio.map ) ) {
     FREE( screen );
     __driUtilMessage("%s: drmMap failed\n", __FUNCTION__ );
     return NULL;
   }

   RADEONMMIO = screen->mmio.map;

   screen->status.handle = dri_priv->statusHandle;
   screen->status.size   = dri_priv->statusSize;
   if ( drmMap( sPriv->fd,
		screen->status.handle,
		screen->status.size,
		&screen->status.map ) ) {
     drmUnmap( screen->mmio.map, screen->mmio.size );
     FREE( screen );
     __driUtilMessage("%s: drmMap (2) failed\n", __FUNCTION__ );
     return NULL;
   }
   if (screen->chip_family < CHIP_FAMILY_R600)
	   screen->scratch = (__volatile__ uint32_t *)
		   ((GLubyte *)screen->status.map + RADEON_SCRATCH_REG_OFFSET);
   else
	   screen->scratch = (__volatile__ uint32_t *)
		   ((GLubyte *)screen->status.map + R600_SCRATCH_REG_OFFSET);

   screen->buffers = drmMapBufs( sPriv->fd );
   if ( !screen->buffers ) {
     drmUnmap( screen->status.map, screen->status.size );
     drmUnmap( screen->mmio.map, screen->mmio.size );
     FREE( screen );
     __driUtilMessage("%s: drmMapBufs failed\n", __FUNCTION__ );
     return NULL;
   }

   if ( dri_priv->gartTexHandle && dri_priv->gartTexMapSize ) {
     screen->gartTextures.handle = dri_priv->gartTexHandle;
     screen->gartTextures.size   = dri_priv->gartTexMapSize;
     if ( drmMap( sPriv->fd,
		  screen->gartTextures.handle,
		  screen->gartTextures.size,
		  (drmAddressPtr)&screen->gartTextures.map ) ) {
       drmUnmapBufs( screen->buffers );
       drmUnmap( screen->status.map, screen->status.size );
       drmUnmap( screen->mmio.map, screen->mmio.size );
       FREE( screen );
       __driUtilMessage("%s: drmMap failed for GART texture area\n", __FUNCTION__);
       return NULL;
    }

     screen->gart_texture_offset = dri_priv->gartTexOffset + screen->gart_base;
   }

   if ((screen->chip_family == CHIP_FAMILY_R350 || screen->chip_family == CHIP_FAMILY_R300) &&
       sPriv->ddx_version.minor < 2) {
      fprintf(stderr, "xf86-video-ati-6.6.2 or newer needed for Radeon 9500/9700/9800 cards.\n");
      return NULL;
   }

   if ((sPriv->drm_version.minor < 29) && (screen->chip_family >= CHIP_FAMILY_RV515)) {
      fprintf(stderr, "R500 support requires a newer drm.\n");
      return NULL;
   }

   if (getenv("R300_NO_TCL"))
	   screen->chip_flags &= ~RADEON_CHIPSET_TCL;

   if (screen->chip_family <= CHIP_FAMILY_RS200)
	   screen->chip_flags |= RADEON_CLASS_R100;
   else if (screen->chip_family <= CHIP_FAMILY_RV280)
	   screen->chip_flags |= RADEON_CLASS_R200;
   else if (screen->chip_family <= CHIP_FAMILY_RV570)
	   screen->chip_flags |= RADEON_CLASS_R300;
   else
	   screen->chip_flags |= RADEON_CLASS_R600;

   screen->cpp = dri_priv->bpp / 8;
   screen->AGPMode = dri_priv->AGPMode;

   ret = radeonGetParam(sPriv, RADEON_PARAM_FB_LOCATION, &temp);

   /* +r6/r7 */
   if(screen->chip_family >= CHIP_FAMILY_R600)
   {
       if (ret)
       {
            FREE( screen );
            fprintf(stderr, "Unable to get fb location need newer drm\n");
            return NULL;
       }
       else
       {
            screen->fbLocation = (temp & 0xffff) << 24;
       }
   }
   else
   {
        if (ret)
        {
            if (screen->chip_family < CHIP_FAMILY_RS600 && !screen->kernel_mm)
	            screen->fbLocation      = ( INREG( RADEON_MC_FB_LOCATION ) & 0xffff) << 16;
            else
            {
                FREE( screen );
                fprintf(stderr, "Unable to get fb location need newer drm\n");
                return NULL;
            }
        }
        else
        {
            screen->fbLocation = (temp & 0xffff) << 16;
        }
   }

   if (IS_R300_CLASS(screen)) {
       ret = radeonGetParam(sPriv, RADEON_PARAM_NUM_GB_PIPES, &temp);
       if (ret) {
	   fprintf(stderr, "Unable to get num_pipes, need newer drm\n");
	   switch (screen->chip_family) {
	   case CHIP_FAMILY_R300:
	   case CHIP_FAMILY_R350:
	       screen->num_gb_pipes = 2;
	       break;
	   case CHIP_FAMILY_R420:
	   case CHIP_FAMILY_R520:
	   case CHIP_FAMILY_R580:
	   case CHIP_FAMILY_RV560:
	   case CHIP_FAMILY_RV570:
	       screen->num_gb_pipes = 4;
	       break;
	   case CHIP_FAMILY_RV350:
	   case CHIP_FAMILY_RV515:
	   case CHIP_FAMILY_RV530:
	   case CHIP_FAMILY_RV410:
	   default:
	       screen->num_gb_pipes = 1;
	       break;
	   }
       } else {
	   screen->num_gb_pipes = temp;
       }

       /* pipe overrides */
       switch (dri_priv->deviceID) {
       case PCI_CHIP_R300_AD: /* 9500 with 1 quadpipe verified by: Reid Linnemann <lreid@cs.okstate.edu> */
       case PCI_CHIP_R350_AH: /* 9800 SE only have 1 quadpipe */
       case PCI_CHIP_RV410_5E4C: /* RV410 SE only have 1 quadpipe */
       case PCI_CHIP_RV410_5E4F: /* RV410 SE only have 1 quadpipe */
	   screen->num_gb_pipes = 1;
	   break;
       default:
	   break;
       }

       if ( sPriv->drm_version.minor >= 31 ) {
	       ret = radeonGetParam(sPriv, RADEON_PARAM_NUM_Z_PIPES, &temp);
	       if (ret)
		       screen->num_z_pipes = 2;
	       else
		       screen->num_z_pipes = temp;
       } else
	       screen->num_z_pipes = 2;
   }

   if ( sPriv->drm_version.minor >= 10 ) {
      drm_radeon_setparam_t sp;

      sp.param = RADEON_SETPARAM_FB_LOCATION;
      sp.value = screen->fbLocation;

      drmCommandWrite( sPriv->fd, DRM_RADEON_SETPARAM,
		       &sp, sizeof( sp ) );
   }

   screen->frontOffset	= dri_priv->frontOffset;
   screen->frontPitch	= dri_priv->frontPitch;
   screen->backOffset	= dri_priv->backOffset;
   screen->backPitch	= dri_priv->backPitch;
   screen->depthOffset	= dri_priv->depthOffset;
   screen->depthPitch	= dri_priv->depthPitch;

   /* Check if ddx has set up a surface reg to cover depth buffer */
   screen->depthHasSurface = (sPriv->ddx_version.major > 4) ||
      /* these chips don't use tiled z without hyperz. So always pretend
         we have set up a surface which will cause linear reads/writes */
      (IS_R100_CLASS(screen) &&
      !(screen->chip_flags & RADEON_CHIPSET_TCL));

   if ( dri_priv->textureSize == 0 ) {
      screen->texOffset[RADEON_LOCAL_TEX_HEAP] = screen->gart_texture_offset;
      screen->texSize[RADEON_LOCAL_TEX_HEAP] = dri_priv->gartTexMapSize;
      screen->logTexGranularity[RADEON_LOCAL_TEX_HEAP] =
	 dri_priv->log2GARTTexGran;
   } else {
      screen->texOffset[RADEON_LOCAL_TEX_HEAP] = dri_priv->textureOffset
				               + screen->fbLocation;
      screen->texSize[RADEON_LOCAL_TEX_HEAP] = dri_priv->textureSize;
      screen->logTexGranularity[RADEON_LOCAL_TEX_HEAP] =
	 dri_priv->log2TexGran;
   }

   if ( !screen->gartTextures.map || dri_priv->textureSize == 0
	|| getenv( "RADEON_GARTTEXTURING_FORCE_DISABLE" ) ) {
      screen->numTexHeaps = RADEON_NR_TEX_HEAPS - 1;
      screen->texOffset[RADEON_GART_TEX_HEAP] = 0;
      screen->texSize[RADEON_GART_TEX_HEAP] = 0;
      screen->logTexGranularity[RADEON_GART_TEX_HEAP] = 0;
   } else {
      screen->numTexHeaps = RADEON_NR_TEX_HEAPS;
      screen->texOffset[RADEON_GART_TEX_HEAP] = screen->gart_texture_offset;
      screen->texSize[RADEON_GART_TEX_HEAP] = dri_priv->gartTexMapSize;
      screen->logTexGranularity[RADEON_GART_TEX_HEAP] =
	 dri_priv->log2GARTTexGran;
   }

   i = 0;
   screen->extensions[i++] = &driCopySubBufferExtension.base;
   screen->extensions[i++] = &driFrameTrackingExtension.base;
   screen->extensions[i++] = &driReadDrawableExtension;

   if ( screen->irq != 0 ) {
       screen->extensions[i++] = &driSwapControlExtension.base;
       screen->extensions[i++] = &driMediaStreamCounterExtension.base;
   }

#if defined(RADEON_R100)
   screen->extensions[i++] = &radeonTexOffsetExtension.base;
#endif

#if defined(RADEON_R200)
   if (IS_R200_CLASS(screen))
      screen->extensions[i++] = &r200AllocateExtension.base;

   screen->extensions[i++] = &r200texOffsetExtension.base;
#endif

#if defined(RADEON_R300)
   screen->extensions[i++] = &r300texOffsetExtension.base;
#endif

#if defined(RADEON_R600)
   screen->extensions[i++] = &r600texOffsetExtension.base;
#endif

   screen->extensions[i++] = NULL;
   sPriv->extensions = screen->extensions;

   screen->driScreen = sPriv;
   screen->sarea_priv_offset = dri_priv->sarea_priv_offset;
   screen->sarea = (drm_radeon_sarea_t *) ((GLubyte *) sPriv->pSAREA +
					       screen->sarea_priv_offset);

   screen->bom = radeon_bo_manager_legacy_ctor(screen);
   if (screen->bom == NULL) {
     free(screen);
     return NULL;
   }

   return screen;
}

static radeonScreenPtr
radeonCreateScreen2(__DRIscreen *sPriv)
{
   radeonScreenPtr screen;
   int i;
   int ret;
   uint32_t device_id = 0;
   uint32_t temp = 0;

   /* Allocate the private area */
   screen = (radeonScreenPtr) CALLOC( sizeof(*screen) );
   if ( !screen ) {
      __driUtilMessage("%s: Could not allocate memory for screen structure",
		       __FUNCTION__);
      fprintf(stderr, "leaving here\n");
      return NULL;
   }

   radeon_init_debug();

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&screen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   screen->kernel_mm = 1;
   screen->chip_flags = 0;

   /* if we have kms we can support all of these */
   screen->drmSupportsCubeMapsR200 = 1;
   screen->drmSupportsBlendColor = 1;
   screen->drmSupportsTriPerf = 1;
   screen->drmSupportsFragShader = 1;
   screen->drmSupportsPointSprites = 1;
   screen->drmSupportsCubeMapsR100 = 1;
   screen->drmSupportsVertexProgram = 1;
   screen->drmSupportsOcclusionQueries = 1;
   screen->irq = 1;

   ret = radeonGetParam(sPriv, RADEON_PARAM_DEVICE_ID, &device_id);
   if (ret) {
     FREE( screen );
     fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_DEVICE_ID): %d\n", ret);
     return NULL;
   }

   ret = radeon_set_screen_flags(screen, device_id);
   if (ret == -1)
     return NULL;

   if (getenv("R300_NO_TCL"))
	   screen->chip_flags &= ~RADEON_CHIPSET_TCL;

   if (screen->chip_family <= CHIP_FAMILY_RS200)
	   screen->chip_flags |= RADEON_CLASS_R100;
   else if (screen->chip_family <= CHIP_FAMILY_RV280)
	   screen->chip_flags |= RADEON_CLASS_R200;
   else if (screen->chip_family <= CHIP_FAMILY_RV570)
	   screen->chip_flags |= RADEON_CLASS_R300;
   else
	   screen->chip_flags |= RADEON_CLASS_R600;

   if (IS_R300_CLASS(screen)) {
       ret = radeonGetParam(sPriv, RADEON_PARAM_NUM_GB_PIPES, &temp);
       if (ret) {
	   fprintf(stderr, "Unable to get num_pipes, need newer drm\n");
	   switch (screen->chip_family) {
	   case CHIP_FAMILY_R300:
	   case CHIP_FAMILY_R350:
	       screen->num_gb_pipes = 2;
	       break;
	   case CHIP_FAMILY_R420:
	   case CHIP_FAMILY_R520:
	   case CHIP_FAMILY_R580:
	   case CHIP_FAMILY_RV560:
	   case CHIP_FAMILY_RV570:
	       screen->num_gb_pipes = 4;
	       break;
	   case CHIP_FAMILY_RV350:
	   case CHIP_FAMILY_RV515:
	   case CHIP_FAMILY_RV530:
	   case CHIP_FAMILY_RV410:
	   default:
	       screen->num_gb_pipes = 1;
	       break;
	   }
       } else {
	   screen->num_gb_pipes = temp;
       }

       /* pipe overrides */
       switch (device_id) {
       case PCI_CHIP_R300_AD: /* 9500 with 1 quadpipe verified by: Reid Linnemann <lreid@cs.okstate.edu> */
       case PCI_CHIP_R350_AH: /* 9800 SE only have 1 quadpipe */
       case PCI_CHIP_RV410_5E4C: /* RV410 SE only have 1 quadpipe */
       case PCI_CHIP_RV410_5E4F: /* RV410 SE only have 1 quadpipe */
	   screen->num_gb_pipes = 1;
	   break;
       default:
	   break;
       }

       ret = radeonGetParam(sPriv, RADEON_PARAM_NUM_Z_PIPES, &temp);
       if (ret)
	       screen->num_z_pipes = 2;
       else
	       screen->num_z_pipes = temp;

   }

   i = 0;
   screen->extensions[i++] = &driCopySubBufferExtension.base;
   screen->extensions[i++] = &driFrameTrackingExtension.base;
   screen->extensions[i++] = &driReadDrawableExtension;

   if ( screen->irq != 0 ) {
       screen->extensions[i++] = &driSwapControlExtension.base;
       screen->extensions[i++] = &driMediaStreamCounterExtension.base;
   }

#if defined(RADEON_R100)
   screen->extensions[i++] = &radeonTexBufferExtension.base;
#endif

#if defined(RADEON_R200)
   if (IS_R200_CLASS(screen))
       screen->extensions[i++] = &r200AllocateExtension.base;

   screen->extensions[i++] = &r200TexBufferExtension.base;
#endif

#if defined(RADEON_R300)
   screen->extensions[i++] = &r300TexBufferExtension.base;
#endif

#if defined(RADEON_R600)
   screen->extensions[i++] = &r600TexBufferExtension.base;
#endif

   screen->extensions[i++] = NULL;
   sPriv->extensions = screen->extensions;

   screen->driScreen = sPriv;
   screen->bom = radeon_bo_manager_gem_ctor(sPriv->fd);
   if (screen->bom == NULL) {
       free(screen);
       return NULL;
   }
   return screen;
}

/* Destroy the device specific screen private data struct.
 */
static void
radeonDestroyScreen( __DRIscreen *sPriv )
{
    radeonScreenPtr screen = (radeonScreenPtr)sPriv->private;

    if (!screen)
        return;

    if (screen->kernel_mm) {
#ifdef RADEON_BO_TRACK
        radeon_tracker_print(&screen->bom->tracker, stderr);
#endif
        radeon_bo_manager_gem_dtor(screen->bom);
    } else {
        radeon_bo_manager_legacy_dtor(screen->bom);

        if ( screen->gartTextures.map ) {
            drmUnmap( screen->gartTextures.map, screen->gartTextures.size );
        }
        drmUnmapBufs( screen->buffers );
        drmUnmap( screen->status.map, screen->status.size );
        drmUnmap( screen->mmio.map, screen->mmio.size );
    }

    /* free all option information */
    driDestroyOptionInfo (&screen->optionCache);

    FREE( screen );
    sPriv->private = NULL;
}


/* Initialize the driver specific screen private data.
 */
static GLboolean
radeonInitDriver( __DRIscreen *sPriv )
{
    if (sPriv->dri2.enabled) {
        sPriv->private = (void *) radeonCreateScreen2( sPriv );
    } else {
        sPriv->private = (void *) radeonCreateScreen( sPriv );
    }
    if ( !sPriv->private ) {
        radeonDestroyScreen( sPriv );
        return GL_FALSE;
    }

    return GL_TRUE;
}



/**
 * Create the Mesa framebuffer and renderbuffers for a given window/drawable.
 *
 * \todo This function (and its interface) will need to be updated to support
 * pbuffers.
 */
static GLboolean
radeonCreateBuffer( __DRIscreen *driScrnPriv,
                    __DRIdrawable *driDrawPriv,
                    const __GLcontextModes *mesaVis,
                    GLboolean isPixmap )
{
    radeonScreenPtr screen = (radeonScreenPtr) driScrnPriv->private;

    const GLboolean swDepth = GL_FALSE;
    const GLboolean swAlpha = GL_FALSE;
    const GLboolean swAccum = mesaVis->accumRedBits > 0;
    const GLboolean swStencil = mesaVis->stencilBits > 0 &&
	mesaVis->depthBits != 24;
    gl_format rgbFormat;
    struct radeon_framebuffer *rfb;

    if (isPixmap)
      return GL_FALSE; /* not implemented */

    rfb = CALLOC_STRUCT(radeon_framebuffer);
    if (!rfb)
      return GL_FALSE;

    _mesa_initialize_window_framebuffer(&rfb->base, mesaVis);

    if (mesaVis->redBits == 5)
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_RGB565 : MESA_FORMAT_RGB565_REV;
    else if (mesaVis->alphaBits == 0)
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_XRGB8888 : MESA_FORMAT_XRGB8888_REV;
    else
        rgbFormat = _mesa_little_endian() ? MESA_FORMAT_ARGB8888 : MESA_FORMAT_ARGB8888_REV;

    /* front color renderbuffer */
    rfb->color_rb[0] = radeon_create_renderbuffer(rgbFormat, driDrawPriv);
    _mesa_add_renderbuffer(&rfb->base, BUFFER_FRONT_LEFT, &rfb->color_rb[0]->base);
    rfb->color_rb[0]->has_surface = 1;

    /* back color renderbuffer */
    if (mesaVis->doubleBufferMode) {
      rfb->color_rb[1] = radeon_create_renderbuffer(rgbFormat, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_BACK_LEFT, &rfb->color_rb[1]->base);
	rfb->color_rb[1]->has_surface = 1;
    }

    if (mesaVis->depthBits == 24) {
      if (mesaVis->stencilBits == 8) {
	struct radeon_renderbuffer *depthStencilRb =
           radeon_create_renderbuffer(MESA_FORMAT_S8_Z24, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depthStencilRb->base);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_STENCIL, &depthStencilRb->base);
	depthStencilRb->has_surface = screen->depthHasSurface;
      } else {
	/* depth renderbuffer */
	struct radeon_renderbuffer *depth =
           radeon_create_renderbuffer(MESA_FORMAT_X8_Z24, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depth->base);
	depth->has_surface = screen->depthHasSurface;
      }
    } else if (mesaVis->depthBits == 16) {
        /* just 16-bit depth buffer, no hw stencil */
	struct radeon_renderbuffer *depth =
           radeon_create_renderbuffer(MESA_FORMAT_Z16, driDrawPriv);
	_mesa_add_renderbuffer(&rfb->base, BUFFER_DEPTH, &depth->base);
	depth->has_surface = screen->depthHasSurface;
    }

    _mesa_add_soft_renderbuffers(&rfb->base,
	    GL_FALSE, /* color */
	    swDepth,
	    swStencil,
	    swAccum,
	    swAlpha,
	    GL_FALSE /* aux */);
    driDrawPriv->driverPrivate = (void *) rfb;

    return (driDrawPriv->driverPrivate != NULL);
}


static void radeon_cleanup_renderbuffers(struct radeon_framebuffer *rfb)
{
	struct radeon_renderbuffer *rb;

	rb = rfb->color_rb[0];
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = rfb->color_rb[1];
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
	rb = radeon_get_renderbuffer(&rfb->base, BUFFER_DEPTH);
	if (rb && rb->bo) {
		radeon_bo_unref(rb->bo);
		rb->bo = NULL;
	}
}

void
radeonDestroyBuffer(__DRIdrawable *driDrawPriv)
{
    struct radeon_framebuffer *rfb;
    if (!driDrawPriv)
	return;

    rfb = (void*)driDrawPriv->driverPrivate;
    if (!rfb)
	return;
    radeon_cleanup_renderbuffers(rfb);
    _mesa_reference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)), NULL);
}


/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * \todo maybe fold this into intelInitDriver
 *
 * \return the __GLcontextModes supported by this driver
 */
static const __DRIconfig **
radeonInitScreen(__DRIscreen *psp)
{
#if defined(RADEON_R100)
   static const char *driver_name = "Radeon";
   static const __DRIutilversion2 ddx_expected = { 4, 5, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 6, 0 };
#elif defined(RADEON_R200)
   static const char *driver_name = "R200";
   static const __DRIutilversion2 ddx_expected = { 4, 5, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 6, 0 };
#elif defined(RADEON_R300)
   static const char *driver_name = "R300";
   static const __DRIutilversion2 ddx_expected = { 4, 5, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 24, 0 };
#elif defined(RADEON_R600)
   static const char *driver_name = "R600";
   static const __DRIutilversion2 ddx_expected = { 4, 5, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 24, 0 };
#endif
   RADEONDRIPtr dri_priv = (RADEONDRIPtr) psp->pDevPriv;

   if ( ! driCheckDriDdxDrmVersions3( driver_name,
				      &psp->dri_version, & dri_expected,
				      &psp->ddx_version, & ddx_expected,
				      &psp->drm_version, & drm_expected ) ) {
      return NULL;
   }

   if (!radeonInitDriver(psp))
       return NULL;

   /* for now fill in all modes */
   return radeonFillInModes( psp,
			     dri_priv->bpp,
			     (dri_priv->bpp == 16) ? 16 : 24,
			     (dri_priv->bpp == 16) ? 0  : 8, 1);
}
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the __GLcontextModes supported by this driver
 */
static const
__DRIconfig **radeonInitScreen2(__DRIscreen *psp)
{
   GLenum fb_format[3];
   GLenum fb_type[3];
   /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
    * support pageflipping at all.
    */
   static const GLenum back_buffer_modes[] = {
     GLX_NONE, GLX_SWAP_UNDEFINED_OML, /*, GLX_SWAP_COPY_OML*/
   };
   uint8_t depth_bits[4], stencil_bits[4], msaa_samples_array[1];
   int color;
   __DRIconfig **configs = NULL;

   if (!radeonInitDriver(psp)) {
       return NULL;
    }
   depth_bits[0] = 0;
   stencil_bits[0] = 0;
   depth_bits[1] = 16;
   stencil_bits[1] = 0;
   depth_bits[2] = 24;
   stencil_bits[2] = 0;
   depth_bits[3] = 24;
   stencil_bits[3] = 8;

   msaa_samples_array[0] = 0;

   fb_format[0] = GL_RGB;
   fb_type[0] = GL_UNSIGNED_SHORT_5_6_5;

   fb_format[1] = GL_BGR;
   fb_type[1] = GL_UNSIGNED_INT_8_8_8_8_REV;

   fb_format[2] = GL_BGRA;
   fb_type[2] = GL_UNSIGNED_INT_8_8_8_8_REV;

   for (color = 0; color < ARRAY_SIZE(fb_format); color++) {
      __DRIconfig **new_configs;

      new_configs = driCreateConfigs(fb_format[color], fb_type[color],
				     depth_bits,
				     stencil_bits,
				     ARRAY_SIZE(depth_bits),
				     back_buffer_modes,
				     ARRAY_SIZE(back_buffer_modes),
				     msaa_samples_array,
				     ARRAY_SIZE(msaa_samples_array),
				     GL_TRUE);
      if (configs == NULL)
	 configs = new_configs;
      else
	 configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

/**
 * Get information about previous buffer swaps.
 */
static int
getSwapInfo( __DRIdrawable *dPriv, __DRIswapInfo * sInfo )
{
    struct radeon_framebuffer *rfb;

    if ( (dPriv == NULL) || (dPriv->driContextPriv == NULL)
	 || (dPriv->driContextPriv->driverPrivate == NULL)
	 || (sInfo == NULL) ) {
	return -1;
   }

    rfb = dPriv->driverPrivate;
    sInfo->swap_count = rfb->swap_count;
    sInfo->swap_ust = rfb->swap_ust;
    sInfo->swap_missed_count = rfb->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
       ? driCalculateSwapUsage( dPriv, 0, rfb->swap_missed_ust )
       : 0.0;

   return 0;
}

const struct __DriverAPIRec driDriverAPI = {
   .InitScreen      = radeonInitScreen,
   .DestroyScreen   = radeonDestroyScreen,
#if defined(RADEON_R200)
   .CreateContext   = r200CreateContext,
   .DestroyContext  = r200DestroyContext,
#elif defined(RADEON_R600)
   .CreateContext   = r600CreateContext,
   .DestroyContext  = radeonDestroyContext,
#elif defined(RADEON_R300)
   .CreateContext   = r300CreateContext,
   .DestroyContext  = radeonDestroyContext,
#else
   .CreateContext   = r100CreateContext,
   .DestroyContext  = radeonDestroyContext,
#endif
   .CreateBuffer    = radeonCreateBuffer,
   .DestroyBuffer   = radeonDestroyBuffer,
   .SwapBuffers     = radeonSwapBuffers,
   .MakeCurrent     = radeonMakeCurrent,
   .UnbindContext   = radeonUnbindContext,
   .GetSwapInfo     = getSwapInfo,
   .GetDrawableMSC  = driDrawableGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL,
   .CopySubBuffer   = radeonCopySubBuffer,
    /* DRI2 */
   .InitScreen2     = radeonInitScreen2,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driLegacyExtension.base,
    &driDRI2Extension.base,
    NULL
};
