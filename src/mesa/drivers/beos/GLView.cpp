/* $Id: GLView.cpp,v 1.5 2000/11/17 21:01:26 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * $Log: GLView.cpp,v $
 * Revision 1.5  2000/11/17 21:01:26  brianp
 * Minor header file changes to silence warnings.
 * Added _mesa_enable_sw_extensions(), called by software-only drivers
 * to enable all s/w-supported GL extensions.
 *
 * Revision 1.4  2000/11/14 17:51:15  brianp
 * more Driver.Color, Driver.Index updates
 *
 * Revision 1.3  2000/09/26 20:54:09  brianp
 * First batch of OpenGL SI related changes:
 * Renamed struct gl_context to struct __GLcontextRec.
 * Include glcore.h, setup GL imports/exports.
 * Replaced gl_ prefix with _mesa_ prefix in context.[ch] functions.
 * GLcontext's Visual field is no longer a pointer.
 *
 * Revision 1.2  2000/03/19 01:13:13  brianp
 * updated for Mesa 3.3
 *
 * Revision 1.1.1.1  1999/08/19 00:55:41  jtg
 * Imported sources
 *
 * Revision 1.7  1999/03/28 21:08:17  brianp
 * updated SetBuffer driver function
 *
 * Revision 1.6  1999/02/14 03:44:37  brianp
 * new copyright
 *
 * Revision 1.5  1999/02/11 03:50:57  brianp
 * added CopySubBufferMESA()
 *
 * Revision 1.4  1999/02/06 17:44:59  brianp
 * code clean-up and bug fixes
 *
 * Revision 1.3  1999/02/04 04:13:15  brianp
 * implemented double buffering
 *
 * Revision 1.2  1999/02/03 04:23:28  brianp
 * basic device driver functions now work (yeah!)
 *
 * Revision 1.1  1999/02/02 04:40:46  brianp
 * Initial revision
 */



#include <assert.h>
#include <stdio.h>
#include <GLView.h>
#include "../src/context.h"
#include "extensions.h"


// BeOS component ordering for B_RGBA32 bitmap format
#define BE_RCOMP 2
#define BE_GCOMP 1
#define BE_BCOMP 0
#define BE_ACOMP 3


//
// This object hangs off of the BGLView object.  We have to use
// Be's BGLView class as-is to maintain binary compatibility (we
// can't add new members to it).  Instead we just put all our data
// in this class and use BGLVIew::m_gc to point to it.
//
class AuxInfo
{
public:
   AuxInfo();
   ~AuxInfo();
   void Init(BGLView *bglView, GLcontext *c, GLvisual *v, GLframebuffer *b);

   void MakeCurrent();
   void SwapBuffers() const;
   void CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const;

private:
   AuxInfo(const AuxInfo &rhs);  // copy constructor illegal
   AuxInfo &operator=(const AuxInfo &rhs);  // assignment oper. illegal

   GLcontext *mContext;
   GLvisual *mVisual;
   GLframebuffer *mBuffer;

   BGLView *mBGLView;
   BBitmap *mBitmap;

   GLubyte mClearColor[4];  // buffer clear color
   GLuint mClearIndex;      // buffer clear color index
   GLint mBottom;           // used for flipping Y coords
   GLint mWidth, mHeight;   // size of buffer

