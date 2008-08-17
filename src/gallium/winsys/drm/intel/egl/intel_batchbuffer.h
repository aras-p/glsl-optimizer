#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "intel_be_batchbuffer.h"

/*
 * Need to redefine the BATCH defines
 */

#undef BEGIN_BATCH
#define BEGIN_BATCH(dwords, relocs) \
   (i915_batchbuffer_check(&intel->base.batch->base, dwords, relocs))

#undef OUT_BATCH
#define OUT_BATCH(d) \
   i915_batchbuffer_dword(&intel->base.batch->base, d)

#undef OUT_RELOC
#define OUT_RELOC(buf,flags,mask,delta) do { 					\
   assert((delta) >= 0);							\
   intel_be_offset_relocation(intel->base.batch, delta, buf, flags, mask); 	\
} while (0)

#endif
