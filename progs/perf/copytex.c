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
 * Measure glCopyTex[Sub]Image() rate.
 * Create a large, off-screen framebuffer object for rendering and
 * copying the texture data from it since we can't make really large
 * on-screen windows.
 *
 * Brian Paul
 * 22 Sep 2009
 */

#include <string.h>
#include "glmain.h"
#include "common.h"

int WinWidth = 100, WinHeight = 100;

static GLuint VBO, FBO, RBO, Tex;

const GLsizei MinSize = 16, MaxSize = 4096;
static GLsizei TexSize;

static const GLboolean DrawPoint = GL_TRUE;
static const GLboolean TexSubImage4 = GL_FALSE;

struct vertex
{
   GLfloat x, y, s, t;
};

static const struct vertex vertices[1] = {
   { 0.0, 0.0, 0.5, 0.5 },
};

#define VOFFSET(F) ((void *) offsetof(struct vertex, F))


/** Called from test harness/main */
void
PerfInit(void)
{
   const GLenum filter = GL_LINEAR;
   GLenum stat;

   if (!PerfExtensionSupported("GL_EXT_framebuffer_object")) {
      perf_printf("copytex: GL_EXT_framebuffer_object not supported\n");
      exit(0);
   }

   /* setup VBO */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(vertices),
                   vertices, GL_STATIC_DRAW_ARB);

   glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(x));
   glTexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(s));
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   /* setup texture */
   glGenTextures(1, &Tex);
   glBindTexture(GL_TEXTURE_2D, Tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
   glEnable(GL_TEXTURE_2D);

   /* setup rbo */
   glGenRenderbuffersEXT(1, &RBO);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, RBO);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, MaxSize, MaxSize);

   /* setup fbo */
   glGenFramebuffersEXT(1, &FBO);
   glBindFramebufferEXT(GL_FRAMEBUFFER, FBO);
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                GL_COLOR_ATTACHMENT0_EXT,
                                GL_RENDERBUFFER_EXT, RBO);

   stat = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (stat != GL_FRAMEBUFFER_COMPLETE_EXT) {
      perf_printf("fboswitch: Error: incomplete FBO!\n");
      exit(1);
   }

   /* clear the FBO */
   glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
   glViewport(0, 0, MaxSize, MaxSize);
   glClear(GL_COLOR_BUFFER_BIT);
}


static void
CopyTexImage(unsigned count)
{
   unsigned i;
   for (i = 1; i < count; i++) {
      /* draw something */
      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);

      /* copy whole texture */
      glCopyTexImage2D(GL_TEXTURE_2D, 0,
                       GL_RGBA, 0, 0, TexSize, TexSize, 0);
   }
   glFinish();
}


static void
CopyTexSubImage(unsigned count)
{
   unsigned i;
   for (i = 1; i < count; i++) {
      /* draw something */
      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);

      /* copy sub texture */
      if (TexSubImage4) {
         /* four sub-copies */
         GLsizei half = TexSize / 2;
         /* lower-left */
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                             0, 0, 0, 0, half, half);
         /* lower-right */
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                             half, 0, half, 0, half, half);
         /* upper-left */
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                             0, half, 0, half, half, half);
         /* upper-right */
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                             half, half, half, half, half, half);
      }
      else {
         /* one big copy */
         glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                             0, 0, 0, 0, TexSize, TexSize);
      }
   }
   glFinish();
}


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
   GLint sub, maxTexSize;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);

   /* loop over whole/sub tex copy */
   for (sub = 0; sub < 2; sub++) {

      /* loop over texture sizes */
      for (TexSize = MinSize; TexSize <= MaxSize; TexSize *= 4) {

         if (TexSize <= maxTexSize) {
            GLint bytesPerImage = 4 * TexSize * TexSize;

            if (sub == 0)
               rate = PerfMeasureRate(CopyTexImage);
            else {
               /* setup empty dest texture */
               glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                            TexSize, TexSize, 0,
                            GL_RGBA, GL_UNSIGNED_BYTE, NULL);
               rate = PerfMeasureRate(CopyTexSubImage);
            }

            mbPerSec = rate * bytesPerImage / (1024.0 * 1024.0);
         }
         else {
            rate = 0.0;
            mbPerSec = 0.0;
         }

         perf_printf("  glCopyTex%sImage(%d x %d): %.1f copies/sec, %.1f Mpixels/sec\n",
                     (sub ? "Sub" : ""), TexSize, TexSize, rate, mbPerSec);
      }
   }

   exit(0);
}
