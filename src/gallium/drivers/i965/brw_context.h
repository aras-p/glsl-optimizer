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

#include "brw_structs.h"
#include "brw_winsys.h"
#include "brw_reg.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "tgsi/tgsi_scan.h"


/* Glossary:
 *
 * URB - uniform resource buffer.  A mid-sized buffer which is
 * partitioned between the fixed function units and used for passing
 * values (vertices, primitives, constants) between them.
 *
 * CURBE - constant URB entry.  An urb region (entry) used to hold
 * constant values which the fixed function units can be instructed to
 * preload into the GRF when spawning a thread.
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
 * by sending messages.  Message parameters are placed in contiguous
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
 * the 6 fixed clipplanes and makes decisions on whether or not the
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


/* Need a value to say a particular vertex shader output isn't
 * present.  Limits us to 63 outputs currently.
 */
#define BRW_OUTPUT_NOT_PRESENT           ((1<<6)-1)


struct brw_context;

struct brw_depth_stencil_state {
   /* Precalculated hardware state:
    */
   struct brw_cc0 cc0;
   struct brw_cc1 cc1;
   struct brw_cc2 cc2;
   struct brw_cc3 cc3;
   struct brw_cc7 cc7;

   unsigned iz_lookup;
};


struct brw_blend_state {
   /* Precalculated hardware state:
    */
   struct brw_cc2 cc2;
   struct brw_cc3 cc3;
   struct brw_cc5 cc5;
   struct brw_cc6 cc6;

   struct brw_surf_ss0 ss0;
};

struct brw_rasterizer_state;

struct brw_immediate_data {
   unsigned nr;
   float (*data)[4];
};

struct brw_vertex_shader {
   const struct tgsi_token *tokens;
   struct brw_winsys_buffer *const_buffer;    /** Program constant buffer/surface */

   struct tgsi_shader_info info;
   struct brw_immediate_data immediates;

   GLuint has_flow_control:1;
   GLuint use_const_buffer:1;

   /* Offsets of special vertex shader outputs required for clipping.
    */
   GLuint output_hpos:6;        /* not always zero? */
   GLuint output_color0:6;
   GLuint output_color1:6;
   GLuint output_bfc0:6;
   GLuint output_bfc1:6;
   GLuint output_edgeflag:6;

   unsigned id;
};

struct brw_fs_signature {
   GLuint nr_inputs;
   struct {
      GLuint interp:3;          /* TGSI_INTERPOLATE_x */
      GLuint semantic:5;        /* TGSI_SEMANTIC_x */
      GLuint semantic_index:24;
   } input[PIPE_MAX_SHADER_INPUTS];
};

#define brw_fs_signature_size(s) (offsetof(struct brw_fs_signature, input) + \
                                  ((s)->nr_inputs * sizeof (s)->input[0])) 


struct brw_fragment_shader {
   const struct tgsi_token *tokens;
   struct tgsi_shader_info info;

   struct brw_fs_signature signature;
   struct brw_immediate_data immediates;

   unsigned iz_lookup;
   /*unsigned wm_lookup;*/
   
   unsigned  uses_depth:1;
   unsigned  has_flow_control:1;

   unsigned id;
   struct brw_winsys_buffer *const_buffer;    /** Program constant buffer/surface */
   GLboolean use_const_buffer;
};


struct brw_sampler {
   struct brw_ss0 ss0;
   struct brw_ss1 ss1;
   float border_color[4];
   struct brw_ss3 ss3;
};



