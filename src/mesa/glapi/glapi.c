/* $Id: glapi.c,v 1.9 1999/11/25 18:17:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 */



#include <assert.h>
#include <stdlib.h>  /* to get NULL */
#include <string.h>
#include "glapi.h"
#include "glapinoop.h"



/*
 * Define the DISPATCH_SETUP and DISPATCH macros here dependant on
 * whether or not threading is enabled.
 */
#if defined(THREADS)

/*** Thread-safe API dispatch ***/
#include "mthreads.h"
static MesaTSD mesa_dispatch_tsd;
static void mesa_dispatch_thread_init() {
  MesaInitTSD(&mesa_dispatch_tsd);
}
#define DISPATCH_SETUP struct _glapi_table *dispatch = (struct _glapi_table *) MesaGetTSD(&mesa_dispatch_tsd)
#define DISPATCH(FUNC, ARGS) (dispatch->FUNC) ARGS


#else

/*** Non-threaded API dispatch ***/
static struct _glapi_table *Dispatch = &__glapi_noop_table;
#define DISPATCH_SETUP
#define DISPATCH(FUNC, ARGS) (Dispatch->FUNC) ARGS

#endif



/*
 * Set the global or per-thread dispatch table pointer.
 */
void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
   if (!dispatch) {
      /* use the no-op functions */
      dispatch = &__glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(THREADS)
   MesaSetTSD(&mesa_dispatch_tsd, (void*) dispatch, mesa_dispatch_thread_init);
#else
   Dispatch = dispatch;
#endif
}


/*
 * Get the global or per-thread dispatch table pointer.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(THREADS)
   /* return this thread's dispatch pointer */
   return (struct _glapi_table *) MesaGetTSD(&mesa_dispatch_tsd);
#else
   return Dispatch;
#endif
}


/*
 * Get API dispatcher version string.
 * XXX this isn't well defined yet.
 */
const char *
_glapi_get_version(void)
{
   return "1.2";
}


/*
 * Return list of hard-coded extension entrypoints in the dispatch table.
 * XXX this isn't well defined yet.
 */
const char *
_glapi_get_extensions(void)
{
   return "GL_EXT_paletted_texture GL_EXT_compiled_vertex_array GL_EXT_point_parameters GL_EXT_polygon_offset GL_EXT_blend_minmax GL_EXT_blend_color GL_ARB_multitexture GL_INGR_blend_func_separate GL_MESA_window_pos GL_MESA_resize_buffers";
}



/*
 * XXX the following dynamic extension code is just a prototype and
 * not used yet.
 */
struct _glapi_ext_entrypoint {
   const char *Name;
   GLuint Offset;
};

#define MAX_EXT_ENTRYPOINTS 100

static struct _glapi_ext_entrypoint ExtEntryTable[MAX_EXT_ENTRYPOINTS];
static GLuint NumExtEntryPoints;



/*
 * Dynamically allocate an extension entry point.
 * Return a slot number or -1 if table is full.
 */
GLint
_glapi_alloc_entrypoint(const char *funcName)
{
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         /* found it */
         return (GLint) ExtEntryTable[i].Offset;
      }
   }
   if (NumExtEntryPoints < MAX_EXT_ENTRYPOINTS) {
      /* add it */
      ExtEntryTable[NumExtEntryPoints].Name = strdup(funcName);
      ExtEntryTable[NumExtEntryPoints].Offset = NumExtEntryPoints; /* OK? */
      NumExtEntryPoints++;
      return (GLint) (NumExtEntryPoints - 1);
   }
   /* no more room in the table */
   return -1;
}



/*
 * Find the dynamic entry point for the named function.
 * Return a slot number or -1 if not found.
 */
GLint
_glapi_get_entrypoint(const char *funcName)
{
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         /* found it */
         return (GLint) ExtEntryTable[i].Offset;
      }
   }
   /* not in table */
   return -1;
}



static GLvoid *
get_static_proc_address(const char *funcName)
{
   struct name_address {
      const char *Name;
      GLvoid *Address;
   };
   static struct name_address apitable[] = {
      { "glAccum", (GLvoid *) glAccum },
      { "glAlphaFunc", (GLvoid *) glAlphaFunc },
      { "glBegin", (GLvoid *) glBegin },
      { "glBitmap", (GLvoid *) glBitmap },
      /* ... many more ... */
      { NULL, NULL }  /* end of list marker */
   };
   GLuint i;
   for (i = 0; apitable[i].Name; i++) {
      if (strcmp(apitable[i].Name, funcName) == 0) {
         return apitable[i].Address;
      }
   }
   return NULL;
}


