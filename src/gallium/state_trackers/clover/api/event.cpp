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

#include "api/util.hpp"
#include "core/event.hpp"

using namespace clover;

PUBLIC cl_event
clCreateUserEvent(cl_context d_ctx, cl_int *errcode_ret) try {
   auto &ctx = obj(d_ctx);

   ret_error(errcode_ret, CL_SUCCESS);
   return new soft_event(ctx, {}, false);

} catch(error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clSetUserEventStatus(cl_event ev, cl_int status) {
   if (!dynamic_cast<soft_event *>(ev))
      return CL_INVALID_EVENT;

   if (status > 0)
      return CL_INVALID_VALUE;

   if (ev->status() <= 0)
      return CL_INVALID_OPERATION;

   if (status)
      ev->abort(status);
   else
      ev->trigger();

   return CL_SUCCESS;
}

PUBLIC cl_int
clWaitForEvents(cl_uint num_evs, const cl_event *evs) try {
   if (!num_evs || !evs)
      throw error(CL_INVALID_VALUE);

   std::for_each(evs, evs + num_evs, [&](const cl_event ev) {
         if (!ev)
            throw error(CL_INVALID_EVENT);

         if (&ev->ctx != &evs[0]->ctx)
            throw error(CL_INVALID_CONTEXT);

         if (ev->status() < 0)
            throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
      });

   // Create a temporary soft event that depends on all the events in
   // the wait list
   ref_ptr<soft_event> sev = transfer(
      new soft_event(evs[0]->ctx, { evs, evs + num_evs }, true));

   // ...and wait on it.
   sev->wait();

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clGetEventInfo(cl_event ev, cl_event_info param,
               size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   if (!ev)
      return CL_INVALID_EVENT;

   switch (param) {
   case CL_EVENT_COMMAND_QUEUE:
      buf.as_scalar<cl_command_queue>() = ev->queue();
      break;

   case CL_EVENT_CONTEXT:
      buf.as_scalar<cl_context>() = &ev->ctx;
      break;

   case CL_EVENT_COMMAND_TYPE:
      buf.as_scalar<cl_command_type>() = ev->command();
      break;

   case CL_EVENT_COMMAND_EXECUTION_STATUS:
      buf.as_scalar<cl_int>() = ev->status();
      break;

   case CL_EVENT_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = ev->ref_count();
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clSetEventCallback(cl_event ev, cl_int type,
                   void (CL_CALLBACK *pfn_event_notify)(cl_event, cl_int,
                                                        void *),
                   void *user_data) try {
   if (!ev)
      throw error(CL_INVALID_EVENT);

   if (!pfn_event_notify || type != CL_COMPLETE)
      throw error(CL_INVALID_VALUE);

   // Create a temporary soft event that depends on ev, with
   // pfn_event_notify as completion action.
   ref_ptr<soft_event> sev = transfer(
      new soft_event(ev->ctx, { ev }, true,
                     [=](event &) {
                        ev->wait();
                        pfn_event_notify(ev, ev->status(), user_data);
                     }));

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clRetainEvent(cl_event ev) {
   if (!ev)
      return CL_INVALID_EVENT;

   ev->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseEvent(cl_event ev) {
   if (!ev)
      return CL_INVALID_EVENT;

   if (ev->release())
      delete ev;

   return CL_SUCCESS;
}

PUBLIC cl_int
clEnqueueMarker(cl_command_queue q, cl_event *ev) try {
   if (!q)
      throw error(CL_INVALID_COMMAND_QUEUE);

   if (!ev)
      throw error(CL_INVALID_VALUE);

   *ev = new hard_event(*q, CL_COMMAND_MARKER, {});

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueBarrier(cl_command_queue q) {
   if (!q)
      return CL_INVALID_COMMAND_QUEUE;

   // No need to do anything, q preserves data ordering strictly.
   return CL_SUCCESS;
}

PUBLIC cl_int
clEnqueueWaitForEvents(cl_command_queue q, cl_uint num_evs,
                       const cl_event *evs) try {
   if (!q)
      throw error(CL_INVALID_COMMAND_QUEUE);

   if (!num_evs || !evs)
      throw error(CL_INVALID_VALUE);

   std::for_each(evs, evs + num_evs, [&](const cl_event ev) {
         if (!ev)
            throw error(CL_INVALID_EVENT);

         if (&ev->ctx != &q->ctx)
            throw error(CL_INVALID_CONTEXT);
      });

   // Create a hard event that depends on the events in the wait list:
   // subsequent commands in the same queue will be implicitly
   // serialized with respect to it -- hard events always are.
   ref_ptr<hard_event> hev = transfer(
      new hard_event(*q, 0, { evs, evs + num_evs }));

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clGetEventProfilingInfo(cl_event ev, cl_profiling_info param,
                        size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };
   hard_event *hev = dynamic_cast<hard_event *>(ev);

   if (!ev)
      return CL_INVALID_EVENT;

   if (!hev || hev->status() != CL_COMPLETE)
      return CL_PROFILING_INFO_NOT_AVAILABLE;

   switch (param) {
   case CL_PROFILING_COMMAND_QUEUED:
      buf.as_scalar<cl_ulong>() = hev->time_queued();
      break;

   case CL_PROFILING_COMMAND_SUBMIT:
      buf.as_scalar<cl_ulong>() = hev->time_submit();
      break;

   case CL_PROFILING_COMMAND_START:
      buf.as_scalar<cl_ulong>() = hev->time_start();
      break;

   case CL_PROFILING_COMMAND_END:
      buf.as_scalar<cl_ulong>() = hev->time_end();
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (lazy<cl_ulong>::undefined_error &e) {
   return CL_PROFILING_INFO_NOT_AVAILABLE;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clFinish(cl_command_queue q) try {
   if (!q)
      throw error(CL_INVALID_COMMAND_QUEUE);

   // Create a temporary hard event -- it implicitly depends on all
   // the previously queued hard events.
   ref_ptr<hard_event> hev = transfer(new hard_event(*q, 0, { }));

   // And wait on it.
   hev->wait();

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}
