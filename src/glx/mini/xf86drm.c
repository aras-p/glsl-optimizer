/**
 * \file xf86drm.c 
 * \brief User-level interface to DRM device
 *
 * This file is an user-friendly interface to the DRM ioctls defined in drm.h.
 * 
 * This covers only the device-independent ioctls -- it is up to the driver to
 * wrap the device-dependent ioctls.
 * 
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Kevin E. Martin <martin@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <ctype.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/time.h>
# include <stdarg.h>
# include "drm.h"

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "xf86drm.h"

#ifndef DRM_MAJOR
#define DRM_MAJOR 226		/* Linux */
#endif

#ifndef __linux__
#undef  DRM_MAJOR
#define DRM_MAJOR 145		/* Should set in drm.h for *BSD */
#endif

#ifndef DRM_MAX_MINOR
#define DRM_MAX_MINOR 16
#endif

#ifdef __linux__
#include <sys/sysmacros.h>	/* for makedev() */
#endif

#ifndef makedev
				/* This definition needs to be changed on
                                   some systems if dev_t is a structure.
                                   If there is a header file we can get it
                                   from, there would be best. */
#define makedev(x,y)    ((dev_t)(((x) << 8) | (y)))
#endif


/**
 * \brief Output a message to stderr.
 *
 * \param format printf() like format string.
 *
 * \internal
 * This function is a wrapper around vfprintf().
 */
static void
drmMsg(const char *format, ...)
{
    va_list	ap;

    const char *env;
    if ((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose")) 
    {
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
    }
}



/**
 * \brief Open the DRM device, creating it if necessary.
 *
 * \param dev major and minor numbers of the device.
 * \param minor minor number of the device.
 * 
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * Assembles the device name from \p minor and opens it, creating the device
 * special file node with the major and minor numbers specified by \p dev and
 * parent directory if necessary and was called by root.
 */
static int drmOpenDevice(long dev, int minor)
{
    struct stat          st;
    char            buf[64];
    int             fd;
    mode_t          devmode = DRM_DEV_MODE;
    int             isroot  = !geteuid();
#if defined(XFree86Server)
    uid_t           user    = DRM_DEV_UID;
    gid_t           group   = DRM_DEV_GID;
#endif

    drmMsg("drmOpenDevice: minor is %d\n", minor);

#if defined(XFree86Server)
    devmode  = xf86ConfigDRI.mode ? xf86ConfigDRI.mode : DRM_DEV_MODE;
    devmode &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
    group = (xf86ConfigDRI.group >= 0) ? xf86ConfigDRI.group : DRM_DEV_GID;
#endif

    if (stat(DRM_DIR_NAME, &st)) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	mkdir(DRM_DIR_NAME, DRM_DEV_DIRMODE);
	chown(DRM_DIR_NAME, 0, 0); /* root:root */
	chmod(DRM_DIR_NAME, DRM_DEV_DIRMODE);
    }

    sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, minor);
    drmMsg("drmOpenDevice: node name is %s\n", buf);
    if (stat(buf, &st)) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	remove(buf);
	mknod(buf, S_IFCHR | devmode, dev);
    }
#if defined(XFree86Server)
    chown(buf, user, group);
    chmod(buf, devmode);
#endif

    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
		fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0) return fd;

    if (st.st_rdev != dev) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	remove(buf);
	mknod(buf, S_IFCHR | devmode, dev);
    }
    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
		fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0) return fd;

    drmMsg("drmOpenDevice: Open failed\n");
    remove(buf);
    return -errno;
}


/**
 * \brief Open the DRM device
 *
 * \param minor device minor number.
 * \param create allow to create the device if set.
 *
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * Calls drmOpenDevice() if \p create is set, otherwise assembles the device
 * name from \p minor and opens it.
 */
static int drmOpenMinor(int minor, int create)
{
    int  fd;
    char buf[64];
    
    if (create) return drmOpenDevice(makedev(DRM_MAJOR, minor), minor);
    
    sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, minor);
    if ((fd = open(buf, O_RDWR, 0)) >= 0) return fd;
    drmMsg("drmOpenMinor: open result is %d, (%s)\n",
		fd, fd < 0 ? strerror(errno) : "OK");
    return -errno;
}


