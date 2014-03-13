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
 *   - Disabling OA counters on Gen6+ (3 DWords = 12 bytes)
 *   - Ending MI_REPORT_PERF_COUNT on Gen5+, plus associated PIPE_CONTROLs:
 *     - Two sets of PIPE_CONTROLs, which become 3 PIPE_CONTROLs each on SNB,
 *       which are 4 DWords each ==> 2 * 3 * 4 * 4 = 96 bytes
 *     - 3 DWords for MI_REPORT_PERF_COUNT itself on Gen6+.  ==> 12 bytes.
 *       On Ironlake, it's 6 DWords, but we have some slack due to the lack of
 *       Sandybridge PIPE_CONTROL madness.
 */
#define BATCH_RESERVED 146

struct intel_batchbuffer;

void intel_batchbuffer_emit_render_ring_prelude(struct brw_context *brw);
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
                            const void *data, GLuint bytes,
                            enum brw_gpu_ring ring);

bool intel_batchbuffer_emit_reloc(struct brw_context *brw,
                                       drm_intel_bo *buffer,
				       uint32_t read_domains,
				       uint32_t write_domain,
				       uint32_t offset);
bool intel_batchbuffer_emit_reloc64(struct brw_context *brw,
                                    drm_intel_bo *buffer,
                                    uint32_t read_domains,
                                    uint32_t write_domain,
                                    uint32_t offset);
void brw_emit_pipe_control_flush(struct brw_context *brw, uint32_t flags);
void brw_emit_pipe_control_write(struct brw_context *brw, uint32_t flags,
                                 drm_intel_bo *bo, uint32_t offset,
                                 uint32_t imm_lower, uint32_t imm_upper);
void intel_batchbuffer_emit_mi_flush(struct brw_context *brw);
void intel_emit_post_sync_nonzero_flush(struct brw_context *brw);
void intel_emit_depth_stall_flushes(struct brw_context *brw);
void gen7_emit_vs_workaround_flush(struct brw_context *brw);
void gen7_emit_cs_stall_flush(struct brw_context *brw);

static inline uint32_t float_as_int(float f)
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
static inline unsigned
intel_batchbuffer_space(struct brw_context *brw)
{
   return (brw->batch.state_batch_offset - brw->batch.reserved_space)
      - brw->batch.used*4;
}


static inline void
intel_batchbuffer_emit_dword(struct brw_context *brw, GLuint dword)
{
#ifdef DEBUG
   assert(intel_batchbuffer_space(brw) >= 4);
#endif
   brw->batch.map[brw->batch.used++] = dword;
   assert(brw->batch.ring != UNKNOWN_RING);
}

static inline void
intel_batchbuffer_emit_float(struct brw_context *brw, float f)
{
   intel_batchbuffer_emit_dword(brw, float_as_int(f));
}

static inline void
intel_batchbuffer_require_space(struct brw_context *brw, GLuint sz,
                                enum brw_gpu_ring ring)
{
   /* If we're switching rings, implicitly flush the batch. */
   if (unlikely(ring != brw->batch.ring) && brw->batch.ring != UNKNOWN_RING &&
       brw->gen >= 6) {
      intel_batchbuffer_flush(brw);
   }

#ifdef DEBUG
   assert(sz < BATCH_SZ - BATCH_RESERVED);
#endif
   if (intel_batchbuffer_space(brw) < sz)
      intel_batchbuffer_flush(brw);

   enum brw_gpu_ring prev_ring = brw->batch.ring;
   /* The intel_batchbuffer_flush() calls above might have changed
    * brw->batch.ring to UNKNOWN_RING, so we need to set it here at the end.
    */
   brw->batch.ring = ring;

   if (unlikely(prev_ring == UNKNOWN_RING && ring == RENDER_RING))
      intel_batchbuffer_emit_render_ring_prelude(brw);
}

static inline void
intel_batchbuffer_begin(struct brw_context *brw, int n, enum brw_gpu_ring ring)
{
   intel_batchbuffer_require_space(brw, n * 4, ring);

   brw->batch.emit = brw->batch.used;
#ifdef DEBUG
   brw->batch.total = n;
#endif
}

static inline void
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

#define BEGIN_BATCH(n) intel_batchbuffer_begin(brw, n, RENDER_RING)
#define BEGIN_BATCH_BLT(n) intel_batchbuffer_begin(brw, n, BLT_RING)
#define OUT_BATCH(d) intel_batchbuffer_emit_dword(brw, d)
#define OUT_BATCH_F(f) intel_batchbuffer_emit_float(brw, f)
#define OUT_RELOC(buf, read_domains, write_domain, delta) do {		\
   intel_batchbuffer_emit_reloc(brw, buf,			\
				read_domains, write_domain, delta);	\
} while (0)

/* Handle 48-bit address relocations for Gen8+ */
#define OUT_RELOC64(buf, read_domains, write_domain, delta) do { \
   intel_batchbuffer_emit_reloc64(brw, buf, read_domains, write_domain, delta);	\
} while (0)

#define ADVANCE_BATCH() intel_batchbuffer_advance(brw);

#ifdef __cplusplus
}
#endif

#endif
