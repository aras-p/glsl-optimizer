/* $Id: t_context.h,v 1.28 2001/06/04 16:09:28 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */

#ifndef _T_CONTEXT_H
#define _T_CONTEXT_H

#include "glheader.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#include "math/m_vector.h"
#include "math/m_xform.h"


#define MAX_PIPELINE_STAGES     30


/* Numbers for sizing immediate structs.
 */
#define IMM_MAX_COPIED_VERTS  3
#define IMM_MAXDATA          (216 + IMM_MAX_COPIED_VERTS)
#define IMM_SIZE             (IMM_MAXDATA + MAX_CLIPPED_VERTICES)


/* Values for IM->BeginState
 */
#define VERT_BEGIN_0    0x1	   /* glBegin (if initially inside beg/end) */
#define VERT_BEGIN_1    0x2	   /* glBegin (if initially outside beg/end) */
#define VERT_ERROR_0    0x4	   /* invalid_operation in initial state 0 */
#define VERT_ERROR_1    0x8        /* invalid_operation in initial state 1 */


/* Flags to be added to the primitive enum in VB->Primitive.
 */
#define PRIM_MODE_MASK  0xff    /* Extract the actual primitive */
#define PRIM_BEGIN      0x100	/* The prim starts here (not wrapped) */
#define PRIM_END        0x200	/* The prim ends in this VB (does not wrap) */
#define PRIM_PARITY     0x400	/* The prim wrapped on an odd number of verts */
#define PRIM_LAST       0x800   /* No more prims in the VB */


/* Flags that describe the inputs and outputs of pipeline stages, and
 * the contents of a vertex-cassette.
 *
 * 5 spare flags, rearrangement of eval flags can secure at least 3
 * more.
 */
#define VERT_OBJ             _NEW_ARRAY_VERTEX
#define VERT_RGBA            _NEW_ARRAY_COLOR
#define VERT_NORM            _NEW_ARRAY_NORMAL
#define VERT_INDEX           _NEW_ARRAY_INDEX
#define VERT_EDGE            _NEW_ARRAY_EDGEFLAG
#define VERT_SPEC_RGB        _NEW_ARRAY_SECONDARYCOLOR
#define VERT_FOG_COORD       _NEW_ARRAY_FOGCOORD
#define VERT_TEX0            _NEW_ARRAY_TEXCOORD_0
#define VERT_TEX1            _NEW_ARRAY_TEXCOORD_1
#define VERT_TEX2            _NEW_ARRAY_TEXCOORD_2
#define VERT_TEX3            _NEW_ARRAY_TEXCOORD_3
#define VERT_TEX4            _NEW_ARRAY_TEXCOORD_4
#define VERT_TEX5            _NEW_ARRAY_TEXCOORD_5
#define VERT_TEX6            _NEW_ARRAY_TEXCOORD_6
#define VERT_TEX7            _NEW_ARRAY_TEXCOORD_7
#define VERT_EVAL_C1         0x8000     /* imm only */
#define VERT_EVAL_C2         0x10000    /* imm only */
#define VERT_EVAL_P1         0x20000    /* imm only */
#define VERT_EVAL_P2         0x40000    /* imm only */
#define VERT_OBJ_3           0x80000    /* imm only */
#define VERT_OBJ_4           0x100000   /* imm only */
#define VERT_MATERIAL        0x200000   /* imm only, but tested in vb code */
#define VERT_ELT             0x400000   /* imm only */
#define VERT_BEGIN           0x800000   /* imm only, but tested in vb code */
#define VERT_END             0x1000000  /* imm only, but tested in vb code */
#define VERT_END_VB          0x2000000  /* imm only, but tested in vb code */
#define VERT_POINT_SIZE      0x4000000  /* vb only, could reuse a bit */
#define VERT_EYE             VERT_BEGIN /* vb only, reuse imm bit */
#define VERT_CLIP            VERT_END   /* vb only, reuse imm bit*/


/* Flags for IM->TexCoordSize.  Enough flags for 16 units.
 */
#define TEX_0_SIZE_3          0x1
#define TEX_0_SIZE_4          0x1001
#define TEX_SIZE_3(unit)      (TEX_0_SIZE_3<<unit)
#define TEX_SIZE_4(unit)      (TEX_0_SIZE_4<<unit)


