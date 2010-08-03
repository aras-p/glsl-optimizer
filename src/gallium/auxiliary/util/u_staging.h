/* Implement transfers using staging resources like in DirectX 10/11 */

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