#define PIPE_NEW_DEPTH_STENCIL_ALPHA    0x1
#define PIPE_NEW_RAST                   0x2
#define PIPE_NEW_BLEND                  0x4
#define PIPE_NEW_VIEWPORT               0x8
#define PIPE_NEW_SAMPLERS               0x10
#define PIPE_NEW_VERTEX_BUFFER          0x20
#define PIPE_NEW_VERTEX_ELEMENT         0x40
#define PIPE_NEW_FRAGMENT_SHADER        0x80
#define PIPE_NEW_VERTEX_SHADER          0x100
#define PIPE_NEW_FRAGMENT_CONSTANTS     0x200
#define PIPE_NEW_VERTEX_CONSTANTS       0x400
#define PIPE_NEW_CLIP                   0x800
#define PIPE_NEW_INDEX_BUFFER           0x1000
#define PIPE_NEW_INDEX_RANGE            0x2000
#define PIPE_NEW_BLEND_COLOR            0x4000
#define PIPE_NEW_POLYGON_STIPPLE        0x8000
#define PIPE_NEW_FRAMEBUFFER_DIMENSIONS 0x10000
#define PIPE_NEW_DEPTH_BUFFER           0x20000
#define PIPE_NEW_COLOR_BUFFERS          0x40000
#define PIPE_NEW_QUERY                  0x80000
#define PIPE_NEW_SCISSOR                0x100000
#define PIPE_NEW_BOUND_TEXTURES         0x200000
#define PIPE_NEW_NR_CBUFS               0x400000
#define PIPE_NEW_FRAGMENT_SIGNATURE     0x800000



#define BRW_NEW_URB_FENCE               0x1
#define BRW_NEW_FRAGMENT_PROGRAM        0x2
#define BRW_NEW_VERTEX_PROGRAM          0x4
#define BRW_NEW_INPUT_DIMENSIONS        0x8
#define BRW_NEW_CURBE_OFFSETS           0x10
#define BRW_NEW_REDUCED_PRIMITIVE       0x20
#define BRW_NEW_PRIMITIVE               0x40
#define BRW_NEW_CONTEXT                 0x80
#define BRW_NEW_WM_INPUT_DIMENSIONS     0x100
#define BRW_NEW_PSP                     0x800
#define BRW_NEW_WM_SURFACES		0x1000
#define BRW_NEW_xxx                     0x2000 /* was FENCE */
#define BRW_NEW_INDICES			0x4000

/**
 * Used for any batch entry with a relocated pointer that will be used
 * by any 3D rendering.  Need to re-emit these fresh in each
 * batchbuffer as the referenced buffers may be relocated in the
 * meantime.
 */
#define BRW_NEW_BATCH			0x10000
#define BRW_NEW_NR_WM_SURFACES		0x40000
#define BRW_NEW_NR_VS_SURFACES		0x80000
#define BRW_NEW_INDEX_BUFFER		0x100000

struct brw_state_flags {
   /** State update flags signalled by mesa internals */
   GLuint mesa;
   /**
    * State update flags signalled as the result of brw_tracked_state updates
    */
   GLuint brw;
   /** State update flags signalled by brw_state_cache.c searches */
   GLuint cache;
};



/* Data about a particular attempt to compile a program.  Note that
 * there can be many of these, each in a different GL state
 * corresponding to a different brw_wm_prog_key struct, with different
 * compiled programs:
 */
struct brw_wm_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;

   GLuint first_curbe_grf;
   GLuint total_grf;
   GLuint total_scratch;

   GLuint nr_params;       /**< number of float params/constants */
   GLboolean error;

   /* Pointer to tracked values (only valid once
    * _mesa_load_state_parameters has been called at runtime).
    */
   const GLfloat *param[BRW_MAX_CURBE];
};

struct brw_sf_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;

   /* Each vertex may have upto 12 attributes, 4 components each,
    * except WPOS which requires only 2.  (11*4 + 2) == 44 ==> 11
    * rows.
    *
    * Actually we use 4 for each, so call it 12 rows.
    */
   GLuint urb_entry_size;
};


struct brw_clip_prog_data;

struct brw_gs_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_vs_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;
   GLuint total_grf;

   GLuint nr_outputs;
   GLuint nr_inputs;

   GLuint nr_params;       /**< number of TGSI_FILE_CONSTANT's */

   GLboolean writes_psiz;

   /* Used for calculating urb partitions:
    */
   GLuint urb_entry_size;
};


