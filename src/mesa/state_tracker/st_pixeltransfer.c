/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/*
 * Generate fragment programs to implement pixel transfer ops, such as
 * scale/bias, colormatrix, colortable, convolution...
 *
 * Authors:
 *   Brian Paul
 */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "st_context.h"


#define MAX_INST 100

/**
 * Returns a fragment program which implements the current pixel transfer ops.
 */
static struct gl_fragment_program *
get_pixel_transfer_program(GLcontext *ctx)
{
   struct prog_instruction inst[MAX_INST];
   struct gl_program_parameter_list *params;
   struct gl_fragment_program *fp;
   GLuint ic = 0;

   fp = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!fp)
      return NULL;

   params = _mesa_new_parameter_list();

   /* TEX result.color, fragment.texcoord[0], texture[0], 2D; */
   _mesa_init_instructions(inst + ic, 1);
   inst[ic].Opcode = OPCODE_TEX;
   inst[ic].DstReg.File = PROGRAM_OUTPUT;
   inst[ic].DstReg.Index = FRAG_RESULT_COLR;
   inst[ic].SrcReg[0].File = PROGRAM_INPUT;
   inst[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   inst[ic].TexSrcUnit = 0;
   inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;
   fp->Base.InputsRead = (1 << FRAG_ATTRIB_TEX0);
   fp->Base.OutputsWritten = (1 << FRAG_RESULT_COLR);

   /* MAD result.color, result.color, scale, bias; */
   if (ctx->Pixel.RedBias != 0.0 || ctx->Pixel.RedScale != 1.0 ||
       ctx->Pixel.GreenBias != 0.0 || ctx->Pixel.GreenScale != 1.0 ||
       ctx->Pixel.BlueBias != 0.0 || ctx->Pixel.BlueScale != 1.0 ||
       ctx->Pixel.AlphaBias != 0.0 || ctx->Pixel.AlphaScale != 1.0) {
      GLfloat scale[4], bias[4];
      GLint scale_p, bias_p;
      scale[0] = ctx->Pixel.RedScale;
      scale[1] = ctx->Pixel.GreenScale;
      scale[2] = ctx->Pixel.BlueScale;
      scale[3] = ctx->Pixel.AlphaScale;
      bias[0] = ctx->Pixel.RedBias;
      bias[1] = ctx->Pixel.GreenBias;
      bias[2] = ctx->Pixel.BlueBias;
      bias[3] = ctx->Pixel.AlphaBias;

      scale_p = _mesa_add_named_constant(params, "RGBA_scale", scale, 4);
      bias_p = _mesa_add_named_constant(params, "RGBA_bias", bias, 4);

      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MAD;
      inst[ic].DstReg.File = PROGRAM_OUTPUT;
      inst[ic].DstReg.Index = FRAG_RESULT_COLR;
      inst[ic].SrcReg[0].File = PROGRAM_OUTPUT;
      inst[ic].SrcReg[0].Index = FRAG_RESULT_COLR;
      inst[ic].SrcReg[1].File = PROGRAM_CONSTANT;
      inst[ic].SrcReg[1].Index = scale_p;
      inst[ic].SrcReg[2].File = PROGRAM_CONSTANT;
      inst[ic].SrcReg[2].Index = bias_p;
      ic++;
   }

   /* END; */
   _mesa_init_instructions(inst + ic, 1);
   inst[ic].Opcode = OPCODE_END;
   ic++;

   assert(ic <= MAX_INST);


   fp->Base.Instructions = _mesa_alloc_instructions(ic);
   if (!fp->Base.Instructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "generating pixel transfer program");
      return NULL;
   }

   _mesa_copy_instructions(fp->Base.Instructions, inst, ic);
   fp->Base.NumInstructions = ic;
   fp->Base.Parameters = params;

   printf("========= pixel transfer prog\n");
   _mesa_print_program(&fp->Base);
   _mesa_print_parameter_list(fp->Base.Parameters);

   return fp;
}



static void
update_pixel_transfer(struct st_context *st)
{
   /* XXX temporary - implement a program cache */
   GLcontext *ctx = st->ctx;
   if (st->pixel_transfer_program) {
      ctx->Driver.DeleteProgram(ctx, &st->pixel_transfer_program->Base);
   }

   st->pixel_transfer_program = get_pixel_transfer_program(ctx);
}



const struct st_tracked_state st_update_pixel_transfer = {
   .name = "st_update_pixel_transfer",
   .dirty = {
      .mesa = _NEW_PIXEL,
      .st  = 0,
   },
   .update = update_pixel_transfer
};
