/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
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


#include "pipe/p_compiler.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "vmw_screen.h"

#include "trace/tr_drm.h"

#include "vmw_screen.h"
#include "vmw_surface.h"
#include "vmw_fence.h"
#include "vmw_context.h"

#include <state_tracker/dri1_api.h>
#include <state_tracker/drm_api.h>
#include <vmwgfx_drm.h>
#include <xf86drm.h>

#include <stdio.h>

static struct dri1_api dri1_api_hooks;
static struct dri1_api_version ddx_required = { 0, 1, 0 };
static struct dri1_api_version ddx_compat = { 0, 0, 0 };
static struct dri1_api_version dri_required = { 4, 0, 0 };
static struct dri1_api_version dri_compat = { 4, 0, 0 };
static struct dri1_api_version drm_required = { 1, 0, 0 };
static struct dri1_api_version drm_compat = { 1, 0, 0 };
static struct dri1_api_version drm_scanout = { 0, 9, 0 };

static boolean
vmw_dri1_check_version(const struct dri1_api_version *cur,
		       const struct dri1_api_version *required,
		       const struct dri1_api_version *compat,
		       const char component[])
{
   if (cur->major > required->major && cur->major <= compat->major)
      return TRUE;
   if (cur->major == required->major && cur->minor >= required->minor)
      return TRUE;

   fprintf(stderr, "%s version failure.\n", component);
   fprintf(stderr, "%s version is %d.%d.%d and this driver can only work\n"
	   "with versions %d.%d.x through %d.x.x.\n",
	   component,
	   cur->major,
	   cur->minor,
	   cur->patch_level, required->major, required->minor, compat->major);
   return FALSE;
}

/* This is actually the entrypoint to the entire driver, called by the
 * libGL (or EGL, or ...) code via the drm_api_hooks table at the
 * bottom of the file.
 */
static struct pipe_screen *
vmw_drm_create_screen(struct drm_api *drm_api,
                      int fd,
                      struct drm_create_screen_arg *arg)
{
   struct vmw_winsys_screen *vws;
   struct pipe_screen *screen;
   struct dri1_create_screen_arg *dri1;
   boolean use_old_scanout_flag = FALSE;

   if (!arg || arg->mode == DRM_CREATE_NORMAL) {
      struct dri1_api_version drm_ver;
      drmVersionPtr ver;

      ver = drmGetVersion(fd);
      if (ver == NULL)
	 return NULL;

      drm_ver.major = ver->version_major;
      drm_ver.minor = ver->version_minor;
      drm_ver.patch_level = 0; /* ??? */

      drmFreeVersion(ver);
      if (!vmw_dri1_check_version(&drm_ver, &drm_required,
				  &drm_compat, "vmwgfx drm driver"))
	 return NULL;

      if (!vmw_dri1_check_version(&drm_ver, &drm_scanout,
				  &drm_compat, "use old scanout field (not a error)"))
         use_old_scanout_flag = TRUE;
   }

   if (arg != NULL) {
      switch (arg->mode) {
      case DRM_CREATE_NORMAL:
	 break;
      case DRM_CREATE_DRI1:
	 dri1 = (struct dri1_create_screen_arg *)arg;
	 if (!vmw_dri1_check_version(&dri1->ddx_version, &ddx_required,
				     &ddx_compat, "ddx - driver api"))
	    return NULL;
	 if (!vmw_dri1_check_version(&dri1->dri_version, &dri_required,
				     &dri_compat, "dri info"))
	    return NULL;
	 if (!vmw_dri1_check_version(&dri1->drm_version, &drm_required,
				     &drm_compat, "vmwgfx drm driver"))
	    return NULL;
	 if (!vmw_dri1_check_version(&dri1->drm_version, &drm_scanout,
				     &drm_compat, "use old scanout field (not a error)"))
	    use_old_scanout_flag = TRUE;
	 dri1->api = &dri1_api_hooks;
	 break;
      default:
	 return NULL;
      }
   }

