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

#include <gtest/gtest.h>

extern "C" {
#include "GL/gl.h"
#include "GL/glext.h"
#include "main/compiler.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/remap.h"
#include "main/vtxfmt.h"
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
   unsigned int Version;
   int offset;
};

extern const struct function gl_core_functions_possible[];
extern const struct function gles11_functions_possible[];
extern const struct function gles2_functions_possible[];
extern const struct function gles3_functions_possible[];

class DispatchSanity_test : public ::testing::Test {
public:
   virtual void SetUp();
   void SetUpCtx(gl_api api, unsigned int version);

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

void
DispatchSanity_test::SetUpCtx(gl_api api, unsigned int version)
{
   _mesa_initialize_context(&ctx,
                            api,
                            &visual,
                            NULL, // share_list
                            &driver_functions);
   _vbo_CreateContext(&ctx);

   ctx.Version = version;

   _mesa_initialize_dispatch_tables(&ctx);
   _mesa_initialize_vbo_vtxfmt(&ctx);
}

static const char *
offset_to_proc_name_safe(unsigned offset)
{
   const char *name = _glapi_get_proc_name(offset);
   return name ? name : "???";
}

/* Scan through the dispatch table and check that all the functions in
 * _glapi_proc *table exist. When found, set their pointers in the table
 * to _mesa_generic_nop.  */
static void
validate_functions(struct gl_context *ctx, const struct function *function_table)
{
   _glapi_proc *table = (_glapi_proc *) ctx->Exec;

   for (unsigned i = 0; function_table[i].name != NULL; i++) {
      /* The context version is >= the GL version where the
         function was introduced. Therefore, the function cannot
         be set to the nop function.
       */
      bool cant_be_nop = ctx->Version >= function_table[i].Version;

      const int offset = (function_table[i].offset != -1)
         ? function_table[i].offset
         : _glapi_get_proc_offset(function_table[i].name);

      ASSERT_NE(-1, offset)
         << "Function: " << function_table[i].name;
      ASSERT_EQ(offset,
                _glapi_get_proc_offset(function_table[i].name))
         << "Function: " << function_table[i].name;
      if (cant_be_nop) {
         EXPECT_NE((_glapi_proc) _mesa_generic_nop, table[offset])
            << "Function: " << function_table[i].name
            << " at offset " << offset;
      }

      table[offset] = (_glapi_proc) _mesa_generic_nop;
   }
}

/* Scan through the table and ensure that there is nothing except
 * _mesa_generic_nop (as set by validate_functions().  */
static void
validate_nops(struct gl_context *ctx)
{
   _glapi_proc *table = (_glapi_proc *) ctx->Exec;

   const unsigned size = _glapi_get_dispatch_table_size();
   for (unsigned i = 0; i < size; i++) {
      EXPECT_EQ((_glapi_proc) _mesa_generic_nop, table[i])
         << "i = " << i << " (" << offset_to_proc_name_safe(i) << ")";
   }
}

TEST_F(DispatchSanity_test, GL31_CORE)
{
   SetUpCtx(API_OPENGL_CORE, 31);
   validate_functions(&ctx, gl_core_functions_possible);
   validate_nops(&ctx);
}

TEST_F(DispatchSanity_test, GLES11)
{
   SetUpCtx(API_OPENGLES, 11);
   validate_functions(&ctx, gles11_functions_possible);
   validate_nops(&ctx);
}

TEST_F(DispatchSanity_test, GLES2)
{
   SetUpCtx(API_OPENGLES2, 20);
   validate_functions(&ctx, gles2_functions_possible);
   validate_nops(&ctx);
}

TEST_F(DispatchSanity_test, GLES3)
{
   SetUpCtx(API_OPENGLES2, 30);
   validate_functions(&ctx, gles2_functions_possible);
   validate_functions(&ctx, gles3_functions_possible);
   validate_nops(&ctx);
}

const struct function gl_core_functions_possible[] = {
   { "glCullFace", 10, -1 },
   { "glFrontFace", 10, -1 },
   { "glHint", 10, -1 },
   { "glLineWidth", 10, -1 },
   { "glPointSize", 10, -1 },
   { "glPolygonMode", 10, -1 },
   { "glScissor", 10, -1 },
   { "glTexParameterf", 10, -1 },
   { "glTexParameterfv", 10, -1 },
   { "glTexParameteri", 10, -1 },
   { "glTexParameteriv", 10, -1 },
   { "glTexImage1D", 10, -1 },
   { "glTexImage2D", 10, -1 },
   { "glDrawBuffer", 10, -1 },
   { "glClear", 10, -1 },
   { "glClearColor", 10, -1 },
   { "glClearStencil", 10, -1 },
   { "glClearDepth", 10, -1 },
   { "glStencilMask", 10, -1 },
   { "glColorMask", 10, -1 },
   { "glDepthMask", 10, -1 },
   { "glDisable", 10, -1 },
   { "glEnable", 10, -1 },
   { "glFinish", 10, -1 },
   { "glFlush", 10, -1 },
   { "glBlendFunc", 10, -1 },
   { "glLogicOp", 10, -1 },
   { "glStencilFunc", 10, -1 },
   { "glStencilOp", 10, -1 },
   { "glDepthFunc", 10, -1 },
   { "glPixelStoref", 10, -1 },
   { "glPixelStorei", 10, -1 },
   { "glReadBuffer", 10, -1 },
   { "glReadPixels", 10, -1 },
   { "glGetBooleanv", 10, -1 },
   { "glGetDoublev", 10, -1 },
   { "glGetError", 10, -1 },
   { "glGetFloatv", 10, -1 },
   { "glGetIntegerv", 10, -1 },
   { "glGetString", 10, -1 },
   { "glGetTexImage", 10, -1 },
   { "glGetTexParameterfv", 10, -1 },
   { "glGetTexParameteriv", 10, -1 },
   { "glGetTexLevelParameterfv", 10, -1 },
   { "glGetTexLevelParameteriv", 10, -1 },
   { "glIsEnabled", 10, -1 },
   { "glDepthRange", 10, -1 },
   { "glViewport", 10, -1 },

   /* GL 1.1 */
   { "glDrawArrays", 11, -1 },
   { "glDrawElements", 11, -1 },
   { "glGetPointerv", 11, -1 },
   { "glPolygonOffset", 11, -1 },
   { "glCopyTexImage1D", 11, -1 },
   { "glCopyTexImage2D", 11, -1 },
   { "glCopyTexSubImage1D", 11, -1 },
   { "glCopyTexSubImage2D", 11, -1 },
   { "glTexSubImage1D", 11, -1 },
   { "glTexSubImage2D", 11, -1 },
   { "glBindTexture", 11, -1 },
   { "glDeleteTextures", 11, -1 },
   { "glGenTextures", 11, -1 },
   { "glIsTexture", 11, -1 },

   /* GL 1.2 */
   { "glBlendColor", 12, -1 },
   { "glBlendEquation", 12, -1 },
   { "glDrawRangeElements", 12, -1 },
   { "glTexImage3D", 12, -1 },
   { "glTexSubImage3D", 12, -1 },
   { "glCopyTexSubImage3D", 12, -1 },

   /* GL 1.3 */
   { "glActiveTexture", 13, -1 },
   { "glSampleCoverage", 13, -1 },
   { "glCompressedTexImage3D", 13, -1 },
   { "glCompressedTexImage2D", 13, -1 },
   { "glCompressedTexImage1D", 13, -1 },
   { "glCompressedTexSubImage3D", 13, -1 },
   { "glCompressedTexSubImage2D", 13, -1 },
   { "glCompressedTexSubImage1D", 13, -1 },
   { "glGetCompressedTexImage", 13, -1 },

   /* GL 1.4 */
   { "glBlendFuncSeparate", 14, -1 },
   { "glMultiDrawArrays", 14, -1 },
   { "glMultiDrawElements", 14, -1 },
   { "glPointParameterf", 14, -1 },
   { "glPointParameterfv", 14, -1 },
   { "glPointParameteri", 14, -1 },
   { "glPointParameteriv", 14, -1 },

   /* GL 1.5 */
   { "glGenQueries", 15, -1 },
   { "glDeleteQueries", 15, -1 },
   { "glIsQuery", 15, -1 },
   { "glBeginQuery", 15, -1 },
   { "glEndQuery", 15, -1 },
   { "glGetQueryiv", 15, -1 },
   { "glGetQueryObjectiv", 15, -1 },
   { "glGetQueryObjectuiv", 15, -1 },
   { "glBindBuffer", 15, -1 },
   { "glDeleteBuffers", 15, -1 },
   { "glGenBuffers", 15, -1 },
   { "glIsBuffer", 15, -1 },
   { "glBufferData", 15, -1 },
   { "glBufferSubData", 15, -1 },
   { "glGetBufferSubData", 15, -1 },
   { "glMapBuffer", 15, -1 },
   { "glUnmapBuffer", 15, -1 },
   { "glGetBufferParameteriv", 15, -1 },
   { "glGetBufferPointerv", 15, -1 },

   /* GL 2.0 */
   { "glBlendEquationSeparate", 20, -1 },
   { "glDrawBuffers", 20, -1 },
   { "glStencilOpSeparate", 20, -1 },
   { "glStencilFuncSeparate", 20, -1 },
   { "glStencilMaskSeparate", 20, -1 },
   { "glAttachShader", 20, -1 },
   { "glBindAttribLocation", 20, -1 },
   { "glCompileShader", 20, -1 },
   { "glCreateProgram", 20, -1 },
   { "glCreateShader", 20, -1 },
   { "glDeleteProgram", 20, -1 },
   { "glDeleteShader", 20, -1 },
   { "glDetachShader", 20, -1 },
   { "glDisableVertexAttribArray", 20, -1 },
   { "glEnableVertexAttribArray", 20, -1 },
   { "glGetActiveAttrib", 20, -1 },
   { "glGetActiveUniform", 20, -1 },
   { "glGetAttachedShaders", 20, -1 },
   { "glGetAttribLocation", 20, -1 },
   { "glGetProgramiv", 20, -1 },
   { "glGetProgramInfoLog", 20, -1 },
   { "glGetShaderiv", 20, -1 },
   { "glGetShaderInfoLog", 20, -1 },
   { "glGetShaderSource", 20, -1 },
   { "glGetUniformLocation", 20, -1 },
   { "glGetUniformfv", 20, -1 },
   { "glGetUniformiv", 20, -1 },
   { "glGetVertexAttribdv", 20, -1 },
   { "glGetVertexAttribfv", 20, -1 },
   { "glGetVertexAttribiv", 20, -1 },
   { "glGetVertexAttribPointerv", 20, -1 },
   { "glIsProgram", 20, -1 },
   { "glIsShader", 20, -1 },
   { "glLinkProgram", 20, -1 },
   { "glShaderSource", 20, -1 },
   { "glUseProgram", 20, -1 },
   { "glUniform1f", 20, -1 },
   { "glUniform2f", 20, -1 },
   { "glUniform3f", 20, -1 },
   { "glUniform4f", 20, -1 },
   { "glUniform1i", 20, -1 },
   { "glUniform2i", 20, -1 },
   { "glUniform3i", 20, -1 },
   { "glUniform4i", 20, -1 },
   { "glUniform1fv", 20, -1 },
   { "glUniform2fv", 20, -1 },
   { "glUniform3fv", 20, -1 },
   { "glUniform4fv", 20, -1 },
   { "glUniform1iv", 20, -1 },
   { "glUniform2iv", 20, -1 },
   { "glUniform3iv", 20, -1 },
   { "glUniform4iv", 20, -1 },
   { "glUniformMatrix2fv", 20, -1 },
   { "glUniformMatrix3fv", 20, -1 },
   { "glUniformMatrix4fv", 20, -1 },
   { "glValidateProgram", 20, -1 },
   { "glVertexAttrib1d", 20, -1 },
   { "glVertexAttrib1dv", 20, -1 },
   { "glVertexAttrib1f", 20, -1 },
   { "glVertexAttrib1fv", 20, -1 },
   { "glVertexAttrib1s", 20, -1 },
   { "glVertexAttrib1sv", 20, -1 },
   { "glVertexAttrib2d", 20, -1 },
   { "glVertexAttrib2dv", 20, -1 },
   { "glVertexAttrib2f", 20, -1 },
   { "glVertexAttrib2fv", 20, -1 },
   { "glVertexAttrib2s", 20, -1 },
   { "glVertexAttrib2sv", 20, -1 },
   { "glVertexAttrib3d", 20, -1 },
   { "glVertexAttrib3dv", 20, -1 },
   { "glVertexAttrib3f", 20, -1 },
   { "glVertexAttrib3fv", 20, -1 },
   { "glVertexAttrib3s", 20, -1 },
   { "glVertexAttrib3sv", 20, -1 },
   { "glVertexAttrib4Nbv", 20, -1 },
   { "glVertexAttrib4Niv", 20, -1 },
   { "glVertexAttrib4Nsv", 20, -1 },
   { "glVertexAttrib4Nub", 20, -1 },
   { "glVertexAttrib4Nubv", 20, -1 },
   { "glVertexAttrib4Nuiv", 20, -1 },
   { "glVertexAttrib4Nusv", 20, -1 },
   { "glVertexAttrib4bv", 20, -1 },
   { "glVertexAttrib4d", 20, -1 },
   { "glVertexAttrib4dv", 20, -1 },
   { "glVertexAttrib4f", 20, -1 },
   { "glVertexAttrib4fv", 20, -1 },
   { "glVertexAttrib4iv", 20, -1 },
   { "glVertexAttrib4s", 20, -1 },
   { "glVertexAttrib4sv", 20, -1 },
   { "glVertexAttrib4ubv", 20, -1 },
   { "glVertexAttrib4uiv", 20, -1 },
   { "glVertexAttrib4usv", 20, -1 },
   { "glVertexAttribPointer", 20, -1 },

   /* GL 2.1 */
   { "glUniformMatrix2x3fv", 21, -1 },
   { "glUniformMatrix3x2fv", 21, -1 },
   { "glUniformMatrix2x4fv", 21, -1 },
   { "glUniformMatrix4x2fv", 21, -1 },
   { "glUniformMatrix3x4fv", 21, -1 },
   { "glUniformMatrix4x3fv", 21, -1 },

   /* GL 3.0 */
   { "glColorMaski", 30, -1 },
   { "glGetBooleani_v", 30, -1 },
   { "glGetIntegeri_v", 30, -1 },
   { "glEnablei", 30, -1 },
   { "glDisablei", 30, -1 },
   { "glIsEnabledi", 30, -1 },
   { "glBeginTransformFeedback", 30, -1 },
   { "glEndTransformFeedback", 30, -1 },
   { "glBindBufferRange", 30, -1 },
   { "glBindBufferBase", 30, -1 },
   { "glTransformFeedbackVaryings", 30, -1 },
   { "glGetTransformFeedbackVarying", 30, -1 },
   { "glClampColor", 30, -1 },
   { "glBeginConditionalRender", 30, -1 },
   { "glEndConditionalRender", 30, -1 },
   { "glVertexAttribIPointer", 30, -1 },
   { "glGetVertexAttribIiv", 30, -1 },
   { "glGetVertexAttribIuiv", 30, -1 },
   { "glVertexAttribI1i", 30, -1 },
   { "glVertexAttribI2i", 30, -1 },
   { "glVertexAttribI3i", 30, -1 },
   { "glVertexAttribI4i", 30, -1 },
   { "glVertexAttribI1ui", 30, -1 },
   { "glVertexAttribI2ui", 30, -1 },
   { "glVertexAttribI3ui", 30, -1 },
   { "glVertexAttribI4ui", 30, -1 },
   { "glVertexAttribI1iv", 30, -1 },
   { "glVertexAttribI2iv", 30, -1 },
   { "glVertexAttribI3iv", 30, -1 },
   { "glVertexAttribI4iv", 30, -1 },
   { "glVertexAttribI1uiv", 30, -1 },
   { "glVertexAttribI2uiv", 30, -1 },
   { "glVertexAttribI3uiv", 30, -1 },
   { "glVertexAttribI4uiv", 30, -1 },
   { "glVertexAttribI4bv", 30, -1 },
   { "glVertexAttribI4sv", 30, -1 },
   { "glVertexAttribI4ubv", 30, -1 },
   { "glVertexAttribI4usv", 30, -1 },
   { "glGetUniformuiv", 30, -1 },
   { "glBindFragDataLocation", 30, -1 },
   { "glGetFragDataLocation", 30, -1 },
   { "glUniform1ui", 30, -1 },
   { "glUniform2ui", 30, -1 },
   { "glUniform3ui", 30, -1 },
   { "glUniform4ui", 30, -1 },
   { "glUniform1uiv", 30, -1 },
   { "glUniform2uiv", 30, -1 },
   { "glUniform3uiv", 30, -1 },
   { "glUniform4uiv", 30, -1 },
   { "glTexParameterIiv", 30, -1 },
   { "glTexParameterIuiv", 30, -1 },
   { "glGetTexParameterIiv", 30, -1 },
   { "glGetTexParameterIuiv", 30, -1 },
   { "glClearBufferiv", 30, -1 },
   { "glClearBufferuiv", 30, -1 },
   { "glClearBufferfv", 30, -1 },
   { "glClearBufferfi", 30, -1 },
   { "glGetStringi", 30, -1 },

   /* GL 3.1 */
   { "glDrawArraysInstanced", 31, -1 },
   { "glDrawElementsInstanced", 31, -1 },
   { "glTexBuffer", 31, -1 },
   { "glPrimitiveRestartIndex", 31, -1 },

   /* GL_ARB_shader_objects */
   { "glDeleteObjectARB", 31, -1 },
   { "glGetHandleARB", 31, -1 },
   { "glDetachObjectARB", 31, -1 },
   { "glCreateShaderObjectARB", 31, -1 },
   { "glCreateProgramObjectARB", 31, -1 },
   { "glAttachObjectARB", 31, -1 },
   { "glGetObjectParameterfvARB", 31, -1 },
   { "glGetObjectParameterivARB", 31, -1 },
   { "glGetInfoLogARB", 31, -1 },
   { "glGetAttachedObjectsARB", 31, -1 },

   /* GL_ARB_get_program_binary */
   { "glGetProgramBinary", 30, -1 },
   { "glProgramBinary", 30, -1 },
   { "glProgramParameteri", 30, -1 },

   /* GL_EXT_transform_feedback */
   { "glBindBufferOffsetEXT", 31, -1 },

   /* GL_IBM_multimode_draw_arrays */
   { "glMultiModeDrawArraysIBM", 31, -1 },
   { "glMultiModeDrawElementsIBM", 31, -1 },

   /* GL_EXT_depth_bounds_test */
   { "glDepthBoundsEXT", 31, -1 },

   /* GL_apple_object_purgeable */
   { "glObjectPurgeableAPPLE", 31, -1 },
   { "glObjectUnpurgeableAPPLE", 31, -1 },
   { "glGetObjectParameterivAPPLE", 31, -1 },

   /* GL_ARB_instanced_arrays */
   { "glVertexAttribDivisorARB", 31, -1 },

   /* GL_NV_texture_barrier */
   { "glTextureBarrierNV", 31, -1 },

   /* GL_EXT_texture_integer */
   { "glClearColorIiEXT", 31, -1 },
   { "glClearColorIuiEXT", 31, -1 },

   /* GL_OES_EGL_image */
   { "glEGLImageTargetRenderbufferStorageOES", 31, -1 },
   { "glEGLImageTargetTexture2DOES", 31, -1 },

   /* GL 3.2 */
   { "glGetInteger64i_v", 32, -1 },
   { "glGetBufferParameteri64v", 32, -1 },
   { "glFramebufferTexture", 32, -1 },

   /* GL_ARB_geometry_shader4 */
   { "glProgramParameteriARB", 32, -1 },
   { "glFramebufferTextureARB", 32, -1 },
   { "glFramebufferTextureLayerARB", 32, -1 },
   { "glFramebufferTextureFaceARB", 32, -1 },

   /* GL 3.3 */
   { "glVertexAttribDivisor", 33, -1 },

   /* GL 4.0 */
   { "glMinSampleShading", 40, -1 },                    // XXX: Add to xml
// { "glBlendEquationi", 40, -1 },                      // XXX: Add to xml
// { "glBlendEquationSeparatei", 40, -1 },              // XXX: Add to xml
// { "glBlendFunci", 40, -1 },                          // XXX: Add to xml
// { "glBlendFuncSeparatei", 40, -1 },                  // XXX: Add to xml

   /* GL 4.3 */
   { "glIsRenderbuffer", 43, -1 },
   { "glBindRenderbuffer", 43, -1 },
   { "glDeleteRenderbuffers", 43, -1 },
   { "glGenRenderbuffers", 43, -1 },
   { "glRenderbufferStorage", 43, -1 },
   { "glGetRenderbufferParameteriv", 43, -1 },
   { "glIsFramebuffer", 43, -1 },
   { "glBindFramebuffer", 43, -1 },
   { "glDeleteFramebuffers", 43, -1 },
   { "glGenFramebuffers", 43, -1 },
   { "glCheckFramebufferStatus", 43, -1 },
   { "glFramebufferTexture1D", 43, -1 },
   { "glFramebufferTexture2D", 43, -1 },
   { "glFramebufferTexture3D", 43, -1 },
   { "glFramebufferRenderbuffer", 43, -1 },
   { "glGetFramebufferAttachmentParameteriv", 43, -1 },
   { "glGenerateMipmap", 43, -1 },
   { "glBlitFramebuffer", 43, -1 },
   { "glRenderbufferStorageMultisample", 43, -1 },
   { "glFramebufferTextureLayer", 43, -1 },
   { "glMapBufferRange", 43, -1 },
   { "glFlushMappedBufferRange", 43, -1 },
   { "glBindVertexArray", 43, -1 },
   { "glDeleteVertexArrays", 43, -1 },
   { "glGenVertexArrays", 43, -1 },
   { "glIsVertexArray", 43, -1 },
   { "glGetUniformIndices", 43, -1 },
   { "glGetActiveUniformsiv", 43, -1 },
   { "glGetActiveUniformName", 43, -1 },
   { "glGetUniformBlockIndex", 43, -1 },
   { "glGetActiveUniformBlockiv", 43, -1 },
   { "glGetActiveUniformBlockName", 43, -1 },
   { "glUniformBlockBinding", 43, -1 },
   { "glCopyBufferSubData", 43, -1 },
   { "glDrawElementsBaseVertex", 43, -1 },
   { "glDrawRangeElementsBaseVertex", 43, -1 },
   { "glDrawElementsInstancedBaseVertex", 43, -1 },
   { "glMultiDrawElementsBaseVertex", 43, -1 },
   { "glProvokingVertex", 43, -1 },
   { "glFenceSync", 43, -1 },
   { "glIsSync", 43, -1 },
   { "glDeleteSync", 43, -1 },
   { "glClientWaitSync", 43, -1 },
   { "glWaitSync", 43, -1 },
   { "glGetInteger64v", 43, -1 },
   { "glGetSynciv", 43, -1 },
   { "glTexImage2DMultisample", 43, -1 },
   { "glTexImage3DMultisample", 43, -1 },
   { "glGetMultisamplefv", 43, -1 },
   { "glSampleMaski", 43, -1 },
   { "glBlendEquationiARB", 43, -1 },
   { "glBlendEquationSeparateiARB", 43, -1 },
   { "glBlendFunciARB", 43, -1 },
   { "glBlendFuncSeparateiARB", 43, -1 },
   { "glMinSampleShadingARB", 43, -1 },                 // XXX: Add to xml
// { "glNamedStringARB", 43, -1 },                      // XXX: Add to xml
// { "glDeleteNamedStringARB", 43, -1 },                // XXX: Add to xml
// { "glCompileShaderIncludeARB", 43, -1 },             // XXX: Add to xml
// { "glIsNamedStringARB", 43, -1 },                    // XXX: Add to xml
// { "glGetNamedStringARB", 43, -1 },                   // XXX: Add to xml
// { "glGetNamedStringivARB", 43, -1 },                 // XXX: Add to xml
   { "glBindFragDataLocationIndexed", 43, -1 },
   { "glGetFragDataIndex", 43, -1 },
   { "glGenSamplers", 43, -1 },
   { "glDeleteSamplers", 43, -1 },
   { "glIsSampler", 43, -1 },
   { "glBindSampler", 43, -1 },
   { "glSamplerParameteri", 43, -1 },
   { "glSamplerParameteriv", 43, -1 },
   { "glSamplerParameterf", 43, -1 },
   { "glSamplerParameterfv", 43, -1 },
   { "glSamplerParameterIiv", 43, -1 },
   { "glSamplerParameterIuiv", 43, -1 },
   { "glGetSamplerParameteriv", 43, -1 },
   { "glGetSamplerParameterIiv", 43, -1 },
   { "glGetSamplerParameterfv", 43, -1 },
   { "glGetSamplerParameterIuiv", 43, -1 },
   { "glQueryCounter", 43, -1 },
   { "glGetQueryObjecti64v", 43, -1 },
   { "glGetQueryObjectui64v", 43, -1 },
   { "glVertexP2ui", 43, -1 },
   { "glVertexP2uiv", 43, -1 },
   { "glVertexP3ui", 43, -1 },
   { "glVertexP3uiv", 43, -1 },
   { "glVertexP4ui", 43, -1 },
   { "glVertexP4uiv", 43, -1 },
   { "glTexCoordP1ui", 43, -1 },
   { "glTexCoordP1uiv", 43, -1 },
   { "glTexCoordP2ui", 43, -1 },
   { "glTexCoordP2uiv", 43, -1 },
   { "glTexCoordP3ui", 43, -1 },
   { "glTexCoordP3uiv", 43, -1 },
   { "glTexCoordP4ui", 43, -1 },
   { "glTexCoordP4uiv", 43, -1 },
   { "glMultiTexCoordP1ui", 43, -1 },
   { "glMultiTexCoordP1uiv", 43, -1 },
   { "glMultiTexCoordP2ui", 43, -1 },
   { "glMultiTexCoordP2uiv", 43, -1 },
   { "glMultiTexCoordP3ui", 43, -1 },
   { "glMultiTexCoordP3uiv", 43, -1 },
   { "glMultiTexCoordP4ui", 43, -1 },
   { "glMultiTexCoordP4uiv", 43, -1 },
   { "glNormalP3ui", 43, -1 },
   { "glNormalP3uiv", 43, -1 },
   { "glColorP3ui", 43, -1 },
   { "glColorP3uiv", 43, -1 },
   { "glColorP4ui", 43, -1 },
   { "glColorP4uiv", 43, -1 },
   { "glSecondaryColorP3ui", 43, -1 },
   { "glSecondaryColorP3uiv", 43, -1 },
   { "glVertexAttribP1ui", 43, -1 },
   { "glVertexAttribP1uiv", 43, -1 },
   { "glVertexAttribP2ui", 43, -1 },
   { "glVertexAttribP2uiv", 43, -1 },
   { "glVertexAttribP3ui", 43, -1 },
   { "glVertexAttribP3uiv", 43, -1 },
   { "glVertexAttribP4ui", 43, -1 },
   { "glVertexAttribP4uiv", 43, -1 },
   { "glDrawArraysIndirect", 43, -1 },
   { "glDrawElementsIndirect", 43, -1 },
// { "glUniform1d", 43, -1 },                           // XXX: Add to xml
// { "glUniform2d", 43, -1 },                           // XXX: Add to xml
// { "glUniform3d", 43, -1 },                           // XXX: Add to xml
// { "glUniform4d", 43, -1 },                           // XXX: Add to xml
// { "glUniform1dv", 43, -1 },                          // XXX: Add to xml
// { "glUniform2dv", 43, -1 },                          // XXX: Add to xml
// { "glUniform3dv", 43, -1 },                          // XXX: Add to xml
// { "glUniform4dv", 43, -1 },                          // XXX: Add to xml
// { "glUniformMatrix2dv", 43, -1 },                    // XXX: Add to xml
// { "glUniformMatrix3dv", 43, -1 },                    // XXX: Add to xml
// { "glUniformMatrix4dv", 43, -1 },                    // XXX: Add to xml
// { "glUniformMatrix2x3dv", 43, -1 },                  // XXX: Add to xml
// { "glUniformMatrix2x4dv", 43, -1 },                  // XXX: Add to xml
// { "glUniformMatrix3x2dv", 43, -1 },                  // XXX: Add to xml
// { "glUniformMatrix3x4dv", 43, -1 },                  // XXX: Add to xml
// { "glUniformMatrix4x2dv", 43, -1 },                  // XXX: Add to xml
// { "glUniformMatrix4x3dv", 43, -1 },                  // XXX: Add to xml
// { "glGetUniformdv", 43, -1 },                        // XXX: Add to xml
// { "glGetSubroutineUniformLocation", 43, -1 },        // XXX: Add to xml
// { "glGetSubroutineIndex", 43, -1 },                  // XXX: Add to xml
// { "glGetActiveSubroutineUniformiv", 43, -1 },        // XXX: Add to xml
// { "glGetActiveSubroutineUniformName", 43, -1 },      // XXX: Add to xml
// { "glGetActiveSubroutineName", 43, -1 },             // XXX: Add to xml
// { "glUniformSubroutinesuiv", 43, -1 },               // XXX: Add to xml
// { "glGetUniformSubroutineuiv", 43, -1 },             // XXX: Add to xml
// { "glGetProgramStageiv", 43, -1 },                   // XXX: Add to xml
// { "glPatchParameteri", 43, -1 },                     // XXX: Add to xml
// { "glPatchParameterfv", 43, -1 },                    // XXX: Add to xml
   { "glBindTransformFeedback", 43, -1 },
   { "glDeleteTransformFeedbacks", 43, -1 },
   { "glGenTransformFeedbacks", 43, -1 },
   { "glIsTransformFeedback", 43, -1 },
   { "glPauseTransformFeedback", 43, -1 },
   { "glResumeTransformFeedback", 43, -1 },
   { "glDrawTransformFeedback", 43, -1 },
   { "glDrawTransformFeedbackStream", 43, -1 },
   { "glBeginQueryIndexed", 43, -1 },
   { "glEndQueryIndexed", 43, -1 },
   { "glGetQueryIndexediv", 43, -1 },
   { "glReleaseShaderCompiler", 43, -1 },
   { "glShaderBinary", 43, -1 },
   { "glGetShaderPrecisionFormat", 43, -1 },
   { "glDepthRangef", 43, -1 },
   { "glClearDepthf", 43, -1 },
   { "glGetProgramBinary", 43, -1 },
   { "glProgramBinary", 43, -1 },
   { "glProgramParameteri", 43, -1 },
   { "glUseProgramStages", 43, -1 },
   { "glActiveShaderProgram", 43, -1 },
   { "glCreateShaderProgramv", 43, -1 },
   { "glBindProgramPipeline", 43, -1 },
   { "glDeleteProgramPipelines", 43, -1 },
   { "glGenProgramPipelines", 43, -1 },
   { "glIsProgramPipeline", 43, -1 },
   { "glGetProgramPipelineiv", 43, -1 },
   { "glProgramUniform1i", 43, -1 },
   { "glProgramUniform1iv", 43, -1 },
   { "glProgramUniform1f", 43, -1 },
   { "glProgramUniform1fv", 43, -1 },
// { "glProgramUniform1d", 43, -1 },                    // XXX: Add to xml
// { "glProgramUniform1dv", 43, -1 },                   // XXX: Add to xml
   { "glProgramUniform1ui", 43, -1 },
   { "glProgramUniform1uiv", 43, -1 },
   { "glProgramUniform2i", 43, -1 },
   { "glProgramUniform2iv", 43, -1 },
   { "glProgramUniform2f", 43, -1 },
   { "glProgramUniform2fv", 43, -1 },
// { "glProgramUniform2d", 43, -1 },                    // XXX: Add to xml
// { "glProgramUniform2dv", 43, -1 },                   // XXX: Add to xml
   { "glProgramUniform2ui", 43, -1 },
   { "glProgramUniform2uiv", 43, -1 },
   { "glProgramUniform3i", 43, -1 },
   { "glProgramUniform3iv", 43, -1 },
   { "glProgramUniform3f", 43, -1 },
   { "glProgramUniform3fv", 43, -1 },
// { "glProgramUniform3d", 43, -1 },                    // XXX: Add to xml
// { "glProgramUniform3dv", 43, -1 },                   // XXX: Add to xml
   { "glProgramUniform3ui", 43, -1 },
   { "glProgramUniform3uiv", 43, -1 },
   { "glProgramUniform4i", 43, -1 },
   { "glProgramUniform4iv", 43, -1 },
   { "glProgramUniform4f", 43, -1 },
   { "glProgramUniform4fv", 43, -1 },
// { "glProgramUniform4d", 43, -1 },                    // XXX: Add to xml
// { "glProgramUniform4dv", 43, -1 },                   // XXX: Add to xml
   { "glProgramUniform4ui", 43, -1 },
   { "glProgramUniform4uiv", 43, -1 },
   { "glProgramUniformMatrix2fv", 43, -1 },
   { "glProgramUniformMatrix3fv", 43, -1 },
   { "glProgramUniformMatrix4fv", 43, -1 },
// { "glProgramUniformMatrix2dv", 43, -1 },             // XXX: Add to xml
// { "glProgramUniformMatrix3dv", 43, -1 },             // XXX: Add to xml
// { "glProgramUniformMatrix4dv", 43, -1 },             // XXX: Add to xml
   { "glProgramUniformMatrix2x3fv", 43, -1 },
   { "glProgramUniformMatrix3x2fv", 43, -1 },
   { "glProgramUniformMatrix2x4fv", 43, -1 },
   { "glProgramUniformMatrix4x2fv", 43, -1 },
   { "glProgramUniformMatrix3x4fv", 43, -1 },
   { "glProgramUniformMatrix4x3fv", 43, -1 },
// { "glProgramUniformMatrix2x3dv", 43, -1 },           // XXX: Add to xml
// { "glProgramUniformMatrix3x2dv", 43, -1 },           // XXX: Add to xml
// { "glProgramUniformMatrix2x4dv", 43, -1 },           // XXX: Add to xml
// { "glProgramUniformMatrix4x2dv", 43, -1 },           // XXX: Add to xml
// { "glProgramUniformMatrix3x4dv", 43, -1 },           // XXX: Add to xml
// { "glProgramUniformMatrix4x3dv", 43, -1 },           // XXX: Add to xml
   { "glValidateProgramPipeline", 43, -1 },
   { "glGetProgramPipelineInfoLog", 43, -1 },
// { "glVertexAttribL1d", 43, -1 },                     // XXX: Add to xml
// { "glVertexAttribL2d", 43, -1 },                     // XXX: Add to xml
// { "glVertexAttribL3d", 43, -1 },                     // XXX: Add to xml
// { "glVertexAttribL4d", 43, -1 },                     // XXX: Add to xml
// { "glVertexAttribL1dv", 43, -1 },                    // XXX: Add to xml
// { "glVertexAttribL2dv", 43, -1 },                    // XXX: Add to xml
// { "glVertexAttribL3dv", 43, -1 },                    // XXX: Add to xml
// { "glVertexAttribL4dv", 43, -1 },                    // XXX: Add to xml
// { "glVertexAttribLPointer", 43, -1 },                // XXX: Add to xml
// { "glGetVertexAttribLdv", 43, -1 },                  // XXX: Add to xml
   { "glViewportArrayv", 43, -1 },
   { "glViewportIndexedf", 43, -1 },
   { "glViewportIndexedfv", 43, -1 },
   { "glScissorArrayv", 43, -1 },
   { "glScissorIndexed", 43, -1 },
   { "glScissorIndexedv", 43, -1 },
   { "glDepthRangeArrayv", 43, -1 },
   { "glDepthRangeIndexed", 43, -1 },
   { "glGetFloati_v", 43, -1 },
   { "glGetDoublei_v", 43, -1 },
// { "glCreateSyncFromCLeventARB", 43, -1 },            // XXX: Add to xml
   { "glGetGraphicsResetStatusARB", 43, -1 },
   { "glGetnMapdvARB", 43, -1 },
   { "glGetnMapfvARB", 43, -1 },
   { "glGetnMapivARB", 43, -1 },
   { "glGetnPixelMapfvARB", 43, -1 },
   { "glGetnPixelMapuivARB", 43, -1 },
   { "glGetnPixelMapusvARB", 43, -1 },
   { "glGetnPolygonStippleARB", 43, -1 },
   { "glGetnColorTableARB", 43, -1 },
   { "glGetnConvolutionFilterARB", 43, -1 },
   { "glGetnSeparableFilterARB", 43, -1 },
   { "glGetnHistogramARB", 43, -1 },
   { "glGetnMinmaxARB", 43, -1 },
   { "glGetnTexImageARB", 43, -1 },
   { "glReadnPixelsARB", 43, -1 },
   { "glGetnCompressedTexImageARB", 43, -1 },
   { "glGetnUniformfvARB", 43, -1 },
   { "glGetnUniformivARB", 43, -1 },
   { "glGetnUniformuivARB", 43, -1 },
   { "glGetnUniformdvARB", 43, -1 },
   { "glDrawArraysInstancedBaseInstance", 43, -1 },
   { "glDrawElementsInstancedBaseInstance", 43, -1 },
   { "glDrawElementsInstancedBaseVertexBaseInstance", 43, -1 },
   { "glDrawTransformFeedbackInstanced", 43, -1 },
   { "glDrawTransformFeedbackStreamInstanced", 43, -1 },
// { "glGetInternalformativ", 43, -1 },                 // XXX: Add to xml
   { "glGetActiveAtomicCounterBufferiv", 43, -1 },
   { "glBindImageTexture", 43, -1 },
   { "glMemoryBarrier", 43, -1 },
   { "glTexStorage1D", 43, -1 },
   { "glTexStorage2D", 43, -1 },
   { "glTexStorage3D", 43, -1 },
   { "glTextureStorage1DEXT", 43, -1 },
   { "glTextureStorage2DEXT", 43, -1 },
   { "glTextureStorage3DEXT", 43, -1 },
   { "glClearBufferData", 43, -1 },
   { "glClearBufferSubData", 43, -1 },
// { "glClearNamedBufferDataEXT", 43, -1 },             // XXX: Add to xml
// { "glClearNamedBufferSubDataEXT", 43, -1 },          // XXX: Add to xml
   { "glDispatchCompute", 43, -1 },
   { "glDispatchComputeIndirect", 43, -1 },
// { "glCopyImageSubData", 43, -1 },                    // XXX: Add to xml
   { "glTextureView", 43, -1 },
   { "glBindVertexBuffer", 43, -1 },
   { "glVertexAttribFormat", 43, -1 },
   { "glVertexAttribIFormat", 43, -1 },
   { "glVertexAttribLFormat", 43, -1 },
   { "glVertexAttribBinding", 43, -1 },
   { "glVertexBindingDivisor", 43, -1 },
// { "glVertexArrayBindVertexBufferEXT", 43, -1 },      // XXX: Add to xml
// { "glVertexArrayVertexAttribFormatEXT", 43, -1 },    // XXX: Add to xml
// { "glVertexArrayVertexAttribIFormatEXT", 43, -1 },   // XXX: Add to xml
// { "glVertexArrayVertexAttribLFormatEXT", 43, -1 },   // XXX: Add to xml
// { "glVertexArrayVertexAttribBindingEXT", 43, -1 },   // XXX: Add to xml
// { "glVertexArrayVertexBindingDivisorEXT", 43, -1 },  // XXX: Add to xml
// { "glFramebufferParameteri", 43, -1 },               // XXX: Add to xml
// { "glGetFramebufferParameteriv", 43, -1 },           // XXX: Add to xml
// { "glNamedFramebufferParameteriEXT", 43, -1 },       // XXX: Add to xml
// { "glGetNamedFramebufferParameterivEXT", 43, -1 },   // XXX: Add to xml
// { "glGetInternalformati64v", 43, -1 },               // XXX: Add to xml
   { "glInvalidateTexSubImage", 43, -1 },
   { "glInvalidateTexImage", 43, -1 },
   { "glInvalidateBufferSubData", 43, -1 },
   { "glInvalidateBufferData", 43, -1 },
   { "glInvalidateFramebuffer", 43, -1 },
   { "glInvalidateSubFramebuffer", 43, -1 },
   { "glMultiDrawArraysIndirect", 43, -1 },
   { "glMultiDrawElementsIndirect", 43, -1 },
// { "glGetProgramInterfaceiv", 43, -1 },               // XXX: Add to xml
// { "glGetProgramResourceIndex", 43, -1 },             // XXX: Add to xml
// { "glGetProgramResourceName", 43, -1 },              // XXX: Add to xml
// { "glGetProgramResourceiv", 43, -1 },                // XXX: Add to xml
// { "glGetProgramResourceLocation", 43, -1 },          // XXX: Add to xml
// { "glGetProgramResourceLocationIndex", 43, -1 },     // XXX: Add to xml
// { "glShaderStorageBlockBinding", 43, -1 },           // XXX: Add to xml
   { "glTexBufferRange", 43, -1 },
// { "glTextureBufferRangeEXT", 43, -1 },               // XXX: Add to xml
   { "glTexStorage2DMultisample", 43, -1 },
   { "glTexStorage3DMultisample", 43, -1 },
// { "glTextureStorage2DMultisampleEXT", 43, -1 },      // XXX: Add to xml
// { "glTextureStorage3DMultisampleEXT", 43, -1 },      // XXX: Add to xml

   /* GL_ARB_internalformat_query */
   { "glGetInternalformativ", 30, -1 },

   /* GL_ARB_multi_bind */
   { "glBindBuffersBase", 44, -1 },
   { "glBindBuffersRange", 44, -1 },
   { "glBindTextures", 44, -1 },
   { "glBindSamplers", 44, -1 },
   { "glBindImageTextures", 44, -1 },
   { "glBindVertexBuffers", 44, -1 },

   /* GL_KHR_debug/GL_ARB_debug_output */
   { "glPushDebugGroup", 11, -1 },
   { "glPopDebugGroup", 11, -1 },
   { "glDebugMessageCallback", 11, -1 },
   { "glDebugMessageControl", 11, -1 },
   { "glDebugMessageInsert", 11, -1 },
   { "glGetDebugMessageLog", 11, -1 },
   { "glGetObjectLabel", 11, -1 },
   { "glGetObjectPtrLabel", 11, -1 },
   { "glObjectLabel", 11, -1 },
   { "glObjectPtrLabel", 11, -1 },
   /* aliased versions checked above */
   //{ "glDebugMessageControlARB", 11, -1 },
   //{ "glDebugMessageInsertARB", 11, -1 },
   //{ "glDebugMessageCallbackARB", 11, -1 },
   //{ "glGetDebugMessageLogARB", 11, -1 },

   /* GL_AMD_performance_monitor */
   { "glGetPerfMonitorGroupsAMD", 11, -1 },
   { "glGetPerfMonitorCountersAMD", 11, -1 },
   { "glGetPerfMonitorGroupStringAMD", 11, -1 },
   { "glGetPerfMonitorCounterStringAMD", 11, -1 },
   { "glGetPerfMonitorCounterInfoAMD", 11, -1 },
   { "glGenPerfMonitorsAMD", 11, -1 },
   { "glDeletePerfMonitorsAMD", 11, -1 },
   { "glSelectPerfMonitorCountersAMD", 11, -1 },
   { "glBeginPerfMonitorAMD", 11, -1 },
   { "glEndPerfMonitorAMD", 11, -1 },
   { "glGetPerfMonitorCounterDataAMD", 11, -1 },

   /* GL_INTEL_performance_query */
   { "glGetFirstPerfQueryIdINTEL", 30, -1 },
   { "glGetNextPerfQueryIdINTEL", 30, -1 },
   { "glGetPerfQueryIdByNameINTEL", 30, -1 },
   { "glGetPerfQueryInfoINTEL", 30, -1 },
   { "glGetPerfCounterInfoINTEL", 30, -1 },
   { "glCreatePerfQueryINTEL", 30, -1 },
   { "glDeletePerfQueryINTEL", 30, -1 },
   { "glBeginPerfQueryINTEL", 30, -1 },
   { "glEndPerfQueryINTEL", 30, -1 },
   { "glGetPerfQueryDataINTEL", 30, -1 },

   /* GL_NV_vdpau_interop */
   { "glVDPAUInitNV", 11, -1 },
   { "glVDPAUFiniNV", 11, -1 },
   { "glVDPAURegisterVideoSurfaceNV", 11, -1 },
   { "glVDPAURegisterOutputSurfaceNV", 11, -1 },
   { "glVDPAUIsSurfaceNV", 11, -1 },
   { "glVDPAUUnregisterSurfaceNV", 11, -1 },
   { "glVDPAUGetSurfaceivNV", 11, -1 },
   { "glVDPAUSurfaceAccessNV", 11, -1 },
   { "glVDPAUMapSurfacesNV", 11, -1 },
   { "glVDPAUUnmapSurfacesNV", 11, -1 },

   /* GL_ARB_buffer_storage */
   { "glBufferStorage", 43, -1 },

   { NULL, 0, -1 }
};

const struct function gles11_functions_possible[] = {
   { "glActiveTexture", 11, _gloffset_ActiveTexture },
   { "glAlphaFunc", 11, _gloffset_AlphaFunc },
   { "glAlphaFuncx", 11, -1 },
   { "glBindBuffer", 11, -1 },
   { "glBindFramebufferOES", 11, -1 },
   { "glBindRenderbufferOES", 11, -1 },
   { "glBindTexture", 11, _gloffset_BindTexture },
   { "glBlendEquationOES", 11, _gloffset_BlendEquation },
   { "glBlendEquationSeparateOES", 11, -1 },
   { "glBlendFunc", 11, _gloffset_BlendFunc },
   { "glBlendFuncSeparateOES", 11, -1 },
   { "glBufferData", 11, -1 },
   { "glBufferSubData", 11, -1 },
   { "glCheckFramebufferStatusOES", 11, -1 },
   { "glClear", 11, _gloffset_Clear },
   { "glClearColor", 11, _gloffset_ClearColor },
   { "glClearColorx", 11, -1 },
   { "glClearDepthf", 11, -1 },
   { "glClearDepthx", 11, -1 },
   { "glClearStencil", 11, _gloffset_ClearStencil },
   { "glClientActiveTexture", 11, _gloffset_ClientActiveTexture },
   { "glClipPlanef", 11, -1 },
   { "glClipPlanex", 11, -1 },
   { "glColor4f", 11, _gloffset_Color4f },
   { "glColor4ub", 11, _gloffset_Color4ub },
   { "glColor4x", 11, -1 },
   { "glColorMask", 11, _gloffset_ColorMask },
   { "glColorPointer", 11, _gloffset_ColorPointer },
   { "glCompressedTexImage2D", 11, -1 },
   { "glCompressedTexSubImage2D", 11, -1 },
   { "glCopyTexImage2D", 11, _gloffset_CopyTexImage2D },
   { "glCopyTexSubImage2D", 11, _gloffset_CopyTexSubImage2D },
   { "glCullFace", 11, _gloffset_CullFace },
   { "glDeleteBuffers", 11, -1 },
   { "glDeleteFramebuffersOES", 11, -1 },
   { "glDeleteRenderbuffersOES", 11, -1 },
   { "glDeleteTextures", 11, _gloffset_DeleteTextures },
   { "glDepthFunc", 11, _gloffset_DepthFunc },
   { "glDepthMask", 11, _gloffset_DepthMask },
   { "glDepthRangef", 11, -1 },
   { "glDepthRangex", 11, -1 },
   { "glDisable", 11, _gloffset_Disable },
   { "glDiscardFramebufferEXT", 11, -1 },
   { "glDisableClientState", 11, _gloffset_DisableClientState },
   { "glDrawArrays", 11, _gloffset_DrawArrays },
   { "glDrawElements", 11, _gloffset_DrawElements },
   { "glDrawTexfOES", 11, -1 },
   { "glDrawTexfvOES", 11, -1 },
   { "glDrawTexiOES", 11, -1 },
   { "glDrawTexivOES", 11, -1 },
   { "glDrawTexsOES", 11, -1 },
   { "glDrawTexsvOES", 11, -1 },
   { "glDrawTexxOES", 11, -1 },
   { "glDrawTexxvOES", 11, -1 },
   { "glEGLImageTargetRenderbufferStorageOES", 11, -1 },
   { "glEGLImageTargetTexture2DOES", 11, -1 },
   { "glEnable", 11, _gloffset_Enable },
   { "glEnableClientState", 11, _gloffset_EnableClientState },
   { "glFinish", 11, _gloffset_Finish },
   { "glFlush", 11, _gloffset_Flush },
   { "glFlushMappedBufferRangeEXT", 11, -1 },
   { "glFogf", 11, _gloffset_Fogf },
   { "glFogfv", 11, _gloffset_Fogfv },
   { "glFogx", 11, -1 },
   { "glFogxv", 11, -1 },
   { "glFramebufferRenderbufferOES", 11, -1 },
   { "glFramebufferTexture2DOES", 11, -1 },
   { "glFrontFace", 11, _gloffset_FrontFace },
   { "glFrustumf", 11, -1 },
   { "glFrustumx", 11, -1 },
   { "glGenBuffers", 11, -1 },
   { "glGenFramebuffersOES", 11, -1 },
   { "glGenRenderbuffersOES", 11, -1 },
   { "glGenTextures", 11, _gloffset_GenTextures },
   { "glGenerateMipmapOES", 11, -1 },
   { "glGetBooleanv", 11, _gloffset_GetBooleanv },
   { "glGetBufferParameteriv", 11, -1 },
   { "glGetBufferPointervOES", 11, -1 },
   { "glGetClipPlanef", 11, -1 },
   { "glGetClipPlanex", 11, -1 },
   { "glGetError", 11, _gloffset_GetError },
   { "glGetFixedv", 11, -1 },
   { "glGetFloatv", 11, _gloffset_GetFloatv },
   { "glGetFramebufferAttachmentParameterivOES", 11, -1 },
   { "glGetIntegerv", 11, _gloffset_GetIntegerv },
   { "glGetLightfv", 11, _gloffset_GetLightfv },
   { "glGetLightxv", 11, -1 },
   { "glGetMaterialfv", 11, _gloffset_GetMaterialfv },
   { "glGetMaterialxv", 11, -1 },
   { "glGetPointerv", 11, _gloffset_GetPointerv },
   { "glGetRenderbufferParameterivOES", 11, -1 },
   { "glGetString", 11, _gloffset_GetString },
   { "glGetTexEnvfv", 11, _gloffset_GetTexEnvfv },
   { "glGetTexEnviv", 11, _gloffset_GetTexEnviv },
   { "glGetTexEnvxv", 11, -1 },
   { "glGetTexGenfvOES", 11, _gloffset_GetTexGenfv },
   { "glGetTexGenivOES", 11, _gloffset_GetTexGeniv },
   { "glGetTexGenxvOES", 11, -1 },
   { "glGetTexParameterfv", 11, _gloffset_GetTexParameterfv },
   { "glGetTexParameteriv", 11, _gloffset_GetTexParameteriv },
   { "glGetTexParameterxv", 11, -1 },
   { "glHint", 11, _gloffset_Hint },
   { "glIsBuffer", 11, -1 },
   { "glIsEnabled", 11, _gloffset_IsEnabled },
   { "glIsFramebufferOES", 11, -1 },
   { "glIsRenderbufferOES", 11, -1 },
   { "glIsTexture", 11, _gloffset_IsTexture },
   { "glLightModelf", 11, _gloffset_LightModelf },
   { "glLightModelfv", 11, _gloffset_LightModelfv },
   { "glLightModelx", 11, -1 },
   { "glLightModelxv", 11, -1 },
   { "glLightf", 11, _gloffset_Lightf },
   { "glLightfv", 11, _gloffset_Lightfv },
   { "glLightx", 11, -1 },
   { "glLightxv", 11, -1 },
   { "glLineWidth", 11, _gloffset_LineWidth },
   { "glLineWidthx", 11, -1 },
   { "glLoadIdentity", 11, _gloffset_LoadIdentity },
   { "glLoadMatrixf", 11, _gloffset_LoadMatrixf },
   { "glLoadMatrixx", 11, -1 },
   { "glLogicOp", 11, _gloffset_LogicOp },
   { "glMapBufferOES", 11, -1 },
   { "glMapBufferRangeEXT", 11, -1 },
   { "glMaterialf", 11, _gloffset_Materialf },
   { "glMaterialfv", 11, _gloffset_Materialfv },
   { "glMaterialx", 11, -1 },
   { "glMaterialxv", 11, -1 },
   { "glMatrixMode", 11, _gloffset_MatrixMode },
   { "glMultMatrixf", 11, _gloffset_MultMatrixf },
   { "glMultMatrixx", 11, -1 },
   { "glMultiDrawArraysEXT", 11, -1 },
   { "glMultiDrawElementsEXT", 11, -1 },
   { "glMultiTexCoord4f", 11, _gloffset_MultiTexCoord4fARB },
   { "glMultiTexCoord4x", 11, -1 },
   { "glNormal3f", 11, _gloffset_Normal3f },
   { "glNormal3x", 11, -1 },
   { "glNormalPointer", 11, _gloffset_NormalPointer },
   { "glOrthof", 11, -1 },
   { "glOrthox", 11, -1 },
   { "glPixelStorei", 11, _gloffset_PixelStorei },
   { "glPointParameterf", 11, -1 },
   { "glPointParameterfv", 11, -1 },
   { "glPointParameterx", 11, -1 },
   { "glPointParameterxv", 11, -1 },
   { "glPointSize", 11, _gloffset_PointSize },
   { "glPointSizePointerOES", 11, -1 },
   { "glPointSizex", 11, -1 },
   { "glPolygonOffset", 11, _gloffset_PolygonOffset },
   { "glPolygonOffsetx", 11, -1 },
   { "glPopMatrix", 11, _gloffset_PopMatrix },
   { "glPushMatrix", 11, _gloffset_PushMatrix },
   { "glQueryMatrixxOES", 11, -1 },
   { "glReadPixels", 11, _gloffset_ReadPixels },
   { "glRenderbufferStorageOES", 11, -1 },
   { "glRotatef", 11, _gloffset_Rotatef },
   { "glRotatex", 11, -1 },
   { "glSampleCoverage", 11, -1 },
   { "glSampleCoveragex", 11, -1 },
   { "glScalef", 11, _gloffset_Scalef },
   { "glScalex", 11, -1 },
   { "glScissor", 11, _gloffset_Scissor },
   { "glShadeModel", 11, _gloffset_ShadeModel },
   { "glStencilFunc", 11, _gloffset_StencilFunc },
   { "glStencilMask", 11, _gloffset_StencilMask },
   { "glStencilOp", 11, _gloffset_StencilOp },
   { "glTexCoordPointer", 11, _gloffset_TexCoordPointer },
   { "glTexEnvf", 11, _gloffset_TexEnvf },
   { "glTexEnvfv", 11, _gloffset_TexEnvfv },
   { "glTexEnvi", 11, _gloffset_TexEnvi },
   { "glTexEnviv", 11, _gloffset_TexEnviv },
   { "glTexEnvx", 11, -1 },
   { "glTexEnvxv", 11, -1 },
   { "glTexGenfOES", 11, _gloffset_TexGenf },
   { "glTexGenfvOES", 11, _gloffset_TexGenfv },
   { "glTexGeniOES", 11, _gloffset_TexGeni },
   { "glTexGenivOES", 11, _gloffset_TexGeniv },
   { "glTexGenxOES", 11, -1 },
   { "glTexGenxvOES", 11, -1 },
   { "glTexImage2D", 11, _gloffset_TexImage2D },
   { "glTexParameterf", 11, _gloffset_TexParameterf },
   { "glTexParameterfv", 11, _gloffset_TexParameterfv },
   { "glTexParameteri", 11, _gloffset_TexParameteri },
   { "glTexParameteriv", 11, _gloffset_TexParameteriv },
   { "glTexParameterx", 11, -1 },
   { "glTexParameterxv", 11, -1 },
   { "glTexSubImage2D", 11, _gloffset_TexSubImage2D },
   { "glTranslatef", 11, _gloffset_Translatef },
   { "glTranslatex", 11, -1 },
   { "glUnmapBufferOES", 11, -1 },
   { "glVertexPointer", 11, _gloffset_VertexPointer },
   { "glViewport", 11, _gloffset_Viewport },
   { NULL, 0, -1 }
};

const struct function gles2_functions_possible[] = {
   { "glActiveTexture", 20, _gloffset_ActiveTexture },
   { "glAttachShader", 20, -1 },
   { "glBindAttribLocation", 20, -1 },
   { "glBindBuffer", 20, -1 },
   { "glBindFramebuffer", 20, -1 },
   { "glBindRenderbuffer", 20, -1 },
   { "glBindTexture", 20, _gloffset_BindTexture },
   { "glBindVertexArrayOES", 20, -1 },
   { "glBlendColor", 20, _gloffset_BlendColor },
   { "glBlendEquation", 20, _gloffset_BlendEquation },
   { "glBlendEquationSeparate", 20, -1 },
   { "glBlendFunc", 20, _gloffset_BlendFunc },
   { "glBlendFuncSeparate", 20, -1 },
   { "glBufferData", 20, -1 },
   { "glBufferSubData", 20, -1 },
   { "glCheckFramebufferStatus", 20, -1 },
   { "glClear", 20, _gloffset_Clear },
   { "glClearColor", 20, _gloffset_ClearColor },
   { "glClearDepthf", 20, -1 },
   { "glClearStencil", 20, _gloffset_ClearStencil },
   { "glColorMask", 20, _gloffset_ColorMask },
   { "glCompileShader", 20, -1 },
   { "glCompressedTexImage2D", 20, -1 },
   { "glCompressedTexImage3DOES", 20, -1 },
   { "glCompressedTexSubImage2D", 20, -1 },
   { "glCompressedTexSubImage3DOES", 20, -1 },
   { "glCopyTexImage2D", 20, _gloffset_CopyTexImage2D },
   { "glCopyTexSubImage2D", 20, _gloffset_CopyTexSubImage2D },
   { "glCopyTexSubImage3DOES", 20, _gloffset_CopyTexSubImage3D },
   { "glCreateProgram", 20, -1 },
   { "glCreateShader", 20, -1 },
   { "glCullFace", 20, _gloffset_CullFace },
   { "glDeleteBuffers", 20, -1 },
   { "glDeleteFramebuffers", 20, -1 },
   { "glDeleteProgram", 20, -1 },
   { "glDeleteRenderbuffers", 20, -1 },
   { "glDeleteShader", 20, -1 },
   { "glDeleteTextures", 20, _gloffset_DeleteTextures },
   { "glDeleteVertexArraysOES", 20, -1 },
   { "glDepthFunc", 20, _gloffset_DepthFunc },
   { "glDepthMask", 20, _gloffset_DepthMask },
   { "glDepthRangef", 20, -1 },
   { "glDetachShader", 20, -1 },
   { "glDisable", 20, _gloffset_Disable },
   { "glDiscardFramebufferEXT", 20, -1 },
   { "glDisableVertexAttribArray", 20, -1 },
   { "glDrawArrays", 20, _gloffset_DrawArrays },
   { "glDrawBuffersNV", 20, -1 },
   { "glDrawElements", 20, _gloffset_DrawElements },
   { "glEGLImageTargetRenderbufferStorageOES", 20, -1 },
   { "glEGLImageTargetTexture2DOES", 20, -1 },
   { "glEnable", 20, _gloffset_Enable },
   { "glEnableVertexAttribArray", 20, -1 },
   { "glFinish", 20, _gloffset_Finish },
   { "glFlush", 20, _gloffset_Flush },
   { "glFlushMappedBufferRangeEXT", 20, -1 },
   { "glFramebufferRenderbuffer", 20, -1 },
   { "glFramebufferTexture2D", 20, -1 },
   { "glFramebufferTexture3DOES", 20, -1 },
   { "glFrontFace", 20, _gloffset_FrontFace },
   { "glGenBuffers", 20, -1 },
   { "glGenFramebuffers", 20, -1 },
   { "glGenRenderbuffers", 20, -1 },
   { "glGenTextures", 20, _gloffset_GenTextures },
   { "glGenVertexArraysOES", 20, -1 },
   { "glGenerateMipmap", 20, -1 },
   { "glGetActiveAttrib", 20, -1 },
   { "glGetActiveUniform", 20, -1 },
   { "glGetAttachedShaders", 20, -1 },
   { "glGetAttribLocation", 20, -1 },
   { "glGetBooleanv", 20, _gloffset_GetBooleanv },
   { "glGetBufferParameteriv", 20, -1 },
   { "glGetBufferPointervOES", 20, -1 },
   { "glGetError", 20, _gloffset_GetError },
   { "glGetFloatv", 20, _gloffset_GetFloatv },
   { "glGetFramebufferAttachmentParameteriv", 20, -1 },
   { "glGetIntegerv", 20, _gloffset_GetIntegerv },
   { "glGetProgramInfoLog", 20, -1 },
   { "glGetProgramiv", 20, -1 },
   { "glGetRenderbufferParameteriv", 20, -1 },
   { "glGetShaderInfoLog", 20, -1 },
   { "glGetShaderPrecisionFormat", 20, -1 },
   { "glGetShaderSource", 20, -1 },
   { "glGetShaderiv", 20, -1 },
   { "glGetString", 20, _gloffset_GetString },
   { "glGetTexParameterfv", 20, _gloffset_GetTexParameterfv },
   { "glGetTexParameteriv", 20, _gloffset_GetTexParameteriv },
   { "glGetUniformLocation", 20, -1 },
   { "glGetUniformfv", 20, -1 },
   { "glGetUniformiv", 20, -1 },
   { "glGetVertexAttribPointerv", 20, -1 },
   { "glGetVertexAttribfv", 20, -1 },
   { "glGetVertexAttribiv", 20, -1 },
   { "glHint", 20, _gloffset_Hint },
   { "glIsBuffer", 20, -1 },
   { "glIsEnabled", 20, _gloffset_IsEnabled },
   { "glIsFramebuffer", 20, -1 },
   { "glIsProgram", 20, -1 },
   { "glIsRenderbuffer", 20, -1 },
   { "glIsShader", 20, -1 },
   { "glIsTexture", 20, _gloffset_IsTexture },
   { "glIsVertexArrayOES", 20, -1 },
   { "glLineWidth", 20, _gloffset_LineWidth },
   { "glLinkProgram", 20, -1 },
   { "glMapBufferOES", 20, -1 },
   { "glMapBufferRangeEXT", 20, -1 },
   { "glMultiDrawArraysEXT", 20, -1 },
   { "glMultiDrawElementsEXT", 20, -1 },
   { "glPixelStorei", 20, _gloffset_PixelStorei },
   { "glPolygonOffset", 20, _gloffset_PolygonOffset },
   { "glReadBufferNV", 20, _gloffset_ReadBuffer },
   { "glReadPixels", 20, _gloffset_ReadPixels },
   { "glReleaseShaderCompiler", 20, -1 },
   { "glRenderbufferStorage", 20, -1 },
   { "glSampleCoverage", 20, -1 },
   { "glScissor", 20, _gloffset_Scissor },
   { "glShaderBinary", 20, -1 },
   { "glShaderSource", 20, -1 },
   { "glStencilFunc", 20, _gloffset_StencilFunc },
   { "glStencilFuncSeparate", 20, -1 },
   { "glStencilMask", 20, _gloffset_StencilMask },
   { "glStencilMaskSeparate", 20, -1 },
   { "glStencilOp", 20, _gloffset_StencilOp },
   { "glStencilOpSeparate", 20, -1 },
   { "glTexImage2D", 20, _gloffset_TexImage2D },
   { "glTexImage3DOES", 20, _gloffset_TexImage3D },
   { "glTexParameterf", 20, _gloffset_TexParameterf },
   { "glTexParameterfv", 20, _gloffset_TexParameterfv },
   { "glTexParameteri", 20, _gloffset_TexParameteri },
   { "glTexParameteriv", 20, _gloffset_TexParameteriv },
   { "glTexSubImage2D", 20, _gloffset_TexSubImage2D },
   { "glTexSubImage3DOES", 20, _gloffset_TexSubImage3D },
   { "glUniform1f", 20, -1 },
   { "glUniform1fv", 20, -1 },
   { "glUniform1i", 20, -1 },
   { "glUniform1iv", 20, -1 },
   { "glUniform2f", 20, -1 },
   { "glUniform2fv", 20, -1 },
   { "glUniform2i", 20, -1 },
   { "glUniform2iv", 20, -1 },
   { "glUniform3f", 20, -1 },
   { "glUniform3fv", 20, -1 },
   { "glUniform3i", 20, -1 },
   { "glUniform3iv", 20, -1 },
   { "glUniform4f", 20, -1 },
   { "glUniform4fv", 20, -1 },
   { "glUniform4i", 20, -1 },
   { "glUniform4iv", 20, -1 },
   { "glUniformMatrix2fv", 20, -1 },
   { "glUniformMatrix3fv", 20, -1 },
   { "glUniformMatrix4fv", 20, -1 },
   { "glUnmapBufferOES", 20, -1 },
   { "glUseProgram", 20, -1 },
   { "glValidateProgram", 20, -1 },
   { "glVertexAttrib1f", 20, -1 },
   { "glVertexAttrib1fv", 20, -1 },
   { "glVertexAttrib2f", 20, -1 },
   { "glVertexAttrib2fv", 20, -1 },
   { "glVertexAttrib3f", 20, -1 },
   { "glVertexAttrib3fv", 20, -1 },
   { "glVertexAttrib4f", 20, -1 },
   { "glVertexAttrib4fv", 20, -1 },
   { "glVertexAttribPointer", 20, -1 },
   { "glViewport", 20, _gloffset_Viewport },

   /* GL_OES_get_program_binary - Also part of OpenGL ES 3.0. */
   { "glGetProgramBinaryOES", 20, -1 },
   { "glProgramBinaryOES", 20, -1 },

   /* GL_EXT_separate_shader_objects - Also part of OpenGL ES 3.1. */
   { "glProgramParameteriEXT", 20, -1 },
   { "glUseProgramStagesEXT", 20, -1 },
   { "glActiveShaderProgramEXT", 20, -1 },
   { "glCreateShaderProgramvEXT", 20, -1 },
   { "glBindProgramPipelineEXT", 20, -1 },
   { "glDeleteProgramPipelinesEXT", 20, -1 },
   { "glGenProgramPipelinesEXT", 20, -1 },
   { "glIsProgramPipelineEXT", 20, -1 },
   { "glGetProgramPipelineivEXT", 20, -1 },
   { "glProgramUniform1iEXT", 20, -1 },
   { "glProgramUniform1ivEXT", 20, -1 },
   { "glProgramUniform1fEXT", 20, -1 },
   { "glProgramUniform1fvEXT", 20, -1 },
   { "glProgramUniform2iEXT", 20, -1 },
   { "glProgramUniform2ivEXT", 20, -1 },
   { "glProgramUniform2fEXT", 20, -1 },
   { "glProgramUniform2fvEXT", 20, -1 },
   { "glProgramUniform3iEXT", 20, -1 },
   { "glProgramUniform3ivEXT", 20, -1 },
   { "glProgramUniform3fEXT", 20, -1 },
   { "glProgramUniform3fvEXT", 20, -1 },
   { "glProgramUniform4iEXT", 20, -1 },
   { "glProgramUniform4ivEXT", 20, -1 },
   { "glProgramUniform4fEXT", 20, -1 },
   { "glProgramUniform4fvEXT", 20, -1 },
   { "glProgramUniformMatrix2fvEXT", 20, -1 },
   { "glProgramUniformMatrix3fvEXT", 20, -1 },
   { "glProgramUniformMatrix4fvEXT", 20, -1 },
   { "glProgramUniformMatrix2x3fvEXT", 20, -1 },
   { "glProgramUniformMatrix3x2fvEXT", 20, -1 },
   { "glProgramUniformMatrix2x4fvEXT", 20, -1 },
   { "glProgramUniformMatrix4x2fvEXT", 20, -1 },
   { "glProgramUniformMatrix3x4fvEXT", 20, -1 },
   { "glProgramUniformMatrix4x3fvEXT", 20, -1 },
   { "glValidateProgramPipelineEXT", 20, -1 },
   { "glGetProgramPipelineInfoLogEXT", 20, -1 },

   /* GL_INTEL_performance_query */
   { "glGetFirstPerfQueryIdINTEL", 20, -1 },
   { "glGetNextPerfQueryIdINTEL", 20, -1 },
   { "glGetPerfQueryIdByNameINTEL", 20, -1 },
   { "glGetPerfQueryInfoINTEL", 20, -1 },
   { "glGetPerfCounterInfoINTEL", 20, -1 },
   { "glCreatePerfQueryINTEL", 20, -1 },
   { "glDeletePerfQueryINTEL", 20, -1 },
   { "glBeginPerfQueryINTEL", 20, -1 },
   { "glEndPerfQueryINTEL", 20, -1 },
   { "glGetPerfQueryDataINTEL", 20, -1 },

   { NULL, 0, -1 }
};

const struct function gles3_functions_possible[] = {
   { "glBeginQuery", 30, -1 },
   { "glBeginTransformFeedback", 30, -1 },
   { "glBindBufferBase", 30, -1 },
   { "glBindBufferRange", 30, -1 },
   { "glBindSampler", 30, -1 },
   { "glBindTransformFeedback", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glBindVertexArray", 30, -1 },
   { "glBlitFramebuffer", 30, -1 },
   { "glClearBufferfi", 30, -1 },
   { "glClearBufferfv", 30, -1 },
   { "glClearBufferiv", 30, -1 },
   { "glClearBufferuiv", 30, -1 },
   { "glClientWaitSync", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCompressedTexImage3D", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCompressedTexSubImage3D", 30, -1 },
   { "glCopyBufferSubData", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCopyTexSubImage3D", 30, -1 },
   { "glDeleteQueries", 30, -1 },
   { "glDeleteSamplers", 30, -1 },
   { "glDeleteSync", 30, -1 },
   { "glDeleteTransformFeedbacks", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glDeleteVertexArrays", 30, -1 },
   { "glDrawArraysInstanced", 30, -1 },
   // We check for the aliased -NV version in GLES 2
   // { "glDrawBuffers", 30, -1 },
   { "glDrawElementsInstanced", 30, -1 },
   { "glDrawRangeElements", 30, -1 },
   { "glEndQuery", 30, -1 },
   { "glEndTransformFeedback", 30, -1 },
   { "glFenceSync", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glFlushMappedBufferRange", 30, -1 },
   { "glFramebufferTextureLayer", 30, -1 },
   { "glGenQueries", 30, -1 },
   { "glGenSamplers", 30, -1 },
   { "glGenTransformFeedbacks", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glGenVertexArrays", 30, -1 },
   { "glGetActiveUniformBlockiv", 30, -1 },
   { "glGetActiveUniformBlockName", 30, -1 },
   { "glGetActiveUniformsiv", 30, -1 },
   { "glGetBufferParameteri64v", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glGetBufferPointerv", 30, -1 },
   { "glGetFragDataLocation", 30, -1 },
   { "glGetInteger64i_v", 30, -1 },
   { "glGetInteger64v", 30, -1 },
   { "glGetIntegeri_v", 30, -1 },
   { "glGetInternalformativ", 30, -1 },
   // glGetProgramBinary aliases glGetProgramBinaryOES in GLES 2
   { "glGetQueryiv", 30, -1 },
   { "glGetQueryObjectuiv", 30, -1 },
   { "glGetSamplerParameterfv", 30, -1 },
   { "glGetSamplerParameteriv", 30, -1 },
   { "glGetStringi", 30, -1 },
   { "glGetSynciv", 30, -1 },
   { "glGetTransformFeedbackVarying", 30, -1 },
   { "glGetUniformBlockIndex", 30, -1 },
   { "glGetUniformIndices", 30, -1 },
   { "glGetUniformuiv", 30, -1 },
   { "glGetVertexAttribIiv", 30, -1 },
   { "glGetVertexAttribIuiv", 30, -1 },
   { "glInvalidateFramebuffer", 30, -1 },
   { "glInvalidateSubFramebuffer", 30, -1 },
   { "glIsQuery", 30, -1 },
   { "glIsSampler", 30, -1 },
   { "glIsSync", 30, -1 },
   { "glIsTransformFeedback", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glIsVertexArray", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glMapBufferRange", 30, -1 },
   { "glPauseTransformFeedback", 30, -1 },
   // glProgramBinary aliases glProgramBinaryOES in GLES 2
   // glProgramParameteri aliases glProgramParameteriEXT in GLES 2
   // We check for the aliased -NV version in GLES 2
   // { "glReadBuffer", 30, -1 },
   { "glRenderbufferStorageMultisample", 30, -1 },
   { "glResumeTransformFeedback", 30, -1 },
   { "glSamplerParameterf", 30, -1 },
   { "glSamplerParameterfv", 30, -1 },
   { "glSamplerParameteri", 30, -1 },
   { "glSamplerParameteriv", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glTexImage3D", 30, -1 },
   { "glTexStorage2D", 30, -1 },
   { "glTexStorage3D", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glTexSubImage3D", 30, -1 },
   { "glTransformFeedbackVaryings", 30, -1 },
   { "glUniform1ui", 30, -1 },
   { "glUniform1uiv", 30, -1 },
   { "glUniform2ui", 30, -1 },
   { "glUniform2uiv", 30, -1 },
   { "glUniform3ui", 30, -1 },
   { "glUniform3uiv", 30, -1 },
   { "glUniform4ui", 30, -1 },
   { "glUniform4uiv", 30, -1 },
   { "glUniformBlockBinding", 30, -1 },
   { "glUniformMatrix2x3fv", 30, -1 },
   { "glUniformMatrix2x4fv", 30, -1 },
   { "glUniformMatrix3x2fv", 30, -1 },
   { "glUniformMatrix3x4fv", 30, -1 },
   { "glUniformMatrix4x2fv", 30, -1 },
   { "glUniformMatrix4x3fv", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glUnmapBuffer", 30, -1 },
   { "glVertexAttribDivisor", 30, -1 },
   { "glVertexAttribI4i", 30, -1 },
   { "glVertexAttribI4iv", 30, -1 },
   { "glVertexAttribI4ui", 30, -1 },
   { "glVertexAttribI4uiv", 30, -1 },
   { "glVertexAttribIPointer", 30, -1 },
   { "glWaitSync", 30, -1 },

   /* GL_EXT_separate_shader_objects - Also part of OpenGL ES 3.1. */
   { "glProgramUniform1uiEXT", 30, -1 },
   { "glProgramUniform1uivEXT", 30, -1 },
   { "glProgramUniform2uiEXT", 30, -1 },
   { "glProgramUniform2uivEXT", 30, -1 },
   { "glProgramUniform3uiEXT", 30, -1 },
   { "glProgramUniform3uivEXT", 30, -1 },
   { "glProgramUniform4uiEXT", 30, -1 },
   { "glProgramUniform4uivEXT", 30, -1 },

   { NULL, 0, -1 }
};
