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

#include "intel_context.h"
#include "brw_structs.h"
#include "main/imports.h"


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

enum brw_state_id {
   BRW_STATE_URB_FENCE,
   BRW_STATE_FRAGMENT_PROGRAM,
   BRW_STATE_VERTEX_PROGRAM,
   BRW_STATE_INPUT_DIMENSIONS,
   BRW_STATE_CURBE_OFFSETS,
   BRW_STATE_REDUCED_PRIMITIVE,
   BRW_STATE_PRIMITIVE,
   BRW_STATE_CONTEXT,
   BRW_STATE_WM_INPUT_DIMENSIONS,
   BRW_STATE_PSP,
   BRW_STATE_WM_SURFACES,
   BRW_STATE_VS_BINDING_TABLE,
   BRW_STATE_GS_BINDING_TABLE,
   BRW_STATE_PS_BINDING_TABLE,
   BRW_STATE_INDICES,
   BRW_STATE_VERTICES,
   BRW_STATE_BATCH,
   BRW_STATE_NR_WM_SURFACES,
   BRW_STATE_NR_VS_SURFACES,
   BRW_STATE_INDEX_BUFFER,
   BRW_STATE_VS_CONSTBUF,
   BRW_STATE_WM_CONSTBUF,
   BRW_STATE_PROGRAM_CACHE,
   BRW_STATE_STATE_BASE_ADDRESS,
};

#define BRW_NEW_URB_FENCE               (1 << BRW_STATE_URB_FENCE)
#define BRW_NEW_FRAGMENT_PROGRAM        (1 << BRW_STATE_FRAGMENT_PROGRAM)
#define BRW_NEW_VERTEX_PROGRAM          (1 << BRW_STATE_VERTEX_PROGRAM)
#define BRW_NEW_INPUT_DIMENSIONS        (1 << BRW_STATE_INPUT_DIMENSIONS)
#define BRW_NEW_CURBE_OFFSETS           (1 << BRW_STATE_CURBE_OFFSETS)
#define BRW_NEW_REDUCED_PRIMITIVE       (1 << BRW_STATE_REDUCED_PRIMITIVE)
#define BRW_NEW_PRIMITIVE               (1 << BRW_STATE_PRIMITIVE)
#define BRW_NEW_CONTEXT                 (1 << BRW_STATE_CONTEXT)
#define BRW_NEW_WM_INPUT_DIMENSIONS     (1 << BRW_STATE_WM_INPUT_DIMENSIONS)
#define BRW_NEW_PSP                     (1 << BRW_STATE_PSP)
#define BRW_NEW_WM_SURFACES		(1 << BRW_STATE_WM_SURFACES)
#define BRW_NEW_VS_BINDING_TABLE	(1 << BRW_STATE_VS_BINDING_TABLE)
#define BRW_NEW_GS_BINDING_TABLE	(1 << BRW_STATE_GS_BINDING_TABLE)
#define BRW_NEW_PS_BINDING_TABLE	(1 << BRW_STATE_PS_BINDING_TABLE)
#define BRW_NEW_INDICES			(1 << BRW_STATE_INDICES)
#define BRW_NEW_VERTICES		(1 << BRW_STATE_VERTICES)
/**
 * Used for any batch entry with a relocated pointer that will be used
 * by any 3D rendering.
 */
#define BRW_NEW_BATCH                  (1 << BRW_STATE_BATCH)
/** \see brw.state.depth_region */
#define BRW_NEW_NR_WM_SURFACES         (1 << BRW_STATE_NR_WM_SURFACES)
#define BRW_NEW_NR_VS_SURFACES         (1 << BRW_STATE_NR_VS_SURFACES)
#define BRW_NEW_INDEX_BUFFER           (1 << BRW_STATE_INDEX_BUFFER)
#define BRW_NEW_VS_CONSTBUF            (1 << BRW_STATE_VS_CONSTBUF)
#define BRW_NEW_WM_CONSTBUF            (1 << BRW_STATE_WM_CONSTBUF)
#define BRW_NEW_PROGRAM_CACHE		(1 << BRW_STATE_PROGRAM_CACHE)
#define BRW_NEW_STATE_BASE_ADDRESS	(1 << BRW_STATE_STATE_BASE_ADDRESS)

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


/** Subclass of Mesa vertex program */
struct brw_vertex_program {
   struct gl_vertex_program program;
   GLuint id;
   GLboolean use_const_buffer;
};