/*
 * Return entrypoint for named function.
 */
const GLvoid *
_glapi_get_proc_address(const char *funcName)
{
   /* look for dynamic extension entry point */

   /* look for static entry point */
   return get_static_proc_address(funcName);
}



/*
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intented for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
   assert(table->Accum);
   assert(table->AlphaFunc);
   assert(table->Begin);
   assert(table->Bitmap);
   assert(table->BlendFunc);
   assert(table->CallList);
   assert(table->CallLists);
   assert(table->Clear);
   assert(table->ClearAccum);
   assert(table->ClearColor);
   assert(table->ClearDepth);
   assert(table->ClearIndex);
   assert(table->ClearStencil);
   assert(table->ClipPlane);
   assert(table->Color3b);
   assert(table->Color3bv);
   assert(table->Color3d);
   assert(table->Color3dv);
   assert(table->Color3f);
   assert(table->Color3fv);
   assert(table->Color3i);
   assert(table->Color3iv);
   assert(table->Color3s);
   assert(table->Color3sv);
   assert(table->Color3ub);
   assert(table->Color3ubv);
   assert(table->Color3ui);
   assert(table->Color3uiv);
   assert(table->Color3us);
   assert(table->Color3usv);
   assert(table->Color4b);
   assert(table->Color4bv);
   assert(table->Color4d);
   assert(table->Color4dv);
   assert(table->Color4f);
   assert(table->Color4fv);
   assert(table->Color4i);
   assert(table->Color4iv);
   assert(table->Color4s);
   assert(table->Color4sv);
   assert(table->Color4ub);
   assert(table->Color4ubv);
   assert(table->Color4ui);
   assert(table->Color4uiv);
   assert(table->Color4us);
   assert(table->Color4usv);
   assert(table->ColorMask);
   assert(table->ColorMaterial);
   assert(table->CopyPixels);
   assert(table->CullFace);
   assert(table->DeleteLists);
   assert(table->DepthFunc);
   assert(table->DepthMask);
   assert(table->DepthRange);
   assert(table->Disable);
   assert(table->DrawBuffer);
   assert(table->DrawElements);
   assert(table->DrawPixels);
   assert(table->EdgeFlag);
   assert(table->EdgeFlagv);
   assert(table->Enable);
   assert(table->End);
   assert(table->EndList);
   assert(table->EvalCoord1d);
   assert(table->EvalCoord1dv);
   assert(table->EvalCoord1f);
   assert(table->EvalCoord1fv);
   assert(table->EvalCoord2d);
   assert(table->EvalCoord2dv);
   assert(table->EvalCoord2f);
   assert(table->EvalCoord2fv);
   assert(table->EvalMesh1);
   assert(table->EvalMesh2);
   assert(table->EvalPoint1);
   assert(table->EvalPoint2);
   assert(table->FeedbackBuffer);
   assert(table->Finish);
   assert(table->Flush);
   assert(table->Fogf);
   assert(table->Fogfv);
   assert(table->Fogi);
   assert(table->Fogiv);
   assert(table->FrontFace);
   assert(table->Frustum);
   assert(table->GenLists);
   assert(table->GetBooleanv);
   assert(table->GetClipPlane);
   assert(table->GetDoublev);
   assert(table->GetError);
   assert(table->GetFloatv);
   assert(table->GetIntegerv);
   assert(table->GetLightfv);
   assert(table->GetLightiv);
   assert(table->GetMapdv);
   assert(table->GetMapfv);
   assert(table->GetMapiv);
   assert(table->GetMaterialfv);
   assert(table->GetMaterialiv);
   assert(table->GetPixelMapfv);
   assert(table->GetPixelMapuiv);
   assert(table->GetPixelMapusv);
   assert(table->GetPolygonStipple);
   assert(table->GetString);
   assert(table->GetTexEnvfv);
   assert(table->GetTexEnviv);
   assert(table->GetTexGendv);
   assert(table->GetTexGenfv);
   assert(table->GetTexGeniv);
   assert(table->GetTexImage);
   assert(table->GetTexLevelParameterfv);
   assert(table->GetTexLevelParameteriv);
   assert(table->GetTexParameterfv);
   assert(table->GetTexParameteriv);
   assert(table->Hint);
   assert(table->IndexMask);
   assert(table->Indexd);
   assert(table->Indexdv);
   assert(table->Indexf);
   assert(table->Indexfv);
   assert(table->Indexi);
   assert(table->Indexiv);
   assert(table->Indexs);
   assert(table->Indexsv);
   assert(table->InitNames);
   assert(table->IsEnabled);
   assert(table->IsList);
   assert(table->LightModelf);
   assert(table->LightModelfv);
   assert(table->LightModeli);
   assert(table->LightModeliv);
   assert(table->Lightf);
   assert(table->Lightfv);
   assert(table->Lighti);
   assert(table->Lightiv);
   assert(table->LineStipple);
   assert(table->LineWidth);
   assert(table->ListBase);
   assert(table->LoadIdentity);
   assert(table->LoadMatrixd);
   assert(table->LoadMatrixf);
   assert(table->LoadName);
   assert(table->LogicOp);
   assert(table->Map1d);
   assert(table->Map1f);
   assert(table->Map2d);
   assert(table->Map2f);
   assert(table->MapGrid1d);
   assert(table->MapGrid1f);
   assert(table->MapGrid2d);
   assert(table->MapGrid2f);
   assert(table->Materialf);
   assert(table->Materialfv);
   assert(table->Materiali);
   assert(table->Materialiv);
   assert(table->MatrixMode);
   assert(table->MultMatrixd);
   assert(table->MultMatrixf);
   assert(table->NewList);
   assert(table->Normal3b);
   assert(table->Normal3bv);
   assert(table->Normal3d);
   assert(table->Normal3dv);
   assert(table->Normal3f);
   assert(table->Normal3fv);
   assert(table->Normal3i);
   assert(table->Normal3iv);
   assert(table->Normal3s);
   assert(table->Normal3sv);
   assert(table->Ortho);
   assert(table->PassThrough);
   assert(table->PixelMapfv);
   assert(table->PixelMapuiv);
   assert(table->PixelMapusv);
   assert(table->PixelStoref);
   assert(table->PixelStorei);
   assert(table->PixelTransferf);
   assert(table->PixelTransferi);
   assert(table->PixelZoom);
   assert(table->PointSize);
   assert(table->PolygonMode);
   assert(table->PolygonOffset);
   assert(table->PolygonStipple);
   assert(table->PopAttrib);
   assert(table->PopMatrix);
   assert(table->PopName);
   assert(table->PushAttrib);
   assert(table->PushMatrix);
   assert(table->PushName);
   assert(table->RasterPos2d);
   assert(table->RasterPos2dv);
   assert(table->RasterPos2f);
   assert(table->RasterPos2fv);
   assert(table->RasterPos2i);
   assert(table->RasterPos2iv);
   assert(table->RasterPos2s);
   assert(table->RasterPos2sv);
   assert(table->RasterPos3d);
   assert(table->RasterPos3dv);
   assert(table->RasterPos3f);
   assert(table->RasterPos3fv);
   assert(table->RasterPos3i);
   assert(table->RasterPos3iv);
   assert(table->RasterPos3s);
   assert(table->RasterPos3sv);
   assert(table->RasterPos4d);
   assert(table->RasterPos4dv);
   assert(table->RasterPos4f);
   assert(table->RasterPos4fv);
   assert(table->RasterPos4i);
   assert(table->RasterPos4iv);
   assert(table->RasterPos4s);
   assert(table->RasterPos4sv);
   assert(table->ReadBuffer);
   assert(table->ReadPixels);
   assert(table->Rectd);
   assert(table->Rectdv);
   assert(table->Rectf);
   assert(table->Rectfv);
   assert(table->Recti);
   assert(table->Rectiv);
   assert(table->Rects);
   assert(table->Rectsv);
   assert(table->RenderMode);
   assert(table->Rotated);
   assert(table->Rotatef);
   assert(table->Scaled);
   assert(table->Scalef);
   assert(table->Scissor);
   assert(table->SelectBuffer);
   assert(table->ShadeModel);
   assert(table->StencilFunc);
   assert(table->StencilMask);
   assert(table->StencilOp);
   assert(table->TexCoord1d);
   assert(table->TexCoord1dv);
   assert(table->TexCoord1f);
   assert(table->TexCoord1fv);
   assert(table->TexCoord1i);
   assert(table->TexCoord1iv);
   assert(table->TexCoord1s);
   assert(table->TexCoord1sv);
   assert(table->TexCoord2d);
   assert(table->TexCoord2dv);
   assert(table->TexCoord2f);
   assert(table->TexCoord2fv);
   assert(table->TexCoord2i);
   assert(table->TexCoord2iv);
   assert(table->TexCoord2s);
   assert(table->TexCoord2sv);
   assert(table->TexCoord3d);
   assert(table->TexCoord3dv);
   assert(table->TexCoord3f);
   assert(table->TexCoord3fv);
   assert(table->TexCoord3i);
   assert(table->TexCoord3iv);
   assert(table->TexCoord3s);
   assert(table->TexCoord3sv);
   assert(table->TexCoord4d);
   assert(table->TexCoord4dv);
   assert(table->TexCoord4f);
   assert(table->TexCoord4fv);
   assert(table->TexCoord4i);
   assert(table->TexCoord4iv);
   assert(table->TexCoord4s);
   assert(table->TexCoord4sv);
   assert(table->TexEnvf);
   assert(table->TexEnvfv);
   assert(table->TexEnvi);
   assert(table->TexEnviv);
   assert(table->TexGend);
   assert(table->TexGendv);
   assert(table->TexGenf);
   assert(table->TexGenfv);
   assert(table->TexGeni);
   assert(table->TexGeniv);
   assert(table->TexImage1D);
   assert(table->TexImage2D);
   assert(table->TexParameterf);
   assert(table->TexParameterfv);
   assert(table->TexParameteri);
   assert(table->TexParameteriv);
   assert(table->Translated);
   assert(table->Translatef);
   assert(table->Vertex2d);
   assert(table->Vertex2dv);
   assert(table->Vertex2f);
   assert(table->Vertex2fv);
   assert(table->Vertex2i);
   assert(table->Vertex2iv);
   assert(table->Vertex2s);
   assert(table->Vertex2sv);
   assert(table->Vertex3d);
   assert(table->Vertex3dv);
   assert(table->Vertex3f);
   assert(table->Vertex3fv);
   assert(table->Vertex3i);
   assert(table->Vertex3iv);
   assert(table->Vertex3s);
   assert(table->Vertex3sv);
   assert(table->Vertex4d);
   assert(table->Vertex4dv);
   assert(table->Vertex4f);
   assert(table->Vertex4fv);
   assert(table->Vertex4i);
   assert(table->Vertex4iv);
   assert(table->Vertex4s);
   assert(table->Vertex4sv);
   assert(table->Viewport);

#ifdef _GLAPI_VERSION_1_1
   assert(table->AreTexturesResident);
   assert(table->ArrayElement);
   assert(table->BindTexture);
   assert(table->ColorPointer);
   assert(table->CopyTexImage1D);
   assert(table->CopyTexImage2D);
   assert(table->CopyTexSubImage1D);
   assert(table->CopyTexSubImage2D);
   assert(table->DeleteTextures);
   assert(table->DisableClientState);
   assert(table->DrawArrays);
   assert(table->EdgeFlagPointer);
   assert(table->EnableClientState);
   assert(table->GenTextures);
   assert(table->GetPointerv);
   assert(table->IndexPointer);
   assert(table->Indexub);
   assert(table->Indexubv);
   assert(table->InterleavedArrays);
   assert(table->IsTexture);
   assert(table->NormalPointer);
   assert(table->PopClientAttrib);
   assert(table->PrioritizeTextures);
   assert(table->PushClientAttrib);
   assert(table->TexCoordPointer);
   assert(table->TexSubImage1D);
   assert(table->TexSubImage2D);
   assert(table->VertexPointer);
#endif

#ifdef _GLAPI_VERSION_1_2
   assert(table->CopyTexSubImage3D);
   assert(table->DrawRangeElements);
   assert(table->TexImage3D);
   assert(table->TexSubImage3D);
#ifdef _GLAPI_ARB_imaging
   assert(table->BlendColor);
   assert(table->BlendEquation);
   assert(table->ColorSubTable);
   assert(table->ColorTable);
   assert(table->ColorTableParameterfv);
   assert(table->ColorTableParameteriv);
   assert(table->ConvolutionFilter1D);
   assert(table->ConvolutionFilter2D);
   assert(table->ConvolutionParameterf);
   assert(table->ConvolutionParameterfv);
   assert(table->ConvolutionParameteri);
   assert(table->ConvolutionParameteriv);
   assert(table->CopyColorSubTable);
   assert(table->CopyColorTable);
   assert(table->CopyConvolutionFilter1D);
   assert(table->CopyConvolutionFilter2D);
   assert(table->GetColorTable);
   assert(table->GetColorTableParameterfv);
   assert(table->GetColorTableParameteriv);
   assert(table->GetConvolutionFilter);
   assert(table->GetConvolutionParameterfv);
   assert(table->GetConvolutionParameteriv);
   assert(table->GetHistogram);
   assert(table->GetHistogramParameterfv);
   assert(table->GetHistogramParameteriv);
   assert(table->GetMinmax);
   assert(table->GetMinmaxParameterfv);
   assert(table->GetMinmaxParameteriv);
   assert(table->Histogram);
   assert(table->Minmax);
   assert(table->ResetHistogram);
   assert(table->ResetMinmax);
   assert(table->SeparableFilter2D);
#endif
#endif


#ifdef _GLAPI_EXT_paletted_texture
   assert(table->ColorTableEXT);
   assert(table->ColorSubTableEXT);
   assert(table->GetColorTableEXT);
   assert(table->GetColorTableParameterfvEXT);
   assert(table->GetColorTableParameterivEXT);
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
   assert(table->LockArraysEXT);
   assert(table->UnlockArraysEXT);
#endif

#ifdef _GLAPI_EXT_point_parameter
   assert(table->PointParameterfEXT);
   assert(table->PointParameterfvEXT);
#endif

#ifdef _GLAPI_EXT_polygon_offset
   assert(table->PolygonOffsetEXT);
#endif

#ifdef _GLAPI_ARB_multitexture
   assert(table->ActiveTextureARB);
   assert(table->ClientActiveTextureARB);
   assert(table->MultiTexCoord1dARB);
   assert(table->MultiTexCoord1dvARB);
   assert(table->MultiTexCoord1fARB);
   assert(table->MultiTexCoord1fvARB);
   assert(table->MultiTexCoord1iARB);
   assert(table->MultiTexCoord1ivARB);
   assert(table->MultiTexCoord1sARB);
   assert(table->MultiTexCoord1svARB);
   assert(table->MultiTexCoord2dARB);
   assert(table->MultiTexCoord2dvARB);
   assert(table->MultiTexCoord2fARB);
   assert(table->MultiTexCoord2fvARB);
   assert(table->MultiTexCoord2iARB);
   assert(table->MultiTexCoord2ivARB);
   assert(table->MultiTexCoord2sARB);
   assert(table->MultiTexCoord2svARB);
   assert(table->MultiTexCoord3dARB);
   assert(table->MultiTexCoord3dvARB);
   assert(table->MultiTexCoord3fARB);
   assert(table->MultiTexCoord3fvARB);
   assert(table->MultiTexCoord3iARB);
   assert(table->MultiTexCoord3ivARB);
   assert(table->MultiTexCoord3sARB);
   assert(table->MultiTexCoord3svARB);
   assert(table->MultiTexCoord4dARB);
   assert(table->MultiTexCoord4dvARB);
   assert(table->MultiTexCoord4fARB);
   assert(table->MultiTexCoord4fvARB);
   assert(table->MultiTexCoord4iARB);
   assert(table->MultiTexCoord4ivARB);
   assert(table->MultiTexCoord4sARB);
   assert(table->MultiTexCoord4svARB);
#endif

#ifdef _GLAPI_INGR_blend_func_separate
   assert(table->BlendFuncSeparateINGR);
#endif

#ifdef _GLAPI_MESA_window_pos
   assert(table->WindowPos4fMESA);
#endif

#ifdef _GLAPI_MESA_resize_buffers
   assert(table->ResizeBuffersMESA);
#endif
}



/*
 * Generate the GL entrypoint functions here.
 */

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#ifdef USE_MGL_NAMESPACE
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#include "glapitemp.h"



