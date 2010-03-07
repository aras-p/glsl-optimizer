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
#include "st_format.h"
#include "st_texture.h"
#include "st_inlines.h"

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "util/u_pack_color.h"


struct state_key
{
   GLuint scaleAndBias:1;
   GLuint colorMatrix:1;
   GLuint colorMatrixPostScaleBias:1;
   GLuint pixelMaps:1;

#if 0
   GLfloat Maps[3][256][4];
   int NumMaps;
   GLint NumStages;
   pipeline_stage Stages[STAGE_MAX];
   GLboolean StagesUsed[STAGE_MAX];
   GLfloat Scale1[4], Bias1[4];
   GLfloat Scale2[4], Bias2[4];
#endif
};


static GLboolean
is_identity(const GLfloat m[16])
{
   GLuint i;
   for (i = 0; i < 16; i++) {
      const int row = i % 4, col = i / 4;
      const float val = (GLfloat)(row == col);
      if (m[i] != val)
         return GL_FALSE;
   }
   return GL_TRUE;
}


static void
make_state_key(GLcontext *ctx,  struct state_key *key)
{
   static const GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
   static const GLfloat one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

   memset(key, 0, sizeof(*key));

   if (ctx->Pixel.RedBias != 0.0 || ctx->Pixel.RedScale != 1.0 ||
       ctx->Pixel.GreenBias != 0.0 || ctx->Pixel.GreenScale != 1.0 ||
       ctx->Pixel.BlueBias != 0.0 || ctx->Pixel.BlueScale != 1.0 ||
       ctx->Pixel.AlphaBias != 0.0 || ctx->Pixel.AlphaScale != 1.0) {
      key->scaleAndBias = 1;
   }

   if (!is_identity(ctx->ColorMatrixStack.Top->m)) {
      key->colorMatrix = 1;
   }

   if (!TEST_EQ_4V(ctx->Pixel.PostColorMatrixScale, one) ||
       !TEST_EQ_4V(ctx->Pixel.PostColorMatrixBias, zero)) {
      key->colorMatrixPostScaleBias = 1;
   }

   key->pixelMaps = ctx->Pixel.MapColorFlag;
}


static struct pipe_texture *
create_color_map_texture(GLcontext *ctx)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_texture *pt;
   enum pipe_format format;
   const uint texSize = 256; /* simple, and usually perfect */

   /* find an RGBA texture format */
   format = st_choose_format(pipe->screen, GL_RGBA,
                             PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_SAMPLER);

   /* create texture for color map/table */
   pt = st_texture_create(ctx->st, PIPE_TEXTURE_2D, format, 0,
                          texSize, texSize, 1, PIPE_TEXTURE_USAGE_SAMPLER);
   return pt;
}


/**
 * Update the pixelmap texture with the contents of the R/G/B/A pixel maps.
 */
static void
load_color_map_texture(GLcontext *ctx, struct pipe_texture *pt)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *transfer;
   const GLuint rSize = ctx->PixelMaps.RtoR.Size;
   const GLuint gSize = ctx->PixelMaps.GtoG.Size;
   const GLuint bSize = ctx->PixelMaps.BtoB.Size;
   const GLuint aSize = ctx->PixelMaps.AtoA.Size;
   const uint texSize = pt->width0;
   uint *dest;
   uint i, j;

   transfer = st_cond_flush_get_tex_transfer(st_context(ctx),
					     pt, 0, 0, 0, PIPE_TRANSFER_WRITE,
					     0, 0, texSize, texSize);
   dest = (uint *) screen->transfer_map(screen, transfer);

   /* Pack four 1D maps into a 2D texture:
    * R map is placed horizontally, indexed by S, in channel 0
    * G map is placed vertically, indexed by T, in channel 1
    * B map is placed horizontally, indexed by S, in channel 2
    * A map is placed vertically, indexed by T, in channel 3
    */
   for (i = 0; i < texSize; i++) {
      for (j = 0; j < texSize; j++) {
         union util_color uc;
         int k = (i * texSize + j);
         ubyte r = ctx->PixelMaps.RtoR.Map8[j * rSize / texSize];
         ubyte g = ctx->PixelMaps.GtoG.Map8[i * gSize / texSize];
         ubyte b = ctx->PixelMaps.BtoB.Map8[j * bSize / texSize];
         ubyte a = ctx->PixelMaps.AtoA.Map8[i * aSize / texSize];
         util_pack_color_ub(r, g, b, a, pt->format, &uc);
         *(dest + k) = uc.ui;
      }
   }

   screen->transfer_unmap(screen, transfer);
   screen->tex_transfer_destroy(transfer);
}



