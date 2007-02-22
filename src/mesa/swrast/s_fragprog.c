/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "program.h"

#include "s_fragprog.h"
#include "s_span.h"
#include "slang_library_noise.h"


/* See comments below for info about this */
#define LAMBDA_ZERO 1

/* debug predicate */
#define DEBUG_FRAG 0


/**
 * Virtual machine state used during execution of a fragment programs.
 */
struct fp_machine
{
   /** Fragment Input attributes */
   GLfloat (*Attribs)[MAX_WIDTH][4];
   GLuint CurFrag; /**< Index into Attribs arrays */

   GLfloat Temporaries[MAX_PROGRAM_TEMPS][4];
   GLfloat Outputs[FRAG_RESULT_MAX][4];
   GLuint CondCodes[4];  /**< COND_* value for x/y/z/w */

   GLuint CallStack[MAX_PROGRAM_CALL_DEPTH]; /**< For CAL/RET instructions */
   GLuint StackDepth; /**< Index/ptr to top of CallStack[] */
};


#if FEATURE_MESA_program_debug
static struct fp_machine *CurrentMachine = NULL;

/**
 * For GL_MESA_program_debug.
 * Return current value (4*GLfloat) of a fragment program register.
 * Called via ctx->Driver.GetFragmentProgramRegister().
 */
void
_swrast_get_program_register(GLcontext *ctx, enum register_file file,
                             GLuint index, GLfloat val[4])
{
   if (CurrentMachine) {
      switch (file) {
      case PROGRAM_INPUT:
         COPY_4V(val, CurrentMachine->Attribs[index][CurrentMachine->CurFrag]);
         break;
      case PROGRAM_OUTPUT:
         COPY_4V(val, CurrentMachine->Outputs[index]);
         break;
      case PROGRAM_TEMPORARY:
         COPY_4V(val, CurrentMachine->Temporaries[index]);
         break;
      default:
         _mesa_problem(NULL,
                       "bad register file in _swrast_get_program_register");
      }
   }
}
#endif /* FEATURE_MESA_program_debug */


/**
 * Fetch a texel.
 */
