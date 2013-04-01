#ifndef __NVC0_SCREEN_H__
#define __NVC0_SCREEN_H__

#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_mm.h"
#include "nouveau/nouveau_fence.h"
#include "nouveau/nouveau_heap.h"

#include "nouveau/nv_object.xml.h"

#include "nvc0_winsys.h"
#include "nvc0_stateobj.h"

#define NVC0_TIC_MAX_ENTRIES 2048
#define NVC0_TSC_MAX_ENTRIES 2048

/* doesn't count reserved slots (for auxiliary constants, immediates, etc.) */
#define NVC0_MAX_PIPE_CONSTBUFS         14
#define NVE4_MAX_PIPE_CONSTBUFS_COMPUTE  7

#define NVC0_MAX_SURFACE_SLOTS 16

struct nvc0_context;

struct nvc0_blitter;

struct nvc0_screen {
   struct nouveau_screen base;

   struct nvc0_context *cur_ctx;

   int num_occlusion_queries_active;

   struct nouveau_bo *text;
   struct nouveau_bo *parm;       /* for COMPUTE */
   struct nouveau_bo *uniform_bo; /* for 3D */
   struct nouveau_bo *tls;
   struct nouveau_bo *txc; /* TIC (offset 0) and TSC (65536) */
   struct nouveau_bo *poly_cache;

   uint16_t mp_count;
   uint16_t mp_count_compute; /* magic reg can make compute use fewer MPs */

   struct nouveau_heap *text_heap;
   struct nouveau_heap *lib_code; /* allocated from text_heap */

   struct nvc0_blitter *blitter;

   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TIC_MAX_ENTRIES / 32];
   } tic;
   
   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TSC_MAX_ENTRIES / 32];
   } tsc;

   struct {
      struct nouveau_bo *bo;
      uint32_t *map;
   } fence;

   struct {
      struct nvc0_program *prog; /* compute state object to read MP counters */
      struct pipe_query *mp_counter[8]; /* counter to query allocation */
      uint8_t num_mp_pm_active[2];
      boolean mp_counters_enabled;
   } pm;

   struct nouveau_mman *mm_VRAM_fe0;

   struct nouveau_object *eng3d; /* sqrt(1/2)|kepler> + sqrt(1/2)|fermi> */
   struct nouveau_object *eng2d;
   struct nouveau_object *m2mf;
   struct nouveau_object *compute;
};

static INLINE struct nvc0_screen *
nvc0_screen(struct pipe_screen *screen)
{
   return (struct nvc0_screen *)screen;
}


/* Performance counter queries:
 */
