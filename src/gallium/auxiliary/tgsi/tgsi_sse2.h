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

#ifndef TGSI_SSE2_H
#define TGSI_SSE2_H

#if defined __cplusplus
extern "C" {
#endif

struct tgsi_token;
struct x86_function;
struct tgsi_interp_coef;

unsigned
tgsi_emit_sse2(
   const struct tgsi_token *tokens,
   struct x86_function *function,
   float (*immediates)[4],
   boolean do_swizzles );


/* This is the function prototype generated when do_swizzles is false
 * -- effectively for fragment shaders.
 */
typedef void (PIPE_CDECL *tgsi_sse2_fs_function) (
   struct tgsi_exec_machine *machine, /* 1 */
   const float (*constant)[4],		    /* 2 */
   const float (*immediate)[4],		    /* 3 */
   const struct tgsi_interp_coef *coef	    /* 4 */
   );


/* This is the function prototype generated when do_swizzles is true
 * -- effectively for vertex shaders.
 */
typedef void (PIPE_CDECL *tgsi_sse2_vs_func) (
   struct tgsi_exec_machine *machine, /* 1 */
   const float (*constant)[4],        /* 2 */
   const float (*immediate)[4],       /* 3 */
   const float (*aos_input)[4], /* 4 */
   uint num_inputs,             /* 5 */
   uint input_stride,           /* 6 */
   float (*aos_output)[4],      /* 7 */
   uint num_outputs,            /* 8 */
   uint output_stride );        /* 9 */


#if defined __cplusplus
}
#endif

#endif /* TGSI_SSE2_H */
