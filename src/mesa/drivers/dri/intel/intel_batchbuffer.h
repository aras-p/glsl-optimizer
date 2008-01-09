#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "mtypes.h"

#include "dri_bufmgr.h"

struct intel_context;

#define BATCH_SZ 16384
#define BATCH_RESERVED 16

enum cliprects_enable {
   INTEL_BATCH_CLIPRECTS = 0,
   INTEL_BATCH_NO_CLIPRECTS = 1
};

struct intel_batchbuffer
{
   struct intel_context *intel;

   dri_bo *buf;
   dri_fence *last_fence;

   GLubyte *map;
   GLubyte *ptr;

   enum cliprects_enable cliprects_enable;

   GLuint size;

   GLuint dirty_state;
   GLuint id;
};

struct intel_batchbuffer *intel_batchbuffer_alloc(struct intel_context
                                                  *intel);

void intel_batchbuffer_free(struct intel_batchbuffer *batch);


void intel_batchbuffer_finish(struct intel_batchbuffer *batch);

void intel_batchbuffer_flush(struct intel_batchbuffer *batch);

void intel_batchbuffer_reset(struct intel_batchbuffer *batch);


/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
void intel_batchbuffer_data(struct intel_batchbuffer *batch,
                            const void *data, GLuint bytes,
			    enum cliprects_enable cliprects_enable);

void intel_batchbuffer_release_space(struct intel_batchbuffer *batch,
                                     GLuint bytes);

GLboolean intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                                       dri_bo *buffer,
                                       GLuint flags, GLuint offset);

/* Inline functions - might actually be better off with these
 * non-inlined.  Certainly better off switching all command packets to
 * be passed as structs rather than dwords, but that's a little bit of
 * work...
 */
static INLINE GLuint
intel_batchbuffer_space(struct intel_batchbuffer *batch)
{
   return (batch->size - BATCH_RESERVED) - (batch->ptr - batch->map);
}


static INLINE void
intel_batchbuffer_emit_dword(struct intel_batchbuffer *batch, GLuint dword)
{
   assert(batch->map);
   assert(intel_batchbuffer_space(batch) >= 4);
   *(GLuint *) (batch->ptr) = dword;
   batch->ptr += 4;
}

static INLINE void
intel_batchbuffer_require_space(struct intel_batchbuffer *batch,
                                GLuint sz,
				enum cliprects_enable cliprects_enable)
{
   assert(sz < batch->size - 8);
   if (intel_batchbuffer_space(batch) < sz)
      intel_batchbuffer_flush(batch);

   /* Upgrade the buffer to being looped over per cliprect if this batch
    * emit needs it.  The code used to emit a batch whenever the
    * cliprects_enable was changed, but reducing the overhead of frequent
    * batch flushing is more important than reducing state parsing,
    * particularly as we move towards private backbuffers and number
    * cliprects always being 1 except at swap.
    */
   if (cliprects_enable == INTEL_BATCH_CLIPRECTS)
      batch->cliprects_enable = INTEL_BATCH_CLIPRECTS;
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n, cliprects_enable) do {				\
   intel_batchbuffer_require_space(intel->batch, (n)*4, cliprects_enable); \
} while (0)

#define OUT_BATCH(d)  intel_batchbuffer_emit_dword(intel->batch, d)

#define OUT_RELOC(buf, cliprects_enable, delta) do { 			\
   assert((delta) >= 0);						\
   intel_batchbuffer_emit_reloc(intel->batch, buf, cliprects_enable, delta); \
} while (0)

#define ADVANCE_BATCH() do { } while(0)


#endif
