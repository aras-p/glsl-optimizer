/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#ifndef _NATIVE_H_
#define _NATIVE_H_

#include "EGL/egl.h"  /* for EGL native types */
#include "GL/gl.h" /* for GL types needed by __GLcontextModes */
#include "GL/internal/glcore.h"  /* for __GLcontextModes */

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "native_modeset.h"

/**
 * Only color buffers are listed.  The others are allocated privately through,
 * for example, st_renderbuffer_alloc_storage().
 */
enum native_attachment {
   NATIVE_ATTACHMENT_FRONT_LEFT,
   NATIVE_ATTACHMENT_BACK_LEFT,
   NATIVE_ATTACHMENT_FRONT_RIGHT,
   NATIVE_ATTACHMENT_BACK_RIGHT,

   NUM_NATIVE_ATTACHMENTS
};

enum native_param_type {
   /*
    * Return TRUE if window/pixmap surfaces use the buffers of the native
    * types.
    */
   NATIVE_PARAM_USE_NATIVE_BUFFER
};

/**
 * Enumerations for probe results.
 */
enum native_probe_result {
   NATIVE_PROBE_UNKNOWN,
   NATIVE_PROBE_FALLBACK,
   NATIVE_PROBE_SUPPORTED,
   NATIVE_PROBE_EXACT,
};

/**
 * A probe object for display probe.
 */
struct native_probe {
   int magic;
   EGLNativeDisplayType display;
   void *data;

   void (*destroy)(struct native_probe *nprobe);
};

struct native_surface {
   /**
    * Available for caller's use.
    */
   void *user_data;

   void (*destroy)(struct native_surface *nsurf);

   /**
    * Swap the front and back buffers so that the back buffer is visible.  It
    * is no-op if the surface is single-buffered.  The contents of the back
    * buffer after swapping may or may not be preserved.
    */
   boolean (*swap_buffers)(struct native_surface *nsurf);

   /**
    * Make the front buffer visible.  In some native displays, changes to the
    * front buffer might not be visible immediately and require manual flush.
    */
   boolean (*flush_frontbuffer)(struct native_surface *nsurf);

   /**
    * Validate the buffers of the surface.  textures, if not NULL, points to an
    * array of size NUM_NATIVE_ATTACHMENTS and the returned textures are owned
    * by the caller.  A sequence number is also returned.  The caller can use
    * it to check if anything has changed since the last call. Any of the
    * pointers may be NULL and it indicates the caller has no interest in those
    * values.
    *
    * If this function is called multiple times with different attachment
    * masks, those not listed in the latest call might be destroyed.  This
    * behavior might change in the future.
    */
   boolean (*validate)(struct native_surface *nsurf, uint attachment_mask,
                       unsigned int *seq_num, struct pipe_texture **textures,
                       int *width, int *height);

   /**
    * Wait until all native commands affecting the surface has been executed.
    */
   void (*wait)(struct native_surface *nsurf);
};

struct native_config {
   /* __GLcontextModes should go away some day */
   __GLcontextModes mode;
   enum pipe_format color_format;
   enum pipe_format depth_format;
   enum pipe_format stencil_format;

   /* treat it as an additional flag to mode.drawableType */
   boolean scanout_bit;
};

/**
 * A pipe winsys abstracts the OS.  A pipe screen abstracts the graphcis
 * hardware.  A native display consists of a pipe winsys, a pipe screen, and
 * the native display server.
 */
struct native_display {
   /**
    * The pipe screen of the native display.
    *
    * Note that the "flush_frontbuffer" and "update_buffer" callbacks will be
    * overridden.
    */
   struct pipe_screen *screen;

   /**
    * Available for caller's use.
    */
   void *user_data;

   void (*destroy)(struct native_display *ndpy);

   /**
    * Query the parameters of the native display.
    *
    * The return value is defined by the parameter.
    */
   int (*get_param)(struct native_display *ndpy,
                    enum native_param_type param);

   /**
    * Get the supported configs.  The configs are owned by the display, but
    * the returned array should be free()ed.
    *
    * The configs will be converted to EGL config by
    * _eglConfigFromContextModesRec and validated by _eglValidateConfig.
    * Those failing to pass the test will be skipped.
    */
   const struct native_config **(*get_configs)(struct native_display *ndpy,
                                               int *num_configs);

   /**
    * Test if a pixmap is supported by the given config.  Required unless no
    * config has GLX_PIXMAP_BIT set.
    *
    * This function is usually called to find a config that supports a given
    * pixmap.  Thus, it is usually called with the same pixmap in a row.
    */
   boolean (*is_pixmap_supported)(struct native_display *ndpy,
                                  EGLNativePixmapType pix,
                                  const struct native_config *nconf);


   /**
    * Create a window surface.  Required unless no config has GLX_WINDOW_BIT
    * set.
    */
   struct native_surface *(*create_window_surface)(struct native_display *ndpy,
                                                   EGLNativeWindowType win,
                                                   const struct native_config *nconf);

   /**
    * Create a pixmap surface.  Required unless no config has GLX_PIXMAP_BIT
    * set.
    */
   struct native_surface *(*create_pixmap_surface)(struct native_display *ndpy,
                                                   EGLNativePixmapType pix,
                                                   const struct native_config *nconf);

   /**
    * Create a pbuffer surface.  Required unless no config has GLX_PBUFFER_BIT
    * set.
    */
   struct native_surface *(*create_pbuffer_surface)(struct native_display *ndpy,
                                                    const struct native_config *nconf,
                                                    uint width, uint height);

   const struct native_display_modeset *modeset;
};

/**
 * The handler for events that a native display may generate.  The events are
 * generated asynchronously and the handler may be called by any thread at any
 * time.
 */
struct native_event_handler {
   /**
    * This function is called when a surface needs to be validated.
    */
   void (*invalid_surface)(struct native_display *ndpy,
                           struct native_surface *nsurf,
                           unsigned int seq_num);
};

/**
 * Test whether an attachment is set in the mask.
 */
static INLINE boolean
native_attachment_mask_test(uint mask, enum native_attachment att)
{
   return !!(mask & (1 << att));
}

/**
 * Return a probe object for the given display.
 *
 * Note that the returned object may be cached and used by different native
 * display modules.  It allows fast probing when multiple modules probe the
 * same display.
 */
struct native_probe *
native_create_probe(EGLNativeDisplayType dpy);

/**
 * Probe the probe object.
 */
enum native_probe_result
native_get_probe_result(struct native_probe *nprobe);

const char *
native_get_name(void);

struct native_display *
native_create_display(EGLNativeDisplayType dpy,
                      struct native_event_handler *handler);

#endif /* _NATIVE_H_ */
