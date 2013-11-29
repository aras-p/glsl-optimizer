/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file performance_monitor.c
 * Core Mesa support for the AMD_performance_monitor extension.
 *
 * In order to implement this extension, start by defining two enums:
 * one for Groups, and one for Counters.  These will be used as indexes into
 * arrays, so they should start at 0 and increment from there.
 *
 * Counter IDs need to be globally unique.  That is, you can't have counter 7
 * in group A and counter 7 in group B.  A global enum of all available
 * counters is a convenient way to guarantee this.
 */

#include <stdbool.h>
#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "hash.h"
#include "macros.h"
#include "mtypes.h"
#include "performance_monitor.h"
#include "bitset.h"
#include "ralloc.h"

void
_mesa_init_performance_monitors(struct gl_context *ctx)
{
   ctx->PerfMonitor.Monitors = _mesa_NewHashTable();
   ctx->PerfMonitor.NumGroups = 0;
   ctx->PerfMonitor.Groups = NULL;
}

static struct gl_perf_monitor_object *
new_performance_monitor(struct gl_context *ctx, GLuint index)
{
   unsigned i;
   struct gl_perf_monitor_object *m = ctx->Driver.NewPerfMonitor(ctx);

   if (m == NULL)
      return NULL;

   m->Name = index;

   m->Active = false;

   m->ActiveGroups =
      rzalloc_array(NULL, unsigned, ctx->PerfMonitor.NumGroups);

   m->ActiveCounters =
      ralloc_array(NULL, BITSET_WORD *, ctx->PerfMonitor.NumGroups);

   if (m->ActiveGroups == NULL || m->ActiveCounters == NULL)
      goto fail;

   for (i = 0; i < ctx->PerfMonitor.NumGroups; i++) {
      const struct gl_perf_monitor_group *g = &ctx->PerfMonitor.Groups[i];

      m->ActiveCounters[i] = rzalloc_array(m->ActiveCounters, BITSET_WORD,
                                           BITSET_WORDS(g->NumCounters));
      if (m->ActiveCounters[i] == NULL)
         goto fail;
   }

   return m;

fail:
   ralloc_free(m->ActiveGroups);
   ralloc_free(m->ActiveCounters);
   ctx->Driver.DeletePerfMonitor(ctx, m);
   return NULL;
}

static void
free_performance_monitor(GLuint key, void *data, void *user)
{
   struct gl_perf_monitor_object *m = data;
   struct gl_context *ctx = user;

   ralloc_free(m->ActiveGroups);
   ralloc_free(m->ActiveCounters);
   ctx->Driver.DeletePerfMonitor(ctx, m);
}

void
_mesa_free_performance_monitors(struct gl_context *ctx)
{
   _mesa_HashDeleteAll(ctx->PerfMonitor.Monitors,
                       free_performance_monitor, ctx);
   _mesa_DeleteHashTable(ctx->PerfMonitor.Monitors);
}

static inline struct gl_perf_monitor_object *
lookup_monitor(struct gl_context *ctx, GLuint id)
{
   return (struct gl_perf_monitor_object *)
      _mesa_HashLookup(ctx->PerfMonitor.Monitors, id);
}

static inline const struct gl_perf_monitor_group *
get_group(const struct gl_context *ctx, GLuint id)
{
   if (id >= ctx->PerfMonitor.NumGroups)
      return NULL;

   return &ctx->PerfMonitor.Groups[id];
}

static inline const struct gl_perf_monitor_counter *
get_counter(const struct gl_perf_monitor_group *group_obj, GLuint id)
{
   if (id >= group_obj->NumCounters)
      return NULL;

   return &group_obj->Counters[id];
}

/*****************************************************************************/

void GLAPIENTRY
_mesa_GetPerfMonitorGroupsAMD(GLint *numGroups, GLsizei groupsSize,
                              GLuint *groups)
{
   GET_CURRENT_CONTEXT(ctx);

   if (numGroups != NULL)
      *numGroups = ctx->PerfMonitor.NumGroups;

   if (groupsSize > 0 && groups != NULL) {
      unsigned i;
      unsigned n = MIN2((GLuint) groupsSize, ctx->PerfMonitor.NumGroups);

      /* We just use the index in the Groups array as the ID. */
      for (i = 0; i < n; i++)
         groups[i] = i;
   }
}

