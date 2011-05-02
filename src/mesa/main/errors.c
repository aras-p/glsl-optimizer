/**
 * \file errors.c
 * Mesa debugging and error handling functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "errors.h"

#include "imports.h"
#include "context.h"
#include "dispatch.h"
#include "mtypes.h"
#include "version.h"


#define MAXSTRING MAX_DEBUG_MESSAGE_LENGTH


static char out_of_memory[] = "Debugging error: out of memory";

/**
 * 'buf' is not necessarily a null-terminated string. When logging, copy
 * 'len' characters from it, store them in a new, null-terminated string,
 * and remember the number of bytes used by that string, *including*
 * the null terminator this time.
 */
static void
_mesa_log_msg(struct gl_context *ctx, GLenum source, GLenum type,
              GLuint id, GLenum severity, GLint len, const char *buf)
{
   GLint nextEmpty;
   struct gl_debug_msg *emptySlot;

   assert(len >= 0 && len < MAX_DEBUG_MESSAGE_LENGTH);

   if (ctx->Debug.Callback) {
      ctx->Debug.Callback(source, type, id, severity,
                          len, buf, ctx->Debug.CallbackData);
      return;
   }

   if (ctx->Debug.NumMessages == MAX_DEBUG_LOGGED_MESSAGES)
      return;

   nextEmpty = (ctx->Debug.NextMsg + ctx->Debug.NumMessages)
                          % MAX_DEBUG_LOGGED_MESSAGES;
   emptySlot = &ctx->Debug.Log[nextEmpty];

   assert(!emptySlot->message && !emptySlot->length);

   emptySlot->message = MALLOC(len+1);
   if (emptySlot->message) {
      (void) strncpy(emptySlot->message, buf, (size_t)len);
      emptySlot->message[len] = '\0';

      emptySlot->length = len+1;
      emptySlot->source = source;
      emptySlot->type = type;
      emptySlot->id = id;
      emptySlot->severity = severity;
   } else {
      /* malloc failed! */
      emptySlot->message = out_of_memory;
      emptySlot->length = strlen(out_of_memory)+1;
      emptySlot->source = GL_DEBUG_SOURCE_OTHER_ARB;
      emptySlot->type = GL_DEBUG_TYPE_ERROR_ARB;
      emptySlot->id = OTHER_ERROR_OUT_OF_MEMORY;
      emptySlot->severity = GL_DEBUG_SEVERITY_HIGH_ARB;
   }

   if (ctx->Debug.NumMessages == 0)
      ctx->Debug.NextMsgLength = ctx->Debug.Log[ctx->Debug.NextMsg].length;

   ctx->Debug.NumMessages++;
}

/**
 * Pop the oldest debug message out of the log.
 * Writes the message string, including the null terminator, into 'buf',
 * using up to 'bufSize' bytes. If 'bufSize' is too small, or
 * if 'buf' is NULL, nothing is written.
 *
 * Returns the number of bytes written on success, or when 'buf' is NULL,
 * the number that would have been written. A return value of 0
 * indicates failure.
 */
static GLsizei
_mesa_get_msg(struct gl_context *ctx, GLenum *source, GLenum *type,
              GLuint *id, GLenum *severity, GLsizei bufSize, char *buf)
{
   struct gl_debug_msg *msg;
   GLsizei length;

   if (ctx->Debug.NumMessages == 0)
      return 0;

   msg = &ctx->Debug.Log[ctx->Debug.NextMsg];
   length = msg->length;

   assert(length > 0 && length == ctx->Debug.NextMsgLength);

   if (bufSize < length && buf != NULL)
      return 0;

   if (severity)
      *severity = msg->severity;
   if (source)
      *source = msg->source;
   if (type)
      *type = msg->type;
   if (id)
      *id = msg->id;

   if (buf) {
      assert(msg->message[length-1] == '\0');
      (void) strncpy(buf, msg->message, (size_t)length);
   }

   if (msg->message != (char*)out_of_memory)
      FREE(msg->message);
   msg->message = NULL;
   msg->length = 0;

   ctx->Debug.NumMessages--;
   ctx->Debug.NextMsg++;
   ctx->Debug.NextMsg %= MAX_DEBUG_LOGGED_MESSAGES;
   ctx->Debug.NextMsgLength = ctx->Debug.Log[ctx->Debug.NextMsg].length;

   return length;
}

/**
 * Verify that source, type, and severity are valid enums.
 * glDebugMessageInsertARB only accepts two values for 'source',
 * and glDebugMessageControlARB will additionally accept GL_DONT_CARE
 * in any parameter, so handle those cases specially.
 */