/* Shorthands.
 */
#define VERT_EVAL_ANY      (VERT_EVAL_C1|VERT_EVAL_P1|	\
                            VERT_EVAL_C2|VERT_EVAL_P2)

#define VERT_OBJ_23       (VERT_OBJ_3|VERT_OBJ)
#define VERT_OBJ_234      (VERT_OBJ_4|VERT_OBJ_23)

#define VERT_TEX0_SHIFT 11

#define VERT_TEX(i)        (VERT_TEX0 << i)

#define VERT_TEX_ANY       (VERT_TEX0 |		\
                            VERT_TEX1 |		\
                            VERT_TEX2 |		\
                            VERT_TEX3 |		\
                            VERT_TEX4 |		\
                            VERT_TEX5 |		\
                            VERT_TEX6 |		\
                            VERT_TEX7)

#define VERT_FIXUP        (VERT_TEX_ANY |	\
                           VERT_RGBA |		\
                           VERT_SPEC_RGB |	\
                           VERT_FOG_COORD |	\
			   VERT_INDEX |		\
                           VERT_EDGE |		\
                           VERT_NORM)

#define VERT_CURRENT_DATA  (VERT_FIXUP |	\
			    VERT_MATERIAL)

#define VERT_DATA          (VERT_TEX_ANY |	\
			    VERT_RGBA |		\
			    VERT_SPEC_RGB |	\
			    VERT_FOG_COORD |	\
                            VERT_INDEX |	\
                            VERT_EDGE |		\
                            VERT_NORM |		\
	                    VERT_OBJ |	\
                            VERT_MATERIAL |	\
                            VERT_ELT |		\
	                    VERT_EVAL_ANY)


/* KW: Represents everything that can take place between a begin and
 * end, and can represent multiple begin/end pairs.  Can be used to
 * losslessly encode this information in display lists.
 */
struct immediate
{
   struct __GLcontextRec *backref;
   GLuint id, ref_count;

   /* This must be saved when immediates are shared in display lists.
    */
   GLuint CopyStart, Start, Count;
   GLuint LastData;		/* count or count+1 */
   GLuint AndFlag, OrFlag;
   GLuint TexSize;		/* keep track of texcoord sizes */
   GLuint BeginState, SavedBeginState;
   GLuint LastPrimitive;

   GLuint ArrayEltFlags;	/* precalc'ed for glArrayElt */
   GLuint ArrayEltIncr;
   GLuint ArrayEltFlush;

#define FLUSH_ELT_EAGER 0x1
#define FLUSH_ELT_LAZY 0x2
   GLuint FlushElt;

   GLuint MaxTextureUnits;	/* precalc'ed for glMultiTexCoordARB */

   /* Temporary values created when vertices are copied into the
    * first 3 slots of the struct:
    */
   GLuint CopyOrFlag;
   GLuint CopyAndFlag;
   GLuint CopyTexSize;


   /* allocate storage for these on demand:
    */
   struct gl_material (*Material)[2];
   GLuint *MaterialMask;
   GLuint LastMaterial;
   GLuint MaterialOrMask;
   GLuint MaterialAndMask;

   GLfloat (*TexCoord[MAX_TEXTURE_UNITS])[4];

   GLuint  Primitive[IMM_SIZE];	    /* BEGIN/END */
   GLuint  PrimitiveLength[IMM_SIZE]; /* BEGIN/END */
   GLuint  Flag[IMM_SIZE];	    /* VERT_* flags */
   GLfloat Color[IMM_SIZE][4];
   GLfloat Obj[IMM_SIZE][4];
   GLfloat Normal[IMM_SIZE][3];
   GLfloat TexCoord0[IMM_SIZE][4];  /* just VERT_TEX0 */
   GLuint  Elt[IMM_SIZE];
   GLubyte EdgeFlag[IMM_SIZE];
   GLuint  Index[IMM_SIZE];
   GLfloat SecondaryColor[IMM_SIZE][4];
   GLfloat FogCoord[IMM_SIZE];
};


