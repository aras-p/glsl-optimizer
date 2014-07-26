/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef RADEON_WINSYS_H
#define RADEON_WINSYS_H

/* The public winsys interface header for the radeon driver. */

/* R300 features in DRM.
 *
 * 2.6.0:
 * - Hyper-Z
 * - GB_Z_PEQ_CONFIG on rv350->r4xx
 * - R500 FG_ALPHA_VALUE
 *
 * 2.8.0:
 * - R500 US_FORMAT regs
 * - R500 ARGB2101010 colorbuffer
 * - CMask and AA regs
 * - R16F/RG16F
 */

#include "pipebuffer/pb_buffer.h"
#include "radeon_surface.h"

#define RADEON_MAX_CMDBUF_DWORDS (16 * 1024)

#define RADEON_FLUSH_ASYNC		(1 << 0)
#define RADEON_FLUSH_KEEP_TILING_FLAGS	(1 << 1) /* needs DRM 2.12.0 */
#define RADEON_FLUSH_COMPUTE		(1 << 2)
#define RADEON_FLUSH_END_OF_FRAME       (1 << 3)

/* Tiling flags. */
enum radeon_bo_layout {
    RADEON_LAYOUT_LINEAR = 0,
    RADEON_LAYOUT_TILED,
    RADEON_LAYOUT_SQUARETILED,

    RADEON_LAYOUT_UNKNOWN
};

enum radeon_bo_domain { /* bitfield */
    RADEON_DOMAIN_GTT  = 2,
    RADEON_DOMAIN_VRAM = 4,
    RADEON_DOMAIN_VRAM_GTT = RADEON_DOMAIN_VRAM | RADEON_DOMAIN_GTT
};

enum radeon_bo_flag { /* bitfield */
   RADEON_FLAG_GTT_WC = (1 << 0)
};

enum radeon_bo_usage { /* bitfield */
    RADEON_USAGE_READ = 2,
    RADEON_USAGE_WRITE = 4,
    RADEON_USAGE_READWRITE = RADEON_USAGE_READ | RADEON_USAGE_WRITE
};

enum radeon_family {
    CHIP_UNKNOWN = 0,
    CHIP_R300, /* R3xx-based cores. */
    CHIP_R350,
    CHIP_RV350,
    CHIP_RV370,
    CHIP_RV380,
    CHIP_RS400,
    CHIP_RC410,
    CHIP_RS480,
    CHIP_R420,     /* R4xx-based cores. */
    CHIP_R423,
    CHIP_R430,
    CHIP_R480,
    CHIP_R481,
    CHIP_RV410,
    CHIP_RS600,
    CHIP_RS690,
    CHIP_RS740,
    CHIP_RV515,    /* R5xx-based cores. */
    CHIP_R520,
    CHIP_RV530,
    CHIP_R580,
    CHIP_RV560,
    CHIP_RV570,
    CHIP_R600,
    CHIP_RV610,
    CHIP_RV630,
    CHIP_RV670,
    CHIP_RV620,
    CHIP_RV635,
    CHIP_RS780,
    CHIP_RS880,
    CHIP_RV770,
    CHIP_RV730,
    CHIP_RV710,
    CHIP_RV740,
    CHIP_CEDAR,
    CHIP_REDWOOD,
    CHIP_JUNIPER,
    CHIP_CYPRESS,
    CHIP_HEMLOCK,
    CHIP_PALM,
    CHIP_SUMO,
    CHIP_SUMO2,
    CHIP_BARTS,
    CHIP_TURKS,
    CHIP_CAICOS,
    CHIP_CAYMAN,
    CHIP_ARUBA,
    CHIP_TAHITI,
    CHIP_PITCAIRN,
    CHIP_VERDE,
    CHIP_OLAND,
    CHIP_HAINAN,
    CHIP_BONAIRE,
    CHIP_KAVERI,
    CHIP_KABINI,
    CHIP_HAWAII,
    CHIP_MULLINS,
    CHIP_LAST,
};

