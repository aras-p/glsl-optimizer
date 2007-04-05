/**
 * \file driver.h
 * \brief DRI utility functions definitions.
 *
 * This module acts as glue between GLX and the actual hardware driver.  A DRI
 * driver doesn't really \e have to use any of this - it's optional.  But, some
 * useful stuff is done here that otherwise would have to be duplicated in most
 * drivers.
 * 
 * Basically, these utility functions take care of some of the dirty details of
 * screen initialization, context creation, context binding, DRM setup, etc.
 *
 * These functions are compiled into each DRI driver so libGL.so knows nothing
 * about them.
 *
 * Look for more comments in the dri_util.c file.
 * 
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Brian Paul <brian@precisioninsight.com>
 */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _driver_H_
#define _driver_H_

#include "GL/gl.h"
#include "GL/internal/glcore.h"

#include "drm.h"
#include "drm_sarea.h"

/**
 * \brief DRIDriverContext type.
 */
typedef struct DRIDriverContextRec {
   const char *pciBusID;
   int pciBus;
   int pciDevice;
   int pciFunc;
   int chipset;
   int bpp; 
   int cpp;  
   int agpmode;
   int isPCI;
   
   int colorTiling;	    /**< \brief color tiling is enabled */

   unsigned long FBStart;   /**< \brief physical address of the framebuffer */
   unsigned long MMIOStart; /**< \brief physical address of the MMIO region */
   
   int FBSize;              /**< \brief size of the mmap'd framebuffer in bytes */
   int MMIOSize;            /**< \brief size of the mmap'd MMIO region in bytes */

   void *FBAddress;         /**< \brief start of the mmap'd framebuffer */
   void *MMIOAddress;       /**< \brief start of the mmap'd MMIO region */
   
   /**
    * \brief Client configuration details
    *
    * These are computed on the server and sent to clients as part of
    * the initial handshaking.
    */
   struct {
      drm_handle_t hSAREA;
      int SAREASize;
      drm_handle_t hFrameBuffer;
      int fbOrigin;
      int fbSize;
      int fbStride;
      int virtualWidth;
      int virtualHeight;
      int Width;
   } shared;
   
   /**
    * \name From DRIInfoRec
    */
   /*@{*/
   int drmFD;  /**< \brief DRM device file descriptor */
   drm_sarea_t *pSAREA;
   unsigned int serverContext;	/**< \brief DRM context only active on server */
   /*@}*/
   
   
   /**
    * \name Driver private
    *
    * Populated by __driInitFBDev()
    */
   /*@{*/
   void *driverPrivate;
   void *driverClientMsg;
   int driverClientMsgSize;
   /*@}*/
} DRIDriverContext;

/**
 * \brief Interface to the DRI driver.
 *
 * This structure is retrieved from the loadable driver by the \e
 * __driDriver symbol to access the Mini GLX specific hardware
 * initialization and take down routines.
 */
typedef struct DRIDriverRec { 
   /**
    * \brief Validate the framebuffer device mode
    */
   int (*validateMode)( const DRIDriverContext *context );

   /**
    * \brief Examine mode returned by fbdev (may differ from the one
    * requested), restore any hw regs clobbered by fbdev.
    */
   int (*postValidateMode)( const DRIDriverContext *context );

   /**
    * \brief Initialize the framebuffer device.
    */
   int (*initFBDev)( DRIDriverContext *context );

   /**
    * \brief Halt the framebuffer device.
    */
   void (*haltFBDev)( DRIDriverContext *context );


   /**
    * \brief Idle and shutdown hardware in preparation for a VT switch.
    */
   int (*shutdownHardware)(  const DRIDriverContext *context );

   /**
    * \brief Restore hardware state after regaining the VT.
    */
   int (*restoreHardware)(  const DRIDriverContext *context );

   /**
    * \brief Notify hardware driver of gain/loose focus.  May be zero
    * as this is of limited utility for most drivers.  
    */
   void (*notifyFocus)( int have_focus );
} DRIDriver;

#endif /* _driver_H_ */