   // Mesa device driver functions
   static void UpdateState(GLcontext *ctx);
   static void ClearIndex(GLcontext *ctx, GLuint index);
   static void ClearColor(GLcontext *ctx, GLubyte r, GLubyte g,
                          GLubyte b, GLubyte a);
   static GLbitfield Clear(GLcontext *ctx, GLbitfield mask,
                                GLboolean all, GLint x, GLint y,
                                GLint width, GLint height);
   static void ClearFront(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                          GLint width, GLint height);
   static void ClearBack(GLcontext *ctx, GLboolean all, GLint x, GLint y,
                         GLint width, GLint height);
   static void Index(GLcontext *ctx, GLuint index);
   static void Color(GLcontext *ctx, GLubyte r, GLubyte g,
                     GLubyte b, GLubyte a);
   static GLboolean SetDrawBuffer(GLcontext *ctx, GLenum mode);
   static void SetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                             GLenum mode);
   static void GetBufferSize(GLcontext *ctgx, GLuint *width,
                             GLuint *height);
   static const GLubyte *GetString(GLcontext *ctx, GLenum name);

   // Front-buffer functions
   static void WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                      GLint x, GLint y,
                                      const GLchan color[4],
                                      const GLubyte mask[]);
   static void WriteRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    CONST GLubyte rgba[][4],
                                    const GLubyte mask[]);
   static void WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                        const GLint x[], const GLint y[],
                                        const GLchan color[4],
                                        const GLubyte mask[]);
   static void WriteCI32SpanFront(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  const GLuint index[], const GLubyte mask[]);
   static void WriteCI8SpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLubyte index[], const GLubyte mask[]);
   static void WriteMonoCISpanFront(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[]);
   static void WriteCI32PixelsFront(const GLcontext *ctx,
                                    GLuint n, const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[]);
   static void WriteMonoCIPixelsFront(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[]);
   static void ReadCI32SpanFront(const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[]);
   static void ReadRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 GLubyte rgba[][4]);
   static void ReadCI32PixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[]);
   static void ReadRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[]);

   // Back buffer functions
   static void WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                  GLint x, GLint y,
                                  CONST GLubyte rgba[][4],
                                  const GLubyte mask[]);
   static void WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][3],
                                 const GLubyte mask[]);
   static void WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[]);
   static void WriteRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[]);
   static void WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[]);
   static void WriteCI32SpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[]);
   static void WriteCI8SpanBack(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[]);
   static void WriteMonoCISpanBack(const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y, GLuint colorIndex,
                                   const GLubyte mask[]);
   static void WriteCI32PixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[]);
   static void WriteMonoCIPixelsBack(const GLcontext *ctx,
                                     GLuint n, const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[]);
   static void ReadCI32SpanBack(const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[]);
   static void ReadRGBASpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                GLubyte rgba[][4]);
   static void ReadCI32PixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLuint indx[], const GLubyte mask[]);
   static void ReadRGBAPixelsBack(const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[]);

};



AuxInfo::AuxInfo()
{
   mContext = NULL;
   mVisual = NULL;
   mBuffer = NULL;
   mBGLView = NULL;
   mBitmap = NULL;
   mClearColor[BE_RCOMP] = 0;
   mClearColor[BE_GCOMP] = 0;
   mClearColor[BE_BCOMP] = 0;
   mClearColor[BE_ACOMP] = 0;
   mClearIndex = 0;
}


AuxInfo::~AuxInfo()
{

   _mesa_destroy_visual(mVisual);
   _mesa_destroy_framebuffer(mBuffer);
   _mesa_destroy_context(mContext);
}


void AuxInfo::Init(BGLView *bglView, GLcontext *c, GLvisual *v, GLframebuffer *b)
{
   mBGLView = bglView;
   mContext = c;
   mVisual = v;
   mBuffer = b;
}


void AuxInfo::MakeCurrent()
{
   UpdateState(mContext);
   _mesa_make_current(mContext, mBuffer);
}


void AuxInfo::SwapBuffers() const
{
   if (mBitmap) {
      mBGLView->DrawBitmap(mBitmap, BPoint(0, 0));
   }
}


void AuxInfo::CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const
{
   if (mBitmap) {
      // Source bitmap and view's bitmap are same size.
      // Source and dest rectangle are the same.
      // Note (x,y) = (0,0) is the lower-left corner, have to flip Y
      BRect srcAndDest;
      srcAndDest.left = x;
      srcAndDest.right = x + width - 1;
      srcAndDest.bottom = mBottom - y;
      srcAndDest.top = srcAndDest.bottom - height + 1;
      mBGLView->DrawBitmap(mBitmap, srcAndDest, srcAndDest);
   }
}