/**
 * \brief Determine whether the DRM kernel driver has been loaded.
 * 
 * \return 1 if the DRM driver is loaded, 0 otherwise.
 *
 * \internal 
 * Determine the presence of the kernel driver by attempting to open the 0
 * minor and get version information.  For backward compatibility with older
 * Linux implementations, /proc/dri is also checked.
 */
int drmAvailable(void)
{
    drmVersionPtr version;
    int           retval = 0;
    int           fd;

    if ((fd = drmOpenMinor(0, 1)) < 0) {
				/* Try proc for backward Linux compatibility */
	if (!access("/proc/dri/0", R_OK)) return 1;
	return 0;
    }
    
    if ((version = drmGetVersion(fd))) {
	retval = 1;
	drmFreeVersion(version);
    }
    close(fd);
    drmMsg("close %d\n", fd);

    return retval;
}


/**
 * \brief Open the device by bus ID.
 *
 * \param busid bus ID.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * This function attempts to open every possible minor (up to DRM_MAX_MINOR),
 * comparing the device bus ID with the one supplied.
 *
 * \sa drmOpenMinor() and drmGetBusid().
 */
static int drmOpenByBusid(const char *busid)
{
    int        i;
    int        fd;
    const char *buf;
    
    drmMsg("drmOpenByBusid: busid is %s\n", busid);
    for (i = 0; i < DRM_MAX_MINOR; i++) {
	fd = drmOpenMinor(i, 1);
	drmMsg("drmOpenByBusid: drmOpenMinor returns %d\n", fd);
	if (fd >= 0) {
	    buf = drmGetBusid(fd);
	    drmMsg("drmOpenByBusid: drmGetBusid reports %s\n", buf);
	    if (buf && !strcmp(buf, busid)) {
		drmFreeBusid(buf);
		return fd;
	    }
	    if (buf) drmFreeBusid(buf);
	    close(fd);
	    drmMsg("close %d\n", fd);
	}
    }
    return -1;
}


/**
 * \brief Open the device by name.
 *
 * \param name driver name.
 * 
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * This function opens the first minor number that matches the driver name and
 * isn't already in use.  If it's in use it then it will already have a bus ID
 * assigned.
 * 
 * \sa drmOpenMinor(), drmGetVersion() and drmGetBusid().
 */
static int drmOpenByName(const char *name)
{
    int           i;
    int           fd;
    drmVersionPtr version;
    char *        id;
    
    if (!drmAvailable()) {
#if !defined(XFree86Server)
	return -1;
#else
        /* try to load the kernel module now */
        if (!xf86LoadKernelModule(name)) {
            ErrorF("[drm] failed to load kernel module \"%s\"\n",
		   name);
            return -1;
        }
#endif
    }

    for (i = 0; i < DRM_MAX_MINOR; i++) {
	if ((fd = drmOpenMinor(i, 1)) >= 0) {
	    if ((version = drmGetVersion(fd))) {
		if (!strcmp(version->name, name)) {
		    drmFreeVersion(version);

/* 			return fd; */

		    id = drmGetBusid(fd);
		    drmMsg("drmGetBusid returned '%s'\n", id ? id : "NULL");
		    if (!id || !*id) {
			if (id) {
			    drmFreeBusid(id);
			}
			return fd;
		    } else {
			drmFreeBusid(id);
		    }
		} else {
		    drmFreeVersion(version);
		}
	    }
	    close(fd);
	    drmMsg("close %d\n", fd);
	}
    }

    return -1;
}


/**
 * \brief Open the DRM device.
 *
 * Looks up the specified name and bus ID, and opens the device found.  The
 * entry in /dev/dri is created if necessary and if called by root.
 *
 * \param name driver name. Not referenced if bus ID is supplied.
 * \param busid bus ID. Zero if not known.
 * 
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * It calls drmOpenByBusid() if \p busid is specified or drmOpenByName()
 * otherwise.
 */
int drmOpen(const char *name, const char *busid)
{

    if (busid) return drmOpenByBusid(busid);
    return drmOpenByName(name);
}


