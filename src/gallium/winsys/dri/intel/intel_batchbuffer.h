#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "mtypes.h"
#include "ws_dri_bufmgr.h"

struct intel_context;

#define BATCH_SZ 16384
#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

struct intel_batchbuffer
{
   struct bufmgr *bm;
   struct intel_context *intel;

   struct _DriBufferObject *buffer;
   struct _DriFenceObject *last_fence;
   GLuint flags;

   struct _DriBufferList *list;
   GLuint list_count;
   GLubyte *map;
   GLubyte *ptr;

   uint32_t *reloc;
   GLuint reloc_size;
   GLuint nr_relocs;

   GLuint size;

   GLuint dirty_state;
   GLuint id;

  uint32_t poolOffset;
  uint8_t *drmBOVirtual;
  struct _drmBONode *node; /* Validation list node for this buffer */
  int dest_location;     /* Validation list sequence for this buffer */
};

struct intel_batchbuffer *intel_batchbuffer_alloc(struct intel_context
                                                  *intel);

void intel_batchbuffer_free(struct intel_batchbuffer *batch);


void intel_batchbuffer_finish(struct intel_batchbuffer *batch);

struct _DriFenceObject *intel_batchbuffer_flush(struct intel_batchbuffer
                                                *batch);

void intel_batchbuffer_reset(struct intel_batchbuffer *batch);


/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multple
 * intel_buffer_dword() calls.
 */
void intel_batchbuffer_data(struct intel_batchbuffer *batch,
                            const void *data, GLuint bytes, GLuint flags);

void intel_batchbuffer_release_space(struct intel_batchbuffer *batch,
                                     GLuint bytes);

void
intel_offset_relocation(struct intel_batchbuffer *batch,
			unsigned pre_add,
			struct _DriBufferObject *driBO,
			uint64_t val_flags,
			uint64_t val_mask);

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
                                GLuint sz, GLuint flags)
{
   struct _DriFenceObject *fence;

   assert(sz < batch->size - 8);
   if (intel_batchbuffer_space(batch) < sz ||
       (batch->flags != 0 && flags != 0 && batch->flags != flags)) {
      fence = intel_batchbuffer_flush(batch);
      driFenceUnReference(&fence);
   }

   batch->flags |= flags;
}

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n, flags) do {				\
   assert(!intel->prim.flush);					\
   intel_batchbuffer_require_space(intel->batch, (n)*4, flags);	\
} while (0)

#define OUT_BATCH(d)  intel_batchbuffer_emit_dword(intel->batch, d)

#define OUT_RELOC(buf,flags,mask,delta) do { 				\
   assert((delta) >= 0);							\
   intel_offset_relocation(intel->batch, delta, buf, flags, mask); \
} while (0)

#define ADVANCE_BATCH() do { } while(0)


#endif
