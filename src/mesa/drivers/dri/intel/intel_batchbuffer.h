#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "mtypes.h"

#include "dri_bufmgr.h"

struct intel_context;

#define BATCH_SZ 16384
#define BATCH_RESERVED 16

enum cliprect_mode {
   /**
    * Batchbuffer contents may be looped over per cliprect, but do not
    * require it.
    */
   IGNORE_CLIPRECTS,
   /**
    * Batchbuffer contents require looping over per cliprect at batch submit
    * time.
    */
   LOOP_CLIPRECTS,
   /**
    * Batchbuffer contents contain drawing that should not be executed multiple
    * times.
    */
   NO_LOOP_CLIPRECTS,
   /**
    * Batchbuffer contents contain drawing that already handles cliprects, such
    * as 2D drawing to front/back/depth that doesn't respect DRAWING_RECTANGLE.
    * Equivalent behavior to NO_LOOP_CLIPRECTS, but may not persist in batch
    * outside of LOCK/UNLOCK.
    */
   REFERENCES_CLIPRECTS
};

struct intel_batchbuffer
{
   struct intel_context *intel;

   dri_bo *buf;
   dri_fence *last_fence;

   GLubyte *map;
   GLubyte *ptr;

   enum cliprect_mode cliprect_mode;

   GLuint size;

   GLuint dirty_state;
};

struct intel_batchbuffer *intel_batchbuffer_alloc(struct intel_context
                                                  *intel);

void intel_batchbuffer_free(struct intel_batchbuffer *batch);


void intel_batchbuffer_finish(struct intel_batchbuffer *batch);

void _intel_batchbuffer_flush(struct intel_batchbuffer *batch,
			      const char *file, int line);

#define intel_batchbuffer_flush(batch) \
	_intel_batchbuffer_flush(batch, __FILE__, __LINE__)

void intel_batchbuffer_reset(struct intel_batchbuffer *batch);


/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
void intel_batchbuffer_data(struct intel_batchbuffer *batch,
                            const void *data, GLuint bytes,
			    enum cliprect_mode cliprect_mode);

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
				enum cliprect_mode cliprect_mode)
{
   assert(sz < batch->size - 8);
   if (intel_batchbuffer_space(batch) < sz)
      intel_batchbuffer_flush(batch);

   if (cliprect_mode != IGNORE_CLIPRECTS) {
      if (batch->cliprect_mode == IGNORE_CLIPRECTS) {
	 batch->cliprect_mode = cliprect_mode;
      } else {
	 if (batch->cliprect_mode != cliprect_mode) {
	    intel_batchbuffer_flush(batch);
	    batch->cliprect_mode = cliprect_mode;
	 }
      }
   }
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n, cliprect_mode) do {				\
   intel_batchbuffer_require_space(intel->batch, (n)*4, cliprect_mode); \
} while (0)

#define OUT_BATCH(d)  intel_batchbuffer_emit_dword(intel->batch, d)

#define OUT_RELOC(buf, cliprect_mode, delta) do { 			\
   assert((delta) >= 0);						\
   intel_batchbuffer_emit_reloc(intel->batch, buf, cliprect_mode, delta); \
} while (0)

#define ADVANCE_BATCH() do { } while(0)


#endif
