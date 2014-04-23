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
 * \file performance_monitor.h
 * Core Mesa support for the AMD_performance_monitor extension and the
 * INTEL_performance_query extension.
 */

#pragma once
#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include "glheader.h"

extern void
_mesa_init_performance_monitors(struct gl_context *ctx);

extern void
_mesa_free_performance_monitors(struct gl_context *ctx);

extern void GLAPIENTRY
_mesa_GetPerfMonitorGroupsAMD(GLint *numGroups, GLsizei groupsSize,
                              GLuint *groups);

extern void GLAPIENTRY
_mesa_GetPerfMonitorCountersAMD(GLuint group, GLint *numCounters,
                                GLint *maxActiveCounters,
                                GLsizei countersSize, GLuint *counters);

extern void GLAPIENTRY
_mesa_GetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize,
                                   GLsizei *length, GLchar *groupString);

extern void GLAPIENTRY
_mesa_GetPerfMonitorCounterStringAMD(GLuint group, GLuint counter,
                                     GLsizei bufSize, GLsizei *length,
                                     GLchar *counterString);

extern void GLAPIENTRY
_mesa_GetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname,
                                   GLvoid *data);

extern void GLAPIENTRY
_mesa_GenPerfMonitorsAMD(GLsizei n, GLuint *monitors);

extern void GLAPIENTRY
_mesa_DeletePerfMonitorsAMD(GLsizei n, GLuint *monitors);

extern void GLAPIENTRY
_mesa_SelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable,
                                   GLuint group, GLint numCounters,
                                   GLuint *counterList);

extern void GLAPIENTRY
_mesa_BeginPerfMonitorAMD(GLuint monitor);

extern void GLAPIENTRY
_mesa_EndPerfMonitorAMD(GLuint monitor);

extern void GLAPIENTRY
_mesa_GetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname,
                                   GLsizei dataSize, GLuint *data,
                                   GLint *bytesWritten);

unsigned
_mesa_perf_monitor_counter_size(const struct gl_perf_monitor_counter *);


extern void GLAPIENTRY
_mesa_GetFirstPerfQueryIdINTEL(GLuint *queryId);

extern void GLAPIENTRY
_mesa_GetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId);

extern void GLAPIENTRY
_mesa_GetPerfQueryIdByNameINTEL(char *queryName, GLuint *queryId);

extern void GLAPIENTRY
_mesa_GetPerfQueryInfoINTEL(GLuint queryId,
                            GLuint queryNameLength, char *queryName,
                            GLuint *dataSize, GLuint *noCounters,
                            GLuint *noActiveInstances,
                            GLuint *capsMask);

extern void GLAPIENTRY
_mesa_GetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId,
                              GLuint counterNameLength, char *counterName,
                              GLuint counterDescLength, char *counterDesc,
                              GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum,
                              GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue);

extern void GLAPIENTRY
_mesa_CreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle);

extern void GLAPIENTRY
_mesa_DeletePerfQueryINTEL(GLuint queryHandle);

extern void GLAPIENTRY
_mesa_BeginPerfQueryINTEL(GLuint queryHandle);

extern void GLAPIENTRY
_mesa_EndPerfQueryINTEL(GLuint queryHandle);

extern void GLAPIENTRY
_mesa_GetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags,
                            GLsizei dataSize, void *data, GLuint *bytesWritten);

#endif