/** Subclass of Mesa fragment program */
struct brw_fragment_program {
   struct gl_fragment_program program;
   GLuint id;  /**< serial no. to identify frag progs, never re-used */

   /** for debugging, which texture units are referenced */
   GLbitfield tex_units_used;
};

struct brw_shader {
   struct gl_shader base;

   /** Shader IR transformed for native compile, at link time. */
   struct exec_list *ir;
};

struct brw_shader_program {
   struct gl_shader_program base;
};

enum param_conversion {
   PARAM_NO_CONVERT,
   PARAM_CONVERT_F2I,
   PARAM_CONVERT_F2U,
   PARAM_CONVERT_F2B,
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
   GLuint first_curbe_grf_16;
   GLuint reg_blocks;
   GLuint reg_blocks_16;
   GLuint total_scratch;

   GLuint nr_params;       /**< number of float params/constants */
   GLuint nr_pull_params;
   GLboolean error;
   int dispatch_width;
   uint32_t prog_offset_16;

   /* Pointer to tracked values (only valid once
    * _mesa_load_state_parameters has been called at runtime).
    */
   const float *param[MAX_UNIFORMS * 4]; /* should be: BRW_MAX_CURBE */
   enum param_conversion param_convert[MAX_UNIFORMS * 4];
   const float *pull_param[MAX_UNIFORMS * 4];
   enum param_conversion pull_param_convert[MAX_UNIFORMS * 4];
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

struct brw_clip_prog_data {
   GLuint curb_read_length;	/* user planes? */
   GLuint clip_mode;
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_gs_prog_data {
   GLuint urb_read_length;
   GLuint total_grf;
};

struct brw_vs_prog_data {
   GLuint curb_read_length;
   GLuint urb_read_length;
   GLuint total_grf;
   GLbitfield64 outputs_written;
   GLuint nr_params;       /**< number of float params/constants */

   GLuint inputs_read;

   /* Used for calculating urb partitions:
    */
   GLuint urb_entry_size;
};


/* Size == 0 if output either not written, or always [0,0,0,1]
 */
struct brw_vs_ouput_sizes {
   GLubyte output_size[VERT_RESULT_MAX];
};


/** Number of texture sampler units */
#define BRW_MAX_TEX_UNIT 16

/** Max number of render targets in a shader */
#define BRW_MAX_DRAW_BUFFERS 8

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
#define SURF_INDEX_DRAW(d)           (d)
#define SURF_INDEX_FRAG_CONST_BUFFER (BRW_MAX_DRAW_BUFFERS) 
#define SURF_INDEX_TEXTURE(t)        (BRW_MAX_DRAW_BUFFERS + 1 + (t))

/**
 * Size of surface binding table for the VS.
 * Only one constant buffer for now.
 */
#define BRW_VS_MAX_SURF 1

/**
 * Only a VS constant buffer
 */
#define SURF_INDEX_VERT_CONST_BUFFER 0


enum brw_cache_id {
   BRW_BLEND_STATE,
   BRW_DEPTH_STENCIL_STATE,
   BRW_COLOR_CALC_STATE,
   BRW_CC_VP,
   BRW_CC_UNIT,
   BRW_WM_PROG,
   BRW_SAMPLER,
   BRW_WM_UNIT,
   BRW_SF_PROG,
   BRW_SF_VP,
   BRW_SF_UNIT, /* scissor state on gen6 */
   BRW_VS_UNIT,
   BRW_VS_PROG,
   BRW_GS_UNIT,
   BRW_GS_PROG,
   BRW_CLIP_VP,
   BRW_CLIP_UNIT,
   BRW_CLIP_PROG,

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
   GLuint aux_size;
   const void *key;

   uint32_t offset;
   uint32_t size;

   struct brw_cache_item *next;
};   



struct brw_cache {
   struct brw_context *brw;

   struct brw_cache_item **items;
   drm_intel_bo *bo;
   GLuint size, n_items;

