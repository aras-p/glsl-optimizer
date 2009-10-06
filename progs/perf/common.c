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
 * Common perf code.  This should be re-usable with other APIs.
 */

#include "common.h"
#include "glmain.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif


/* Need to add a fflush windows console with mingw, otherwise nothing
 * shows up until program exit.  May want to add logging here.
 */
void
perf_printf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);

   fflush(stdout);
   vfprintf(stdout, format, ap);
   fflush(stdout);

   va_end(ap);
}



/**
 * Run function 'f' for enough iterations to reach a steady state.
 * Return the rate (iterations/second).
 */
double
PerfMeasureRate(PerfRateFunc f)
{
   const double minDuration = 1.0;
   double rate = 0.0, prevRate = 0.0;
   unsigned subiters;

   /* Compute initial number of iterations to try.
    * If the test function is pretty slow this helps to avoid
    * extraordarily long run times.
    */
   subiters = 2;
   {
      const double t0 = PerfGetTime();
      double t1;
      do {
         f(subiters); /* call the rendering function */
         t1 = PerfGetTime();
         subiters *= 2;
      } while (t1 - t0 < 0.1 * minDuration);
   }
   /*perf_printf("initial subIters = %u\n", subiters);*/

   while (1) {
      const double t0 = PerfGetTime();
      unsigned iters = 0;
      double t1;

      do {
         f(subiters); /* call the rendering function */
         t1 = PerfGetTime();
         iters += subiters;
      } while (t1 - t0 < minDuration);

      rate = iters / (t1 - t0);

      if (0)
         perf_printf("prevRate %f  rate  %f  ratio %f  iters %u\n",
                prevRate, rate, rate/prevRate, iters);

      /* Try and speed the search up by skipping a few steps:
       */
      if (rate > prevRate * 1.6)
         subiters *= 8;
      else if (rate > prevRate * 1.2)
         subiters *= 4;
      else if (rate > prevRate * 1.05)
         subiters *= 2;
      else
         break;

      prevRate = rate;
   }

   if (0)
      perf_printf("%s returning iters %u  rate %f\n", __FUNCTION__, subiters, rate);
   return rate;
}


/* Note static buffer, can only use once per printf.
 */
const char *
PerfHumanFloat( double d )
{
   static char buf[80];

   if (d > 1000000000.0)
      snprintf(buf, sizeof(buf), "%.1f billion", d / 1000000000.0);
   else if (d > 1000000.0)
      snprintf(buf, sizeof(buf), "%.1f million", d / 1000000.0);
   else if (d > 1000.0)
      snprintf(buf, sizeof(buf), "%.1f thousand", d / 1000.0);
   else
      snprintf(buf, sizeof(buf), "%.1f", d);

   return buf;
}
