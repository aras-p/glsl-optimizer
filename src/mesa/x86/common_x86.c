
/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


/*
 *  Check CPU capabilities & initialize optimized funtions for this particular
 *   processor.
 *
 *  Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 *  Changed by Andre Werthmann <wertmann@cs.uni-potsdam.de> for using the
 *  new Katmai functions
 */

#include <stdlib.h>
#include <stdio.h>
#include "common_x86asm.h"

int gl_x86_cpu_features = 0;


void gl_init_all_x86_asm (void)
{

#ifdef USE_X86_ASM
   gl_x86_cpu_features = gl_identify_x86_cpu_features ();
   gl_x86_cpu_features |= GL_CPU_AnyX86;

   if (getenv("MESA_NO_ASM") != 0)
      gl_x86_cpu_features = 0;

   if (gl_x86_cpu_features & GL_CPU_GenuineIntel) {
      fprintf (stderr, "GenuineIntel cpu detected.\n");
   }

   if (gl_x86_cpu_features) {
     gl_init_x86_asm_transforms ();
   }

#ifdef USE_MMX_ASM
   if (gl_x86_cpu_features & GL_CPU_MMX) {
      char *s = getenv( "MESA_NO_MMX" );
      if (s == NULL) { 
         fprintf (stderr, "MMX cpu detected.\n");
      } else {
         gl_x86_cpu_features &= (~GL_CPU_MMX); 
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if (gl_x86_cpu_features & GL_CPU_3Dnow) {
      char *s = getenv( "MESA_NO_3DNOW" );
      if (s == NULL) {
         fprintf (stderr, "3Dnow cpu detected.\n");
         gl_init_3dnow_asm_transforms ();
      } else {
         gl_x86_cpu_features &= (~GL_CPU_3Dnow); 
      }
   }
#endif


#ifdef USE_KATMAI_ASM
   if (gl_x86_cpu_features & GL_CPU_Katmai) {
      char *s = getenv( "MESA_NO_KATMAI" );
      if (s == NULL) {
         fprintf (stderr, "Katmai cpu detected.\n");
         gl_init_katmai_asm_transforms ();
      } else {
         gl_x86_cpu_features &= (~GL_CPU_Katmai); 
      }
   }
#endif

#endif
}

