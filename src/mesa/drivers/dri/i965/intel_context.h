/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTELCONTEXT_INC
#define INTELCONTEXT_INC


#include <stdbool.h>
#include <string.h>
#include "main/mtypes.h"
#include "main/mm.h"

#ifdef __cplusplus
extern "C" {
	/* Evil hack for using libdrm in a c++ compiler. */
	#define virtual virt
#endif

#include "drm.h"
#include "intel_bufmgr.h"

#include "intel_screen.h"
#include "intel_tex_obj.h"
#include "i915_drm.h"

#ifdef __cplusplus
	#undef virtual
#endif

#include "tnl/t_vertex.h"

struct intel_region;

#define INTEL_WRITE_PART  0x1
#define INTEL_WRITE_FULL  0x2
#define INTEL_READ        0x4

#ifndef likely
#ifdef __GNUC__
#define likely(expr) (__builtin_expect(expr, 1))
#define unlikely(expr) (__builtin_expect(expr, 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif
#endif

struct intel_sync_object {
   struct gl_sync_object Base;

   /** Batch associated with this sync object */
   drm_intel_bo *bo;
};

struct brw_context;

struct intel_batchbuffer {
   /** Current batchbuffer being queued up. */
   drm_intel_bo *bo;
   /** Last BO submitted to the hardware.  Used for glFinish(). */
   drm_intel_bo *last_bo;
   /** BO for post-sync nonzero writes for gen6 workaround. */
   drm_intel_bo *workaround_bo;
   bool need_workaround_flush;

   struct cached_batch_item *cached_items;

   uint16_t emit, total;
   uint16_t used, reserved_space;
   uint32_t *map;
   uint32_t *cpu_map;
#define BATCH_SZ (8192*sizeof(uint32_t))

   uint32_t state_batch_offset;
   bool is_blit;
   bool needs_sol_reset;

   struct {
      uint16_t used;
      int reloc_count;
   } saved;
};

/**
 * Align a value down to an alignment value
 *
 * If \c value is not already aligned to the requested alignment value, it
 * will be rounded down.
 *
 * \param value  Value to be rounded
 * \param alignment  Alignment value to be used.  This must be a power of two.
 *
 * \sa ALIGN()
 */
#define ROUND_DOWN_TO(value, alignment) ((value) & ~(alignment - 1))

static INLINE uint32_t
U_FIXED(float value, uint32_t frac_bits)
{
   value *= (1 << frac_bits);
   return value < 0 ? 0 : value;
}

static INLINE uint32_t
S_FIXED(float value, uint32_t frac_bits)
{
   return value * (1 << frac_bits);
}

/* ================================================================
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 * XXX Put this in src/mesa/main/imports.h ???
 */
#if defined(i386) || defined(__i386__)
static INLINE void * __memcpy(void * to, const void * from, size_t n)
{
   int d0, d1, d2;
   __asm__ __volatile__(
      "rep ; movsl\n\t"
      "testb $2,%b4\n\t"
      "je 1f\n\t"
      "movsw\n"
      "1:\ttestb $1,%b4\n\t"
      "je 2f\n\t"
      "movsb\n"
      "2:"
      : "=&c" (d0), "=&D" (d1), "=&S" (d2)
      :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
      : "memory");
   return (to);
}
#else
#define __memcpy(a,b,c) memcpy(a,b,c)
#endif


/* ================================================================
 * Debugging:
 */
extern int INTEL_DEBUG;

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_BLIT	0x8
#define DEBUG_MIPTREE   0x10
#define DEBUG_PERF	0x20
#define DEBUG_BATCH     0x80
#define DEBUG_PIXEL     0x100
#define DEBUG_BUFMGR    0x200
#define DEBUG_REGION    0x400
#define DEBUG_FBO       0x800
#define DEBUG_GS        0x1000
#define DEBUG_SYNC	0x2000
#define DEBUG_PRIMS	0x4000
#define DEBUG_VERTS	0x8000
#define DEBUG_DRI       0x10000
#define DEBUG_SF        0x20000
#define DEBUG_STATS     0x100000
#define DEBUG_WM        0x400000
#define DEBUG_URB       0x800000
#define DEBUG_VS        0x1000000
#define DEBUG_CLIP      0x2000000
#define DEBUG_AUB       0x4000000
#define DEBUG_SHADER_TIME 0x8000000
#define DEBUG_BLORP     0x10000000
#define DEBUG_NO16      0x20000000
#define DEBUG_VUE       0x40000000

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "INTEL-MESA"
#include <cutils/log.h>
#ifndef ALOGW
#define ALOGW LOGW
#endif
#define dbg_printf(...)	ALOGW(__VA_ARGS__)
#else
#define dbg_printf(...)	printf(__VA_ARGS__)
#endif /* HAVE_ANDROID_PLATFORM */

#define DBG(...) do {						\
	if (unlikely(INTEL_DEBUG & FILE_DEBUG_FLAG))		\
		dbg_printf(__VA_ARGS__);			\
} while(0)

#define perf_debug(...) do {					\
   static GLuint msg_id = 0;                                    \
   if (unlikely(INTEL_DEBUG & DEBUG_PERF))                      \
      dbg_printf(__VA_ARGS__);                                  \
   if (brw->perf_debug)                                         \
      _mesa_gl_debug(&brw->ctx, &msg_id,                        \
                     MESA_DEBUG_TYPE_PERFORMANCE,               \
                     MESA_DEBUG_SEVERITY_MEDIUM,                \
                     __VA_ARGS__);                              \
} while(0)

#define WARN_ONCE(cond, fmt...) do {                            \
   if (unlikely(cond)) {                                        \
      static bool _warned = false;                              \
      static GLuint msg_id = 0;                                 \
      if (!_warned) {                                           \
         fprintf(stderr, "WARNING: ");                          \
         fprintf(stderr, fmt);                                  \
         _warned = true;                                        \
                                                                \
         _mesa_gl_debug(ctx, &msg_id,                           \
                        MESA_DEBUG_TYPE_OTHER,                  \
                        MESA_DEBUG_SEVERITY_HIGH, fmt);         \
      }                                                         \
   }                                                            \
} while (0)

/* ================================================================
 * intel_context.c:
 */

extern bool intelInitContext(struct brw_context *brw,
                             int api,
                             unsigned major_version,
                             unsigned minor_version,
                             const struct gl_config * mesaVis,
                             __DRIcontext * driContextPriv,
                             void *sharedContextPrivate,
                             struct dd_function_table *functions,
                             unsigned *dri_ctx_error);

extern void intelFinish(struct gl_context * ctx);

extern void intelInitDriverFunctions(struct dd_function_table *functions);

void intel_init_syncobj_functions(struct dd_function_table *functions);

enum {
   DRI_CONF_BO_REUSE_DISABLED,
   DRI_CONF_BO_REUSE_ALL
};

extern int intel_translate_shadow_compare_func(GLenum func);
extern int intel_translate_compare_func(GLenum func);
extern int intel_translate_stencil_op(GLenum op);
extern int intel_translate_logic_op(GLenum opcode);

void intel_update_renderbuffers(__DRIcontext *context,
				__DRIdrawable *drawable);
void intel_prepare_render(struct brw_context *brw);

void
intel_resolve_for_dri2_flush(struct brw_context *brw,
                             __DRIdrawable *drawable);

extern void
intelInitExtensions(struct gl_context *ctx);
extern void
intelInitClearFuncs(struct dd_function_table *functions);

static INLINE bool
is_power_of_two(uint32_t value)
{
   return (value & (value - 1)) == 0;
}

#ifdef __cplusplus
}
#endif

#endif
