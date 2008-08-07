/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * @file
 * Trace dumping functions.
 * 
 * For now we just use standard XML for dumping the trace calls, as this is
 * simple to write, parse, and visually inspect, but the actual representation 
 * is abstracted out of this file, so that we can switch to a binary 
 * representation if/when it becomes justified.
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>   
 */


#include "pipe/p_compiler.h"
#include "util/u_string.h"

#include "tr_stream.h"
#include "tr_dump.h"


static INLINE void 
trace_dump_write(struct trace_stream *stream, const char *s)
{
   trace_stream_write(stream, s, strlen(s));
}


static INLINE void 
trace_dump_writef(struct trace_stream *stream, const char *format, ...)
{
   char buf[1024];
   va_list ap;
   va_start(ap, format);
   util_vsnprintf(buf, sizeof(buf), format, ap);
   va_end(ap);
   trace_dump_write(stream, buf);
}


static INLINE void 
trace_dump_escape(struct trace_stream *stream, const char *str) 
{
   const unsigned char *p = (const unsigned char *)str;
   unsigned char c;
   while((c = *p++) != 0) {
      if(c == '<')
         trace_dump_write(stream, "&lt;");
      else if(c == '>')
         trace_dump_write(stream, "&gt;");
      else if(c == '&')
         trace_dump_write(stream, "&amp;");
      else if(c == '\'')
         trace_dump_write(stream, "&apos;");
      else if(c == '\"')
         trace_dump_write(stream, "&quot;");
      else if(c >= 0x20 && c <= 0x7e)
         trace_dump_writef(stream, "%c", c);
      else
         trace_dump_writef(stream, "&#%u;", c);
   }
}


static INLINE void 
trace_dump_indent(struct trace_stream *stream, unsigned level)
{
   unsigned i;
   for(i = 0; i < level; ++i)
      trace_dump_write(stream, "\t");
}


static INLINE void 
trace_dump_newline(struct trace_stream *stream) 
{
   trace_dump_write(stream, "\n");
}


static INLINE void 
trace_dump_tag(struct trace_stream *stream, 
               const char *name)
{
   trace_dump_write(stream, "<");
   trace_dump_write(stream, name);
   trace_dump_write(stream, "/>");
}


static INLINE void 
trace_dump_tag_begin(struct trace_stream *stream, 
                     const char *name)
{
   trace_dump_write(stream, "<");
   trace_dump_write(stream, name);
   trace_dump_write(stream, ">");
}

static INLINE void 
trace_dump_tag_begin1(struct trace_stream *stream, 
                      const char *name, 
                      const char *attr1, const char *value1)
{
   trace_dump_write(stream, "<");
   trace_dump_write(stream, name);
   trace_dump_write(stream, " ");
   trace_dump_write(stream, attr1);
   trace_dump_write(stream, "='");
   trace_dump_escape(stream, value1);
   trace_dump_write(stream, "'>");
}


static INLINE void 
trace_dump_tag_begin2(struct trace_stream *stream, 
                      const char *name, 
                      const char *attr1, const char *value1,
                      const char *attr2, const char *value2)
{
   trace_dump_write(stream, "<");
   trace_dump_write(stream, name);
   trace_dump_write(stream, " ");
   trace_dump_write(stream, attr1);
   trace_dump_write(stream, "=\'");
   trace_dump_escape(stream, value1);
   trace_dump_write(stream, "\' ");
   trace_dump_write(stream, attr2);
   trace_dump_write(stream, "=\'");
   trace_dump_escape(stream, value2);
   trace_dump_write(stream, "\'>");
}


static INLINE void 
trace_dump_tag_begin3(struct trace_stream *stream, 
                      const char *name, 
                      const char *attr1, const char *value1,
                      const char *attr2, const char *value2,
                      const char *attr3, const char *value3)
{
   trace_dump_write(stream, "<");
   trace_dump_write(stream, name);
   trace_dump_write(stream, " ");
   trace_dump_write(stream, attr1);
   trace_dump_write(stream, "=\'");
   trace_dump_escape(stream, value1);
   trace_dump_write(stream, "\' ");
   trace_dump_write(stream, attr2);
   trace_dump_write(stream, "=\'");
   trace_dump_escape(stream, value2);
   trace_dump_write(stream, "\' ");
   trace_dump_write(stream, attr3);
   trace_dump_write(stream, "=\'");
   trace_dump_escape(stream, value3);
   trace_dump_write(stream, "\'>");
}


