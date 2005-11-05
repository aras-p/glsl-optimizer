/*
 * Mesa 3-D graphics library
 * Version:  6.2
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

#define DEBUG_VP 0

/**
 * \file arbvertparse.c
 * ARB_vertex_program parser.
 * \author Karl Rasche
 */

#include "glheader.h"
#include "context.h"
#include "arbvertparse.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "program.h"
#include "nvprogram.h"
#include "nvvertparse.h"
#include "program_instruction.h"

#include "arbprogparse.h"


/**
 * Parse the vertex program string.  If success, update the given
 * vertex_program object with the new program.  Else, leave the vertex_program
 * object unchanged.
 */
void
_mesa_parse_arb_vertex_program(GLcontext * ctx, GLenum target,
			       const GLubyte * str, GLsizei len,
			       struct vertex_program *program)
{
   struct arb_program ap;
   (void) target;
   struct prog_instruction *newInstructions;

   /* set the program target before parsing */
   ap.Base.Target = GL_VERTEX_PROGRAM_ARB;

   if (!_mesa_parse_arb_program(ctx, str, len, &ap)) {
      /* Error in the program. Just return. */
      return;
   }

   /* Copy the relevant contents of the arb_program struct into the 
    * vertex_program struct.
    */
   /* copy instruction buffer */
   newInstructions = (struct prog_instruction *)
      _mesa_malloc(ap.Base.NumInstructions * sizeof(struct prog_instruction));
   if (!newInstructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return;
   }
   _mesa_memcpy(newInstructions, ap.VPInstructions,
                ap.Base.NumInstructions * sizeof(struct prog_instruction));
   if (program->Instructions)
      _mesa_free(program->Instructions);
   program->Instructions = newInstructions;
   program->Base.String          = ap.Base.String;
   program->Base.NumInstructions = ap.Base.NumInstructions;
   program->Base.NumTemporaries  = ap.Base.NumTemporaries;
   program->Base.NumParameters   = ap.Base.NumParameters;
   program->Base.NumAttributes   = ap.Base.NumAttributes;
   program->Base.NumAddressRegs  = ap.Base.NumAddressRegs;
   program->IsPositionInvariant = ap.HintPositionInvariant;
   program->InputsRead     = ap.InputsRead;
   program->OutputsWritten = ap.OutputsWritten;

   if (program->Parameters) {
      /* free previous program's parameters */
      _mesa_free_parameter_list(program->Parameters);
   }
   program->Parameters     = ap.Parameters; 

#if 1/*DEBUG_VP*/
   _mesa_print_program(ap.Base.NumInstructions, ap.VPInstructions);
#endif
}
