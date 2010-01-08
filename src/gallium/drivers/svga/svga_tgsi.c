/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_defines.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "util/u_memory.h"
#include "util/u_bitmask.h"

#include "svgadump/svga_shader_dump.h"

#include "svga_context.h"
#include "svga_tgsi.h"
#include "svga_tgsi_emit.h"
#include "svga_debug.h"

#include "svga_hw_reg.h"
#include "svga3d_shaderdefs.h"


/* Sinkhole used only in error conditions.
 */
static char err_buf[128];

#if 0
static void svga_destroy_shader_emitter( struct svga_shader_emitter *emit )
{
   if (emit->buf != err_buf)
      FREE(emit->buf);
}
#endif


static boolean svga_shader_expand( struct svga_shader_emitter *emit )
{
   char *new_buf;
   unsigned newsize = emit->size * 2;

   if(emit->buf != err_buf)
      new_buf = REALLOC(emit->buf, emit->size, newsize);
   else
      new_buf = NULL;

   if (new_buf == NULL) {
      emit->ptr = err_buf;
      emit->buf = err_buf;
      emit->size = sizeof(err_buf);
      return FALSE;
   }

   emit->size = newsize;
   emit->ptr = new_buf + (emit->ptr - emit->buf);
   emit->buf = new_buf;
   return TRUE;
}   

static INLINE boolean reserve(  struct svga_shader_emitter *emit,
                                unsigned nr_dwords )
{
   if (emit->ptr - emit->buf + nr_dwords * sizeof(unsigned) >= emit->size) {
      if (!svga_shader_expand( emit ))
         return FALSE;
   }

   return TRUE;
}

boolean svga_shader_emit_dword( struct svga_shader_emitter *emit,
                                unsigned dword )
{
   if (!reserve(emit, 1))
      return FALSE;

   *(unsigned *)emit->ptr = dword;
   emit->ptr += sizeof dword;
   return TRUE;
}

boolean svga_shader_emit_dwords( struct svga_shader_emitter *emit,
                                 const unsigned *dwords,
                                 unsigned nr )
{
   if (!reserve(emit, nr))
      return FALSE;

   memcpy( emit->ptr, dwords, nr * sizeof *dwords );
   emit->ptr += nr * sizeof *dwords;
   return TRUE;
}

boolean svga_shader_emit_opcode( struct svga_shader_emitter *emit,
                                 unsigned opcode )
{
   SVGA3dShaderInstToken *here;

   if (!reserve(emit, 1))
      return FALSE;

   here = (SVGA3dShaderInstToken *)emit->ptr;
   here->value = opcode;

   if (emit->insn_offset) {
      SVGA3dShaderInstToken *prev = (SVGA3dShaderInstToken *)(emit->buf + 
                                                              emit->insn_offset);
      prev->size = (here - prev) - 1;
   }
   
   emit->insn_offset = emit->ptr - emit->buf;
   emit->ptr += sizeof(unsigned);
   return TRUE;
}

#define SVGA3D_PS_2X (SVGA3D_PS_20 | 1)
#define SVGA3D_VS_2X (SVGA3D_VS_20 | 1)

static boolean svga_shader_emit_header( struct svga_shader_emitter *emit )
{
   SVGA3dShaderVersion header;

   memset( &header, 0, sizeof header );

   switch (emit->unit) {
   case PIPE_SHADER_FRAGMENT:
      header.value = emit->use_sm30 ? SVGA3D_PS_30 : SVGA3D_PS_2X;
      break;
   case PIPE_SHADER_VERTEX:
      header.value = emit->use_sm30 ? SVGA3D_VS_30 : SVGA3D_VS_2X;
      break;
   }
 
   return svga_shader_emit_dword( emit, header.value );
}





/* Parse TGSI shader and translate to SVGA/DX9 serialized
 * representation.  
 *
 * In this function SVGA shader is emitted to an in-memory buffer that
 * can be dynamically grown.  Once we've finished and know how large
 * it is, it will be copied to a hardware buffer for upload.
 */
static struct svga_shader_result *
svga_tgsi_translate( const struct svga_shader *shader,
                     union svga_compile_key key,
                     unsigned unit )
{
   struct svga_shader_result *result = NULL;
   struct svga_shader_emitter emit;
   int ret = 0;

   memset(&emit, 0, sizeof(emit));

   emit.use_sm30 = shader->use_sm30;
   emit.size = 1024;
   emit.buf = MALLOC(emit.size);
   if (emit.buf == NULL) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto fail;
   }

   emit.ptr = emit.buf;
   emit.unit = unit;
   emit.key = key;

   tgsi_scan_shader( shader->tokens, &emit.info);

   emit.imm_start = emit.info.file_max[TGSI_FILE_CONSTANT] + 1;
   
   if (unit == PIPE_SHADER_FRAGMENT)
      emit.imm_start += key.fkey.num_unnormalized_coords;

   if (unit == PIPE_SHADER_VERTEX) {
      emit.imm_start += key.vkey.need_prescale ? 2 : 0;
      emit.imm_start += key.vkey.num_zero_stride_vertex_elements;
   }

   emit.nr_hw_const = (emit.imm_start + emit.info.file_max[TGSI_FILE_IMMEDIATE] + 1);

   emit.nr_hw_temp = emit.info.file_max[TGSI_FILE_TEMPORARY] + 1;
   emit.in_main_func = TRUE;

   if (!svga_shader_emit_header( &emit ))
      goto fail;

   if (!svga_shader_emit_instructions( &emit, shader->tokens ))
      goto fail;
   
   result = CALLOC_STRUCT(svga_shader_result);
   if (result == NULL)
      goto fail;

   result->shader = shader;
   result->tokens = (const unsigned *)emit.buf;
   result->nr_tokens = (emit.ptr - emit.buf) / sizeof(unsigned);
   memcpy(&result->key, &key, sizeof key);
   result->id = UTIL_BITMASK_INVALID_INDEX;

   if (SVGA_DEBUG & DEBUG_TGSI) 
   {
      debug_printf( "#####################################\n" );
      debug_printf( "Shader %u below\n", shader->id );
      tgsi_dump( shader->tokens, 0 );
      if (SVGA_DEBUG & DEBUG_TGSI) {
         debug_printf( "Shader %u compiled below\n", shader->id );
         svga_shader_dump( result->tokens,
                           result->nr_tokens ,
                           FALSE );
      }
      debug_printf( "#####################################\n" );
   }

   return result;

fail:
   FREE(result);
   FREE(emit.buf);
   return NULL;
}




struct svga_shader_result *
svga_translate_fragment_program( const struct svga_fragment_shader *fs,
                                 const struct svga_fs_compile_key *fkey )
{
   union svga_compile_key key;
   memcpy(&key.fkey, fkey, sizeof *fkey);

   return svga_tgsi_translate( &fs->base, 
                               key,
                               PIPE_SHADER_FRAGMENT );
}

struct svga_shader_result *
svga_translate_vertex_program( const struct svga_vertex_shader *vs,
                               const struct svga_vs_compile_key *vkey )
{
   union svga_compile_key key;
   memcpy(&key.vkey, vkey, sizeof *vkey);

   return svga_tgsi_translate( &vs->base, 
                               key,
                               PIPE_SHADER_VERTEX );
}


void svga_destroy_shader_result( struct svga_shader_result *result )
{
   FREE((unsigned *)result->tokens);
   FREE(result);
}

