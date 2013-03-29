#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

#include "pipe/p_screen.h"
#include "util/u_memory.h"

#ifdef DEBUG
# define NOUVEAU_ENABLE_DRIVER_STATISTICS
#endif

typedef uint32_t u32;
typedef uint16_t u16;

extern int nouveau_mesa_debug;

struct nouveau_bo;

struct nouveau_screen {
	struct pipe_screen base;
	struct nouveau_device *device;
	struct nouveau_object *channel;
	struct nouveau_client *client;
	struct nouveau_pushbuf *pushbuf;

	unsigned vidmem_bindings; /* PIPE_BIND_* where VRAM placement is desired */
	unsigned sysmem_bindings; /* PIPE_BIND_* where GART placement is desired */
	unsigned lowmem_bindings; /* PIPE_BIND_* that require an address < 4 GiB */
	/*
	 * For bindings with (vidmem & sysmem) bits set, PIPE_USAGE_* decides
	 * placement.
	 */

	uint16_t class_3d;

	struct {
		struct nouveau_fence *head;
		struct nouveau_fence *tail;
		struct nouveau_fence *current;
		u32 sequence;
		u32 sequence_ack;
		void (*emit)(struct pipe_screen *, u32 *sequence);
		u32  (*update)(struct pipe_screen *);
	} fence;

	struct nouveau_mman *mm_VRAM;
	struct nouveau_mman *mm_GART;

	int64_t cpu_gpu_time_delta;

	boolean hint_buf_keep_sysmem_copy;

#ifdef NOUVEAU_ENABLE_DRIVER_STATISTICS
   union {
      uint64_t v[29];
      struct {
         uint64_t tex_obj_current_count;
         uint64_t tex_obj_current_bytes;
         uint64_t buf_obj_current_count;
         uint64_t buf_obj_current_bytes_vid;
         uint64_t buf_obj_current_bytes_sys;
         uint64_t tex_transfers_rd;
         uint64_t tex_transfers_wr;
         uint64_t tex_copy_count;
         uint64_t tex_blit_count;
         uint64_t tex_cache_flush_count;
         uint64_t buf_transfers_rd;
         uint64_t buf_transfers_wr;
         uint64_t buf_read_bytes_staging_vid;
         uint64_t buf_write_bytes_direct;
         uint64_t buf_write_bytes_staging_vid;
         uint64_t buf_write_bytes_staging_sys;
         uint64_t buf_copy_bytes;
         uint64_t buf_non_kernel_fence_sync_count;
         uint64_t any_non_kernel_fence_sync_count;
         uint64_t query_sync_count;
         uint64_t gpu_serialize_count;
         uint64_t draw_calls_array;
         uint64_t draw_calls_indexed;
         uint64_t draw_calls_fallback_count;
         uint64_t user_buffer_upload_bytes;
         uint64_t constbuf_upload_count;
         uint64_t constbuf_upload_bytes;
         uint64_t pushbuf_count;
         uint64_t resource_validate_count;
      } named;
   } stats;
#endif
};

#ifdef NOUVEAU_ENABLE_DRIVER_STATISTICS
# define NOUVEAU_DRV_STAT(s, n, v) do {         \
      (s)->stats.named.n += (v);               \
   } while(0)
# define NOUVEAU_DRV_STAT_RES(r, n, v) do {                       \
      nouveau_screen((r)->base.screen)->stats.named.n += (v);    \
   } while(0)
# define NOUVEAU_DRV_STAT_IFD(x) x
#else
# define NOUVEAU_DRV_STAT(s, n, v)     do { } while(0)
# define NOUVEAU_DRV_STAT_RES(r, n, v) do { } while(0)
# define NOUVEAU_DRV_STAT_IFD(x)
#endif

static INLINE struct nouveau_screen *
nouveau_screen(struct pipe_screen *pscreen)
{
	return (struct nouveau_screen *)pscreen;
}

boolean
nouveau_screen_bo_get_handle(struct pipe_screen *pscreen,
			     struct nouveau_bo *bo,
			     unsigned stride,
			     struct winsys_handle *whandle);
struct nouveau_bo *
nouveau_screen_bo_from_handle(struct pipe_screen *pscreen,
			      struct winsys_handle *whandle,
			      unsigned *out_stride);


int nouveau_screen_init(struct nouveau_screen *, struct nouveau_device *);
void nouveau_screen_fini(struct nouveau_screen *);

void nouveau_screen_init_vdec(struct nouveau_screen *);

#endif