static INLINE void
trace_dump_tag_end(struct trace_stream *stream, 
                   const char *name)
{
   trace_dump_write(stream, "</");
   trace_dump_write(stream, name);
   trace_dump_write(stream, ">");
}


void  trace_dump_trace_begin(struct trace_stream *stream,
                             unsigned version)
{
   trace_dump_write(stream, "<?xml version='1.0' encoding='UTF-8'?>\n");
   trace_dump_write(stream, "<?xml-stylesheet type='text/xsl' href='trace.xsl'?>\n");
   trace_dump_writef(stream, "<trace version='%u'>\n", version);
}


void trace_dump_trace_end(struct trace_stream *stream)
{
   trace_dump_write(stream, "</trace>\n");
}

void trace_dump_call_begin(struct trace_stream *stream,
                           const char *klass, const char *method)
{
   trace_dump_indent(stream, 1);
   trace_dump_tag_begin2(stream, "call", "class", klass, "method", method);
   trace_dump_newline(stream);
}

void trace_dump_call_end(struct trace_stream *stream)
{
   trace_dump_indent(stream, 1);
   trace_dump_tag_end(stream, "call");
   trace_dump_newline(stream);
}

void trace_dump_arg_begin(struct trace_stream *stream,
                          const char *name)
{
   trace_dump_indent(stream, 2);
   trace_dump_tag_begin1(stream, "arg", "name", name);
}

void trace_dump_arg_end(struct trace_stream *stream)
{
   trace_dump_tag_end(stream, "arg");
   trace_dump_newline(stream);
}

void trace_dump_ret_begin(struct trace_stream *stream)
{
   trace_dump_indent(stream, 2);
   trace_dump_tag_begin(stream, "ret");
}

void trace_dump_ret_end(struct trace_stream *stream)
{
   trace_dump_tag_end(stream, "ret");
   trace_dump_newline(stream);
}

void trace_dump_bool(struct trace_stream *stream, 
                     int value)
{
   trace_dump_writef(stream, "<bool>%c</bool>", value ? '1' : '0');
}

void trace_dump_int(struct trace_stream *stream, 
                    long int value)
{
   trace_dump_writef(stream, "<int>%li</int>", value);
}

void trace_dump_uint(struct trace_stream *stream, 
                     long unsigned value)
{
   trace_dump_writef(stream, "<uint>%lu</uint>", value);
}

void trace_dump_float(struct trace_stream *stream, 
                      double value)
{
   trace_dump_writef(stream, "<float>%g</float>", value);
}

void trace_dump_string(struct trace_stream *stream, 
                       const char *str)
{
   trace_dump_write(stream, "<string>");
   trace_dump_escape(stream, str);
   trace_dump_write(stream, "</string>");
}

void trace_dump_array_begin(struct trace_stream *stream)
{
   trace_dump_write(stream, "<array>");
}

void trace_dump_array_end(struct trace_stream *stream)
{
   trace_dump_write(stream, "</array>");
}

void trace_dump_elem_begin(struct trace_stream *stream)
{
   trace_dump_write(stream, "<elem>");
}

void trace_dump_elem_end(struct trace_stream *stream)
{
   trace_dump_write(stream, "</elem>");
}

void trace_dump_struct_begin(struct trace_stream *stream, 
                             const char *name)
{
   trace_dump_writef(stream, "<struct name='%s'>", name);
}

void trace_dump_struct_end(struct trace_stream *stream)
{
   trace_dump_write(stream, "</struct>");
}

void trace_dump_member_begin(struct trace_stream *stream, 
                             const char *name)
{
   trace_dump_writef(stream, "<member name='%s'>", name);
}

void trace_dump_member_end(struct trace_stream *stream)
{
   trace_dump_write(stream, "</member>");
}

void trace_dump_null(struct trace_stream *stream)
{
   trace_dump_write(stream, "<null/>");
}

void trace_dump_ptr(struct trace_stream *stream, 
                    const void *value)
{
   if(value)
      trace_dump_writef(stream, "<ptr>%p</ptr>", value);
   else
      trace_dump_null(stream);
}
