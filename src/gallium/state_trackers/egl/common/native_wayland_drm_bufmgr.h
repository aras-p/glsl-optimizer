/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2011 Benjamin Franzke <benjaminfranzke@googlemail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NATIVE_WAYLAND_DRM_BUFMGR_H_
#define _NATIVE_WAYLAND_DRM_BUFMGR_H_

typedef int (*wayland_drm_bufmgr_authenticate_func)(void *, uint32_t);

struct native_display_wayland_bufmgr *
wayland_drm_bufmgr_create(wayland_drm_bufmgr_authenticate_func authenticate,
                          void *user_data, char *device_name);

void
wayland_drm_bufmgr_destroy(struct native_display_wayland_bufmgr *bufmgr);

#endif /* _NATIVE_WAYLAND_DRM_BUFMGR_H_ */
