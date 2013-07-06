#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "main/mtypes.h"

#include "brw_context.h"
#include "intel_bufmgr.h"
#include "intel_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of bytes to reserve for commands necessary to complete a batch.
 *
 * This includes:
 * - MI_BATCHBUFFER_END (4 bytes)
 * - Optional MI_NOOP for ensuring the batch length is qword aligned (4 bytes)
 * - Any state emitted by vtbl->finish_batch():
 *   - Gen4-5 record ending occlusion query values (4 * 4 = 16 bytes)
 */
#define BATCH_RESERVED 24

struct intel_batchbuffer;

void intel_batchbuffer_init(struct brw_context *brw);
void intel_batchbuffer_free(struct brw_context *brw);
void intel_batchbuffer_save_state(struct brw_context *brw);
void intel_batchbuffer_reset_to_saved(struct brw_context *brw);

int _intel_batchbuffer_flush(struct brw_context *brw,
			     const char *file, int line);

#define intel_batchbuffer_flush(intel) \
	_intel_batchbuffer_flush(intel, __FILE__, __LINE__)



/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
void intel_batchbuffer_data(struct brw_context *brw,
                            const void *data, GLuint bytes, bool is_blit);

bool intel_batchbuffer_emit_reloc(struct brw_context *brw,
                                       drm_intel_bo *buffer,
				       uint32_t read_domains,
				       uint32_t write_domain,
				       uint32_t offset);
bool intel_batchbuffer_emit_reloc_fenced(struct brw_context *brw,
					      drm_intel_bo *buffer,
					      uint32_t read_domains,
					      uint32_t write_domain,
					      uint32_t offset);
void intel_batchbuffer_emit_mi_flush(struct brw_context *brw);
void intel_emit_post_sync_nonzero_flush(struct brw_context *brw);
void intel_emit_depth_stall_flushes(struct brw_context *brw);
void gen7_emit_vs_workaround_flush(struct brw_context *brw);

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
static INLINE unsigned
intel_batchbuffer_space(struct brw_context *brw)
{
   return (brw->batch.state_batch_offset - brw->batch.reserved_space)
      - brw->batch.used*4;
}


static INLINE void
intel_batchbuffer_emit_dword(struct brw_context *brw, GLuint dword)
{
#ifdef DEBUG
   assert(intel_batchbuffer_space(brw) >= 4);
#endif
   brw->batch.map[brw->batch.used++] = dword;
}

static INLINE void
intel_batchbuffer_emit_float(struct brw_context *brw, float f)
{
   intel_batchbuffer_emit_dword(brw, float_as_int(f));
}

static INLINE void
intel_batchbuffer_require_space(struct brw_context *brw, GLuint sz, int is_blit)
{
   if (brw->gen >= 6 &&
       brw->batch.is_blit != is_blit && brw->batch.used) {
      intel_batchbuffer_flush(brw);
   }

   brw->batch.is_blit = is_blit;

#ifdef DEBUG
   assert(sz < BATCH_SZ - BATCH_RESERVED);
#endif
   if (intel_batchbuffer_space(brw) < sz)
      intel_batchbuffer_flush(brw);
}

static INLINE void
intel_batchbuffer_begin(struct brw_context *brw, int n, bool is_blit)
{
   intel_batchbuffer_require_space(brw, n * 4, is_blit);

   brw->batch.emit = brw->batch.used;
#ifdef DEBUG
   brw->batch.total = n;
#endif
}

static INLINE void
intel_batchbuffer_advance(struct brw_context *brw)
{
#ifdef DEBUG
   struct intel_batchbuffer *batch = &brw->batch;
   unsigned int _n = batch->used - batch->emit;
   assert(batch->total != 0);
   if (_n != batch->total) {
      fprintf(stderr, "ADVANCE_BATCH: %d of %d dwords emitted\n",
	      _n, batch->total);
      abort();
   }
   batch->total = 0;
#endif
}

void intel_batchbuffer_cached_advance(struct brw_context *brw);

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n) intel_batchbuffer_begin(brw, n, false)
#define BEGIN_BATCH_BLT(n) intel_batchbuffer_begin(brw, n, true)
#define OUT_BATCH(d) intel_batchbuffer_emit_dword(brw, d)
#define OUT_BATCH_F(f) intel_batchbuffer_emit_float(brw, f)
#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   intel_batchbuffer_emit_reloc(brw, buf,			\
				read_domains, write_domain, delta);	\
} while (0)
#define OUT_RELOC_FENCED(buf, read_domains, write_domain, delta) do {	\
   intel_batchbuffer_emit_reloc_fenced(brw, buf,		\
				       read_domains, write_domain, delta); \
} while (0)

#define ADVANCE_BATCH() intel_batchbuffer_advance(brw);
#define CACHED_BATCH() intel_batchbuffer_cached_advance(brw);

#ifdef __cplusplus
}
#endif

#endif