   vws = vmw_winsys_create( fd, use_old_scanout_flag );
   if (!vws)
      goto out_no_vws;

   screen = svga_screen_create( &vws->base );
   if (!screen)
      goto out_no_screen;

   return screen;

   /* Failure cases:
    */
out_no_screen:
   vmw_winsys_destroy( vws );

out_no_vws:
   return NULL;
}

static INLINE boolean
vmw_dri1_intersect_src_bbox(struct drm_clip_rect *dst,
			    int dst_x,
			    int dst_y,
			    const struct drm_clip_rect *src,
			    const struct drm_clip_rect *bbox)
{
   int xy1;
   int xy2;

   xy1 = ((int)src->x1 > (int)bbox->x1 + dst_x) ? src->x1 :
      (int)bbox->x1 + dst_x;
   xy2 = ((int)src->x2 < (int)bbox->x2 + dst_x) ? src->x2 :
      (int)bbox->x2 + dst_x;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->x1 = xy1;
   dst->x2 = xy2;

   xy1 = ((int)src->y1 > (int)bbox->y1 + dst_y) ? src->y1 :
      (int)bbox->y1 + dst_y;
   xy2 = ((int)src->y2 < (int)bbox->y2 + dst_y) ? src->y2 :
      (int)bbox->y2 + dst_y;
   if (xy1 >= xy2 || xy1 < 0)
      return FALSE;

   dst->y1 = xy1;
   dst->y2 = xy2;
   return TRUE;
}

/**
 * No fancy get-surface-from-sarea stuff here.
 * Just use the present blit.
 */

static void
vmw_dri1_present_locked(struct pipe_context *locked_pipe,
			struct pipe_surface *surf,
			const struct drm_clip_rect *rect,
			unsigned int num_clip,
			int x_draw, int y_draw,
			const struct drm_clip_rect *bbox,
			struct pipe_fence_handle **p_fence)
{
   struct svga_winsys_surface *srf =
      svga_screen_texture_get_winsys_surface(surf->texture);
   struct vmw_svga_winsys_surface *vsrf = vmw_svga_winsys_surface(srf);
   struct vmw_winsys_screen *vws =
      vmw_winsys_screen(svga_winsys_screen(locked_pipe->screen));
   struct drm_clip_rect clip;
   int i;
   struct
   {
      SVGA3dCmdHeader header;
      SVGA3dCmdPresent body;
      SVGA3dCopyRect rect;
   } cmd;
   boolean visible = FALSE;
   uint32_t fence_seq = 0;

   VMW_FUNC;
   cmd.header.id = SVGA_3D_CMD_PRESENT;
   cmd.header.size = sizeof cmd.body + sizeof cmd.rect;
   cmd.body.sid = vsrf->sid;

   for (i = 0; i < num_clip; ++i) {
      if (!vmw_dri1_intersect_src_bbox(&clip, x_draw, y_draw, rect++, bbox))
	 continue;

      cmd.rect.x = clip.x1;
      cmd.rect.y = clip.y1;
      cmd.rect.w = clip.x2 - clip.x1;
      cmd.rect.h = clip.y2 - clip.y1;
      cmd.rect.srcx = (int)clip.x1 - x_draw;
      cmd.rect.srcy = (int)clip.y1 - y_draw;

      vmw_printf("%s: Clip %d x %d y %d w %d h %d srcx %d srcy %d\n",
		   __FUNCTION__,
		   i,
		   cmd.rect.x,
		   cmd.rect.y,
		   cmd.rect.w, cmd.rect.h, cmd.rect.srcx, cmd.rect.srcy);

      vmw_ioctl_command(vws, &cmd, sizeof cmd.header + cmd.header.size,
                        &fence_seq);
      visible = TRUE;
   }

   *p_fence = (visible) ? vmw_pipe_fence(fence_seq) : NULL;
   vmw_svga_winsys_surface_reference(&vsrf, NULL);
}

