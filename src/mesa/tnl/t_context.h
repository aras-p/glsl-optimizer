
/* $Id: t_context.h,v 1.1 2000/11/16 21:05:42 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _T_CONTEXT_H
#define _T_CONTEXT_H

#include "glheader.h"
#include "types.h"

#include "math/m_matrix.h"
#include "math/m_vector.h"
#include "math/m_xform.h"

#include "t_trans_elt.h"



/*
 * Bits to indicate which faces a vertex participates in,
 * what facing the primitive provoked by that vertex has,
 * and some misc. flags.
 */
#define VERT_FACE_FRONT       0x1	/* is in a front-color primitive */
#define VERT_FACE_REAR        0x2	/* is in a rear-color primitive */
#define PRIM_FACE_FRONT       0x4	/* use front color */
#define PRIM_FACE_REAR        0x8	/* use rear color */
#define PRIM_CLIPPED          0x10	/* needs clipping */
#define PRIM_USER_CLIPPED     CLIP_USER_BIT	/* 0x40 */


#define PRIM_FLAG_SHIFT  2
#define PRIM_FACE_FLAGS  (PRIM_FACE_FRONT|PRIM_FACE_REAR)
#define VERT_FACE_FLAGS  (VERT_FACE_FRONT|VERT_FACE_REAR)

#define PRIM_ANY_CLIP    (PRIM_CLIPPED|PRIM_USER_CLIPPED)
#define PRIM_NOT_CULLED  (PRIM_ANY_CLIP|PRIM_FACE_FLAGS)

/* Flags for VB->CullMode.
 */
#define CULL_MASK_ACTIVE  0x1
#define COMPACTED_NORMALS 0x2
#define CLIP_MASK_ACTIVE  0x4

/* Flags for selecting a shading function.  The first two bits are
 * shared with the cull mode (ie. cull_mask_active and
 * compacted_normals.)
 */
#define SHADE_TWOSIDE           0x4


/* KW: Flags that describe the current vertex state, and the contents
 * of a vertex in a vertex-cassette.  
 *
 * For really major expansion, consider a 'VERT_ADDITIONAL_FLAGS' flag,
 * which means there is data in another flags array (eg, extra_flags[]).
 */

#define VERT_OBJ_2           0x1	/* glVertex2 */
#define VERT_OBJ_3           0x2        /* glVertex3 */
#define VERT_OBJ_4           0x4        /* glVertex4 */
#define VERT_BEGIN           0x8	/* glBegin */
#define VERT_END             0x10	/* glEnd */
#define VERT_ELT             0x20	/* glArrayElement */
#define VERT_RGBA            0x40	/* glColor */
#define VERT_NORM            0x80	/* glNormal */
#define VERT_INDEX           0x100	/* glIndex */
#define VERT_EDGE            0x200	/* glEdgeFlag */
#define VERT_MATERIAL        0x400	/* glMaterial */
#define VERT_END_VB          0x800	/* end vb marker */
#define VERT_TEX0_12         0x1000	   
#define VERT_TEX0_3          0x2000
#define VERT_TEX0_4          0x4000
#define VERT_TEX1_12         0x8000
#define VERT_TEX1_3          0x10000
#define VERT_TEX1_4          0x20000
#define VERT_TEX2_12         0x40000
#define VERT_TEX2_3          0x80000
#define VERT_TEX2_4          0x100000
#define VERT_TEX3_12         0x200000
#define VERT_TEX3_3          0x400000
#define VERT_TEX3_4          0x800000
#define VERT_EVAL_C1         0x1000000  /* could reuse OBJ bits for this? */
#define VERT_EVAL_C2         0x2000000  /*    - or just use 3 bits */
#define VERT_EVAL_P1         0x4000000  /*  */
#define VERT_EVAL_P2         0x8000000  /*  */
#define VERT_SPEC_RGB        0x10000000	
#define VERT_FOG_COORD       0x20000000	/* internal use only, currently */

#define VERT_EYE             VERT_BEGIN /* reuse */
#define VERT_WIN             VERT_END   /* reuse */
#define VERT_SETUP_FULL      VERT_EVAL_P1 /* Rastersetup has been done */
#define VERT_PRECALC_DATA    VERT_END_VB /* reuse */

/* Shorthands.
 */
#define VERT_TEX0_SHIFT 11

#define VERT_EVAL_ANY      (VERT_EVAL_C1|VERT_EVAL_P1|	\
                            VERT_EVAL_C2|VERT_EVAL_P2)

