/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#define _BSD_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gbm.h"
#include "gbmint.h"
#include "common.h"
#include "backend.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

struct gbm_device *devices[16];

static int device_num = 0;

/** Returns the file description for the gbm device
 *
 * \return The fd that the struct gbm_device was created with
 */
GBM_EXPORT int
gbm_device_get_fd(struct gbm_device *gbm)
{
   return gbm->fd;
}

/* FIXME: maybe superfluous, use udev subclass from the fd? */
/** Get the backend name for the given gbm device
 *
 * \return The backend name string - this belongs to the device and must not
 * be freed
 */
GBM_EXPORT const char *
gbm_device_get_backend_name(struct gbm_device *gbm)
{
   return gbm->name;
}

/** Test if a format is supported for a given set of usage flags.
 *
 * \param gbm The created buffer manager
 * \param format The format to test
 * \param usage A bitmask of the usages to test the format against
 * \return 1 if the format is supported otherwise 0
 *
 * \sa enum gbm_bo_flags for the list of flags that the format can be
 * tested against
 *
 * \sa enum gbm_bo_format for the list of formats
 */
int
gbm_device_is_format_supported(struct gbm_device *gbm,
                               uint32_t format, uint32_t usage)
{
   return gbm->is_format_supported(gbm, format, usage);
}

/** Destroy the gbm device and free all resources associated with it.
 *
 * \param gbm The device created using gbm_create_device()
 */
GBM_EXPORT void
gbm_device_destroy(struct gbm_device *gbm)
{
   gbm->refcount--;
   if (gbm->refcount == 0)
      gbm->destroy(gbm);
}

GBM_EXPORT struct gbm_device *
_gbm_mesa_get_device(int fd)
{
   struct gbm_device *gbm = NULL;
   struct stat buf;
   dev_t dev;
   int i;

   if (fd < 0 || fstat(fd, &buf) < 0 || !S_ISCHR(buf.st_mode)) {
      fprintf(stderr, "_gbm_mesa_get_device: invalid fd: %d\n", fd);
      return NULL;
   }

   for (i = 0; i < device_num; ++i) {
      dev = devices[i]->stat.st_rdev;
      if (major(dev) == major(buf.st_rdev) &&
          minor(dev) == minor(buf.st_rdev)) {
         gbm = devices[i];
         gbm->refcount++;
         break;
      }
   }

   return gbm;
}

/** Create a gbm device for allocating buffers
 *
 * The file descriptor passed in is used by the backend to communicate with
 * platform for allocating the memory. For allocations using DRI this would be
 * the file descriptor returned when opening a device such as \c
 * /dev/dri/card0
 *
 * \param fd The file descriptor for an backend specific device
 * \return The newly created struct gbm_device. The resources associated with
 * the device should be freed with gbm_device_destroy() when it is no longer
 * needed. If the creation of the device failed NULL will be returned.
 */
GBM_EXPORT struct gbm_device *
gbm_create_device(int fd)
{
   struct gbm_device *gbm = NULL;
   struct stat buf;

   if (fd < 0 || fstat(fd, &buf) < 0 || !S_ISCHR(buf.st_mode)) {
      fprintf(stderr, "gbm_create_device: invalid fd: %d\n", fd);
      return NULL;
   }

   if (device_num == 0)
      memset(devices, 0, sizeof devices);

   gbm = _gbm_create_device(fd);
   if (gbm == NULL)
      return NULL;

   gbm->dummy = gbm_create_device;
   gbm->stat = buf;
   gbm->refcount = 1;

   if (device_num < ARRAY_SIZE(devices)-1)
      devices[device_num++] = gbm;

   return gbm;
}

/** Get the width of the buffer object
 *
 * \param bo The buffer object
 * \return The width of the allocated buffer object
 *
 */
GBM_EXPORT unsigned int
gbm_bo_get_width(struct gbm_bo *bo)
{
   return bo->width;
}

