/**********************************************************
 * Copyright 2009-2012 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "svga_cmd.h"
#include "util/u_debug.h"
#include "util/u_memory.h"

#include "vmw_shader.h"
#include "vmw_screen.h"

void
vmw_svga_winsys_shader_reference(struct vmw_svga_winsys_shader **pdst,
                                 struct vmw_svga_winsys_shader *src)
{
   struct pipe_reference *src_ref;
   struct pipe_reference *dst_ref;
   struct vmw_svga_winsys_shader *dst;

   if(pdst == NULL || *pdst == src)
      return;

   dst = *pdst;

   src_ref = src ? &src->refcnt : NULL;
   dst_ref = dst ? &dst->refcnt : NULL;

   if (pipe_reference(dst_ref, src_ref)) {
      struct svga_winsys_screen *sws = &dst->screen->base;

      vmw_ioctl_shader_destroy(dst->screen, dst->shid);
#ifdef DEBUG
      /* to detect dangling pointers */
      assert(p_atomic_read(&dst->validated) == 0);
      dst->shid = SVGA3D_INVALID_ID;
#endif
      sws->buffer_destroy(sws, dst->buf);
      FREE(dst);
   }

   *pdst = src;
}
