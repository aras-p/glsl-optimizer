
#ifndef _TDFX_DRI_
#define _TDFX_DRI_

#include "xf86drm.h"
#include "drm.h"

typedef struct {
  drm_handle_t regs;
  drmSize regsSize;
  int deviceID;
  int width;
  int height;
  int mem;
  int cpp;
  int stride;
  int fifoOffset;
  int fifoSize;
  int fbOffset;
  int backOffset;
  int depthOffset;
  int textureOffset;
  int textureSize;
  unsigned int sarea_priv_offset;
} TDFXDRIRec, *TDFXDRIPtr;

#endif