enum chip_class {
    CLASS_UNKNOWN = 0,
    R300,
    R400,
    R500,
    R600,
    R700,
    EVERGREEN,
    CAYMAN,
    SI,
    CIK,
};

enum ring_type {
    RING_GFX = 0,
    RING_DMA,
    RING_UVD,
    RING_VCE,
    RING_LAST,
};

enum radeon_value_id {
    RADEON_REQUESTED_VRAM_MEMORY,
    RADEON_REQUESTED_GTT_MEMORY,
    RADEON_BUFFER_WAIT_TIME_NS,
    RADEON_TIMESTAMP,
    RADEON_NUM_CS_FLUSHES,
    RADEON_NUM_BYTES_MOVED,
    RADEON_VRAM_USAGE,
    RADEON_GTT_USAGE
};

enum radeon_bo_priority {
    RADEON_PRIO_MIN,
    RADEON_PRIO_SHADER_DATA, /* shader code, resource descriptors */
    RADEON_PRIO_SHADER_BUFFER_RO, /* read-only */
    RADEON_PRIO_SHADER_TEXTURE_RO, /* read-only */
    RADEON_PRIO_SHADER_RESOURCE_RW, /* buffers, textures, streamout, GS rings, RATs; read/write */
    RADEON_PRIO_COLOR_BUFFER,
    RADEON_PRIO_DEPTH_BUFFER,
    RADEON_PRIO_SHADER_TEXTURE_MSAA,
    RADEON_PRIO_COLOR_BUFFER_MSAA,
    RADEON_PRIO_DEPTH_BUFFER_MSAA,
    RADEON_PRIO_COLOR_META,
    RADEON_PRIO_DEPTH_META,
    RADEON_PRIO_MAX /* must be <= 15 */
};

struct winsys_handle;
struct radeon_winsys_cs_handle;

struct radeon_winsys_cs {
    unsigned                    cdw;  /* Number of used dwords. */
    uint32_t                    *buf; /* The command buffer. */
    enum ring_type              ring_type;
};

struct radeon_info {
    uint32_t                    pci_id;
    enum radeon_family          family;
    enum chip_class             chip_class;
    uint64_t                    gart_size;
    uint64_t                    vram_size;
    uint32_t                    max_sclk;
    uint32_t                    max_compute_units;
    uint32_t                    max_se;
    uint32_t                    max_sh_per_se;

    uint32_t                    drm_major; /* version */
    uint32_t                    drm_minor;
    uint32_t                    drm_patchlevel;

    boolean                     has_uvd;
    uint32_t                    vce_fw_version;

    uint32_t                    r300_num_gb_pipes;
    uint32_t                    r300_num_z_pipes;

    uint32_t                    r600_num_backends;
    uint32_t                    r600_clock_crystal_freq;
    uint32_t                    r600_tiling_config;
    uint32_t                    r600_num_tile_pipes;
    uint32_t                    r600_max_pipes;
    boolean                     r600_virtual_address;
    boolean                     r600_has_dma;

    uint32_t                    r600_backend_map;
    boolean                     r600_backend_map_valid;

    boolean                     si_tile_mode_array_valid;
    uint32_t                    si_tile_mode_array[32];

    boolean                     cik_macrotile_mode_array_valid;
    uint32_t                    cik_macrotile_mode_array[16];
};

enum radeon_feature_id {
    RADEON_FID_R300_HYPERZ_ACCESS,     /* ZMask + HiZ */
    RADEON_FID_R300_CMASK_ACCESS,
};

struct radeon_winsys {
    /**
     * The screen object this winsys was created for
     */
    struct pipe_screen *screen;

    /**
     * Decrement the winsys reference count.
     *
     * \param ws  The winsys this function is called for.
     * \return    True if the winsys and screen should be destroyed.
     */
    bool (*unref)(struct radeon_winsys *ws);

