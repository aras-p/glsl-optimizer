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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "util/u_math.h"
#include "util/u_memory.h"

#include "pipe/p_shader_tokens.h"

#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

#include "translate/translate.h"
#include "translate/translate_cache.h"

#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_exec.h"



void
draw_vs_set_constants(struct draw_context *draw,
                      unsigned slot,
                      const void *constants,
                      unsigned size)
{
   if (((uintptr_t)constants) & 0xf) {
      if (size > draw->vs.const_storage_size[slot]) {
         if (draw->vs.aligned_constant_storage[slot]) {
            align_free((void *)draw->vs.aligned_constant_storage[slot]);
         }
         draw->vs.aligned_constant_storage[slot] = align_malloc(size, 16);
      }
      assert(constants);
      memcpy((void *)draw->vs.aligned_constant_storage[slot],
             constants,
             size);
      constants = draw->vs.aligned_constant_storage[slot];
   }

   draw->vs.aligned_constants[slot] = constants;
   draw_vs_aos_machine_constants(draw->vs.aos_machine, slot, constants);
}


void draw_vs_set_viewport( struct draw_context *draw,
                           const struct pipe_viewport_state *viewport )
{
   draw_vs_aos_machine_viewport( draw->vs.aos_machine, viewport );
}



struct draw_vertex_shader *
draw_create_vertex_shader(struct draw_context *draw,
                          const struct pipe_shader_state *shader)
{
   struct draw_vertex_shader *vs;

   if (draw->dump_vs) {
      tgsi_dump(shader->tokens, 0);
   }

   vs = draw_create_vs_llvm( draw, shader );
   if (!vs) {
      vs = draw_create_vs_sse( draw, shader );
      if (!vs) {
         vs = draw_create_vs_ppc( draw, shader );
         if (!vs) {
            vs = draw_create_vs_exec( draw, shader );
         }
      }
   }

   if (vs)
   {
      uint i;
      for (i = 0; i < vs->info.num_outputs; i++) {
         if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
             vs->info.output_semantic_index[i] == 0)
            vs->position_output = i;
         else if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_EDGEFLAG &&
             vs->info.output_semantic_index[i] == 0)
            vs->edgeflag_output = i;
      }
   }

   assert(vs);
   return vs;
}


void
draw_bind_vertex_shader(struct draw_context *draw,
                        struct draw_vertex_shader *dvs)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   
   if (dvs) 
   {
      draw->vs.vertex_shader = dvs;
      draw->vs.num_vs_outputs = dvs->info.num_outputs;
      draw->vs.position_output = dvs->position_output;
      draw->vs.edgeflag_output = dvs->edgeflag_output;
      dvs->prepare( dvs, draw );
   }
   else {
      draw->vs.vertex_shader = NULL;
      draw->vs.num_vs_outputs = 0;
   }
}


void
draw_delete_vertex_shader(struct draw_context *draw,
                          struct draw_vertex_shader *dvs)
{
   unsigned i;

   for (i = 0; i < dvs->nr_varients; i++) 
      dvs->varient[i]->destroy( dvs->varient[i] );

   dvs->nr_varients = 0;

   dvs->delete( dvs );
}



boolean 
draw_vs_init( struct draw_context *draw )
{
   draw->dump_vs = debug_get_bool_option("GALLIUM_DUMP_VS", FALSE);

   draw->vs.machine = tgsi_exec_machine_create();
   if (!draw->vs.machine)
      return FALSE;

   draw->vs.emit_cache = translate_cache_create();
   if (!draw->vs.emit_cache) 
      return FALSE;
      
   draw->vs.fetch_cache = translate_cache_create();
   if (!draw->vs.fetch_cache) 
      return FALSE;

   draw->vs.aos_machine = draw_vs_aos_machine();
#ifdef PIPE_ARCH_X86
   if (!draw->vs.aos_machine)
      return FALSE;
#endif
      
   return TRUE;
}

void
draw_vs_destroy( struct draw_context *draw )
{
   uint i;

   if (draw->vs.fetch_cache)
      translate_cache_destroy(draw->vs.fetch_cache);

   if (draw->vs.emit_cache)
      translate_cache_destroy(draw->vs.emit_cache);

   if (draw->vs.aos_machine)
      draw_vs_aos_machine_destroy(draw->vs.aos_machine);

   for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      if (draw->vs.aligned_constant_storage[i]) {
         align_free((void *)draw->vs.aligned_constant_storage[i]);
      }
   }

   tgsi_exec_machine_destroy(draw->vs.machine);
}


struct draw_vs_varient *
draw_vs_lookup_varient( struct draw_vertex_shader *vs,
                        const struct draw_vs_varient_key *key )
{
   struct draw_vs_varient *varient;
   unsigned i;

   /* Lookup existing varient: 
    */
   for (i = 0; i < vs->nr_varients; i++)
      if (draw_vs_varient_key_compare(key, &vs->varient[i]->key) == 0)
         return vs->varient[i];
   
   /* Else have to create a new one: 
    */
   varient = vs->create_varient( vs, key );
   if (varient == NULL)
      return NULL;

   /* Add it to our list, could be smarter: 
    */
   if (vs->nr_varients < Elements(vs->varient)) {
      vs->varient[vs->nr_varients++] = varient;
   }
   else {
      vs->last_varient++;
      vs->last_varient %= Elements(vs->varient);
      vs->varient[vs->last_varient]->destroy(vs->varient[vs->last_varient]);
      vs->varient[vs->last_varient] = varient;
   }

   /* Done 
    */
   return varient;
}


struct translate *
draw_vs_get_fetch( struct draw_context *draw,
                   struct translate_key *key )
{
   if (!draw->vs.fetch ||
       translate_key_compare(&draw->vs.fetch->key, key) != 0) 
   {
      translate_key_sanitize(key);
      draw->vs.fetch = translate_cache_find(draw->vs.fetch_cache, key);
   }
   
   return draw->vs.fetch;
}

struct translate *
draw_vs_get_emit( struct draw_context *draw,
                  struct translate_key *key )
{
   if (!draw->vs.emit ||
       translate_key_compare(&draw->vs.emit->key, key) != 0) 
   {
      translate_key_sanitize(key);
      draw->vs.emit = translate_cache_find(draw->vs.emit_cache, key);
   }
   
   return draw->vs.emit;
}
