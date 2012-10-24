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
   unsigned int Version;
   int offset;
};

extern const struct function gles2_functions_possible[];
extern const struct function gles3_functions_possible[];

#if FEATURE_ES1
extern const struct function gles11_functions_possible[];
#endif /* FEATURE_ES1 */

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

#if FEATURE_ES1
TEST_F(DispatchSanity_test, GLES11)
{
   ctx.Version = 11;
   _mesa_initialize_context(&ctx,
                            API_OPENGLES,
                            &visual,
                            NULL /* share_list */,
                            &driver_functions);

   _swrast_CreateContext(&ctx);
   _vbo_CreateContext(&ctx);
   _tnl_CreateContext(&ctx);
   _swsetup_CreateContext(&ctx);

   validate_functions(&ctx, gles11_functions_possible);
   validate_nops(&ctx);
}
#endif /* FEATURE_ES1 */

TEST_F(DispatchSanity_test, GLES2)
{
   ctx.Version = 20;
   _mesa_initialize_context(&ctx,
                            API_OPENGLES2, //api,
                            &visual,
                            NULL, //&share_list,
                            &driver_functions);

   _swrast_CreateContext(&ctx);
   _vbo_CreateContext(&ctx);
   _tnl_CreateContext(&ctx);
   _swsetup_CreateContext(&ctx);

   validate_functions(&ctx, gles2_functions_possible);
   validate_nops(&ctx);
}

TEST_F(DispatchSanity_test, GLES3)
{
   ctx.Version = 30;
   _mesa_initialize_context(&ctx,
                            API_OPENGLES2, //api,
                            &visual,
                            NULL, //&share_list,
                            &driver_functions);

   _swrast_CreateContext(&ctx);
   _vbo_CreateContext(&ctx);
   _tnl_CreateContext(&ctx);
   _swsetup_CreateContext(&ctx);

   validate_functions(&ctx, gles2_functions_possible);
   validate_functions(&ctx, gles3_functions_possible);
   validate_nops(&ctx);
}

#if FEATURE_ES1
const struct function gles11_functions_possible[] = {
   { "glActiveTexture", 11, _gloffset_ActiveTextureARB },
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
   { "glClientActiveTexture", 11, _gloffset_ClientActiveTextureARB },
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
#endif /* FEATURE_ES1 */

const struct function gles2_functions_possible[] = {
   { "glActiveTexture", 20, _gloffset_ActiveTextureARB },
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
   // We have an implementation (added Jan 1 2010, 1fbc7193) but never tested...
   // { "glGetBufferParameteri64v", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glGetBufferPointerv", 30, -1 },
   { "glGetFragDataLocation", 30, -1 },
   /// XXX: Missing implementation of glGetInteger64i_v
   // { "glGetInteger64i_v", 30, -1 },
   { "glGetInteger64v", 30, -1 },
   { "glGetIntegeri_v", 30, -1 },
   // XXX: Missing implementation of ARB_internalformat_query
   // { "glGetInternalformativ", 30, -1 },
   // XXX: Missing implementation of ARB_get_program_binary
   /// { "glGetProgramBinary", 30, -1 },
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
   // XXX: Missing implementation of ARB_get_program_binary
   // { "glProgramBinary", 30, -1 },
   // XXX: Missing implementation of ARB_get_program_binary
   // { "glProgramParameteri", 30, -1 },
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
   { NULL, 0, -1 }
};
