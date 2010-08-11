/* Direct3D 10/11 has no concept of transfers. Applications instead
 * create resources with a STAGING or DYNAMIC usage, copy between them
 * and the real resource and use Map to map the STAGING/DYNAMIC resource.
 *
 * This util module allows to implement Gallium drivers as a Direct3D
 * driver would be implemented: transfers allocate a resource with
 * PIPE_USAGE_STAGING, and copy the data between it and the real resource
 * with resource_copy_region.
 */

#ifndef U_STAGING_H
#define U_STAGING_H

#include "pipe/p_state.h"

struct util_staging_transfer {
   struct pipe_transfer base;

   /* if direct, same as base.resource, otherwise the temporary staging resource */
   struct pipe_resource *staging_resource;
};

/* user must be stride, slice_stride and offset */
/* pt->usage == PIPE_USAGE_DYNAMIC should be a good value to pass for direct */
/* staging resource is currently created with PIPE_USAGE_DYNAMIC */
struct util_staging_transfer *
util_staging_transfer_new(struct pipe_context *pipe,
           struct pipe_resource *pt,
           struct pipe_subresource sr,
           unsigned usage,
           const struct pipe_box *box,
           bool direct);

void
util_staging_transfer_destroy(struct pipe_context *pipe, struct pipe_transfer *ptx);

#endif
