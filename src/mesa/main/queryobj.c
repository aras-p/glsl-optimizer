/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "context.h"
#include "enums.h"
#include "hash.h"
#include "imports.h"
#include "queryobj.h"
#include "mtypes.h"
#include "main/dispatch.h"


/**
 * Allocate a new query object.  This is a fallback routine called via
 * ctx->Driver.NewQueryObject().
 * \param ctx - rendering context
 * \param id - the new object's ID
 * \return pointer to new query_object object or NULL if out of memory.
 */
static struct gl_query_object *
_mesa_new_query_object(struct gl_context *ctx, GLuint id)
{
   struct gl_query_object *q = CALLOC_STRUCT(gl_query_object);
   (void) ctx;
   if (q) {
      q->Id = id;
      q->Result = 0;
      q->Active = GL_FALSE;

      /* This is to satisfy the language of the specification: "In the initial
       * state of a query object, the result is available" (OpenGL 3.1 §
       * 2.13).
       */
      q->Ready = GL_TRUE;

      /* OpenGL 3.1 § 2.13 says about GenQueries, "These names are marked as
       * used, but no object is associated with them until the first time they
       * are used by BeginQuery." Since our implementation actually does
       * allocate an object at this point, use a flag to indicate that this
       * object has not yet been bound so should not be considered a query.
       */
      q->EverBound = GL_FALSE;
   }
   return q;
}


/**
 * Begin a query.  Software driver fallback.
 * Called via ctx->Driver.BeginQuery().
 */
static void
_mesa_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   ctx->NewState |= _NEW_DEPTH; /* for swrast */
}


/**
 * End a query.  Software driver fallback.
 * Called via ctx->Driver.EndQuery().
 */
static void
_mesa_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   ctx->NewState |= _NEW_DEPTH; /* for swrast */
   q->Ready = GL_TRUE;
}


/**
 * Wait for query to complete.  Software driver fallback.
 * Called via ctx->Driver.WaitQuery().
 */
static void
_mesa_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   /* For software drivers, _mesa_end_query() should have completed the query.
    * For real hardware, implement a proper WaitQuery() driver function,
    * which may require issuing a flush.
    */
   assert(q->Ready);
}


/**
 * Check if a query results are ready.  Software driver fallback.
 * Called via ctx->Driver.CheckQuery().
 */
static void
_mesa_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   /* No-op for sw rendering.
    * HW drivers may need to flush at this time.
    */
}


/**
 * Delete a query object.  Called via ctx->Driver.DeleteQuery().
 * Not removed from hash table here.
 */
static void
_mesa_delete_query(struct gl_context *ctx, struct gl_query_object *q)
{
   free(q->Label);
   free(q);
}


void
_mesa_init_query_object_functions(struct dd_function_table *driver)
{
   driver->NewQueryObject = _mesa_new_query_object;
   driver->DeleteQuery = _mesa_delete_query;
   driver->BeginQuery = _mesa_begin_query;
   driver->EndQuery = _mesa_end_query;
   driver->WaitQuery = _mesa_wait_query;
   driver->CheckQuery = _mesa_check_query;
}


/**
 * Return pointer to the query object binding point for the given target.
 * \return NULL if invalid target, else the address of binding point
 */
static struct gl_query_object **
get_query_binding_point(struct gl_context *ctx, GLenum target)
{
   switch (target) {
   case GL_SAMPLES_PASSED_ARB:
      if (ctx->Extensions.ARB_occlusion_query)
         return &ctx->Query.CurrentOcclusionObject;
      else
         return NULL;
   case GL_ANY_SAMPLES_PASSED:
      if (ctx->Extensions.ARB_occlusion_query2)
         return &ctx->Query.CurrentOcclusionObject;
      else
         return NULL;
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      if (ctx->Extensions.ARB_ES3_compatibility
          || (ctx->API == API_OPENGLES2 && ctx->Version >= 30))
         return &ctx->Query.CurrentOcclusionObject;
      else
         return NULL;
   case GL_TIME_ELAPSED_EXT:
      if (ctx->Extensions.EXT_timer_query)
         return &ctx->Query.CurrentTimerObject;
      else
         return NULL;
   case GL_PRIMITIVES_GENERATED:
      if (ctx->Extensions.EXT_transform_feedback)
         return &ctx->Query.PrimitivesGenerated;
      else
         return NULL;
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      if (ctx->Extensions.EXT_transform_feedback)
         return &ctx->Query.PrimitivesWritten;
      else
         return NULL;
   default:
      return NULL;
   }
}


