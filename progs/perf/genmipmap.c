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
 * Measure glGenerateMipmap() speed.
 *
 * Brian Paul
 * 24 Sep 2009
 */

#include <string.h>
#include <stdio.h>
#include "glmain.h"
#include "common.h"


int WinWidth = 100, WinHeight = 100;

static GLboolean DrawPoint = GL_TRUE;
static GLuint VBO;
static GLuint TexObj = 0;
static GLint BaseLevel, MaxLevel;

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
   if (!PerfExtensionSupported("GL_ARB_framebuffer_object")) {
      printf("Sorry, this test requires GL_ARB_framebuffer_object\n");
      exit(1);
   }

   /* setup VBO w/ vertex data */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                   sizeof(vertices), vertices, GL_STATIC_DRAW_ARB);
   glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(x));
   glTexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(s));
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glGenTextures(1, &TexObj);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glEnable(GL_TEXTURE_2D);
}


static void
GenMipmap(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      GLubyte texel[4];
      texel[0] = texel[1] = texel[2] = texel[3] = i & 0xff;
      /* dirty the base image */
      glTexSubImage2D(GL_TEXTURE_2D, BaseLevel,
                      0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texel);
      glGenerateMipmap(GL_TEXTURE_2D);
      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
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
   const GLint NumLevels = 12;
   const GLint TexWidth = 2048, TexHeight = 2048;
   GLubyte *img;
   double rate;

   /* Make 2K x 2K texture */
   img = (GLubyte *) malloc(TexWidth * TexHeight * 4);
   memset(img, 128, TexWidth * TexHeight * 4);
   glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGBA, TexWidth, TexHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, img);
   free(img);

   perf_printf("Texture level[0] size: %d x %d, %d levels\n",
               TexWidth, TexHeight, NumLevels);

   /* loop over base levels 0, 2, 4 */
   for (BaseLevel = 0; BaseLevel <= 4; BaseLevel += 2) {

      /* loop over max level */
      for (MaxLevel = NumLevels; MaxLevel > BaseLevel; MaxLevel--) {

         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, BaseLevel);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLevel);

         rate = PerfMeasureRate(GenMipmap);

         perf_printf("   glGenerateMipmap(levels %d..%d): %.2f gens/sec\n",
                     BaseLevel + 1, MaxLevel, rate);
      }
   }

   exit(0);
}
