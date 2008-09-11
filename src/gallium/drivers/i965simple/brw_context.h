/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#ifndef BRWCONTEXT_INC
#define BRWCONTEXT_INC


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "tgsi/tgsi_scan.h"

#include "brw_structs.h"
#include "brw_winsys.h"


/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawining a thread.
 *
 * VUE - vertex URB entry.  An urb entry holding a vertex and usually
 * a vertex header.  The header contains control information and
 * things like primitive type, Begin/end flags and clip codes.
 *
 * PUE - primitive URB entry.  An urb entry produced by the setup (SF)
 * unit holding rasterization and interpolation parameters.
 *
 * GRF - general register file.  One of several register files
 * addressable by programmed threads.  The inputs (r0, payload, curbe,
 * urb) of the thread are preloaded to this area before the thread is
 * spawned.  The registers are individually 8 dwords wide and suitable
 * for general usage.  Registers holding thread input values are not
 * special and may be overwritten.
 *
 * MRF - message register file.  Threads communicate (and terminate)
 * by sending messages.  Message parameters are placed in contigous
 * MRF registers.  All program output is via these messages.  URB
 * entries are populated by sending a message to the shared URB
 * function containing the new data, together with a control word,
 * often an unmodified copy of R0.
 *
 * R0 - GRF register 0.  Typically holds control information used when
 * sending messages to other threads.
 *
 * EU or GEN4 EU: The name of the programmable subsystem of the
 * i965 hardware.  Threads are executed by the EU, the registers
 * described above are part of the EU architecture.
 *
 * Fixed function units:
 *
 * CS - Command streamer.  Notional first unit, little software
 * interaction.  Holds the URB entries used for constant data, ie the
 * CURBEs.
 *
 * VF/VS - Vertex Fetch / Vertex Shader.  The fixed function part of
 * this unit is responsible for pulling vertices out of vertex buffers
 * in vram and injecting them into the processing pipe as VUEs.  If
 * enabled, it first passes them to a VS thread which is a good place
 * for the driver to implement any active vertex shader.
 *
 * GS - Geometry Shader.  This corresponds to a new DX10 concept.  If
 * enabled, incoming strips etc are passed to GS threads in individual
 * line/triangle/point units.  The GS thread may perform arbitary
 * computation and emit whatever primtives with whatever vertices it
 * chooses.  This makes GS an excellent place to implement GL's
 * unfilled polygon modes, though of course it is capable of much
 * more.  Additionally, GS is used to translate away primitives not
 * handled by latter units, including Quads and Lineloops.
 *
 * CS - Clipper.  Mesa's clipping algorithms are imported to run on
 * this unit.  The fixed function part performs cliptesting against
 * the 6 fixed clipplanes and makes descisions on whether or not the
 * incoming primitive needs to be passed to a thread for clipping.
 * User clip planes are handled via cooperation with the VS thread.
 *
 * SF - Strips Fans or Setup: Triangles are prepared for
 * rasterization.  Interpolation coefficients are calculated.
 * Flatshading and two-side lighting usually performed here.
 *
 * WM - Windower.  Interpolation of vertex attributes performed here.
 * Fragment shader implemented here.  SIMD aspects of EU taken full
 * advantage of, as pixels are processed in blocks of 16.
 *
 * CC - Color Calculator.  No EU threads associated with this unit.
 * Handles blending and (presumably) depth and stencil testing.
 */

#define BRW_MAX_CURBE                    (32*16)

struct brw_context;
struct brw_winsys;


/* Raised when we receive new state across the pipe interface:
 */
#define BRW_NEW_VIEWPORT                0x1
#define BRW_NEW_RASTERIZER              0x2
#define BRW_NEW_FS                      0x4
#define BRW_NEW_BLEND                   0x8
#define BRW_NEW_CLIP                    0x10
#define BRW_NEW_SCISSOR                 0x20
#define BRW_NEW_STIPPLE                 0x40
#define BRW_NEW_FRAMEBUFFER             0x80
#define BRW_NEW_ALPHA_TEST              0x100
#define BRW_NEW_DEPTH_STENCIL           0x200
#define BRW_NEW_SAMPLER                 0x400
#define BRW_NEW_TEXTURE                 0x800
#define BRW_NEW_CONSTANTS               0x1000
#define BRW_NEW_VBO                     0x2000
#define BRW_NEW_VS                      0x4000

/* Raised for other internal events:
 */