void GLAPIENTRY
_mesa_GenQueries(GLsizei n, GLuint *ids)
{
   GLuint first;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGenQueries(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenQueriesARB(n < 0)");
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Query.QueryObjects, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct gl_query_object *q
            = ctx->Driver.NewQueryObject(ctx, first + i);
         if (!q) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenQueriesARB");
            return;
         }
         ids[i] = first + i;
         _mesa_HashInsert(ctx->Query.QueryObjects, first + i, q);
      }
   }
}


void GLAPIENTRY
_mesa_DeleteQueries(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDeleteQueries(%d)\n", n);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteQueriesARB(n < 0)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] > 0) {
         struct gl_query_object *q = _mesa_lookup_query_object(ctx, ids[i]);
         if (q) {
            if (q->Active) {
               struct gl_query_object **bindpt;
               bindpt = get_query_binding_point(ctx, q->Target);
               assert(bindpt); /* Should be non-null for active q. */
               if (bindpt) {
                  *bindpt = NULL;
               }
               q->Active = GL_FALSE;
               ctx->Driver.EndQuery(ctx, q);
            }
            _mesa_HashRemove(ctx->Query.QueryObjects, ids[i]);
            ctx->Driver.DeleteQuery(ctx, q);
         }
      }
   }
}


GLboolean GLAPIENTRY
_mesa_IsQuery(GLuint id)
{
   struct gl_query_object *q;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glIsQuery(%u)\n", id);

   if (id == 0)
      return GL_FALSE;

   q = _mesa_lookup_query_object(ctx, id);
   if (q == NULL)
      return GL_FALSE;

   return q->EverBound;
}

static GLboolean
query_error_check_index(struct gl_context *ctx, GLenum target, GLuint index)
{
   switch (target) {
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
   case GL_PRIMITIVES_GENERATED:
      if (index >= ctx->Const.MaxVertexStreams) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glBeginQueryIndexed(index>=MaxVertexStreams)");
         return GL_FALSE;
      }
      break;
   default:
      if (index > 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glBeginQueryIndexed(index>0)");
         return GL_FALSE;
      }
   }
   return GL_TRUE;
}

void GLAPIENTRY
_mesa_BeginQueryIndexed(GLenum target, GLuint index, GLuint id)
{
   struct gl_query_object *q, **bindpt;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBeginQueryIndexed(%s, %u, %u)\n",
                  _mesa_lookup_enum_by_nr(target), index, id);

   if (!query_error_check_index(ctx, target, index))
      return;

   FLUSH_VERTICES(ctx, 0);

   bindpt = get_query_binding_point(ctx, target);
   if (!bindpt) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQuery{Indexed}(target)");
      return;
   }

   /* From the GL_ARB_occlusion_query spec:
    *
    *     "If BeginQueryARB is called while another query is already in
    *      progress with the same target, an INVALID_OPERATION error is
    *      generated."
    */
   if (*bindpt) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginQuery{Indexed}(target=%s is active)",
                  _mesa_lookup_enum_by_nr(target));
      return;
   }

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQuery{Indexed}(id==0)");
      return;
   }

   q = _mesa_lookup_query_object(ctx, id);
   if (!q) {
      if (ctx->API != API_OPENGL_COMPAT) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginQuery{Indexed}(non-gen name)");
         return;
      } else {
         /* create new object */
         q = ctx->Driver.NewQueryObject(ctx, id);
         if (!q) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBeginQuery{Indexed}");
            return;
         }
         _mesa_HashInsert(ctx->Query.QueryObjects, id, q);
      }
   }
   else {
      /* pre-existing object */
      if (q->Active) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginQuery{Indexed}(query already active)");
         return;
      }
   }

   q->Target = target;
   q->Active = GL_TRUE;
   q->Result = 0;
   q->Ready = GL_FALSE;
   q->EverBound = GL_TRUE;

   /* XXX should probably refcount query objects */
   *bindpt = q;

   ctx->Driver.BeginQuery(ctx, q);
}


