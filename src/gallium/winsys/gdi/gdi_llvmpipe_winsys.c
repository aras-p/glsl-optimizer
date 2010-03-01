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

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "llvmpipe/lp_winsys.h"
#include "llvmpipe/lp_texture.h"
#include "stw_winsys.h"


struct gdi_llvmpipe_displaytarget
{
   enum pipe_format format;
   unsigned width;
   unsigned height;
   unsigned stride;

   unsigned size;

   void *data;

   BITMAPINFO bmi;
};


/** Cast wrapper */
static INLINE struct gdi_llvmpipe_displaytarget *
gdi_llvmpipe_displaytarget( struct llvmpipe_displaytarget *buf )
{
   return (struct gdi_llvmpipe_displaytarget *)buf;
}


static boolean
gdi_llvmpipe_is_displaytarget_format_supported( struct llvmpipe_winsys *ws,
                                                enum pipe_format format )
{
   switch(format) {
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return TRUE;

   /* TODO: Support other formats possible with BMPs, as described in 
    * http://msdn.microsoft.com/en-us/library/dd183376(VS.85).aspx */
      
   default:
      return FALSE;
   }
}


static void *
gdi_llvmpipe_displaytarget_map(struct llvmpipe_winsys *ws,
                               struct llvmpipe_displaytarget *dt,
                               unsigned flags )
{
   struct gdi_llvmpipe_displaytarget *gdt = gdi_llvmpipe_displaytarget(dt);

   return gdt->data;
}


static void
gdi_llvmpipe_displaytarget_unmap(struct llvmpipe_winsys *ws,
                                 struct llvmpipe_displaytarget *dt )
{

}


static void
gdi_llvmpipe_displaytarget_destroy(struct llvmpipe_winsys *winsys,
                                   struct llvmpipe_displaytarget *dt)
{
   struct gdi_llvmpipe_displaytarget *gdt = gdi_llvmpipe_displaytarget(dt);

   align_free(gdt->data);
   FREE(gdt);
}


static struct llvmpipe_displaytarget *
gdi_llvmpipe_displaytarget_create(struct llvmpipe_winsys *winsys,
                                  enum pipe_format format,
                                  unsigned width, unsigned height,
                                  unsigned alignment,
                                  unsigned *stride)
{
   struct gdi_llvmpipe_displaytarget *gdt;
   unsigned cpp;
   unsigned bpp;
   
   gdt = CALLOC_STRUCT(gdi_llvmpipe_displaytarget);
   if(!gdt)
      goto no_gdt;

   gdt->format = format;
   gdt->width = width;
   gdt->height = height;

   bpp = util_format_get_blocksizebits(format);
   cpp = util_format_get_blocksize(format);
   
   gdt->stride = align(width * cpp, alignment);
   gdt->size = gdt->stride * height;
   
   gdt->data = align_malloc(gdt->size, alignment);
   if(!gdt->data)
      goto no_data;

   gdt->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   gdt->bmi.bmiHeader.biWidth = gdt->stride / cpp;
   gdt->bmi.bmiHeader.biHeight= -(long)height;
   gdt->bmi.bmiHeader.biPlanes = 1;
   gdt->bmi.bmiHeader.biBitCount = bpp;
   gdt->bmi.bmiHeader.biCompression = BI_RGB;
   gdt->bmi.bmiHeader.biSizeImage = 0;
   gdt->bmi.bmiHeader.biXPelsPerMeter = 0;
   gdt->bmi.bmiHeader.biYPelsPerMeter = 0;
   gdt->bmi.bmiHeader.biClrUsed = 0;
   gdt->bmi.bmiHeader.biClrImportant = 0;

   *stride = gdt->stride;
   return (struct llvmpipe_displaytarget *)gdt;

no_data:
   FREE(gdt);
no_gdt:
   return NULL;
}


static void
gdi_llvmpipe_displaytarget_display(struct llvmpipe_winsys *winsys, 
                                   struct llvmpipe_displaytarget *dt,
                                   void *context_private)
{
   assert(0);
}


static void
gdi_llvmpipe_destroy(struct llvmpipe_winsys *winsys)
{
   FREE(winsys);
}


static struct pipe_screen *
gdi_llvmpipe_screen_create(void)
{
   static struct llvmpipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = CALLOC_STRUCT(llvmpipe_winsys);
   if(!winsys)
      goto no_winsys;

   winsys->destroy = gdi_llvmpipe_destroy;
   winsys->is_displaytarget_format_supported = gdi_llvmpipe_is_displaytarget_format_supported;
   winsys->displaytarget_create = gdi_llvmpipe_displaytarget_create;
   winsys->displaytarget_map = gdi_llvmpipe_displaytarget_map;
   winsys->displaytarget_unmap = gdi_llvmpipe_displaytarget_unmap;
   winsys->displaytarget_display = gdi_llvmpipe_displaytarget_display;
   winsys->displaytarget_destroy = gdi_llvmpipe_displaytarget_destroy;

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