#define VERT_OBJ_23       (VERT_OBJ_3|VERT_OBJ_2)
#define VERT_OBJ_234      (VERT_OBJ_4|VERT_OBJ_23)
#define VERT_OBJ_ANY      VERT_OBJ_2

#define VERT_TEX0_123     (VERT_TEX0_3|VERT_TEX0_12)
#define VERT_TEX0_1234    (VERT_TEX0_4|VERT_TEX0_123)
#define VERT_TEX0_ANY     VERT_TEX0_12

#define VERT_TEX1_123     (VERT_TEX1_3|VERT_TEX1_12)
#define VERT_TEX1_1234    (VERT_TEX1_4|VERT_TEX1_123)
#define VERT_TEX1_ANY     VERT_TEX1_12

#define VERT_TEX2_123     (VERT_TEX2_3|VERT_TEX2_12)
#define VERT_TEX2_1234    (VERT_TEX2_4|VERT_TEX2_123)
#define VERT_TEX2_ANY     VERT_TEX2_12

#define VERT_TEX3_123     (VERT_TEX3_3|VERT_TEX3_12)
#define VERT_TEX3_1234    (VERT_TEX3_4|VERT_TEX3_123)
#define VERT_TEX3_ANY     VERT_TEX3_12

#define NR_TEXSIZE_BITS   3
#define VERT_TEX_ANY(i)   (VERT_TEX0_ANY << ((i) * NR_TEXSIZE_BITS))

#define VERT_FIXUP         (VERT_TEX0_ANY | \
                            VERT_TEX1_ANY | \
                            VERT_TEX2_ANY | \
                            VERT_TEX3_ANY | \
			    VERT_RGBA | \
			    VERT_SPEC_RGB | \
			    VERT_FOG_COORD | \
                            VERT_INDEX | \
                            VERT_EDGE | \
                            VERT_NORM)

#define VERT_DATA          (VERT_TEX0_ANY | \
                            VERT_TEX1_ANY | \
                            VERT_TEX2_ANY | \
                            VERT_TEX3_ANY | \
			    VERT_RGBA | \
			    VERT_SPEC_RGB | \
			    VERT_FOG_COORD | \
                            VERT_INDEX | \
                            VERT_EDGE | \
                            VERT_NORM | \
	                    VERT_OBJ_ANY | \
                            VERT_MATERIAL | \
                            VERT_ELT | \
	                    VERT_EVAL_ANY | \
                            VERT_FOG_COORD)


/* For beginstate
 */
#define VERT_BEGIN_0    0x1	   /* glBegin (if initially inside beg/end) */
#define VERT_BEGIN_1    0x2	   /* glBegin (if initially outside beg/end) */
#define VERT_ERROR_0    0x4	   /* invalid_operation in initial state 0 */
#define VERT_ERROR_1    0x8        /* invalid_operation in initial state 1 */


struct gl_pipeline;
struct tnl_context;

/**
 ** Vertex buffer/array structures
 **/

struct vertex_data
{
   GLfloat (*Obj)[4];
   GLfloat (*Normal)[3];
   GLchan  (*Color)[4];
   GLuint   *Index;
   GLubyte  *EdgeFlag;
   GLfloat (*TexCoord[MAX_TEXTURE_UNITS])[4];
   GLuint   *Elt;
   GLfloat  *FogCoord;
   GLubyte  (*SecondaryColor)[4];
};

struct vertex_arrays
{
   GLvector4f  Obj;
   GLvector3f  Normal;
   GLvector4ub Color;
   GLvector1ui Index;
   GLvector1ub EdgeFlag;
   GLvector4f  TexCoord[MAX_TEXTURE_UNITS];
   GLvector1ui Elt;     
   GLvector4ub SecondaryColor;
   GLvector1f  FogCoord;
};

struct vertex_array_pointers
{
   GLvector4f  *Obj;
   GLvector3f  *Normal;
   GLvector4ub *Color;
   GLvector1ui *Index;
   GLvector1ub *EdgeFlag;
   GLvector4f  *TexCoord[MAX_TEXTURE_UNITS];
   GLvector1ui *Elt;     
   GLvector4ub *SecondaryColor;
   GLvector1f *FogCoord;     
};

/* Values for VB->Type */
enum {
   VB_IMMEDIATE,
   VB_CVA_PRECALC
};


/* Values for immediate->BeginState */
#define VERT_BEGIN_0    0x1	   /* glBegin (if initially inside beg/end) */
#define VERT_BEGIN_1    0x2	   /* glBegin (if initially outside beg/end) */
#define VERT_ERROR_0    0x4	   /* invalid_operation in initial state 0 */
#define VERT_ERROR_1    0x8        /* invalid_operation in initial state 1 */