void GLAPIENTRY
_mesa_EndQueryIndexed(GLenum target, GLuint index)
{
   struct gl_query_object *q, **bindpt;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glEndQueryIndexed(%s, %u)\n",
                  _mesa_lookup_enum_by_nr(target), index);

   if (!query_error_check_index(ctx, target, index))
      return;

   FLUSH_VERTICES(ctx, 0);

   bindpt = get_query_binding_point(ctx, target);
   if (!bindpt) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glEndQuery{Indexed}(target)");
      return;
   }

   /* XXX should probably refcount query objects */
   q = *bindpt;

   /* Check for GL_ANY_SAMPLES_PASSED vs GL_SAMPLES_PASSED. */
   if (q && q->Target != target) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndQuery(target=%s with active query of target %s)",
                  _mesa_lookup_enum_by_nr(target),
                  _mesa_lookup_enum_by_nr(q->Target));
      return;
   }

   *bindpt = NULL;

   if (!q || !q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndQuery{Indexed}(no matching glBeginQuery{Indexed})");
      return;
   }

   q->Active = GL_FALSE;
   ctx->Driver.EndQuery(ctx, q);
}

void GLAPIENTRY
_mesa_BeginQuery(GLenum target, GLuint id)
{
   _mesa_BeginQueryIndexed(target, 0, id);
}

void GLAPIENTRY
_mesa_EndQuery(GLenum target)
{
   _mesa_EndQueryIndexed(target, 0);
}

void GLAPIENTRY
_mesa_QueryCounter(GLuint id, GLenum target)
{
   struct gl_query_object *q;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glQueryCounter(%u, %s)\n", id,
                  _mesa_lookup_enum_by_nr(target));

   /* error checking */
   if (target != GL_TIMESTAMP) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glQueryCounter(target)");
      return;
   }

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glQueryCounter(id==0)");
      return;
   }

   q = _mesa_lookup_query_object(ctx, id);
   if (!q) {
      /* XXX the Core profile should throw INVALID_OPERATION here */

      /* create new object */
      q = ctx->Driver.NewQueryObject(ctx, id);
      if (!q) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glQueryCounter");
         return;
      }
      _mesa_HashInsert(ctx->Query.QueryObjects, id, q);
   }
   else {
      if (q->Target && q->Target != GL_TIMESTAMP) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glQueryCounter(id has an invalid target)");
         return;
      }
   }

   if (q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glQueryCounter(id is active)");
      return;
   }

   q->Target = target;
   q->Result = 0;
   q->Ready = GL_FALSE;
   q->EverBound = GL_TRUE;

   if (ctx->Driver.QueryCounter) {
      ctx->Driver.QueryCounter(ctx, q);
   } else {
      /* QueryCounter is implemented using EndQuery without BeginQuery
       * in drivers. This is actually Direct3D and Gallium convention.
       */
      ctx->Driver.EndQuery(ctx, q);
   }
}


void GLAPIENTRY
_mesa_GetQueryIndexediv(GLenum target, GLuint index, GLenum pname,
                        GLint *params)
{
   struct gl_query_object *q = NULL, **bindpt = NULL;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetQueryIndexediv(%s, %u, %s)\n",
                  _mesa_lookup_enum_by_nr(target),
                  index,
                  _mesa_lookup_enum_by_nr(pname));

   if (!query_error_check_index(ctx, target, index))
      return;

   if (target == GL_TIMESTAMP) {
      if (!ctx->Extensions.ARB_timer_query) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryARB(target)");
         return;
      }
   }
   else {
      bindpt = get_query_binding_point(ctx, target);
      if (!bindpt) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQuery{Indexed}iv(target)");
         return;
      }

      q = *bindpt;
   }

   switch (pname) {
      case GL_QUERY_COUNTER_BITS_ARB:
         switch (target) {
         case GL_SAMPLES_PASSED:
            *params = ctx->Const.QueryCounterBits.SamplesPassed;
            break;
         case GL_ANY_SAMPLES_PASSED:
            /* The minimum value of this is 1 if it's nonzero, and the value
             * is only ever GL_TRUE or GL_FALSE, so no sense in reporting more
             * bits.
             */
            *params = 1;
            break;
         case GL_TIME_ELAPSED:
            *params = ctx->Const.QueryCounterBits.TimeElapsed;
            break;
         case GL_TIMESTAMP:
            *params = ctx->Const.QueryCounterBits.Timestamp;
            break;
         case GL_PRIMITIVES_GENERATED:
            *params = ctx->Const.QueryCounterBits.PrimitivesGenerated;
            break;
         case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
            *params = ctx->Const.QueryCounterBits.PrimitivesWritten;
            break;
         default:
            _mesa_problem(ctx,
                          "Unknown target in glGetQueryIndexediv(target = %s)",
                          _mesa_lookup_enum_by_nr(target));
            *params = 0;
            break;
         }
         break;
      case GL_CURRENT_QUERY_ARB:
         *params = (q && q->Target == target) ? q->Id : 0;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQuery{Indexed}iv(pname)");
         return;
   }
}