/**
 * \brief Free the version information returned by drmGetVersion().
 *
 * \param v pointer to the version information.
 *
 * \internal
 * It frees the memory pointed by \p %v as well as all the non-null strings
 * pointers in it.
 */
void drmFreeVersion(drmVersionPtr v)
{
    if (!v) return;
    if (v->name) free(v->name);
    if (v->date) free(v->date);
    if (v->desc) free(v->desc);
    free(v);
}


/**
 * \brief Free the non-public version information returned by the kernel.
 *
 * \param v pointer to the version information.
 *
 * \internal
 * Used by drmGetVersion() to free the memory pointed by \p %v as well as all
 * the non-null strings pointers in it.
 */
static void drmFreeKernelVersion(drm_version_t *v)
{
    if (!v) return;
    if (v->name) free(v->name);
    if (v->date) free(v->date);
    if (v->desc) free(v->desc);
    free(v);
}


/**
 * \brief Copy version information.
 * 
 * \param d destination pointer.
 * \param s source pointer.
 * 
 * \internal
 * Used by drmGetVersion() to translate the information returned by the ioctl
 * interface in a private structure into the public structure counterpart.
 */
static void drmCopyVersion(drmVersionPtr d, const drm_version_t *s)
{
    d->version_major      = s->version_major;
    d->version_minor      = s->version_minor;
    d->version_patchlevel = s->version_patchlevel;
    d->name_len           = s->name_len;
    d->name               = strdup(s->name);
    d->date_len           = s->date_len;
    d->date               = strdup(s->date);
    d->desc_len           = s->desc_len;
    d->desc               = strdup(s->desc);
}


/**
 * \brief Query the driver version information.
 *
 * \param fd file descriptor.
 * 
 * \return pointer to a drmVersion structure which should be freed with
 * drmFreeVersion().
 * 
 * \note Similar information is available via /proc/dri.
 * 
 * \internal
 * It gets the version information via successive DRM_IOCTL_VERSION ioctls,
 * first with zeros to get the string lengths, and then the actually strings.
 * It also null-terminates them since they might not be already.
 */
drmVersionPtr drmGetVersion(int fd)
{
    drmVersionPtr retval;
    drm_version_t *version = malloc(sizeof(*version));

				/* First, get the lengths */
    version->name_len    = 0;
    version->name        = NULL;
    version->date_len    = 0;
    version->date        = NULL;
    version->desc_len    = 0;
    version->desc        = NULL;

    if (ioctl(fd, DRM_IOCTL_VERSION, version)) {
	drmFreeKernelVersion(version);
	return NULL;
    }

				/* Now, allocate space and get the data */
    if (version->name_len)
	version->name    = malloc(version->name_len + 1);
    if (version->date_len)
	version->date    = malloc(version->date_len + 1);
    if (version->desc_len)
	version->desc    = malloc(version->desc_len + 1);

    if (ioctl(fd, DRM_IOCTL_VERSION, version)) {
	drmFreeKernelVersion(version);
	return NULL;
    }

				/* The results might not be null-terminated
                                   strings, so terminate them. */

    if (version->name_len) version->name[version->name_len] = '\0';
    if (version->date_len) version->date[version->date_len] = '\0';
    if (version->desc_len) version->desc[version->desc_len] = '\0';

				/* Now, copy it all back into the
                                   client-visible data structure... */
    retval = malloc(sizeof(*retval));
    drmCopyVersion(retval, version);
    drmFreeKernelVersion(version);
    return retval;
}


/**
 * \brief Get version information for the DRM user space library.
 * 
 * This version number is driver independent.
 * 
 * \param fd file descriptor.
 *
 * \return version information.
 * 
 * \internal
 * This function allocates and fills a drm_version structure with a hard coded
 * version number.
 */
drmVersionPtr drmGetLibVersion(int fd)
{
    drm_version_t *version = malloc(sizeof(*version));

    /* Version history:
     *   revision 1.0.x = original DRM interface with no drmGetLibVersion
     *                    entry point and many drm<Device> extensions
     *   revision 1.1.x = added drmCommand entry points for device extensions
     *                    added drmGetLibVersion to identify libdrm.a version
     */
    version->version_major      = 1;
    version->version_minor      = 1;
    version->version_patchlevel = 0;

    return (drmVersionPtr)version;
}