static struct pipe_texture *
vmw_drm_texture_from_handle(struct drm_api *drm_api,
			    struct pipe_screen *screen,
			    struct pipe_texture *templat,
			    const char *name,
			    unsigned stride,
			    unsigned handle)
{
    struct vmw_svga_winsys_surface *vsrf;
    struct svga_winsys_surface *ssrf;
    struct vmw_winsys_screen *vws =
	vmw_winsys_screen(svga_winsys_screen(screen));
    struct pipe_texture *tex;
    union drm_vmw_surface_reference_arg arg;
    struct drm_vmw_surface_arg *req = &arg.req;
    struct drm_vmw_surface_create_req *rep = &arg.rep;
    int ret;
    int i;

    /**
     * The vmware device specific handle is the hardware SID.
     * FIXME: We probably want to move this to the ioctl implementations.
     */

    memset(&arg, 0, sizeof(arg));
    req->sid = handle;

    ret = drmCommandWriteRead(vws->ioctl.drm_fd, DRM_VMW_REF_SURFACE,
			      &arg, sizeof(arg));

    if (ret) {
	fprintf(stderr, "Failed referencing shared surface. SID %d.\n"
		"Error %d (%s).\n",
		handle, ret, strerror(-ret));
	return NULL;
    }

    if (rep->mip_levels[0] != 1) {
	fprintf(stderr, "Incorrect number of mipmap levels on shared surface."
		" SID %d, levels %d\n",
		handle, rep->mip_levels[0]);
	goto out_mip;
    }

    for (i=1; i < DRM_VMW_MAX_SURFACE_FACES; ++i) {
	if (rep->mip_levels[i] != 0) {
	    fprintf(stderr, "Incorrect number of faces levels on shared surface."
		    " SID %d, face %d present.\n",
		    handle, i);
	    goto out_mip;
	}
    }

    vsrf = CALLOC_STRUCT(vmw_svga_winsys_surface);
    if (!vsrf)
	goto out_mip;

    pipe_reference_init(&vsrf->refcnt, 1);
    p_atomic_set(&vsrf->validated, 0);
    vsrf->screen = vws;
    vsrf->sid = handle;
    ssrf = svga_winsys_surface(vsrf);
    tex = svga_screen_texture_wrap_surface(screen, templat, rep->format, ssrf);
    if (!tex)
	vmw_svga_winsys_surface_reference(&vsrf, NULL);

    return tex;
  out_mip:
    vmw_ioctl_surface_destroy(vws, handle);
    return NULL;
}

static boolean
vmw_drm_handle_from_texture(struct drm_api *drm_api,
                           struct pipe_screen *screen,
			   struct pipe_texture *texture,
			   unsigned *stride,
			   unsigned *handle)
{
    struct svga_winsys_surface *surface =
	svga_screen_texture_get_winsys_surface(texture);
    struct vmw_svga_winsys_surface *vsrf;

    if (!surface)
	return FALSE;

    vsrf = vmw_svga_winsys_surface(surface);
    *handle = vsrf->sid;
    *stride = util_format_get_nblocksx(texture->format, texture->width0) *
       util_format_get_blocksize(texture->format);

    vmw_svga_winsys_surface_reference(&vsrf, NULL);
    return TRUE;
}


static struct dri1_api dri1_api_hooks = {
   .front_srf_locked = NULL,
   .present_locked = vmw_dri1_present_locked
};

static struct drm_api vmw_drm_api_hooks = {
   .name = "vmwgfx",
   .driver_name = "vmwgfx",
   .create_screen = vmw_drm_create_screen,
   .texture_from_shared_handle = vmw_drm_texture_from_handle,
   .shared_handle_from_texture = vmw_drm_handle_from_texture,
   .local_handle_from_texture = vmw_drm_handle_from_texture,
};

struct drm_api* drm_api_create()
{
   return trace_drm_create(&vmw_drm_api_hooks);
}