void GLAPIENTRY
_mesa_GetPerfMonitorCountersAMD(GLuint group, GLint *numCounters,
                                GLint *maxActiveCounters,
                                GLsizei countersSize, GLuint *counters)
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_perf_monitor_group *group_obj = get_group(ctx, group);
   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCountersAMD(invalid group)");
      return;
   }

   if (maxActiveCounters != NULL)
      *maxActiveCounters = group_obj->MaxActiveCounters;

   if (numCounters != NULL)
      *numCounters = group_obj->NumCounters;

   if (counters != NULL) {
      unsigned i;
      unsigned n = MIN2(group_obj->NumCounters, (GLuint) countersSize);
      for (i = 0; i < n; i++) {
         /* We just use the index in the Counters array as the ID. */
         counters[i] = i;
      }
   }
}

void GLAPIENTRY
_mesa_GetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize,
                                   GLsizei *length, GLchar *groupString)
{
   GET_CURRENT_CONTEXT(ctx);

   const struct gl_perf_monitor_group *group_obj = get_group(ctx, group);

   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetPerfMonitorGroupStringAMD");
      return;
   }

   if (bufSize == 0) {
      /* Return the number of characters that would be required to hold the
       * group string, excluding the null terminator.
       */
      if (length != NULL)
         *length = strlen(group_obj->Name);
   } else {
      if (length != NULL)
         *length = MIN2(strlen(group_obj->Name), bufSize);
      if (groupString != NULL)
         strncpy(groupString, group_obj->Name, bufSize);
   }
}

void GLAPIENTRY
_mesa_GetPerfMonitorCounterStringAMD(GLuint group, GLuint counter,
                                     GLsizei bufSize, GLsizei *length,
                                     GLchar *counterString)
{
   GET_CURRENT_CONTEXT(ctx);

   const struct gl_perf_monitor_group *group_obj;
   const struct gl_perf_monitor_counter *counter_obj;

   group_obj = get_group(ctx, group);

   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCounterStringAMD(invalid group)");
      return;
   }

   counter_obj = get_counter(group_obj, counter);

   if (counter_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCounterStringAMD(invalid counter)");
      return;
   }

   if (bufSize == 0) {
      /* Return the number of characters that would be required to hold the
       * counter string, excluding the null terminator.
       */
      if (length != NULL)
         *length = strlen(counter_obj->Name);
   } else {
      if (length != NULL)
         *length = MIN2(strlen(counter_obj->Name), bufSize);
      if (counterString != NULL)
         strncpy(counterString, counter_obj->Name, bufSize);
   }
}

void GLAPIENTRY
_mesa_GetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname,
                                   GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);

   const struct gl_perf_monitor_group *group_obj;
   const struct gl_perf_monitor_counter *counter_obj;

   group_obj = get_group(ctx, group);

   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCounterInfoAMD(invalid group)");
      return;
   }

   counter_obj = get_counter(group_obj, counter);

   if (counter_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCounterInfoAMD(invalid counter)");
      return;
   }

   switch (pname) {
   case GL_COUNTER_TYPE_AMD:
      *((GLenum *) data) = counter_obj->Type;
      break;

   case GL_COUNTER_RANGE_AMD:
      switch (counter_obj->Type) {
      case GL_FLOAT:
      case GL_PERCENTAGE_AMD: {
         float *f_data = data;
         f_data[0] = counter_obj->Minimum.f;
         f_data[1] = counter_obj->Maximum.f;
         break;
      }
      case GL_UNSIGNED_INT: {
         uint32_t *u32_data = data;
         u32_data[0] = counter_obj->Minimum.u32;
         u32_data[1] = counter_obj->Maximum.u32;
         break;
      }
      case GL_UNSIGNED_INT64_AMD: {
         uint64_t *u64_data = data;
         u64_data[0] = counter_obj->Minimum.u64;
         u64_data[1] = counter_obj->Maximum.u64;
         break;
      }
      default:
         assert(!"Should not get here: invalid counter type");
      }
      break;

   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetPerfMonitorCounterInfoAMD(pname)");
      return;
   }
}

