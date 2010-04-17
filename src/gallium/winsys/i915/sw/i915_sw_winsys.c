
#include "i915_sw_winsys.h"
#include "util/u_memory.h"

#include "i915/i915_context.h"
#include "i915/i915_screen.h"


/*
 * Helper functions
 */


static void
i915_sw_get_device_id(unsigned int *device_id)
{
   /* just pick a i945 hw id */
   *device_id = 0x27A2;
}

static void
i915_sw_destroy(struct i915_winsys *iws)
{
   struct i915_sw_winsys *isws = i915_sw_winsys(iws);
   FREE(isws);
}


/*
 * Exported functions
 */


struct pipe_screen *
i915_sw_create_screen()
{
   struct i915_sw_winsys *isws;
   unsigned int deviceID;

   isws = CALLOC_STRUCT(i915_sw_winsys);
   if (!isws)
      return NULL;

   i915_sw_get_device_id(&deviceID);

   i915_sw_winsys_init_batchbuffer_functions(isws);
   i915_sw_winsys_init_buffer_functions(isws);
   i915_sw_winsys_init_fence_functions(isws);

   isws->base.destroy = i915_sw_destroy;

   isws->id = deviceID;
   isws->max_batch_size = 16 * 4096;

   isws->dump_cmd = debug_get_bool_option("INTEL_DUMP_CMD", FALSE);

   /* XXX so this will leak winsys:es */
   return i915_create_screen(&isws->base, deviceID);
}
