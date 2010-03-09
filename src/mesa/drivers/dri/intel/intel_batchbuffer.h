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

#ifdef DEBUG
   /** Tracking of BEGIN_BATCH()/OUT_BATCH()/ADVANCE_BATCH() debugging */
   struct {
      GLuint total;
      GLubyte *start_ptr;
   } emit;
#endif

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
GLboolean intel_batchbuffer_emit_reloc_fenced(struct intel_batchbuffer *batch,
					      drm_intel_bo *buffer,
					      uint32_t read_domains,
					      uint32_t write_domain,
					      uint32_t offset);
void intel_batchbuffer_emit_mi_flush(struct intel_batchbuffer *batch);

static INLINE uint32_t float_as_int(float f)
{
   union {
      float f;
      uint32_t d;
   } fi;

   fi.f = f;
   return fi.d;
}

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
#ifdef DEBUG
   assert(intel_batchbuffer_space(batch) >= 4);
#endif
   *(GLuint *) (batch->ptr) = dword;
   batch->ptr += 4;
}

static INLINE void
intel_batchbuffer_emit_float(struct intel_batchbuffer *batch, float f)
{
   intel_batchbuffer_emit_dword(batch, float_as_int(f));
}

static INLINE void
intel_batchbuffer_require_space(struct intel_batchbuffer *batch,
                                GLuint sz)
{
#ifdef DEBUG
   assert(sz < batch->size - 8);
#endif
   if (intel_batchbuffer_space(batch) < sz)
      intel_batchbuffer_flush(batch);
}

static INLINE void
intel_batchbuffer_begin(struct intel_batchbuffer *batch, int n)
{
   intel_batchbuffer_require_space(batch, n * 4);
#ifdef DEBUG
   assert(batch->map);
   assert(batch->emit.start_ptr == NULL);
   batch->emit.total = n * 4;
   batch->emit.start_ptr = batch->ptr;
#endif
}

static INLINE void
intel_batchbuffer_advance(struct intel_batchbuffer *batch)
{
#ifdef DEBUG
   unsigned int _n = batch->ptr - batch->emit.start_ptr;
   assert(batch->emit.start_ptr != NULL);
   if (_n != batch->emit.total) {
      fprintf(stderr, "ADVANCE_BATCH: %d of %d dwords emitted\n",
	      _n, batch->emit.total);
      abort();
   }
   batch->emit.start_ptr = NULL;
#endif
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n) intel_batchbuffer_begin(intel->batch, n)
#define OUT_BATCH(d) intel_batchbuffer_emit_dword(intel->batch, d)
#define OUT_BATCH_F(f) intel_batchbuffer_emit_float(intel->batch,f)
#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   intel_batchbuffer_emit_reloc(intel->batch, buf,			\
				read_domains, write_domain, delta);	\
} while (0)
#define OUT_RELOC_FENCED(buf, read_domains, write_domain, delta) do {	\
   intel_batchbuffer_emit_reloc_fenced(intel->batch, buf,		\
				       read_domains, write_domain, delta); \
} while (0)

#define ADVANCE_BATCH() intel_batchbuffer_advance(intel->batch);

#endif