#define MAX_INST 100

/**
 * Returns a fragment program which implements the current pixel transfer ops.
 */
static struct gl_fragment_program *
get_pixel_transfer_program(GLcontext *ctx, const struct state_key *key)
{
   struct st_context *st = ctx->st;
   struct prog_instruction inst[MAX_INST];
   struct gl_program_parameter_list *params;
   struct gl_fragment_program *fp;
   GLuint ic = 0;
   const GLuint colorTemp = 0;

   fp = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!fp)
      return NULL;

   params = _mesa_new_parameter_list();

   /*
    * Get initial pixel color from the texture.
    * TEX colorTemp, fragment.texcoord[0], texture[0], 2D;
    */
   _mesa_init_instructions(inst + ic, 1);
   inst[ic].Opcode = OPCODE_TEX;
   inst[ic].DstReg.File = PROGRAM_TEMPORARY;
   inst[ic].DstReg.Index = colorTemp;
   inst[ic].SrcReg[0].File = PROGRAM_INPUT;
   inst[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   inst[ic].TexSrcUnit = 0;
   inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;
   fp->Base.InputsRead = (1 << FRAG_ATTRIB_TEX0);
   fp->Base.OutputsWritten = (1 << FRAG_RESULT_COLOR);
   fp->Base.SamplersUsed = 0x1;  /* sampler 0 (bit 0) is used */

   if (key->scaleAndBias) {
      static const gl_state_index scale_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_SCALE, 0, 0, 0 };
      static const gl_state_index bias_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_BIAS, 0, 0, 0 };
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

      scale_p = _mesa_add_state_reference(params, scale_state);
      bias_p = _mesa_add_state_reference(params, bias_state);

      /* MAD colorTemp, colorTemp, scale, bias; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MAD;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = scale_p;
      inst[ic].SrcReg[2].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[2].Index = bias_p;
      ic++;
   }

   if (key->pixelMaps) {
      const GLuint temp = 1;

      /* create the colormap/texture now if not already done */
      if (!st->pixel_xfer.pixelmap_texture) {
         st->pixel_xfer.pixelmap_texture = create_color_map_texture(ctx);
      }

      /* with a little effort, we can do four pixel map look-ups with
       * two TEX instructions:
       */

      /* TEX temp.rg, colorTemp.rgba, texture[1], 2D; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_TEX;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_XY; /* write R,G */
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].TexSrcUnit = 1;
      inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;

      /* TEX temp.ba, colorTemp.baba, texture[1], 2D; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_TEX;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_ZW; /* write B,A */
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[0].Swizzle = MAKE_SWIZZLE4(SWIZZLE_Z, SWIZZLE_W,
                                                 SWIZZLE_Z, SWIZZLE_W);
      inst[ic].TexSrcUnit = 1;
      inst[ic].TexSrcTarget = TEXTURE_2D_INDEX;
      ic++;

      /* MOV colorTemp, temp; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MOV;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = temp;
      ic++;

      fp->Base.SamplersUsed |= (1 << 1);  /* sampler 1 is used */
   }

   if (key->colorMatrix) {
      static const gl_state_index row0_state[STATE_LENGTH] =
         { STATE_COLOR_MATRIX, 0, 0, 0, 0 };
      static const gl_state_index row1_state[STATE_LENGTH] =
         { STATE_COLOR_MATRIX, 0, 1, 1, 0 };
      static const gl_state_index row2_state[STATE_LENGTH] =
         { STATE_COLOR_MATRIX, 0, 2, 2, 0 };
      static const gl_state_index row3_state[STATE_LENGTH] =
         { STATE_COLOR_MATRIX, 0, 3, 3, 0 };

      GLint row0_p = _mesa_add_state_reference(params, row0_state);
      GLint row1_p = _mesa_add_state_reference(params, row1_state);
      GLint row2_p = _mesa_add_state_reference(params, row2_state);
      GLint row3_p = _mesa_add_state_reference(params, row3_state);
      const GLuint temp = 1;

      /* DP4 temp.x, colorTemp, matrow0; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_DP4;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_X;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = row0_p;
      ic++;

      /* DP4 temp.y, colorTemp, matrow1; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_DP4;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_Y;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = row1_p;
      ic++;

      /* DP4 temp.z, colorTemp, matrow2; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_DP4;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_Z;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = row2_p;
      ic++;

      /* DP4 temp.w, colorTemp, matrow3; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_DP4;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = temp;
      inst[ic].DstReg.WriteMask = WRITEMASK_W;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = row3_p;
      ic++;

      /* MOV colorTemp, temp; */
      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MOV;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = temp;
      ic++;
   }

   if (key->colorMatrixPostScaleBias) {
      static const gl_state_index scale_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_SCALE, 0, 0, 0 };
      static const gl_state_index bias_state[STATE_LENGTH] =
         { STATE_INTERNAL, STATE_PT_BIAS, 0, 0, 0 };
      GLint scale_param, bias_param;

      scale_param = _mesa_add_state_reference(params, scale_state);
      bias_param = _mesa_add_state_reference(params, bias_state);

      _mesa_init_instructions(inst + ic, 1);
      inst[ic].Opcode = OPCODE_MAD;
      inst[ic].DstReg.File = PROGRAM_TEMPORARY;
      inst[ic].DstReg.Index = colorTemp;
      inst[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
      inst[ic].SrcReg[0].Index = colorTemp;
      inst[ic].SrcReg[1].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[1].Index = scale_param;
      inst[ic].SrcReg[2].File = PROGRAM_STATE_VAR;
      inst[ic].SrcReg[2].Index = bias_param;
      ic++;
   }

   /* Modify last instruction's dst reg to write to result.color */
   {
      struct prog_instruction *last = &inst[ic - 1];
      last->DstReg.File = PROGRAM_OUTPUT;
      last->DstReg.Index = FRAG_RESULT_COLOR;
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

#if 0
   printf("========= pixel transfer prog\n");
   _mesa_print_program(&fp->Base);
   _mesa_print_parameter_list(fp->Base.Parameters);
#endif

   return fp;
}



