/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/**
 * \file nvvertexec.c
 * \brief Code to execute vertex programs.
 * \author Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvvertexec.h"
#include "nvvertprog.h"
#include "math/m_matrix.h"


static const GLfloat zeroVec[4] = { 0, 0, 0, 0 };


/**
 * Load/initialize the vertex program registers.
 * This needs to be done per vertex.
 */
void
_mesa_init_vp_registers(GLcontext *ctx)
{
   struct vp_machine *machine = &(ctx->VertexProgram.Machine);
   GLuint i;

   /* Input registers get initialized from the current vertex attribs */
   MEMCPY(machine->Registers[VP_INPUT_REG_START],
          ctx->Current.Attrib,
          16 * 4 * sizeof(GLfloat));

   /* Output and temp regs are initialized to [0,0,0,1] */
   for (i = VP_OUTPUT_REG_START; i <= VP_OUTPUT_REG_END; i++) {
      machine->Registers[i][0] = 0.0F;
      machine->Registers[i][1] = 0.0F;
      machine->Registers[i][2] = 0.0F;
      machine->Registers[i][3] = 1.0F;
   }
   for (i = VP_TEMP_REG_START; i <= VP_TEMP_REG_END; i++) {
      machine->Registers[i][0] = 0.0F;
      machine->Registers[i][1] = 0.0F;
      machine->Registers[i][2] = 0.0F;
      machine->Registers[i][3] = 1.0F;
   }

   /* The program regs aren't touched */
}



/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
   GLuint i;
   pos += VP_PROG_REG_START;
   for (i = 0; i < 4; i++) {
      registers[pos + i][0] = mat[0 + i];
      registers[pos + i][1] = mat[4 + i];
      registers[pos + i][2] = mat[8 + i];
      registers[pos + i][3] = mat[12 + i];
   }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
                      const GLfloat mat[16])
{
   pos += VP_PROG_REG_START;
   MEMCPY(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load all currently tracked matrices into the program registers.
 * This needs to be done per glBegin/glEnd.
 */
void
_mesa_init_tracked_matrices(GLcontext *ctx)
{
   GLuint i;

   for (i = 0; i < VP_NUM_PROG_REGS / 4; i++) {
      /* point 'mat' at source matrix */
      GLmatrix *mat;
      if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
         mat = ctx->ModelviewMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
         mat = ctx->ProjectionMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
         mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
         mat = ctx->ColorMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
         /* XXX verify the combined matrix is up to date */
         mat = &ctx->_ModelProjectMatrix;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
               ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
         GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
         ASSERT(n < MAX_PROGRAM_MATRICES);
         mat = ctx->ProgramMatrixStack[n].Top;
      }
      else {
         /* no matrix is tracked, but we leave the register values as-is */
         assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
         continue;
      }

      /* load the matrix */
      if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
         load_matrix(ctx->VertexProgram.Machine.Registers, i*4, mat->m);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
         _math_matrix_analyse(mat); /* update the inverse */
         assert((mat->flags & MAT_DIRTY_INVERSE) == 0);
         load_matrix(ctx->VertexProgram.Machine.Registers, i*4, mat->inv);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
         load_transpose_matrix(ctx->VertexProgram.Machine.Registers, i*4, mat->m);
      }
      else {
         assert(ctx->VertexProgram.TrackMatrixTransform[i]
                == GL_INVERSE_TRANSPOSE_NV);
         _math_matrix_analyse(mat); /* update the inverse */
         assert((mat->flags & MAT_DIRTY_INVERSE) == 0);
         load_transpose_matrix(ctx->VertexProgram.Machine.Registers,
                               i*4, mat->inv);
      }
   }
}



/**
 * For debugging.  Dump the current vertex program machine registers.
 */
