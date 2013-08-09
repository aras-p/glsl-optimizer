//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//

#include "core/event.hpp"
#include "pipe/p_screen.h"

using namespace clover;

_cl_event::_cl_event(clover::context &ctx,
                     std::vector<clover::event *> deps,
                     action action_ok, action action_fail) :
   ctx(ctx), __status(0), wait_count(1),
   action_ok(action_ok), action_fail(action_fail) {
   for (auto ev : deps)
      ev->chain(this);
}

_cl_event::~_cl_event() {
}

void
_cl_event::abort(cl_int status) {
   __status = status;
   action_fail(*this);

   while (!__chain.empty()) {
      __chain.back()->abort(status);
      __chain.pop_back();
   }
}

bool
_cl_event::signalled() const {
   return !wait_count;
}

void
_cl_event::chain(clover::event *ev) {
   if (wait_count) {
      ev->wait_count++;
      __chain.push_back(ev);
   }
   ev->deps.push_back(this);
}

hard_event::hard_event(clover::command_queue &q, cl_command_type command,
                       std::vector<clover::event *> deps, action action) :
   _cl_event(q.ctx, deps, action, [](event &ev){}),
   __queue(q), __command(command), __fence(NULL),
   __query_start(NULL), __query_end(NULL) {
   q.sequence(this);

   if(q.props() & CL_QUEUE_PROFILING_ENABLE) {
      pipe_screen *screen = q.dev.pipe;
      __ts_queued = screen->get_timestamp(screen);
   }

   trigger();
}

hard_event::~hard_event() {
   pipe_screen *screen = queue()->dev.pipe;
   pipe_context *pipe = queue()->pipe;
   screen->fence_reference(screen, &__fence, NULL);

   if(__query_start) {
      pipe->destroy_query(pipe, __query_start);
      __query_start = 0;
   }

   if(__query_end) {
      pipe->destroy_query(pipe, __query_end);
      __query_end = 0;
   }
}

void
hard_event::trigger() {
   if (!--wait_count) {
   /* XXX: Currently, a timestamp query gives wrong results for memory
    * transfers. This is, because we use memcpy instead of the DMA engines. */

      if(queue()->props() & CL_QUEUE_PROFILING_ENABLE) {
         pipe_context *pipe = queue()->pipe;
         __query_start = pipe->create_query(pipe, PIPE_QUERY_TIMESTAMP);
         pipe->end_query(queue()->pipe, __query_start);
      }

      action_ok(*this);

      if(queue()->props() & CL_QUEUE_PROFILING_ENABLE) {
         pipe_context *pipe = queue()->pipe;
         pipe_screen *screen = queue()->dev.pipe;
         __query_end = pipe->create_query(pipe, PIPE_QUERY_TIMESTAMP);
         pipe->end_query(pipe, __query_end);
         __ts_submit = screen->get_timestamp(screen);
      }

      while (!__chain.empty()) {
         __chain.back()->trigger();
         __chain.pop_back();
      }
   }
}

cl_int
hard_event::status() const {
   pipe_screen *screen = queue()->dev.pipe;

   if (__status < 0)
      return __status;

   else if (!__fence)
      return CL_QUEUED;

   else if (!screen->fence_signalled(screen, __fence))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

cl_command_queue
hard_event::queue() const {
   return &__queue;
}

cl_command_type
hard_event::command() const {
   return __command;
}

void
hard_event::wait() const {
   pipe_screen *screen = queue()->dev.pipe;

   if (status() == CL_QUEUED)
      queue()->flush();

   if (!__fence ||
       !screen->fence_finish(screen, __fence, PIPE_TIMEOUT_INFINITE))
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}

cl_ulong
hard_event::ts_queued() const {
   return __ts_queued;
}

cl_ulong
hard_event::ts_submit() const {
   return __ts_submit;
}

cl_ulong
hard_event::ts_start() {
   get_query_results();
   return __ts_start;
}

cl_ulong
hard_event::ts_end() {
   get_query_results();
   return __ts_end;
}

void
hard_event::get_query_results() {
   pipe_context *pipe = queue()->pipe;

   if(__query_start) {
      pipe_query_result result;
      pipe->get_query_result(pipe, __query_start, true, &result);
      __ts_start = result.u64;
      pipe->destroy_query(pipe, __query_start);
      __query_start = 0;
   }

   if(__query_end) {
      pipe_query_result result;
      pipe->get_query_result(pipe, __query_end, true, &result);
      __ts_end = result.u64;
      pipe->destroy_query(pipe, __query_end);
      __query_end = 0;
   }
}

void
hard_event::fence(pipe_fence_handle *fence) {
   pipe_screen *screen = queue()->dev.pipe;
   screen->fence_reference(screen, &__fence, fence);
}

soft_event::soft_event(clover::context &ctx,
                       std::vector<clover::event *> deps,
                       bool __trigger, action action) :
   _cl_event(ctx, deps, action, action) {
   if (__trigger)
      trigger();
}

void
soft_event::trigger() {
   if (!--wait_count) {
      action_ok(*this);

      while (!__chain.empty()) {
         __chain.back()->trigger();
         __chain.pop_back();
      }
   }
}

cl_int
soft_event::status() const {
   if (__status < 0)
      return __status;

   else if (!signalled() ||
            any_of([](const ref_ptr<event> &ev) {
                  return ev->status() != CL_COMPLETE;
               }, deps.begin(), deps.end()))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

cl_command_queue
soft_event::queue() const {
   return NULL;
}

cl_command_type
soft_event::command() const {
   return CL_COMMAND_USER;
}

void
soft_event::wait() const {
   for (auto ev : deps)
      ev->wait();

   if (status() != CL_COMPLETE)
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}
