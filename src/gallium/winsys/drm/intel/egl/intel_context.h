/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef INTEL_CONTEXT_H
#define INTEL_CONTEXT_H

#include "pipe/p_debug.h"
#include "intel_be_context.h"


struct st_context;
struct egl_drm_device;
struct egl_drm_context;
struct egl_drm_frontbuffer;


/**
 * Intel rendering context, contains a state tracker and intel-specific info.
 */
struct intel_context
{
	struct intel_be_context base;

	struct st_context *st;

	struct intel_device *intel_device;

	/* new egl stuff */
	struct egl_drm_device *egl_device;
	struct egl_drm_context *egl_context;
	struct egl_drm_drawable *egl_drawable;
};



/**
 * Intel framebuffer.
 */
struct intel_framebuffer
{
	struct st_framebuffer *stfb;

	struct intel_device *device;
	struct _DriBufferObject *front_buffer;
	struct egl_drm_frontbuffer *front;
};




/* These are functions now:
 */
void LOCK_HARDWARE( struct intel_context *intel );
void UNLOCK_HARDWARE( struct intel_context *intel );

extern char *__progname;



/* ================================================================
 * Debugging:
 */
#ifdef DEBUG
extern int __intel_debug;

#define DEBUG_SWAP	0x1
#define DEBUG_LOCK      0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_BATCH     0x8

#define DBG(flag, ...)  do { 			\
   if (__intel_debug & (DEBUG_##flag)) 		\
      printf(__VA_ARGS__); 		\
} while(0)

#else
#define DBG(flag, ...)
#endif


#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_I855_GM		0x3582
#define PCI_CHIP_I865_G			0x2572
#define PCI_CHIP_I915_G			0x2582
#define PCI_CHIP_I915_GM		0x2592
#define PCI_CHIP_I945_G			0x2772
#define PCI_CHIP_I945_GM		0x27A2
#define PCI_CHIP_I945_GME		0x27AE
#define PCI_CHIP_G33_G			0x29C2
#define PCI_CHIP_Q35_G			0x29B2
#define PCI_CHIP_Q33_G			0x29D2

#endif
