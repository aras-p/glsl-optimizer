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

event::event(clover::context &ctx, const ref_vector<event> &deps,
             action action_ok, action action_fail) :
   context(ctx), _status(0), wait_count(1),
   action_ok(action_ok), action_fail(action_fail) {
   for (auto &ev : deps)
      ev.chain(*this);
}

event::~event() {
}

void
event::trigger() {
   if (!--wait_count) {
      action_ok(*this);

      while (!_chain.empty()) {
         _chain.back()().trigger();
         _chain.pop_back();
      }
   }
}

void
event::abort(cl_int status) {
   _status = status;
   action_fail(*this);

   while (!_chain.empty()) {
      _chain.back()().abort(status);
      _chain.pop_back();
   }
}

bool
event::signalled() const {
   return !wait_count;
}

void
event::chain(event &ev) {
   if (wait_count) {
      ev.wait_count++;
      _chain.push_back(ev);
   }
   ev.deps.push_back(*this);
}

hard_event::hard_event(command_queue &q, cl_command_type command,
                       const ref_vector<event> &deps, action action) :
   event(q.context(), deps, profile(q, action), [](event &ev){}),
   _queue(q), _command(command), _fence(NULL) {
   if (q.profiling_enabled())
      _time_queued = timestamp::current(q);

   q.sequence(*this);
   trigger();
}

hard_event::~hard_event() {
   pipe_screen *screen = queue()->device().pipe;
   screen->fence_reference(screen, &_fence, NULL);
}

cl_int
hard_event::status() const {
   pipe_screen *screen = queue()->device().pipe;

   if (_status < 0)
      return _status;

   else if (!_fence)
      return CL_QUEUED;

   else if (!screen->fence_signalled(screen, _fence))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

command_queue *
hard_event::queue() const {
   return &_queue();
}

cl_command_type
hard_event::command() const {
   return _command;
}

void
hard_event::wait() const {
   pipe_screen *screen = queue()->device().pipe;

   if (status() == CL_QUEUED)
      queue()->flush();

   if (!_fence ||
       !screen->fence_finish(screen, _fence, PIPE_TIMEOUT_INFINITE))
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}

const lazy<cl_ulong> &
hard_event::time_queued() const {
   return _time_queued;
}

const lazy<cl_ulong> &
hard_event::time_submit() const {
   return _time_submit;
}

const lazy<cl_ulong> &
hard_event::time_start() const {
   return _time_start;
}

const lazy<cl_ulong> &
hard_event::time_end() const {
   return _time_end;
}

void
hard_event::fence(pipe_fence_handle *fence) {
   pipe_screen *screen = queue()->device().pipe;
   screen->fence_reference(screen, &_fence, fence);
}

event::action
hard_event::profile(command_queue &q, const action &action) const {
   if (q.profiling_enabled()) {
      return [&q, action] (event &ev) {
         auto &hev = static_cast<hard_event &>(ev);

         hev._time_submit = timestamp::current(q);
         hev._time_start = timestamp::query(q);

         action(ev);

         hev._time_end = timestamp::query(q);
      };

   } else {
      return action;
   }
}

soft_event::soft_event(clover::context &ctx, const ref_vector<event> &deps,
                       bool _trigger, action action) :
   event(ctx, deps, action, action) {
   if (_trigger)
      trigger();
}

cl_int
soft_event::status() const {
   if (_status < 0)
      return _status;

   else if (!signalled() ||
            any_of([](const event &ev) {
                  return ev.status() != CL_COMPLETE;
               }, deps))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

command_queue *
soft_event::queue() const {
   return NULL;
}

cl_command_type
soft_event::command() const {
   return CL_COMMAND_USER;
}

void
soft_event::wait() const {
   for (event &ev : deps)
      ev.wait();

   if (status() != CL_COMPLETE)
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}
