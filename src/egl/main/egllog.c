/**
 * Logging facility for debug/info messages.
 * _EGL_FATAL messages are printed to stderr
 * The EGL_LOG_LEVEL var controls the output of other warning/info/debug msgs.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "egllog.h"
#include "eglmutex.h"

#define MAXSTRING 1000
#define FALLBACK_LOG_LEVEL _EGL_WARNING


static struct {
   _EGLMutex mutex;

   EGLBoolean initialized;
   EGLint level;
   _EGLLogProc logger;
   EGLint num_messages;
} logging = {
   _EGL_MUTEX_INITIALIZER,
   EGL_FALSE,
   FALLBACK_LOG_LEVEL,
   NULL,
   0
};

static const char *level_strings[] = {
   /* the order is important */
   "fatal",
   "warning",
   "info",
   "debug",
   NULL
};


/**
 * Set the function to be called when there is a message to log.
 * Note that the function will be called with an internal lock held.
 * Recursive logging is not allowed.
 */
void
_eglSetLogProc(_EGLLogProc logger)
{
   EGLint num_messages = 0;

   _eglLockMutex(&logging.mutex);

   if (logging.logger != logger) {
      logging.logger = logger;

      num_messages = logging.num_messages;
      logging.num_messages = 0;
   }

   _eglUnlockMutex(&logging.mutex);

   if (num_messages)
      _eglLog(_EGL_DEBUG,
              "New logger installed. "
              "Messages before the new logger might not be available.");
}


/**
 * Set the log reporting level.
 */
void
_eglSetLogLevel(EGLint level)
{
   switch (level) {
   case _EGL_FATAL:
   case _EGL_WARNING:
   case _EGL_INFO:
   case _EGL_DEBUG:
      _eglLockMutex(&logging.mutex);
      logging.level = level;
      _eglUnlockMutex(&logging.mutex);
      break;
   default:
      break;
   }
}


/**
 * The default logger.  It prints the message to stderr.
 */
static void
_eglDefaultLogger(EGLint level, const char *msg)
{
   fprintf(stderr, "libEGL %s: %s\n", level_strings[level], msg);
}


/**
 * Initialize the logging facility.
 */
static void
_eglInitLogger(void)
{
   const char *log_env;
   EGLint i, level = -1;

   if (logging.initialized)
      return;

   log_env = getenv("EGL_LOG_LEVEL");
   if (log_env) {
      for (i = 0; level_strings[i]; i++) {
         if (strcasecmp(log_env, level_strings[i]) == 0) {
            level = i;
            break;
         }
      }
   }
   else {
      level = FALLBACK_LOG_LEVEL;
   }

   logging.logger = _eglDefaultLogger;
   logging.level = (level >= 0) ? level : FALLBACK_LOG_LEVEL;
   logging.initialized = EGL_TRUE;

   /* it is fine to call _eglLog now */
   if (log_env && level < 0) {
      _eglLog(_EGL_WARNING,
              "Unrecognized EGL_LOG_LEVEL environment variable value. "
              "Expected one of \"fatal\", \"warning\", \"info\", \"debug\". "
              "Got \"%s\". Falling back to \"%s\".",
              log_env, level_strings[FALLBACK_LOG_LEVEL]);
   }
}


/**
 * Log a message with message logger.
 * \param level one of _EGL_FATAL, _EGL_WARNING, _EGL_INFO, _EGL_DEBUG.
 */
void
_eglLog(EGLint level, const char *fmtStr, ...)
{
   va_list args;
   char msg[MAXSTRING];

   /* one-time initialization; a little race here is fine */
   if (!logging.initialized)
      _eglInitLogger();
   if (level > logging.level || level < 0)
      return;

   _eglLockMutex(&logging.mutex);

   if (logging.logger) {
      va_start(args, fmtStr);
      vsnprintf(msg, MAXSTRING, fmtStr, args);
      va_end(args);

      logging.logger(level, msg);
      logging.num_messages++;
   }

   _eglUnlockMutex(&logging.mutex);

   if (level == _EGL_FATAL)
      exit(1); /* or abort()? */
}
