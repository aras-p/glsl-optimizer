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

/**
 * \name dispatch_sanity.cpp
 *
 * Verify that only set of functions that should be available in a particular
 * API are available in that API.
 *
 * The list of expected functions originally came from the functions set by
 * api_exec_es2.c.  This file no longer exists in Mesa (but api_exec_es1.c was
 * still generated at the time this test was written).  It was the generated
 * file that configured the dispatch table for ES2 contexts.  This test
 * verifies that all of the functions set by the old api_exec_es2.c (with the
 * recent addition of VAO functions) are set in the dispatch table and
 * everything else is a NOP.
 *
 * When adding extensions that add new functions, this test will need to be
 * modified to expect dispatch functions for the new extension functions.
 */

extern "C" {
#include "main/mfeatures.h"
}

#if FEATURE_ES2

#include <gtest/gtest.h>

extern "C" {
#include "GL/gl.h"
#include "GL/glext.h"
#include "main/compiler.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/remap.h"
#include "glapi/glapi.h"
#include "drivers/common/driverfuncs.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#ifndef GLAPIENTRYP
#define GLAPIENTRYP GL_APIENTRYP
#endif

#include "main/dispatch.h"
}

struct function {
   const char *name;
   int offset;
};

extern const struct function gles2_functions_possible[];

class DispatchSanity_test : public ::testing::Test {
public:
   virtual void SetUp();

   struct gl_config visual;
   struct dd_function_table driver_functions;
   struct gl_context share_list;
   struct gl_context ctx;
};

void
DispatchSanity_test::SetUp()
{
   memset(&visual, 0, sizeof(visual));
   memset(&driver_functions, 0, sizeof(driver_functions));
   memset(&share_list, 0, sizeof(share_list));
   memset(&ctx, 0, sizeof(ctx));

   _mesa_init_driver_functions(&driver_functions);
}

static void
validate_functions(_glapi_proc *table, const struct function *function_table)
{
   for (unsigned i = 0; function_table[i].name != NULL; i++) {
      const int offset = (function_table[i].offset != -1)
         ? function_table[i].offset
         : _glapi_get_proc_offset(function_table[i].name);

      ASSERT_NE(-1, offset)
         << "Function: " << function_table[i].name;
      ASSERT_EQ(offset,
                _glapi_get_proc_offset(function_table[i].name))
         << "Function: " << function_table[i].name;
      EXPECT_NE((_glapi_proc) _mesa_generic_nop, table[offset])
         << "Function: " << function_table[i].name
         << " at offset " << offset;

      table[offset] = (_glapi_proc) _mesa_generic_nop;
   }

   const unsigned size = _glapi_get_dispatch_table_size();
   for (unsigned i = 0; i < size; i++) {
      EXPECT_EQ((_glapi_proc) _mesa_generic_nop, table[i]) << "i = " << i;
   }
}

TEST_F(DispatchSanity_test, GLES2)
{
   ctx.Version = 20;
   _mesa_initialize_context(&ctx,
                            API_OPENGLES2, //api,
                            &visual,
                            NULL, //&share_list,
                            &driver_functions,
                            (void *) NULL);

   _swrast_CreateContext(&ctx);
   _vbo_CreateContext(&ctx);
   _tnl_CreateContext(&ctx);
   _swsetup_CreateContext(&ctx);

   validate_functions((_glapi_proc *) ctx.Exec, gles2_functions_possible);
}

