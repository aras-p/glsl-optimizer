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

/**
 * @file
 * Contains the init code for the VMware Xorg driver.
 *
 * @author Jakob Bornecrantz <jakob@vmware.com>
 */

#include "vmw_hook.h"
#include "vmw_driver.h"

#include "cursorstr.h"

/* modified version of crtc functions */
xf86CrtcFuncsRec vmw_screen_crtc_funcs;

static void
vmw_screen_cursor_load_argb(xf86CrtcPtr crtc, CARD32 *image)
{
    struct vmw_customizer *vmw =
	vmw_customizer(xorg_customizer(crtc->scrn));
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    xf86CrtcFuncsPtr funcs = vmw->cursor_priv;
    CursorPtr c = config->cursor;

    /* Run the ioctl before uploading the image */
    vmw_ioctl_cursor_bypass(vmw, c->bits->xhot, c->bits->yhot);

    funcs->load_cursor_argb(crtc, image);
}

static void
vmw_screen_cursor_init(struct vmw_customizer *vmw)
{
    ScrnInfoPtr pScrn = vmw->pScrn;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int i;

    /* XXX assume that all crtc's have the same function struct */

    /* Save old struct need to call the old functions as well */
    vmw->cursor_priv = (void*)(config->crtc[0]->funcs);
    memcpy(&vmw_screen_crtc_funcs, vmw->cursor_priv, sizeof(xf86CrtcFuncsRec));
    vmw_screen_crtc_funcs.load_cursor_argb = vmw_screen_cursor_load_argb;

    for (i = 0; i < config->num_crtc; i++)
	config->crtc[i]->funcs = &vmw_screen_crtc_funcs;
}

static void
vmw_screen_cursor_close(struct vmw_customizer *vmw)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(vmw->pScrn);
    int i;

    vmw_ioctl_cursor_bypass(vmw, 0, 0);

    for (i = 0; i < config->num_crtc; i++)
	config->crtc[i]->funcs = vmw->cursor_priv;
}

static Bool
vmw_screen_init(CustomizerPtr cust, int fd)
{
    struct vmw_customizer *vmw = vmw_customizer(cust);

    vmw->fd = fd;
    vmw_screen_cursor_init(vmw);

    /* if gallium is used then we don't need to do anything more. */
    if (xorg_has_gallium(vmw->pScrn))
	return TRUE;

    vmw_video_init(vmw);

    return TRUE;
}

static Bool
vmw_screen_close(CustomizerPtr cust)
{
    struct vmw_customizer *vmw = vmw_customizer(cust);

    if (!vmw)
	return TRUE;

    vmw_screen_cursor_close(vmw);

    vmw_video_close(vmw);

    return TRUE;
}

static Bool
vmw_screen_enter_vt(CustomizerPtr cust)
{
    debug_printf("%s: enter\n", __func__);

    return TRUE;
}

static Bool
vmw_screen_leave_vt(CustomizerPtr cust)
{
    struct vmw_customizer *vmw = vmw_customizer(cust);

    debug_printf("%s: enter\n", __func__);

    vmw_video_stop_all(vmw);

    return TRUE;
}

/*
 * Functions for setting up hooks into the xorg state tracker
 */

static Bool (*vmw_screen_pre_init_saved)(ScrnInfoPtr pScrn, int flags) = NULL;

static Bool
vmw_screen_pre_init(ScrnInfoPtr pScrn, int flags)
{
    struct vmw_customizer *vmw;
    CustomizerPtr cust;

    vmw = xnfcalloc(1, sizeof(*vmw));
    if (!vmw)
	return FALSE;

    cust = &vmw->base;

    cust->winsys_screen_init = vmw_screen_init;
    cust->winsys_screen_close = vmw_screen_close;
    cust->winsys_enter_vt = vmw_screen_enter_vt;
    cust->winsys_leave_vt = vmw_screen_leave_vt;
    vmw->pScrn = pScrn;

    pScrn->driverPrivate = cust;

    pScrn->PreInit = vmw_screen_pre_init_saved;
    if (!pScrn->PreInit(pScrn, flags))
	return FALSE;

    return TRUE;
}

void
vmw_screen_set_functions(ScrnInfoPtr pScrn)
{
    assert(!vmw_screen_pre_init_saved);

    vmw_screen_pre_init_saved = pScrn->PreInit;
    pScrn->PreInit = vmw_screen_pre_init;
}
