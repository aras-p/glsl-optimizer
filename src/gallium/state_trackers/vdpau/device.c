/**************************************************************************
 *
 * Copyright 2010 Younes Manton.
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

#include <vdpau/vdpau_x11.h>
#include <pipe/p_compiler.h>
#include <vl_winsys.h>
#include <util/u_memory.h>
#include "vdpau_private.h"

VdpDeviceCreateX11 vdp_imp_device_create_x11;

PUBLIC VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device, VdpGetProcAddress **get_proc_address)
{
   VdpStatus    ret;
   vlVdpDevice *dev;

   if (!(display && device && get_proc_address))
      return VDP_STATUS_INVALID_POINTER;

   if (!vlCreateHTAB()) {
      ret = VDP_STATUS_RESOURCES;
      goto no_htab;
   }

   dev = CALLOC(1, sizeof(vlVdpDevice));
   if (!dev) {
      ret = VDP_STATUS_RESOURCES;
      goto no_dev;
   }

   *device = vlAddDataHTAB(dev);
   if (*device == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }

   *get_proc_address = &vlVdpGetProcAddress;

   return VDP_STATUS_OK;

no_handle:
   FREE(dev);
no_dev:
   vlDestroyHTAB();
no_htab:
   return ret;
}

VdpStatus vlVdpDeviceDestroy(VdpDevice device)
{
   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;
   FREE(dev);
   vlDestroyHTAB();

   return VDP_STATUS_OK;
}

VdpStatus vlVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   if (!function_pointer)
      return VDP_STATUS_INVALID_POINTER;

   if (!vlGetFuncFTAB(function_id, function_pointer))
      return VDP_STATUS_INVALID_FUNC_ID;

   return VDP_STATUS_OK;
}
