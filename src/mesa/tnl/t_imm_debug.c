/* $Id: t_imm_debug.c,v 1.3 2001/04/28 08:39:18 keithw Exp $ */

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
	   (flags & VERT_EDGE)       ? "edgeflag, " : "",
	   (flags & VERT_ELT)        ? "array-elt, " : "",
	   (flags & VERT_END_VB)     ? "end-vb, " : "",
	   (flags & VERT_EVAL_ANY)   ? "eval-coord, " : "",
	   (flags & VERT_EYE)        ? "eye/glbegin, " : "",
	   (flags & VERT_FOG_COORD)  ? "fog-coord, " : "",
	   (flags & VERT_INDEX)      ? "index, " : "",
	   (flags & VERT_MATERIAL)   ? "material, " : "",
	   (flags & VERT_NORM)       ? "normals, " : "",
	   (flags & VERT_OBJ)        ? "obj, " : "",
	   (flags & VERT_OBJ_3)      ? "obj-3, " : "",
	   (flags & VERT_OBJ_4)      ? "obj-4, " : "",
	   (flags & VERT_POINT_SIZE) ? "point-size, " : "",
	   (flags & VERT_RGBA)       ? "colors, " : "",
	   (flags & VERT_SPEC_RGB)   ? "specular, " : "",
	   (flags & VERT_TEX0)       ? "texcoord0, " : "",
	   (flags & VERT_TEX1)       ? "texcoord1, " : "",
	   (flags & VERT_TEX2)       ? "texcoord2, " : "",
	   (flags & VERT_TEX3)       ? "texcoord3, " : "",
	   (flags & VERT_TEX4)       ? "texcoord4, " : "",
	   (flags & VERT_TEX5)       ? "texcoord5, " : "",
	   (flags & VERT_TEX6)       ? "texcoord6, " : "",
	   (flags & VERT_TEX7)       ? "texcoord7, " : ""
      );
}

void _tnl_print_cassette( struct immediate *IM )
{
   GLuint i;
   GLuint *flags = IM->Flag;
   GLuint andflag = IM->CopyAndFlag;
   GLuint orflag = IM->CopyOrFlag;
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
	    fprintf(stderr, "EvalCoord %f ", IM->Obj[i][0]);
	 else if (flags[i] & VERT_EVAL_P1)
	    fprintf(stderr, "EvalPoint %.0f ", IM->Obj[i][0]);
	 else if (flags[i] & VERT_EVAL_C2)
	    fprintf(stderr, "EvalCoord %f %f ", IM->Obj[i][0], IM->Obj[i][1]);
	 else if (flags[i] & VERT_EVAL_P2)
	    fprintf(stderr, "EvalPoint %.0f %.0f ", IM->Obj[i][0], IM->Obj[i][1]);
	 else if (i < IM->Count && (flags[i]&VERT_OBJ_234)) {
	    fprintf(stderr, "Obj %f %f %f %f",
		   IM->Obj[i][0], IM->Obj[i][1], IM->Obj[i][2], IM->Obj[i][3]);
	 }
      }

      if (req & flags[i] & VERT_ELT)
	 fprintf(stderr, " Elt %u\t", IM->Elt[i]);

      if (req & flags[i] & VERT_NORM)
	 fprintf(stderr, " Norm %f %f %f ",
		IM->Normal[i][0], IM->Normal[i][1], IM->Normal[i][2]);

      if (req & flags[i] & VERT_TEX_ANY) {
	 GLuint j;
	 for (j = 0 ; j < MAX_TEXTURE_UNITS ; j++) {
	    if (req & flags[i] & VERT_TEX(j)) {
	       fprintf(stderr,
		       "TC%d %f %f %f %f",
		       j,
		       IM->TexCoord[j][i][0], IM->TexCoord[j][i][1],
		       IM->TexCoord[j][i][2], IM->TexCoord[j][i][2]);
	    }
	 }
      }

      if (req & flags[i] & VERT_RGBA)
	 fprintf(stderr, " Rgba %f %f %f %f ",
		IM->Color[i][0], IM->Color[i][1],
		IM->Color[i][2], IM->Color[i][3]);

      if (req & flags[i] & VERT_SPEC_RGB)
	 fprintf(stderr, " Spec %f %f %f ",
		IM->SecondaryColor[i][0], IM->SecondaryColor[i][1],
		IM->SecondaryColor[i][2]);

      if (req & flags[i] & VERT_FOG_COORD)
	 fprintf(stderr, " Fog %f ", IM->FogCoord[i]);

      if (req & flags[i] & VERT_INDEX)
	 fprintf(stderr, " Index %u ", IM->Index[i]);

      if (req & flags[i] & VERT_EDGE)
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