void
_mesa_dump_vp_machine( const struct vp_machine *machine )
{
   int i;
   _mesa_printf("VertexIn:\n");
   for (i = 0; i < VP_NUM_INPUT_REGS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
             machine->Registers[i + VP_INPUT_REG_START][0],
             machine->Registers[i + VP_INPUT_REG_START][1],
             machine->Registers[i + VP_INPUT_REG_START][2],
             machine->Registers[i + VP_INPUT_REG_START][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("VertexOut:\n");
   for (i = 0; i < VP_NUM_OUTPUT_REGS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
             machine->Registers[i + VP_OUTPUT_REG_START][0],
             machine->Registers[i + VP_OUTPUT_REG_START][1],
             machine->Registers[i + VP_OUTPUT_REG_START][2],
             machine->Registers[i + VP_OUTPUT_REG_START][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Registers:\n");
   for (i = 0; i < VP_NUM_TEMP_REGS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
             machine->Registers[i + VP_TEMP_REG_START][0],
             machine->Registers[i + VP_TEMP_REG_START][1],
             machine->Registers[i + VP_TEMP_REG_START][2],
             machine->Registers[i + VP_TEMP_REG_START][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Parameters:\n");
   for (i = 0; i < VP_NUM_PROG_REGS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
             machine->Registers[i + VP_PROG_REG_START][0],
             machine->Registers[i + VP_PROG_REG_START][1],
             machine->Registers[i + VP_PROG_REG_START][2],
             machine->Registers[i + VP_PROG_REG_START][3]);
   }
   _mesa_printf("\n");
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static void
fetch_vector4( const struct vp_src_register *source,
               const struct vp_machine *machine,
               GLfloat result[4] )
{
   const GLfloat *src;

   if (source->RelAddr) {
      const GLint reg = source->Register + machine->AddressReg;
      if (reg < 0 || reg > MAX_NV_VERTEX_PROGRAM_PARAMS)
         src = zeroVec;
      else
         src = machine->Registers[VP_PROG_REG_START + reg];
   }
   else {
      src = machine->Registers[source->Register];
   }

   if (source->Negate) {
      result[0] = -src[source->Swizzle[0]];
      result[1] = -src[source->Swizzle[1]];
      result[2] = -src[source->Swizzle[2]];
      result[3] = -src[source->Swizzle[3]];
   }
   else {
      result[0] = src[source->Swizzle[0]];
      result[1] = src[source->Swizzle[1]];
      result[2] = src[source->Swizzle[2]];
      result[3] = src[source->Swizzle[3]];
   }
}


/**
 * As above, but only return result[0] element.
 */
static void
fetch_vector1( const struct vp_src_register *source,
               const struct vp_machine *machine,
               GLfloat result[4] )
{
   const GLfloat *src;

   if (source->RelAddr) {
      const GLint reg = source->Register + machine->AddressReg;
      if (reg < 0 || reg > MAX_NV_VERTEX_PROGRAM_PARAMS)
         src = zeroVec;
      else
         src = machine->Registers[VP_PROG_REG_START + reg];
   }
   else {
      src = machine->Registers[source->Register];
   }

   if (source->Negate) {
      result[0] = -src[source->Swizzle[0]];
   }
   else {
      result[0] = src[source->Swizzle[0]];
   }
}


/**
 * Store 4 floats into a register.
 */
static void
store_vector4( const struct vp_dst_register *dest, struct vp_machine *machine,
               const GLfloat value[4] )
{
   GLfloat *dst = machine->Registers[dest->Register];

   if (dest->WriteMask[0])
      dst[0] = value[0];
   if (dest->WriteMask[1])
      dst[1] = value[1];
   if (dest->WriteMask[2])
      dst[2] = value[2];
   if (dest->WriteMask[3])
      dst[3] = value[3];
}


/**
 * Set x to positive or negative infinity.
 */
#ifdef USE_IEEE
#define SET_POS_INFINITY(x)  ( *((GLuint *) &x) = 0x7F800000 )
#define SET_NEG_INFINITY(x)  ( *((GLuint *) &x) = 0xFF800000 )
#elif defined(VMS)
#define SET_POS_INFINITY(x)  x = __MAXFLOAT
#define SET_NEG_INFINITY(x)  x = -__MAXFLOAT
#else
#define SET_POS_INFINITY(x)  x = (GLfloat) HUGE_VAL
#define SET_NEG_INFINITY(x)  x = (GLfloat) -HUGE_VAL
#endif

#define SET_FLOAT_BITS(x, bits) ((fi_type *) &(x))->i = bits


/**
 * Execute the given vertex program
 */
void
_mesa_exec_vertex_program(GLcontext *ctx, const struct vertex_program *program)
{
   struct vp_machine *machine = &ctx->VertexProgram.Machine;
   const struct vp_instruction *inst;

   for (inst = program->Instructions; inst->Opcode != VP_OPCODE_END; inst++) {
      switch (inst->Opcode) {
         case VP_OPCODE_MOV:
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_LIT:
            {
               const GLfloat epsilon = 1.0e-5F; /* XXX fix? */
               GLfloat t[4], lit[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               if (t[3] < -(128.0F - epsilon))
                   t[3] = - (128.0F - epsilon);
               else if (t[3] > 128.0F - epsilon)
                  t[3] = 128.0F - epsilon;
               if (t[0] < 0.0)
                  t[0] = 0.0;
               if (t[1] < 0.0)
                  t[1] = 0.0;
               lit[0] = 1.0;
               lit[1] = t[0];
               lit[2] = (t[0] > 0.0) ? (GLfloat) exp(t[3] * log(t[1])) : 0.0F;
               lit[3] = 1.0;
               store_vector4( &inst->DstReg, machine, lit );
            }
            break;
         case VP_OPCODE_RCP:
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], machine, t );
               if (t[0] != 1.0F)
                  t[0] = 1.0F / t[0];  /* div by zero is infinity! */
               t[1] = t[2] = t[3] = t[0];
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_RSQ:
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], machine, t );
               t[0] = INV_SQRTF(FABSF(t[0]));
               t[1] = t[2] = t[3] = t[0];
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_EXP:
            {
               GLfloat t[4], q[4], floor_t0;
               fetch_vector1( &inst->SrcReg[0], machine, t );
               floor_t0 = (float) floor(t[0]);
               if (floor_t0 > FLT_MAX_EXP) {
                  SET_POS_INFINITY(q[0]);
                  SET_POS_INFINITY(q[2]);
               }
               else if (floor_t0 < FLT_MIN_EXP) {
                  q[0] = 0.0F;
                  q[2] = 0.0F;
               }
               else {
#ifdef USE_IEEE
                  GLint ii = (GLint) floor_t0;
                  ii = (ii < 23) + 0x3f800000;
                  SET_FLOAT_BITS(q[0], ii);
                  q[0] = *((GLfloat *) &ii);
#else
                  q[0] = (GLfloat) pow(2.0, floor_t0);
#endif
                  q[2] = (GLfloat) (q[0] * LOG2(q[1]));
               }
               q[1] = t[0] - floor_t0;
               q[3] = 1.0F;
               store_vector4( &inst->DstReg, machine, q );
            }
            break;
         case VP_OPCODE_LOG:
            {
               GLfloat t[4], q[4], abs_t0;
               fetch_vector1( &inst->SrcReg[0], machine, t );
               abs_t0 = (GLfloat) fabs(t[0]);
               if (abs_t0 != 0.0F) {
                  /* Since we really can't handle infinite values on VMS
                   * like other OSes we'll use __MAXFLOAT to represent
                   * infinity.  This may need some tweaking.
                   */
#ifdef VMS
                  if (abs_t0 == __MAXFLOAT) {
#else
                  if (IS_INF_OR_NAN(abs_t0)) {
#endif
                     SET_POS_INFINITY(q[0]);
                     q[1] = 1.0F;
                     SET_POS_INFINITY(q[2]);
                  }
                  else {
                     int exponent;
                     double mantissa = frexp(t[0], &exponent);
                     q[0] = (GLfloat) (exponent - 1);
                     q[1] = (GLfloat) (2.0 * mantissa); /* map [.5, 1) -> [1, 2) */
                     q[2] = (GLfloat) (q[0] + LOG2(q[1]));
                  }
               }
               else {
                  SET_NEG_INFINITY(q[0]);
                  q[1] = 1.0F;
                  SET_NEG_INFINITY(q[2]);
               }
               q[3] = 1.0;
               store_vector4( &inst->DstReg, machine, q );
            }
            break;
         case VP_OPCODE_MUL:
            {
               GLfloat t[4], u[4], prod[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               prod[0] = t[0] * u[0];
               prod[1] = t[1] * u[1];
               prod[2] = t[2] * u[2];
               prod[3] = t[3] * u[3];
               store_vector4( &inst->DstReg, machine, prod );
            }
            break;
         case VP_OPCODE_ADD:
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               sum[0] = t[0] + u[0];
               sum[1] = t[1] + u[1];
               sum[2] = t[2] + u[2];
               sum[3] = t[3] + u[3];
               store_vector4( &inst->DstReg, machine, sum );
            }
            break;
         case VP_OPCODE_DP3:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, machine, dot );
            }
            break;
         case VP_OPCODE_DP4:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + t[3] * u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, machine, dot );
            }
            break;
         case VP_OPCODE_DST:
            {
               GLfloat t[4], u[4], dst[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               dst[0] = 1.0F;
               dst[1] = t[1] * u[1];
               dst[2] = t[2];
               dst[3] = u[3];
               store_vector4( &inst->DstReg, machine, dst );
            }
            break;
         case VP_OPCODE_MIN:
            {
               GLfloat t[4], u[4], min[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               min[0] = (t[0] < u[0]) ? t[0] : u[0];
               min[1] = (t[1] < u[1]) ? t[1] : u[1];
               min[2] = (t[2] < u[2]) ? t[2] : u[2];
               min[3] = (t[3] < u[3]) ? t[3] : u[3];
               store_vector4( &inst->DstReg, machine, min );
            }
            break;
         case VP_OPCODE_MAX:
            {
               GLfloat t[4], u[4], max[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               max[0] = (t[0] > u[0]) ? t[0] : u[0];
               max[1] = (t[1] > u[1]) ? t[1] : u[1];
               max[2] = (t[2] > u[2]) ? t[2] : u[2];
               max[3] = (t[3] > u[3]) ? t[3] : u[3];
               store_vector4( &inst->DstReg, machine, max );
            }
            break;
         case VP_OPCODE_SLT:
            {
               GLfloat t[4], u[4], slt[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               slt[0] = (t[0] < u[0]) ? 1.0F : 0.0F;
               slt[1] = (t[1] < u[1]) ? 1.0F : 0.0F;
               slt[2] = (t[2] < u[2]) ? 1.0F : 0.0F;
               slt[3] = (t[3] < u[3]) ? 1.0F : 0.0F;
               store_vector4( &inst->DstReg, machine, slt );
            }
            break;
         case VP_OPCODE_SGE:
            {
               GLfloat t[4], u[4], sge[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               sge[0] = (t[0] >= u[0]) ? 1.0F : 0.0F;
               sge[1] = (t[1] >= u[1]) ? 1.0F : 0.0F;
               sge[2] = (t[2] >= u[2]) ? 1.0F : 0.0F;
               sge[3] = (t[3] >= u[3]) ? 1.0F : 0.0F;
               store_vector4( &inst->DstReg, machine, sge );
            }
            break;
         case VP_OPCODE_MAD:
            {
               GLfloat t[4], u[4], v[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               fetch_vector4( &inst->SrcReg[2], machine, v );
               sum[0] = t[0] * u[0] + v[0];
               sum[1] = t[1] * u[1] + v[1];
               sum[2] = t[2] * u[2] + v[2];
               sum[3] = t[3] * u[3] + v[3];
               store_vector4( &inst->DstReg, machine, sum );
            }
            break;
         case VP_OPCODE_ARL:
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               machine->AddressReg = (GLint) floor(t[0]);
            }
            break;
         case VP_OPCODE_DPH:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, machine, dot );
            }
            break;
         case VP_OPCODE_RCC:
            {
               GLfloat t[4], u;
               fetch_vector1( &inst->SrcReg[0], machine, t );
               if (t[0] == 1.0F)
                  u = 1.0F;
               else
                  u = 1.0F / t[0];
               if (u > 0.0F) {
                  if (u > 1.884467e+019F) {
                     u = 1.884467e+019F;  /* IEEE 32-bit binary value 0x5F800000 */
                  }
                  else if (u < 5.42101e-020F) {
                     u = 5.42101e-020F;   /* IEEE 32-bit binary value 0x1F800000 */
                  }
               }
               else {
                  if (u < -1.884467e+019F) {
                     u = -1.884467e+019F; /* IEEE 32-bit binary value 0xDF800000 */
                  }
                  else if (u > -5.42101e-020F) {
                     u = -5.42101e-020F;  /* IEEE 32-bit binary value 0x9F800000 */
                  }
               }
               t[0] = t[1] = t[2] = t[3] = u;
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_SUB: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               sum[0] = t[0] - u[0];
               sum[1] = t[1] - u[1];
               sum[2] = t[2] - u[2];
               sum[3] = t[3] - u[3];
               store_vector4( &inst->DstReg, machine, sum );
            }
            break;
         case VP_OPCODE_ABS: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               if (t[0] < 0.0)  t[0] = -t[0];
               if (t[1] < 0.0)  t[1] = -t[1];
               if (t[2] < 0.0)  t[2] = -t[2];
               if (t[3] < 0.0)  t[3] = -t[3];
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_FLR: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               t[0] = FLOORF(t[0]);
               t[1] = FLOORF(t[1]);
               t[2] = FLOORF(t[2]);
               t[3] = FLOORF(t[3]);
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_FRC: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               t[0] = t[0] - FLOORF(t[0]);
               t[1] = t[1] - FLOORF(t[1]);
               t[2] = t[2] - FLOORF(t[2]);
               t[3] = t[3] - FLOORF(t[3]);
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_EX2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], machine, t );
               t[0] = t[1] = t[2] = t[3] = _mesa_pow(2.0, t[0]);
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_LG2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], machine, t );
               t[0] = t[1] = t[2] = t[3] = LOG2(t[0]);
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_POW: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4];
               fetch_vector1( &inst->SrcReg[0], machine, t );
               fetch_vector1( &inst->SrcReg[1], machine, u );
               t[0] = t[1] = t[2] = t[3] = _mesa_pow(t[0], u[0]);
               store_vector4( &inst->DstReg, machine, t );
            }
            break;
         case VP_OPCODE_XPD: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4], cross[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               fetch_vector4( &inst->SrcReg[1], machine, u );
               cross[0] = t[1] * u[2] - t[2] * u[1];
               cross[1] = t[2] * u[0] - t[0] * u[2];
               cross[2] = t[0] * u[1] - t[1] * u[0];
               store_vector4( &inst->DstReg, machine, cross );
            }
            break;
         case VP_OPCODE_SWZ: /* GL_ARB_vertex_program */
            {
               const struct vp_src_register *source = &inst->SrcReg[0];
               const GLfloat *src;
               GLfloat result[4];
               GLuint i;

               /* Code similar to fetch_vector4() */
               if (source->RelAddr) {
                  const GLint reg = source->Register + machine->AddressReg;
                  if (reg < 0 || reg > MAX_NV_VERTEX_PROGRAM_PARAMS)
                     src = zeroVec;
                  else
                     src = machine->Registers[VP_PROG_REG_START + reg];
               }
               else {
                  src = machine->Registers[source->Register];
               }

               /* extended swizzling here */
               for (i = 0; i < 3; i++) {
                  if (source->Swizzle[i] == SWIZZLE_ZERO)
                     result[i] = 0.0;
                  else if (source->Swizzle[i] == SWIZZLE_ONE)
                     result[i] = -1.0;
                  else
                     result[i] = -src[source->Swizzle[i]];
                  if (source->Negate)
                     result[i] = -result[i];
               }
               store_vector4( &inst->DstReg, machine, result );
            }
            break;

         case VP_OPCODE_END:
            return;
         default:
            /* bad instruction opcode */
            _mesa_problem(ctx, "Bad VP Opcode in _mesa_exec_vertex_program");
            return;
      }
   }
}



