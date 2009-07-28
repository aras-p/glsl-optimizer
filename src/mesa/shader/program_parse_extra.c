/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include "main/mtypes.h"
#include "prog_instruction.h"
#include "program_parser.h"


/**
 * Extra assembly-level parser routines
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

int
_mesa_ARBvp_parse_option(struct asm_parser_state *state, const char *option)
{
   if (strcmp(option, "ARB_position_invariant") == 0) {
      state->option.PositionInvariant = 1;
      return 1;
   }

   return 0;
}


int
_mesa_ARBfp_parse_option(struct asm_parser_state *state, const char *option)
{
   /* All of the options currently supported start with "ARB_".  The code is
    * currently structured with nested if-statements because eventually options
    * that start with "NV_" will be supported.  This structure will result in
    * less churn when those options are added.
    */
   if (strncmp(option, "ARB_", 4) == 0) {
      /* Advance the pointer past the "ARB_" prefix.
       */
      option += 4;


      if (strncmp(option, "fog_", 4) == 0) {
	 option += 4;

	 if (state->option.Fog == OPTION_NONE) {
	    if (strcmp(option, "exp") == 0) {
	       state->option.Fog = OPTION_FOG_EXP;
	       return 1;
	    } else if (strcmp(option, "exp2") == 0) {
	       state->option.Fog = OPTION_FOG_EXP2;
	       return 1;
	    } else if (strcmp(option, "linear") == 0) {
	       state->option.Fog = OPTION_FOG_LINEAR;
	       return 1;
	    }
	 }

	 return 0;
      } else if (strncmp(option, "precision_hint_", 15) == 0) {
	 option += 15;

	 if (state->option.PrecisionHint == OPTION_NONE) {
	    if (strcmp(option, "nicest") == 0) {
	       state->option.PrecisionHint = OPTION_NICEST;
	       return 1;
	    } else if (strcmp(option, "fastest") == 0) {
	       state->option.PrecisionHint = OPTION_FASTEST;
	       return 1;
	    }
	 }

	 return 0;
      } else if (strcmp(option, "draw_buffers") == 0) {
	 /* Don't need to check extension availability because all Mesa-based
	  * drivers support GL_ARB_draw_buffers.
	  */
	 state->option.DrawBuffers = 1;
	 return 1;
      } else if (strcmp(option, "fragment_program_shadow") == 0) {
	 if (state->ctx->Extensions.ARB_fragment_program_shadow) {
	    state->option.Shadow = 1;
	    return 1;
	 }
      }
   } else if (strncmp(option, "MESA_", 5) == 0) {
      option += 5;

      if (strcmp(option, "texture_array") == 0) {
	 if (state->ctx->Extensions.MESA_texture_array) {
	    state->option.TexArray = 1;
	    return 1;
	 }
      }
   }

   return 0;
}