void GLAPIENTRY
_mesa_GetQueryiv(GLenum target, GLenum pname, GLint *params)
{
   _mesa_GetQueryIndexediv(target, 0, pname, params);
}

void GLAPIENTRY
_mesa_GetQueryObjectiv(GLuint id, GLenum pname, GLint *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetQueryObjectiv(%u, %s)\n", id,
                  _mesa_lookup_enum_by_nr(pname));

   if (id)
      q = _mesa_lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectivARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         /* if result is too large for returned type, clamp to max value */
         if (q->Target == GL_ANY_SAMPLES_PASSED
             || q->Target == GL_ANY_SAMPLES_PASSED_CONSERVATIVE) {
            if (q->Result)
               *params = GL_TRUE;
            else
               *params = GL_FALSE;
         } else {
            if (q->Result > 0x7fffffff) {
               *params = 0x7fffffff;
            }
            else {
               *params = (GLint)q->Result;
            }
         }
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetQueryObjectuiv(%u, %s)\n", id,
                  _mesa_lookup_enum_by_nr(pname));

   if (id)
      q = _mesa_lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectuivARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         /* if result is too large for returned type, clamp to max value */
         if (q->Target == GL_ANY_SAMPLES_PASSED
             || q->Target == GL_ANY_SAMPLES_PASSED_CONSERVATIVE) {
            if (q->Result)
               *params = GL_TRUE;
            else
               *params = GL_FALSE;
         } else {
            if (q->Result > 0xffffffff) {
               *params = 0xffffffff;
            }
            else {
               *params = (GLuint)q->Result;
            }
         }
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectuivARB(pname)");
         return;
   }
}


/**
 * New with GL_EXT_timer_query
 */
void GLAPIENTRY
_mesa_GetQueryObjecti64v(GLuint id, GLenum pname, GLint64EXT *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetQueryObjecti64v(%u, %s)\n", id,
                  _mesa_lookup_enum_by_nr(pname));

   if (id)
      q = _mesa_lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectui64vARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjecti64vARB(pname)");
         return;
   }
}


/**
 * New with GL_EXT_timer_query
 */
void GLAPIENTRY
_mesa_GetQueryObjectui64v(GLuint id, GLenum pname, GLuint64EXT *params)
{
   struct gl_query_object *q = NULL;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetQueryObjectui64v(%u, %s)\n", id,
                  _mesa_lookup_enum_by_nr(pname));

   if (id)
      q = _mesa_lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectuui64vARB(id=%d is invalid or active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         if (!q->Ready)
            ctx->Driver.WaitQuery(ctx, q);
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
	 if (!q->Ready)
	    ctx->Driver.CheckQuery( ctx, q );
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectui64vARB(pname)");
         return;
   }
}

/**
 * Allocate/init the context state related to query objects.
 */
void
_mesa_init_queryobj(struct gl_context *ctx)
{
   ctx->Query.QueryObjects = _mesa_NewHashTable();
   ctx->Query.CurrentOcclusionObject = NULL;

   ctx->Const.QueryCounterBits.SamplesPassed = 64;
   ctx->Const.QueryCounterBits.TimeElapsed = 64;
   ctx->Const.QueryCounterBits.Timestamp = 64;
   ctx->Const.QueryCounterBits.PrimitivesGenerated = 64;
   ctx->Const.QueryCounterBits.PrimitivesWritten = 64;
}


/**
 * Callback for deleting a query object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_queryobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_query_object *q= (struct gl_query_object *) data;
   struct gl_context *ctx = (struct gl_context *)userData;
   ctx->Driver.DeleteQuery(ctx, q);
}


/**
 * Free the context state related to query objects.
 */
void
_mesa_free_queryobj_data(struct gl_context *ctx)
{
   _mesa_HashDeleteAll(ctx->Query.QueryObjects, delete_queryobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Query.QueryObjects);
}
