 /**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <drm/drm.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"

#include "st_winsys.h"

#include "radeon_winsys.h"

#include "xf86dri.h"


/* XXX: Force init_gallium symbol to be linked */
extern void init_gallium(void);
void (*force_init_gallium_linkage)(void) = &init_gallium;


static struct pipe_screen *
radeon_hardpipe_screen_create(void)
{
   Display *dpy;
   Window rootWin;
   XWindowAttributes winAttr;
   int isCapable;
   int screen;
   char *driverName;
   char *curBusID;
   unsigned magic;
   int ddxDriverMajor;
   int ddxDriverMinor;
   int ddxDriverPatch;
   drm_handle_t sAreaOffset;
   int ret;
   int drmFD;
   drm_context_t hHWContext;
   XID id;

   dpy = XOpenDisplay(":0");
   if (!dpy) {
      fprintf(stderr, "Open Display Failed\n");
      return NULL;
   }

   screen = DefaultScreen(dpy);
   rootWin = RootWindow(dpy, screen);
   XGetWindowAttributes(dpy, rootWin, &winAttr);

   ret = uniDRIQueryDirectRenderingCapable(dpy, screen, &isCapable);
   if (!ret || !isCapable) {
      fprintf(stderr, "No DRI on this display:sceen\n");
      goto error;
   }

   if (!uniDRIOpenConnection(dpy, screen, &sAreaOffset,
                             &curBusID)) {
      fprintf(stderr, "Could not open DRI connection.\n");
      goto error;
   }

   if (!uniDRIGetClientDriverName(dpy, screen, &ddxDriverMajor,
                                  &ddxDriverMinor, &ddxDriverPatch,
                                  &driverName)) {
      fprintf(stderr, "Could not get DRI driver name.\n");
      goto error;
   }

   if ((drmFD = drmOpen(NULL, curBusID)) < 0) {
      perror("DRM Device could not be opened");
      goto error;
   }

   drmGetMagic(drmFD, &magic);
   if (!uniDRIAuthConnection(dpy, screen, magic)) {
      fprintf(stderr, "Could not get X server to authenticate us.\n");
      goto error;
   }

   if (!uniDRICreateContext(dpy, screen, winAttr.visual,
                            &id, &hHWContext)) {
      fprintf(stderr, "Could not create DRI context.\n");
      goto error;
   }

   /* FIXME: create a radeon pipe_screen from drmFD and hHWContext */

   return NULL;
   
error:
   return NULL;
}




const struct st_winsys st_hardpipe_winsys = {
   &radeon_hardpipe_screen_create,
};