#ifdef DEBUG
/*
 * This struct is just used to be sure we've defined all the API functions.
 */
static struct _glapi_table completeness_test = {
   glAccum,
   glAlphaFunc,
   glBegin,
   glBitmap,
   glBlendFunc,
   glCallList,
   glCallLists,
   glClear,
   glClearAccum,
   glClearColor,
   glClearDepth,
   glClearIndex,
   glClearStencil,
   glClipPlane,
   glColor3b,
   glColor3bv,
   glColor3d,
   glColor3dv,
   glColor3f,
   glColor3fv,
   glColor3i,
   glColor3iv,
   glColor3s,
   glColor3sv,
   glColor3ub,
   glColor3ubv,
   glColor3ui,
   glColor3uiv,
   glColor3us,
   glColor3usv,
   glColor4b,
   glColor4bv,
   glColor4d,
   glColor4dv,
   glColor4f,
   glColor4fv,
   glColor4i,
   glColor4iv,
   glColor4s,
   glColor4sv,
   glColor4ub,
   glColor4ubv,
   glColor4ui,
   glColor4uiv,
   glColor4us,
   glColor4usv,
   glColorMask,
   glColorMaterial,
   glCopyPixels,
   glCullFace,
   glDeleteLists,
   glDepthFunc,
   glDepthMask,
   glDepthRange,
   glDisable,
   glDrawBuffer,
   glDrawPixels,
   glEdgeFlag,
   glEdgeFlagv,
   glEnable,
   glEnd,
   glEndList,
   glEvalCoord1d,
   glEvalCoord1dv,
   glEvalCoord1f,
   glEvalCoord1fv,
   glEvalCoord2d,
   glEvalCoord2dv,
   glEvalCoord2f,
   glEvalCoord2fv,
   glEvalMesh1,
   glEvalMesh2,
   glEvalPoint1,
   glEvalPoint2,
   glFeedbackBuffer,
   glFinish,
   glFlush,
   glFogf,
   glFogfv,
   glFogi,
   glFogiv,
   glFrontFace,
   glFrustum,
   glGenLists,
   glGetBooleanv,
   glGetClipPlane,
   glGetDoublev,
   glGetError,
   glGetFloatv,
   glGetIntegerv,
   glGetLightfv,
   glGetLightiv,
   glGetMapdv,
   glGetMapfv,
   glGetMapiv,
   glGetMaterialfv,
   glGetMaterialiv,
   glGetPixelMapfv,
   glGetPixelMapuiv,
   glGetPixelMapusv,
   glGetPolygonStipple,
   glGetString,
   glGetTexEnvfv,
   glGetTexEnviv,
   glGetTexGendv,
   glGetTexGenfv,
   glGetTexGeniv,
   glGetTexImage,
   glGetTexLevelParameterfv,
   glGetTexLevelParameteriv,
   glGetTexParameterfv,
   glGetTexParameteriv,
   glHint,
   glIndexMask,
   glIndexd,
   glIndexdv,
   glIndexf,
   glIndexfv,
   glIndexi,
   glIndexiv,
   glIndexs,
   glIndexsv,
   glInitNames,
   glIsEnabled,
   glIsList,
   glLightModelf,
   glLightModelfv,
   glLightModeli,
   glLightModeliv,
   glLightf,
   glLightfv,
   glLighti,
   glLightiv,
   glLineStipple,
   glLineWidth,
   glListBase,
   glLoadIdentity,
   glLoadMatrixd,
   glLoadMatrixf,
   glLoadName,
   glLogicOp,
   glMap1d,
   glMap1f,
   glMap2d,
   glMap2f,
   glMapGrid1d,
   glMapGrid1f,
   glMapGrid2d,
   glMapGrid2f,
   glMaterialf,
   glMaterialfv,
   glMateriali,
   glMaterialiv,
   glMatrixMode,
   glMultMatrixd,
   glMultMatrixf,
   glNewList,
   glNormal3b,
   glNormal3bv,
   glNormal3d,
   glNormal3dv,
   glNormal3f,
   glNormal3fv,
   glNormal3i,
   glNormal3iv,
   glNormal3s,
   glNormal3sv,
   glOrtho,
   glPassThrough,
   glPixelMapfv,
   glPixelMapuiv,
   glPixelMapusv,
   glPixelStoref,
   glPixelStorei,
   glPixelTransferf,
   glPixelTransferi,
   glPixelZoom,
   glPointSize,
   glPolygonMode,
   glPolygonOffset,
   glPolygonStipple,
   glPopAttrib,
   glPopMatrix,
   glPopName,
   glPushAttrib,
   glPushMatrix,
   glPushName,
   glRasterPos2d,
   glRasterPos2dv,
   glRasterPos2f,
   glRasterPos2fv,
   glRasterPos2i,
   glRasterPos2iv,
   glRasterPos2s,
   glRasterPos2sv,
   glRasterPos3d,
   glRasterPos3dv,
   glRasterPos3f,
   glRasterPos3fv,
   glRasterPos3i,
   glRasterPos3iv,
   glRasterPos3s,
   glRasterPos3sv,
   glRasterPos4d,
   glRasterPos4dv,
   glRasterPos4f,
   glRasterPos4fv,
   glRasterPos4i,
   glRasterPos4iv,
   glRasterPos4s,
   glRasterPos4sv,
   glReadBuffer,
   glReadPixels,
   glRectd,
   glRectdv,
   glRectf,
   glRectfv,
   glRecti,
   glRectiv,
   glRects,
   glRectsv,
   glRenderMode,
   glRotated,
   glRotatef,
   glScaled,
   glScalef,
   glScissor,
   glSelectBuffer,
   glShadeModel,
   glStencilFunc,
   glStencilMask,
   glStencilOp,
   glTexCoord1d,
   glTexCoord1dv,
   glTexCoord1f,
   glTexCoord1fv,
   glTexCoord1i,
   glTexCoord1iv,
   glTexCoord1s,
   glTexCoord1sv,
   glTexCoord2d,
   glTexCoord2dv,
   glTexCoord2f,
   glTexCoord2fv,
   glTexCoord2i,
   glTexCoord2iv,
   glTexCoord2s,
   glTexCoord2sv,
   glTexCoord3d,
   glTexCoord3dv,
   glTexCoord3f,
   glTexCoord3fv,
   glTexCoord3i,
   glTexCoord3iv,
   glTexCoord3s,
   glTexCoord3sv,
   glTexCoord4d,
   glTexCoord4dv,
   glTexCoord4f,
   glTexCoord4fv,
   glTexCoord4i,
   glTexCoord4iv,
   glTexCoord4s,
   glTexCoord4sv,
   glTexEnvf,
   glTexEnvfv,
   glTexEnvi,
   glTexEnviv,
   glTexGend,
   glTexGendv,
   glTexGenf,
   glTexGenfv,
   glTexGeni,
   glTexGeniv,
   glTexImage1D,
   glTexImage2D,
   glTexParameterf,
   glTexParameterfv,
   glTexParameteri,
   glTexParameteriv,
   glTranslated,
   glTranslatef,
   glVertex2d,
   glVertex2dv,
   glVertex2f,
   glVertex2fv,
   glVertex2i,
   glVertex2iv,
   glVertex2s,
   glVertex2sv,
   glVertex3d,
   glVertex3dv,
   glVertex3f,
   glVertex3fv,
   glVertex3i,
   glVertex3iv,
   glVertex3s,
   glVertex3sv,
   glVertex4d,
   glVertex4dv,
   glVertex4f,
   glVertex4fv,
   glVertex4i,
   glVertex4iv,
   glVertex4s,
   glVertex4sv,
   glViewport,

#ifdef _GLAPI_VERSION_1_1
   glAreTexturesResident,
   glArrayElement,
   glBindTexture,
   glColorPointer,
   glCopyTexImage1D,
   glCopyTexImage2D,
   glCopyTexSubImage1D,
   glCopyTexSubImage2D,
   glDeleteTextures,
   glDisableClientState,
   glDrawArrays,
   glDrawElements,
   glEdgeFlagPointer,
   glEnableClientState,
   glGenTextures,
   glGetPointerv,
   glIndexPointer,
   glIndexub,
   glIndexubv,
   glInterleavedArrays,
   glIsTexture,
   glNormalPointer,
   glPopClientAttrib,
   glPrioritizeTextures,
   glPushClientAttrib,
   glTexCoordPointer,
   glTexSubImage1D,
   glTexSubImage2D,
   glVertexPointer,
#endif

#ifdef _GLAPI_VERSION_1_2
   glCopyTexSubImage3D,
   glDrawRangeElements,
   glTexImage3D,
   glTexSubImage3D,

#ifdef _GLAPI_ARB_imaging
   glBlendColor,
   glBlendEquation,
   glColorSubTable,
   glColorTable,
   glColorTableParameterfv,
   glColorTableParameteriv,
   glConvolutionFilter1D,
   glConvolutionFilter2D,
   glConvolutionParameterf,
   glConvolutionParameterfv,
   glConvolutionParameteri,
   glConvolutionParameteriv,
   glCopyColorSubTable,
   glCopyColorTable,
   glCopyConvolutionFilter1D,
   glCopyConvolutionFilter2D,
   glGetColorTable,
   glGetColorTableParameterfv,
   glGetColorTableParameteriv,
   glGetConvolutionFilter,
   glGetConvolutionParameterfv,
   glGetConvolutionParameteriv,
   glGetHistogram,
   glGetHistogramParameterfv,
   glGetHistogramParameteriv,
   glGetMinmax,
   glGetMinmaxParameterfv,
   glGetMinmaxParameteriv,
   glGetSeparableFilter,
   glHistogram,
   glMinmax,
   glResetHistogram,
   glResetMinmax,
   glSeparableFilter2D,
#endif
#endif