void GLAPIENTRY
_mesa_GenPerfMonitorsAMD(GLsizei n, GLuint *monitors)
{
   GLuint first;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGenPerfMonitorsAMD(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPerfMonitorsAMD(n < 0)");
      return;
   }

   if (monitors == NULL)
      return;

   /* We don't actually need them to be contiguous, but this is what
    * the rest of Mesa does, so we may as well.
    */
   first = _mesa_HashFindFreeKeyBlock(ctx->PerfMonitor.Monitors, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct gl_perf_monitor_object *m =
            new_performance_monitor(ctx, first + i);
         if (!m) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenPerfMonitorsAMD");
            return;
         }
         monitors[i] = first + i;
         _mesa_HashInsert(ctx->PerfMonitor.Monitors, first + i, m);
      }
   } else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenPerfMonitorsAMD");
      return;
   }
}

void GLAPIENTRY
_mesa_DeletePerfMonitorsAMD(GLsizei n, GLuint *monitors)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDeletePerfMonitorsAMD(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeletePerfMonitorsAMD(n < 0)");
      return;
   }

   if (monitors == NULL)
      return;

   for (i = 0; i < n; i++) {
      struct gl_perf_monitor_object *m = lookup_monitor(ctx, monitors[i]);

      if (m) {
         /* Give the driver a chance to stop the monitor if it's active. */
         if (m->Active) {
            ctx->Driver.ResetPerfMonitor(ctx, m);
            m->Ended = false;
         }

         _mesa_HashRemove(ctx->PerfMonitor.Monitors, monitors[i]);
         ralloc_free(m->ActiveGroups);
         ralloc_free(m->ActiveCounters);
         ctx->Driver.DeletePerfMonitor(ctx, m);
      } else {
         /* "INVALID_VALUE error will be generated if any of the monitor IDs
          *  in the <monitors> parameter to DeletePerfMonitorsAMD do not
          *  reference a valid generated monitor ID."
          */
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDeletePerfMonitorsAMD(invalid monitor)");
      }
   }
}

void GLAPIENTRY
_mesa_SelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable,
                                   GLuint group, GLint numCounters,
                                   GLuint *counterList)
{
   GET_CURRENT_CONTEXT(ctx);
   int i;
   struct gl_perf_monitor_object *m;
   const struct gl_perf_monitor_group *group_obj;

   m = lookup_monitor(ctx, monitor);

   /* "INVALID_VALUE error will be generated if the <monitor> parameter to
    *  SelectPerfMonitorCountersAMD does not reference a monitor created by
    *  GenPerfMonitorsAMD."
    */
   if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glSelectPerfMonitorCountersAMD(invalid monitor)");
      return;
   }

   group_obj = get_group(ctx, group);

   /* "INVALID_VALUE error will be generated if the <group> parameter to
    *  GetPerfMonitorCountersAMD, GetPerfMonitorCounterStringAMD,
    *  GetPerfMonitorCounterStringAMD, GetPerfMonitorCounterInfoAMD, or
    *  SelectPerfMonitorCountersAMD does not reference a valid group ID."
    */
   if (group_obj == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glSelectPerfMonitorCountersAMD(invalid group)");
      return;
   }

   /* "INVALID_VALUE error will be generated if the <numCounters> parameter to
    *  SelectPerfMonitorCountersAMD is less than 0."
    */
   if (numCounters < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glSelectPerfMonitorCountersAMD(numCounters < 0)");
      return;
   }

   /* "When SelectPerfMonitorCountersAMD is called on a monitor, any outstanding
    *  results for that monitor become invalidated and the result queries
    *  PERFMON_RESULT_SIZE_AMD and PERFMON_RESULT_AVAILABLE_AMD are reset to 0."
    */
   ctx->Driver.ResetPerfMonitor(ctx, m);

   /* Sanity check the counter ID list. */
   for (i = 0; i < numCounters; i++) {
      if (counterList[i] >= group_obj->NumCounters) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glSelectPerfMonitorCountersAMD(invalid counter ID)");
         return;
      }
   }

   if (enable) {
      /* Enable the counters */
      for (i = 0; i < numCounters; i++) {
         ++m->ActiveGroups[group];
         BITSET_SET(m->ActiveCounters[group], counterList[i]);
      }
   } else {
      /* Disable the counters */
      for (i = 0; i < numCounters; i++) {
         --m->ActiveGroups[group];
         BITSET_CLEAR(m->ActiveCounters[group], counterList[i]);
      }
   }
}

