/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 **************************************************************************/

#ifndef _INTEL_INIT_H_
#define _INTEL_INIT_H_

#include <sys/time.h>
#include "dri_util.h"


typedef struct {
   drm_handle_t handle;
   drmSize size;
   char *map;
} intelRegion;

typedef struct 
{
   intelRegion front;
   intelRegion back;
   intelRegion depth;
   intelRegion tex;
   
   int deviceID;
   int width;
   int height;
   int mem;
   
   int cpp;         /* for front and back buffers */
   int bitsPerPixel;
   
   int fbFormat;

   int frontOffset;
   int frontPitch;

   int backOffset;
   int backPitch;

   int depthOffset;
   int depthPitch;
   
   
   int textureOffset;
   int textureSize;
   int logTextureGranularity;
   
   __DRIscreenPrivate *driScrnPriv;
   unsigned int sarea_priv_offset;

   int drmMinor;

   int irq_active;
   int allow_batchbuffer;
} intelScreenPrivate;


extern void
intelDestroyContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
intelUnbindContext(__DRIcontextPrivate *driContextPriv);

extern GLboolean
intelMakeCurrent(__DRIcontextPrivate *driContextPriv,
                __DRIdrawablePrivate *driDrawPriv,
                __DRIdrawablePrivate *driReadPriv);

extern void
intelSwapBuffers( __DRIdrawablePrivate *dPriv);

#endif
