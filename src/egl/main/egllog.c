/**
 * Logging facility for debug/info messages.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "egllog.h"

#define MAXSTRING 1000
#define FALLBACK_LOG_LEVEL      _EGL_DEBUG
#define FALLBACK_LOG_LEVEL_STR  "debug"

static EGLint ReportingLevel = -1;


static void
log_level_initialize (void)
{
   char *log_env = getenv ("EGL_LOG_LEVEL");

   if (log_env == NULL) {
      ReportingLevel = FALLBACK_LOG_LEVEL;
   }
   else if (strcasecmp (log_env, "fatal") == 0) {
      ReportingLevel = _EGL_FATAL;
   }
   else if (strcasecmp (log_env, "warning") == 0) {
      ReportingLevel = _EGL_WARNING;
   }
   else if (strcasecmp (log_env, "info") == 0) {
      ReportingLevel = _EGL_INFO;
   }
   else if (strcasecmp (log_env, "debug") == 0) {
      ReportingLevel = _EGL_DEBUG;
   }
   else {
      fprintf (stderr, "Unrecognized EGL_LOG_LEVEL environment variable value. "
               "Expected one of \"fatal\", \"warning\", \"info\", \"debug\". "
               "Got \"%s\". Falling back to \"%s\".\n",
               log_env, FALLBACK_LOG_LEVEL_STR);
      ReportingLevel = FALLBACK_LOG_LEVEL;
   }
}


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
   static int log_level_initialized = 0;

   if (!log_level_initialized) {
      log_level_initialize ();
      log_level_initialized = 1;
   }

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
