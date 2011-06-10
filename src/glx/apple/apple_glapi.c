/*
 * GLX implementation that uses Apple's OpenGL.framework
 *
 * Copyright (c) 2007-2011 Apple Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"
#include "main/dispatch.h"

#include "apple_glx.h"
#include "apple_xgl_api.h"

#ifndef OPENGL_FRAMEWORK_PATH
#define OPENGL_FRAMEWORK_PATH "/System/Library/Frameworks/OpenGL.framework/OpenGL"
#endif

struct _glapi_table * __ogl_framework_api = NULL;
struct _glapi_table * __applegl_api = NULL;

void apple_xgl_init_direct(void) {
    static void *handle;
    const char *opengl_framework_path;

    if(__applegl_api)  {
        _glapi_set_dispatch(__applegl_api);
        return;
    }

    opengl_framework_path = getenv("OPENGL_FRAMEWORK_PATH");
    if (!opengl_framework_path) {
        opengl_framework_path = OPENGL_FRAMEWORK_PATH;
    }

    (void) dlerror();            /*drain dlerror */
    handle = dlopen(opengl_framework_path, RTLD_LOCAL);

    if (!handle) {
        fprintf(stderr, "error: unable to dlopen %s : %s\n",
                opengl_framework_path, dlerror());
        abort();
    }

    __ogl_framework_api = _glapi_create_table_from_handle(handle, "gl");
    assert(__ogl_framework_api);

    __applegl_api = malloc(sizeof(struct _glapi_table));
    assert(__applegl_api);
    memcpy(__applegl_api, __ogl_framework_api, sizeof(struct _glapi_table));

    SET_ReadPixels(__applegl_api, __applegl_glReadPixels);
    SET_CopyPixels(__applegl_api, __applegl_glCopyPixels);
    SET_CopyColorTable(__applegl_api, __applegl_glCopyColorTable);
    SET_DrawBuffer(__applegl_api, __applegl_glDrawBuffer);
    SET_DrawBuffersARB(__applegl_api, __applegl_glDrawBuffersARB);
    SET_Viewport(__applegl_api, __applegl_glViewport);

    _glapi_set_dispatch(__applegl_api);
}
