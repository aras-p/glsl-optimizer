/*
 * Copyright © 2017 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * glsl-optimizer change: minimal logging. Upstream log.c also supports
 * Android, Windows debugger output, syslog, log files and process-name
 * prefixes (MESA_LOG*), which an offline compiler does not need. Everything
 * here goes to stderr; the API is the one in log.h.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/log.h"
#include "util/ralloc.h"

static const char *
level_to_str(enum mesa_log_level l)
{
   switch (l) {
   case MESA_LOG_ERROR: return "error";
   case MESA_LOG_WARN: return "warning";
   case MESA_LOG_INFO: return "info";
   default: return "debug";
   }
}

FILE *
mesa_log_get_file(void)
{
   return stderr;
}

void
mesa_log_v(enum mesa_log_level level, const char *tag, const char *format,
           va_list va)
{
   if (level > MESA_DEFAULT_LOG_LEVEL)
      return;
   fprintf(stderr, "%s: %s: ", tag, level_to_str(level));
   vfprintf(stderr, format, va);
   if (format[0] == '\0' || format[strlen(format) - 1] != '\n')
      fputc('\n', stderr);
}

void
mesa_log(enum mesa_log_level level, const char *tag, const char *format, ...)
{
   va_list va;
   va_start(va, format);
   mesa_log_v(level, tag, format, va);
   va_end(va);
}

void
_mesa_log(const char *fmtString, ...)
{
   va_list va;
   va_start(va, fmtString);
   vfprintf(stderr, fmtString, va);
   va_end(va);
}

void
_mesa_log_direct(const char *string)
{
   fputs(string, stderr);
}

void
mesa_log_if_debug(enum mesa_log_level level, const char *outputString)
{
   if (getenv("MESA_DEBUG"))
      mesa_log(level, MESA_LOG_TAG, "%s", outputString);
}

struct log_stream *
_mesa_log_stream_create(enum mesa_log_level level, const char *tag)
{
   struct log_stream *stream = ralloc(NULL, struct log_stream);
   stream->level = level;
   stream->tag = tag;
   stream->msg = ralloc_strdup(stream, "");
   stream->pos = 0;
   return stream;
}

void
mesa_log_stream_destroy(struct log_stream *stream)
{
   if (stream->pos != 0)
      mesa_log(stream->level, stream->tag, "%s", stream->msg);
   ralloc_free(stream);
}

void
mesa_log_stream_printf(struct log_stream *stream, const char *format, ...)
{
   va_list va;
   va_start(va, format);
   ralloc_vasprintf_rewrite_tail(&stream->msg, &stream->pos, format, va);
   va_end(va);
}

void
_mesa_log_multiline(enum mesa_log_level level, const char *tag, const char *lines)
{
   if (level > MESA_DEFAULT_LOG_LEVEL)
      return;
   fputs(lines, stderr);
}
