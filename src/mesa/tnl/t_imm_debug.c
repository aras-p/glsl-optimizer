
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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

#include "mtypes.h"
#include "context.h"
#include "imports.h"
#include "t_context.h"
#include "t_imm_debug.h"


void _tnl_print_vert_flags( const char *name, GLuint flags )
{
   _mesa_debug(NULL,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & VERT_BIT_CLIP)       ? "clip/proj-clip/glend, " : "",
	   (flags & VERT_BIT_EDGEFLAG)       ? "edgeflag, " : "",
	   (flags & VERT_BIT_ELT)        ? "array-elt, " : "",
	   (flags & VERT_BIT_END_VB)     ? "end-vb, " : "",
	   (flags & VERT_BITS_EVAL_ANY)   ? "eval-coord, " : "",
	   (flags & VERT_BIT_EYE)        ? "eye/glbegin, " : "",
	   (flags & VERT_BIT_FOG)  ? "fog-coord, " : "",
	   (flags & VERT_BIT_INDEX)      ? "index, " : "",
	   (flags & VERT_BIT_MATERIAL)   ? "material, " : "",
	   (flags & VERT_BIT_NORMAL)       ? "normals, " : "",
	   (flags & VERT_BIT_POS)        ? "obj, " : "",
	   (flags & VERT_BIT_OBJ_3)      ? "obj-3, " : "",
	   (flags & VERT_BIT_OBJ_4)      ? "obj-4, " : "",
	   (flags & VERT_BIT_POINT_SIZE) ? "point-size, " : "",
	   (flags & VERT_BIT_COLOR0)       ? "colors, " : "",
	   (flags & VERT_BIT_COLOR1)   ? "specular, " : "",
	   (flags & VERT_BIT_TEX0)       ? "texcoord0, " : "",
	   (flags & VERT_BIT_TEX1)       ? "texcoord1, " : "",
	   (flags & VERT_BIT_TEX2)       ? "texcoord2, " : "",
	   (flags & VERT_BIT_TEX3)       ? "texcoord3, " : "",
	   (flags & VERT_BIT_TEX4)       ? "texcoord4, " : "",
	   (flags & VERT_BIT_TEX5)       ? "texcoord5, " : "",
	   (flags & VERT_BIT_TEX6)       ? "texcoord6, " : "",
	   (flags & VERT_BIT_TEX7)       ? "texcoord7, " : ""
      );
}

void _tnl_print_cassette( struct immediate *IM )
{
   GLuint i;
   GLuint *flags = IM->Flag;
   GLuint andflag = IM->CopyAndFlag;
   GLuint orflag = (IM->CopyOrFlag|IM->Evaluated);
   GLuint state = IM->BeginState;
   GLuint req = ~0;

   _mesa_debug(NULL, "Cassette id %d, %u rows.\n", IM->id,
	   IM->Count - IM->CopyStart);

   _tnl_print_vert_flags("Contains at least one", orflag);

   if (IM->Count != IM->CopyStart)
   {
      _tnl_print_vert_flags("Contains a full complement of", andflag);

      _mesa_debug(NULL, "Final begin/end state %s/%s, errors %s/%s\n",
	     (state & VERT_BEGIN_0) ? "in" : "out",
	     (state & VERT_BEGIN_1) ? "in" : "out",
	     (state & VERT_ERROR_0) ? "y" : "n",
	     (state & VERT_ERROR_1) ? "y" : "n");

   }

   for (i = IM->CopyStart ; i <= IM->Count ; i++) {
      _mesa_debug(NULL, "%u: ", i);
      if (req & VERT_BITS_OBJ_234) {
	 if (flags[i] & VERT_BIT_EVAL_C1)
	    _mesa_debug(NULL, "EvalCoord %f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0]);
	 else if (flags[i] & VERT_BIT_EVAL_P1)
	    _mesa_debug(NULL, "EvalPoint %.0f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0]);
	 else if (flags[i] & VERT_BIT_EVAL_C2)
	    _mesa_debug(NULL, "EvalCoord %f %f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1]);
	 else if (flags[i] & VERT_BIT_EVAL_P2)
	    _mesa_debug(NULL, "EvalPoint %.0f %.0f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1]);
	 else if (i < IM->Count && (flags[i] & VERT_BITS_OBJ_234)) {
	    _mesa_debug(NULL, "Obj %f %f %f %f",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1],
                    IM->Attrib[VERT_ATTRIB_POS][i][2],
                    IM->Attrib[VERT_ATTRIB_POS][i][3]);
	 }
      }

      if (req & flags[i] & VERT_BIT_ELT)
	 _mesa_debug(NULL, " Elt %u\t", IM->Elt[i]);

      if (req & flags[i] & VERT_BIT_NORMAL)
	 _mesa_debug(NULL, " Norm %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][0],
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][1],
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][2]);

      if (req & flags[i] & VERT_BITS_TEX_ANY) {
	 GLuint j;
	 for (j = 0 ; j < MAX_TEXTURE_COORD_UNITS ; j++) {
	    if (req & flags[i] & VERT_BIT_TEX(j)) {
	       _mesa_debug(NULL, "TC%d %f %f %f %f", j,
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][0],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][1],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][2],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][3]);
	    }
	 }
      }

      if (req & flags[i] & VERT_BIT_COLOR0)
	 _mesa_debug(NULL, " Rgba %f %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][0],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][1],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][2],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][3]);

      if (req & flags[i] & VERT_BIT_COLOR1)
	 _mesa_debug(NULL, " Spec %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][0],
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][1],
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][2]);

      if (req & flags[i] & VERT_BIT_FOG)
	 _mesa_debug(NULL, " Fog %f ", IM->Attrib[VERT_ATTRIB_FOG][i][0]);

      if (req & flags[i] & VERT_BIT_INDEX)
	 _mesa_debug(NULL, " Index %u ", IM->Index[i]);

      if (req & flags[i] & VERT_BIT_EDGEFLAG)
	 _mesa_debug(NULL, " Edgeflag %d ", IM->EdgeFlag[i]);

      if (req & flags[i] & VERT_BIT_MATERIAL)
	 _mesa_debug(NULL, " Material ");


      /* The order of these two is not easily knowable, but this is
       * the usually correct way to look at them.
       */
      if (req & flags[i] & VERT_BIT_END)
	 _mesa_debug(NULL, " END ");

      if (req & flags[i] & VERT_BIT_BEGIN)
	 _mesa_debug(NULL, " BEGIN(%s) (%s%s%s%s)",
		 _mesa_prim_name[IM->Primitive[i] & PRIM_MODE_MASK],
		 (IM->Primitive[i] & PRIM_LAST) ? "LAST," : "",
		 (IM->Primitive[i] & PRIM_BEGIN) ? "BEGIN," : "",
		 (IM->Primitive[i] & PRIM_END) ? "END," : "",
		 (IM->Primitive[i] & PRIM_PARITY) ? "PARITY," : "");

      _mesa_debug(NULL, "\n");
   }
}
