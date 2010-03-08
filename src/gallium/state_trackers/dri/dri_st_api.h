/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef _DRI_ST_API_H_
#define _DRI_ST_API_H_

#include "state_tracker/st_api.h"

struct dri_screen;
struct dri_drawable;

struct st_api *
dri_get_st_api(void);

struct st_manager *
dri_create_st_manager(struct dri_screen *screen);

void
dri_destroy_st_manager(struct st_manager *smapi);

struct st_framebuffer_iface *
dri_create_st_framebuffer(struct dri_drawable *drawable);

void
dri_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi);

struct pipe_texture *
dri_get_st_framebuffer_texture(struct st_framebuffer_iface *stfbi,
                               enum st_attachment_type statt);

#endif /* _DRI_ST_API_H_ */
