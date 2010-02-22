/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef ST_PUBLIC_H
#define ST_PUBLIC_H

#include "GL/gl.h"
#include "GL/internal/glcore.h"  /* for __GLcontextModes */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

/** Renderbuffer surfaces (should match Mesa names) */
#define ST_SURFACE_FRONT_LEFT   0
#define ST_SURFACE_BACK_LEFT    1
#define ST_SURFACE_FRONT_RIGHT  2
#define ST_SURFACE_BACK_RIGHT   3
#define ST_SURFACE_DEPTH        4

#define ST_TEXTURE_2D    0x2
#define ST_TEXTURE_RECT  0x4

#define ST_TEXTURE_RGB   0x1
#define ST_TEXTURE_RGBA  0x2


struct st_context;
struct st_framebuffer;
struct pipe_context;
struct pipe_fence_handle;
struct pipe_surface;
struct pipe_texture;


PUBLIC
struct st_context *st_create_context(struct pipe_context *pipe,
                                     const __GLcontextModes *visual,
                                     struct st_context *share);

PUBLIC
void st_destroy_context( struct st_context *st );

PUBLIC
void st_copy_context_state(struct st_context *dst, struct st_context *src,
                           uint mask);

PUBLIC
struct st_framebuffer *st_create_framebuffer( const __GLcontextModes *visual,
                                              enum pipe_format colorFormat,
                                              enum pipe_format depthFormat,
                                              enum pipe_format stencilFormat,
                                              uint width, uint height,
                                              void *privateData);

PUBLIC
void st_resize_framebuffer( struct st_framebuffer *stfb,
                            uint width, uint height );

PUBLIC
void st_set_framebuffer_surface(struct st_framebuffer *stfb,
                                uint surfIndex, struct pipe_surface *surf);

PUBLIC
void st_get_framebuffer_dimensions( struct st_framebuffer *stfb,
				    uint *width, uint *height);

PUBLIC
int st_get_framebuffer_surface(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_surface **surface);

PUBLIC
int st_get_framebuffer_texture(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_texture **texture);

PUBLIC
void *st_framebuffer_private( struct st_framebuffer *stfb );

PUBLIC
void st_unreference_framebuffer( struct st_framebuffer *stfb );

PUBLIC
GLboolean st_make_current(struct st_context *st,
                          struct st_framebuffer *draw,
                          struct st_framebuffer *read);

PUBLIC
struct st_context *st_get_current(void);

PUBLIC
void st_flush( struct st_context *st, uint pipeFlushFlags,
               struct pipe_fence_handle **fence );
PUBLIC
void st_finish( struct st_context *st );

PUBLIC
void st_notify_swapbuffers(struct st_framebuffer *stfb);

PUBLIC
void st_swapbuffers(struct st_framebuffer *stfb,
                    struct pipe_surface **front_left,
                    struct pipe_surface **front_right);

PUBLIC
int st_bind_texture_surface(struct pipe_surface *ps, int target, int level,
                            enum pipe_format format);
PUBLIC
int st_unbind_texture_surface(struct pipe_surface *ps, int target, int level);

/** Redirect rendering into stfb's surface to a texture image */
PUBLIC
int st_bind_teximage(struct st_framebuffer *stfb, uint surfIndex,
                     int target, int format, int level);

/** Undo surface-to-texture binding */
PUBLIC
int st_release_teximage(struct st_framebuffer *stfb, uint surfIndex,
                        int target, int format, int level);


/** Generic function type */
typedef void (*st_proc)();

PUBLIC
st_proc st_get_proc_address(const char *procname);


#endif
