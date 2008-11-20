/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <windows.h>

#include "pipe/p_debug.h"

#include "stw_device.h"
#include "stw_winsys.h"
#include "stw_pixelformat.h"


struct stw_device *stw_dev = NULL;


static BOOL
st_init(void)
{
   static struct stw_device stw_dev_storage;

   assert(!stw_dev);

   stw_dev = &stw_dev_storage;
   memset(stw_dev, 0, sizeof(*stw_dev));

   stw_dev->screen = stw_winsys.create_screen();
   if(!stw_dev->screen)
      goto error1;

   pixelformat_init();

   return TRUE;

error1:
   stw_dev = NULL;
   return FALSE;
}


static void
st_cleanup(void)
{
   DHGLRC dhglrc;

   if(!stw_dev)
      return;

   /* Ensure all contexts are destroyed */
   for (dhglrc = 1; dhglrc <= DRV_CONTEXT_MAX; dhglrc++)
      if (stw_dev->ctx_array[dhglrc - 1].hglrc)
         DrvDeleteContext( dhglrc );

   stw_dev = NULL;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   switch (fdwReason) {
   case DLL_PROCESS_ATTACH:
      return st_init();

   case DLL_PROCESS_DETACH:
      st_cleanup();
      break;
   }
   return TRUE;
}