/**
 * \brief Free the bus ID information.
 *
 * \param busid bus ID information string as given by drmGetBusid().
 *
 * \internal
 * This function is just frees the memory pointed by \p busid.
 */
void drmFreeBusid(const char *busid)
{
    free((void *)busid);
}


/**
 * \brief Get the bus ID of the device.
 *
 * \param fd file descriptor.
 *
 * \return bus ID string.
 *
 * \internal
 * This function gets the bus ID via successive DRM_IOCTL_GET_UNIQUE ioctls to
 * get the string length and data, passing the arguments in a drm_unique
 * structure.
 */
char *drmGetBusid(int fd)
{
    drm_unique_t u;

    u.unique_len = 0;
    u.unique     = NULL;

    if (ioctl(fd, DRM_IOCTL_GET_UNIQUE, &u)) return NULL;
    u.unique = malloc(u.unique_len + 1);
    if (ioctl(fd, DRM_IOCTL_GET_UNIQUE, &u)) return NULL;
    u.unique[u.unique_len] = '\0';
    return u.unique;
}


/**
 * \brief Set the bus ID of the device.
 *
 * \param fd file descriptor.
 * \param busid bus ID string.
 *
 * \return zero on success, negative on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_SET_UNIQUE ioctl, passing
 * the arguments in a drm_unique structure.
 */
int drmSetBusid(int fd, const char *busid)
{
    drm_unique_t u;

    u.unique     = (char *)busid;
    u.unique_len = strlen(busid);

    if (ioctl(fd, DRM_IOCTL_SET_UNIQUE, &u)) {
	return -errno;
    }
    return 0;
}


/**
 * \brief Specifies a range of memory that is available for mapping by a
 * non-root process.
 *
 * \param fd file descriptor.
 * \param offset usually the physical address. The actual meaning depends of
 * the \p type parameter. See below.
 * \param size of the memory in bytes.
 * \param type type of the memory to be mapped.
 * \param flags combination of several flags to modify the function actions.
 * \param handle will be set to a value that may be used as the offset
 * parameter for mmap().
 * 
 * \return zero on success or a negative value on error.
 *
 * \par Mapping the frame buffer
 * For the frame buffer
 * - \p offset will be the physical address of the start of the frame buffer,
 * - \p size will be the size of the frame buffer in bytes, and
 * - \p type will be DRM_FRAME_BUFFER.
 *
 * \par
 * The area mapped will be uncached. If MTRR support is available in the
 * kernel, the frame buffer area will be set to write combining. 
 *
 * \par Mapping the MMIO register area
 * For the MMIO register area,
 * - \p offset will be the physical address of the start of the register area,
 * - \p size will be the size of the register area bytes, and
 * - \p type will be DRM_REGISTERS.
 * \par
 * The area mapped will be uncached. 
 * 
 * \par Mapping the SAREA
 * For the SAREA,
 * - \p offset will be ignored and should be set to zero,
 * - \p size will be the desired size of the SAREA in bytes,
 * - \p type will be DRM_SHM.
 * 
 * \par
 * A shared memory area of the requested size will be created and locked in
 * kernel memory. This area may be mapped into client-space by using the handle
 * returned. 
 * 
 * \note May only be called by root.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_MAP ioctl, passing
 * the arguments in a drm_map structure.
 */
int drmAddMap(int fd,
	      drmHandle offset,
	      drmSize size,
	      drmMapType type,
	      drmMapFlags flags,
	      drmHandlePtr handle)
{
    drm_map_t map;

    map.offset  = offset;
    map.size    = size;
    map.handle  = 0;
    map.type    = type;
    map.flags   = flags;
    if (ioctl(fd, DRM_IOCTL_ADD_MAP, &map)) return -errno;
    if (handle) *handle = (drmHandle)map.handle;
    return 0;
}