#define BRW_NEW_URB_FENCE               0x10000
#define BRW_NEW_PSP                     0x20000
#define BRW_NEW_CURBE_OFFSETS           0x40000
#define BRW_NEW_REDUCED_PRIMITIVE       0x80000
#define BRW_NEW_PRIMITIVE               0x100000
#define BRW_NEW_SCENE                 0x200000
#define BRW_NEW_SF_LINKAGE              0x400000

extern int BRW_DEBUG;

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_PRIMS	0x8
#define DEBUG_VERTS	0x10
#define DEBUG_FALLBACKS	0x20
#define DEBUG_VERBOSE	0x40
#define DEBUG_DRI       0x80
#define DEBUG_DMA       0x100
#define DEBUG_SANITY    0x200
#define DEBUG_SYNC      0x400
#define DEBUG_SLEEP     0x800
#define DEBUG_PIXEL     0x1000
#define DEBUG_STATS     0x2000
#define DEBUG_TILE      0x4000
#define DEBUG_SINGLE_THREAD   0x8000
#define DEBUG_WM        0x10000
#define DEBUG_URB       0x20000
#define DEBUG_VS        0x40000
#define DEBUG_BATCH	0x80000
#define DEBUG_BUFMGR	0x100000
#define DEBUG_BLIT	0x200000
#define DEBUG_REGION	0x400000
#define DEBUG_MIPTREE	0x800000

#define DBG(...) do {						\
   if (BRW_DEBUG & FILE_DEBUG_FLAG)				\
      debug_printf(__VA_ARGS__);				\
} while(0)

#define PRINT(...) do {						\
   debug_printf(__VA_ARGS__);			                \
} while(0)

struct brw_state_flags {
   unsigned cache;
   unsigned brw;
};


struct brw_vertex_program {
   struct pipe_shader_state program;
   struct tgsi_shader_info info;
   int id;
};


struct brw_fragment_program {
   struct pipe_shader_state program;
   struct tgsi_shader_info info;
   
   boolean UsesDepth; /* XXX add this to tgsi_shader_info? */
   int id;
};


struct pipe_setup_linkage {
   struct {
      unsigned vp_output:5;
      unsigned interp_mode:4;
      unsigned bf_vp_output:5;
   } fp_input[PIPE_MAX_SHADER_INPUTS];

   unsigned fp_input_count:5;
   unsigned max_vp_output:5;
};
   


struct brw_texture {
   struct pipe_texture base;

   /* Derived from the above:
    */
   unsigned stride;
   unsigned depth_pitch;          /* per-image on i945? */
   unsigned total_nblocksy;

   unsigned nr_images[PIPE_MAX_TEXTURE_LEVELS];

   /* Explicitly store the offset of each image for each cube face or
    * depth value.  Pretty much have to accept that hardware formats
    * are going to be so diverse that there is no unified way to
    * compute the offsets of depth/cube images within a mipmap level,
    * so have to store them as a lookup table:
    */
   unsigned *image_offset[PIPE_MAX_TEXTURE_LEVELS];   /**< array [depth] of offsets */

   /* Includes image offset tables:
    */
   unsigned level_offset[PIPE_MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct pipe_buffer *buffer;
};

/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs:
 */
/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs:
 */

struct brw_wm_prog_data {
   unsigned curb_read_length;
   unsigned urb_read_length;

   unsigned first_curbe_grf;
   unsigned total_grf;
   unsigned total_scratch;

   /* Internally generated constants for the CURBE.  These are loaded
    * ahead of the data from the constant buffer.
    */
   const float internal_const[8];
   unsigned nr_internal_consts;
   unsigned max_const;

   boolean error;
};

struct brw_sf_prog_data {
   unsigned urb_read_length;
   unsigned total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   unsigned urb_entry_size;
};

struct brw_clip_prog_data {
   unsigned curb_read_length;	/* user planes? */
   unsigned clip_mode;
   unsigned urb_read_length;
   unsigned total_grf;
};

struct brw_gs_prog_data {
   unsigned urb_read_length;
   unsigned total_grf;
};

struct brw_vs_prog_data {
   unsigned curb_read_length;
   unsigned urb_read_length;
   unsigned total_grf;
   unsigned outputs_written;

   unsigned inputs_read;

   unsigned max_const;

   float    imm_buf[PIPE_MAX_CONSTANT][4];
   unsigned num_imm;
   unsigned num_consts;

   /* Used for calculating urb partitions:
    */
   unsigned urb_entry_size;
};


#define BRW_MAX_TEX_UNIT 8
#define BRW_WM_MAX_SURF BRW_MAX_TEX_UNIT + 1

/* Create a fixed sized struct for caching binding tables:
 */
struct brw_surface_binding_table {
   unsigned surf_ss_offset[BRW_WM_MAX_SURF];
};


struct brw_cache;

struct brw_mem_pool {
   struct pipe_buffer *buffer;