    /**
     * Destroy this winsys.
     *
     * \param ws        The winsys this function is called from.
     */
    void (*destroy)(struct radeon_winsys *ws);

    /**
     * Query an info structure from winsys.
     *
     * \param ws        The winsys this function is called from.
     * \param info      Return structure
     */
    void (*query_info)(struct radeon_winsys *ws,
                       struct radeon_info *info);

    /**************************************************************************
     * Buffer management. Buffer attributes are mostly fixed over its lifetime.
     *
     * Remember that gallium gets to choose the interface it needs, and the
     * window systems must then implement that interface (rather than the
     * other way around...).
     *************************************************************************/

    /**
     * Create a buffer object.
     *
     * \param ws        The winsys this function is called from.
     * \param size      The size to allocate.
     * \param alignment An alignment of the buffer in memory.
     * \param use_reusable_pool Whether the cache buffer manager should be used.
     * \param domain    A bitmask of the RADEON_DOMAIN_* flags.
     * \return          The created buffer object.
     */
    struct pb_buffer *(*buffer_create)(struct radeon_winsys *ws,
                                       unsigned size,
                                       unsigned alignment,
                                       boolean use_reusable_pool,
                                       enum radeon_bo_domain domain,
                                       enum radeon_bo_flag flags);

    struct radeon_winsys_cs_handle *(*buffer_get_cs_handle)(
            struct pb_buffer *buf);

    /**
     * Map the entire data store of a buffer object into the client's address
     * space.
     *
     * \param buf       A winsys buffer object to map.
     * \param cs        A command stream to flush if the buffer is referenced by it.
     * \param usage     A bitmask of the PIPE_TRANSFER_* flags.
     * \return          The pointer at the beginning of the buffer.
     */
    void *(*buffer_map)(struct radeon_winsys_cs_handle *buf,
                        struct radeon_winsys_cs *cs,
                        enum pipe_transfer_usage usage);

    /**
     * Unmap a buffer object from the client's address space.
     *
     * \param buf       A winsys buffer object to unmap.
     */
    void (*buffer_unmap)(struct radeon_winsys_cs_handle *buf);

    /**
     * Return TRUE if a buffer object is being used by the GPU.
     *
     * \param buf       A winsys buffer object.
     * \param usage     Only check whether the buffer is busy for the given usage.
     */
    boolean (*buffer_is_busy)(struct pb_buffer *buf,
                              enum radeon_bo_usage usage);

    /**
     * Wait for a buffer object until it is not used by a GPU. This is
     * equivalent to a fence placed after the last command using the buffer,
     * and synchronizing to the fence.
     *
     * \param buf       A winsys buffer object to wait for.
     * \param usage     Only wait until the buffer is idle for the given usage,
     *                  but may still be busy for some other usage.
     */
    void (*buffer_wait)(struct pb_buffer *buf, enum radeon_bo_usage usage);

    /**
     * Return tiling flags describing a memory layout of a buffer object.
     *
     * \param buf       A winsys buffer object to get the flags from.
     * \param macrotile A pointer to the return value of the microtile flag.
     * \param microtile A pointer to the return value of the macrotile flag.
     *
     * \note microtile and macrotile are not bitmasks!
     */
    void (*buffer_get_tiling)(struct pb_buffer *buf,
                              enum radeon_bo_layout *microtile,
                              enum radeon_bo_layout *macrotile,
                              unsigned *bankw, unsigned *bankh,
                              unsigned *tile_split,
                              unsigned *stencil_tile_split,
                              unsigned *mtilea,
                              bool *scanout);

