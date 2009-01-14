
#ifndef COMMON_CONTEXT_H
#define COMMON_CONTEXT_H

#include "main/mm.h"
#include "math/m_vector.h"
#include "texmem.h"
#include "tnl/t_context.h"
#include "main/colormac.h"

#include "radeon_screen.h"
#include "radeon_drm.h"
#include "dri_util.h"

/* This union is used to avoid warnings/miscompilation
   with float to uint32_t casts due to strict-aliasing */
typedef union { GLfloat f; uint32_t ui32; } float_ui32_type;

struct radeon_context;
typedef struct radeon_context radeonContextRec;
typedef struct radeon_context *radeonContextPtr;


#define TEX_0   0x1
#define TEX_1   0x2
#define TEX_2   0x4
#define TEX_3	0x8
#define TEX_4	0x10
#define TEX_5	0x20

/* Rasterizing fallbacks */
/* See correponding strings in r200_swtcl.c */
#define RADEON_FALLBACK_TEXTURE		0x0001
#define RADEON_FALLBACK_DRAW_BUFFER	0x0002
#define RADEON_FALLBACK_STENCIL		0x0004
#define RADEON_FALLBACK_RENDER_MODE	0x0008
#define RADEON_FALLBACK_BLEND_EQ	0x0010
#define RADEON_FALLBACK_BLEND_FUNC	0x0020
#define RADEON_FALLBACK_DISABLE 	0x0040
#define RADEON_FALLBACK_BORDER_MODE	0x0080

#define R200_FALLBACK_TEXTURE           0x01
#define R200_FALLBACK_DRAW_BUFFER       0x02
#define R200_FALLBACK_STENCIL           0x04
#define R200_FALLBACK_RENDER_MODE       0x08
#define R200_FALLBACK_DISABLE           0x10
#define R200_FALLBACK_BORDER_MODE       0x20

/* The blit width for texture uploads
 */
#define BLIT_WIDTH_BYTES 1024

/* Use the templated vertex format:
 */
#define COLOR_IS_RGBA
#define TAG(x) radeon##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

struct radeon_colorbuffer_state {
	GLuint clear;
	int roundEnable;
	struct radeon_renderbuffer *rrb;
};

struct radeon_depthbuffer_state {
	GLuint clear;
	GLfloat scale;
	struct radeon_renderbuffer *rrb;
};

struct radeon_scissor_state {
	drm_clip_rect_t rect;
	GLboolean enabled;

	GLuint numClipRects;	/* Cliprects active */
	GLuint numAllocedClipRects;	/* Cliprects available */
	drm_clip_rect_t *pClipRects;
};

struct radeon_stencilbuffer_state {
	GLboolean hwBuffer;
	GLuint clear;		/* rb3d_stencilrefmask value */
};

struct radeon_stipple_state {
	GLuint mask[32];
};

struct radeon_state_atom {
	struct radeon_state_atom *next, *prev;
	const char *name;	/* for debug */
	int cmd_size;		/* size in bytes */
        GLuint idx;
	GLuint is_tcl;
        GLuint *cmd;		/* one or more cmd's */
	GLuint *lastcmd;		/* one or more cmd's */
	GLboolean dirty;	/* dirty-mark in emit_state_list */
        int (*check) (GLcontext *, struct radeon_state_atom *atom); /* is this state active? */
        void (*emit) (GLcontext *, struct radeon_state_atom *atom);
};

typedef struct radeon_tex_obj radeonTexObj, *radeonTexObjPtr;

/* Texture object in locally shared texture space.
 */
#ifndef RADEON_COMMON_FOR_R300
struct radeon_tex_obj {
	driTextureObject base;

	GLuint bufAddr;		/* Offset to start of locally
				   shared texture block */

	GLuint dirty_state;	/* Flags (1 per texunit) for
				   whether or not this texobj
				   has dirty hardware state
				   (pp_*) that needs to be
				   brought into the
				   texunit. */

	drm_radeon_tex_image_t image[6][RADEON_MAX_TEXTURE_LEVELS];
	/* Six, for the cube faces */

	GLboolean image_override; /* Image overridden by GLX_EXT_tfp */

	GLuint pp_txfilter;	/* hardware register values */
	GLuint pp_txformat;
        GLuint pp_txformat_x;
	GLuint pp_txoffset;	/* Image location in texmem.
				   All cube faces follow. */
	GLuint pp_txsize;	/* npot only */
	GLuint pp_txpitch;	/* npot only */
	GLuint pp_border_color;
	GLuint pp_cubic_faces;	/* cube face 1,2,3,4 log2 sizes */

	GLboolean border_fallback;

	GLuint tile_bits;	/* hw texture tile bits used on this texture */
};
#endif

/* Need refcounting on dma buffers:
 */
struct radeon_dma_buffer {
	int refcount;		/* the number of retained regions in buf */
	drmBufPtr buf;
};

/* A retained region, eg vertices for indexed vertices.
 */
struct radeon_dma_region {
   struct radeon_dma_buffer *buf;
   char *address;		/* == buf->address */
   int start, end, ptr;		/* offsets from start of buf */
   int aos_start;
   int aos_stride;
   int aos_size;
};


struct radeon_dma {
   /* Active dma region.  Allocations for vertices and retained
    * regions come from here.  Also used for emitting random vertices,
    * these may be flushed by calling flush_current();
    */
   struct radeon_dma_region current;
   
