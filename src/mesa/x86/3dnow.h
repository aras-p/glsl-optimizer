/* $Id: 3dnow.h,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

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
 * 3DNow! optimizations contributed by
 * Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 */


#ifndef _3dnow_h
#define _3dnow_h



#include "xform.h"


void gl_init_3dnow_asm_transforms (void);




#if 0
GLvector4f *gl_project_points( GLvector4f *proj_vec,
                               const GLvector4f *clip_vec )
{
   __asm__ (
   "   femms                                                              \n"
   "                                                                      \n"
   "   movq       (%0),     %%mm0      # x1             | x0              \n"
   "   movq       8(%0),    %%mm1      # oow            | x2              \n"
   "                                                                      \n"
   "1: movq       %%mm1,    %%mm2      # oow            | x2              \n"
   "   addl       %2,       %0         # next point                       \n"
   "                                                                      \n"
   "   punpckhdq  %%mm2,    %%mm2      # oow            | oow             \n"
   "   addl       $16,      %1         # next point                       \n"
   "                                                                      \n"
   "   pfrcp      %%mm2,    %%mm3      # 1/oow          | 1/oow           \n"
   "   decl       %3                                                      \n"
   "                                                                      \n"
   "   pfmul      %%mm3,    %%mm0      # x1/oow         | x0/oow          \n"
   "   movq       %%mm0,    -16(%1)    # write r0, r1                     \n"
   "                                                                      \n"
   "   pfmul      %%mm3,    %%mm1      # 1              | x2/oow          \n"
   "   movq       (%0),     %%mm0      # x1             | x0              \n"
   "                                                                      \n"
   "   movd       %%mm1,    8(%1)      # write r2                         \n"
   "   movd       %%mm3,    12(%1)     # write r3                         \n"
   "                                                                      \n"
   "   movq       8(%0),    %%mm1      # oow            | x2              \n"
   "   ja         1b                                                      \n"
   "                                                                      \n"
   "   femms                                                              \n"
   "                                                                        "
   ::"a" (clip_vec->start),
   "c" (proj_vec->start),
   "g" (clip_vec->stride),
   "d" (clip_vec->count)
   );

   proj_vec->flags |= VEC_SIZE_4;
   proj_vec->size = 3;
   proj_vec->count = clip_vec->count;
   return proj_vec;
}
#endif



#endif