static void
fetch_texel( GLcontext *ctx, const GLfloat texcoord[4], GLfloat lambda,
             GLuint unit, GLfloat color[4] )
{
   GLchan rgba[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* XXX use a float-valued TextureSample routine here!!! */
   swrast->TextureSample[unit](ctx, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}


/**
 * Fetch a texel with the given partial derivatives to compute a level
 * of detail in the mipmap.
 */
static void
fetch_texel_deriv( GLcontext *ctx, const GLfloat texcoord[4],
                   const GLfloat texdx[4], const GLfloat texdy[4],
                   GLuint unit, GLfloat color[4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;
   const struct gl_texture_image *texImg = texObj->Image[0][texObj->BaseLevel];
   const GLfloat texW = (GLfloat) texImg->WidthScale;
   const GLfloat texH = (GLfloat) texImg->HeightScale;
   GLchan rgba[4];

   GLfloat lambda = _swrast_compute_lambda(texdx[0], texdy[0], /* ds/dx, ds/dy */
                                         texdx[1], texdy[1], /* dt/dx, dt/dy */
                                         texdx[3], texdy[2], /* dq/dx, dq/dy */
                                         texW, texH,
                                         texcoord[0], texcoord[1], texcoord[3],
                                         1.0F / texcoord[3]);

   swrast->TextureSample[unit](ctx, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}


/**
 * Return a pointer to the 4-element float vector specified by the given
 * source register.
 */
static INLINE const GLfloat *
get_register_pointer( GLcontext *ctx,
                      const struct prog_src_register *source,
                      const struct fp_machine *machine,
                      const struct gl_fragment_program *program )
{
   /* XXX relative addressing... */
   switch (source->File) {
   case PROGRAM_TEMPORARY:
      ASSERT(source->Index < MAX_PROGRAM_TEMPS);
      return machine->Temporaries[source->Index];
   case PROGRAM_INPUT:
      ASSERT(source->Index < FRAG_ATTRIB_MAX);
      return machine->Attribs[source->Index][machine->CurFrag];
   case PROGRAM_OUTPUT:
      /* This is only for PRINT */
      ASSERT(source->Index < FRAG_RESULT_MAX);
      return machine->Outputs[source->Index];
   case PROGRAM_LOCAL_PARAM:
      ASSERT(source->Index < MAX_PROGRAM_LOCAL_PARAMS);
      return program->Base.LocalParams[source->Index];
   case PROGRAM_ENV_PARAM:
      ASSERT(source->Index < MAX_PROGRAM_ENV_PARAMS);
      return ctx->FragmentProgram.Parameters[source->Index];
   case PROGRAM_STATE_VAR:
      /* Fallthrough */
   case PROGRAM_CONSTANT:
      /* Fallthrough */
   case PROGRAM_UNIFORM:
      /* Fallthrough */
   case PROGRAM_NAMED_PARAM:
      ASSERT(source->Index < (GLint) program->Base.Parameters->NumParameters);
      return program->Base.Parameters->ParameterValues[source->Index];
   default:
      _mesa_problem(ctx, "Invalid input register file %d in fp "
                    "get_register_pointer", source->File);
      return NULL;
   }
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static void
fetch_vector4( GLcontext *ctx,
               const struct prog_src_register *source,
               const struct fp_machine *machine,
               const struct gl_fragment_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
   ASSERT(src);

   if (source->Swizzle == SWIZZLE_NOOP) {
      /* no swizzling */
      COPY_4V(result, src);
   }
   else {
      ASSERT(GET_SWZ(source->Swizzle, 0) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 1) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 2) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 3) <= 3);
      result[0] = src[GET_SWZ(source->Swizzle, 0)];
      result[1] = src[GET_SWZ(source->Swizzle, 1)];
      result[2] = src[GET_SWZ(source->Swizzle, 2)];
      result[3] = src[GET_SWZ(source->Swizzle, 3)];
   }

   if (source->NegateBase) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
      result[1] = FABSF(result[1]);
      result[2] = FABSF(result[2]);
      result[3] = FABSF(result[3]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
}


/**
 * Fetch the derivative with respect to X for the given register.
 * \return GL_TRUE if it was easily computed or GL_FALSE if we
 * need to execute another instance of the program (ugh)!
 */
static GLboolean
fetch_vector4_deriv( GLcontext *ctx,
                     const struct prog_src_register *source,
                     const SWspan *span,
                     char xOrY, GLint column, GLfloat result[4] )
{
   GLfloat src[4];

   ASSERT(xOrY == 'X' || xOrY == 'Y');

   switch (source->Index) {
   case FRAG_ATTRIB_WPOS:
      if (xOrY == 'X') {
         src[0] = 1.0;
         src[1] = 0.0;
         src[2] = span->attrStepX[FRAG_ATTRIB_WPOS][2]
                / ctx->DrawBuffer->_DepthMaxF;
         src[3] = span->attrStepX[FRAG_ATTRIB_WPOS][3];
      }
      else {
         src[0] = 0.0;
         src[1] = 1.0;
         src[2] = span->attrStepY[FRAG_ATTRIB_WPOS][2]
                / ctx->DrawBuffer->_DepthMaxF;
         src[3] = span->attrStepY[FRAG_ATTRIB_WPOS][3];
      }
      break;
   case FRAG_ATTRIB_COL0:
   case FRAG_ATTRIB_COL1:
      if (xOrY == 'X') {
         src[0] = span->attrStepX[source->Index][0] * (1.0F / CHAN_MAXF);
         src[1] = span->attrStepX[source->Index][1] * (1.0F / CHAN_MAXF);
         src[2] = span->attrStepX[source->Index][2] * (1.0F / CHAN_MAXF);
         src[3] = span->attrStepX[source->Index][3] * (1.0F / CHAN_MAXF);
      }
      else {
         src[0] = span->attrStepY[source->Index][0] * (1.0F / CHAN_MAXF);
         src[1] = span->attrStepY[source->Index][1] * (1.0F / CHAN_MAXF);
         src[2] = span->attrStepY[source->Index][2] * (1.0F / CHAN_MAXF);
         src[3] = span->attrStepY[source->Index][3] * (1.0F / CHAN_MAXF);
      }
      break;
   case FRAG_ATTRIB_FOGC:
      if (xOrY == 'X') {
         src[0] = span->attrStepX[FRAG_ATTRIB_FOGC][0] * (1.0F / CHAN_MAXF);
         src[1] = 0.0;
         src[2] = 0.0;
         src[3] = 0.0;
      }
      else {
         src[0] = span->attrStepY[FRAG_ATTRIB_FOGC][0] * (1.0F / CHAN_MAXF);
         src[1] = 0.0;
         src[2] = 0.0;
         src[3] = 0.0;
      }
      break;
   default:
      assert(source->Index < FRAG_ATTRIB_MAX);
      /* texcoord or varying */
      if (xOrY == 'X') {
         /* this is a little tricky - I think I've got it right */
         const GLfloat invQ = 1.0f / (span->attrStart[source->Index][3]
                               + span->attrStepX[source->Index][3] * column);
         src[0] = span->attrStepX[source->Index][0] * invQ;
         src[1] = span->attrStepX[source->Index][1] * invQ;
         src[2] = span->attrStepX[source->Index][2] * invQ;
         src[3] = span->attrStepX[source->Index][3] * invQ;
      }
      else {
         /* Tricky, as above, but in Y direction */
         const GLfloat invQ = 1.0f / (span->attrStart[source->Index][3]
                                      + span->attrStepY[source->Index][3]);
         src[0] = span->attrStepY[source->Index][0] * invQ;
         src[1] = span->attrStepY[source->Index][1] * invQ;
         src[2] = span->attrStepY[source->Index][2] * invQ;
         src[3] = span->attrStepY[source->Index][3] * invQ;
      }
      break;
   }

   result[0] = src[GET_SWZ(source->Swizzle, 0)];
   result[1] = src[GET_SWZ(source->Swizzle, 1)];
   result[2] = src[GET_SWZ(source->Swizzle, 2)];
   result[3] = src[GET_SWZ(source->Swizzle, 3)];

   if (source->NegateBase) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
      result[1] = FABSF(result[1]);
      result[2] = FABSF(result[2]);
      result[3] = FABSF(result[3]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
   return GL_TRUE;
}


/**
 * As above, but only return result[0] element.
 */
static void
fetch_vector1( GLcontext *ctx,
               const struct prog_src_register *source,
               const struct fp_machine *machine,
               const struct gl_fragment_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
   ASSERT(src);

   result[0] = src[GET_SWZ(source->Swizzle, 0)];

   if (source->NegateBase) {
      result[0] = -result[0];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
   }
}


/**
 * Test value against zero and return GT, LT, EQ or UN if NaN.
 */
static INLINE GLuint
generate_cc( float value )
{
   if (value != value)
      return COND_UN;  /* NaN */
   if (value > 0.0F)
      return COND_GT;
   if (value < 0.0F)
      return COND_LT;
   return COND_EQ;
}


/**
 * Test if the ccMaskRule is satisfied by the given condition code.
 * Used to mask destination writes according to the current condition code.
 */
static INLINE GLboolean
test_cc(GLuint condCode, GLuint ccMaskRule)
{
   switch (ccMaskRule) {
   case COND_EQ: return (condCode == COND_EQ);
   case COND_NE: return (condCode != COND_EQ);
   case COND_LT: return (condCode == COND_LT);
   case COND_GE: return (condCode == COND_GT || condCode == COND_EQ);
   case COND_LE: return (condCode == COND_LT || condCode == COND_EQ);
   case COND_GT: return (condCode == COND_GT);
   case COND_TR: return GL_TRUE;
   case COND_FL: return GL_FALSE;
   default:      return GL_TRUE;
   }
}


/**
 * Evaluate the 4 condition codes against a predicate and return GL_TRUE
 * or GL_FALSE to indicate result.
 */
static INLINE GLboolean
eval_condition(const struct fp_machine *machine,
               const struct prog_instruction *inst)
{
   const GLuint swizzle = inst->DstReg.CondSwizzle;
   const GLuint condMask = inst->DstReg.CondMask;
   if (test_cc(machine->CondCodes[GET_SWZ(swizzle, 0)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 1)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 2)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 3)], condMask)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/**
 * Store 4 floats into a register.  Observe the instructions saturate and
 * set-condition-code flags.
 */
static void
store_vector4( const struct prog_instruction *inst,
               struct fp_machine *machine,
               const GLfloat value[4] )
{
   const struct prog_dst_register *dest = &(inst->DstReg);
   const GLboolean clamp = inst->SaturateMode == SATURATE_ZERO_ONE;
   GLfloat *dstReg;
   GLfloat dummyReg[4];
   GLfloat clampedValue[4];
   GLuint writeMask = dest->WriteMask;

   switch (dest->File) {
      case PROGRAM_OUTPUT:
         dstReg = machine->Outputs[dest->Index];
         break;
      case PROGRAM_TEMPORARY:
         dstReg = machine->Temporaries[dest->Index];
         break;
      case PROGRAM_WRITE_ONLY:
         dstReg = dummyReg;
         return;
      default:
         _mesa_problem(NULL, "bad register file in store_vector4(fp)");
         return;
   }

#if 0
   if (value[0] > 1.0e10 ||
       IS_INF_OR_NAN(value[0]) ||
       IS_INF_OR_NAN(value[1]) ||
       IS_INF_OR_NAN(value[2]) ||
       IS_INF_OR_NAN(value[3])  )
      printf("store %g %g %g %g\n", value[0], value[1], value[2], value[3]);
#endif

   if (clamp) {
      clampedValue[0] = CLAMP(value[0], 0.0F, 1.0F);
      clampedValue[1] = CLAMP(value[1], 0.0F, 1.0F);
      clampedValue[2] = CLAMP(value[2], 0.0F, 1.0F);
      clampedValue[3] = CLAMP(value[3], 0.0F, 1.0F);
      value = clampedValue;
   }

   if (dest->CondMask != COND_TR) {
      /* condition codes may turn off some writes */
      if (writeMask & WRITEMASK_X) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 0)],
                      dest->CondMask))
            writeMask &= ~WRITEMASK_X;
      }
      if (writeMask & WRITEMASK_Y) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 1)],
                      dest->CondMask))
            writeMask &= ~WRITEMASK_Y;
      }
      if (writeMask & WRITEMASK_Z) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 2)],
                      dest->CondMask))
            writeMask &= ~WRITEMASK_Z;
      }
      if (writeMask & WRITEMASK_W) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 3)],
                      dest->CondMask))
            writeMask &= ~WRITEMASK_W;
      }
   }

   if (writeMask & WRITEMASK_X)
      dstReg[0] = value[0];
   if (writeMask & WRITEMASK_Y)
      dstReg[1] = value[1];
   if (writeMask & WRITEMASK_Z)
      dstReg[2] = value[2];
   if (writeMask & WRITEMASK_W)
      dstReg[3] = value[3];

   if (inst->CondUpdate) {
      if (writeMask & WRITEMASK_X)
         machine->CondCodes[0] = generate_cc(value[0]);
      if (writeMask & WRITEMASK_Y)
         machine->CondCodes[1] = generate_cc(value[1]);
      if (writeMask & WRITEMASK_Z)
         machine->CondCodes[2] = generate_cc(value[2]);
      if (writeMask & WRITEMASK_W)
         machine->CondCodes[3] = generate_cc(value[3]);
   }
}


