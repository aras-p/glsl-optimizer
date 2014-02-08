/**********************************************************
 * Copyright 2008-2012 VMware, Inc.  All rights reserved.
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

#include "util/u_bitmask.h"
#include "util/u_memory.h"
#include "svga_context.h"
#include "svga_cmd.h"
#include "svga_shader.h"



/**
 * Issue the SVGA3D commands to define a new shader.
 * \param result  contains the shader tokens, etc.  The result->id field will
 *                be set here.
 */
enum pipe_error
svga_define_shader(struct svga_context *svga,
                   SVGA3dShaderType type,
                   struct svga_shader_variant *variant)
{
   unsigned codeLen = variant->nr_tokens * sizeof(variant->tokens[0]);

   if (svga_have_gb_objects(svga)) {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;

      variant->gb_shader = sws->shader_create(sws, type,
                                              variant->tokens, codeLen);
      if (!variant->gb_shader)
         return PIPE_ERROR_OUT_OF_MEMORY;

      return PIPE_OK;
   }
   else {
      enum pipe_error ret;

      /* Allocate an integer ID for the shader */
      variant->id = util_bitmask_add(svga->shader_id_bm);
      if (variant->id == UTIL_BITMASK_INVALID_INDEX) {
         return PIPE_ERROR_OUT_OF_MEMORY;
      }

      /* Issue SVGA3D device command to define the shader */
      ret = SVGA3D_DefineShader(svga->swc,
                                variant->id,
                                type,
                                variant->tokens,
                                codeLen);
      if (ret != PIPE_OK) {
         /* free the ID */
         assert(variant->id != UTIL_BITMASK_INVALID_INDEX);
         util_bitmask_clear(svga->shader_id_bm, variant->id);
         variant->id = UTIL_BITMASK_INVALID_INDEX;
         return ret;
      }
   }

   return PIPE_OK;
}



enum pipe_error
svga_destroy_shader_variant(struct svga_context *svga,
                            SVGA3dShaderType type,
                            struct svga_shader_variant *variant)
{
   enum pipe_error ret = PIPE_OK;

   if (svga_have_gb_objects(svga)) {
      struct svga_winsys_screen *sws = svga_screen(svga->pipe.screen)->sws;

      sws->shader_destroy(sws, variant->gb_shader);
      variant->gb_shader = NULL;
      goto end;
   }

   /* first try */
   if (variant->id != UTIL_BITMASK_INVALID_INDEX) {
      ret = SVGA3D_DestroyShader(svga->swc, variant->id, type);

      if (ret != PIPE_OK) {
         /* flush and try again */
         svga_context_flush(svga, NULL);

         ret = SVGA3D_DestroyShader(svga->swc, variant->id, type);
         assert(ret == PIPE_OK);
      }

      util_bitmask_clear(svga->shader_id_bm, variant->id);
   }

end:
   FREE((unsigned *)variant->tokens);
   FREE(variant);

   return ret;
}
