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
/* $XFree86: xc/lib/GL/mesa/src/drv/common/hwlog.h,v 1.5 2001/10/31 23:22:57 tsi Exp $ */
 
/* Usage:
 * - use mgaError for error messages. Always write to X error and log file.
 * - use mgaMsg for debugging. Can be disabled by undefining MGA_LOG_ENABLED.
 */
 
#ifndef HWLOG_INC
#define HWLOG_INC


#include <stdarg.h>
#include <stdio.h>

#define DBG_LEVEL_BASE          1
#define DBG_LEVEL_VERBOSE       10
#define DBG_LEVEL_ENTEREXIT     20

typedef struct
{
  FILE *file;
  int   level;
  unsigned int timeTemp;
  char *prefix;
} hwlog_t;

extern hwlog_t hwlog;


#ifdef HW_LOG_ENABLED

/* open and close log file. */
int  hwOpenLog(const char *filename, char *prefix);
void hwCloseLog(void);

/* return 1 if log file is succesfully opened */
int  hwIsLogReady(void);

/* set current log level to 'level'. Messages with level less than or equal
   the current log level will be written to the log file. */
void hwSetLogLevel(int level);
int  hwGetLogLevel(void);

/* hwLog and hwLogv write a message to the log file.	*/
/* do not call these directly, use hwMsg() instead	*/
void hwLog(int level, const char *format, ...);
void hwLogv(int level, const char *format, va_list ap);

int usec( void );

/* hwMsg writes a message to the log file or to the standard X error file. */
void hwMsg(int level, const char *format, ...);


#else


static __inline__ int hwOpenLog(const char *f, char *prefix) { hwlog.prefix=prefix; return -1; }
#define hwIsLogReady() (0)
#define hwGetLogLevel() (-1)
#define hwLogLevel(n) (0)
#define hwLog()
#define hwMsg()

#define hwCloseLog()
#define hwSetLogLevel(x)
#define hwLogv(l,f,a)


#endif

void hwError(const char *format, ...);


#endif