/**
 * Initialize a new machine state instance from an existing one, adding
 * the partial derivatives onto the input registers.
 * Used to implement DDX and DDY instructions in non-trivial cases.
 */
static void
init_machine_deriv( GLcontext *ctx,
                    const struct fp_machine *machine,
                    const struct gl_fragment_program *program,
                    const SWspan *span, char xOrY,
                    struct fp_machine *dMachine )
{
   GLuint attr;

   ASSERT(xOrY == 'X' || xOrY == 'Y');

   /* copy existing machine */
   _mesa_memcpy(dMachine, machine, sizeof(struct fp_machine));

   if (program->Base.Target == GL_FRAGMENT_PROGRAM_NV) {
      /* XXX also need to do this when using valgrind */
      /* Clear temporary registers (undefined for ARB_f_p) */
      _mesa_bzero( (void*) machine->Temporaries,
                   MAX_PROGRAM_TEMPS * 4 * sizeof(GLfloat));
   }

   /* Add derivatives */
   if (program->Base.InputsRead & FRAG_BIT_WPOS) {
      GLfloat *wpos = machine->Attribs[FRAG_ATTRIB_WPOS][machine->CurFrag];
      if (xOrY == 'X') {
         wpos[0] += 1.0F;
         wpos[1] += 0.0F;
         wpos[2] += span->attrStepX[FRAG_ATTRIB_WPOS][2];
         wpos[3] += span->attrStepX[FRAG_ATTRIB_WPOS][3];
      }
      else {
         wpos[0] += 0.0F;
         wpos[1] += 1.0F;
         wpos[2] += span->attrStepY[FRAG_ATTRIB_WPOS][2];
         wpos[3] += span->attrStepY[FRAG_ATTRIB_WPOS][3];
      }
   }