   uint32_t next_offset;
   bool bo_used_by_gpu;
};


/* Considered adding a member to this struct to document which flags
 * an update might raise so that ordering of the state atoms can be
 * checked or derived at runtime.  Dropped the idea in favor of having
 * a debug mode where the state is monitored for flags which are
 * raised that have already been tested against.
 */
struct brw_tracked_state {
   struct brw_state_flags dirty;
   void (*prepare)( struct brw_context *brw );
   void (*emit)( struct brw_context *brw );
};

/* Flags for brw->state.cache.
 */
#define CACHE_NEW_BLEND_STATE            (1<<BRW_BLEND_STATE)
#define CACHE_NEW_DEPTH_STENCIL_STATE    (1<<BRW_DEPTH_STENCIL_STATE)
#define CACHE_NEW_COLOR_CALC_STATE       (1<<BRW_COLOR_CALC_STATE)
#define CACHE_NEW_CC_VP                  (1<<BRW_CC_VP)
#define CACHE_NEW_CC_UNIT                (1<<BRW_CC_UNIT)
#define CACHE_NEW_WM_PROG                (1<<BRW_WM_PROG)
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

struct brw_cached_batch_item {
   struct header *header;
   GLuint sz;
   struct brw_cached_batch_item *next;
};
   


/* Protect against a future where VERT_ATTRIB_MAX > 32.  Wouldn't life
 * be easier if C allowed arrays of packed elements?
 */
#define ATTRIB_BIT_DWORDS  ((VERT_ATTRIB_MAX+31)/32)

struct brw_vertex_buffer {
   /** Buffer object containing the uploaded vertex data */
   drm_intel_bo *bo;
   uint32_t offset;
   /** Byte stride between elements in the uploaded array */
   GLuint stride;
};
struct brw_vertex_element {
   const struct gl_client_array *glarray;

   int buffer;

   /** The corresponding Mesa vertex attribute */
   gl_vert_attrib attrib;
   /** Size of a complete element */
   GLuint element_size;
   /** Offset of the first element within the buffer object */
   unsigned int offset;
};



struct brw_vertex_info {
   GLuint sizes[ATTRIB_BIT_DWORDS * 2]; /* sizes:2[VERT_ATTRIB_MAX] */
};

struct brw_query_object {
   struct gl_query_object Base;

   /** Last query BO associated with this query. */
   drm_intel_bo *bo;
   /** First index in bo with query data for this object. */
   int first_index;
   /** Last index in bo with query data for this object. */
   int last_index;
};


/**
 * brw_context is derived from intel_context.
 */
struct brw_context 
{
   struct intel_context intel;  /**< base class, must be first field */
   GLuint primitive;

   GLboolean emit_state_always;
   GLboolean has_surface_tile_offset;
   GLboolean has_compr4;
   GLboolean has_negative_rhw_bug;
   GLboolean has_aa_line_parameters;
   GLboolean has_pln;

   struct {
      struct brw_state_flags dirty;
      /**
       * List of buffers accumulated in brw_validate_state to receive
       * drm_intel_bo_check_aperture treatment before exec, so we can
       * know if we should flush the batch and try again before
       * emitting primitives.
       *
       * This can be a fixed number as we only have a limited number of
       * objects referenced from the batchbuffer in a primitive emit,
       * consisting of the vertex buffers, pipelined state pointers,
       * the CURBE, the depth buffer, and a query BO.
       */
      drm_intel_bo *validated_bos[VERT_ATTRIB_MAX + BRW_WM_MAX_SURF + 16];
      int validated_bo_count;
   } state;

   struct brw_cache cache;
   struct brw_cached_batch_item *cached_batch_items;

   struct {
      struct brw_vertex_element inputs[VERT_ATTRIB_MAX];
      struct brw_vertex_buffer buffers[VERT_ATTRIB_MAX];
      struct {
	      uint32_t handle;
	      uint32_t offset;
	      uint32_t stride;
      } current_buffers[VERT_ATTRIB_MAX];

      struct brw_vertex_element *enabled[VERT_ATTRIB_MAX];
      GLuint nr_enabled;
      GLuint nr_buffers, nr_current_buffers;

      /* Summary of size and varying of active arrays, so we can check
       * for changes to this state:
       */
      struct brw_vertex_info info;
      unsigned int min_index, max_index;

      /* Offset from start of vertex buffer so we can avoid redefining
       * the same VB packed over and over again.
       */
      unsigned int start_vertex_bias;
   } vb;

   struct {
      /**
       * Index buffer for this draw_prims call.
       *
       * Updates are signaled by BRW_NEW_INDICES.
       */
      const struct _mesa_index_buffer *ib;

      /* Updates are signaled by BRW_NEW_INDEX_BUFFER. */
      drm_intel_bo *bo;
      GLuint type;

      /* Offset to index buffer index to use in CMD_3D_PRIM so that we can
       * avoid re-uploading the IB packet over and over if we're actually
       * referencing the same index buffer.
       */
      unsigned int start_vertex_offset;
   } ib;

   /* Active vertex program: 
    */
   const struct gl_vertex_program *vertex_program;
   const struct gl_fragment_program *fragment_program;