/* KW: Represents everything that can take place between a begin and
 * end, and can represent multiple begin/end pairs.  This plus *any*
 * state variable (GLcontext) should be all you need to replay the
 * represented begin/end pairs as if they took place in that state.  
 *
 * Thus this is sufficient for both immediate and compiled modes, but
 * we could/should throw some elements away for compiled mode if we
 * know they were empty. 
 */
struct immediate 
{ 
   struct immediate *next;	/* for cache of free IM's */
   GLuint id, ref_count;

   /* This must be saved when immediates are shared in display lists.
    */
   GLuint Start, Count;
   GLuint LastData;		/* count or count+1 */
   GLuint AndFlag, OrFlag, BeginState;
   GLuint LastPrimitive;	

   GLuint ArrayAndFlags;	/* precalc'ed for glArrayElt */
   GLuint ArrayIncr;
   GLuint ArrayEltFlush;
   GLuint FlushElt;

   GLuint TF1[MAX_TEXTURE_UNITS];	/* precalc'ed for glTexCoord */
   GLuint TF2[MAX_TEXTURE_UNITS];
   GLuint TF3[MAX_TEXTURE_UNITS];
   GLuint TF4[MAX_TEXTURE_UNITS];

   GLuint  Primitive[VB_SIZE];	/* GLubyte would do... */
   GLuint  NextPrimitive[VB_SIZE]; 

   /* allocate storage for these on demand:
    */
   struct  gl_material (*Material)[2]; 
   GLuint  *MaterialMask;       

   GLfloat (*TexCoordPtr[MAX_TEXTURE_UNITS])[4]; 

   struct vertex_arrays v;
   
   struct __GLcontextRec *backref;		    

   /* Normal lengths, zero if not available.
    */
   GLfloat   *NormalLengths;
   GLuint     LastCalcedLength;

   GLuint  Flag[VB_SIZE];    /* bitwise-OR of VERT_ flags */
   GLchan  Color[VB_SIZE][4];
   GLfloat Obj[VB_SIZE][4];
   GLfloat Normal[VB_SIZE][3];
   GLfloat TexCoord[MAX_TEXTURE_UNITS][VB_SIZE][4];
   GLuint  Elt[VB_SIZE];
   GLubyte EdgeFlag[VB_SIZE];
   GLuint  Index[VB_SIZE];
   GLubyte SecondaryColor[VB_SIZE][4];
   GLfloat FogCoord[VB_SIZE];
};


/* Not so big on storage these days, although still has pointers to
 * arrays used for temporary results.
 */
