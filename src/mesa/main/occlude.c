/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Functions to implement the GL_ARB_occlusion_query extension.
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "occlude.h"
#include "mtypes.h"


/**
 * Allocate a new occlusion query object.
 * \param target - must be GL_SAMPLES_PASSED_ARB at this time
 * \param id - the object's ID
 * \return pointer to new query_object object or NULL if out of memory.
 */
static struct gl_query_object *
new_query_object(GLenum target, GLuint id)
{
   struct gl_query_object *q = MALLOC_STRUCT(gl_query_object);
   if (q) {
      q->Target = target;
      q->Id = id;
      q->Result = 0;
      q->Active = GL_FALSE;
      q->Ready = GL_TRUE;   /* correct, see spec */
   }
   return q;
}


/**
 * Delete an occlusion query object.
 * Not removed from hash table here.
 */
static void
delete_query_object(struct gl_query_object *q)
{
   FREE(q);
}


struct gl_query_object *
lookup_query_object(GLcontext *ctx, GLuint id)
{
   return (struct gl_query_object *)
      _mesa_HashLookup(ctx->Query.QueryObjects, id);
}



void GLAPIENTRY
_mesa_GenQueriesARB(GLsizei n, GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenQueriesARB(n < 0)");
      return;
   }

   /* No query objects can be active at this time! */
   if (ctx->Query.CurrentOcclusionObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGenQueriesARB");
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Query.QueryObjects, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct gl_query_object *q = new_query_object(GL_SAMPLES_PASSED_ARB,
                                                      first + i);
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
_mesa_DeleteQueriesARB(GLsizei n, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteQueriesARB(n < 0)");
      return;
   }

   /* No query objects can be active at this time! */
   if (ctx->Query.CurrentOcclusionObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteQueriesARB");
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] > 0) {
         struct gl_query_object *q = lookup_query_object(ctx, ids[i]);
         if (q) {
            ASSERT(!q->Active); /* should be caught earlier */
            _mesa_HashRemove(ctx->Query.QueryObjects, ids[i]);
            delete_query_object(q);
         }
      }
   }
}


GLboolean GLAPIENTRY
_mesa_IsQueryARB(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id && lookup_query_object(ctx, id))
      return GL_TRUE;
   else
      return GL_FALSE;
}


void GLAPIENTRY
_mesa_BeginQueryARB(GLenum target, GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_query_object *q;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   if (target != GL_SAMPLES_PASSED_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginQueryARB(target)");
      return;
   }

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB(id==0)");
      return;
   }

   if (ctx->Query.CurrentOcclusionObject) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBeginQueryARB(target)");
      return;
   }

   q = lookup_query_object(ctx, id);
   if (!q) {
      /* create new object */
      q = new_query_object(target, id);
      if (!q) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBeginQueryARB");
         return;
      }
      _mesa_HashInsert(ctx->Query.QueryObjects, id, q);
   }
   else {
      /* pre-existing object */
      if (q->Target != target) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginQueryARB(target mismatch)");
         return;
      }
      if (q->Active) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginQueryARB(query already active)");
         return;
      }
   }

   q->Active = GL_TRUE;
   q->Result = 0;
   q->Ready = GL_FALSE;
   ctx->Query.CurrentOcclusionObject = q;

   if (ctx->Driver.BeginQuery) {
      ctx->Driver.BeginQuery(ctx, q);
   }
}


void GLAPIENTRY
_mesa_EndQueryARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_query_object *q;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   FLUSH_VERTICES(ctx, _NEW_DEPTH);

   switch (target) {
      case GL_SAMPLES_PASSED_ARB:
         q = ctx->Query.CurrentOcclusionObject;
         ctx->Query.CurrentOcclusionObject = NULL;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glEndQueryARB(target)");
         return;
   }

   if (!q || !q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndQueryARB(no matching glBeginQueryARB)");
      return;
   }

   q->Active = GL_FALSE;
   if (ctx->Driver.EndQuery) {
      ctx->Driver.EndQuery(ctx, q);
   }
   else {
      q->Ready = GL_TRUE;
   }
}


void GLAPIENTRY
_mesa_GetQueryivARB(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_query_object *q;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
      case GL_SAMPLES_PASSED_ARB:
         q = ctx->Query.CurrentOcclusionObject;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(target)");
         return;
   }

   switch (pname) {
      case GL_QUERY_COUNTER_BITS_ARB:
         *params = 8 * sizeof(q->Result);
         break;
      case GL_CURRENT_QUERY_ARB:
         *params = q ? q->Id : 0;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectivARB(GLuint id, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_query_object *q = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectivARB(id=%d is active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         while (!q->Ready) {
            /* Wait for the query to finish! */
            /* If using software rendering, the result will always be ready
             * by time we get here.  Otherwise, we must be using hardware!
             */
            ASSERT(ctx->Driver.EndQuery);
         }
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
         /* XXX revisit when we have a hardware implementation! */
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectivARB(pname)");
         return;
   }
}


void GLAPIENTRY
_mesa_GetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_query_object *q = NULL;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id)
      q = lookup_query_object(ctx, id);

   if (!q || q->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetQueryObjectuivARB(id=%d is active)", id);
      return;
   }

   switch (pname) {
      case GL_QUERY_RESULT_ARB:
         while (!q->Ready) {
            /* Wait for the query to finish! */
            /* If using software rendering, the result will always be ready
             * by time we get here.  Otherwise, we must be using hardware!
             */
            ASSERT(ctx->Driver.EndQuery);
         }
         *params = q->Result;
         break;
      case GL_QUERY_RESULT_AVAILABLE_ARB:
         /* XXX revisit when we have a hardware implementation! */
         *params = q->Ready;
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetQueryObjectuivARB(pname)");
         return;
   }
}



/**
 * Allocate/init the context state related to occlusion query objects.
 */
void
_mesa_init_occlude(GLcontext *ctx)
{
#if FEATURE_ARB_occlusion_query
   ctx->Query.QueryObjects = _mesa_NewHashTable();
   ctx->Query.CurrentOcclusionObject = NULL;
#endif
}


/**
 * Free the context state related to occlusion query objects.
 */
void
_mesa_free_occlude_data(GLcontext *ctx)
{
   while (1) {
      GLuint id = _mesa_HashFirstEntry(ctx->Query.QueryObjects);
      if (id) {
         struct gl_query_object *q = lookup_query_object(ctx, id);
         ASSERT(q);
         delete_query_object(q);
         _mesa_HashRemove(ctx->Query.QueryObjects, id);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ctx->Query.QueryObjects);
}