   unsigned size;
   unsigned offset;		/* offset of first free byte */

   struct brw_context *brw;
};

struct brw_cache_item {
   unsigned hash;
   unsigned key_size;		/* for variable-sized keys */
   const void *key;

   unsigned offset;		/* offset within pool's buffer */
   unsigned data_size;

   struct brw_cache_item *next;
};



struct brw_cache {
   unsigned id;

   const char *name;

   struct brw_context *brw;
   struct brw_mem_pool *pool;

   struct brw_cache_item **items;
   unsigned size, n_items;

   unsigned key_size;		/* for fixed-size keys */
   unsigned aux_size;

   unsigned last_addr;			/* offset of active item */
};




/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*update)( struct brw_context *brw );
};


/* Flags for brw->state.cache.
 */
#define CACHE_NEW_CC_VP                  (1<<BRW_CC_VP)
#define CACHE_NEW_CC_UNIT                (1<<BRW_CC_UNIT)
#define CACHE_NEW_WM_PROG                (1<<BRW_WM_PROG)
#define CACHE_NEW_SAMPLER_DEFAULT_COLOR  (1<<BRW_SAMPLER_DEFAULT_COLOR)
#define CACHE_NEW_SAMPLER                (1<<BRW_SAMPLER)
#define CACHE_NEW_WM_UNIT                (1<<BRW_WM_UNIT)
#define CACHE_NEW_SF_PROG                (1<<BRW_SF_PROG)
#define CACHE_NEW_SF_VP                  (1<<BRW_SF_VP)
#define CACHE_NEW_SF_UNIT                (1<<BRW_SF_UNIT)
#define CACHE_NEW_VS_UNIT                (1<<BRW_VS_UNIT)
#define CACHE_NEW_VS_PROG                (1<<BRW_VS_PROG)
#define CACHE_NEW_GS_UNIT                (1<<BRW_GS_UNIT)
#define CACHE_NEW_GS_PROG                (1<<BRW_GS_PROG)
#define CACHE_NEW_CLIP_VP                (1<<BRW_CLIP_VP)
#define CACHE_NEW_CLIP_UNIT              (1<<BRW_CLIP_UNIT)
#define CACHE_NEW_CLIP_PROG              (1<<BRW_CLIP_PROG)
#define CACHE_NEW_SURFACE                (1<<BRW_SS_SURFACE)
#define CACHE_NEW_SURF_BIND              (1<<BRW_SS_SURF_BIND)




enum brw_mempool_id {
   BRW_GS_POOL,
   BRW_SS_POOL,
   BRW_MAX_POOL
};


struct brw_cached_batch_item {
   struct header *header;
   unsigned sz;
   struct brw_cached_batch_item *next;
};



/* Protect against a future where PIPE_MAX_ATTRIBS > 32.  Wouldn't life
 * be easier if C allowed arrays of packed elements?
 */
#define ATTRIB_BIT_DWORDS  ((PIPE_MAX_ATTRIBS+31)/32)




struct brw_vertex_info {
   unsigned varying;  /* varying:1[PIPE_MAX_ATTRIBS] */
   unsigned sizes[ATTRIB_BIT_DWORDS * 2]; /* sizes:2[PIPE_MAX_ATTRIBS] */
};





struct brw_context
{
   struct pipe_context pipe;
   struct brw_winsys *winsys;

   unsigned primitive;
   unsigned reduced_primitive;

   boolean emit_state_always;

   struct {
      struct brw_state_flags dirty;
   } state;


   struct {
      const struct pipe_blend_state         *Blend;
      const struct pipe_depth_stencil_alpha_state *DepthStencil;
      const struct pipe_poly_stipple        *PolygonStipple;
      const struct pipe_rasterizer_state    *Raster;
      const struct pipe_sampler_state       *Samplers[PIPE_MAX_SAMPLERS];
      const struct brw_vertex_program       *VertexProgram;
      const struct brw_fragment_program     *FragmentProgram;

      struct pipe_clip_state          Clip;
      struct pipe_blend_color         BlendColor;
      struct pipe_scissor_state       Scissor;
      struct pipe_viewport_state      Viewport;
      struct pipe_framebuffer_state   FrameBuffer;

      const struct pipe_constant_buffer *Constants[2];
      const struct brw_texture          *Texture[PIPE_MAX_SAMPLERS];
   } attribs;