typedef struct vertex_buffer
{
   /* Backpointers.
    */
   struct __GLcontextRec *ctx;
   struct tnl_context *tnlctx;

   /* Driver_data is allocated in Driver.RegisterVB(), if required.
    */
   void *driver_data;

   /* List of operations to process vertices in current state.
    */
   struct gl_pipeline *pipeline;

   /* Temporary storage used by immediate mode functions and various
    * operations in the pipeline.
    */
   struct immediate *IM;	        
   struct vertex_array_pointers store;	

   /* Where to find outstanding untransformed vertices.
    */
   struct immediate *prev_buffer;

   GLuint     Type;            /* Either VB_IMMEDIATE or VB_CVA_PRECALC */

   GLuint     Size, Start, Count;
   GLuint     Free, FirstFree;
   GLuint     CopyStart;
   GLuint     Parity, Ovf;
   GLuint     PurgeFlags;
   GLuint     IndirectCount;	/* defaults to count */
   GLuint     OrFlag, SavedOrFlag;
   GLuint     EarlyCull;
   GLuint     Culled, CullDone;

   /* Pointers to input data - default to buffers in 'im' above.
    */
   GLvector4f  *ObjPtr;
   GLvector3f  *NormalPtr;
   GLvector4ub *ColorPtr;
   GLvector1ui *IndexPtr;
   GLvector1ub *EdgeFlagPtr;
   GLvector4f  *TexCoordPtr[MAX_TEXTURE_UNITS];
   GLvector1ui *EltPtr;
   GLvector4ub *SecondaryColorPtr;
   GLvector1f  *FogCoordPtr;
   GLuint      *Flag, FlagMax;
   struct      gl_material (*Material)[2];
   GLuint      *MaterialMask;       

   GLuint      *NextPrimitive; 
   GLuint      *Primitive;     
   GLuint      LastPrimitive;

   GLfloat (*BoundsPtr)[3];	/* Bounds for cull check */
   GLfloat  *NormalLengthPtr;	/* Array of precomputed inv. normal lengths */
   
   /* Holds malloced storage for pipeline data not supplied by 
    * the immediate struct.
    */
   GLvector4f Eye;
   GLvector4f Clip;
   GLvector4f Win;
   GLvector4ub BColor;		/* not used in cva vb's */
   GLvector1ui BIndex;		/* not used in cva vb's */
   GLvector4ub BSecondary;	/* not used in cva vb's */

   /* Temporary storage - may point into IM, or be dynamically
    * allocated (for cva).  
    */
   GLubyte *ClipMask;
   GLubyte *UserClipMask;
   
   /* Internal values.  Where these point depends on whether
    * there were any identity matrices defined as transformations
    * in the pipeline.
    */
   GLvector4f *EyePtr;
   GLvector4f *ClipPtr;
   GLvector4f *Unprojected;
   GLvector4f *Projected;
   GLvector4f *CurrentTexCoord;
   GLuint     *Indirect;           /* For eval rescue and cva render */

   /* Currently active colors
    */
   GLvector4ub *Color[2];
   GLvector1ui *Index[2];
   GLvector4ub *SecondaryColor[2];

   /* Storage for colors which have been lit but not yet fogged.  
    * Required for CVA, just point into store for normal VB's.
    */
   GLvector4ub *LitColor[2];
   GLvector1ui *LitIndex[2];
   GLvector4ub *LitSecondary[2];

   /* Temporary values used in texgen.
    */
   GLfloat (*tmp_f)[3];
   GLfloat *tmp_m;

   /* Temporary values used in eval.
    */
   GLuint *EvaluatedFlags;

   /* Not used for cva: 
    */
   GLubyte *NormCullStart;
   GLubyte *CullMask;	        /* Results of vertex culling */
   GLubyte *NormCullMask;       /* Compressed onto shared normals */

   GLubyte ClipOrMask;		/* bitwise-OR of all ClipMask[] values */
   GLubyte ClipAndMask;		/* bitwise-AND of all ClipMask[] values */
   GLubyte CullFlag[2];
   GLubyte CullMode;		/* see flags below */

   GLuint CopyCount;		/* max 3 vertices to copy after transform */
   GLuint Copy[3];
   GLfloat CopyProj[3][4];	/* temporary store for projected clip coords */

   /* Hooks for module private data
    */
   void *swsetup_vb;

} TNLvertexbuffer;


typedef void (*gl_shade_func)( struct vertex_buffer *VB );

typedef void (*clip_interp_func)( struct vertex_buffer *VB, GLuint dst,
                                  GLfloat t, GLuint in, GLuint out );

typedef GLuint (*clip_line_func)( struct vertex_buffer *VB, 
				  GLuint *i, GLuint *j,
				  GLubyte mask);

typedef GLuint (*clip_poly_func)( struct vertex_buffer *VB,
				  GLuint n, GLuint vlist[],
				  GLubyte mask );


#define MAX_PIPELINE_STAGES     30

#define PIPE_IMMEDIATE          0x1
#define PIPE_PRECALC            0x2

#define PIPE_OP_VERT_XFORM        0x1
#define PIPE_OP_NORM_XFORM        0x2
#define PIPE_OP_LIGHT             0x4
#define PIPE_OP_FOG               0x8
#define PIPE_OP_TEX0             0x10
#define PIPE_OP_TEX1             0x20
#define PIPE_OP_TEX2             0x40
#define PIPE_OP_TEX3             0x80
#define PIPE_OP_RAST_SETUP_0    0x100
#define PIPE_OP_RAST_SETUP_1    0x200
#define PIPE_OP_RENDER          0x400
#define PIPE_OP_CVA_PREPARE     0x800



struct gl_pipeline_stage {
   const char *name;
   GLuint ops;			/* PIPE_OP flags */
   GLuint type;			/* VERT flags */
   GLuint special;		/* VERT flags - force update_inputs() */
   GLuint state_change;	        /* state flags - trigger update_inputs() */
   GLuint cva_state_change;          /* state flags - recalc cva buffer */
   GLuint elt_forbidden_inputs;      /* VERT flags - force a pipeline recalc */
   GLuint pre_forbidden_inputs;	     /* VERT flags - force a pipeline recalc */
   GLuint active;		     /* VERT flags */
   GLuint inputs;		     /* VERT flags */
   GLuint outputs;		     /* VERT flags */
   void (*check)( GLcontext *ctx, struct gl_pipeline_stage * );
   void (*run)( struct vertex_buffer *VB );
};


