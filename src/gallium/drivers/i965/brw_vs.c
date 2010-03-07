/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "tgsi/tgsi_dump.h"           

#include "brw_context.h"
#include "brw_vs.h"
#include "brw_state.h"



static enum pipe_error do_vs_prog( struct brw_context *brw, 
                                   struct brw_vertex_shader *vp,
                                   struct brw_vs_prog_key *key,
                                   struct brw_winsys_buffer **bo_out)
{
   enum pipe_error ret;
   GLuint program_size;
   const GLuint *program;
   struct brw_vs_compile c;

   memset(&c, 0, sizeof(c));
   memcpy(&c.key, key, sizeof(*key));

   brw_init_compile(brw, &c.func);
   c.vp = vp;

   c.prog_data.nr_outputs = vp->info.num_outputs;
   c.prog_data.nr_inputs = vp->info.num_inputs;

   if (1)
      tgsi_dump(c.vp->tokens, 0);

   /* Emit GEN4 code.
    */
   brw_vs_emit(&c);

   /* get the program
    */
   ret = brw_get_program(&c.func, &program, &program_size);
   if (ret)
      return ret;

   ret = brw_upload_cache( &brw->cache, BRW_VS_PROG,
                           &c.key, brw_vs_prog_key_size(&c.key),
                           NULL, 0,
                           program, program_size,
                           &c.prog_data,
                           &brw->vs.prog_data,
                           bo_out);
   if (ret)
      return ret;

   return PIPE_OK;
}


static enum pipe_error brw_upload_vs_prog(struct brw_context *brw)
{
   struct brw_vs_prog_key key;
   struct brw_vertex_shader *vp = brw->curr.vertex_shader;
   struct brw_fs_signature *sig = &brw->curr.fragment_shader->signature;
   enum pipe_error ret;

   memset(&key, 0, sizeof(key));

   key.program_string_id = vp->id;
   key.nr_userclip = brw->curr.ucp.nr;

   memcpy(&key.fs_signature, sig, brw_fs_signature_size(sig));


   /* Make an early check for the key.
    */
   if (brw_search_cache(&brw->cache, BRW_VS_PROG,
                        &key, brw_vs_prog_key_size(&key),
                        NULL, 0,
                        &brw->vs.prog_data,
                        &brw->vs.prog_bo))
      return PIPE_OK;

   ret = do_vs_prog(brw, vp, &key, &brw->vs.prog_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}


/* See brw_vs.c:
 */
const struct brw_tracked_state brw_vs_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_CLIP | 
                PIPE_NEW_RAST |
                PIPE_NEW_FRAGMENT_SIGNATURE),
      .brw   = BRW_NEW_VERTEX_PROGRAM,
      .cache = 0
   },
   .prepare = brw_upload_vs_prog
};
