/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "mtypes.h"
#include "context.h"
#include "glformats.h"
#include "macros.h"
#include "mfeatures.h"
#include "enums.h"
#include "fbobject.h"
#include "formatquery.h"

void GLAPIENTRY
_mesa_GetInternalformativ(GLenum target, GLenum internalformat, GLenum pname,
                          GLsizei bufSize, GLint *params)
{
   GLint buffer[16];
   GLsizei count = 0;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.ARB_internalformat_query) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetInternalformativ");
      return;
   }

   assert(ctx->Driver.QuerySamplesForFormat != NULL);

   /* The ARB_internalformat_query spec says:
    *
    *     "If the <target> parameter to GetInternalformativ is not one of
    *     TEXTURE_2D_MULTISAMPLE, TEXTURE_2D_MULTISAMPLE_ARRAY or RENDERBUFFER
    *     then an INVALID_ENUM error is generated."
    */
   switch (target) {
   case GL_RENDERBUFFER:
      break;

   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      /* Mesa does not currently support GL_ARB_texture_multisample, so these
       * enums are not valid on this implementation either.
       */
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetInternalformativ(target=%s)",
                  _mesa_lookup_enum_by_nr(target));
      return;
   }

   /* The ARB_internalformat_query spec says:
    *
    *     "If the <internalformat> parameter to GetInternalformativ is not
    *     color-, depth- or stencil-renderable, then an INVALID_ENUM error is
    *     generated."
    */
   if (_mesa_base_fbo_format(ctx, internalformat) == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetInternalformativ(internalformat=%s)",
                  _mesa_lookup_enum_by_nr(internalformat));
      return;
   }

   /* The ARB_internalformat_query spec says:
    *
    *     "If the <bufSize> parameter to GetInternalformativ is negative, then
    *     an INVALID_VALUE error is generated."
    */
   if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetInternalformativ(target=%s)",
                  _mesa_lookup_enum_by_nr(target));
      return;
   }

   switch (pname) {
   case GL_SAMPLES:
      count = ctx->Driver.QuerySamplesForFormat(ctx, internalformat, buffer);
      break;
   case GL_NUM_SAMPLE_COUNTS: {
      /* The driver can return 0, and we should pass that along to the
       * application.  The ARB decided that ARB_internalformat_query should
       * behave as ARB_internalformat_query2 in this situation.
       *
       * The ARB_internalformat_query2 spec says:
       *
       *     "- NUM_SAMPLE_COUNTS: The number of sample counts that would be
       *        returned by querying SAMPLES is returned in <params>.
       *        * If <internalformat> is not color-renderable,
       *          depth-renderable, or stencil-renderable (as defined in
       *          section 4.4.4), or if <target> does not support multiple
       *          samples (ie other than TEXTURE_2D_MULTISAMPLE,
       *          TEXTURE_2D_MULTISAMPLE_ARRAY, or RENDERBUFFER), 0 is
       *          returned."
       */
      const size_t num_samples =
         ctx->Driver.QuerySamplesForFormat(ctx, internalformat, buffer);

      /* QuerySamplesForFormat writes some stuff to buffer, so we have to
       * separately over-write it with the requested value.
       */
      buffer[0] = (GLint) num_samples;
      count = 1;
      break;
   }
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetInternalformativ(pname=%s)",
                  _mesa_lookup_enum_by_nr(pname));
      return;
   }

   if (bufSize != 0 && params == NULL) {
      /* Emit a warning to aid application debugging, but go ahead and do the
       * memcpy (and probably crash) anyway.
       */
      _mesa_warning(ctx,
                    "glGetInternalformativ(bufSize = %d, but params = NULL)",
                    bufSize);
   }

   /* Copy the data from the temporary buffer to the buffer supplied by the
    * application.  Clamp the size of the copy to the size supplied by the
    * application.
    */
   memcpy(params, buffer, MIN2(count, bufSize) * sizeof(GLint));

   return;
}