/** Get the height of the buffer object
 *
 * \param bo The buffer object
 * \return The height of the allocated buffer object
 */
GBM_EXPORT unsigned int
gbm_bo_get_height(struct gbm_bo *bo)
{
   return bo->height;
}

/** Get the stride of the buffer object
 *
 * This is calculated by the backend when it does the allocation in
 * gbm_bo_create()
 *
 * \param bo The buffer object
 * \return The stride of the allocated buffer object
 */
GBM_EXPORT uint32_t
gbm_bo_get_pitch(struct gbm_bo *bo)
{
   return bo->pitch;
}

/** Get the format of the buffer object
 *
 * The format of the pixels in the buffer.
 *
 * \param bo The buffer object
 * \return The format of buffer object, on of the GBM_FORMAT_* codes
 */
GBM_EXPORT uint32_t
gbm_bo_get_format(struct gbm_bo *bo)
{
   return bo->format;
}

/** Get the handle of the buffer object
 *
 * This is stored in the platform generic union gbm_bo_handle type. However
 * the format of this handle is platform specific.
 *
 * \param bo The buffer object
 * \return Returns the handle of the allocated buffer object
 */
GBM_EXPORT union gbm_bo_handle
gbm_bo_get_handle(struct gbm_bo *bo)
{
   return bo->handle;
}

/** Set the user data associated with a buffer object
 *
 * \param bo The buffer object
 * \param data The data to associate to the buffer object
 * \param destroy_user_data A callback (which may be %NULL) that will be
 * called prior to the buffer destruction
 */
GBM_EXPORT void
gbm_bo_set_user_data(struct gbm_bo *bo, void *data,
		     void (*destroy_user_data)(struct gbm_bo *, void *))
{
   bo->user_data = data;
   bo->destroy_user_data = destroy_user_data;
}

/** Get the user data associated with a buffer object
 *
 * \param bo The buffer object
 * \return Returns the user data associated with the buffer object or %NULL
 * if no data was associated with it
 *
 * \sa gbm_bo_set_user_data()
 */
GBM_EXPORT void *
gbm_bo_get_user_data(struct gbm_bo *bo)
{
   return bo->user_data;
}

/**
 * Destroys the given buffer object and frees all resources associated with
 * it.
 *
 * \param bo The buffer object
 */
GBM_EXPORT void
gbm_bo_destroy(struct gbm_bo *bo)
{
   if (bo->destroy_user_data)
      bo->destroy_user_data(bo, bo->user_data);

   bo->gbm->bo_destroy(bo);
}

/**
 * Allocate a buffer object for the given dimensions
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param width The width for the buffer
 * \param height The height for the buffer
 * \param format The format to use for the buffer
 * \param usage The union of the usage flags for this buffer
 *
 * \return A newly allocated buffer that should be freed with gbm_bo_destroy()
 * when no longer needed. If an error occurs during allocation %NULL will be
 * returned.
 *
 * \sa enum gbm_bo_format for the list of formats
 * \sa enum gbm_bo_flags for the list of usage flags
 */
GBM_EXPORT struct gbm_bo *
gbm_bo_create(struct gbm_device *gbm,
              uint32_t width, uint32_t height,
              uint32_t format, uint32_t usage)
{
   if (width == 0 || height == 0)
      return NULL;

   if (usage & GBM_BO_USE_CURSOR_64X64 &&
       (width != 64 || height != 64))
      return NULL;

   return gbm->bo_create(gbm, width, height, format, usage);
}

/**
 * Create a buffer object representing the contents of an EGLImage
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param egl_dpy The EGLDisplay on which the EGLImage was created
 * \param egl_image The EGLImage to create the buffer from
 * \param width The width to use in the creation of the buffer object
 * \param height The height to use in the creation of the buffer object
 * \param usage The union of the usage flags for this buffer
 *
 * \return A newly allocated buffer object that should be freed with
 * gbm_bo_destroy() when no longer needed.
 *
 * \sa enum gbm_bo_flags for the list of usage flags
 *
 * \note The expectation is that this function will use an efficient method
 * for making the contents of the EGLImage available as a buffer object.
 */
