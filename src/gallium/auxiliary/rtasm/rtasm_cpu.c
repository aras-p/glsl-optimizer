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


#include "util/u_debug.h"
#include "rtasm_cpu.h"


#if defined(PIPE_ARCH_X86)
static boolean rtasm_sse_enabled(void)
{
   static boolean firsttime = 1;
   static boolean enabled;
   
   /* This gets called quite often at the moment:
    */
   if (firsttime) {
      enabled =  !debug_get_bool_option("GALLIUM_NOSSE", FALSE);
      firsttime = FALSE;
   }
   return enabled;
}
#endif

int rtasm_cpu_has_sse(void)
{
   /* FIXME: actually detect this at run-time */
#if defined(PIPE_ARCH_X86)
   return rtasm_sse_enabled();
#else
   return 0;
#endif
}

int rtasm_cpu_has_sse2(void) 
{
   /* FIXME: actually detect this at run-time */
#if defined(PIPE_ARCH_X86)
   return rtasm_sse_enabled();
#else
   return 0;
#endif
}
