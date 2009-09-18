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
 * Measure VBO upload speed.
 * That is, measure glBufferDataARB() and glBufferSubDataARB().
 *
 * Brian Paul
 * 16 Sep 2009
 */

#include <string.h>
#include "glmain.h"
#include "common.h"


int WinWidth = 100, WinHeight = 100;

static GLuint VBO;

static GLsizei VBOSize = 0;
static GLubyte *VBOData = NULL;

static const GLboolean DrawPoint = GL_TRUE;
static const GLboolean BufferSubDataInHalves = GL_TRUE;

static const GLfloat Vertex0[2] = { 0.0, 0.0 };


/** Called from test harness/main */
void
PerfInit(void)
{
   /* setup VBO */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glVertexPointer(2, GL_FLOAT, sizeof(Vertex0), (void *) 0);
   glEnableClientState(GL_VERTEX_ARRAY);
}


static void
UploadVBO(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      glBufferDataARB(GL_ARRAY_BUFFER, VBOSize, VBOData, GL_STREAM_DRAW_ARB);

      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
   }
   glFinish();
}


static void
UploadSubVBO(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      if (BufferSubDataInHalves) {
         GLsizei half = VBOSize / 2;
         glBufferSubDataARB(GL_ARRAY_BUFFER, 0, half, VBOData);
         glBufferSubDataARB(GL_ARRAY_BUFFER, half, half, VBOData + half);
      }
      else {
         glBufferSubDataARB(GL_ARRAY_BUFFER, 0, VBOSize, VBOData);
      }

      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
   }
   glFinish();
}


static const GLsizei Sizes[] = {
   64,
   1024,
   16*1024,
   256*1024,
   1024*1024,
   16*1024*1024,
   0 /* end of list */
};


/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate, mbPerSec;
   int sub, sz;

   /* loop over whole/sub buffer upload */
   for (sub = 0; sub < 2; sub++) {

      /* loop over VBO sizes */
      for (sz = 0; Sizes[sz]; sz++) {
         VBOSize = Sizes[sz];

         VBOData = malloc(VBOSize);
         memcpy(VBOData, Vertex0, sizeof(Vertex0));

         if (sub)
            rate = PerfMeasureRate(UploadSubVBO);
         else
            rate = PerfMeasureRate(UploadVBO);

         mbPerSec = rate * VBOSize / (1024.0 * 1024.0);

         printf("  glBuffer%sDataARB(size = %d): %.1f MB/sec\n",
                (sub ? "Sub" : ""), VBOSize, mbPerSec);

         free(VBOData);
      }
   }

   exit(0);
}
