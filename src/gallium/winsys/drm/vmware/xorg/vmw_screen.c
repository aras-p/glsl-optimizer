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

static Bool
vmw_screen_init(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct vmw_driver *vmw;

    /* if gallium is used then we don't need to do anything. */
    if (ms->screen)
	return TRUE;

    vmw = xnfcalloc(sizeof(*vmw), 1);
    if (!vmw)
	return FALSE;

    vmw->fd = ms->fd;
    ms->winsys_priv = vmw;

    return TRUE;
}

static Bool
vmw_screen_close(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct vmw_driver *vmw = vmw_driver(pScrn);

    if (!vmw)
	return TRUE;

    ms->winsys_priv = NULL;
    xfree(vmw);

    return TRUE;
}

/*
 * Functions for setting up hooks into the xorg state tracker
 */

static Bool (*vmw_screen_pre_init_saved)(ScrnInfoPtr pScrn, int flags) = NULL;

static Bool
vmw_screen_pre_init(ScrnInfoPtr pScrn, int flags)
{
    modesettingPtr ms;

    pScrn->PreInit = vmw_screen_pre_init_saved;
    if (!pScrn->PreInit(pScrn, flags))
	return FALSE;

    ms = modesettingPTR(pScrn);
    ms->winsys_screen_init = vmw_screen_init;
    ms->winsys_screen_close = vmw_screen_close;

    return TRUE;
}

void
vmw_screen_set_functions(ScrnInfoPtr pScrn)
{
    assert(!vmw_screen_pre_init_saved);

    vmw_screen_pre_init_saved = pScrn->PreInit;
    pScrn->PreInit = vmw_screen_pre_init;
}
