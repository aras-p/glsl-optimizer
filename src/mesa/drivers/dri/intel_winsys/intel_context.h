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

#ifndef INTELCONTEXT_INC
#define INTELCONTEXT_INC



#include "mtypes.h"
#include "drm.h"
#include "mm.h"
#include "texmem.h"

#include "intel_screen.h"
#include "i915_drm.h"
#include "i830_common.h"


struct pipe_context;
struct pipe_region;
struct intel_context;
struct _DriBufferObject;



#define INTEL_MAX_FIXUP 64

struct intel_context
{
   GLcontext ctx;               /* the parent class */
   
   struct pipe_context *pipe;

   GLint refcount;

   struct _DriFenceObject *last_swap_fence;
   struct _DriFenceObject *first_swap_fence;

   struct intel_batchbuffer *batch;


   GLboolean locked;
   char *prevLockFile;
   int prevLockLine;


   /* These refer to the current drawing buffer:
    */
   GLuint numClipRects;         /**< cliprects for drawing */
   drm_clip_rect_t *pClipRects;
   drm_clip_rect_t fboRect;     /**< cliprect for rendering */

   GLuint irqsEmitted;
   drm_i915_irq_wait_t iw;

   drm_context_t hHWContext;
   drmLock *driHwLock;
   int driFd;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   intelScreenPrivate *intelScreen;
   drmI830Sarea *sarea;

   GLuint lastStamp;

   /**
    * Configuration cache
    */
   driOptionCache optionCache;
};

/* These are functions now:
 */
void LOCK_HARDWARE( struct intel_context *intel );
void UNLOCK_HARDWARE( struct intel_context *intel );

extern char *__progname;

/* Will become a call into state_tracker:
 */
#define INTEL_FIREVERTICES(intel)	       


/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		0
#if DO_DEBUG
extern int INTEL_DEBUG;
#else
#define INTEL_DEBUG		0
#endif

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_BLIT	0x8
#define DEBUG_MIPTREE   0x10
#define DEBUG_VERBOSE	0x40
#define DEBUG_BATCH     0x80
#define DEBUG_PIXEL     0x100
#define DEBUG_BUFMGR    0x200
#define DEBUG_REGION    0x400
#define DEBUG_FBO       0x800
#define DEBUG_LOCK      0x1000

#define DBG(...)  do { if (INTEL_DEBUG & FILE_DEBUG_FLAG) _mesa_printf(__VA_ARGS__); } while(0)


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


/* ================================================================
 * intel_context.c:
 */

extern void intelGetLock(struct intel_context *intel, GLuint flags);

extern void intelFinish(GLcontext * ctx);
extern void intelFlush(GLcontext * ctx);

/*======================================================================
 * Inline conversion functions.  
 * These are better-typed than the macros used previously:
 */
static INLINE struct intel_context *
intel_context(GLcontext * ctx)
{
   return (struct intel_context *) ctx;
}

extern struct intel_renderbuffer *intel_renderbuffer(struct gl_renderbuffer
                                                     *rb);

extern void intel_init_region_functions(struct pipe_context *pipe);


#endif