static GLboolean
validate_params(struct gl_context *ctx, unsigned caller,
                GLenum source, GLenum type, GLenum severity)
{
#define INSERT 1
#define CONTROL 2
   switch(source) {
   case GL_DEBUG_SOURCE_APPLICATION_ARB:
   case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
      break;
   case GL_DEBUG_SOURCE_API_ARB:
   case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
   case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
   case GL_DEBUG_SOURCE_OTHER_ARB:
      if (caller != INSERT)
         break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }

   switch(type) {
   case GL_DEBUG_TYPE_ERROR_ARB:
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
   case GL_DEBUG_TYPE_PERFORMANCE_ARB:
   case GL_DEBUG_TYPE_PORTABILITY_ARB:
   case GL_DEBUG_TYPE_OTHER_ARB:
      break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }

   switch(severity) {
   case GL_DEBUG_SEVERITY_HIGH_ARB:
   case GL_DEBUG_SEVERITY_MEDIUM_ARB:
   case GL_DEBUG_SEVERITY_LOW_ARB:
      break;
   case GL_DONT_CARE:
      if (caller == CONTROL)
         break;
   default:
      goto error;
   }
   return GL_TRUE;

error:
   {
      const char *callerstr;
      if (caller == INSERT)
         callerstr = "glDebugMessageInsertARB";
      else if (caller == CONTROL)
         callerstr = "glDebugMessageControlARB";
      else
         return GL_FALSE;

      _mesa_error( ctx, GL_INVALID_ENUM, "bad values passed to %s"
                  "(source=0x%x, type=0x%x, severity=0x%x)", callerstr,
                  source, type, severity);
   }
   return GL_FALSE;
}

static void GLAPIENTRY
_mesa_DebugMessageInsertARB(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLint length,
                            const GLcharARB* buf)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!validate_params(ctx, INSERT, source, type, severity))
      return; /* GL_INVALID_ENUM */

   if (length < 0)
      length = strlen(buf);

   if (length >= MAX_DEBUG_MESSAGE_LENGTH) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDebugMessageInsertARB"
                 "(length=%d, which is not less than "
                 "GL_MAX_DEBUG_MESSAGE_LENGTH_ARB=%d)", length,
                 MAX_DEBUG_MESSAGE_LENGTH);
      return;
   }

   _mesa_log_msg(ctx, source, type, id, severity, length, buf);
}

static GLuint GLAPIENTRY
_mesa_GetDebugMessageLogARB(GLuint count, GLsizei logSize, GLenum* sources,
                            GLenum* types, GLenum* ids, GLenum* severities,
                            GLsizei* lengths, GLcharARB* messageLog)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint ret;

   if (!messageLog)
      logSize = 0;

   if (logSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetDebugMessageLogARB"
                 "(logSize=%d : logSize must not be negative)", logSize);
      return 0;
   }

   for (ret = 0; ret < count; ret++) {
      GLsizei written = _mesa_get_msg(ctx, sources, types, ids, severities,
                                      logSize, messageLog);
      if (!written)
         break;

      if (messageLog) {
         messageLog += written;
         logSize -= written;
      }
      if (lengths) {
         *lengths = written;
         lengths++;
      }

      if (severities)
         severities++;
      if (sources)
         sources++;
      if (types)
         types++;
      if (ids)
         ids++;
   }

   return ret;
}

static void GLAPIENTRY
_mesa_DebugMessageCallbackARB(GLvoid *callback, GLvoid *userParam)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Debug.Callback = (GLDEBUGPROCARB)callback;
   ctx->Debug.CallbackData = userParam;
}

void
_mesa_init_errors_dispatch(struct _glapi_table *disp)
{
   SET_DebugMessageCallbackARB(disp, _mesa_DebugMessageCallbackARB);
   SET_DebugMessageInsertARB(disp, _mesa_DebugMessageInsertARB);
   SET_GetDebugMessageLogARB(disp, _mesa_GetDebugMessageLogARB);
}

void
_mesa_init_errors(struct gl_context *ctx)
{
   ctx->Debug.Callback = NULL;
   ctx->Debug.SyncOutput = GL_FALSE;
   ctx->Debug.Log[0].length = 0;
   ctx->Debug.NumMessages = 0;
   ctx->Debug.NextMsg = 0;
   ctx->Debug.NextMsgLength = 0;
}

/**********************************************************************/
/** \name Diagnostics */
/*@{*/

static void
output_if_debug(const char *prefixString, const char *outputString,
                GLboolean newline)
{
   static int debug = -1;

   /* Check the MESA_DEBUG environment variable if it hasn't
    * been checked yet.  We only have to check it once...
    */
   if (debug == -1) {
      char *env = _mesa_getenv("MESA_DEBUG");

      /* In a debug build, we print warning messages *unless*
       * MESA_DEBUG is 0.  In a non-debug build, we don't
       * print warning messages *unless* MESA_DEBUG is
       * set *to any value*.
       */
#ifdef DEBUG
      debug = (env != NULL && atoi(env) == 0) ? 0 : 1;
#else
      debug = (env != NULL) ? 1 : 0;
#endif
   }

   /* Now only print the string if we're required to do so. */
   if (debug) {
      fprintf(stderr, "%s: %s", prefixString, outputString);
      if (newline)
         fprintf(stderr, "\n");

#if defined(_WIN32) && !defined(_WIN32_WCE)
      /* stderr from windows applications without console is not usually 
       * visible, so communicate with the debugger instead */ 
      {
         char buf[4096];
         _mesa_snprintf(buf, sizeof(buf), "%s: %s%s", prefixString, outputString, newline ? "\n" : "");
         OutputDebugStringA(buf);
      }
#endif
   }
}


