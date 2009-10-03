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
static GLubyte *VBOData = NULL;  /* array[DATA_SIZE] */

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


/* Do multiple small SubData uploads, then call DrawArrays.  This may be a
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


/**
 * Test the sequence:
 *    create/load VBO
 *    draw
 *    destroy VBO
 */
static void
CreateDrawDestroyVBO(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      GLuint vbo;
      /* create/load */
      glGenBuffersARB(1, &vbo);
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
      glBufferDataARB(GL_ARRAY_BUFFER, VBOSize, VBOData, GL_STREAM_DRAW_ARB);
      /* draw */
      glVertexPointer(2, GL_FLOAT, sizeof(Vertex0), (void *) 0);
      glDrawArrays(GL_POINTS, 0, 1);
      /* destroy */
      glDeleteBuffersARB(1, &vbo);
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

void
PerfNextRound(void)
{
}

/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate, mbPerSec;
   int i, sz;

   /* Load VBOData buffer with duplicated Vertex0.
    */
   VBOData = calloc(DATA_SIZE, 1);

   for (i = 0; i < DATA_SIZE / sizeof(Vertex0); i++) {
      memcpy(VBOData + i * sizeof(Vertex0), 
             Vertex0, 
             sizeof(Vertex0));
   }

   /* glBufferDataARB()
    */
   for (sz = 0; Sizes[sz]; sz++) {
      SubSize = VBOSize = Sizes[sz];
      rate = PerfMeasureRate(UploadVBO);
      mbPerSec = rate * VBOSize / (1024.0 * 1024.0);
      perf_printf("  glBufferDataARB(size = %d): %.1f MB/sec\n",
                  VBOSize, mbPerSec);
   }

   /* glBufferSubDataARB()
    */
   for (sz = 0; Sizes[sz]; sz++) {
      SubSize = VBOSize = Sizes[sz];
      rate = PerfMeasureRate(UploadSubVBO);
      mbPerSec = rate * VBOSize / (1024.0 * 1024.0);
      perf_printf("  glBufferSubDataARB(size = %d): %.1f MB/sec\n",
                  VBOSize, mbPerSec);
   }

   /* Batch upload
    */
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

   /* Create/Draw/Destroy
    */
   for (sz = 0; Sizes[sz]; sz++) {
      SubSize = VBOSize = Sizes[sz];
      rate = PerfMeasureRate(CreateDrawDestroyVBO);
      mbPerSec = rate * VBOSize / (1024.0 * 1024.0);
      perf_printf("  VBO Create/Draw/Destroy(size = %d): %.1f MB/sec, %.1f draws/sec\n",
                  VBOSize, mbPerSec, rate);
   }

   exit(0);
}
