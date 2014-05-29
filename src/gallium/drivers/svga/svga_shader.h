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

#ifndef SVGA_SHADER_H
#define SVGA_SHADER_H

#include "svga3d_reg.h"

struct svga_shader_variant;

enum pipe_error
svga_define_shader(struct svga_context *svga,
                   SVGA3dShaderType type,
                   struct svga_shader_variant *variant);

enum pipe_error
svga_destroy_shader_variant(struct svga_context *svga,
                            SVGA3dShaderType type,
                            struct svga_shader_variant *variant);


/**
 * Check if a shader's bytecode exceeds the device limits.
 */
static INLINE boolean
svga_shader_too_large(const struct svga_context *svga,
                      const struct svga_shader_variant *variant)
{
   if (svga_have_gb_objects(svga)) {
      return FALSE;
   }

   if (variant->nr_tokens * sizeof(variant->tokens[0])
       + sizeof(SVGA3dCmdDefineShader) + sizeof(SVGA3dCmdHeader)
       < SVGA_CB_MAX_COMMAND_SIZE) {
      return FALSE;
   }

   return TRUE;
}


#endif /* SVGA_SHADER_H */