   unsigned num_samplers;
   unsigned num_textures;

   struct brw_mem_pool pool[BRW_MAX_POOL];
   struct brw_cache cache[BRW_MAX_CACHE];
   struct brw_cached_batch_item *cached_batch_items;

   struct {

      /* Arrays with buffer objects to copy non-bufferobj arrays into
       * for upload:
       */
      const struct pipe_vertex_buffer *vbo_array[PIPE_MAX_ATTRIBS];

      struct brw_vertex_element_state inputs[PIPE_MAX_ATTRIBS];

#define BRW_NR_UPLOAD_BUFS 17
#define BRW_UPLOAD_INIT_SIZE (128*1024)

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      struct brw_vertex_info info;
   } vb;


   unsigned hardware_dirty;
   unsigned dirty;
   unsigned pci_id;
   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      unsigned vsize;		/* vertex size plus header in urb registers */
      unsigned csize;		/* constant buffer size in urb registers */
      unsigned sfsize;		/* setup data size in urb registers */

      boolean constrained;

      unsigned nr_vs_entries;
      unsigned nr_gs_entries;
      unsigned nr_clip_entries;
      unsigned nr_sf_entries;
      unsigned nr_cs_entries;

/*       unsigned vs_size; */
/*       unsigned gs_size; */
/*       unsigned clip_size; */
/*       unsigned sf_size; */
/*       unsigned cs_size; */

      unsigned vs_start;
      unsigned gs_start;
      unsigned clip_start;
      unsigned sf_start;
      unsigned cs_start;
   } urb;


   /* BRW_NEW_CURBE_OFFSETS:
    */
   struct {
      unsigned wm_start;
      unsigned wm_size;
      unsigned clip_start;
      unsigned clip_size;
      unsigned vs_start;
      unsigned vs_size;
      unsigned total_size;

      unsigned gs_offset;

      float *last_buf;
      unsigned last_bufsz;
   } curbe;

   struct {
      struct brw_vs_prog_data *prog_data;

      unsigned prog_gs_offset;
      unsigned state_gs_offset;
   } vs;

   struct {
      struct brw_gs_prog_data *prog_data;

      boolean prog_active;
      unsigned prog_gs_offset;
      unsigned state_gs_offset;
   } gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      unsigned prog_gs_offset;
      unsigned vp_gs_offset;
      unsigned state_gs_offset;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      struct pipe_setup_linkage linkage;

      unsigned prog_gs_offset;
      unsigned vp_gs_offset;
      unsigned state_gs_offset;
   } sf;

   struct {
      struct brw_wm_prog_data *prog_data;

//      struct brw_wm_compiler *compile_data;


      /**
       * Array of sampler state uploaded at sampler_gs_offset of BRW_SAMPLER
       * cache
       */
      struct brw_sampler_state sampler[BRW_MAX_TEX_UNIT];

      unsigned render_surf;
      unsigned nr_surfaces;

      unsigned max_threads;
      struct pipe_buffer *scratch_buffer;
      unsigned scratch_buffer_size;

      unsigned sampler_count;
      unsigned sampler_gs_offset;

      struct brw_surface_binding_table bind;
      unsigned bind_ss_offset;

      unsigned prog_gs_offset;
      unsigned state_gs_offset;
   } wm;


   struct {
      unsigned vp_gs_offset;
      unsigned state_gs_offset;
   } cc;


   /* Used to give every program string a unique id
    */
   unsigned program_id;
};


#define BRW_PACKCOLOR8888(r,g,b,a)  ((r<<24) | (g<<16) | (b<<8) | a)


/*======================================================================
 * brw_vtbl.c
 */
void brw_do_flush( struct brw_context *brw,
		   unsigned flags );


/*======================================================================
 * brw_state.c
 */
void brw_validate_state(struct brw_context *brw);
void brw_init_state(struct brw_context *brw);
void brw_destroy_state(struct brw_context *brw);


/*======================================================================
 * brw_tex.c
 */
void brwUpdateTextureState( struct brw_context *brw );


/* brw_urb.c
 */
void brw_upload_urb_fence(struct brw_context *brw);

void brw_upload_constant_buffer_state(struct brw_context *brw);

void brw_init_surface_functions(struct brw_context *brw);
void brw_init_state_functions(struct brw_context *brw);
void brw_init_flush_functions(struct brw_context *brw);
void brw_init_string_functions(struct brw_context *brw);

/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static inline struct brw_context *
brw_context( struct pipe_context *ctx )
{
   return (struct brw_context *)ctx;
}

#endif