void AuxInfo::UpdateState( GLcontext *ctx )
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;

   assert(aux->mContext == ctx );

   ctx->Driver.UpdateState = AuxInfo::UpdateState;
   ctx->Driver.SetDrawBuffer = AuxInfo::SetDrawBuffer;
   ctx->Driver.SetReadBuffer = AuxInfo::SetReadBuffer;
   ctx->Driver.ClearIndex = AuxInfo::ClearIndex;
   ctx->Driver.ClearColor = AuxInfo::ClearColor;
   ctx->Driver.GetBufferSize = AuxInfo::GetBufferSize;
   ctx->Driver.GetString = AuxInfo::GetString;
   ctx->Driver.Clear = AuxInfo::Clear;

   if (ctx->Color.DrawBuffer == GL_FRONT) {
      /* read/write front buffer */
      ctx->Driver.WriteRGBASpan = AuxInfo::WriteRGBASpanFront;
      ctx->Driver.WriteRGBSpan = AuxInfo::WriteRGBSpanFront;
      ctx->Driver.WriteRGBAPixels = AuxInfo::WriteRGBAPixelsFront;
      ctx->Driver.WriteMonoRGBASpan = AuxInfo::WriteMonoRGBASpanFront;
      ctx->Driver.WriteMonoRGBAPixels = AuxInfo::WriteMonoRGBAPixelsFront;
      ctx->Driver.WriteCI32Span = AuxInfo::WriteCI32SpanFront;
      ctx->Driver.WriteCI8Span = AuxInfo::WriteCI8SpanFront;
      ctx->Driver.WriteMonoCISpan = AuxInfo::WriteMonoCISpanFront;
      ctx->Driver.WriteCI32Pixels = AuxInfo::WriteCI32PixelsFront;
      ctx->Driver.WriteMonoCIPixels = AuxInfo::WriteMonoCIPixelsFront;
      ctx->Driver.ReadRGBASpan = AuxInfo::ReadRGBASpanFront;
      ctx->Driver.ReadRGBAPixels = AuxInfo::ReadRGBAPixelsFront;
      ctx->Driver.ReadCI32Span = AuxInfo::ReadCI32SpanFront;
      ctx->Driver.ReadCI32Pixels = AuxInfo::ReadCI32PixelsFront;
   }
   else {
      /* read/write back buffer */
      ctx->Driver.WriteRGBASpan = AuxInfo::WriteRGBASpanBack;
      ctx->Driver.WriteRGBSpan = AuxInfo::WriteRGBSpanBack;
      ctx->Driver.WriteRGBAPixels = AuxInfo::WriteRGBAPixelsBack;
      ctx->Driver.WriteMonoRGBASpan = AuxInfo::WriteMonoRGBASpanBack;
      ctx->Driver.WriteMonoRGBAPixels = AuxInfo::WriteMonoRGBAPixelsBack;
      ctx->Driver.WriteCI32Span = AuxInfo::WriteCI32SpanBack;
      ctx->Driver.WriteCI8Span = AuxInfo::WriteCI8SpanBack;
      ctx->Driver.WriteMonoCISpan = AuxInfo::WriteMonoCISpanBack;
      ctx->Driver.WriteCI32Pixels = AuxInfo::WriteCI32PixelsBack;
      ctx->Driver.WriteMonoCIPixels = AuxInfo::WriteMonoCIPixelsBack;
      ctx->Driver.ReadRGBASpan = AuxInfo::ReadRGBASpanBack;
      ctx->Driver.ReadRGBAPixels = AuxInfo::ReadRGBAPixelsBack;
      ctx->Driver.ReadCI32Span = AuxInfo::ReadCI32SpanBack;
      ctx->Driver.ReadCI32Pixels = AuxInfo::ReadCI32PixelsBack;
    }
}


void AuxInfo::ClearIndex(GLcontext *ctx, GLuint index)
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   aux->mClearIndex = index;
}


void AuxInfo::ClearColor(GLcontext *ctx, GLubyte r, GLubyte g,
                         GLubyte b, GLubyte a)
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   aux->mClearColor[BE_RCOMP] = r;
   aux->mClearColor[BE_GCOMP] = g;
   aux->mClearColor[BE_BCOMP] = b;
   aux->mClearColor[BE_ACOMP] = a;
   assert(aux->mBGLView);
}


GLbitfield AuxInfo::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
   if (mask & DD_FRONT_LEFT_BIT)
      ClearFront(ctx, all, x, y, width, height);
   if (mask & DD_BACK_LEFT_BIT)
      ClearBack(ctx, all, x, y, width, height);
   return mask & ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);
}