/* Size == 0 if output either not written, or always [0,0,0,1]
 */
struct brw_vs_ouput_sizes {
   GLubyte output_size[PIPE_MAX_SHADER_OUTPUTS];
};


/** Number of texture sampler units */
#define BRW_MAX_TEX_UNIT 16

/** Max number of render targets in a shader */
#define BRW_MAX_DRAW_BUFFERS 4

/**
 * Size of our surface binding table for the WM.
 * This contains pointers to the drawing surfaces and current texture
 * objects and shader constant buffers (+2).
 */
#define BRW_WM_MAX_SURF (BRW_MAX_DRAW_BUFFERS + BRW_MAX_TEX_UNIT + 1)

/**
 * Helpers to convert drawing buffers, textures and constant buffers
 * to surface binding table indexes, for WM.
 */
#define BTI_COLOR_BUF(d)          (d)
#define BTI_FRAGMENT_CONSTANTS    (BRW_MAX_DRAW_BUFFERS) 
#define BTI_TEXTURE(t)            (BRW_MAX_DRAW_BUFFERS + 1 + (t))

/**
 * Size of surface binding table for the VS.
 * Only one constant buffer for now.
 */
#define BRW_VS_MAX_SURF 1

/**
 * Only a VS constant buffer
 */
#define SURF_INDEX_VERT_CONST_BUFFER 0


/* Bit of a hack to align these with the winsys buffer_data_type enum.
 */
enum brw_cache_id {
   BRW_CC_VP         = BRW_DATA_GS_CC_VP,
   BRW_CC_UNIT       = BRW_DATA_GS_CC_UNIT,
   BRW_WM_PROG       = BRW_DATA_GS_WM_PROG,
   BRW_SAMPLER_DEFAULT_COLOR    = BRW_DATA_GS_SAMPLER_DEFAULT_COLOR,
   BRW_SAMPLER       = BRW_DATA_GS_SAMPLER,
   BRW_WM_UNIT       = BRW_DATA_GS_WM_UNIT,
   BRW_SF_PROG       = BRW_DATA_GS_SF_PROG,
   BRW_SF_VP         = BRW_DATA_GS_SF_VP,
   BRW_SF_UNIT       = BRW_DATA_GS_SF_UNIT,
   BRW_VS_UNIT       = BRW_DATA_GS_VS_UNIT,
   BRW_VS_PROG       = BRW_DATA_GS_VS_PROG,
   BRW_GS_UNIT       = BRW_DATA_GS_GS_UNIT,
   BRW_GS_PROG       = BRW_DATA_GS_GS_PROG,
   BRW_CLIP_VP       = BRW_DATA_GS_CLIP_VP,
   BRW_CLIP_UNIT     = BRW_DATA_GS_CLIP_UNIT,
   BRW_CLIP_PROG     = BRW_DATA_GS_CLIP_PROG,
   BRW_SS_SURFACE    = BRW_DATA_SS_SURFACE,
   BRW_SS_SURF_BIND  = BRW_DATA_SS_SURF_BIND,

   BRW_MAX_CACHE
};

struct brw_cache_item {
   /**
    * Effectively part of the key, cache_id identifies what kind of state
    * buffer is involved, and also which brw->state.dirty.cache flag should
    * be set when this cache item is chosen.
    */
   enum brw_cache_id cache_id;
   /** 32-bit hash of the key data */
   GLuint hash;
   GLuint key_size;		/* for variable-sized keys */
   const void *key;
   struct brw_winsys_reloc *relocs;
   GLuint nr_relocs;

   struct brw_winsys_buffer *bo;
   GLuint data_size;

   struct brw_cache_item *next;
};   



struct brw_cache {
   struct brw_context *brw;
   struct brw_winsys_screen *sws;

   struct brw_cache_item **items;
   GLuint size, n_items;

   enum brw_buffer_type buffer_type;

