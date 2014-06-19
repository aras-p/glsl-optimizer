#include <sys/stat.h>
#include <unistd.h>
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_hash_table.h"
#include "os/os_thread.h"

#include "nouveau_drm_public.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_screen.h"

static struct util_hash_table *fd_tab = NULL;

pipe_static_mutex(nouveau_screen_mutex);

boolean nouveau_drm_screen_unref(struct nouveau_screen *screen)
{
	int ret;
	if (screen->refcount == -1)
		return true;

	pipe_mutex_lock(nouveau_screen_mutex);
	ret = --screen->refcount;
	assert(ret >= 0);
	if (ret == 0)
		util_hash_table_remove(fd_tab, intptr_to_pointer(screen->device->fd));
	pipe_mutex_unlock(nouveau_screen_mutex);
	return ret == 0;
}

static unsigned hash_fd(void *key)
{
    int fd = pointer_to_intptr(key);
    struct stat stat;
    fstat(fd, &stat);

    return stat.st_dev ^ stat.st_ino ^ stat.st_rdev;
}

static int compare_fd(void *key1, void *key2)
{
    int fd1 = pointer_to_intptr(key1);
    int fd2 = pointer_to_intptr(key2);
    struct stat stat1, stat2;
    fstat(fd1, &stat1);
    fstat(fd2, &stat2);

    return stat1.st_dev != stat2.st_dev ||
           stat1.st_ino != stat2.st_ino ||
           stat1.st_rdev != stat2.st_rdev;
}

PUBLIC struct pipe_screen *
nouveau_drm_screen_create(int fd)
{
	struct nouveau_device *dev = NULL;
	struct pipe_screen *(*init)(struct nouveau_device *);
	struct nouveau_screen *screen;
	int ret, dupfd = -1;

	pipe_mutex_lock(nouveau_screen_mutex);
	if (!fd_tab) {
		fd_tab = util_hash_table_create(hash_fd, compare_fd);
		if (!fd_tab)
			goto err;
	}

	screen = util_hash_table_get(fd_tab, intptr_to_pointer(fd));
	if (screen) {
		screen->refcount++;
		pipe_mutex_unlock(nouveau_screen_mutex);
		return &screen->base;
	}

	/* Since the screen re-use is based on the device node and not the fd,
	 * create a copy of the fd to be owned by the device. Otherwise a
	 * scenario could occur where two screens are created, and the first
	 * one is shut down, along with the fd being closed. The second
	 * (identical) screen would now have a reference to the closed fd. We
	 * avoid this by duplicating the original fd. Note that
	 * nouveau_device_wrap does not close the fd in case of a device
	 * creation error.
	 */
	dupfd = dup(fd);
	ret = nouveau_device_wrap(dupfd, 1, &dev);
	if (ret)
		goto err;

	switch (dev->chipset & ~0xf) {
	case 0x30:
	case 0x40:
	case 0x60:
		init = nv30_screen_create;
		break;
	case 0x50:
	case 0x80:
	case 0x90:
	case 0xa0:
		init = nv50_screen_create;
		break;
	case 0xc0:
	case 0xd0:
	case 0xe0:
	case 0xf0:
	case 0x100:
	case 0x110:
		init = nvc0_screen_create;
		break;
	default:
		debug_printf("%s: unknown chipset nv%02x\n", __func__,
			     dev->chipset);
		goto err;
	}

	screen = (struct nouveau_screen*)init(dev);
	if (!screen)
		goto err;

	util_hash_table_set(fd_tab, intptr_to_pointer(fd), screen);
	screen->refcount = 1;
	pipe_mutex_unlock(nouveau_screen_mutex);
	return &screen->base;

err:
	if (dev)
		nouveau_device_del(&dev);
	else if (dupfd >= 0)
		close(dupfd);
	pipe_mutex_unlock(nouveau_screen_mutex);
	return NULL;
}
