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
 * Measure drawing overhead
 *
 * This is the first in a series of simple performance benchmarks.
 * The code in this file should be as simple as possible to make it
 * easily portable to other APIs.
 *
 * All the window-system stuff should be contained in glmain.c (or TBDmain.c).
 *
 * Brian Paul
 * 15 Sep 2009
 */

#include "glmain.h"
#include "common.h"


int WinWidth = 100, WinHeight = 100;

static GLuint VBO;

struct vertex
{
   GLfloat x, y;
};

static const struct vertex vertices[4] = {
   { -1.0, -1.0 },
   {  1.0, -1.0 },
   {  1.0,  1.0 },
   { -1.0,  1.0 }
};


/** Called from test harness/main */
void
PerfInit(void)
{
   /* setup VBO w/ vertex data */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                   sizeof(vertices), vertices, GL_STATIC_DRAW_ARB);
   glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), (void *) 0);
   glEnableClientState(GL_VERTEX_ARRAY);

   /* misc GL state */
   glAlphaFunc(GL_ALWAYS, 0.0);
}


static void
DrawNoStateChange(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      glDrawArrays(GL_POINTS, 0, 4);
   }
   glFinish();
}


static void
DrawNopStateChange(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      glDisable(GL_ALPHA_TEST);
      glDrawArrays(GL_POINTS, 0, 4);
   }
   glFinish();
}


static void
DrawStateChange(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      if (i & 1)
         glEnable(GL_TEXTURE_GEN_S);
      else
         glDisable(GL_TEXTURE_GEN_S);
      glDrawArrays(GL_POINTS, 0, 4);
   }
   glFinish();
}

void
PerfNextRound(void)
{
}

/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate0, rate1, rate2, overhead;

   rate0 = PerfMeasureRate(DrawNoStateChange);
   perf_printf("   Draw only: %s draws/second\n", 
               PerfHumanFloat(rate0));
   
   rate1 = PerfMeasureRate(DrawNopStateChange);
   overhead = 1000.0 * (1.0 / rate1 - 1.0 / rate0);
   perf_printf("   Draw w/ nop state change: %s draws/sec (overhead: %f ms/draw)\n",
               PerfHumanFloat(rate1), overhead);

   rate2 = PerfMeasureRate(DrawStateChange);
   overhead = 1000.0 * (1.0 / rate2 - 1.0 / rate0);
   perf_printf("   Draw w/ state change: %s draws/sec (overhead: %f ms/draw)\n",
               PerfHumanFloat(rate2), overhead);

   exit(0);
}

