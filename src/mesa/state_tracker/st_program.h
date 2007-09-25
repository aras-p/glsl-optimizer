/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
    

#ifndef ST_PROGRAM_H
#define ST_PROGRAM_H

#include "mtypes.h"
#include "pipe/tgsi/exec/tgsi_token.h"
#include "x86/rtasm/x86sse.h"

#define ST_FP_MAX_TOKENS 1024

struct cso_fragment_shader;
struct cso_vertex_shader;

struct st_fragment_program
{
   struct gl_fragment_program Base;
   GLboolean error;             /* If program is malformed for any reason. */

   GLuint serialNo;

   /** The program in TGSI format */
   struct tgsi_token tokens[ST_FP_MAX_TOKENS];

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   struct x86_function  sse2_program;
#endif

   /** Pointer to the corresponding cached shader */
   const struct cso_fragment_shader *fs;

   GLuint param_state;
};


struct st_vertex_program
{
   struct gl_vertex_program Base;  /**< The Mesa vertex program */
   GLboolean error;        /**< Set if program is malformed for any reason. */

   GLuint serialNo;

   /** maps a Mesa VERT_ATTRIB_x to a packed TGSI input index */
   GLuint input_to_index[MAX_VERTEX_PROGRAM_ATTRIBS];
   /** maps a TGSI input index back to a Mesa VERT_ATTRIB_x */
   GLuint index_to_input[MAX_VERTEX_PROGRAM_ATTRIBS];

   /** The program in TGSI format */
   struct tgsi_token tokens[ST_FP_MAX_TOKENS];

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   struct x86_function  sse2_program;
#endif

   /** Pointer to the corresponding cached shader */
   const struct cso_vertex_shader *vs;

   GLuint param_state;
};


extern void
st_init_program_functions(struct dd_function_table *functions);


static inline struct st_fragment_program *
st_fragment_program( struct gl_fragment_program *fp )
{
   return (struct st_fragment_program *)fp;
}


static inline struct st_vertex_program *
st_vertex_program( struct gl_vertex_program *vp )
{
   return (struct st_vertex_program *)vp;
}


extern const struct cso_fragment_shader *
st_translate_fragment_program(struct st_context *st,
                              struct st_fragment_program *fp,
                              const GLuint inputMapping[],
                              struct tgsi_token *tokens,
                              GLuint maxTokens);


extern const struct cso_vertex_shader *
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *vp,
                            const GLuint vert_output_to_slot[],
                            struct tgsi_token *tokens,
                            GLuint maxTokens);

#endif