   /*
    * Extensions
    */

#ifdef _GLAPI_EXT_paletted_texture
   glColorTableEXT,
   glColorSubTableEXT,
   glGetColorTableEXT,
   glGetColorTableParameterfvEXT,
   glGetColorTableParameterivEXT,
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
   glLockArraysEXT,
   glUnlockArraysEXT,
#endif

#ifdef _GLAPI_EXT_point_parameters
   glPointParameterfEXT,
   glPointParameterfvEXT,
#endif

#ifdef _GLAPI_EXT_polygon_offset
   glPolygonOffsetEXT,
#endif

#ifdef _GLAPI_EXT_blend_minmax
   glBlendEquationEXT,
#endif

#ifdef _GLAPI_EXT_blend_color
   glBlendColorEXT,
#endif

#ifdef _GLAPI_ARB_multitexture
   glActiveTextureARB,
   glClientActiveTextureARB,
   glMultiTexCoord1dARB,
   glMultiTexCoord1dvARB,
   glMultiTexCoord1fARB,
   glMultiTexCoord1fvARB,
   glMultiTexCoord1iARB,
   glMultiTexCoord1ivARB,
   glMultiTexCoord1sARB,
   glMultiTexCoord1svARB,
   glMultiTexCoord2dARB,
   glMultiTexCoord2dvARB,
   glMultiTexCoord2fARB,
   glMultiTexCoord2fvARB,
   glMultiTexCoord2iARB,
   glMultiTexCoord2ivARB,
   glMultiTexCoord2sARB,
   glMultiTexCoord2svARB,
   glMultiTexCoord3dARB,
   glMultiTexCoord3dvARB,
   glMultiTexCoord3fARB,
   glMultiTexCoord3fvARB,
   glMultiTexCoord3iARB,
   glMultiTexCoord3ivARB,
   glMultiTexCoord3sARB,
   glMultiTexCoord3svARB,
   glMultiTexCoord4dARB,
   glMultiTexCoord4dvARB,
   glMultiTexCoord4fARB,
   glMultiTexCoord4fvARB,
   glMultiTexCoord4iARB,
   glMultiTexCoord4ivARB,
   glMultiTexCoord4sARB,
   glMultiTexCoord4svARB,
#endif

#ifdef _GLAPI_INGR_blend_func_separate
   glBlendFuncSeparateINGR,
#endif

#ifdef _GLAPI_MESA_window_pos
   glWindowPos4fMESA,
#endif

#ifdef _GLAPI_MESA_resize_buffers
   glResizeBuffersMESA
#endif

};

#endif /*DEBUG*/