/**
 * \brief Make buffers available for DMA transfers.
 * 
 * \param fd file descriptor.
 * \param count number of buffers.
 * \param size size of each buffer.
 * \param flags buffer allocation flags.
 * \param agp_offset offset in the AGP aperture 
 *
 * \return number of buffers allocated, negative on error.
 *
 * \internal
 * This function is a wrapper around DRM_IOCTL_ADD_BUFS ioctl.
 *
 * \sa drm_buf_desc.
 */
int drmAddBufs(int fd, int count, int size, drmBufDescFlags flags,
	       int agp_offset)
{
    drm_buf_desc_t request;

    request.count     = count;
    request.size      = size;
    request.low_mark  = 0;
    request.high_mark = 0;
    request.flags     = flags;
    request.agp_start = agp_offset;

    if (ioctl(fd, DRM_IOCTL_ADD_BUFS, &request)) return -errno;
    return request.count;
}


/**
 * \brief Free buffers.
 *
 * \param fd file descriptor.
 * \param count number of buffers to free.
 * \param list list of buffers to be freed.
 *
 * \return zero on success, or a negative value on failure.
 * 
 * \note This function is primarily used for debugging.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_FREE_BUFS ioctl, passing
 * the arguments in a drm_buf_free structure.
 */
int drmFreeBufs(int fd, int count, int *list)
{
    drm_buf_free_t request;

    request.count = count;
    request.list  = list;
    if (ioctl(fd, DRM_IOCTL_FREE_BUFS, &request)) return -errno;
    return 0;
}


/**
 * \brief Close the device.
 *
 * \param fd file descriptor.
 *
 * \internal
 * This function closes the file descriptor.
 */
int drmClose(int fd)
{
    drmMsg("close %d\n", fd);
    return close(fd);
}


/**
 * \brief Map a region of memory.
 *
 * \param fd file descriptor.
 * \param handle handle returned by drmAddMap().
 * \param size size in bytes. Must match the size used by drmAddMap().
 * \param address will contain the user-space virtual address where the mapping
 * begins.
 *
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper for mmap().
 */
int drmMap(int fd,
	   drmHandle handle,
	   drmSize size,
	   drmAddressPtr address)
{
    static unsigned long pagesize_mask = 0;

    if (fd < 0) return -EINVAL;

    if (!pagesize_mask)
	pagesize_mask = getpagesize() - 1;

    size = (size + pagesize_mask) & ~pagesize_mask;

    *address = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, handle);
    if (*address == MAP_FAILED) return -errno;
    return 0;
}


/**
 * \brief Unmap mappings obtained with drmMap().
 *
 * \param address address as given by drmMap().
 * \param size size in bytes. Must match the size used by drmMap().
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper for unmap().
 */
int drmUnmap(drmAddress address, drmSize size)
{
    return munmap(address, size);
}


/**
 * \brief Map all DMA buffers into client-virtual space.
 *
 * \param fd file descriptor.
 *
 * \return a pointer to a ::drmBufMap structure.
 *
 * \note The client may not use these buffers until obtaining buffer indices
 * with drmDMA().
 * 
 * \internal
 * This function calls the DRM_IOCTL_MAP_BUFS ioctl and copies the returned
 * information about the buffers in a drm_buf_map structure into the
 * client-visible data structures.
 */ 
drmBufMapPtr drmMapBufs(int fd)
{
    drm_buf_map_t bufs;
    drmBufMapPtr  retval;
    int           i;

    bufs.count = 0;
    bufs.list  = NULL;
    if (ioctl(fd, DRM_IOCTL_MAP_BUFS, &bufs)) return NULL;

    if (bufs.count) {
	if (!(bufs.list = malloc(bufs.count * sizeof(*bufs.list))))
	    return NULL;

	if (ioctl(fd, DRM_IOCTL_MAP_BUFS, &bufs)) {
	    free(bufs.list);
	    return NULL;
	}
				/* Now, copy it all back into the
                                   client-visible data structures... */
	retval = malloc(sizeof(*retval));
	retval->count = bufs.count;
	retval->list  = malloc(bufs.count * sizeof(*retval->list));
	for (i = 0; i < bufs.count; i++) {
	    retval->list[i].idx     = bufs.list[i].idx;
	    retval->list[i].total   = bufs.list[i].total;
	    retval->list[i].used    = 0;
	    retval->list[i].address = bufs.list[i].address;
	}
	return retval;
    }
    return NULL;
}


