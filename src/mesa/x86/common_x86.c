/* $Id: common_x86.c,v 1.7 2000/10/23 00:16:28 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 * Changed by Andre Werthmann <wertmann@cs.uni-potsdam.de> for using the
 * new Katmai functions.
 */

#include <stdlib.h>
#include <stdio.h>

#include "common_x86_asm.h"


int gl_x86_cpu_features = 0;

/* No reason for this to be public.
 */
extern int gl_identify_x86_cpu_features( void );


static void message( const char *msg )
{
   if ( getenv( "MESA_DEBUG" ) ) {
      fprintf( stderr, "%s\n", msg );
   }
}


void gl_init_all_x86_transform_asm( void )
{
#ifdef USE_X86_ASM
   gl_x86_cpu_features = gl_identify_x86_cpu_features();

   if ( getenv( "MESA_NO_ASM" ) ) {
      gl_x86_cpu_features = 0;
   }

   if ( gl_x86_cpu_features ) {
      gl_init_x86_transform_asm();
   }

#ifdef USE_MMX_ASM
   if ( cpu_has_mmx ) {
      if ( getenv( "MESA_NO_MMX" ) == 0 ) {
         message( "MMX cpu detected." );
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_MMX);
      }
   }
#endif

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow ) {
      if ( getenv( "MESA_NO_3DNOW" ) == 0 ) {
         message( "3Dnow cpu detected." );
         gl_init_3dnow_transform_asm();
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_3DNOW);
      }
   }
#endif

#ifdef USE_KATMAI_ASM
   if ( cpu_has_xmm ) {
      if ( getenv( "MESA_NO_KATMAI" ) == 0 ) {
         message( "Katmai cpu detected." );
         gl_init_katmai_transform_asm();
      } else {
         gl_x86_cpu_features &= ~(X86_FEATURE_XMM);
      }
   }
#endif
#endif
}

/* Note: the above function must be called before this one, so that
 * gl_x86_cpu_features gets correctly initialized.
 */
void gl_init_all_x86_vertex_asm( void )
{
#ifdef USE_X86_ASM
   if ( gl_x86_cpu_features ) {
      gl_init_x86_vertex_asm();
   }

#ifdef USE_3DNOW_ASM
   if ( cpu_has_3dnow && getenv( "MESA_NO_3DNOW" ) == 0 ) {
      gl_init_3dnow_vertex_asm();
   }
#endif

#ifdef USE_KATMAI_ASM
   if ( cpu_has_xmm && getenv( "MESA_NO_KATMAI" ) == 0 ) {
      gl_init_katmai_vertex_asm();
   }
#endif
#endif
}