#define NVE4_PM_QUERY_COUNT  38
#define NVE4_PM_QUERY(i)    (PIPE_QUERY_DRIVER_SPECIFIC + (i))
#define NVE4_PM_QUERY_LAST   NVE4_PM_QUERY(NVE4_PM_QUERY_COUNT - 1)
#define NVE4_PM_QUERY_PROF_TRIGGER_0            0
#define NVE4_PM_QUERY_PROF_TRIGGER_1            1
#define NVE4_PM_QUERY_PROF_TRIGGER_2            2
#define NVE4_PM_QUERY_PROF_TRIGGER_3            3
#define NVE4_PM_QUERY_PROF_TRIGGER_4            4
#define NVE4_PM_QUERY_PROF_TRIGGER_5            5
#define NVE4_PM_QUERY_PROF_TRIGGER_6            6
#define NVE4_PM_QUERY_PROF_TRIGGER_7            7
#define NVE4_PM_QUERY_LAUNCHED_WARPS            8
#define NVE4_PM_QUERY_LAUNCHED_THREADS          9
#define NVE4_PM_QUERY_LAUNCHED_CTA              10
#define NVE4_PM_QUERY_INST_ISSUED1              11
#define NVE4_PM_QUERY_INST_ISSUED2              12
#define NVE4_PM_QUERY_INST_EXECUTED             13
#define NVE4_PM_QUERY_LD_LOCAL                  14
#define NVE4_PM_QUERY_ST_LOCAL                  15
#define NVE4_PM_QUERY_LD_SHARED                 16
#define NVE4_PM_QUERY_ST_SHARED                 17
#define NVE4_PM_QUERY_L1_LOCAL_LOAD_HIT         18
#define NVE4_PM_QUERY_L1_LOCAL_LOAD_MISS        19
#define NVE4_PM_QUERY_L1_LOCAL_STORE_HIT        20
#define NVE4_PM_QUERY_L1_LOCAL_STORE_MISS       21
#define NVE4_PM_QUERY_GLD_REQUEST               22
#define NVE4_PM_QUERY_GST_REQUEST               23
#define NVE4_PM_QUERY_L1_GLOBAL_LOAD_HIT        24
#define NVE4_PM_QUERY_L1_GLOBAL_LOAD_MISS       25
#define NVE4_PM_QUERY_GLD_TRANSACTIONS_UNCACHED 26
#define NVE4_PM_QUERY_GST_TRANSACTIONS          27
#define NVE4_PM_QUERY_BRANCH                    28
#define NVE4_PM_QUERY_BRANCH_DIVERGENT          29
#define NVE4_PM_QUERY_ACTIVE_WARPS              30
#define NVE4_PM_QUERY_ACTIVE_CYCLES             31
#define NVE4_PM_QUERY_METRIC_IPC                32
#define NVE4_PM_QUERY_METRIC_IPAC               33
#define NVE4_PM_QUERY_METRIC_IPEC               34
#define NVE4_PM_QUERY_METRIC_MP_OCCUPANCY       35
#define NVE4_PM_QUERY_METRIC_MP_EFFICIENCY      36
#define NVE4_PM_QUERY_METRIC_INST_REPLAY_OHEAD  37
/*
#define NVE4_PM_QUERY_GR_IDLE                   50
#define NVE4_PM_QUERY_BSP_IDLE                  51
#define NVE4_PM_QUERY_VP_IDLE                   52
#define NVE4_PM_QUERY_PPP_IDLE                  53
#define NVE4_PM_QUERY_CE0_IDLE                  54
#define NVE4_PM_QUERY_CE1_IDLE                  55
#define NVE4_PM_QUERY_CE2_IDLE                  56
*/
/* L2 queries (PCOUNTER) */
/*
#define NVE4_PM_QUERY_L2_SUBP_WRITE_L1_SECTOR_QUERIES 57
...
*/
/* TEX queries (PCOUNTER) */
/*
#define NVE4_PM_QUERY_TEX0_CACHE_SECTOR_QUERIES 58
...
*/

/* Driver statistics queries:
 */
#ifdef NOUVEAU_ENABLE_DRIVER_STATISTICS

#define NVC0_QUERY_DRV_STAT(i)    (PIPE_QUERY_DRIVER_SPECIFIC + 1024 + (i))
#define NVC0_QUERY_DRV_STAT_COUNT  29
#define NVC0_QUERY_DRV_STAT_LAST   NVC0_QUERY_DRV_STAT(NVC0_QUERY_DRV_STAT_COUNT - 1)
#define NVC0_QUERY_DRV_STAT_TEX_OBJECT_CURRENT_COUNT         0
#define NVC0_QUERY_DRV_STAT_TEX_OBJECT_CURRENT_BYTES         1
#define NVC0_QUERY_DRV_STAT_BUF_OBJECT_CURRENT_COUNT         2
#define NVC0_QUERY_DRV_STAT_BUF_OBJECT_CURRENT_BYTES_VID     3
#define NVC0_QUERY_DRV_STAT_BUF_OBJECT_CURRENT_BYTES_SYS     4
#define NVC0_QUERY_DRV_STAT_TEX_TRANSFERS_READ               5
#define NVC0_QUERY_DRV_STAT_TEX_TRANSFERS_WRITE              6
#define NVC0_QUERY_DRV_STAT_TEX_COPY_COUNT                   7
#define NVC0_QUERY_DRV_STAT_TEX_BLIT_COUNT                   8
#define NVC0_QUERY_DRV_STAT_TEX_CACHE_FLUSH_COUNT            9
#define NVC0_QUERY_DRV_STAT_BUF_TRANSFERS_READ              10
#define NVC0_QUERY_DRV_STAT_BUF_TRANSFERS_WRITE             11
#define NVC0_QUERY_DRV_STAT_BUF_READ_BYTES_STAGING_VID      12
#define NVC0_QUERY_DRV_STAT_BUF_WRITE_BYTES_DIRECT          13
#define NVC0_QUERY_DRV_STAT_BUF_WRITE_BYTES_STAGING_VID     14
#define NVC0_QUERY_DRV_STAT_BUF_WRITE_BYTES_STAGING_SYS     15
#define NVC0_QUERY_DRV_STAT_BUF_COPY_BYTES                  16
#define NVC0_QUERY_DRV_STAT_BUF_NON_KERNEL_FENCE_SYNC_COUNT 17
#define NVC0_QUERY_DRV_STAT_ANY_NON_KERNEL_FENCE_SYNC_COUNT 18
#define NVC0_QUERY_DRV_STAT_QUERY_SYNC_COUNT                19
#define NVC0_QUERY_DRV_STAT_GPU_SERIALIZE_COUNT             20
#define NVC0_QUERY_DRV_STAT_DRAW_CALLS_ARRAY                21
#define NVC0_QUERY_DRV_STAT_DRAW_CALLS_INDEXED              22
#define NVC0_QUERY_DRV_STAT_DRAW_CALLS_FALLBACK_COUNT       23
#define NVC0_QUERY_DRV_STAT_USER_BUFFER_UPLOAD_BYTES        24
#define NVC0_QUERY_DRV_STAT_CONSTBUF_UPLOAD_COUNT           25
#define NVC0_QUERY_DRV_STAT_CONSTBUF_UPLOAD_BYTES           26
#define NVC0_QUERY_DRV_STAT_PUSHBUF_COUNT                   27
#define NVC0_QUERY_DRV_STAT_RESOURCE_VALIDATE_COUNT         28

