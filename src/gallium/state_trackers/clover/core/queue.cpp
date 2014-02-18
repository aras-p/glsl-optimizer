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

#include "core/queue.hpp"
#include "core/event.hpp"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

using namespace clover;

command_queue::command_queue(clover::context &ctx, clover::device &dev,
                             cl_command_queue_properties props) :
   context(ctx), device(dev), props(props) {
   pipe = dev.pipe->context_create(dev.pipe, NULL);
   if (!pipe)
      throw error(CL_INVALID_DEVICE);
}

command_queue::~command_queue() {
   pipe->destroy(pipe);
}

void
command_queue::flush() {
   pipe_screen *screen = device().pipe;
   pipe_fence_handle *fence = NULL;

   if (!queued_events.empty()) {
      pipe->flush(pipe, &fence, 0);

      while (!queued_events.empty() &&
             queued_events.front()().signalled()) {
         queued_events.front()().fence(fence);
         queued_events.pop_front();
      }

      screen->fence_reference(screen, &fence, NULL);
   }
}

cl_command_queue_properties
command_queue::properties() const {
   return props;
}

bool
command_queue::profiling_enabled() const {
   return props & CL_QUEUE_PROFILING_ENABLE;
}

void
command_queue::sequence(hard_event &ev) {
   if (!queued_events.empty())
      queued_events.back()().chain(ev);

   queued_events.push_back(ev);
}