    /**
     * Set tiling flags describing a memory layout of a buffer object.
     *
     * \param buf       A winsys buffer object to set the flags for.
     * \param cs        A command stream to flush if the buffer is referenced by it.
     * \param macrotile A macrotile flag.
     * \param microtile A microtile flag.
     * \param stride    A stride of the buffer in bytes, for texturing.
     *
     * \note microtile and macrotile are not bitmasks!
     */
    void (*buffer_set_tiling)(struct pb_buffer *buf,
                              struct radeon_winsys_cs *rcs,
                              enum radeon_bo_layout microtile,
                              enum radeon_bo_layout macrotile,
                              unsigned bankw, unsigned bankh,
                              unsigned tile_split,
                              unsigned stencil_tile_split,
                              unsigned mtilea,
                              unsigned stride,
                              bool scanout);

    /**
     * Get a winsys buffer from a winsys handle. The internal structure
     * of the handle is platform-specific and only a winsys should access it.
     *
     * \param ws        The winsys this function is called from.
     * \param whandle   A winsys handle pointer as was received from a state
     *                  tracker.
     * \param stride    The returned buffer stride in bytes.
     */
    struct pb_buffer *(*buffer_from_handle)(struct radeon_winsys *ws,
                                            struct winsys_handle *whandle,
                                            unsigned *stride);

    /**
     * Get a winsys handle from a winsys buffer. The internal structure
     * of the handle is platform-specific and only a winsys should access it.
     *
     * \param buf       A winsys buffer object to get the handle from.
     * \param whandle   A winsys handle pointer.
     * \param stride    A stride of the buffer in bytes, for texturing.
     * \return          TRUE on success.
     */
    boolean (*buffer_get_handle)(struct pb_buffer *buf,
                                 unsigned stride,
                                 struct winsys_handle *whandle);

    /**
     * Return the virtual address of a buffer.
     *
     * \param buf       A winsys buffer object
     * \return          virtual address
     */
    uint64_t (*buffer_get_virtual_address)(struct radeon_winsys_cs_handle *buf);

    /**
     * Query the initial placement of the buffer from the kernel driver.
     */
    enum radeon_bo_domain (*buffer_get_initial_domain)(struct radeon_winsys_cs_handle *buf);

    /**************************************************************************
     * Command submission.
     *
     * Each pipe context should create its own command stream and submit
     * commands independently of other contexts.
     *************************************************************************/

    /**
     * Create a command stream.
     *
     * \param ws        The winsys this function is called from.
     * \param ring_type The ring type (GFX, DMA, UVD)
     * \param flush     Flush callback function associated with the command stream.
     * \param user      User pointer that will be passed to the flush callback.
     * \param trace_buf Trace buffer when tracing is enabled
     */
    struct radeon_winsys_cs *(*cs_create)(struct radeon_winsys *ws,
                                          enum ring_type ring_type,
                                          void (*flush)(void *ctx, unsigned flags,
							struct pipe_fence_handle **fence),
                                          void *flush_ctx,
                                          struct radeon_winsys_cs_handle *trace_buf);

    /**
     * Destroy a command stream.
     *
     * \param cs        A command stream to destroy.
     */
    void (*cs_destroy)(struct radeon_winsys_cs *cs);

    /**
     * Add a new buffer relocation. Every relocation must first be added
     * before it can be written.
     *
     * \param cs  A command stream to add buffer for validation against.
     * \param buf A winsys buffer to validate.
     * \param usage   Whether the buffer is used for read and/or write.
     * \param domain  Bitmask of the RADEON_DOMAIN_* flags.
     * \param priority  A higher number means a greater chance of being
     *                  placed in the requested domain. 15 is the maximum.
     * \return Relocation index.
     */
    unsigned (*cs_add_reloc)(struct radeon_winsys_cs *cs,
                             struct radeon_winsys_cs_handle *buf,
                             enum radeon_bo_usage usage,
                             enum radeon_bo_domain domain,
                             enum radeon_bo_priority priority);