   /* hw-dependent 3DSTATE_VF_STATISTICS opcode */
   uint32_t CMD_VF_STATISTICS;
   /* hw-dependent 3DSTATE_PIPELINE_SELECT opcode */
   uint32_t CMD_PIPELINE_SELECT;
   int vs_max_threads;
   int wm_max_threads;

   /* BRW_NEW_URB_ALLOCATIONS:
    */
   struct {
      GLuint vsize;		/* vertex size plus header in urb registers */
      GLuint csize;		/* constant buffer size in urb registers */
      GLuint sfsize;		/* setup data size in urb registers */

      GLboolean constrained;

      GLuint max_vs_entries;	/* Maximum number of VS entries */
      GLuint max_gs_entries;	/* Maximum number of GS entries */

      GLuint nr_vs_entries;
      GLuint nr_gs_entries;
      GLuint nr_clip_entries;
      GLuint nr_sf_entries;
      GLuint nr_cs_entries;

      /* gen6:
       * The length of each URB entry owned by the VS (or GS), as
       * a number of 1024-bit (128-byte) rows.  Should be >= 1.
       *
       * gen7: Same meaning, but in 512-bit (64-byte) rows.
       */
      GLuint vs_size;
      GLuint gs_size;

      GLuint vs_start;
      GLuint gs_start;
      GLuint clip_start;
      GLuint sf_start;
      GLuint cs_start;
      GLuint size; /* Hardware URB size, in KB. */
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

      drm_intel_bo *curbe_bo;
      /** Offset within curbe_bo of space for current curbe entry */
      GLuint curbe_offset;
      /** Offset within curbe_bo of space for next curbe entry */
      GLuint curbe_next_offset;

      /**
       * Copy of the last set of CURBEs uploaded.  Frequently we'll end up
       * in brw_curbe.c with the same set of constant data to be uploaded,
       * so we'd rather not upload new constants in that case (it can cause
       * a pipeline bubble since only up to 4 can be pipelined at a time).
       */
      GLfloat *last_buf;
      /**
       * Allocation for where to calculate the next set of CURBEs.
       * It's a hot enough path that malloc/free of that data matters.
       */
      GLfloat *next_buf;
      GLuint last_bufsz;
   } curbe;

   struct {
      struct brw_vs_prog_data *prog_data;
      int8_t *constant_map; /* variable array following prog_data */

      drm_intel_bo *const_bo;
      /** Offset in the program cache to the VS program */
      uint32_t prog_offset;
      uint32_t state_offset;

      /** Binding table of pointers to surf_bo entries */
      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_VS_MAX_SURF];
      GLuint nr_surfaces;      

      uint32_t push_const_offset; /* Offset in the batchbuffer */
      int push_const_size; /* in 256-bit register increments */
   } vs;

   struct {
      struct brw_gs_prog_data *prog_data;

      GLboolean prog_active;
      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;
   } gs;

   struct {
      struct brw_clip_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;

      /* Offset in the batch to the CLIP state on pre-gen6. */
      uint32_t state_offset;

      /* As of gen6, this is the offset in the batch to the CLIP VP,
       * instead of vp_bo.
       */
      uint32_t vp_offset;
   } clip;


   struct {
      struct brw_sf_prog_data *prog_data;

      /** Offset in the program cache to the CLIP program pre-gen6 */
      uint32_t prog_offset;
      uint32_t state_offset;
      uint32_t vp_offset;
   } sf;

   struct {
      struct brw_wm_prog_data *prog_data;
      struct brw_wm_compile *compile_data;

      /** Input sizes, calculated from active vertex program.
       * One bit per fragment program input attribute.
       */
      GLbitfield input_size_masks[4];

      /** offsets in the batch to sampler default colors (texture border color)
       */
      uint32_t sdc_offset[BRW_MAX_TEX_UNIT];

      GLuint render_surf;
      GLuint nr_surfaces;      

      GLuint max_threads;
      drm_intel_bo *scratch_bo;

      GLuint sampler_count;
      uint32_t sampler_offset;

      /** Offset in the program cache to the WM program */
      uint32_t prog_offset;

      /** Binding table of pointers to surf_bo entries */
      uint32_t bind_bo_offset;
      uint32_t surf_offset[BRW_WM_MAX_SURF];
      uint32_t state_offset; /* offset in batchbuffer to pre-gen6 WM state */

