/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/* glsl-optimizer change: Generate direct predicates from compiler-used extension metadata; omit
 * extensions_table.c, runtime names/offsets/years, string enumeration and
 * overrides. Preserve capability aliases, API-specific uint8_t thresholds
 * and the 255 sentinel, including explicit parser API/version checks. */

#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include "mtypes.h"

/* Preserve the upstream uint8_t thresholds, including its 255 sentinel. */
static inline bool
_mesa_extension_version_supported(gl_api api, uint8_t version,
                                  uint8_t compat, uint8_t core,
                                  uint8_t es1, uint8_t es2)
{
   switch (api) {
   case API_OPENGL_COMPAT: return version >= compat;
   case API_OPENGL_CORE:   return version >= core;
   case API_OPENGLES:      return version >= es1;
   case API_OPENGLES2:     return version >= es2;
   default:               return false;
   }
}

/* Parser checks may use an API/version different from the context's. */
#define EXT(name, capability, compat, core, es1, es2) \
static inline bool \
_mesa_has_##name##_for_version(const struct gl_context *ctx, \
                              gl_api api, uint8_t version) \
{ \
   return ctx->Extensions.capability && \
      _mesa_extension_version_supported(api, version, compat, core, es1, es2); \
} \
static inline bool \
_mesa_has_##name(const struct gl_context *ctx) \
{ \
   return _mesa_has_##name##_for_version(ctx, ctx->API, ctx->Extensions.Version); \
}
#include "extensions_table.h"
#undef EXT

#endif /* _EXTENSIONS_H_ */
