/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef LOADER_H
#define LOADER_H

/* Helpers to figure out driver and device name, eg. from pci-id, etc. */

#define _LOADER_DRI          (1 << 0)
#define _LOADER_GALLIUM      (1 << 1)

int
loader_get_pci_id_for_fd(int fd, int *vendor_id, int *chip_id);

char *
loader_get_driver_for_fd(int fd, unsigned driver_types);

char *
loader_get_device_name_for_fd(int fd);


/* for logging.. keep this aligned with egllog.h so we can just use
 * _eglLog directly.
 */

#define _LOADER_FATAL   0   /* unrecoverable error */
#define _LOADER_WARNING 1   /* recoverable error/problem */
#define _LOADER_INFO    2   /* just useful info */
#define _LOADER_DEBUG   3   /* useful info for debugging */

void
loader_set_logger(void (*logger)(int level, const char *fmt, ...));

#endif /* LOADER_H */
