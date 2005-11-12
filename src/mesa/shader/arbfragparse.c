/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

#define DEBUG_FP 0

/**
 * \file arbfragparse.c
 * ARB_fragment_program parser.
 * \author Karl Rasche
 */

#include "glheader.h"
#include "imports.h"
#include "program.h"
#include "arbprogparse.h"
#include "arbfragparse.h"


void
_mesa_parse_arb_fragment_program(GLcontext * ctx, GLenum target,
                                 const GLubyte * str, GLsizei len,
                                 struct fragment_program *program)
{
   GLuint i;
   struct arb_program ap;
   struct prog_instruction *newInstructions;
   (void) target;

   /* set the program target before parsing */
   ap.Base.Target = GL_FRAGMENT_PROGRAM_ARB;

   if (!_mesa_parse_arb_program(ctx, str, len, &ap)) {
      /* Error in the program. Just return. */
      return;
   }

   /* Copy the relevant contents of the arb_program struct into the
    * fragment_program struct.
    */
   /* copy instruction buffer */
   newInstructions = (struct prog_instruction *)
      _mesa_malloc(ap.Base.NumInstructions * sizeof(struct prog_instruction));
   if (!newInstructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return;
   }
   _mesa_memcpy(newInstructions, ap.FPInstructions,
                ap.Base.NumInstructions * sizeof(struct prog_instruction));
   if (program->Base.Instructions)
      _mesa_free(program->Base.Instructions);
   program->Base.Instructions = newInstructions;
   program->Base.String          = ap.Base.String;
   program->Base.NumInstructions = ap.Base.NumInstructions;
   program->Base.NumTemporaries  = ap.Base.NumTemporaries;
   program->Base.NumParameters   = ap.Base.NumParameters;
   program->Base.NumAttributes   = ap.Base.NumAttributes;
   program->Base.NumAddressRegs  = ap.Base.NumAddressRegs;
   program->NumAluInstructions   = ap.NumAluInstructions;
   program->NumTexInstructions   = ap.NumTexInstructions;
   program->NumTexIndirections   = ap.NumTexIndirections;
   program->Base.InputsRead      = ap.Base.InputsRead;
   program->Base.OutputsWritten  = ap.Base.OutputsWritten;
   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
      program->TexturesUsed[i] = ap.TexturesUsed[i];

   if (program->Base.Parameters) {
      /* free previous program's parameters */
      _mesa_free_parameter_list(program->Base.Parameters);
   }
   program->Base.Parameters    = ap.Base.Parameters;
   program->FogOption          = ap.FogOption;

#if DEBUG_FP
   _mesa_print_program(&program.Base);
#endif
}
