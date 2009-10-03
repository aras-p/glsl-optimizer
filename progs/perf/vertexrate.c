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
 * Measure simple vertex processing rate via:
 *  - immediate mode
 *  - vertex arrays
 *  - VBO vertex arrays
 *  - glDrawElements
 *  - VBO glDrawElements
 *  - glDrawRangeElements
 *  - VBO glDrawRangeElements
 *
 * Brian Paul
 * 16 Sep 2009
 */

#include <assert.h>
#include <string.h>
#include "glmain.h"
#include "common.h"


#define MAX_VERTS (100 * 100)

/** glVertex2/3/4 size */
#define VERT_SIZE 4

int WinWidth = 500, WinHeight = 500;

static GLuint VertexBO, ElementBO;

static unsigned NumVerts = MAX_VERTS;
static unsigned VertBytes = VERT_SIZE * sizeof(float);
static float *VertexData = NULL;

static unsigned NumElements = MAX_VERTS;
static GLuint *Elements = NULL;


/**
 * Load VertexData buffer with a 2-D grid of points in the range [-1,1]^2.
 */
static void
InitializeVertexData(void)
{
   unsigned i;
   float x = -1.0, y = -1.0;
   float dx = 2.0 / 100;
   float dy = 2.0 / 100;

   VertexData = (float *) malloc(NumVerts * VertBytes);

   for (i = 0; i < NumVerts; i++) {
      VertexData[i * VERT_SIZE + 0] = x;
      VertexData[i * VERT_SIZE + 1] = y;
      VertexData[i * VERT_SIZE + 2] = 0.0;
      VertexData[i * VERT_SIZE + 3] = 1.0;
      x += dx;
      if (x > 1.0) {
         x = -1.0;
         y += dy;
      }
   }

   Elements = (GLuint *) malloc(NumVerts * sizeof(GLuint));

   for (i = 0; i < NumVerts; i++) {
      Elements[i] = NumVerts - i - 1;
   }
}


/** Called from test harness/main */
void
PerfInit(void)
{
   InitializeVertexData();

   /* setup VertexBO */
   glGenBuffersARB(1, &VertexBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VertexBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                   NumVerts * VertBytes, VertexData, GL_STATIC_DRAW_ARB);
   glEnableClientState(GL_VERTEX_ARRAY);

   /* setup ElementBO */
   glGenBuffersARB(1, &ElementBO);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ElementBO);
   glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
                   NumElements * sizeof(GLuint), Elements, GL_STATIC_DRAW_ARB);
}


static void
DrawImmediate(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   for (i = 0; i < count; i++) {
      unsigned j;
      glBegin(GL_POINTS);
      for (j = 0; j < NumVerts; j++) {
#if VERT_SIZE == 4
         glVertex4fv(VertexData + j * 4);
#elif VERT_SIZE == 3
         glVertex3fv(VertexData + j * 3);
#elif VERT_SIZE == 2
         glVertex2fv(VertexData + j * 2);
#else
         abort();
#endif
      }
      glEnd();
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawArraysMem(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, VertexData);
   for (i = 0; i < count; i++) {
      glDrawArrays(GL_POINTS, 0, NumVerts);
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawArraysVBO(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ARRAY_BUFFER, VertexBO);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, (void *) 0);
   for (i = 0; i < count; i++) {
      glDrawArrays(GL_POINTS, 0, NumVerts);
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawElementsMem(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, VertexData);
   for (i = 0; i < count; i++) {
      glDrawElements(GL_POINTS, NumVerts, GL_UNSIGNED_INT, Elements);
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawElementsBO(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, ElementBO);
   glBindBufferARB(GL_ARRAY_BUFFER, VertexBO);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, (void *) 0);
   for (i = 0; i < count; i++) {
      glDrawElements(GL_POINTS, NumVerts, GL_UNSIGNED_INT, (void *) 0);
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawRangeElementsMem(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, VertexData);
   for (i = 0; i < count; i++) {
      glDrawRangeElements(GL_POINTS, 0, NumVerts - 1,
                          NumVerts, GL_UNSIGNED_INT, Elements);
   }
   glFinish();
   PerfSwapBuffers();
}


static void
DrawRangeElementsBO(unsigned count)
{
   unsigned i;
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, ElementBO);
   glBindBufferARB(GL_ARRAY_BUFFER, VertexBO);
   glVertexPointer(VERT_SIZE, GL_FLOAT, VertBytes, (void *) 0);
   for (i = 0; i < count; i++) {
      glDrawRangeElements(GL_POINTS, 0, NumVerts - 1,
                          NumVerts, GL_UNSIGNED_INT, (void *) 0);
   }
   glFinish();
   PerfSwapBuffers();
}

void
PerfNextRound(void)
{
}


/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   perf_printf("Vertex rate (%d x Vertex%df)\n", NumVerts, VERT_SIZE);

   rate = PerfMeasureRate(DrawImmediate);
   rate *= NumVerts;
   perf_printf("  Immediate mode: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawArraysMem);
   rate *= NumVerts;
   perf_printf("  glDrawArrays: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawArraysVBO);
   rate *= NumVerts;
   perf_printf("  VBO glDrawArrays: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawElementsMem);
   rate *= NumVerts;
   perf_printf("  glDrawElements: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawElementsBO);
   rate *= NumVerts;
   perf_printf("  VBO glDrawElements: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawRangeElementsMem);
   rate *= NumVerts;
   perf_printf("  glDrawRangeElements: %s verts/sec\n", PerfHumanFloat(rate));

   rate = PerfMeasureRate(DrawRangeElementsBO);
   rate *= NumVerts;
   perf_printf("  VBO glDrawRangeElements: %s verts/sec\n", PerfHumanFloat(rate));

   exit(0);
}