const struct function gles2_functions_possible[] = {
   { "glActiveTexture", _gloffset_ActiveTextureARB },
   { "glAttachShader", -1 },
   { "glBindAttribLocation", -1 },
   { "glBindBuffer", -1 },
   { "glBindFramebuffer", -1 },
   { "glBindRenderbuffer", -1 },
   { "glBindTexture", _gloffset_BindTexture },
   { "glBindVertexArrayOES", -1 },
   { "glBlendColor", _gloffset_BlendColor },
   { "glBlendEquation", _gloffset_BlendEquation },
   { "glBlendEquationSeparate", -1 },
   { "glBlendFunc", _gloffset_BlendFunc },
   { "glBlendFuncSeparate", -1 },
   { "glBufferData", -1 },
   { "glBufferSubData", -1 },
   { "glCheckFramebufferStatus", -1 },
   { "glClear", _gloffset_Clear },
   { "glClearColor", _gloffset_ClearColor },
   { "glClearDepthf", -1 },
   { "glClearStencil", _gloffset_ClearStencil },
   { "glColorMask", _gloffset_ColorMask },
   { "glCompileShader", -1 },
   { "glCompressedTexImage2D", -1 },
   { "glCompressedTexImage3DOES", -1 },
   { "glCompressedTexSubImage2D", -1 },
   { "glCompressedTexSubImage3DOES", -1 },
   { "glCopyTexImage2D", _gloffset_CopyTexImage2D },
   { "glCopyTexSubImage2D", _gloffset_CopyTexSubImage2D },
   { "glCopyTexSubImage3DOES", _gloffset_CopyTexSubImage3D },
   { "glCreateProgram", -1 },
   { "glCreateShader", -1 },
   { "glCullFace", _gloffset_CullFace },
   { "glDeleteBuffers", -1 },
   { "glDeleteFramebuffers", -1 },
   { "glDeleteProgram", -1 },
   { "glDeleteRenderbuffers", -1 },
   { "glDeleteShader", -1 },
   { "glDeleteTextures", _gloffset_DeleteTextures },
   { "glDeleteVertexArraysOES", -1 },
   { "glDepthFunc", _gloffset_DepthFunc },
   { "glDepthMask", _gloffset_DepthMask },
   { "glDepthRangef", -1 },
   { "glDetachShader", -1 },
   { "glDisable", _gloffset_Disable },
   { "glDisableVertexAttribArray", -1 },
   { "glDrawArrays", _gloffset_DrawArrays },
   { "glDrawBuffersNV", -1 },
   { "glDrawElements", _gloffset_DrawElements },
   { "glEGLImageTargetRenderbufferStorageOES", -1 },
   { "glEGLImageTargetTexture2DOES", -1 },
   { "glEnable", _gloffset_Enable },
   { "glEnableVertexAttribArray", -1 },
   { "glFinish", _gloffset_Finish },
   { "glFlush", _gloffset_Flush },
   { "glFramebufferRenderbuffer", -1 },
   { "glFramebufferTexture2D", -1 },
   { "glFramebufferTexture3DOES", -1 },
   { "glFrontFace", _gloffset_FrontFace },
   { "glGenBuffers", -1 },
   { "glGenFramebuffers", -1 },
   { "glGenRenderbuffers", -1 },
   { "glGenTextures", _gloffset_GenTextures },
   { "glGenVertexArraysOES", -1 },
   { "glGenerateMipmap", -1 },
   { "glGetActiveAttrib", -1 },
   { "glGetActiveUniform", -1 },
   { "glGetAttachedShaders", -1 },
   { "glGetAttribLocation", -1 },
   { "glGetBooleanv", _gloffset_GetBooleanv },
   { "glGetBufferParameteriv", -1 },
   { "glGetBufferPointervOES", -1 },
   { "glGetError", _gloffset_GetError },
   { "glGetFloatv", _gloffset_GetFloatv },
   { "glGetFramebufferAttachmentParameteriv", -1 },
   { "glGetIntegerv", _gloffset_GetIntegerv },
   { "glGetProgramInfoLog", -1 },
   { "glGetProgramiv", -1 },
   { "glGetRenderbufferParameteriv", -1 },
   { "glGetShaderInfoLog", -1 },
   { "glGetShaderPrecisionFormat", -1 },
   { "glGetShaderSource", -1 },
   { "glGetShaderiv", -1 },
   { "glGetString", _gloffset_GetString },
   { "glGetTexParameterfv", _gloffset_GetTexParameterfv },
   { "glGetTexParameteriv", _gloffset_GetTexParameteriv },
   { "glGetUniformLocation", -1 },
   { "glGetUniformfv", -1 },
   { "glGetUniformiv", -1 },
   { "glGetVertexAttribPointerv", -1 },
   { "glGetVertexAttribfv", -1 },
   { "glGetVertexAttribiv", -1 },
   { "glHint", _gloffset_Hint },
   { "glIsBuffer", -1 },
   { "glIsEnabled", _gloffset_IsEnabled },
   { "glIsFramebuffer", -1 },
   { "glIsProgram", -1 },
   { "glIsRenderbuffer", -1 },
   { "glIsShader", -1 },
   { "glIsTexture", _gloffset_IsTexture },
   { "glIsVertexArrayOES", -1 },
   { "glLineWidth", _gloffset_LineWidth },
   { "glLinkProgram", -1 },
   { "glMapBufferOES", -1 },
   { "glMultiDrawArraysEXT", -1 },
   { "glMultiDrawElementsEXT", -1 },
   { "glPixelStorei", _gloffset_PixelStorei },
   { "glPolygonOffset", _gloffset_PolygonOffset },
   { "glReadBufferNV", _gloffset_ReadBuffer },
   { "glReadPixels", _gloffset_ReadPixels },
   { "glReleaseShaderCompiler", -1 },
   { "glRenderbufferStorage", -1 },
   { "glSampleCoverage", -1 },
   { "glScissor", _gloffset_Scissor },
   { "glShaderBinary", -1 },
   { "glShaderSource", -1 },
   { "glStencilFunc", _gloffset_StencilFunc },
   { "glStencilFuncSeparate", -1 },
   { "glStencilMask", _gloffset_StencilMask },
   { "glStencilMaskSeparate", -1 },
   { "glStencilOp", _gloffset_StencilOp },
   { "glStencilOpSeparate", -1 },
   { "glTexImage2D", _gloffset_TexImage2D },
   { "glTexImage3DOES", _gloffset_TexImage3D },
   { "glTexParameterf", _gloffset_TexParameterf },
   { "glTexParameterfv", _gloffset_TexParameterfv },
   { "glTexParameteri", _gloffset_TexParameteri },
   { "glTexParameteriv", _gloffset_TexParameteriv },
   { "glTexSubImage2D", _gloffset_TexSubImage2D },
   { "glTexSubImage3DOES", _gloffset_TexSubImage3D },
   { "glUniform1f", -1 },
   { "glUniform1fv", -1 },
   { "glUniform1i", -1 },
   { "glUniform1iv", -1 },
   { "glUniform2f", -1 },
   { "glUniform2fv", -1 },
   { "glUniform2i", -1 },
   { "glUniform2iv", -1 },
   { "glUniform3f", -1 },
   { "glUniform3fv", -1 },
   { "glUniform3i", -1 },
   { "glUniform3iv", -1 },
   { "glUniform4f", -1 },
   { "glUniform4fv", -1 },
   { "glUniform4i", -1 },
   { "glUniform4iv", -1 },
   { "glUniformMatrix2fv", -1 },
   { "glUniformMatrix3fv", -1 },
   { "glUniformMatrix4fv", -1 },
   { "glUnmapBufferOES", -1 },
   { "glUseProgram", -1 },
   { "glValidateProgram", -1 },
   { "glVertexAttrib1f", -1 },
   { "glVertexAttrib1fv", -1 },
   { "glVertexAttrib2f", -1 },
   { "glVertexAttrib2fv", -1 },
   { "glVertexAttrib3f", -1 },
   { "glVertexAttrib3fv", -1 },
   { "glVertexAttrib4f", -1 },
   { "glVertexAttrib4fv", -1 },
   { "glVertexAttribPointer", -1 },
   { "glViewport", _gloffset_Viewport },
   { NULL, -1 }
};

#endif /* FEATURE_ES2 */
