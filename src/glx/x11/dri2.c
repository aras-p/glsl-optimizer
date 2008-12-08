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
#include <X11/extensions/dri2proto.h>
#include "xf86drm.h"
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

Bool DRI2Connect(Display *dpy, XID window,
		 char **driverName, char **deviceName)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2ConnectReply rep;
    xDRI2ConnectReq *req;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2Connect, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2Connect;
    req->window = window;
    req->driverType = DRI2DriverDRI;
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    if (rep.driverNameLength == 0 && rep.deviceNameLength == 0) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    *driverName = Xmalloc(rep.driverNameLength + 1);
    if (*driverName == NULL) {
	_XEatData(dpy, 
		  ((rep.driverNameLength + 3) & ~3) +
		  ((rep.deviceNameLength + 3) & ~3));
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    _XReadPad(dpy, *driverName, rep.driverNameLength);
    (*driverName)[rep.driverNameLength] = '\0';

    *deviceName = Xmalloc(rep.deviceNameLength + 1);
    if (*deviceName == NULL) {
	Xfree(*driverName);
	_XEatData(dpy, ((rep.deviceNameLength + 3) & ~3));
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }
    _XReadPad(dpy, *deviceName, rep.deviceNameLength);
    (*deviceName)[rep.deviceNameLength] = '\0';

    UnlockDisplay(dpy);
    SyncHandle();

    return True;
}

Bool DRI2Authenticate(Display *dpy, XID window, drm_magic_t magic)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2AuthenticateReq *req;
    xDRI2AuthenticateReply rep;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReq(DRI2Authenticate, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2Authenticate;
    req->window = window;
    req->magic = magic;

    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return False;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return rep.authenticated;
}

void DRI2CreateDrawable(Display *dpy, XID drawable)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2CreateDrawableReq *req;

    XextSimpleCheckExtension (dpy, info, dri2ExtensionName);

    LockDisplay(dpy);
    GetReq(DRI2CreateDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2CreateDrawable;
    req->drawable = drawable;
    UnlockDisplay(dpy);
    SyncHandle();
}

void DRI2DestroyDrawable(Display *dpy, XID drawable)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2DestroyDrawableReq *req;

    XextSimpleCheckExtension (dpy, info, dri2ExtensionName);

    XSync(dpy, False);

    LockDisplay(dpy);
    GetReq(DRI2DestroyDrawable, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2DestroyDrawable;
    req->drawable = drawable;
    UnlockDisplay(dpy);
    SyncHandle();
}

DRI2Buffer *DRI2GetBuffers(Display *dpy, XID drawable,
			   int *width, int *height,
			   unsigned int *attachments, int count,
			   int *outCount)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2GetBuffersReply rep;
    xDRI2GetBuffersReq *req;
    DRI2Buffer *buffers;
    xDRI2Buffer repBuffer;
    CARD32 *p;
    int i;

    XextCheckExtension (dpy, info, dri2ExtensionName, False);

    LockDisplay(dpy);
    GetReqExtra(DRI2GetBuffers, count * 4, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2GetBuffers;
    req->drawable = drawable;
    req->count = count;
    p = (CARD32 *) &req[1];
    for (i = 0; i < count; i++)
	p[i] = attachments[i];

    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return NULL;
    }

    *width = rep.width;
    *height = rep.height;
    *outCount = rep.count;

    buffers = Xmalloc(count * sizeof buffers[0]);
    if (buffers == NULL) {
	_XEatData(dpy, rep.count * sizeof repBuffer);
	UnlockDisplay(dpy);
	SyncHandle();
	return NULL;
    }

    for (i = 0; i < rep.count; i++) {
	_XReadPad(dpy, (char *) &repBuffer, sizeof repBuffer);
	buffers[i].attachment = repBuffer.attachment;
	buffers[i].name = repBuffer.name;
	buffers[i].pitch = repBuffer.pitch;
	buffers[i].cpp = repBuffer.cpp;
	buffers[i].flags = repBuffer.flags;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return buffers;
}

void DRI2CopyRegion(Display *dpy, XID drawable, XserverRegion region,
		    CARD32 dest, CARD32 src)
{
    XExtDisplayInfo *info = DRI2FindDisplay(dpy);
    xDRI2CopyRegionReq *req;
    xDRI2CopyRegionReply rep;

    XextSimpleCheckExtension (dpy, info, dri2ExtensionName);

    LockDisplay(dpy);
    GetReq(DRI2CopyRegion, req);
    req->reqType = info->codes->major_opcode;
    req->dri2ReqType = X_DRI2CopyRegion;
    req->drawable = drawable;
    req->region = region;
    req->dest = dest;
    req->src = src;

    _XReply(dpy, (xReply *)&rep, 0, xFalse);

    UnlockDisplay(dpy);
    SyncHandle();
}