void AuxInfo::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);

   bglview->SetHighColor(aux->mClearColor[BE_RCOMP],
                         aux->mClearColor[BE_GCOMP],
                         aux->mClearColor[BE_BCOMP],
                         aux->mClearColor[BE_ACOMP]);
   bglview->SetLowColor(aux->mClearColor[BE_RCOMP],
                        aux->mClearColor[BE_GCOMP],
                        aux->mClearColor[BE_BCOMP],
                        aux->mClearColor[BE_ACOMP]);
   if (all) {
      BRect b = bglview->Bounds();
      bglview->FillRect(b);
   }
   else {
      // XXX untested
      BRect b;
      b.left = x;
      b.right = x + width;
      b.bottom = aux->mHeight - y - 1;
      b.top = b.bottom - height;
      bglview->FillRect(b);
   }

   // restore drawing color
#if 0
   bglview->SetHighColor(aux->mColor[BE_RCOMP],
                         aux->mColor[BE_GCOMP],
                         aux->mColor[BE_BCOMP],
                         aux->mColor[BE_ACOMP]);
   bglview->SetLowColor(aux->mColor[BE_RCOMP],
                        aux->mColor[BE_GCOMP],
                        aux->mColor[BE_BCOMP],
                        aux->mColor[BE_ACOMP]);
#endif
}


void AuxInfo::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   GLuint *start = (GLuint *) bitmap->Bits();
   const GLuint *clearPixelPtr = (const GLuint *) aux->mClearColor;
   const GLuint clearPixel = *clearPixelPtr;

   if (all) {
      const int numPixels = aux->mWidth * aux->mHeight;
      if (clearPixel == 0) {
         memset(start, 0, numPixels * 4);
      }
      else {
         for (int i = 0; i < numPixels; i++) {
             start[i] = clearPixel;
         }
      }
   }
   else {
      // XXX untested
      start += y * aux->mWidth + x;
      for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
            start[j] = clearPixel;
         }
         start += aux->mWidth;
      }
   }
}


GLboolean AuxInfo::SetDrawBuffer(GLcontext *ctx, GLenum buffer)
{
   if (buffer == GL_FRONT_LEFT)
      return GL_TRUE;
   else if (buffer == GL_BACK_LEFT)
      return GL_TRUE;
   else
      return GL_FALSE;
}

void AuxInfo::SetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                            GLenum buffer)
{
   /* XXX to do */
}

void AuxInfo::GetBufferSize(GLcontext *ctx, GLuint *width,
                            GLuint *height)
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   BRect b = bglview->Bounds();
   *width = (GLuint) (b.right - b.left + 1);
   *height = (GLuint) (b.bottom - b.top + 1);
   aux->mBottom = (GLint) b.bottom;

   if (ctx->Visual->DBflag) {
      if (*width != aux->mWidth || *height != aux->mHeight) {
         // allocate new size of back buffer bitmap
         if (aux->mBitmap)
            delete aux->mBitmap;
         BRect rect(0.0, 0.0, *width - 1, *height - 1);
         aux->mBitmap = new BBitmap(rect, B_RGBA32);
      }
   }
   else
   {
      aux->mBitmap = NULL;
   }

   aux->mWidth = *width;
   aux->mHeight = *height;
}


const GLubyte *AuxInfo::GetString(GLcontext *ctx, GLenum name)
{
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa BeOS";
      default:
         // Let core library handle all other cases
         return NULL;
   }
}


// Plot a pixel.  (0,0) is upper-left corner
// This is only used when drawing to the front buffer.
static void Plot(BGLView *bglview, int x, int y)
{
   // XXX There's got to be a better way!
   BPoint p(x, y), q(x+1, y);
   bglview->StrokeLine(p, q);
}


void AuxInfo::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   int flippedY = aux->mBottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void AuxInfo::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   int flippedY = aux->mBottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void AuxInfo::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   int flippedY = aux->mBottom - y;
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x++, flippedY);
      }
   }
}

void AuxInfo::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x[i], aux->mBottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x[i], aux->mBottom - y[i]);
      }
   }
}


void AuxInfo::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BGLView *bglview = aux->mBGLView;
   assert(bglview);
   // plot points using current color
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x[i], aux->mBottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x[i], aux->mBottom - y[i]);
      }
   }
}