/**
 * \brief Unmap buffers allocated with drmMapBufs().
 *
 * \return zero on success, or negative value on failure.
 *
 * \internal
 * Calls munmap() for every buffer stored in \p bufs.
 */
int drmUnmapBufs(drmBufMapPtr bufs)
{
    int i;

    for (i = 0; i < bufs->count; i++) {
	munmap(bufs->list[i].address, bufs->list[i].total);
    }
    return 0;
}


#define DRM_DMA_RETRY		16

/**
 * \brief Reserve DMA buffers.
 *
 * \param fd file descriptor.
 * \param request 
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * Assemble the arguments into a drm_dma structure and keeps issuing the
 * DRM_IOCTL_DMA ioctl until success or until maximum number of retries.
 */
int drmDMA(int fd, drmDMAReqPtr request)
{
    drm_dma_t dma;
    int ret, i = 0;

				/* Copy to hidden structure */
    dma.context         = request->context;
    dma.send_count      = request->send_count;
    dma.send_indices    = request->send_list;
    dma.send_sizes      = request->send_sizes;
    dma.flags           = request->flags;
    dma.request_count   = request->request_count;
    dma.request_size    = request->request_size;
    dma.request_indices = request->request_list;
    dma.request_sizes   = request->request_sizes;

    do {
	ret = ioctl( fd, DRM_IOCTL_DMA, &dma );
    } while ( ret && errno == EAGAIN && i++ < DRM_DMA_RETRY );

    if ( ret == 0 ) {
	request->granted_count = dma.granted_count;
	return 0;
    } else {
	return -errno;
    }
}


/**
 * \brief Obtain heavyweight hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 * \param flags flags that determine the sate of the hardware when the function
 * returns.
 * 
 * \return always zero.
 * 
 * \internal
 * This function translates the arguments into a drm_lock structure and issue
 * the DRM_IOCTL_LOCK ioctl until the lock is successfully acquired.
 */
int drmGetLock(int fd, drmContext context, drmLockFlags flags)
{
    drm_lock_t lock;

    lock.context = context;
    lock.flags   = 0;
    if (flags & DRM_LOCK_READY)      lock.flags |= _DRM_LOCK_READY;
    if (flags & DRM_LOCK_QUIESCENT)  lock.flags |= _DRM_LOCK_QUIESCENT;
    if (flags & DRM_LOCK_FLUSH)      lock.flags |= _DRM_LOCK_FLUSH;
    if (flags & DRM_LOCK_FLUSH_ALL)  lock.flags |= _DRM_LOCK_FLUSH_ALL;
    if (flags & DRM_HALT_ALL_QUEUES) lock.flags |= _DRM_HALT_ALL_QUEUES;
    if (flags & DRM_HALT_CUR_QUEUES) lock.flags |= _DRM_HALT_CUR_QUEUES;

    while (ioctl(fd, DRM_IOCTL_LOCK, &lock))
	;
    return 0;
}

static void (*drm_unlock_callback)( void ) = 0;

/**
 * \brief Release the hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_UNLOCK ioctl, passing the
 * argument in a drm_lock structure.
 */
int drmUnlock(int fd, drmContext context)
{
    drm_lock_t lock;
    int ret;

    lock.context = context;
    lock.flags   = 0;
    ret = ioctl(fd, DRM_IOCTL_UNLOCK, &lock);

    /* Need this to synchronize vt releasing.  Could also teach fbdev
     * about the drm lock...
     */
    if (drm_unlock_callback) {
       drm_unlock_callback();
    }

    return ret;
}


/**
 * \brief Create context.
 *
 * Used by the X server during GLXContext initialization. This causes
 * per-context kernel-level resources to be allocated.
 *
 * \param fd file descriptor.
 * \param handle is set on success. To be used by the client when requesting DMA
 * dispatch with drmDMA().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \note May only be called by root.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmCreateContext(int fd, drmContextPtr handle)
{
    drm_ctx_t ctx;

    ctx.flags = 0;	/* Modified with functions below */
    if (ioctl(fd, DRM_IOCTL_ADD_CTX, &ctx)) return -errno;
    *handle = ctx.handle;
    return 0;
}