      drm_intel_bo *const_bo; /* pull constant buffer. */
      /**
       * This is offset in the batch to the push constants on gen6.
       *
       * Pre-gen6, push constants live in the CURBE.
       */
      uint32_t push_const_offset;
   } wm;


   struct {
      uint32_t state_offset;
      uint32_t blend_state_offset;
      uint32_t depth_stencil_state_offset;
      uint32_t vp_offset;
   } cc;

   struct {
      struct brw_query_object *obj;
      drm_intel_bo *bo;
      int index;
      GLboolean active;
   } query;
   /* Used to give every program string a unique id
    */
   GLuint program_id;

   int num_prepare_atoms, num_emit_atoms;
   struct brw_tracked_state prepare_atoms[64], emit_atoms[64];
};


#define BRW_PACKCOLOR8888(r,g,b,a)  ((r<<24) | (g<<16) | (b<<8) | a)

struct brw_instruction_info {
    char    *name;
    int	    nsrc;
    int	    ndst;
    GLboolean is_arith;
};
extern const struct brw_instruction_info brw_opcodes[128];

/*======================================================================
 * brw_vtbl.c
 */
void brwInitVtbl( struct brw_context *brw );

/*======================================================================
 * brw_context.c
 */
GLboolean brwCreateContext( int api,
			    const struct gl_config *mesaVis,
			    __DRIcontext *driContextPriv,
			    void *sharedContextPrivate);

/*======================================================================
 * brw_queryobj.c
 */
void brw_init_queryobj_functions(struct dd_function_table *functions);
void brw_prepare_query_begin(struct brw_context *brw);
void brw_emit_query_begin(struct brw_context *brw);
void brw_emit_query_end(struct brw_context *brw);

/*======================================================================
 * brw_state_dump.c
 */
void brw_debug_batch(struct intel_context *intel);

/*======================================================================
 * brw_tex.c
 */
void brw_validate_textures( struct brw_context *brw );


/*======================================================================
 * brw_program.c
 */
void brwInitFragProgFuncs( struct dd_function_table *functions );


/* brw_urb.c
 */
void brw_upload_urb_fence(struct brw_context *brw);

/* brw_curbe.c
 */
void brw_upload_cs_urb_state(struct brw_context *brw);

/* brw_disasm.c */
int brw_disasm (FILE *file, struct brw_instruction *inst, int gen);

/*======================================================================
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct brw_context *
brw_context( struct gl_context *ctx )
{
   return (struct brw_context *)ctx;
}

static INLINE struct brw_vertex_program *
brw_vertex_program(struct gl_vertex_program *p)
{
   return (struct brw_vertex_program *) p;
}

static INLINE const struct brw_vertex_program *
brw_vertex_program_const(const struct gl_vertex_program *p)
{
   return (const struct brw_vertex_program *) p;
}

static INLINE struct brw_fragment_program *
brw_fragment_program(struct gl_fragment_program *p)
{
   return (struct brw_fragment_program *) p;
}

static INLINE const struct brw_fragment_program *
brw_fragment_program_const(const struct gl_fragment_program *p)
{
   return (const struct brw_fragment_program *) p;
}

static inline
float convert_param(enum param_conversion conversion, float param)
{
   union {
      float f;
      uint32_t u;
      int32_t i;
   } fi;

   switch (conversion) {
   case PARAM_NO_CONVERT:
      return param;
   case PARAM_CONVERT_F2I:
      fi.i = param;
      return fi.f;
   case PARAM_CONVERT_F2U:
      fi.u = param;
      return fi.f;
   case PARAM_CONVERT_F2B:
      if (param != 0.0)
	 fi.i = 1;
      else
	 fi.i = 0;
      return fi.f;
   default:
      return param;
   }
}

/**
 * Pre-gen6, the register file of the EUs was shared between threads,
 * and each thread used some subset allocated on a 16-register block
 * granularity.  The unit states wanted these block counts.
 */
static inline int
brw_register_blocks(int reg_count)
{
   return ALIGN(reg_count, 16) / 16 - 1;
}

static inline uint32_t
brw_program_reloc(struct brw_context *brw, uint32_t state_offset,
		  uint32_t prog_offset)
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 5) {
      /* Using state base address. */
      return prog_offset;
   }

   drm_intel_bo_emit_reloc(intel->batch.bo,
			   state_offset,
			   brw->cache.bo,
			   prog_offset,
			   I915_GEM_DOMAIN_INSTRUCTION, 0);

   return brw->cache.bo->offset + prog_offset;
}

GLboolean brw_do_cubemap_normalize(struct exec_list *instructions);

#endif
