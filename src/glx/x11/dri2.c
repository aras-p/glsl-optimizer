/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */


#define NEED_REPLIES
#include <X11/Xlibint.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include "glheader.h"
#include "xf86drm.h"
#include "dri2proto.h"
#include "dri2.h"

static char dri2ExtensionName[] = DRI2_NAME;
static XExtensionInfo *dri2Info;
static XEXT_GENERATE_CLOSE_DISPLAY (DRI2CloseDisplay, dri2Info)
static /* const */ XExtensionHooks dri2ExtensionHooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    DRI2CloseDisplay,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (DRI2FindDisplay, dri2Info, 
				   dri2ExtensionName, 
				   &dri2ExtensionHooks, 
				   0, NULL)

Bool DRI2QueryExtension(Display *dpy, int *eventBase, int *errorBase)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);

    if (XextHasExtension(info)) {
	*eventBase = info->codes->first_event;
	*errorBase = info->codes->first_error;
	return True;
    }

    return False;
}

Bool DRI2QueryVersion(Display *dpy, int *major, int *minor)
{
    XExtDisplayInfo *info = DRI2FindDisplay (dpy);
    xDRI2QueryVersionReply rep;
    xDRI2QueryVersionReq *req;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2QueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2QueryVersion;
    req->majorVersion = DRI2_MAJOR;
    req->minorVersion = DRI2_MINOR;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    *major = rep.majorVersion;
    *minor = rep.minorVersion;
    UnlockDisplay(dpy);
    SyncHandle();

    return True;
}

Bool DRI2Connect(Display *dpy, int screen,
		 char **driverName, char **busId, unsigned int *sareaHandle)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2ConnectReply rep;
    xDRI2ConnectReq *req;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2Connect, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2Connect;
    req->screen = screen;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *sareaHandle = rep.sareaHandle;

    *driverName = Xmalloc(rep.driverNameLength + 1);
    if (*driverName == NULL) {
	_XEatData(dpy, 
		  ((rep.driverNameLength + 3) & ~3) +
		  ((rep.busIdLength + 3) & ~3));
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    _XReadPad(dpy, *driverName, rep.driverNameLength);
    (*driverName)[rep.driverNameLength] = '\0';

    *busId = Xmalloc(rep.busIdLength + 1);
    if (*busId == NULL) {
	Xfree(*driverName);
	_XEatData(dpy, ((rep.busIdLength + 3) & ~3));
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    _XReadPad(dpy, *busId, rep.busIdLength);
    (*busId)[rep.busIdLength] = '\0';

    UnlockDisplay(dpy);
    SyncHandle();

    return rep.sareaHandle != 0;
}

Bool DRI2AuthConnection(Display *dpy, int screen, drm_magic_t magic)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2AuthConnectionReq *req;
    xDRI2AuthConnectionReply rep;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2AuthConnection, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2AuthConnection;
    req->screen = screen;
    req->magic = magic;
    rep.authenticated = 0;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();

    return rep.authenticated;
}

Bool DRI2CreateDrawable(Display *dpy, XID drawable,
			unsigned int *handle, unsigned int *head)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2CreateDrawableReply rep;
    xDRI2CreateDrawableReq *req;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2CreateDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2CreateDrawable;
    req->drawable = drawable;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();

    *handle = rep.handle;
    *head = rep.head;

    return True;
}

void DRI2DestroyDrawable(Display *dpy, XID drawable)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2DestroyDrawableReq *req;

    XextSimpleCheckExtension (dpy, info, dri2ExtensionName);

    XSync(dpy, GL_FALSE);

    LockDisplay(dpy);
    GetReq(DRI2DestroyDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2DestroyDrawable;
    req->drawable = drawable;
    UnlockDisplay(dpy);
    SyncHandle();
}

Bool DRI2ReemitDrawableInfo(Display *dpy, XID drawable, unsigned int *head)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2ReemitDrawableInfoReply rep;
    xDRI2ReemitDrawableInfoReq *req;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2ReemitDrawableInfo, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2ReemitDrawableInfo;
    req->drawable = drawable;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();

    *head = rep.head;

    return True;
}
