/*
 * GLX Hardware Device Driver common code 
 * 
 * Based on the original MGA G200 driver (c) 1999 Wittawat Yamwong
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
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/common/hwlog.c,v 1.3 2001/08/18 02:51:03 dawes Exp $ */
 
#include "hwlog.h"
hwlog_t hwlog = { 0,0,0, "[???] "};


/* Should be shared, but is this a good place for it?
 */
#include <sys/time.h>
#include <stdarg.h>


int usec( void ) 
{
   struct timeval tv;
   struct timezone tz;
   
   gettimeofday( &tv, &tz );
   
   return (tv.tv_sec & 2047) * 1000000 + tv.tv_usec;
}


#ifdef HW_LOG_ENABLED
int hwOpenLog(const char *filename, char *prefix)
{
  hwCloseLog();
  hwSetLogLevel(0);
  hwlog.prefix=prefix;
  if (!filename)
    return -1;
  if ((hwlog.file = fopen(filename,"w")) == NULL)
      return -1;
  return 0;
}

void hwCloseLog()
{
  if (hwlog.file) {
    fclose(hwlog.file);
    hwlog.file = NULL;
  }
}

int hwIsLogReady()
{
  return (hwlog.file != NULL);
}

void hwSetLogLevel(int level)
{
  hwlog.level = level;
}

int hwGetLogLevel()
{
  return hwlog.level;
}

void hwLog(int level, const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  hwLogv(level,format,ap);
  va_end(ap);
}

void hwLogv(int l, const char *format, va_list ap)
{
  if (hwlog.file && (l <= hwlog.level)) {
    vfprintf(hwlog.file,format,ap);
    fflush(hwlog.file);
  }
}

void hwMsg(int l, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  if (l <= hwlog.level) {
    if (hwIsLogReady()) {
      int t = usec();

      hwLog(l, "%6i:", t - hwlog.timeTemp);
      hwlog.timeTemp = t;
      hwLogv(l, format, ap);
    } else {
      fprintf(stderr, hwlog.prefix);
      vfprintf(stderr, format, ap);
    }
  }

  va_end(ap);
}

#else /* ifdef HW_LOG_ENABLED */

int hwlogdummy()
{
  return 0;
}

#endif

void hwError(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  fprintf(stderr, hwlog.prefix);
  vfprintf(stderr, format, ap);
  hwLogv(0, format, ap);

  va_end(ap);
}