/**
 * \brief Destroy context.
 *
 * Free any kernel-level resources allocated with drmCreateContext() associated
 * with the context.
 * 
 * \param fd file descriptor.
 * \param handle handle given by drmCreateContext().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \note May only be called by root.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_RM_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmDestroyContext(int fd, drmContext handle)
{
    drm_ctx_t ctx;
    ctx.handle = handle;
    if (ioctl(fd, DRM_IOCTL_RM_CTX, &ctx)) return -errno;
    return 0;
}


/**
 * \brief Acquire the AGP device.
 *
 * Must be called before any of the other AGP related calls.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ACQUIRE ioctl.
 */
int drmAgpAcquire(int fd)
{
    if (ioctl(fd, DRM_IOCTL_AGP_ACQUIRE, NULL)) return -errno;
    return 0;
}


/**
 * \brief Release the AGP device.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_RELEASE ioctl.
 */
int drmAgpRelease(int fd)
{
    if (ioctl(fd, DRM_IOCTL_AGP_RELEASE, NULL)) return -errno;
    return 0;
}


/**
 * \brief Set the AGP mode.
 *
 * \param fd file descriptor.
 * \param mode AGP mode.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ENABLE ioctl, passing the
 * argument in a drm_agp_mode structure.
 */
int drmAgpEnable(int fd, unsigned long mode)
{
    drm_agp_mode_t m;

    m.mode = mode;
    if (ioctl(fd, DRM_IOCTL_AGP_ENABLE, &m)) return -errno;
    return 0;
}


/**
 * \brief Allocate a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param size requested memory size in bytes. Will be rounded to page boundary.
 * \param type type of memory to allocate.
 * \param address if not zero, will be set to the physical address of the
 * allocated memory.
 * \param handle on success will be set to a handle of the allocated memory.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ALLOC ioctl, passing the
 * arguments in a drm_agp_buffer structure.
 */
int drmAgpAlloc(int fd, unsigned long size, unsigned long type,
		unsigned long *address, unsigned long *handle)
{
    drm_agp_buffer_t b;
    *handle = 0;
    b.size   = size;
    b.handle = 0;
    b.type   = type;
    if (ioctl(fd, DRM_IOCTL_AGP_ALLOC, &b)) return -errno;
    if (address != 0UL) *address = b.physical;
    *handle = b.handle;
    return 0;
}


/**
 * \brief Free a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_FREE ioctl, passing the
 * argument in a drm_agp_buffer structure.
 */
int drmAgpFree(int fd, unsigned long handle)
{
    drm_agp_buffer_t b;

    b.size   = 0;
    b.handle = handle;
    if (ioctl(fd, DRM_IOCTL_AGP_FREE, &b)) return -errno;
    return 0;
}


/**
 * \brief Bind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * \param offset offset in bytes. It will round to page boundary.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_BIND ioctl, passing the
 * argument in a drm_agp_binding structure.
 */
int drmAgpBind(int fd, unsigned long handle, unsigned long offset)
{
    drm_agp_binding_t b;

    b.handle = handle;
    b.offset = offset;
    if (ioctl(fd, DRM_IOCTL_AGP_BIND, &b)) return -errno;
    return 0;
}


/**
 * \brief Unbind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_UNBIND ioctl, passing
 * the argument in a drm_agp_binding structure.
 */
int drmAgpUnbind(int fd, unsigned long handle)
{
    drm_agp_binding_t b;

    b.handle = handle;
    b.offset = 0;
    if (ioctl(fd, DRM_IOCTL_AGP_UNBIND, &b)) return -errno;
    return 0;
}