    /**
     * Return the index of an already-added buffer.
     *
     * \param cs        Command stream
     * \param buf       Buffer
     * \return          The buffer index, or -1 if the buffer has not been added.
     */
    int (*cs_get_reloc)(struct radeon_winsys_cs *cs,
                        struct radeon_winsys_cs_handle *buf);

    /**
     * Return TRUE if there is enough memory in VRAM and GTT for the relocs
     * added so far. If the validation fails, all the relocations which have
     * been added since the last call of cs_validate will be removed and
     * the CS will be flushed (provided there are still any relocations).
     *
     * \param cs        A command stream to validate.
     */
    boolean (*cs_validate)(struct radeon_winsys_cs *cs);

    /**
     * Return TRUE if there is enough memory in VRAM and GTT for the relocs
     * added so far.
     *
     * \param cs        A command stream to validate.
     * \param vram      VRAM memory size pending to be use
     * \param gtt       GTT memory size pending to be use
     */
    boolean (*cs_memory_below_limit)(struct radeon_winsys_cs *cs, uint64_t vram, uint64_t gtt);

    /**
     * Flush a command stream.
     *
     * \param cs          A command stream to flush.
     * \param flags,      RADEON_FLUSH_ASYNC or 0.
     * \param fence       Pointer to a fence. If non-NULL, a fence is inserted
     *                    after the CS and is returned through this parameter.
     * \param cs_trace_id A unique identifier of the cs, used for tracing.
     */
    void (*cs_flush)(struct radeon_winsys_cs *cs,
                     unsigned flags,
                     struct pipe_fence_handle **fence,
                     uint32_t cs_trace_id);

    /**
     * Return TRUE if a buffer is referenced by a command stream.
     *
     * \param cs        A command stream.
     * \param buf       A winsys buffer.
     */
    boolean (*cs_is_buffer_referenced)(struct radeon_winsys_cs *cs,
                                       struct radeon_winsys_cs_handle *buf,
                                       enum radeon_bo_usage usage);

    /**
     * Request access to a feature for a command stream.
     *
     * \param cs        A command stream.
     * \param fid       Feature ID, one of RADEON_FID_*
     * \param enable    Whether to enable or disable the feature.
     */
    boolean (*cs_request_feature)(struct radeon_winsys_cs *cs,
                                  enum radeon_feature_id fid,
                                  boolean enable);
     /**
      * Make sure all asynchronous flush of the cs have completed
      *
      * \param cs        A command stream.
      */
    void (*cs_sync_flush)(struct radeon_winsys_cs *cs);

    /**
     * Wait for the fence and return true if the fence has been signalled.
     * The timeout of 0 will only return the status.
     * The timeout of PIPE_TIMEOUT_INFINITE will always wait until the fence
     * is signalled.
     */
    bool (*fence_wait)(struct radeon_winsys *ws,
                       struct pipe_fence_handle *fence,
                       uint64_t timeout);

    /**
     * Reference counting for fences.
     */
    void (*fence_reference)(struct pipe_fence_handle **dst,
                            struct pipe_fence_handle *src);

    /**
     * Initialize surface
     *
     * \param ws        The winsys this function is called from.
     * \param surf      Surface structure ptr
     */
    int (*surface_init)(struct radeon_winsys *ws,
                        struct radeon_surface *surf);

    /**
     * Find best values for a surface
     *
     * \param ws        The winsys this function is called from.
     * \param surf      Surface structure ptr
     */
    int (*surface_best)(struct radeon_winsys *ws,
                        struct radeon_surface *surf);

    uint64_t (*query_value)(struct radeon_winsys *ws,
                            enum radeon_value_id value);
};


static INLINE void radeon_emit(struct radeon_winsys_cs *cs, uint32_t value)
{
    cs->buf[cs->cdw++] = value;
}

static INLINE void radeon_emit_array(struct radeon_winsys_cs *cs,
				     const uint32_t *values, unsigned count)
{
    memcpy(cs->buf+cs->cdw, values, count * 4);
    cs->cdw += count;
}

#endif
