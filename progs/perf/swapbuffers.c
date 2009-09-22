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
 * Measure SwapBuffers.
 *
 * Keith Whitwell
 * 22 Sep 2009
 */

#include "glmain.h"
#include "common.h"


int WinWidth = 100, WinHeight = 100;
int real_WinWidth, real_WinHeight; /* don't know whats going on here */

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
SwapNaked(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      PerfSwapBuffers();
   }
}


static void
SwapClear(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      glClear(GL_COLOR_BUFFER_BIT);
      PerfSwapBuffers();
   }
}

static void
SwapClearPoint(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawArrays(GL_POINTS, 0, 4);
      PerfSwapBuffers();
   }
}


static const struct {
   unsigned w;
   unsigned h;
} sizes[] = {
   { 320, 240 },
   { 640, 480 },
   { 1024, 768 },
   { 1200, 1024 },
   { 1600, 1200 }
};

void
PerfNextRound(void)
{
   static unsigned i;
   
   if (i < sizeof(sizes) / sizeof(sizes[0]) &&
      PerfReshapeWindow( sizes[i].w, sizes[i].h )) 
   {
      perf_printf("Reshape %dx%d\n", sizes[i].w, sizes[i].h);
      real_WinWidth = sizes[i].w;
      real_WinHeight = sizes[i].h;
      i++;
   }
   else {
      exit(0);
   }
}




/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate0;

   rate0 = PerfMeasureRate(SwapNaked);
   perf_printf("   Swapbuffers      %dx%d: %s swaps/second", 
               real_WinWidth, real_WinHeight,
               PerfHumanFloat(rate0));
   perf_printf(" %s pixels/second\n",
               PerfHumanFloat(rate0 * real_WinWidth * real_WinHeight));



   rate0 = PerfMeasureRate(SwapClear);
   perf_printf("   Swap/Clear       %dx%d: %s swaps/second", 
               real_WinWidth, real_WinHeight,
               PerfHumanFloat(rate0));
   perf_printf(" %s pixels/second\n",
               PerfHumanFloat(rate0 * real_WinWidth * real_WinHeight));


   rate0 = PerfMeasureRate(SwapClearPoint);
   perf_printf("   Swap/Clear/Draw  %dx%d: %s swaps/second", 
               real_WinWidth, real_WinHeight,
               PerfHumanFloat(rate0));
   perf_printf(" %s pixels/second\n",
               PerfHumanFloat(rate0 * real_WinWidth * real_WinHeight));
}

