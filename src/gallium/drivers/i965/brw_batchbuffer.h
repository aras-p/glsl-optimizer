#ifndef BRW_BATCHBUFFER_H
#define BRW_BATCHBUFFER_H

#include "brw_types.h"
#include "brw_winsys.h"
#include "brw_reg.h"

#define BATCH_SZ 16384
#define BATCH_RESERVED 16

/* All ignored:
 */
enum cliprect_mode {
   IGNORE_CLIPRECTS,
   LOOP_CLIPRECTS,
   NO_LOOP_CLIPRECTS,
   REFERENCES_CLIPRECTS
};

void brw_batchbuffer_free(struct brw_batchbuffer *batch);

void _brw_batchbuffer_flush(struct brw_batchbuffer *batch,
			      const char *file, int line);

#define brw_batchbuffer_flush(batch) \
	_brw_batchbuffer_flush(batch, __FILE__, __LINE__)

void brw_batchbuffer_reset(struct brw_batchbuffer *batch);


/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
void brw_batchbuffer_data(struct brw_batchbuffer *batch,
                            const void *data, GLuint bytes,
			    enum cliprect_mode cliprect_mode);

void brw_batchbuffer_release_space(struct brw_batchbuffer *batch,
                                     GLuint bytes);

GLboolean brw_batchbuffer_emit_reloc(struct brw_batchbuffer *batch,
                                       struct brw_winsys_buffer *buffer,
				       uint32_t read_domains,
				       uint32_t write_domain,
				       uint32_t offset);

/* Inline functions - might actually be better off with these
 * non-inlined.  Certainly better off switching all command packets to
 * be passed as structs rather than dwords, but that's a little bit of
 * work...
 */
static INLINE GLint
brw_batchbuffer_space(struct brw_batchbuffer *batch)
{
   return (batch->size - BATCH_RESERVED) - (batch->ptr - batch->map);
}


static INLINE void
brw_batchbuffer_emit_dword(struct brw_batchbuffer *batch, GLuint dword)
{
   assert(batch->map);
   assert(brw_batchbuffer_space(batch) >= 4);
   *(GLuint *) (batch->ptr) = dword;
   batch->ptr += 4;
}

static INLINE boolean
brw_batchbuffer_require_space(struct brw_batchbuffer *batch,
                                GLuint sz,
				enum cliprect_mode cliprect_mode)
{
   assert(sz < batch->size - 8);
   if (brw_batchbuffer_space(batch) < sz) {
      assert(0);
      return FALSE;
   }

   /* All commands should be executed once regardless of cliprect
    * mode.
    */
   (void)cliprect_mode;
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n, cliprect_mode) do {				\
   brw_batchbuffer_require_space(intel->batch, (n)*4, cliprect_mode); \
   assert(intel->batch->emit.start_ptr == NULL);			\
   intel->batch->emit.total = (n) * 4;					\
   intel->batch->emit.start_ptr = intel->batch->ptr;			\
} while (0)

#define OUT_BATCH(d) brw_batchbuffer_emit_dword(intel->batch, d)

#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   assert((unsigned) (delta) < buf->size);				\
   brw_batchbuffer_emit_reloc(intel->batch, buf,			\
				read_domains, write_domain, delta);	\
} while (0)

#define ADVANCE_BATCH() do {						\
   unsigned int _n = intel->batch->ptr - intel->batch->emit.start_ptr;	\
   assert(intel->batch->emit.start_ptr != NULL);			\
   if (_n != intel->batch->emit.total) {				\
      fprintf(stderr, "ADVANCE_BATCH: %d of %d dwords emitted\n",	\
	      _n, intel->batch->emit.total);				\
      abort();								\
   }									\
   intel->batch->emit.start_ptr = NULL;					\
} while(0)


static INLINE void
brw_batchbuffer_emit_mi_flush(struct brw_batchbuffer *batch)
{
   brw_batchbuffer_require_space(batch, 4, IGNORE_CLIPRECTS);
   brw_batchbuffer_emit_dword(batch, MI_FLUSH);
}

#endif
