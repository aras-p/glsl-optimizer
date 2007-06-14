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

#include "imports.h"
#include "st_public.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_program.h"
#include "softpipe/sp_context.h"

void st_invalidate_state(GLcontext * ctx, GLuint new_state)
{
   struct st_context *st = st_context(ctx);

   st->dirty.mesa |= new_state;
   st->dirty.st |= ST_NEW_MESA;
}


struct st_context *st_create_context( GLcontext *ctx,
				      struct softpipe_context *softpipe )
{
   struct st_context *st = CALLOC_STRUCT( st_context );
   
   ctx->st = st;

   st->ctx = ctx;
   st->softpipe = softpipe;

   st->dirty.mesa = ~0;
   st->dirty.st = ~0;

   st_init_atoms( st );
   st_init_draw( st );
   st_init_cb_program( st );

   return st;
}


void st_destroy_context( struct st_context *st )
{
   st_destroy_atoms( st );
   st_destroy_draw( st );
   st_destroy_cb_program( st );
   st->softpipe->destroy( st->softpipe );
   FREE( st );
}

 

