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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

/**
 * @file
 * LLVMpipe support.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include <windows.h>

#include "gdi_sw_winsys.h"
#include "llvmpipe/lp_texture.h"


static struct pipe_screen *
gdi_llvmpipe_screen_create(void)
{
   static struct llvmpipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = gdi_create_sw_winsys();
   if(!winsys)
      goto no_winsys;

   screen = llvmpipe_create_screen(winsys);
   if(!screen)
      goto no_screen;

   return screen;
   
no_screen:
   FREE(winsys);
no_winsys:
   return NULL;
}




static void
gdi_llvmpipe_present(struct pipe_screen *screen,
                     struct pipe_surface *surface,
                     HDC hDC)
{
    struct llvmpipe_texture *texture;
    struct gdi_llvmpipe_displaytarget *gdt;

    texture = llvmpipe_texture(surface->texture);
    gdt = gdi_llvmpipe_displaytarget(texture->dt);

    StretchDIBits(hDC,
                  0, 0, gdt->width, gdt->height,
                  0, 0, gdt->width, gdt->height,
                  gdt->data, &gdt->bmi, 0, SRCCOPY);
}


static const struct stw_winsys stw_winsys = {
   &gdi_llvmpipe_screen_create,
   &gdi_llvmpipe_present,
   NULL, /* get_adapter_luid */
   NULL, /* shared_surface_open */
   NULL, /* shared_surface_close */
   NULL  /* compose */
};


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   switch (fdwReason) {
   case DLL_PROCESS_ATTACH:
      stw_init(&stw_winsys);
      stw_init_thread();
      break;

   case DLL_THREAD_ATTACH:
      stw_init_thread();
      break;

   case DLL_THREAD_DETACH:
      stw_cleanup_thread();
      break;

   case DLL_PROCESS_DETACH:
      stw_cleanup_thread();
      stw_cleanup();
      break;
   }
   return TRUE;
}
