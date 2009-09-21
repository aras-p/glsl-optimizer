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

/* Copy data out of a large array to avoid caching effects:
 */
#define DATA_SIZE (16*1024*1024)

int WinWidth = 100, WinHeight = 100;

static GLuint VBO;

static GLsizei VBOSize = 0;
static GLsizei SubSize = 0;
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
   unsigned total = 0;
   unsigned src = 0;

   for (i = 0; i < count; i++) {
      glBufferDataARB(GL_ARRAY_BUFFER, VBOSize, VBOData + src, GL_STREAM_DRAW_ARB);
      glDrawArrays(GL_POINTS, 0, 1);

      /* Throw in an occasional flush to work around a driver crash:
       */
      total += VBOSize;
      if (total >= 16*1024*1024) {
         glFlush();
         total = 0;
      }

      src += VBOSize;
      src %= DATA_SIZE;
   }
   glFinish();
}


static void
UploadSubVBO(unsigned count)
{
   unsigned i;
   unsigned src = 0;

   for (i = 0; i < count; i++) {
      unsigned offset = (i * SubSize) % VBOSize;
      glBufferSubDataARB(GL_ARRAY_BUFFER, offset, SubSize, VBOData + src);

      if (DrawPoint) {
         glDrawArrays(GL_POINTS, offset / sizeof(Vertex0), 1);
      }

      src += SubSize;
      src %= DATA_SIZE;
   }
   glFinish();
}

/* Do multiple small SubData uploads, the a DrawArrays.  This may be a
 * fairer comparison to back-to-back BufferData calls:
 */
static void
BatchUploadSubVBO(unsigned count)
{
   unsigned i = 0, j;
   unsigned period = VBOSize / SubSize;
   unsigned src = 0;

   while (i < count) {
      for (j = 0; j < period && i < count; j++, i++) {
         unsigned offset = j * SubSize;
         glBufferSubDataARB(GL_ARRAY_BUFFER, offset, SubSize, VBOData + src);
      }

      glDrawArrays(GL_POINTS, 0, 1);

      src += SubSize;
      src %= DATA_SIZE;
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
   int i;

   VBOData = calloc(DATA_SIZE, 1);

   for (i = 0; i < DATA_SIZE / sizeof(Vertex0); i++) {
      memcpy(VBOData + i * sizeof(Vertex0), 
             Vertex0, 
             sizeof(Vertex0));
   }


   /* loop over whole/sub buffer upload */
   for (sub = 0; sub < 3; sub++) {

      if (sub == 2) {
         VBOSize = 1024 * 1024;

         glBufferDataARB(GL_ARRAY_BUFFER, VBOSize, VBOData, GL_STREAM_DRAW_ARB);

         for (sz = 0; Sizes[sz] < VBOSize; sz++) {
            SubSize = Sizes[sz];
            rate = PerfMeasureRate(UploadSubVBO);

            mbPerSec = rate * SubSize / (1024.0 * 1024.0);
         
            perf_printf("  glBufferSubDataARB(size = %d, VBOSize = %d): %.1f MB/sec\n",
                        SubSize, VBOSize, mbPerSec);
         }

         for (sz = 0; Sizes[sz] < VBOSize; sz++) {
            SubSize = Sizes[sz];
            rate = PerfMeasureRate(BatchUploadSubVBO);

            mbPerSec = rate * SubSize / (1024.0 * 1024.0);
         
            perf_printf("  glBufferSubDataARB(size = %d, VBOSize = %d), batched: %.1f MB/sec\n",
                        SubSize, VBOSize, mbPerSec);
         }
      }
      else {

         /* loop over VBO sizes */
         for (sz = 0; Sizes[sz]; sz++) {
            SubSize = VBOSize = Sizes[sz];

            if (sub == 1)
               rate = PerfMeasureRate(UploadSubVBO);
            else
               rate = PerfMeasureRate(UploadVBO);

            mbPerSec = rate * VBOSize / (1024.0 * 1024.0);

            perf_printf("  glBuffer%sDataARB(size = %d): %.1f MB/sec\n",
                        (sub ? "Sub" : ""), VBOSize, mbPerSec);
         }
      }
   }

   exit(0);
}
