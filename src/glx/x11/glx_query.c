/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glx_query.c
 * Generic utility functions to query internal data from the server.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "glxclient.h"

/**
 * GLX protocol structure for the ficticious "GXLGenericGetString" request.
 * 
 * This is a non-existant protocol packet.  It just so happens that all of
 * the real protocol packets used to request a string from the server have
 * an identical binary layout.  The only difference between them is the
 * meaning of the \c for_whom field and the value of the \c glxCode.
 */
typedef struct GLXGenericGetString {
    CARD8 reqType;
    CARD8 glxCode;
    CARD16 length B16;
    CARD32 for_whom B32;
    CARD32 name B32;
} xGLXGenericGetStringReq;

/* These defines are only needed to make the GetReq macro happy.
 */
#define sz_xGLXGenericGetStringReq 12
#define X_GLXGenericGetString 0

/**
 * Query the Server GLX string and cache it in the display private.
 * This routine will allocate the necessay space for the string.
 */
char *
__glXGetStringFromServer( Display * dpy, int opcode, CARD32 glxCode,
			  CARD32 for_whom, CARD32 name )
{
    xGLXGenericGetStringReq *req;
    xGLXSingleReply reply;
    int length;
    int numbytes;
    char * buf;


    LockDisplay( dpy );


    /* All of the GLX protocol requests for getting a string from the server
     * look the same.  The exact meaning of the for_whom field is usually
     * either the screen number (for glXQueryServerString) or the context tag
     * (for GLXSingle).
     */

    GetReq( GLXGenericGetString, req );
    req->reqType = opcode;
    req->glxCode = glxCode;
    req->for_whom = for_whom;
    req->name = name;

    _XReply( dpy, (xReply *) & reply, 0, False );

    length = reply.length * 4;
    numbytes = reply.size;

    buf = (char *) Xmalloc( numbytes );
    if ( buf != NULL ) {
        _XRead( dpy, buf, numbytes );
	length -= numbytes;
    }

    _XEatData( dpy, length );

    UnlockDisplay( dpy );
    SyncHandle();

    return buf;
}