/**
Thoughts on vertex program optimization:

The obvious thing to do is to compile the vertex program into X86/SSE/3DNow!
assembly code.  That will probably be a lot of work.

Another approach might be to replace the vp_instruction->Opcode field with
a pointer to a specialized C function which executes the instruction.
In particular we can write functions which skip swizzling, negating,
masking, relative addressing, etc. when they're not needed.

For example:

void simple_add( struct vp_instruction *inst )
{
   GLfloat *sum = machine->Registers[inst->DstReg.Register];
   GLfloat *a = machine->Registers[inst->SrcReg[0].Register];
   GLfloat *b = machine->Registers[inst->SrcReg[1].Register];
   sum[0] = a[0] + b[0];
   sum[1] = a[1] + b[1];
   sum[2] = a[2] + b[2];
   sum[3] = a[3] + b[3];
}

*/

/*

KW:

A first step would be to 'vectorize' the programs in the same way as
the normal transformation code in the tnl module.  Thus each opcode
takes zero or more input vectors (registers) and produces one or more
output vectors.

These operations would intially be coded in C, with machine-specific
assembly following, as is currently the case for matrix
transformations in the math/ directory.  The preprocessing scheme for
selecting simpler operations Brian describes above would also work
here.

This should give reasonable performance without excessive effort.

*/
