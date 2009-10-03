/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Measure glReadPixels speed.
 * XXX also read into a PBO?
 * XXX also read from FBOs?
 *
 * Brian Paul
 * 23 Sep 2009
 */

#include <string.h>
#include <assert.h>
#include "glmain.h"
#include "common.h"

int WinWidth = 1000, WinHeight = 1000;

static GLuint VBO;

static const GLboolean DrawPoint = GL_TRUE;
static const GLboolean BufferSubDataInHalves = GL_TRUE;

static const GLfloat Vertex0[2] = { 0.0, 0.0 };

static GLenum HaveDepthStencil;

static GLenum ReadFormat, ReadType;
static GLint ReadWidth, ReadHeight;
static GLvoid *ReadBuffer;


/** Called from test harness/main */
void
PerfInit(void)
{
   /* setup VBO */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(Vertex0), Vertex0, GL_STATIC_DRAW_ARB);
   glVertexPointer(2, GL_FLOAT, sizeof(Vertex0), (void *) 0);
   glEnableClientState(GL_VERTEX_ARRAY);

   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   HaveDepthStencil = PerfExtensionSupported("GL_EXT_packed_depth_stencil");

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_STENCIL_TEST);
}


static void
ReadPixels(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      /* read from random pos */
      GLint x, y;

      x = WinWidth - ReadWidth;
      y = WinHeight - ReadHeight;
      if (x > 0)
         x = rand() % x;
      if (y > 0)
         y = rand() % y;

      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);

      glReadPixels(x, y, ReadWidth, ReadHeight,
                   ReadFormat, ReadType, ReadBuffer);
   }
   glFinish();
}


static const GLsizei Sizes[] = {
   10,
   100,
   500,
   1000,
   0
};


static const struct {
   GLenum format;
   GLenum type;
   const char *name;
   GLuint pixel_size;
} DstFormats[] = {
   { GL_RGBA, GL_UNSIGNED_BYTE,           "RGBA/ubyte", 4 },
   { GL_BGRA, GL_UNSIGNED_BYTE,           "BGRA/ubyte", 4 },
   { GL_RGB, GL_UNSIGNED_SHORT_5_6_5,     "RGB/565", 2 },
   { GL_LUMINANCE, GL_UNSIGNED_BYTE,      "L/ubyte", 1 },
   { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, "Z/uint", 4 },
   { GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, "Z+S/uint", 4 },
   { 0, 0, NULL, 0 }
};



/** Called from test harness/main */
void
PerfNextRound(void)
{
}


/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate, mbPerSec;
   int fmt, sz;

   /* loop over formats */
   for (fmt = 0; DstFormats[fmt].format; fmt++) {
      ReadFormat = DstFormats[fmt].format;
      ReadType = DstFormats[fmt].type;

      /* loop over sizes */
      for (sz = 0; Sizes[sz]; sz++) {
         int imgSize;

         ReadWidth = ReadHeight = Sizes[sz];
         imgSize = ReadWidth * ReadHeight * DstFormats[fmt].pixel_size;
         ReadBuffer = malloc(imgSize);

         if (ReadFormat == GL_DEPTH_STENCIL_EXT && !HaveDepthStencil) {
            rate = 0.0;
            mbPerSec = 0.0;
         }
         else {
            rate = PerfMeasureRate(ReadPixels);
            mbPerSec = rate * imgSize / (1024.0 * 1024.0);
         }

         perf_printf("glReadPixels(%d x %d, %s): %.1f images/sec, %.1f Mpixels/sec\n",
                     ReadWidth, ReadHeight,
                     DstFormats[fmt].name, rate, mbPerSec);

         free(ReadBuffer);
      }
   }

   exit(0);
}