   GLuint key_size[BRW_MAX_CACHE];		/* for fixed-size keys */
   GLuint aux_size[BRW_MAX_CACHE];
   char *name[BRW_MAX_CACHE];
   

   /* Record of the last BOs chosen for each cache_id.  Used to set
    * brw->state.dirty.cache when a new cache item is chosen.
    */
   struct brw_winsys_buffer *last_bo[BRW_MAX_CACHE];
};


struct brw_tracked_state {
   struct brw_state_flags dirty;
   int (*prepare)( struct brw_context *brw );
   int (*emit)( struct brw_context *brw );
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

struct brw_cached_batch_item {
   struct header *header;
   GLuint sz;
   struct brw_cached_batch_item *next;
};
   


/* Protect against a future where VERT_ATTRIB_MAX > 32.  Wouldn't life
 * be easier if C allowed arrays of packed elements?
 */
#define VS_INPUT_BITMASK_DWORDS  ((PIPE_MAX_SHADER_INPUTS+31)/32)




struct brw_vertex_info {
   GLuint sizes[VS_INPUT_BITMASK_DWORDS * 2]; /* sizes:2[VERT_ATTRIB_MAX] */
};


struct brw_query_object {
   /** Doubly linked list of active query objects in the context. */
   struct brw_query_object *prev, *next;

   /** Last query BO associated with this query. */
   struct brw_winsys_buffer *bo;
   /** First index in bo with query data for this object. */
   int first_index;
   /** Last index in bo with query data for this object. */
   int last_index;

   /* Total count of pixels from previous BOs */
   uint64_t result;
};

#define CC_RELOC_VP 0


/**
 * brw_context is derived from pipe_context
 */
struct brw_context 
{
   struct pipe_context base;
   struct brw_chipset chipset;

   struct brw_winsys_screen *sws;

   struct brw_batchbuffer *batch;

   GLuint primitive;
   GLuint reduced_primitive;

   /* Active state from the state tracker: 
    */
   struct {
      struct brw_vertex_shader *vertex_shader;
      struct brw_fragment_shader *fragment_shader;
      const struct brw_blend_state *blend;
      const struct brw_rasterizer_state *rast;
      const struct brw_depth_stencil_state *zstencil;

      const struct brw_sampler *sampler[PIPE_MAX_SAMPLERS];
      unsigned num_samplers;

      struct pipe_texture *texture[PIPE_MAX_SAMPLERS];
      struct pipe_vertex_buffer vertex_buffer[PIPE_MAX_ATTRIBS];
      struct pipe_vertex_element vertex_element[PIPE_MAX_ATTRIBS];
      unsigned num_vertex_elements;
      unsigned num_textures;
      unsigned num_vertex_buffers;

      struct pipe_scissor_state scissor;
      struct pipe_viewport_state viewport;
      struct pipe_stencil_ref stencil_ref;
      struct pipe_framebuffer_state fb;
      struct pipe_clip_state ucp;
      struct pipe_buffer *vertex_constants;
      struct pipe_buffer *fragment_constants;

      struct brw_blend_constant_color bcc;
      struct brw_cc1 cc1_stencil_ref;
      struct brw_polygon_stipple bps;
      struct brw_cc_viewport ccv;

      /**
       * Index buffer for this draw_prims call.
       *
       * Updates are signaled by PIPE_NEW_INDEX_BUFFER.
       */
      struct pipe_buffer *index_buffer;
      unsigned index_size;

      /* Updates are signalled by PIPE_NEW_INDEX_RANGE:
       */
      unsigned min_index;
      unsigned max_index;

   } curr;

   struct {
      struct brw_state_flags dirty;

      /**
       * List of buffers accumulated in brw_validate_state to receive
       * dri_bo_check_aperture treatment before exec, so we can know if we
       * should flush the batch and try again before emitting primitives.
       *
       * This can be a fixed number as we only have a limited number of
       * objects referenced from the batchbuffer in a primitive emit,
       * consisting of the vertex buffers, pipelined state pointers,
       * the CURBE, the depth buffer, and a query BO.
       */
      struct brw_winsys_buffer *validated_bos[PIPE_MAX_SHADER_INPUTS + 16];
      int validated_bo_count;
   } state;