#else

#define NVC0_QUERY_DRV_STAT_COUNT 0

#endif

int nvc0_screen_get_driver_query_info(struct pipe_screen *, unsigned,
                                      struct pipe_driver_query_info *);

boolean nvc0_blitter_create(struct nvc0_screen *);
void nvc0_blitter_destroy(struct nvc0_screen *);

void nvc0_screen_make_buffers_resident(struct nvc0_screen *);

int nvc0_screen_tic_alloc(struct nvc0_screen *, void *);
int nvc0_screen_tsc_alloc(struct nvc0_screen *, void *);

int nve4_screen_compute_setup(struct nvc0_screen *, struct nouveau_pushbuf *);

boolean nvc0_screen_resize_tls_area(struct nvc0_screen *, uint32_t lpos,
                                    uint32_t lneg, uint32_t cstack);

static INLINE void
nvc0_resource_fence(struct nv04_resource *res, uint32_t flags)
{
   struct nvc0_screen *screen = nvc0_screen(res->base.screen);

   if (res->mm) {
      nouveau_fence_ref(screen->base.fence.current, &res->fence);
      if (flags & NOUVEAU_BO_WR)
         nouveau_fence_ref(screen->base.fence.current, &res->fence_wr);
   }
}

static INLINE void
nvc0_resource_validate(struct nv04_resource *res, uint32_t flags)
{
   if (likely(res->bo)) {
      if (flags & NOUVEAU_BO_WR)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING |
            NOUVEAU_BUFFER_STATUS_DIRTY;
      if (flags & NOUVEAU_BO_RD)
         res->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;

      nvc0_resource_fence(res, flags);
   }
}

struct nvc0_format {
   uint32_t rt;
   uint32_t tic;
   uint32_t vtx;
   uint32_t usage;
};

extern const struct nvc0_format nvc0_format_table[];

static INLINE void
nvc0_screen_tic_unlock(struct nvc0_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0)
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
}

static INLINE void
nvc0_screen_tsc_unlock(struct nvc0_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0)
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
}

static INLINE void
nvc0_screen_tic_free(struct nvc0_screen *screen, struct nv50_tic_entry *tic)
{
   if (tic->id >= 0) {
      screen->tic.entries[tic->id] = NULL;
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
   }
}

static INLINE void
nvc0_screen_tsc_free(struct nvc0_screen *screen, struct nv50_tsc_entry *tsc)
{
   if (tsc->id >= 0) {
      screen->tsc.entries[tsc->id] = NULL;
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
   }
}

#endif