struct vertex_arrays
{
   GLvector4f  Obj;
   GLvector3f  Normal;
   struct gl_client_array Color;
   struct gl_client_array SecondaryColor;
   GLvector1ui Index;
   GLvector1ub EdgeFlag;
   GLvector4f  TexCoord[MAX_TEXTURE_UNITS];
   GLvector1ui Elt;
   GLvector1f  FogCoord;
};


typedef struct gl_material GLmaterial;

/* Contains the current state of a running pipeline.
 */
typedef struct vertex_buffer
{
   /* Constant over life of the vertex_buffer.
    */
   GLuint Size;

   /* Constant over the pipeline.
    */
   GLuint     Count;		                /* for everything except Elts */
   GLuint     FirstClipped;	                /* temp verts for clipping */
   GLuint     FirstPrimitive;	                /* usually zero */

   /* Pointers to current data.
    */
   GLuint      *Elts;		                /* VERT_ELT */
   GLvector4f  *ObjPtr;		                /* VERT_OBJ */
   GLvector4f  *EyePtr;		                /* VERT_EYE */
   GLvector4f  *ClipPtr;	                /* VERT_CLIP */
   GLvector4f  *ProjectedClipPtr;               /* VERT_CLIP (2) */
   GLubyte     ClipOrMask;	                /* VERT_CLIP (3) */
   GLubyte     *ClipMask;		        /* VERT_CLIP (4) */
   GLvector3f  *NormalPtr;	                /* VERT_NORM */
   GLboolean   *EdgeFlag;	                /* VERT_EDGE */
   GLvector4f  *TexCoordPtr[MAX_TEXTURE_UNITS];	/* VERT_TEX_0..n */
   GLvector1ui *IndexPtr[2];	                /* VERT_INDEX */
   struct gl_client_array *ColorPtr[2];	                /* VERT_RGBA */
   struct gl_client_array *SecondaryColorPtr[2];         /* VERT_SPEC_RGB */
   GLvector1f  *FogCoordPtr;	                /* VERT_FOG_COORD */
   GLvector1f  *PointSizePtr;	                /* VERT_POINT_SIZE */
   GLmaterial (*Material)[2];                   /* VERT_MATERIAL, optional */
   GLuint      *MaterialMask;	                /* VERT_MATERIAL, optional */
   GLuint      *Flag;		                /* VERT_* flags, optional */
   GLuint      *Primitive;	                /* GL_(mode)|PRIM_* flags */
   GLuint      *PrimitiveLength;	        /* integers */


   GLuint importable_data;
   void *import_source;
   void (*import_data)( GLcontext *ctx, GLuint flags, GLuint vecflags );
   /* Callback to the provider of the untransformed input for the
    * render stage (or other stages) to call if they need to write into
    * write-protected arrays, or fixup the stride on input arrays.
    *
    * This is currently only necessary for client arrays that make it
    * as far down the pipeline as the render stage.
    */

   GLuint LastClipped;
   /* Private data from _tnl_render_stage that has no business being
    * in this struct.
    */

} TNLvertexbuffer;



/* Describes an individual operation on the pipeline.
 */
struct gl_pipeline_stage {
   const char *name;
   GLuint check_state;		/* All state referenced in check() --
				 * When is the pipeline_stage struct
				 * itself invalidated?  Must be
				 * constant.
				 */

   /* Usually constant or set by the 'check' callback:
    */
   GLuint run_state;		/* All state referenced in run() --
				 * When is the cached output of the
				 * stage invalidated?
				 */

   GLboolean active;		/* True if runnable in current state */
   GLuint inputs;		/* VERT_* inputs to the stage */
   GLuint outputs;		/* VERT_* outputs of the stage */

   /* Set in _tnl_run_pipeline():
    */
   GLuint changed_inputs;	/* Generated value -- inputs to the
				 * stage that have changed since last
				 * call to 'run'.
				 */

   /* Private data for the pipeline stage:
    */
   void *privatePtr;

   /* Free private data.  May not be null.
    */
   void (*destroy)( struct gl_pipeline_stage * );

   /* Called from _tnl_validate_pipeline().  Must update all fields in
    * the pipeline_stage struct for the current state.
    */
   void (*check)( GLcontext *ctx, struct gl_pipeline_stage * );