   /* primary, secondary colors */
   for (attr = FRAG_ATTRIB_COL0; attr <= FRAG_ATTRIB_COL1; attr++) {
      if (program->Base.InputsRead & (1 << attr)) {
         GLfloat *col = machine->Attribs[attr][machine->CurFrag];
         if (xOrY == 'X') {
            col[0] += span->attrStepX[attr][0] * (1.0F / CHAN_MAXF);
            col[1] += span->attrStepX[attr][1] * (1.0F / CHAN_MAXF);
            col[2] += span->attrStepX[attr][2] * (1.0F / CHAN_MAXF);
            col[3] += span->attrStepX[attr][3] * (1.0F / CHAN_MAXF);
         }
         else {
            col[0] += span->attrStepY[attr][0] * (1.0F / CHAN_MAXF);
            col[1] += span->attrStepY[attr][1] * (1.0F / CHAN_MAXF);
            col[2] += span->attrStepY[attr][2] * (1.0F / CHAN_MAXF);
            col[3] += span->attrStepY[attr][3] * (1.0F / CHAN_MAXF);
         }
      }
   }
   if (program->Base.InputsRead & FRAG_BIT_FOGC) {
      GLfloat *fogc = machine->Attribs[FRAG_ATTRIB_FOGC][machine->CurFrag];
      if (xOrY == 'X') {
         fogc[0] += span->attrStepX[FRAG_ATTRIB_FOGC][0];
      }
      else {
         fogc[0] += span->attrStepY[FRAG_ATTRIB_FOGC][0];
      }
   }
   /* texcoord and varying vars */
   for (attr = FRAG_ATTRIB_TEX0; attr < FRAG_ATTRIB_MAX; attr++) {
      if (program->Base.InputsRead & (1 << attr)) {
         GLfloat *val = machine->Attribs[attr][machine->CurFrag];
         /* XXX perspective-correct interpolation */
         if (xOrY == 'X') {
            val[0] += span->attrStepX[attr][0];
            val[1] += span->attrStepX[attr][1];
            val[2] += span->attrStepX[attr][2];
            val[3] += span->attrStepX[attr][3];
         }
         else {
            val[0] += span->attrStepY[attr][0];
            val[1] += span->attrStepY[attr][1];
            val[2] += span->attrStepY[attr][2];
            val[3] += span->attrStepY[attr][3];
         }
      }
   }

   /* init condition codes */
   dMachine->CondCodes[0] = COND_EQ;
   dMachine->CondCodes[1] = COND_EQ;
   dMachine->CondCodes[2] = COND_EQ;
   dMachine->CondCodes[3] = COND_EQ;
}


/**
 * Execute the given vertex program.
 * NOTE: we do everything in single-precision floating point; we don't
 * currently observe the single/half/fixed-precision qualifiers.
 * \param ctx - rendering context
 * \param program - the fragment program to execute
 * \param machine - machine state (register file)
 * \param maxInst - max number of instructions to execute
 * \return GL_TRUE if program completed or GL_FALSE if program executed KIL.
 */