   void (*flush)( GLcontext *ctx );

   char *buf0_address;		/* start of buf[0], for index calcs */
   GLuint nr_released_bufs;	/* flush after so many buffers released */
};

struct radeon_ioctl {
	GLuint vertex_offset;
	GLuint vertex_size;
};

#define RADEON_MAX_PRIMS 64

struct radeon_prim {
	GLuint start;
	GLuint end;
	GLuint prim;
};

static INLINE GLuint radeonPackColor(GLuint cpp,
                                     GLubyte r, GLubyte g,
                                     GLubyte b, GLubyte a)
{
	switch (cpp) {
	case 2:
		return PACK_COLOR_565(r, g, b);
	case 4:
		return PACK_COLOR_8888(a, r, g, b);
	default:
		return 0;
	}
}

#define MAX_CMD_BUF_SZ (16*1024)

struct radeon_store {
	GLuint statenr;
	GLuint primnr;
	char cmd_buf[MAX_CMD_BUF_SZ];
	int cmd_used;
	int elts_start;
};

struct radeon_dri_mirror {
	__DRIcontextPrivate *context;	/* DRI context */
	__DRIscreenPrivate *screen;	/* DRI screen */

   /**
    * DRI drawable bound to this context for drawing.
    */
	__DRIdrawablePrivate *drawable;

   /**
    * DRI drawable bound to this context for reading.
    */
	__DRIdrawablePrivate *readable;

	drm_context_t hwContext;
	drm_hw_lock_t *hwLock;
	int fd;
	int drmMinor;
};

#define DEBUG_TEXTURE	0x001
#define DEBUG_STATE	0x002
#define DEBUG_IOCTL	0x004
#define DEBUG_PRIMS	0x008
#define DEBUG_VERTS	0x010
#define DEBUG_FALLBACKS	0x020
#define DEBUG_VFMT	0x040
#define DEBUG_CODEGEN	0x080
#define DEBUG_VERBOSE	0x100
#define DEBUG_DRI       0x200
#define DEBUG_DMA       0x400
#define DEBUG_SANITY    0x800
#define DEBUG_SYNC      0x1000
#define DEBUG_PIXEL     0x2000
#define DEBUG_MEMORY    0x4000



typedef void (*radeon_tri_func) (radeonContextPtr,
				 radeonVertex *,
				 radeonVertex *, radeonVertex *);

typedef void (*radeon_line_func) (radeonContextPtr,
				  radeonVertex *, radeonVertex *);

typedef void (*radeon_point_func) (radeonContextPtr, radeonVertex *);

struct radeon_state {
	struct radeon_colorbuffer_state color;
	struct radeon_depthbuffer_state depth;
	struct radeon_scissor_state scissor;
	struct radeon_stencilbuffer_state stencil;
};

/**
 * This structure holds the command buffer while it is being constructed.
 *
 * The first batch of commands in the buffer is always the state that needs
 * to be re-emitted when the context is lost. This batch can be skipped
 * otherwise.
 */
struct radeon_cmdbuf {
	struct radeon_cs_manager    *csm;
	struct radeon_cs            *cs;
	int size; /** # of dwords total */
	unsigned int flushing:1; /** whether we're currently in FlushCmdBufLocked */
};

struct radeon_context {
   GLcontext *glCtx;
   radeonScreenPtr radeonScreen;	/* Screen private DRI data */
  
   /* Texture object bookkeeping
    */
   unsigned              nr_heaps;
   driTexHeap          * texture_heaps[ RADEON_NR_TEX_HEAPS ];
   driTextureObject      swapped;
   int                   texture_depth;
   float                 initialMaxAnisotropy;

   /* Rasterization and vertex state:
    */
   GLuint TclFallback;
   GLuint Fallback;
   GLuint NewGLState;
   DECLARE_RENDERINPUTS(tnl_index_bitset);	/* index of bits for last tnl_install_attrs */

   /* Page flipping */
   GLuint doPageFlip;

   /* Drawable, cliprect and scissor information */
   GLuint numClipRects;	/* Cliprects for the draw buffer */
   drm_clip_rect_t *pClipRects;
   unsigned int lastStamp;
   GLboolean lost_context;
   drm_radeon_sarea_t *sarea;	/* Private SAREA data */

   /* Mirrors of some DRI state */
   struct radeon_dri_mirror dri;

   /* Busy waiting */
   GLuint do_usleeps;
   GLuint do_irqs;
   GLuint irqsEmitted;
   drm_radeon_irq_wait_t iw;

   /* buffer swap */
   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;

   /* Derived state - for r300 only */
   struct radeon_state state;

   /* Configuration cache
    */
   driOptionCache optionCache;

   struct radeon_cmdbuf cmdbuf;

   struct {
      void (*get_lock)(radeonContextPtr radeon);
      void (*update_viewport_offset)(GLcontext *ctx);
      void (*flush)(GLcontext *ctx);
      void (*set_all_dirty)(GLcontext *ctx);
      void (*update_draw_buffer)(GLcontext *ctx);
      void (*emit_cs_header)(struct radeon_cs *cs, radeonContextPtr rmesa);
   } vtbl;
};

#define RADEON_CONTEXT(glctx) ((radeonContextPtr)(ctx->DriverCtx))

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int RADEON_DEBUG;
#else
#define RADEON_DEBUG		0
#endif

#endif