   /* Called from _tnl_run_pipeline().  The stage.changed_inputs value
    * encodes all inputs to thee struct which have changed.  If
    * non-zero, recompute all affected outputs of the stage, otherwise
    * execute any 'sideeffects' of the stage.
    *
    * Return value: GL_TRUE - keep going
    *               GL_FALSE - finished pipeline
    */
   GLboolean (*run)( GLcontext *ctx, struct gl_pipeline_stage * );
};


struct gl_pipeline {
   GLuint build_state_trigger;	  /* state changes which require build */
   GLuint build_state_changes;    /* state changes since last build */
   GLuint run_state_changes;	  /* state changes since last run */
   GLuint run_input_changes;	  /* VERT_* changes since last run */
   GLuint inputs;		  /* VERT_* inputs to pipeline */
   struct gl_pipeline_stage stages[MAX_PIPELINE_STAGES+1];
   GLuint nr_stages;
};


struct tnl_eval_store {
   GLuint EvalMap1Flags;
   GLuint EvalMap2Flags;
   GLuint EvalNewState;
   struct immediate *im;	/* used for temporary data */
};


typedef void (*points_func)( GLcontext *ctx, GLuint first, GLuint last );
typedef void (*line_func)( GLcontext *ctx, GLuint v1, GLuint v2 );
typedef void (*triangle_func)( GLcontext *ctx,
                               GLuint v1, GLuint v2, GLuint v3 );
typedef void (*quad_func)( GLcontext *ctx, GLuint v1, GLuint v2,
                           GLuint v3, GLuint v4 );
typedef void (*render_func)( GLcontext *ctx, GLuint start, GLuint count,
			     GLuint flags );
typedef void (*interp_func)( GLcontext *ctx,
			     GLfloat t, GLuint dst, GLuint out, GLuint in,
			     GLboolean force_boundary );
typedef void (*copy_pv_func)( GLcontext *ctx, GLuint dst, GLuint src );


struct tnl_device_driver {
   /***
    *** TNL Pipeline
    ***/

   void (*RunPipeline)(GLcontext *ctx);
   /* Replaces PipelineStart/PipelineFinish -- intended to allow
    * drivers to wrap _tnl_run_pipeline() with code to validate state
    * and grab/release hardware locks.  
    */

   /***
    *** Rendering
    ***/

   void (*RenderStart)(GLcontext *ctx);
   void (*RenderFinish)(GLcontext *ctx);
   /* Called before and after all rendering operations, including DrawPixels,
    * ReadPixels, Bitmap, span functions, and CopyTexImage, etc commands.
    * These are a suitable place for grabbing/releasing hardware locks.
    */

   void (*RenderPrimitive)(GLcontext *ctx, GLenum mode);
   /* Called between RednerStart() and RenderFinish() to indicate the
    * type of primitive we're about to draw.  Mode will be one of the
    * modes accepted by glBegin().
    */

   interp_func RenderInterp;
   /* The interp function is called by the clipping routines when we need
    * to generate an interpolated vertex.  All pertinant vertex ancilliary
    * data should be computed by interpolating between the 'in' and 'out'
    * vertices.
    */

   copy_pv_func RenderCopyPV;
   /* The copy function is used to make a copy of a vertex.  All pertinant
    * vertex attributes should be copied.
    */

   void (*RenderClippedPolygon)( GLcontext *ctx, const GLuint *elts, GLuint n );
   /* Render a polygon with <n> vertices whose indexes are in the <elts>
    * array.
    */

   void (*RenderClippedLine)( GLcontext *ctx, GLuint v0, GLuint v1 );
   /* Render a line between the two vertices given by indexes v0 and v1. */

   points_func           PointsFunc; /* must now respect vb->elts */
   line_func             LineFunc;
   triangle_func         TriangleFunc;
   quad_func             QuadFunc;
   /* These functions are called in order to render points, lines,
    * triangles and quads.  These are only called via the T&L module.
    */