void AuxInfo::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
   // XXX to do
}


void AuxInfo::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
   // XXX to do
}


void AuxInfo::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
   // XXX to do
}




void AuxInfo::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   int row = aux->mBottom - y;
   GLubyte *pixel = (GLubyte *) bitmap->Bits() + (row * aux->mWidth + x) * 4;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            pixel[BE_RCOMP] = rgba[i][RCOMP];
            pixel[BE_GCOMP] = rgba[i][GCOMP];
            pixel[BE_BCOMP] = rgba[i][BCOMP];
            pixel[BE_ACOMP] = rgba[i][ACOMP];
         }
         pixel += 4;
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         pixel[BE_RCOMP] = rgba[i][RCOMP];
         pixel[BE_GCOMP] = rgba[i][GCOMP];
         pixel[BE_BCOMP] = rgba[i][BCOMP];
         pixel[BE_ACOMP] = rgba[i][ACOMP];
         pixel += 4;
      }
   }
}


void AuxInfo::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   int row = aux->mBottom - y;
   GLubyte *pixel = (GLubyte *) bitmap->Bits() + (row * aux->mWidth + x) * 4;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            pixel[BE_RCOMP] = rgb[i][RCOMP];
            pixel[BE_GCOMP] = rgb[i][GCOMP];
            pixel[BE_BCOMP] = rgb[i][BCOMP];
            pixel[BE_ACOMP] = 255;
         }
         pixel += 4;
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         pixel[BE_RCOMP] = rgb[i][RCOMP];
         pixel[BE_GCOMP] = rgb[i][GCOMP];
         pixel[BE_BCOMP] = rgb[i][BCOMP];
         pixel[BE_ACOMP] = 255;
         pixel += 4;
      }
   }
}


void AuxInfo::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   int row = aux->mBottom - y;
   GLuint *pixelPtr = (GLuint *) bitmap->Bits() + row * aux->mWidth + x;
   GLuint pixel;
   GLubyte *mColor= (GLubyte *) &pixel;
   mColor[BE_RCOMP] = color[RCOMP];
   mColor[BE_GCOMP] = color[GCOMP];
   mColor[BE_BCOMP] = color[BCOMP];
   mColor[BE_ACOMP] = color[ACOMP];
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i])
            *pixelPtr = pixel;
         pixelPtr++;
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         *pixelPtr++ = pixel;
      }
   }
}


void AuxInfo::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + (aux->mBottom - y[i]) * bitmap->BytesPerRow() + x[i] * 4;
            pixel[BE_RCOMP] = rgba[i][RCOMP];
            pixel[BE_GCOMP] = rgba[i][GCOMP];
            pixel[BE_BCOMP] = rgba[i][BCOMP];
            pixel[BE_ACOMP] = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + (aux->mBottom - y[i]) * bitmap->BytesPerRow() + x[i] * 4;
         pixel[BE_RCOMP] = rgba[i][RCOMP];
         pixel[BE_GCOMP] = rgba[i][GCOMP];
         pixel[BE_BCOMP] = rgba[i][BCOMP];
         pixel[BE_ACOMP] = rgba[i][ACOMP];
      }
   }
}


void AuxInfo::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   BBitmap *bitmap = aux->mBitmap;
   GLuint pixel;
   GLubyte *mColor= (GLubyte *) &pixel;
   mColor[BE_RCOMP] = color[RCOMP];
   mColor[BE_GCOMP] = color[GCOMP];
   mColor[BE_BCOMP] = color[BCOMP];
   mColor[BE_ACOMP] = color[ACOMP];
   assert(bitmap);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLuint *pixelPtr = (GLuint *) bitmap->Bits()
                    + (aux->mBottom - y[i]) * aux->mWidth + x[i];
            *pixelPtr = pixel;
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         GLuint *pixelPtr = (GLuint *) bitmap->Bits()
                 + (aux->mBottom - y[i]) * aux->mWidth + x[i];
         *pixelPtr = pixel;
      }
   }
}


void AuxInfo::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void AuxInfo::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
   // XXX to do
}