static GLboolean
execute_program( GLcontext *ctx,
                 const struct gl_fragment_program *program, GLuint maxInst,
                 struct fp_machine *machine, const SWspan *span,
                 GLuint column )
{
   const GLuint MAX_EXEC = 10000;
   GLint pc, total = 0;

   if (DEBUG_FRAG) {
      printf("execute fragment program --------------------\n");
   }

   for (pc = 0; pc < maxInst; pc++) {
      const struct prog_instruction *inst = program->Base.Instructions + pc;

      if (ctx->FragmentProgram.CallbackEnabled &&
          ctx->FragmentProgram.Callback) {
         ctx->FragmentProgram.CurrentPosition = inst->StringPos;
         ctx->FragmentProgram.Callback(program->Base.Target,
                                       ctx->FragmentProgram.CallbackData);
      }

      if (DEBUG_FRAG) {
         _mesa_print_instruction(inst);
      }

      switch (inst->Opcode) {
         case OPCODE_ABS:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = FABSF(a[0]);
               result[1] = FABSF(a[1]);
               result[2] = FABSF(a[2]);
               result[3] = FABSF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_ADD:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] + b[0];
               result[1] = a[1] + b[1];
               result[2] = a[2] + b[2];
               result[3] = a[3] + b[3];
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("ADD (%g %g %g %g) = (%g %g %g %g) + (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3], 
                         a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3]);
               }
            }
            break;
         case OPCODE_BGNLOOP:
            /* no-op */
            break;
         case OPCODE_ENDLOOP:
            /* subtract 1 here since pc is incremented by for(pc) loop */
            pc = inst->BranchTarget - 1; /* go to matching BNGLOOP */
            break;
         case OPCODE_BGNSUB: /* begin subroutine */
            break;
         case OPCODE_ENDSUB: /* end subroutine */
            break;
         case OPCODE_BRA: /* branch (conditional) */
            /* fall-through */
         case OPCODE_BRK: /* break out of loop (conditional) */
            /* fall-through */
         case OPCODE_CONT: /* continue loop (conditional) */
            if (eval_condition(machine, inst)) {
               /* take branch */
               /* Subtract 1 here since we'll do pc++ at end of for-loop */
               pc = inst->BranchTarget - 1;
            }
            break;
         case OPCODE_CAL: /* Call subroutine (conditional) */
            if (eval_condition(machine, inst)) {
               /* call the subroutine */
               if (machine->StackDepth >= MAX_PROGRAM_CALL_DEPTH) {
                  return GL_TRUE; /* Per GL_NV_vertex_program2 spec */
               }
               machine->CallStack[machine->StackDepth++] = pc + 1;
               pc = inst->BranchTarget; /* XXX - 1 ??? */
            }
            break;
         case OPCODE_CMP:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] < 0.0F ? b[0] : c[0];
               result[1] = a[1] < 0.0F ? b[1] : c[1];
               result[2] = a[2] < 0.0F ? b[2] : c[2];
               result[3] = a[3] < 0.0F ? b[3] : c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_COS:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3]
                  = (GLfloat) _mesa_cos(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_DDX: /* Partial derivative with respect to X */
            {
               GLfloat a[4], aNext[4], result[4];
               struct fp_machine dMachine;
               if (!fetch_vector4_deriv(ctx, &inst->SrcReg[0], span, 'X',
                                        column, result)) {
                  /* This is tricky.  Make a copy of the current machine state,
                   * increment the input registers by the dx or dy partial
                   * derivatives, then re-execute the program up to the
                   * preceeding instruction, then fetch the source register.
                   * Finally, find the difference in the register values for
                   * the original and derivative runs.
                   */
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  init_machine_deriv(ctx, machine, program, span,
                                     'X', &dMachine);
                  execute_program(ctx, program, pc, &dMachine, span, column);
                  fetch_vector4( ctx, &inst->SrcReg[0], &dMachine, program, aNext );
                  result[0] = aNext[0] - a[0];
                  result[1] = aNext[1] - a[1];
                  result[2] = aNext[2] - a[2];
                  result[3] = aNext[3] - a[3];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_DDY: /* Partial derivative with respect to Y */
            {
               GLfloat a[4], aNext[4], result[4];
               struct fp_machine dMachine;
               if (!fetch_vector4_deriv(ctx, &inst->SrcReg[0], span, 'Y',
                                        column, result)) {
                  init_machine_deriv(ctx, machine, program, span,
                                     'Y', &dMachine);
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  execute_program(ctx, program, pc, &dMachine, span, column);
                  fetch_vector4( ctx, &inst->SrcReg[0], &dMachine, program, aNext );
                  result[0] = aNext[0] - a[0];
                  result[1] = aNext[1] - a[1];
                  result[2] = aNext[2] - a[2];
                  result[3] = aNext[3] - a[3];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_DP3:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] = DOT3(a, b);
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("DP3 %g = (%g %g %g) . (%g %g %g)\n",
                         result[0], a[0], a[1], a[2], b[0], b[1], b[2]);
               }
            }
            break;
         case OPCODE_DP4:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] = DOT4(a,b);
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("DP4 %g = (%g, %g %g %g) . (%g, %g %g %g)\n",
                         result[0], a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3]);
               }
            }
            break;
         case OPCODE_DPH:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] = 
                  a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_DST: /* Distance vector */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = 1.0F;
               result[1] = a[1] * b[1];
               result[2] = a[2];
               result[3] = b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_EX2: /* Exponential base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3] =
                  (GLfloat) _mesa_pow(2.0, a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_FLR:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = FLOORF(a[0]);
               result[1] = FLOORF(a[1]);
               result[2] = FLOORF(a[2]);
               result[3] = FLOORF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_FRC:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = a[0] - FLOORF(a[0]);
               result[1] = a[1] - FLOORF(a[1]);
               result[2] = a[2] - FLOORF(a[2]);
               result[3] = a[3] - FLOORF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_IF:
            if (eval_condition(machine, inst)) {
               /* do if-clause (just continue execution) */
            }
            else {
               /* go to the instruction after ELSE or ENDIF */
               assert(inst->BranchTarget >= 0);
               pc = inst->BranchTarget - 1;
            }
            break;
         case OPCODE_ELSE:
            /* goto ENDIF */
            assert(inst->BranchTarget >= 0);
            pc = inst->BranchTarget - 1;
            break;
         case OPCODE_ENDIF:
            /* nothing */
            break;
         case OPCODE_INT: /* float to int */
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = (GLfloat) (GLint) a[0];
               result[1] = (GLfloat) (GLint) a[1];
               result[2] = (GLfloat) (GLint) a[2];
               result[3] = (GLfloat) (GLint) a[3];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_KIL_NV: /* NV_f_p only (conditional) */
            if (eval_condition(machine, inst)) {
               return GL_FALSE;
            }
            break;
         case OPCODE_KIL: /* ARB_f_p only */
            {
               GLfloat a[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               if (a[0] < 0.0F || a[1] < 0.0F || a[2] < 0.0F || a[3] < 0.0F) {
                  return GL_FALSE;
               }
            }
            break;
         case OPCODE_LG2:  /* log base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3] = LOG2(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_LIT:
            {
               const GLfloat epsilon = 1.0F / 256.0F; /* from NV VP spec */
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = MAX2(a[0], 0.0F);
               a[1] = MAX2(a[1], 0.0F);
               /* XXX ARB version clamps a[3], NV version doesn't */
               a[3] = CLAMP(a[3], -(128.0F - epsilon), (128.0F - epsilon));
               result[0] = 1.0F;
               result[1] = a[0];
               /* XXX we could probably just use pow() here */
               if (a[0] > 0.0F) {
                  if (a[1] == 0.0 && a[3] == 0.0)
                     result[2] = 1.0;
                  else
                     result[2] = EXPF(a[3] * LOGF(a[1]));
               }
               else {
                  result[2] = 0.0;
               }
               result[3] = 1.0F;
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("LIT (%g %g %g %g) : (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3],
                         a[0], a[1], a[2], a[3]);
               }
            }
            break;
         case OPCODE_LRP:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] * b[0] + (1.0F - a[0]) * c[0];
               result[1] = a[1] * b[1] + (1.0F - a[1]) * c[1];
               result[2] = a[2] * b[2] + (1.0F - a[2]) * c[2];
               result[3] = a[3] * b[3] + (1.0F - a[3]) * c[3];
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("LRP (%g %g %g %g) = (%g %g %g %g), "
                         "(%g %g %g %g), (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3],
                         a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3],
                         c[0], c[1], c[2], c[3]);
               }
            }
            break;
         case OPCODE_MAD:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] * b[0] + c[0];
               result[1] = a[1] * b[1] + c[1];
               result[2] = a[2] * b[2] + c[2];
               result[3] = a[3] * b[3] + c[3];
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("MAD (%g %g %g %g) = (%g %g %g %g) * "
                         "(%g %g %g %g) + (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3],
                         a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3],
                         c[0], c[1], c[2], c[3]);
               }
            }
            break;
         case OPCODE_MAX:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = MAX2(a[0], b[0]);
               result[1] = MAX2(a[1], b[1]);
               result[2] = MAX2(a[2], b[2]);
               result[3] = MAX2(a[3], b[3]);
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("MAX (%g %g %g %g) = (%g %g %g %g), (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3], 
                         a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3]);
               }
            }
            break;
         case OPCODE_MIN:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = MIN2(a[0], b[0]);
               result[1] = MIN2(a[1], b[1]);
               result[2] = MIN2(a[2], b[2]);
               result[3] = MIN2(a[3], b[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_MOV:
            {
               GLfloat result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, result );
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("MOV (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3]);
               }
            }
            break;
         case OPCODE_MUL:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] * b[0];
               result[1] = a[1] * b[1];
               result[2] = a[2] * b[2];
               result[3] = a[3] * b[3];
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("MUL (%g %g %g %g) = (%g %g %g %g) * (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3], 
                         a[0], a[1], a[2], a[3],
                         b[0], b[1], b[2], b[3]);
               }
            }
            break;
         case OPCODE_NOISE1:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] =
               result[1] =
               result[2] =
               result[3] = _slang_library_noise1(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_NOISE2:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] =
               result[1] =
               result[2] =
               result[3] = _slang_library_noise2(a[0], a[1]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_NOISE3:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] =
               result[1] =
               result[2] =
               result[3] = _slang_library_noise3(a[0], a[1], a[2]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_NOISE4:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] =
               result[1] =
               result[2] =
               result[3] = _slang_library_noise4(a[0], a[1], a[2], a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_NOP:
            break;
         case OPCODE_PK2H: /* pack two 16-bit floats in one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLhalfNV hx, hy;
               GLuint *rawResult = (GLuint *) result;
               GLuint twoHalves;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               hx = _mesa_float_to_half(a[0]);
               hy = _mesa_float_to_half(a[1]);
               twoHalves = hx | (hy << 16);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = twoHalves;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_PK2US: /* pack two GLushorts into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint usx, usy, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[1], 0.0F, 1.0F);
               usx = IROUND(a[0] * 65535.0F);
               usy = IROUND(a[1] * 65535.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = usx | (usy << 16);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_PK4B: /* pack four GLbytes into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], -128.0F / 127.0F, 1.0F);
               a[1] = CLAMP(a[1], -128.0F / 127.0F, 1.0F);
               a[2] = CLAMP(a[2], -128.0F / 127.0F, 1.0F);
               a[3] = CLAMP(a[3], -128.0F / 127.0F, 1.0F);
               ubx = IROUND(127.0F * a[0] + 128.0F);
               uby = IROUND(127.0F * a[1] + 128.0F);
               ubz = IROUND(127.0F * a[2] + 128.0F);
               ubw = IROUND(127.0F * a[3] + 128.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_PK4UB: /* pack four GLubytes into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[1], 0.0F, 1.0F);
               a[2] = CLAMP(a[2], 0.0F, 1.0F);
               a[3] = CLAMP(a[3], 0.0F, 1.0F);
               ubx = IROUND(255.0F * a[0]);
               uby = IROUND(255.0F * a[1]);
               ubz = IROUND(255.0F * a[2]);
               ubw = IROUND(255.0F * a[3]);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_POW:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector1( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3]
                  = (GLfloat)_mesa_pow(a[0], b[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_RCP:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               if (DEBUG_FRAG) {
                  if (a[0] == 0)
                     printf("RCP(0)\n");
                  else if (IS_INF_OR_NAN(a[0]))
                     printf("RCP(inf)\n");
               }
               result[0] = result[1] = result[2] = result[3] = 1.0F / a[0];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_RET: /* return from subroutine (conditional) */
            if (eval_condition(machine, inst)) {
               if (machine->StackDepth == 0) {
                  return GL_TRUE; /* Per GL_NV_vertex_program2 spec */
               }
               pc = machine->CallStack[--machine->StackDepth];
            }
            break;
         case OPCODE_RFL: /* reflection vector */
            {
               GLfloat axis[4], dir[4], result[4], tmpX, tmpW;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, axis );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, dir );
               tmpW = DOT3(axis, axis);
               tmpX = (2.0F * DOT3(axis, dir)) / tmpW;
               result[0] = tmpX * axis[0] - dir[0];
               result[1] = tmpX * axis[1] - dir[1];
               result[2] = tmpX * axis[2] - dir[2];
               /* result[3] is never written! XXX enforce in parser! */
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_RSQ: /* 1 / sqrt() */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = FABSF(a[0]);
               result[0] = result[1] = result[2] = result[3] = INV_SQRTF(a[0]);
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("RSQ %g = 1/sqrt(|%g|)\n", result[0], a[0]);
               }
            }
            break;
         case OPCODE_SCS: /* sine and cos */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = (GLfloat) _mesa_cos(a[0]);
               result[1] = (GLfloat) _mesa_sin(a[0]);
               result[2] = 0.0;  /* undefined! */
               result[3] = 0.0;  /* undefined! */
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SEQ: /* set on equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] == b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] == b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] == b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] == b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SFL: /* set false, operands ignored */
            {
               static const GLfloat result[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SGE: /* set on greater or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] >= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] >= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] >= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] >= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SGT: /* set on greater */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] > b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] > b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] > b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] > b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("SGT %g %g %g %g\n",
                         result[0], result[1], result[2], result[3]);
               }
            }
            break;
         case OPCODE_SIN:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3]
                  = (GLfloat) _mesa_sin(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SLE: /* set on less or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] <= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] <= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] <= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] <= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SLT: /* set on less */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] < b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] < b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] < b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] < b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SNE: /* set on not equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] != b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] != b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] != b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] != b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_STR: /* set true, operands ignored */
            {
               static const GLfloat result[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_SUB:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] - b[0];
               result[1] = a[1] - b[1];
               result[2] = a[2] - b[2];
               result[3] = a[3] - b[3];
               store_vector4( inst, machine, result );
               if (DEBUG_FRAG) {
                  printf("SUB (%g %g %g %g) = (%g %g %g %g) - (%g %g %g %g)\n",
                         result[0], result[1], result[2], result[3],
                         a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
               }
            }
            break;
         case OPCODE_SWZ: /* extended swizzle */
            {
               const struct prog_src_register *source = &inst->SrcReg[0];
               const GLfloat *src = get_register_pointer(ctx, source,
                                                         machine, program);
               GLfloat result[4];
               GLuint i;
               for (i = 0; i < 4; i++) {
                  const GLuint swz = GET_SWZ(source->Swizzle, i);
                  if (swz == SWIZZLE_ZERO)
                     result[i] = 0.0;
                  else if (swz == SWIZZLE_ONE)
                     result[i] = 1.0;
                  else {
                     ASSERT(swz >= 0);
                     ASSERT(swz <= 3);
                     result[i] = src[swz];
                  }
                  if (source->NegateBase & (1 << i))
                     result[i] = -result[i];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_TEX: /* Both ARB and NV frag prog */
            /* Texel lookup */
            {
               /* Note: only use the precomputed lambda value when we're
                * sampling texture unit [K] with texcoord[K].
                * Otherwise, the lambda value may have no relation to the
                * instruction's texcoord or texture image.  Using the wrong
                * lambda is usually bad news.
                * The rest of the time, just use zero (until we get a more
                * sophisticated way of computing lambda).
                */
               GLfloat coord[4], color[4], lambda;
               if (inst->SrcReg[0].File == PROGRAM_INPUT &&
                   inst->SrcReg[0].Index == FRAG_ATTRIB_TEX0+inst->TexSrcUnit)
                  lambda = span->array->lambda[inst->TexSrcUnit][column];
               else
                  lambda = 0.0;
               fetch_vector4(ctx, &inst->SrcReg[0], machine, program, coord);
               fetch_texel( ctx, coord, lambda, inst->TexSrcUnit, color );
               if (DEBUG_FRAG) {
                  printf("TEX (%g, %g, %g, %g) = texture[%d][%g, %g, %g, %g], "
                         "lod %f\n",
                         color[0], color[1], color[2], color[3],
                         inst->TexSrcUnit,
                         coord[0], coord[1], coord[2], coord[3], lambda);
               }
               store_vector4( inst, machine, color );
            }
            break;
         case OPCODE_TXB: /* GL_ARB_fragment_program only */
            /* Texel lookup with LOD bias */
            {
               const struct gl_texture_unit *texUnit
                  = &ctx->Texture.Unit[inst->TexSrcUnit];
               GLfloat coord[4], color[4], lambda, bias;
               if (inst->SrcReg[0].File == PROGRAM_INPUT &&
                   inst->SrcReg[0].Index == FRAG_ATTRIB_TEX0+inst->TexSrcUnit)
                  lambda = span->array->lambda[inst->TexSrcUnit][column];
               else
                  lambda = 0.0;
               fetch_vector4(ctx, &inst->SrcReg[0], machine, program, coord);
               /* coord[3] is the bias to add to lambda */
               bias = texUnit->LodBias + coord[3];
               if (texUnit->_Current)
                  bias += texUnit->_Current->LodBias;
               fetch_texel(ctx, coord, lambda + bias, inst->TexSrcUnit, color);
               store_vector4( inst, machine, color );
            }
            break;
         case OPCODE_TXD: /* GL_NV_fragment_program only */
            /* Texture lookup w/ partial derivatives for LOD */
            {
               GLfloat texcoord[4], dtdx[4], dtdy[4], color[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, dtdx );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, dtdy );
               fetch_texel_deriv( ctx, texcoord, dtdx, dtdy, inst->TexSrcUnit,
                                  color );
               store_vector4( inst, machine, color );
            }
            break;
         case OPCODE_TXP: /* GL_ARB_fragment_program only */
            /* Texture lookup w/ projective divide */
            {
               GLfloat texcoord[4], color[4], lambda;
               if (inst->SrcReg[0].File == PROGRAM_INPUT &&
                   inst->SrcReg[0].Index == FRAG_ATTRIB_TEX0+inst->TexSrcUnit)
                  lambda = span->array->lambda[inst->TexSrcUnit][column];
               else
                  lambda = 0.0;
               fetch_vector4(ctx, &inst->SrcReg[0], machine, program,texcoord);
	       /* Not so sure about this test - if texcoord[3] is
		* zero, we'd probably be fine except for an ASSERT in
		* IROUND_POS() which gets triggered by the inf values created.
		*/
	       if (texcoord[3] != 0.0) {
		  texcoord[0] /= texcoord[3];
		  texcoord[1] /= texcoord[3];
		  texcoord[2] /= texcoord[3];
	       }
               fetch_texel( ctx, texcoord, lambda, inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case OPCODE_TXP_NV: /* GL_NV_fragment_program only */
            /* Texture lookup w/ projective divide */
            {
               GLfloat texcoord[4], color[4], lambda;
               if (inst->SrcReg[0].File == PROGRAM_INPUT &&
                   inst->SrcReg[0].Index == FRAG_ATTRIB_TEX0+inst->TexSrcUnit)
                  lambda = span->array->lambda[inst->TexSrcUnit][column];
               else
                  lambda = 0.0;
               fetch_vector4(ctx, &inst->SrcReg[0], machine, program,texcoord);
               if (inst->TexSrcTarget != TEXTURE_CUBE_INDEX &&
		   texcoord[3] != 0.0) {
                  texcoord[0] /= texcoord[3];
                  texcoord[1] /= texcoord[3];
                  texcoord[2] /= texcoord[3];
               }
               fetch_texel( ctx, texcoord, lambda, inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case OPCODE_UP2H: /* unpack two 16-bit floats */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLhalfNV hx, hy;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               hx = rawBits[0] & 0xffff;
               hy = rawBits[0] >> 16;
               result[0] = result[2] = _mesa_half_to_float(hx);
               result[1] = result[3] = _mesa_half_to_float(hy);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_UP2US: /* unpack two GLushorts */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLushort usx, usy;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               usx = rawBits[0] & 0xffff;
               usy = rawBits[0] >> 16;
               result[0] = result[2] = usx * (1.0f / 65535.0f);
               result[1] = result[3] = usy * (1.0f / 65535.0f);
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_UP4B: /* unpack four GLbytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = (((rawBits[0] >>  0) & 0xff) - 128) / 127.0F;
               result[1] = (((rawBits[0] >>  8) & 0xff) - 128) / 127.0F;
               result[2] = (((rawBits[0] >> 16) & 0xff) - 128) / 127.0F;
               result[3] = (((rawBits[0] >> 24) & 0xff) - 128) / 127.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_UP4UB: /* unpack four GLubytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = ((rawBits[0] >>  0) & 0xff) / 255.0F;
               result[1] = ((rawBits[0] >>  8) & 0xff) / 255.0F;
               result[2] = ((rawBits[0] >> 16) & 0xff) / 255.0F;
               result[3] = ((rawBits[0] >> 24) & 0xff) / 255.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_XPD: /* cross product */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[1] * b[2] - a[2] * b[1];
               result[1] = a[2] * b[0] - a[0] * b[2];
               result[2] = a[0] * b[1] - a[1] * b[0];
               result[3] = 1.0;
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_X2D: /* 2-D matrix transform */
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] + b[0] * c[0] + b[1] * c[1];
               result[1] = a[1] + b[0] * c[2] + b[1] * c[3];
               result[2] = a[2] + b[0] * c[0] + b[1] * c[1];
               result[3] = a[3] + b[0] * c[2] + b[1] * c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case OPCODE_PRINT:
            {
               if (inst->SrcReg[0].File != -1) {
                  GLfloat a[4];
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  _mesa_printf("%s%g, %g, %g, %g\n", (const char *) inst->Data,
                               a[0], a[1], a[2], a[3]);
               }
               else {
                  _mesa_printf("%s\n", (const char *) inst->Data);
               }
            }
            break;
         case OPCODE_END:
            return GL_TRUE;
         default:
            _mesa_problem(ctx, "Bad opcode %d in _mesa_exec_fragment_program",
                          inst->Opcode);
            return GL_TRUE; /* return value doesn't matter */

      }
      total++;
      if (total > MAX_EXEC) {
         _mesa_problem(ctx, "Infinite loop detected in fragment program");
         return GL_TRUE;
         abort();
      }
   }
   return GL_TRUE;
}


/**
 * Initialize the virtual fragment program machine state prior to running
 * fragment program on a fragment.  This involves initializing the input
 * registers, condition codes, etc.
 * \param machine  the virtual machine state to init
 * \param program  the fragment program we're about to run
 * \param span  the span of pixels we'll operate on
 * \param col  which element (column) of the span we'll operate on
 */
static void
init_machine( GLcontext *ctx, struct fp_machine *machine,
              const struct gl_fragment_program *program,
              const SWspan *span, GLuint col )
{
   GLuint inputsRead = program->Base.InputsRead;

   if (ctx->FragmentProgram.CallbackEnabled)
      inputsRead = ~0;

   if (program->Base.Target == GL_FRAGMENT_PROGRAM_NV) {
      /* Clear temporary registers (undefined for ARB_f_p) */
      _mesa_bzero(machine->Temporaries,
                  MAX_PROGRAM_TEMPS * 4 * sizeof(GLfloat));
   }

   /* Setup pointer to input attributes */
   machine->Attribs = span->array->attribs;
   machine->CurFrag = col;

   /* init condition codes */
   machine->CondCodes[0] = COND_EQ;
   machine->CondCodes[1] = COND_EQ;
   machine->CondCodes[2] = COND_EQ;
   machine->CondCodes[3] = COND_EQ;

   /* init call stack */
   machine->StackDepth = 0;
}


/**
 * Run fragment program on the pixels in span from 'start' to 'end' - 1.
 */
static void
run_program(GLcontext *ctx, SWspan *span, GLuint start, GLuint end)
{
   const struct gl_fragment_program *program = ctx->FragmentProgram._Current;
   struct fp_machine machine;
   GLuint i;

   CurrentMachine = &machine;

   for (i = start; i < end; i++) {
      if (span->array->mask[i]) {
         init_machine(ctx, &machine, program, span, i);

         if (execute_program(ctx, program, program->Base.NumInstructions,
                             &machine, span, i)) {
            /* Store result color */
            COPY_4V(span->array->attribs[FRAG_ATTRIB_COL0][i],
                    machine.Outputs[FRAG_RESULT_COLR]);

            /* Store result depth/z */
            if (program->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR)) {
               const GLfloat depth = machine.Outputs[FRAG_RESULT_DEPR][2];
               if (depth <= 0.0)
                  span->array->z[i] = 0;
               else if (depth >= 1.0)
                  span->array->z[i] = ctx->DrawBuffer->_DepthMax;
               else
                  span->array->z[i] = IROUND(depth * ctx->DrawBuffer->_DepthMaxF);
            }
         }
         else {
            /* killed fragment */
            span->array->mask[i] = GL_FALSE;
            span->writeAll = GL_FALSE;
         }
      }
   }

   CurrentMachine = NULL;
}


/**
 * Execute the current fragment program for all the fragments
 * in the given span.
 */
void
_swrast_exec_fragment_program( GLcontext *ctx, SWspan *span )
{
   const struct gl_fragment_program *program = ctx->FragmentProgram._Current;

   /* incoming colors should be floats */
   ASSERT(span->array->ChanType == GL_FLOAT);

   ctx->_CurrentProgram = GL_FRAGMENT_PROGRAM_ARB; /* or NV, doesn't matter */

   run_program(ctx, span, 0, span->end);

   if (program->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR)) {
      span->interpMask &= ~SPAN_Z;
      span->arrayMask |= SPAN_Z;
   }

   ctx->_CurrentProgram = 0;
}

