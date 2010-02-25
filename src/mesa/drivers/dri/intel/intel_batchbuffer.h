#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "main/mtypes.h"

#include "intel_context.h"
#include "intel_bufmgr.h"
#include "intel_reg.h"

#define BATCH_SZ 16384
#define BATCH_RESERVED 16


struct intel_batchbuffer
{
   struct intel_context *intel;

   dri_bo *buf;

   GLubyte *buffer;

   GLubyte *map;
   GLubyte *ptr;

   GLuint size;

   /** Tracking of BEGIN_BATCH()/OUT_BATCH()/ADVANCE_BATCH() debugging */
   struct {
      GLuint total;
      GLubyte *start_ptr;
   } emit;

   GLuint dirty_state;
   GLuint reserved_space;
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
                            const void *data, GLuint bytes);

void intel_batchbuffer_release_space(struct intel_batchbuffer *batch,
                                     GLuint bytes);

GLboolean intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                                       dri_bo *buffer,
				       uint32_t read_domains,
				       uint32_t write_domain,
				       uint32_t offset);
void intel_batchbuffer_emit_mi_flush(struct intel_batchbuffer *batch);

/* Inline functions - might actually be better off with these
 * non-inlined.  Certainly better off switching all command packets to
 * be passed as structs rather than dwords, but that's a little bit of
 * work...
 */
static INLINE GLint
intel_batchbuffer_space(struct intel_batchbuffer *batch)
{
   return (batch->size - batch->reserved_space) - (batch->ptr - batch->map);
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
                                GLuint sz)
{
   assert(sz < batch->size - 8);
   if (intel_batchbuffer_space(batch) < sz)
      intel_batchbuffer_flush(batch);
}

static INLINE uint32_t float_as_int(float f)
{
   union {
      float f;
      uint32_t d;
   } fi;

   fi.f = f;
   return fi.d;
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n) do {				\
   intel_batchbuffer_require_space(intel->batch, (n)*4); \
   assert(intel->batch->emit.start_ptr == NULL);			\
   intel->batch->emit.total = (n) * 4;					\
   intel->batch->emit.start_ptr = intel->batch->ptr;			\
} while (0)

#define OUT_BATCH(d) intel_batchbuffer_emit_dword(intel->batch, d)
#define OUT_BATCH_F(f) intel_batchbuffer_emit_dword(intel->batch,	\
						    float_as_int(f))

#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   assert((unsigned) (delta) < buf->size);				\
   intel_batchbuffer_emit_reloc(intel->batch, buf,			\
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

#endif