struct gl_pipeline {
   GLuint state_change;		/* state changes which require recalc */
   GLuint cva_state_change;	/* ... which require re-run */
   GLuint forbidden_inputs;	/* inputs which require recalc */
   GLuint ops;			/* what gets done in this pipe */
   GLuint changed_ops;
   GLuint inputs;
   GLuint outputs;
   GLuint new_inputs;
   GLuint new_outputs;
   GLuint fallback;
   GLuint type;
   GLuint pipeline_valid:1;
   GLuint data_valid:1;
   GLuint copy_transformed_data:1;
   GLuint replay_copied_vertices:1;
   GLuint new_state;		/* state changes since last recalc */
   struct gl_pipeline_stage *stages[MAX_PIPELINE_STAGES];
};



/* All fields are derived.
 */
struct gl_cva {
   struct gl_pipeline pre;
   struct gl_pipeline elt;
	
   struct gl_client_array Elt;
   trans_1ui_func EltFunc;

   struct vertex_buffer *VB;
   struct vertex_arrays v;
   struct vertex_data store;

   GLuint elt_count;
   GLenum elt_mode;
   GLuint elt_size;

   GLuint forbidden_inputs;
   GLuint orflag;
   GLuint merge;

   GLuint lock_changed;
   GLuint last_orflag;
   GLuint last_array_flags;
   GLuint last_array_new_state;
};



typedef void (*texgen_func)( struct vertex_buffer *VB, 
			     GLuint textureSet);



typedef struct tnl_context {	

   GLuint _ArrayFlag[VB_SIZE];	/* crock */
   GLuint _ArrayFlags;
   GLuint _ArraySummary;	/* Like flags, but no size information */
   GLuint _ArrayNewState;	/* Tracks which arrays have been changed. */


   /* Pipeline stages - shared between the two pipelines,
    * which live in CVA.
    */
   struct gl_pipeline_stage PipelineStage[MAX_PIPELINE_STAGES];
   GLuint NrPipelineStages;

   /* Per-texunit derived state.
    */
   GLuint _TexgenSize[MAX_TEXTURE_UNITS];
   GLuint _TexgenHoles[MAX_TEXTURE_UNITS];
   texgen_func *_TexgenFunc[MAX_TEXTURE_UNITS];


   /* Display list extensions
    */
   GLuint opcode_vertex_cassette;

   /* Cva 
    */
   struct gl_cva CVA;
   GLboolean CompileCVAFlag;

   clip_poly_func *_poly_clip_tab;
   clip_line_func *_line_clip_tab;
   clip_interp_func _ClipInterpFunc; /* Clip interpolation function */
   normal_func *_NormalTransform; 
   gl_shade_func *_shade_func_tab;   /* Current shading function table */

   GLenum _CurrentPrimitive;         /* Prim or GL_POLYGON+1 */
   GLuint _CurrentFlag;

   GLuint _RenderFlags;	      /* Active inputs to render stage */

   /* Cache of unused immediate structs */
   struct immediate *freed_im_queue; 
   GLuint nr_im_queued;

} TNLcontext;



#define TNL_CONTEXT(ctx) ((TNLcontext *)(ctx->swtnl_context))
#define TNL_CURRENT_IM(ctx) ((struct immediate *)(ctx->swtnl_im))
#define TNL_VB(ctx) ((struct vertex_buffer *)(ctx->swtnl_vb))

extern void _tnl_reset_immediate( GLcontext *ctx );
extern GLboolean _tnl_flush_vertices( GLcontext *ctx, GLuint flush_flags );


extern void
_tnl_MakeCurrent( GLcontext *ctx, 
		  GLframebuffer *drawBuffer, 
		  GLframebuffer *readBuffer );


extern void
_tnl_LightingSpaceChange( GLcontext *ctx );

/*
 * Macros for fetching current input buffer.
 */
#ifdef THREADS
#define GET_IMMEDIATE  struct immediate *IM = TNL_CURRENT_IM(((GLcontext *) (_glapi_Context ? _glapi_Context : _glapi_get_context())))
#define SET_IMMEDIATE(ctx, im)  ctx->swtnl_im = (void *)im
#else
extern struct immediate *_mesa_CurrentInput;
#define GET_IMMEDIATE struct immediate *IM = _mesa_CurrentInput
#define SET_IMMEDIATE(ctx, im)			\
do {						\
   TNL_CURRENT_IM(ctx) = im;			\
   _mesa_CurrentInput = im;			\
} while (0)
#endif

#endif