void AuxInfo::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   const BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   int row = aux->mBottom - y;
   const GLubyte *pixel = (GLubyte *) bitmap->Bits()
                        + row * bitmap->BytesPerRow() + x * 4;
   for (GLuint i = 0; i < n; i++) {
      rgba[i][RCOMP] = pixel[BE_RCOMP];
      rgba[i][GCOMP] = pixel[BE_GCOMP];
      rgba[i][BCOMP] = pixel[BE_BCOMP];
      rgba[i][ACOMP] = pixel[BE_ACOMP];
      pixel += 4;
   }
}


void AuxInfo::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
   // XXX to do
}


void AuxInfo::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
   AuxInfo *aux = (AuxInfo *) ctx->DriverCtx;
   const BBitmap *bitmap = aux->mBitmap;
   assert(bitmap);
   for (GLuint i = 0; i < n; i++) {
      if (y[i] < aux->mHeight) {
         const GLubyte *pixel = (const GLubyte *) bitmap->Bits()
            + ((aux->mBottom - y[i]) * aux->mWidth + x[i]) * 4;
         rgba[i][RCOMP] = pixel[BE_RCOMP];
         rgba[i][GCOMP] = pixel[BE_GCOMP];
         rgba[i][BCOMP] = pixel[BE_BCOMP];
         rgba[i][ACOMP] = pixel[BE_ACOMP];
      }
   }
}




//------------------------------------------------------------------
// Public interface methods
//------------------------------------------------------------------


//
// Input:  rect - initial rectangle
//         name - window name
//         resizingMode - example: B_FOLLOW_NONE
//         mode - usually 0 ?
//         options - Bitwise-OR of BGL_* tokens
//
BGLView::BGLView(BRect rect, char *name,
                 ulong resizingMode, ulong mode,
                 ulong options)
   :BView(rect, name, resizingMode, mode)
{
   const GLboolean rgbFlag = (options & BGL_RGB) == BGL_RGB;
   const GLboolean alphaFlag = (options & BGL_ALPHA) == BGL_ALPHA;
   const GLboolean dblFlag = (options & BGL_DOUBLE) == BGL_DOUBLE;
   const GLboolean stereoFlag = false;
   const GLint depth = (options & BGL_DEPTH) ? 16 : 0;
   const GLint stencil = (options & BGL_STENCIL) ? 8 : 0;
   const GLint accum = (options & BGL_ACCUM) ? 16 : 0;
   const GLint index = (options & BGL_INDEX) ? 32 : 0;
   const GLint red = (options & BGL_RGB) ? 8 : 0;
   const GLint green = (options & BGL_RGB) ? 8 : 0;
   const GLint blue = (options & BGL_RGB) ? 8 : 0;
   const GLint alpha = (options & BGL_RGB) ? 8 : 0;

   if (!rgbFlag) {
      fprintf(stderr, "Mesa Warning: color index mode not supported\n");
   }

   // Allocate auxiliary data object
   AuxInfo *aux = new AuxInfo;

   // examine option flags and create gl_context struct
   GLvisual *visual = _mesa__create_visual( rgbFlag,
                                            dblFlag,
                                            stereoFlag,
                                            red, green, blue, alpha,
                                            index,
                                            depth,
                                            stencil,
                                            accum, accum, accum, accum,
                                            1
                                            );

   // create core framebuffer
   GLframebuffer *buffer = _mesa_create_framebuffer(visual,
                                              depth > 0 ? GL_TRUE : GL_FALSE,
                                              stencil > 0 ? GL_TRUE: GL_FALSE,
                                              accum > 0 ? GL_TRUE : GL_FALSE,
                                              alphaFlag
                                              );

   // create core context
   const GLboolean direct = GL_TRUE;
   GLcontext *ctx = _mesa_create_context( visual, NULL, aux, direct );

   _mesa_enable_sw_extensions(ctx);

   aux->Init(this, ctx, visual, buffer );

   // Hook aux data into BGLView object
   m_gc = aux;
}


BGLView::~BGLView()
{
   printf("BGLView destructor\n");
   AuxInfo *aux = (AuxInfo *) m_gc;
   assert(aux);
   delete aux;
}

void BGLView::LockGL()
{
   AuxInfo *aux = (AuxInfo *) m_gc;
   assert(aux);
   aux->MakeCurrent();
}

