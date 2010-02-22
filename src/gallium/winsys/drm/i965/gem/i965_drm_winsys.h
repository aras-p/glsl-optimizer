
#ifndef INTEL_DRM_WINSYS_H
#define INTEL_DRM_WINSYS_H

#include "i965/brw_winsys.h"

#include "drm.h"
#include "intel_bufmgr.h"



/*
 * Winsys
 */


struct i965_libdrm_winsys
{
   struct brw_winsys_screen base;
   drm_intel_bufmgr *gem;

   boolean send_cmd;

   int fd; /**< Drm file discriptor */

   unsigned id;
};

static INLINE struct i965_libdrm_winsys *
i965_libdrm_winsys(struct brw_winsys_screen *iws)
{
   return (struct i965_libdrm_winsys *)iws;
}

struct i965_libdrm_winsys *i965_libdrm_winsys_create(int fd, unsigned pci_id);

void i965_libdrm_winsys_init_buffer_functions(struct i965_libdrm_winsys *idws);


/* Buffer.  
 */
struct i965_libdrm_buffer {
   struct brw_winsys_buffer base;

   drm_intel_bo *bo;

   void *ptr;
   unsigned map_count;
   unsigned data_type;		/* valid while mapped */
   unsigned tiling;

   boolean map_gtt;
   boolean flinked;
   unsigned flink;
};

static INLINE struct i965_libdrm_buffer *
i965_libdrm_buffer(struct brw_winsys_buffer *buffer)
{
   return (struct i965_libdrm_buffer *)buffer;
}


#endif