/**
 * Return string version of GL error code.
 */
static const char *
error_string( GLenum error )
{
   switch (error) {
   case GL_NO_ERROR:
      return "GL_NO_ERROR";
   case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
   case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
   case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
   case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
   case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
   case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
   case GL_TABLE_TOO_LARGE:
      return "GL_TABLE_TOO_LARGE";
   case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
   default:
      return "unknown";
   }
}


/**
 * When a new type of error is recorded, print a message describing
 * previous errors which were accumulated.
 */
static void
flush_delayed_errors( struct gl_context *ctx )
{
   char s[MAXSTRING];

   if (ctx->ErrorDebugCount) {
      _mesa_snprintf(s, MAXSTRING, "%d similar %s errors", 
                     ctx->ErrorDebugCount,
                     error_string(ctx->ErrorValue));

      output_if_debug("Mesa", s, GL_TRUE);

      ctx->ErrorDebugCount = 0;
   }
}


/**
 * Report a warning (a recoverable error condition) to stderr if
 * either DEBUG is defined or the MESA_DEBUG env var is set.
 *
 * \param ctx GL context.
 * \param fmtString printf()-like format string.
 */
void
_mesa_warning( struct gl_context *ctx, const char *fmtString, ... )
{
   char str[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   (void) _mesa_vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
   
   if (ctx)
      flush_delayed_errors( ctx );

   output_if_debug("Mesa warning", str, GL_TRUE);
}


/**
 * Report an internal implementation problem.
 * Prints the message to stderr via fprintf().
 *
 * \param ctx GL context.
 * \param fmtString problem description string.
 */
void
_mesa_problem( const struct gl_context *ctx, const char *fmtString, ... )
{
   va_list args;
   char str[MAXSTRING];
   static int numCalls = 0;

   (void) ctx;

   if (numCalls < 50) {
      numCalls++;

      va_start( args, fmtString );  
      _mesa_vsnprintf( str, MAXSTRING, fmtString, args );
      va_end( args );
      fprintf(stderr, "Mesa %s implementation error: %s\n",
              MESA_VERSION_STRING, str);
      fprintf(stderr, "Please report at bugs.freedesktop.org\n");
   }
}


/**
 * Record an OpenGL state error.  These usually occur when the user
 * passes invalid parameters to a GL function.
 *
 * If debugging is enabled (either at compile-time via the DEBUG macro, or
 * run-time via the MESA_DEBUG environment variable), report the error with
 * _mesa_debug().
 * 
 * \param ctx the GL context.
 * \param error the error value.
 * \param fmtString printf() style format string, followed by optional args
 */
void
_mesa_error( struct gl_context *ctx, GLenum error, const char *fmtString, ... )
{
   static GLint debug = -1;

   /* Check debug environment variable only once:
    */
   if (debug == -1) {
      const char *debugEnv = _mesa_getenv("MESA_DEBUG");

#ifdef DEBUG
      if (debugEnv && strstr(debugEnv, "silent"))
         debug = GL_FALSE;
      else
         debug = GL_TRUE;
#else
      if (debugEnv)
         debug = GL_TRUE;
      else
         debug = GL_FALSE;
#endif
   }

   if (debug) {      
      if (ctx->ErrorValue == error &&
          ctx->ErrorDebugFmtString == fmtString) {
         ctx->ErrorDebugCount++;
      }
      else {
         char s[MAXSTRING], s2[MAXSTRING];
         va_list args;

         flush_delayed_errors( ctx );
         
         va_start(args, fmtString);
         _mesa_vsnprintf(s, MAXSTRING, fmtString, args);
         va_end(args);

         _mesa_snprintf(s2, MAXSTRING, "%s in %s", error_string(error), s);
         output_if_debug("Mesa: User error", s2, GL_TRUE);
         
         ctx->ErrorDebugFmtString = fmtString;
         ctx->ErrorDebugCount = 0;
      }
   }

   _mesa_record_error(ctx, error);
}


/**
 * Report debug information.  Print error message to stderr via fprintf().
 * No-op if DEBUG mode not enabled.
 * 
 * \param ctx GL context.
 * \param fmtString printf()-style format string, followed by optional args.
 */
void
_mesa_debug( const struct gl_context *ctx, const char *fmtString, ... )
{
#ifdef DEBUG
   char s[MAXSTRING];
   va_list args;
   va_start(args, fmtString);
   _mesa_vsnprintf(s, MAXSTRING, fmtString, args);
   va_end(args);
   output_if_debug("Mesa", s, GL_FALSE);
#endif /* DEBUG */
   (void) ctx;
   (void) fmtString;
}

/*@}*/
