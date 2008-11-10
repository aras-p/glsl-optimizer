#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "main/mtypes.h"

#include "intel_context.h"
#include "intel_bufmgr.h"
#include "intel_reg.h"

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
    *
    * This will be upgraded to NO_LOOP_CLIPRECTS when there's a single
    * constant cliprect, as in DRI2 or FBO rendering.
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
    *
    * Equivalent behavior to NO_LOOP_CLIPRECTS, but may not persist in batch
    * outside of LOCK/UNLOCK.  This is upgraded to just NO_LOOP_CLIPRECTS when
    * there's a constant cliprect, as in DRI2 or FBO rendering.
    */
   REFERENCES_CLIPRECTS
};

struct intel_batchbuffer
{
   struct intel_context *intel;

   dri_bo *buf;

   GLubyte *buffer;

   GLubyte *map;
   GLubyte *ptr;

   enum cliprect_mode cliprect_mode;

   GLuint size;

   GLuint dirty_state;
};

struct intel_batchbuffer *intel_batchbuffer_alloc(struct intel_context
                                                  *intel);

void intel_batchbuffer_free(struct intel_batchbuffer *batch);


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
				       uint32_t read_domains,
				       uint32_t write_domain,
				       uint32_t offset);

/* Inline functions - might actually be better off with these
 * non-inlined.  Certainly better off switching all command packets to
 * be passed as structs rather than dwords, but that's a little bit of
 * work...
 */
static INLINE GLint
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

   if ((cliprect_mode == LOOP_CLIPRECTS ||
	cliprect_mode == REFERENCES_CLIPRECTS) &&
       batch->intel->constant_cliprect)
      cliprect_mode = NO_LOOP_CLIPRECTS;

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

#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   assert((delta) >= 0);						\
   intel_batchbuffer_emit_reloc(intel->batch, buf,			\
				read_domains, write_domain, delta);	\
} while (0)

#define ADVANCE_BATCH() do { } while(0)


static INLINE void
intel_batchbuffer_emit_mi_flush(struct intel_batchbuffer *batch)
{
   intel_batchbuffer_require_space(batch, 4, IGNORE_CLIPRECTS);
   intel_batchbuffer_emit_dword(batch, MI_FLUSH);
}

#endif
