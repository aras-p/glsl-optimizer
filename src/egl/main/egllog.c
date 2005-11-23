/**
 * Logging facility for debug/info messages.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "egllog.h"

#define MAXSTRING 1000


/* XXX init this with an env var or something */
static EGLint ReportingLevel = _EGL_DEBUG;


/**
 * Log a message to stderr.
 * \param level one of _EGL_FATAL, _EGL_WARNING, _EGL_INFO, _EGL_DEBUG.
 */
void
_eglLog(EGLint level, const char *fmtStr, ...)
{
   va_list args;
   char msg[MAXSTRING];
   const char *levelStr;

   if (level <= ReportingLevel) {
      switch (level) {
      case _EGL_FATAL:
         levelStr = "Fatal";
         break;
      case _EGL_WARNING:
         levelStr = "Warning";
         break;
      case _EGL_INFO:
         levelStr = "Info";
         break;
      case _EGL_DEBUG:
         levelStr = "Debug";
         break;
      default:
         levelStr = "";
      }

      va_start(args, fmtStr);
      vsnprintf(msg, MAXSTRING, fmtStr, args);
      va_end(args);

      fprintf(stderr, "EGL %s: %s\n", levelStr, msg);

      if (level == _EGL_FATAL) {
         exit(1); /* or abort()? */
      }
   }
}
