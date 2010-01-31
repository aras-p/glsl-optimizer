#ifndef BRW_BATCHBUFFER_H
#define BRW_BATCHBUFFER_H

#include "util/u_debug.h"

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




struct brw_batchbuffer {

   struct brw_winsys_screen *sws;
   struct brw_winsys_buffer *buf;
   struct brw_chipset chipset;

   /**
    * Values exported to speed up the writing the batchbuffer,
    * instead of having to go trough a accesor function for
    * each dword written.
    */
   /*{@*/
   uint8_t *map;
   uint8_t *ptr;
   size_t size;
   struct {
      uint8_t *end_ptr;
   } emit;


   size_t relocs;
   size_t max_relocs;
   /*@}*/
};

struct brw_batchbuffer *brw_batchbuffer_alloc( struct brw_winsys_screen *sws,
                                               struct brw_chipset chipset );

void brw_batchbuffer_free(struct brw_batchbuffer *batch);

void _brw_batchbuffer_flush(struct brw_batchbuffer *batch,
			      const char *file, int line);


enum pipe_error
brw_batchbuffer_reset(struct brw_batchbuffer *batch);


/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
enum pipe_error brw_batchbuffer_data(struct brw_batchbuffer *batch,
                            const void *data, GLuint bytes,
			    enum cliprect_mode cliprect_mode);


enum pipe_error brw_batchbuffer_emit_reloc(struct brw_batchbuffer *batch,
			       struct brw_winsys_buffer *buffer,
			       enum brw_buffer_usage usage,
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

static INLINE enum pipe_error
brw_batchbuffer_require_space(struct brw_batchbuffer *batch,
                                GLuint sz)
{
   assert(sz < batch->size - 8);
   if (brw_batchbuffer_space(batch) < sz) {
      assert(0);
      return PIPE_ERROR_OUT_OF_MEMORY;
   }
#ifdef DEBUG
   batch->emit.end_ptr = batch->ptr + sz;
#endif
   return 0;
}

/* Here are the crusty old macros, to be removed:
 */
#define BEGIN_BATCH(n, cliprect_mode) do {				\
      brw_batchbuffer_require_space(brw->batch, (n)*4);			\
   } while (0)

#define OUT_BATCH(d) brw_batchbuffer_emit_dword(brw->batch, d)

#define OUT_RELOC(buf, usage, delta) do {				\
      assert((unsigned) (delta) < buf->size);				\
      brw_batchbuffer_emit_reloc(brw->batch, buf,			\
				 usage, delta);				\
   } while (0)

#ifdef DEBUG
#define ADVANCE_BATCH() do {						\
      unsigned int _n = brw->batch->ptr - brw->batch->emit.end_ptr;	\
      if (_n != 0) {							\
	 debug_printf("%s: %d too many bytes emitted to batch\n",	\
		      __FUNCTION__, _n);				\
	 abort();							\
      }									\
      brw->batch->emit.end_ptr = NULL;					\
   } while(0)
#else
#define ADVANCE_BATCH()
#endif

static INLINE void
brw_batchbuffer_emit_mi_flush(struct brw_batchbuffer *batch)
{
   brw_batchbuffer_require_space(batch, 4);
   brw_batchbuffer_emit_dword(batch, MI_FLUSH);
}

#endif