   struct brw_cache cache;  /** non-surface items */
   struct brw_cache surface_cache;  /* surface items */
   struct brw_cached_batch_item *cached_batch_items;

   struct {
      struct u_upload_mgr *upload_vertex;
      struct u_upload_mgr *upload_index;
      
      /* Information on uploaded vertex buffers:
       */
      struct {
	 unsigned stride;	/* in bytes between successive vertices */
	 unsigned offset;	/* in bytes, of first vertex in bo */
	 unsigned vertex_count;	/* count of valid vertices which may be accessed */
	 struct brw_winsys_buffer *bo;
      } vb[PIPE_MAX_ATTRIBS];

      unsigned nr_vb;		/* currently the same as curr.num_vertex_buffers */
   } vb;

   struct {
      /* Updates to these fields are signaled by BRW_NEW_INDEX_BUFFER. */
      struct brw_winsys_buffer *bo;
      unsigned int offset;
      unsigned int size;
      /* Offset to index buffer index to use in CMD_3D_PRIM so that we can
       * avoid re-uploading the IB packet over and over if we're actually
       * referencing the same index buffer.
       */
      unsigned int start_vertex_offset;
   } ib;


   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;		/* vertex size plus header in urb registers */
      GLuint csize;		/* constant buffer size in urb registers */
      GLuint sfsize;		/* setup data size in urb registers */

      GLboolean constrained;

      GLuint nr_vs_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

      GLuint vs_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
   } urb;

   
   /* BRW_NEW_CURBE_OFFSETS: 
    */
   struct {
      GLuint wm_start;  /**< pos of first wm const in CURBE buffer */
      GLuint wm_size;   /**< number of float[4] consts, multiple of 16 */
      GLuint clip_start;
      GLuint clip_size;
      GLuint vs_start;
      GLuint vs_size;
      GLuint total_size;

      struct brw_winsys_buffer *curbe_bo;
      /** Offset within curbe_bo of space for current curbe entry */
      GLuint curbe_offset;
      /** Offset within curbe_bo of space for next curbe entry */
      GLuint curbe_next_offset;

      GLfloat *last_buf;
      GLuint last_bufsz;
      /**
       *  Whether we should create a new bo instead of reusing the old one
       * (if we just dispatch the batch pointing at the old one.
       */
      GLboolean need_new_bo;
   } curbe;

   struct {
      struct brw_vs_prog_data *prog_data;

      struct brw_winsys_buffer *prog_bo;
      struct brw_winsys_buffer *state_bo;

      /** Binding table of pointers to surf_bo entries */
      struct brw_winsys_buffer *bind_bo;
      struct brw_winsys_buffer *surf_bo[BRW_VS_MAX_SURF];
      GLuint nr_surfaces;      
   } vs;

   struct {
      struct brw_gs_prog_data *prog_data;

