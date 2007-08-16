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
#include "pipe/tgsi/core/tgsi_token.h"

#define ST_FP_MAX_TOKENS 1024


struct st_fragment_program
{
   struct gl_fragment_program Base;
   GLboolean error;             /* If program is malformed for any reason. */

   GLuint    id;		/* String id, for tracking
				 * ProgramStringNotify changes.
				 */


   struct tgsi_token tokens[ST_FP_MAX_TOKENS];
   GLboolean dirty;
   
   struct pipe_constant_buffer constants;

#if 0   
   GLfloat (*cbuffer)[4];
   GLuint nr_constants;

   /* Translate all the parameters, etc, into a constant buffer which
    * we update on state changes.
    */
   struct
   {
      GLuint reg;               /* Constant idx */
      const GLfloat *values;    /* Pointer to tracked values */
   } *param;
   GLuint nr_params;
#endif

   GLuint param_state;
};


struct st_vertex_program
{
   struct gl_vertex_program Base;
   GLboolean error;             /* If program is malformed for any reason. */

   GLuint    id;		/* String id, for tracking
				 * ProgramStringNotify changes.
				 */

   struct tgsi_token tokens[ST_FP_MAX_TOKENS];
   GLboolean dirty;
   struct pipe_constant_buffer constants;
   GLuint param_state;
};


extern void st_init_program_functions(struct dd_function_table *functions);


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

#endif
