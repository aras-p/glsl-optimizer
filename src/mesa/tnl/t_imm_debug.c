/* $Id: t_imm_debug.c,v 1.6 2002/01/05 20:51:13 brianp Exp $ */

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
 */

#include "mtypes.h"
#include "t_context.h"
#include "t_imm_debug.h"

void _tnl_print_vert_flags( const char *name, GLuint flags )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & VERT_CLIP)       ? "clip/proj-clip/glend, " : "",
	   (flags & VERT_EDGEFLAG_BIT)       ? "edgeflag, " : "",
	   (flags & VERT_ELT)        ? "array-elt, " : "",
	   (flags & VERT_END_VB)     ? "end-vb, " : "",
	   (flags & VERT_EVAL_ANY)   ? "eval-coord, " : "",
	   (flags & VERT_EYE)        ? "eye/glbegin, " : "",
	   (flags & VERT_FOG_BIT)  ? "fog-coord, " : "",
	   (flags & VERT_INDEX_BIT)      ? "index, " : "",
	   (flags & VERT_MATERIAL)   ? "material, " : "",
	   (flags & VERT_NORMAL_BIT)       ? "normals, " : "",
	   (flags & VERT_OBJ_BIT)        ? "obj, " : "",
	   (flags & VERT_OBJ_3)      ? "obj-3, " : "",
	   (flags & VERT_OBJ_4)      ? "obj-4, " : "",
	   (flags & VERT_POINT_SIZE) ? "point-size, " : "",
	   (flags & VERT_COLOR0_BIT)       ? "colors, " : "",
	   (flags & VERT_COLOR1_BIT)   ? "specular, " : "",
	   (flags & VERT_TEX0_BIT)       ? "texcoord0, " : "",
	   (flags & VERT_TEX1_BIT)       ? "texcoord1, " : "",
	   (flags & VERT_TEX2_BIT)       ? "texcoord2, " : "",
	   (flags & VERT_TEX3_BIT)       ? "texcoord3, " : "",
	   (flags & VERT_TEX4_BIT)       ? "texcoord4, " : "",
	   (flags & VERT_TEX5_BIT)       ? "texcoord5, " : "",
	   (flags & VERT_TEX6_BIT)       ? "texcoord6, " : "",
	   (flags & VERT_TEX7_BIT)       ? "texcoord7, " : ""
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

   fprintf(stderr, "Cassette id %d, %u rows.\n", IM->id,
	   IM->Count - IM->CopyStart);

   _tnl_print_vert_flags("Contains at least one", orflag);

   if (IM->Count != IM->CopyStart)
   {
      _tnl_print_vert_flags("Contains a full complement of", andflag);

      fprintf(stderr, "Final begin/end state %s/%s, errors %s/%s\n",
	     (state & VERT_BEGIN_0) ? "in" : "out",
	     (state & VERT_BEGIN_1) ? "in" : "out",
	     (state & VERT_ERROR_0) ? "y" : "n",
	     (state & VERT_ERROR_1) ? "y" : "n");

   }

   for (i = IM->CopyStart ; i <= IM->Count ; i++) {
      fprintf(stderr, "%u: ", i);
      if (req & VERT_OBJ_234) {
	 if (flags[i] & VERT_EVAL_C1)
	    fprintf(stderr, "EvalCoord %f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0]);
	 else if (flags[i] & VERT_EVAL_P1)
	    fprintf(stderr, "EvalPoint %.0f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0]);
	 else if (flags[i] & VERT_EVAL_C2)
	    fprintf(stderr, "EvalCoord %f %f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1]);
	 else if (flags[i] & VERT_EVAL_P2)
	    fprintf(stderr, "EvalPoint %.0f %.0f ",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1]);
	 else if (i < IM->Count && (flags[i]&VERT_OBJ_234)) {
	    fprintf(stderr, "Obj %f %f %f %f",
                    IM->Attrib[VERT_ATTRIB_POS][i][0],
                    IM->Attrib[VERT_ATTRIB_POS][i][1],
                    IM->Attrib[VERT_ATTRIB_POS][i][2],
                    IM->Attrib[VERT_ATTRIB_POS][i][3]);
	 }
      }

      if (req & flags[i] & VERT_ELT)
	 fprintf(stderr, " Elt %u\t", IM->Elt[i]);

      if (req & flags[i] & VERT_NORMAL_BIT)
	 fprintf(stderr, " Norm %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][0],
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][1],
                 IM->Attrib[VERT_ATTRIB_NORMAL][i][2]);

      if (req & flags[i] & VERT_TEX_ANY) {
	 GLuint j;
	 for (j = 0 ; j < MAX_TEXTURE_UNITS ; j++) {
	    if (req & flags[i] & VERT_TEX(j)) {
	       fprintf(stderr, "TC%d %f %f %f %f", j,
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][0],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][1],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][2],
		       IM->Attrib[VERT_ATTRIB_TEX0 + j][i][3]);
	    }
	 }
      }

      if (req & flags[i] & VERT_COLOR0_BIT)
	 fprintf(stderr, " Rgba %f %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][0],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][1],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][2],
                 IM->Attrib[VERT_ATTRIB_COLOR0][i][3]);

      if (req & flags[i] & VERT_COLOR1_BIT)
	 fprintf(stderr, " Spec %f %f %f ",
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][0],
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][1],
                 IM->Attrib[VERT_ATTRIB_COLOR1][i][2]);

      if (req & flags[i] & VERT_FOG_BIT)
	 fprintf(stderr, " Fog %f ", IM->Attrib[VERT_ATTRIB_FOG][i][0]);

      if (req & flags[i] & VERT_INDEX_BIT)
	 fprintf(stderr, " Index %u ", IM->Index[i]);

      if (req & flags[i] & VERT_EDGEFLAG_BIT)
	 fprintf(stderr, " Edgeflag %d ", IM->EdgeFlag[i]);

      if (req & flags[i] & VERT_MATERIAL)
	 fprintf(stderr, " Material ");


      /* The order of these two is not easily knowable, but this is
       * the usually correct way to look at them.
       */
      if (req & flags[i] & VERT_END)
	 fprintf(stderr, " END ");

      if (req & flags[i] & VERT_BEGIN)
	 fprintf(stderr, " BEGIN(%s) (%s%s%s%s)",
		 _mesa_prim_name[IM->Primitive[i] & PRIM_MODE_MASK],
		 (IM->Primitive[i] & PRIM_LAST) ? "LAST," : "",
		 (IM->Primitive[i] & PRIM_BEGIN) ? "BEGIN," : "",
		 (IM->Primitive[i] & PRIM_END) ? "END," : "",
		 (IM->Primitive[i] & PRIM_PARITY) ? "PARITY," : "");

      fprintf(stderr, "\n");
   }
}