GBM_EXPORT struct gbm_bo *
gbm_bo_create_from_egl_image(struct gbm_device *gbm,
                             void *egl_dpy, void *egl_image,
                             uint32_t width, uint32_t height,
                             uint32_t usage)
{
   if (width == 0 || height == 0)
      return NULL;

   return gbm->bo_create_from_egl_image(gbm, egl_dpy, egl_image,
                                        width, height, usage);
}

/**
 * Allocate a surface object
 *
 * \param gbm The gbm device returned from gbm_create_device()
 * \param width The width for the surface
 * \param height The height for the surface
 * \param format The format to use for the surface
 *
 * \return A newly allocated surface that should be freed with
 * gbm_surface_destroy() when no longer needed. If an error occurs
 * during allocation %NULL will be returned.
 *
 * \sa enum gbm_bo_format for the list of formats
 */
GBM_EXPORT struct gbm_surface *
gbm_surface_create(struct gbm_device *gbm,
                   uint32_t width, uint32_t height,
		   uint32_t format, uint32_t flags)
{
   return gbm->surface_create(gbm, width, height, format, flags);
}

/**
 * Destroys the given surface and frees all resources associated with
 * it.
 *
 * All buffers locked with gbm_surface_lock_front_buffer() should be
 * released prior to calling this function.
 *
 * \param surf The surface
 */
GBM_EXPORT void
gbm_surface_destroy(struct gbm_surface *surf)
{
   surf->gbm->surface_destroy(surf);
}

/**
 * Lock the surface's current front buffer
 *
 * Lock rendering to the surface's current front buffer until it is
 * released with gbm_surface_release_buffer().
 *
 * This function must be called exactly once after calling
 * eglSwapBuffers.  Calling it before any eglSwapBuffer has happened
 * on the surface or two or more times after eglSwapBuffers is an
 * error.  A new bo representing the new front buffer is returned.  On
 * multiple invocations, all the returned bos must be released in
 * order to release the actual surface buffer.
 *
 * \param surf The surface
 *
 * \return A newly allocated buffer object that should be released
 * with gbm_surface_release_buffer() when no longer needed.  This bo
 * should not be destroyed using gbm_bo_destroy().  If an error occurs
 * this function returns %NULL.
 */
GBM_EXPORT struct gbm_bo *
gbm_surface_lock_front_buffer(struct gbm_surface *surf)
{
   return surf->gbm->surface_lock_front_buffer(surf);
}

/**
 * Release a locked buffer obtained with gbm_surface_lock_front_buffer()
 *
 * The bo is destroyed after a call to this function and returns the
 * underlying buffer to the gbm surface.  Releasing a bo will
 * typically make gbm_surface_has_free_buffer() return 1 and thus
 * allow rendering the next frame, but not always.
 *
 * \param surf The surface
 * \param bo The buffer object
 */
GBM_EXPORT void
gbm_surface_release_buffer(struct gbm_surface *surf, struct gbm_bo *bo)
{
   surf->gbm->surface_release_buffer(surf, bo);
}

/**
 * Return whether or not a surface has free (non-locked) buffers
 *
 * Before starting a new frame, the surface must have a buffer
 * available for rendering.  Initially, a gbm surface will have a free
 * buffer, but after one of more buffers have been locked (\sa
 * gbm_surface_lock_front_buffer()), the application must check for a
 * free buffer before rendering.
 *
 * If a surface doesn't have a free buffer, the application must
 * return a buffer to the surface using gbm_surface_release_buffer()
 * and after that, the application can query for free buffers again.
 *
 * \param surf The surface
 * \return 1 if the surface has free buffers, 0 otherwise
 */
GBM_EXPORT int
gbm_surface_has_free_buffers(struct gbm_surface *surf)
{
   return surf->gbm->surface_has_free_buffers(surf);
}