/**
 * \brief Get AGP driver major version number.
 *
 * \param fd file descriptor.
 * 
 * \return major version number on success, or a negative value on failure..
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMajor(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return -errno;
    return i.agp_version_major;
}


/**
 * \brief Get AGP driver minor version number.
 *
 * \param fd file descriptor.
 * 
 * \return minor version number on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMinor(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return -errno;
    return i.agp_version_minor;
}


/**
 * \brief Get AGP mode.
 *
 * \param fd file descriptor.
 * 
 * \return mode on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpGetMode(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.mode;
}


/**
 * \brief Get AGP aperture base.
 *
 * \param fd file descriptor.
 * 
 * \return aperture base on success, zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpBase(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.aperture_base;
}


/**
 * \brief Get AGP aperture size.
 *
 * \param fd file descriptor.
 * 
 * \return aperture size on success, zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpSize(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.aperture_size;
}


/**
 * \brief Get used AGP memory.
 *
 * \param fd file descriptor.
 * 
 * \return memory used on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryUsed(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.memory_used;
}


/**
 * \brief Get available AGP memory.
 *
 * \param fd file descriptor.
 * 
 * \return memory available on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryAvail(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.memory_allowed;
}


/**
 * \brief Get hardware vendor ID.
 *
 * \param fd file descriptor.
 * 
 * \return vendor ID on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpVendorId(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.id_vendor;
}


/**
 * \brief Get hardware device ID.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpDeviceId(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.id_device;
}

int drmScatterGatherAlloc(int fd, unsigned long size, unsigned long *handle)
{
    drm_scatter_gather_t sg;

    *handle = 0;
    sg.size   = size;
    sg.handle = 0;
    if (ioctl(fd, DRM_IOCTL_SG_ALLOC, &sg)) return -errno;
    *handle = sg.handle;
    return 0;
}

int drmScatterGatherFree(int fd, unsigned long handle)
{
    drm_scatter_gather_t sg;

    sg.size   = 0;
    sg.handle = handle;
    if (ioctl(fd, DRM_IOCTL_SG_FREE, &sg)) return -errno;
    return 0;
}

/**
 * \brief Wait for VBLANK.
 *
 * \param fd file descriptor.
 * \param vbl pointer to a drmVBlank structure.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_WAIT_VBLANK ioctl.
 */
int drmWaitVBlank(int fd, drmVBlankPtr vbl)
{
    int ret;

    do {
       ret = ioctl(fd, DRM_IOCTL_WAIT_VBLANK, vbl);
    } while (ret && errno == EINTR);

    return ret;
}


/**
 * \brief Install IRQ handler.
 *
 * \param fd file descriptor.
 * \param irq IRQ number.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlInstHandler(int fd, int irq)
{
    drm_control_t ctl;

    ctl.func  = DRM_INST_HANDLER;
    ctl.irq   = irq;
    if (ioctl(fd, DRM_IOCTL_CONTROL, &ctl)) return -errno;
    return 0;
}


/**
 * \brief Uninstall IRQ handler.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlUninstHandler(int fd)
{
    drm_control_t ctl;

    ctl.func  = DRM_UNINST_HANDLER;
    ctl.irq   = 0;
    if (ioctl(fd, DRM_IOCTL_CONTROL, &ctl)) return -errno;
    return 0;
}


/**
 * \brief Get IRQ from bus ID.
 *
 * \param fd file descriptor.
 * \param busnum bus number.
 * \param devnum device number.
 * \param funcnum function number.
 * 
 * \return IRQ number on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_IRQ_BUSID ioctl, passing the
 * arguments in a drm_irq_busid structure.
 */
int drmGetInterruptFromBusID(int fd, int busnum, int devnum, int funcnum)
{
    drm_irq_busid_t p;

    p.busnum  = busnum;
    p.devnum  = devnum;
    p.funcnum = funcnum;
    if (ioctl(fd, DRM_IOCTL_IRQ_BUSID, &p)) return -errno;
    return p.irq;
}


/**
 * \brief Send a device-specific command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    void *data = NULL; /* dummy */
    unsigned long request;

    request = DRM_IO( DRM_COMMAND_BASE + drmCommandIndex);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * \brief Send a device-specific read command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data destination pointer of the data to be read.
 * \param size size of the data to be read.
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a read ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandRead(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * \brief Send a device-specific write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data source pointer of the data to be written.
 * \param size size of the data to be written.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a write ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWrite(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_WRITE, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * \brief Send a device-specific read-write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data source pointer of the data to be read and written.
 * \param size size of the data to be read and written.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a read-write ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ|DRM_IOC_WRITE, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}