/**
 * Update st->pixel_xfer.program in response to new pixel-transfer state.
 */
static void
update_pixel_transfer(struct st_context *st)
{
   GLcontext *ctx = st->ctx;
   struct state_key key;
   struct gl_fragment_program *fp;

   make_state_key(st->ctx, &key);

   fp = (struct gl_fragment_program *)
      _mesa_search_program_cache(st->pixel_xfer.cache, &key, sizeof(key));
   if (!fp) {
      fp = get_pixel_transfer_program(st->ctx, &key);
      _mesa_program_cache_insert(st->ctx, st->pixel_xfer.cache,
                                 &key, sizeof(key), &fp->Base);
   }

   if (ctx->Pixel.MapColorFlag) {
      load_color_map_texture(ctx, st->pixel_xfer.pixelmap_texture);
   }
   st->pixel_xfer.pixelmap_enabled = ctx->Pixel.MapColorFlag;

   st->pixel_xfer.program = (struct st_fragment_program *) fp;
}



const struct st_tracked_state st_update_pixel_transfer = {
   "st_update_pixel_transfer",				/* name */
   {							/* dirty */
      _NEW_PIXEL | _NEW_COLOR_MATRIX,			/* mesa */
      0,						/* st */
   },
   update_pixel_transfer				/* update */
};
