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

#include "main/imports.h"
#include "vbo/vbo.h"
#include "st_public.h"
#include "st_context.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_clear.h"
#include "st_cb_drawpixels.h"
#include "st_cb_fbo.h"
#include "st_cb_queryobj.h"
#include "st_cb_rasterpos.h"
#include "st_cb_readpixels.h"
#include "st_cb_texture.h"
#include "st_cb_flush.h"
#include "st_cb_strings.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_program.h"
#include "pipe/p_context.h"


void st_invalidate_state(GLcontext * ctx, GLuint new_state)
{
   struct st_context *st = st_context(ctx);

   st->dirty.mesa |= new_state;
   st->dirty.st |= ST_NEW_MESA;
}


struct st_context *st_create_context( GLcontext *ctx,
				      struct pipe_context *pipe )
{
   struct st_context *st = CALLOC_STRUCT( st_context );
   
   ctx->st = st;

   st->ctx = ctx;
   st->pipe = pipe;

   st->dirty.mesa = ~0;
   st->dirty.st = ~0;

   st_init_atoms( st );
   st_init_draw( st );

   /* we want all vertex data to be placed in buffer objects */
   vbo_use_buffer_objects(ctx);

   /* Need these flags:
    */
   st->ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   st->ctx->FragmentProgram._UseTexEnvProgram = GL_TRUE;

   st->ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;


#if 0
   st_init_cb_clear( st );
   st_init_cb_program( st );
   st_init_cb_drawpixels( st );
   st_init_cb_texture( st );
#endif

   /* XXXX This is temporary! */
   _mesa_enable_sw_extensions(ctx);

   return st;
}


void st_destroy_context( struct st_context *st )
{
   st_destroy_atoms( st );
   st_destroy_draw( st );

#if 0
   st_destroy_cb_clear( st );
   st_destroy_cb_program( st );
   st_destroy_cb_drawpixels( st );
   /*st_destroy_cb_teximage( st );*/
   st_destroy_cb_texture( st );
#endif

   st->pipe->destroy( st->pipe );
   FREE( st );
}

 

void st_init_driver_functions(struct dd_function_table *functions)
{
   st_init_bufferobject_functions(functions);
   st_init_clear_functions(functions);
   st_init_drawpixels_functions(functions);
   st_init_fbo_functions(functions);
   st_init_program_functions(functions);
   st_init_query_functions(functions);
   st_init_rasterpos_functions(functions);
   st_init_readpixels_functions(functions);
   st_init_texture_functions(functions);
   st_init_flush_functions(functions);
   st_init_string_functions(functions);
}