   render_func          *RenderTabVerts;
   render_func          *RenderTabElts;
   /* Render whole unclipped primitives (points, lines, linestrips,
    * lineloops, etc).  The tables are indexed by the GL enum of the
    * primitive to be rendered.  RenderTabVerts is used for non-indexed
    * arrays of vertices.  RenderTabElts is used for indexed arrays of
    * vertices.
    */

   void (*ResetLineStipple)( GLcontext *ctx );
   /* Reset the hardware's line stipple counter.
    */

   void (*BuildProjectedVertices)( GLcontext *ctx,
				   GLuint start, GLuint end,
				   GLuint new_inputs);
   /* This function is called whenever new vertices are required for
    * rendering.  The vertices in question are those n such that start
    * <= n < end.  The new_inputs parameter indicates those fields of
    * the vertex which need to be updated, if only a partial repair of
    * the vertex is required.
    *
    * This function is called only from _tnl_render_stage in tnl/t_render.c.
    */


   GLboolean (*MultipassFunc)( GLcontext *ctx, GLuint passno );
   /* Driver may request additional render passes by returning GL_TRUE
    * when this function is called.  This function will be called
    * after the first pass, and passes will be made until the function
    * returns GL_FALSE.  If no function is registered, only one pass
    * is made.
    *
    * This function will be first invoked with passno == 1.
    */
};


typedef struct {

   /* Driver interface.
    */
   struct tnl_device_driver Driver;

   /* Track whether the module is active.
    */
   GLboolean bound_exec;

   /* Display list extensions
    */
   GLuint opcode_vertex_cassette;

   /* Pipeline
    */
   struct gl_pipeline pipeline;
   struct vertex_buffer vb;

   /* GLvectors for binding to vb:
    */
   struct vertex_arrays imm_inputs;
   struct vertex_arrays array_inputs;
   GLuint *tmp_primitive;
   GLuint *tmp_primitive_length;

   /* Set when executing an internally generated begin/end object.  If
    * such an object is encountered in a display list, it will be
    * replayed only if the list is outside any existing begin/end
    * objects.  
    */
   GLboolean ReplayHardBeginEnd;

   /* Note which vertices need copying over succesive immediates.
    * Will add save versions to precompute vertex copying where
    * possible.
    */
   struct immediate *ExecCopySource;
   GLuint ExecCopyCount;
   GLuint ExecCopyElts[IMM_MAX_COPIED_VERTS];
   GLuint ExecCopyTexSize;
   GLuint ExecParity;

   GLuint DlistPrimitive;
   GLuint DlistPrimitiveLength;
   GLuint DlistLastPrimitive;

   /* Cache a single free immediate (refcount == 0)
    */
   struct immediate *freed_immediate;   

   /* Probably need a better configuration mechanism:
    */
   GLboolean NeedProjCoords;
   GLboolean LoopbackDListCassettes;

   /* Derived state and storage for _tnl_eval_vb:
    */
   struct tnl_eval_store eval;

   /* Functions to be plugged into dispatch when tnl is active.
    */
   GLvertexformat vtxfmt;

} TNLcontext;



#define TNL_CONTEXT(ctx) ((TNLcontext *)(ctx->swtnl_context))
#define TNL_CURRENT_IM(ctx) ((struct immediate *)(ctx->swtnl_im))


#define TYPE_IDX(t) ((t) & 0xf)
#define MAX_TYPES TYPE_IDX(GL_DOUBLE)+1      /* 0xa + 1 */

extern void _tnl_MakeCurrent( GLcontext *ctx,
			      GLframebuffer *drawBuffer,
			      GLframebuffer *readBuffer );


/*
 * Macros for fetching current input buffer.
 */
#ifdef THREADS
#define GET_IMMEDIATE  struct immediate *IM = TNL_CURRENT_IM(((GLcontext *) (_glapi_Context ? _glapi_Context : _glapi_get_context())))
#define SET_IMMEDIATE(ctx, im)  ctx->swtnl_im = (void *)im
#else
extern struct immediate *_tnl_CurrentInput;
#define GET_IMMEDIATE struct immediate *IM = _tnl_CurrentInput
#define SET_IMMEDIATE(ctx, im)			\
do {						\
   ctx->swtnl_im = (void *)im;			\
   _tnl_CurrentInput = im;			\
} while (0)
#endif


#endif
