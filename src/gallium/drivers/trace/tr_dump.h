/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation streams (the
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
 * Trace data dumping primitives.
 */

#ifndef TR_DUMP_H
#define TR_DUMP_H


#include "pipe/p_util.h"


struct trace_stream;


void trace_dump_trace_begin(struct trace_stream *stream, unsigned version);
void trace_dump_trace_end(struct trace_stream *stream);
void trace_dump_call_begin(struct trace_stream *stream, const char *klass, const char *method);
void trace_dump_call_end(struct trace_stream *stream);
void trace_dump_arg_begin(struct trace_stream *stream, const char *name);
void trace_dump_arg_end(struct trace_stream *stream);
void trace_dump_ret_begin(struct trace_stream *stream);
void trace_dump_ret_end(struct trace_stream *stream);
void trace_dump_bool(struct trace_stream *stream, int value);
void trace_dump_int(struct trace_stream *stream, long int value);
void trace_dump_uint(struct trace_stream *stream, long unsigned value);
void trace_dump_float(struct trace_stream *stream, double value);
void trace_dump_string(struct trace_stream *stream, const char *str);
void trace_dump_enum(struct trace_stream *stream, const char *value);
void trace_dump_array_begin(struct trace_stream *stream);
void trace_dump_array_end(struct trace_stream *stream);
void trace_dump_elem_begin(struct trace_stream *stream);
void trace_dump_elem_end(struct trace_stream *stream);
void trace_dump_struct_begin(struct trace_stream *stream, const char *name);
void trace_dump_struct_end(struct trace_stream *stream);
void trace_dump_member_begin(struct trace_stream *stream, const char *name);
void trace_dump_member_end(struct trace_stream *stream);
void trace_dump_null(struct trace_stream *stream);
void trace_dump_ptr(struct trace_stream *stream, const void *value);


/*
 * Code saving macros. 
 */

#define trace_dump_arg(_stream, _type, _arg) \
   do { \
      trace_dump_arg_begin(_stream, #_arg); \
      trace_dump_##_type(_stream, _arg); \
      trace_dump_arg_end(_stream); \
   } while(0)

#define trace_dump_ret(_stream, _type, _arg) \
   do { \
      trace_dump_ret_begin(_stream); \
      trace_dump_##_type(_stream, _arg); \
      trace_dump_ret_end(_stream); \
   } while(0)

#define trace_dump_array(_stream, _type, _obj, _size) \
   do { \
      unsigned long idx; \
      trace_dump_array_begin(_stream); \
      for(idx = 0; idx < (_size); ++idx) { \
         trace_dump_elem_begin(_stream); \
         trace_dump_##_type(_stream, (_obj)[idx]); \
         trace_dump_elem_end(_stream); \
      } \
      trace_dump_array_end(_stream); \
   } while(0)

#define trace_dump_struct_array(_stream, _type, _obj, _size) \
   do { \
      unsigned long idx; \
      trace_dump_array_begin(_stream); \
      for(idx = 0; idx < (_size); ++idx) { \
         trace_dump_elem_begin(_stream); \
         trace_dump_##_type(_stream, &(_obj)[idx]); \
         trace_dump_elem_end(_stream); \
      } \
      trace_dump_array_end(_stream); \
   } while(0)

#define trace_dump_member(_stream, _type, _obj, _member) \
   do { \
      trace_dump_member_begin(_stream, #_member); \
      trace_dump_##_type(_stream, (_obj)->_member); \
      trace_dump_member_end(_stream); \
   } while(0)

#define trace_dump_arg_array(_stream, _type, _arg, _size) \
   do { \
      trace_dump_arg_begin(_stream, #_arg); \
      trace_dump_array(_stream, _type, _arg, _size); \
      trace_dump_arg_end(_stream); \
   } while(0)

#define trace_dump_member_array(_stream, _type, _obj, _member) \
   do { \
      trace_dump_member_begin(_stream, #_member); \
      trace_dump_array(_stream, _type, (_obj)->_member, sizeof((_obj)->_member)/sizeof((_obj)->_member[0])); \
      trace_dump_member_end(_stream); \
   } while(0)


#endif /* TR_DUMP_H */