void GLAPIENTRY
_mesa_BeginPerfMonitorAMD(GLuint monitor)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_perf_monitor_object *m = lookup_monitor(ctx, monitor);

   if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBeginPerfMonitorAMD(invalid monitor)");
      return;
   }

   /* "INVALID_OPERATION error will be generated if BeginPerfMonitorAMD is
    *  called when a performance monitor is already active."
    */
   if (m->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginPerfMonitor(already active)");
      return;
   }

   /* The driver is free to return false if it can't begin monitoring for
    * any reason.  This translates into an INVALID_OPERATION error.
    */
   if (ctx->Driver.BeginPerfMonitor(ctx, m)) {
      m->Active = true;
      m->Ended = false;
   } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginPerfMonitor(driver unable to begin monitoring)");
   }
}

void GLAPIENTRY
_mesa_EndPerfMonitorAMD(GLuint monitor)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_perf_monitor_object *m = lookup_monitor(ctx, monitor);

   if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glEndPerfMonitorAMD(invalid monitor)");
      return;
   }

   /* "INVALID_OPERATION error will be generated if EndPerfMonitorAMD is called
    *  when a performance monitor is not currently started."
    */
   if (!m->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginPerfMonitor(not active)");
      return;
   }

   ctx->Driver.EndPerfMonitor(ctx, m);

   m->Active = false;
   m->Ended = true;
}

/**
 * Return the number of bytes needed to store a monitor's result.
 */
static unsigned
perf_monitor_result_size(const struct gl_context *ctx,
                         const struct gl_perf_monitor_object *m)
{
   unsigned group, counter;
   unsigned size = 0;

   for (group = 0; group < ctx->PerfMonitor.NumGroups; group++) {
      const struct gl_perf_monitor_group *g = &ctx->PerfMonitor.Groups[group];
      for (counter = 0; counter < g->NumCounters; counter++) {
         const struct gl_perf_monitor_counter *c = &g->Counters[counter];

         if (!BITSET_TEST(m->ActiveCounters[group], counter))
            continue;

         size += sizeof(uint32_t); /* Group ID */
         size += sizeof(uint32_t); /* Counter ID */
         size += _mesa_perf_monitor_counter_size(c);
      }
   }
   return size;
}

void GLAPIENTRY
_mesa_GetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname,
                                   GLsizei dataSize, GLuint *data,
                                   GLint *bytesWritten)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_perf_monitor_object *m = lookup_monitor(ctx, monitor);
   bool result_available;

   if (m == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetPerfMonitorCounterDataAMD(invalid monitor)");
      return;
   }

   /* "It is an INVALID_OPERATION error for <data> to be NULL." */
   if (data == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetPerfMonitorCounterDataAMD(data == NULL)");
      return;
   }

   /* We need at least enough room for a single value. */
   if (dataSize < sizeof(GLuint)) {
      if (bytesWritten != NULL)
         *bytesWritten = 0;
      return;
   }

   /* If the monitor has never ended, there is no result. */
   result_available = m->Ended &&
      ctx->Driver.IsPerfMonitorResultAvailable(ctx, m);

   /* AMD appears to return 0 for all queries unless a result is available. */
   if (!result_available) {
      *data = 0;
      if (bytesWritten != NULL)
         *bytesWritten = sizeof(GLuint);
      return;
   }

   switch (pname) {
   case GL_PERFMON_RESULT_AVAILABLE_AMD:
      *data = 1;
      if (bytesWritten != NULL)
         *bytesWritten = sizeof(GLuint);
      break;
   case GL_PERFMON_RESULT_SIZE_AMD:
      *data = perf_monitor_result_size(ctx, m);
      if (bytesWritten != NULL)
         *bytesWritten = sizeof(GLuint);
      break;
   case GL_PERFMON_RESULT_AMD:
      ctx->Driver.GetPerfMonitorResult(ctx, m, dataSize, data, bytesWritten);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetPerfMonitorCounterDataAMD(pname)");
   }
}

/**
 * Returns how many bytes a counter's value takes up.
 */
unsigned
_mesa_perf_monitor_counter_size(const struct gl_perf_monitor_counter *c)
{
   switch (c->Type) {
   case GL_FLOAT:
   case GL_PERCENTAGE_AMD:
      return sizeof(GLfloat);
   case GL_UNSIGNED_INT:
      return sizeof(GLuint);
   case GL_UNSIGNED_INT64_AMD:
      return sizeof(uint64_t);
   default:
      assert(!"Should not get here: invalid counter type");
      return 0;
   }
}
