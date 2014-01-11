#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "freedreno_drm_public.h"

#include "freedreno/freedreno_screen.h"

struct pipe_screen *
fd_drm_screen_create(int fd)
{
	struct fd_device *dev = fd_device_new_dup(fd);
	if (!dev)
		return NULL;
	return fd_screen_create(dev);
}