      GLboolean prog_active;
      struct brw_winsys_buffer *prog_bo;
      struct brw_winsys_buffer *state_bo;
   } gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      struct brw_winsys_buffer *prog_bo;
      struct brw_winsys_buffer *state_bo;
      struct brw_winsys_buffer *vp_bo;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      struct brw_winsys_buffer *prog_bo;
      struct brw_winsys_buffer *state_bo;
      struct brw_winsys_buffer *vp_bo;
   } sf;

   struct {
      struct brw_wm_prog_data *prog_data;
      struct brw_wm_compile *compile_data;

      /** Input sizes, calculated from active vertex program.
       * One bit per fragment program input attribute.
       */
      /*GLbitfield input_size_masks[4];*/

      /** Array of surface default colors (texture border color) */
      struct brw_winsys_buffer *sdc_bo[BRW_MAX_TEX_UNIT];

      GLuint render_surf;
      GLuint nr_surfaces;      

      GLuint max_threads;
      struct brw_winsys_buffer *scratch_bo;

      GLuint sampler_count;
      struct brw_winsys_buffer *sampler_bo;

      /** Binding table of pointers to surf_bo entries */
      struct brw_winsys_buffer *bind_bo;
      struct brw_winsys_buffer *surf_bo[BRW_WM_MAX_SURF];

      struct brw_winsys_buffer *prog_bo;
      struct brw_winsys_buffer *state_bo;
   } wm;


   struct {
      struct brw_winsys_buffer *state_bo;

      struct brw_cc_unit_state cc;
      struct brw_winsys_reloc reloc[1];
   } cc;

   struct {
      struct brw_query_object active_head;
      struct brw_winsys_buffer *bo;
      int index;
      GLboolean active;
      int stats_wm;
   } query;

   struct {
      unsigned always_emit_state:1;
      unsigned always_flush_batch:1;
      unsigned force_swtnl:1;
      unsigned no_swtnl:1;
   } flags;

   /* Used to give every program string a unique id
    */
   GLuint program_id;
};



/*======================================================================
 * brw_queryobj.c
 */
void brw_init_query(struct brw_context *brw);
enum pipe_error brw_prepare_query_begin(struct brw_context *brw);
void brw_emit_query_begin(struct brw_context *brw);
void brw_emit_query_end(struct brw_context *brw);

/*======================================================================
 * brw_state_dump.c
 */
void brw_debug_batch(struct brw_context *intel);


/*======================================================================
 * brw_pipe_*.c
 */
void brw_pipe_blend_init( struct brw_context *brw );
void brw_pipe_depth_stencil_init( struct brw_context *brw );
void brw_pipe_framebuffer_init( struct brw_context *brw );
void brw_pipe_flush_init( struct brw_context *brw );
void brw_pipe_misc_init( struct brw_context *brw );
void brw_pipe_query_init( struct brw_context *brw );
void brw_pipe_rast_init( struct brw_context *brw );
void brw_pipe_sampler_init( struct brw_context *brw );
void brw_pipe_shader_init( struct brw_context *brw );
void brw_pipe_vertex_init( struct brw_context *brw );
void brw_pipe_clear_init( struct brw_context *brw );


void brw_pipe_blend_cleanup( struct brw_context *brw );
void brw_pipe_depth_stencil_cleanup( struct brw_context *brw );
void brw_pipe_framebuffer_cleanup( struct brw_context *brw );
void brw_pipe_flush_cleanup( struct brw_context *brw );
void brw_pipe_misc_cleanup( struct brw_context *brw );
void brw_pipe_query_cleanup( struct brw_context *brw );
void brw_pipe_rast_cleanup( struct brw_context *brw );
void brw_pipe_sampler_cleanup( struct brw_context *brw );
void brw_pipe_shader_cleanup( struct brw_context *brw );
void brw_pipe_vertex_cleanup( struct brw_context *brw );
void brw_pipe_clear_cleanup( struct brw_context *brw );

void brw_hw_cc_init( struct brw_context *brw );
void brw_hw_cc_cleanup( struct brw_context *brw );



void brw_context_flush( struct brw_context *brw );


/* brw_urb.c
 */
int brw_upload_urb_fence(struct brw_context *brw);

/* brw_curbe.c
 */
int brw_upload_cs_urb_state(struct brw_context *brw);

/* brw_context.c
 */
struct pipe_context *brw_create_context(struct pipe_screen *screen,
					void *priv);

/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct brw_context *
brw_context( struct pipe_context *ctx )
{
   return (struct brw_context *)ctx;
}


#define BRW_IS_965(brw)    ((brw)->chipset.is_965)
#define BRW_IS_IGDNG(brw)  ((brw)->chipset.is_igdng)
#define BRW_IS_G4X(brw)    ((brw)->chipset.is_g4x)


#endif