void BGLView::UnlockGL()
{
   AuxInfo *aux = (AuxInfo *) m_gc;
   assert(aux);
   // Could call _mesa_make_current(NULL, NULL) but it would just
   // hinder performance
}

void BGLView::SwapBuffers()
{
   AuxInfo *aux = (AuxInfo *) m_gc;
   assert(aux);
   aux->SwapBuffers();
}


void BGLView::CopySubBufferMESA(GLint x, GLint y, GLuint width, GLuint height)
{
   AuxInfo *aux = (AuxInfo *) m_gc;
   assert(aux);
   aux->CopySubBuffer(x, y, width, height);
}


BView *BGLView::EmbeddedView()
{
   // XXX to do

}

status_t BGLView::CopyPixelsOut(BPoint source, BBitmap *dest)
{
   // XXX to do
}


status_t BGLView::CopyPixelsIn(BBitmap *source, BPoint dest)
{
   // XXX to do
}

void BGLView::ErrorCallback(GLenum errorCode)
{
   // XXX to do
}

void BGLView::Draw(BRect updateRect)
{
//   printf("BGLView draw\n");
   // XXX to do
}

void BGLView::AttachedToWindow()
{
   BView::AttachedToWindow();

   // don't paint window background white when resized
   SetViewColor(B_TRANSPARENT_32_BIT);
}

void BGLView::AllAttached()
{
   BView::AllAttached();
//   printf("BGLView AllAttached\n");
}

void BGLView::DetachedFromWindow()
{
   BView::DetachedFromWindow();
}

void BGLView::AllDetached()
{
   BView::AllDetached();
//   printf("BGLView AllDetached");
}

void BGLView::FrameResized(float width, float height)
{
   return BView::FrameResized(width, height);
}

status_t BGLView::Perform(perform_code d, void *arg)
{
   return BView::Perform(d, arg);
}


status_t BGLView::Archive(BMessage *data, bool deep) const
{
   return BView::Archive(data, deep);
}

void BGLView::MessageReceived(BMessage *msg)
{
   BView::MessageReceived(msg);
}

void BGLView::SetResizingMode(uint32 mode)
{
   BView::SetResizingMode(mode);
}

void BGLView::Show()
{
//   printf("BGLView Show\n");
   BView::Show();
}

void BGLView::Hide()
{
//   printf("BGLView Hide\n");
   BView::Hide();
}

BHandler *BGLView::ResolveSpecifier(BMessage *msg, int32 index,
                                    BMessage *specifier, int32 form,
                                    const char *property)
{
   return BView::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BGLView::GetSupportedSuites(BMessage *data)
{
   return BView::GetSupportedSuites(data);
}

void BGLView::DirectConnected( direct_buffer_info *info )
{
   // XXX to do
}

void BGLView::EnableDirectMode( bool enabled )
{
   // XXX to do
}



//---- private methods ----------

void BGLView::_ReservedGLView1() {}
void BGLView::_ReservedGLView2() {}
void BGLView::_ReservedGLView3() {}
void BGLView::_ReservedGLView4() {}
void BGLView::_ReservedGLView5() {}
void BGLView::_ReservedGLView6() {}
void BGLView::_ReservedGLView7() {}
void BGLView::_ReservedGLView8() {}

#if 0
BGLView::BGLView(const BGLView &v)
	: BView(v)
{
   // XXX not sure how this should work
   printf("Warning BGLView::copy constructor not implemented\n");
}
#endif


BGLView &BGLView::operator=(const BGLView &v)
{
   printf("Warning BGLView::operator= not implemented\n");
}

void BGLView::dither_front()
{
   // no-op
}

bool BGLView::confirm_dither()
{
   // no-op
   return false;
}

void BGLView::draw(BRect r)
{
   // XXX no-op ???
}

/* Direct Window stuff */
void BGLView::drawScanline( int x1, int x2, int y, void *data )
{
   // no-op
}

void BGLView::scanlineHandler(struct rasStateRec *state,
                              GLint x1, GLint x2)
{
   // no-op
}

void BGLView::lock_draw()
{
   // no-op
}

void BGLView::unlock_draw()
{
   // no-op
}

bool BGLView::validateView()
{
   // no-op
   return true;
}

