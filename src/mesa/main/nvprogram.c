/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/**
 * \file nvprogram.c
 * NVIDIA vertex/fragment program state management functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvfragparse.h"
#include "nvfragprog.h"
#include "nvvertexec.h"
#include "nvvertparse.h"
#include "nvvertprog.h"
#include "nvprogram.h"


/**
 * Set the vertex/fragment program error state (position and error string).
 * This is generally called from within the parsers.
 */
void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string)
{
   ctx->Program.ErrorPos = pos;
   _mesa_free((void *) ctx->Program.ErrorString);
   if (!string)
      string = "";
   ctx->Program.ErrorString = _mesa_strdup(string);
}


/**
 * Find the line number and column for 'pos' within 'string'.
 * Return a copy of the line which contains 'pos'.  Free the line with
 * _mesa_free().
 * \param string  the program string
 * \param pos     the position within the string
 * \param line    returns the line number corresponding to 'pos'.
 * \param col     returns the column number corresponding to 'pos'.
 * \return copy of the line containing 'pos'.
 */
const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col)
{
   const GLubyte *lineStart = string;
   const GLubyte *p = string;
   GLubyte *s;
   int len;

   *line = 1;

   while (p != pos) {
      if (*p == (GLubyte) '\n') {
         (*line)++;
         lineStart = p + 1;
      }
      p++;
   }

   *col = (pos - lineStart) + 1;

   /* return copy of this line */
   while (*p != 0 && *p != '\n')
      p++;
   len = p - lineStart;
   s = (GLubyte *) _mesa_malloc(len + 1);
   _mesa_memcpy(s, lineStart, len);
   s[len] = 0;

   return s;
}



/**
 * Allocate and initialize a new fragment/vertex program object
 * \param ctx  context
 * \param id   program id/number
 * \param target  program target/type
 * \return  pointer to new program object
 */
struct program *
_mesa_alloc_program(GLcontext *ctx, GLenum target, GLuint id)
{
   struct program *prog;

   if (target == GL_VERTEX_PROGRAM_NV
       || target == GL_VERTEX_PROGRAM_ARB) {
      struct vertex_program *vprog = CALLOC_STRUCT(vertex_program);
      if (!vprog) {
         return NULL;
      }
      prog = &(vprog->Base);
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV
            || target == GL_FRAGMENT_PROGRAM_ARB) {
      struct fragment_program *fprog = CALLOC_STRUCT(fragment_program);
      if (!fprog) {
         return NULL;
      }
      prog = &(fprog->Base);
   }
   else {
      _mesa_problem(ctx, "bad target in _mesa_alloc_program");
      return NULL;
   }
   prog->Id = id;
   prog->Target = target;
   prog->Resident = GL_TRUE;
   prog->RefCount = 1;
   return prog;
}


/**
 * Delete a program and remove it from the hash table, ignoring the
 * reference count.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_delete_program(GLcontext *ctx, struct program *prog)
{
   ASSERT(prog);

   if (prog->String)
      _mesa_free(prog->String);
   if (prog->Target == GL_VERTEX_PROGRAM_NV ||
       prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
      struct vertex_program *vprog = (struct vertex_program *) prog;
      if (vprog->Instructions)
         _mesa_free(vprog->Instructions);
   }
   else if (prog->Target == GL_FRAGMENT_PROGRAM_NV) {
      struct fragment_program *fprog = (struct fragment_program *) prog;
      if (fprog->Instructions)
         _mesa_free(fprog->Instructions);
      if (fprog->Parameters) {
         GLuint i;
         for (i = 0; i < fprog->NumParameters; i++) {
            _mesa_free((void *) fprog->Parameters[i].Name);
         }
         _mesa_free(fprog->Parameters);
      }
   }
   _mesa_free(prog);
}


/**
 * Bind a program (make it current)
 * \note Called from the GL API dispatcher by both glBindProgramNV
 * and glBindProgramARB.
 */
void
_mesa_BindProgramNV(GLenum target, GLuint id)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if ((target == GL_VERTEX_PROGRAM_NV
        && ctx->Extensions.NV_vertex_program) ||
       (target == GL_VERTEX_PROGRAM_ARB
        && ctx->Extensions.ARB_vertex_program)) {
      if (ctx->VertexProgram.Current &&
          ctx->VertexProgram.Current->Base.Id == id)
         return;
      /* decrement refcount on previously bound vertex program */
      if (ctx->VertexProgram.Current) {
         ctx->VertexProgram.Current->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (ctx->VertexProgram.Current->Base.RefCount <= 0) {
            _mesa_delete_program(ctx, &(ctx->VertexProgram.Current->Base));
            _mesa_HashRemove(ctx->Shared->Programs, id);
         }
      }
   }
   else if ((target == GL_FRAGMENT_PROGRAM_NV
             && ctx->Extensions.NV_fragment_program) ||
            (target == GL_FRAGMENT_PROGRAM_ARB
             && ctx->Extensions.ARB_fragment_program)) {
      if (ctx->FragmentProgram.Current &&
          ctx->FragmentProgram.Current->Base.Id == id)
         return;
      /* decrement refcount on previously bound fragment program */
      if (ctx->FragmentProgram.Current) {
         ctx->FragmentProgram.Current->Base.RefCount--;
         /* and delete if refcount goes below one */
         if (ctx->FragmentProgram.Current->Base.RefCount <= 0) {
            _mesa_delete_program(ctx, &(ctx->FragmentProgram.Current->Base));
            _mesa_HashRemove(ctx->Shared->Programs, id);
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramNV/ARB(target)");
      return;
   }

   /* NOTE: binding to a non-existant program is not an error.
    * That's supposed to be caught in glBegin.
    */
   if (id == 0) {
      /* default program */
      prog = NULL;
      if (target == GL_VERTEX_PROGRAM_NV || target == GL_VERTEX_PROGRAM_ARB)
         prog = ctx->Shared->DefaultVertexProgram;
      else
         prog = ctx->Shared->DefaultFragmentProgram;
   }
   else {
      prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
      if (prog) {
         if (prog->Target == 0) {
            /* prog was allocated with glGenProgramsNV */
            prog->Target = target;
         }
         else if (prog->Target != target) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBindProgramNV/ARB(target mismatch)");
            return;
         }
      }
      else {
         /* allocate a new program now */
         prog = _mesa_alloc_program(ctx, target, id);
         if (!prog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindProgramNV/ARB");
            return;
         }
         prog->Id = id;
         prog->Target = target;
         prog->Resident = GL_TRUE;
         prog->RefCount = 1;
         _mesa_HashInsert(ctx->Shared->Programs, id, prog);
      }
   }

   /* bind now */
   if (target == GL_VERTEX_PROGRAM_NV || target == GL_VERTEX_PROGRAM_ARB) {
      ctx->VertexProgram.Current = (struct vertex_program *) prog;
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV || target == GL_FRAGMENT_PROGRAM_ARB) {
      ctx->FragmentProgram.Current = (struct fragment_program *) prog;
   }

   if (prog)
      prog->RefCount++;
}


/**
 * Delete a list of programs.
 * \note Not compiled into display lists.
 * \note Called by both glDeleteProgramsNV and glDeleteProgramsARB.
 */
void
_mesa_DeleteProgramsNV(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteProgramsNV" );
      return;
   }

   for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
         struct program *prog = (struct program *)
            _mesa_HashLookup(ctx->Shared->Programs, ids[i]);
         if (prog) {
            if (prog->Target == GL_VERTEX_PROGRAM_NV ||
                prog->Target == GL_VERTEX_STATE_PROGRAM_NV) {
               if (ctx->VertexProgram.Current &&
                   ctx->VertexProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgramNV(prog->Target, 0);
               }
            }
            else if (prog->Target == GL_FRAGMENT_PROGRAM_NV) {
               if (ctx->FragmentProgram.Current &&
                   ctx->FragmentProgram.Current->Base.Id == ids[i]) {
                  /* unbind this currently bound program */
                  _mesa_BindProgramNV(prog->Target, 0);
               }
            }
            else {
               _mesa_problem(ctx, "bad target in glDeleteProgramsNV");
               return;
            }
            prog->RefCount--;
            if (prog->RefCount <= 0) {
               _mesa_delete_program(ctx, prog);
            }
         }
      }
   }
}


/**
 * Execute a vertex state program.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
   struct vertex_program *vprog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_VERTEX_STATE_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glExecuteProgramNV");
      return;
   }

   vprog = (struct vertex_program *)
      _mesa_HashLookup(ctx->Shared->Programs, id);

   if (!vprog || vprog->Base.Target != GL_VERTEX_STATE_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glExecuteProgramNV");
      return;
   }
   
   _mesa_init_vp_registers(ctx);
   _mesa_init_tracked_matrices(ctx);
   COPY_4V(ctx->VertexProgram.Inputs[VERT_ATTRIB_POS], params);
   _mesa_exec_vertex_program(ctx, vprog);
}


/**
 * Generate a list of new program identifiers.
 * \note Not compiled into display lists.
 * \note Called by both glGenProgramsNV and glGenProgramsARB.
 */
void
_mesa_GenProgramsNV(GLsizei n, GLuint *ids)
{
   GLuint first;
   GLuint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPrograms");
      return;
   }

   if (!ids)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->Programs, n);

   for (i = 0; i < (GLuint) n; i++) {
      const int bytes = MAX2(sizeof(struct vertex_program),
                             sizeof(struct fragment_program));
      struct program *prog = (struct program *) _mesa_calloc(bytes);
      if (!prog) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenPrograms");
         return;
      }
      prog->RefCount = 1;
      prog->Id = first + i;
      _mesa_HashInsert(ctx->Shared->Programs, first + i, prog);
   }

   /* Return the program names */
   for (i = 0; i < (GLuint) n; i++) {
      ids[i] = first + i;
   }
}


/**
 * Determine if a set of programs is resident in hardware.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
GLboolean _mesa_AreProgramsResidentNV(GLsizei n, const GLuint *ids,
                                      GLboolean *residences)
{
   GLint i, j;
   GLboolean allResident = GL_TRUE;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV(n)");
      return GL_FALSE;
   }

   for (i = 0; i < n; i++) {
      const struct program *prog;
      if (ids[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV");
         return GL_FALSE;
      }
      prog = (const struct program *)
         _mesa_HashLookup(ctx->Shared->Programs, ids[i]);
      if (!prog) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glAreProgramsResidentNV");
         return GL_FALSE;
      }
      if (prog->Resident) {
	 if (!allResident)
	    residences[i] = GL_TRUE;
      }
      else {
         if (allResident) {
	    allResident = GL_FALSE;
	    for (j = 0; j < i; j++)
	       residences[j] = GL_TRUE;
	 }
	 residences[i] = GL_FALSE;
      }
   }

   return allResident;
}


/**
 * Request that a set of programs be resident in hardware.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_RequestResidentProgramsNV(GLsizei n, const GLuint *ids)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(n)");
      return;
   }

   /* just error checking for now */
   for (i = 0; i < n; i++) {
      struct program *prog;

      if (ids[i] == 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(id)");
         return;
      }

      prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, ids[i]);

      if (!prog) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glRequestResidentProgramsNV(id)");
         return;
      }

      prog->Resident = GL_TRUE;
   }
}


/**
 * Get a program parameter register.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetProgramParameterfvNV(GLenum target, GLuint index,
                              GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV) {
      if (pname == GL_PROGRAM_PARAMETER_NV) {
         if (index < MAX_NV_VERTEX_PROGRAM_PARAMS) {
            COPY_4V(params, ctx->VertexProgram.Parameters[index]);
         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramParameterfvNV(index)");
            return;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterfvNV(pname)");
         return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterfvNV(target)");
      return;
   }
}


/**
 * Get a program parameter register.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetProgramParameterdvNV(GLenum target, GLuint index,
                              GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV) {
      if (pname == GL_PROGRAM_PARAMETER_NV) {
         if (index < MAX_NV_VERTEX_PROGRAM_PARAMS) {
            COPY_4V(params, ctx->VertexProgram.Parameters[index]);
         }
         else {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glGetProgramParameterdvNV(index)");
            return;
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterdvNV(pname)");
         return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramParameterdvNV(target)");
      return;
   }
}


/**
 * Get a program attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetProgramivNV(GLuint id, GLenum pname, GLint *params)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   if (!prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramivNV");
      return;
   }

   switch (pname) {
      case GL_PROGRAM_TARGET_NV:
         *params = prog->Target;
         return;
      case GL_PROGRAM_LENGTH_NV:
         *params = prog->String ? _mesa_strlen((char *) prog->String) : 0;
         return;
      case GL_PROGRAM_RESIDENT_NV:
         *params = prog->Resident;
         return;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivNV(pname)");
         return;
   }
}


/**
 * Get the program source code.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetProgramStringNV(GLuint id, GLenum pname, GLubyte *program)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (pname != GL_PROGRAM_STRING_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringNV(pname)");
      return;
   }

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   if (!prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramStringNV");
      return;
   }

   if (prog->String) {
      MEMCPY(program, prog->String, _mesa_strlen((char *) prog->String));
   }
   else {
      program[0] = 0;
   }
}


/**
 * Get matrix tracking information.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetTrackMatrixivNV(GLenum target, GLuint address,
                         GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV
       && ctx->Extensions.NV_vertex_program) {
      GLuint i;

      if ((address & 0x3) || address >= MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetTrackMatrixivNV(address)");
         return;
      }

      i = address / 4;

      switch (pname) {
         case GL_TRACK_MATRIX_NV:
            params[0] = (GLint) ctx->VertexProgram.TrackMatrix[i];
            return;
         case GL_TRACK_MATRIX_TRANSFORM_NV:
            params[0] = (GLint) ctx->VertexProgram.TrackMatrixTransform[i];
            return;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetTrackMatrixivNV");
            return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTrackMatrixivNV");
      return;
   }
}


/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index == 0 || index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
	 FLUSH_CURRENT(ctx, 0);
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}

/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index == 0 || index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = (GLfloat) ctx->Array.VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
	 FLUSH_CURRENT(ctx, 0);
         COPY_4V(params, ctx->Current.Attrib[index]);
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}

/**
 * Get a vertex (or vertex array) attribute.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetVertexAttribivNV(GLuint index, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index == 0 || index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribdvNV(index)");
      return;
   }

   switch (pname) {
      case GL_ATTRIB_ARRAY_SIZE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Size;
         break;
      case GL_ATTRIB_ARRAY_STRIDE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Stride;
         break;
      case GL_ATTRIB_ARRAY_TYPE_NV:
         params[0] = ctx->Array.VertexAttrib[index].Type;
         break;
      case GL_CURRENT_ATTRIB_NV:
	 FLUSH_CURRENT(ctx, 0);
         params[0] = (GLint) ctx->Current.Attrib[index][0];
         params[1] = (GLint) ctx->Current.Attrib[index][1];
         params[2] = (GLint) ctx->Current.Attrib[index][2];
         params[3] = (GLint) ctx->Current.Attrib[index][3];
         break;
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB:
         if (!ctx->Extensions.ARB_vertex_buffer_object) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
            return;
         }
         params[0] = ctx->Array.VertexAttribArrayBufferBinding[index];
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribdvNV");
         return;
   }
}


/**
 * Get a vertex array attribute pointer.
 * \note Not compiled into display lists.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_GetVertexAttribPointervNV(GLuint index, GLenum pname, GLvoid **pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (index >= MAX_NV_VERTEX_PROGRAM_INPUTS) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetVertexAttribPointerNV(index)");
      return;
   }

   if (pname != GL_ATTRIB_ARRAY_POINTER_NV) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetVertexAttribPointerNV(pname)");
      return;
   }

   *pointer = ctx->Array.VertexAttrib[index].Ptr;;
}


/**
 * Determine if id names a program.
 * \note Not compiled into display lists.
 * \note Called from both glIsProgramNV and glIsProgramARB.
 * \param id is the program identifier
 * \return GL_TRUE if id is a program, else GL_FALSE.
 */
GLboolean
_mesa_IsProgramNV(GLuint id)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (id == 0)
      return GL_FALSE;

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   if (prog && prog->Target)
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Load/parse/compile a program.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_LoadProgramNV(GLenum target, GLuint id, GLsizei len,
                    const GLubyte *program)
{
   struct program *prog;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (id == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glLoadProgramNV(id)");
      return;
   }

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);

   if (prog && prog->Target != 0 && prog->Target != target) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(target)");
      return;
   }

   if ((target == GL_VERTEX_PROGRAM_NV ||
        target == GL_VERTEX_STATE_PROGRAM_NV)
       && ctx->Extensions.NV_vertex_program) {
      struct vertex_program *vprog = (struct vertex_program *) prog;
      if (!vprog) {
         vprog = CALLOC_STRUCT(vertex_program);
         if (!vprog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
            return;
         }
         vprog->Base.RefCount = 1;
         vprog->Base.Resident = GL_TRUE;
         _mesa_HashInsert(ctx->Shared->Programs, id, vprog);
      }
      _mesa_parse_nv_vertex_program(ctx, target, program, len, vprog);
   }
   else if (target == GL_FRAGMENT_PROGRAM_NV
            && ctx->Extensions.NV_fragment_program) {
      struct fragment_program *fprog = (struct fragment_program *) prog;
      if (!fprog) {
         fprog = CALLOC_STRUCT(fragment_program);
         if (!fprog) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
            return;
         }
         fprog->Base.RefCount = 1;
         fprog->Base.Resident = GL_TRUE;
         _mesa_HashInsert(ctx->Shared->Programs, id, fprog);
      }
      _mesa_parse_nv_fragment_program(ctx, target, program, len, fprog);
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glLoadProgramNV(target)");
   }
}



/**
 * Set a program parameter register.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameter4dNV(GLenum target, GLuint index,
                           GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramParameter4fNV(target, index, 
		   	      (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}


/**
 * Set a program parameter register.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameter4dvNV(GLenum target, GLuint index,
                            const GLdouble *params)
{
   _mesa_ProgramParameter4fNV(target, index,
                              (GLfloat)params[0], (GLfloat)params[1], 
			      (GLfloat)params[2], (GLfloat)params[3]);
}


/**
 * Set a program parameter register.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameter4fNV(GLenum target, GLuint index,
                           GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      if (index < MAX_NV_VERTEX_PROGRAM_PARAMS) {
         ASSIGN_4V(ctx->VertexProgram.Parameters[index], x, y, z, w);
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramParameterNV(index)");
         return;
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameterNV");
      return;
   }
}


/**
 * Set a program parameter register.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameter4fvNV(GLenum target, GLuint index,
                            const GLfloat *params)
{
   _mesa_ProgramParameter4fNV(target, index,
                              params[0], params[1], params[2], params[3]);
}



/**
 * Set a sequence of program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameters4dvNV(GLenum target, GLuint index,
                             GLuint num, const GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      GLuint i;
      if (index + num > MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramParameters4dvNV");
         return;
      }
      for (i = 0; i < num; i++) {
         ctx->VertexProgram.Parameters[index + i][0] = (GLfloat) params[0];
         ctx->VertexProgram.Parameters[index + i][1] = (GLfloat) params[1];
         ctx->VertexProgram.Parameters[index + i][2] = (GLfloat) params[2];
         ctx->VertexProgram.Parameters[index + i][3] = (GLfloat) params[3];
         params += 4;
      };
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameters4dvNV");
      return;
   }
}


/**
 * Set a sequence of program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_ProgramParameters4fvNV(GLenum target, GLuint index,
                             GLuint num, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      GLuint i;
      if (index + num > MAX_NV_VERTEX_PROGRAM_PARAMS) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glProgramParameters4fvNV");
         return;
      }
      for (i = 0; i < num; i++) {
         COPY_4V(ctx->VertexProgram.Parameters[index + i], params);
         params += 4;
      };
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameters4fvNV");
      return;
   }
}



/**
 * Setup tracking of matrices into program parameter registers.
 * \note Called from the GL API dispatcher.
 */
void
_mesa_TrackMatrixNV(GLenum target, GLuint address,
                    GLenum matrix, GLenum transform)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target == GL_VERTEX_PROGRAM_NV && ctx->Extensions.NV_vertex_program) {
      if (address & 0x3) {
         /* addr must be multiple of four */
         _mesa_error(ctx, GL_INVALID_VALUE, "glTrackMatrixNV(address)");
         return;
      }

      switch (matrix) {
         case GL_NONE:
         case GL_MODELVIEW:
         case GL_PROJECTION:
         case GL_TEXTURE:
         case GL_COLOR:
         case GL_MODELVIEW_PROJECTION_NV:
         case GL_MATRIX0_NV:
         case GL_MATRIX1_NV:
         case GL_MATRIX2_NV:
         case GL_MATRIX3_NV:
         case GL_MATRIX4_NV:
         case GL_MATRIX5_NV:
         case GL_MATRIX6_NV:
         case GL_MATRIX7_NV:
            /* OK, fallthrough */
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(matrix)");
            return;
      }

      switch (transform) {
         case GL_IDENTITY_NV:
         case GL_INVERSE_NV:
         case GL_TRANSPOSE_NV:
         case GL_INVERSE_TRANSPOSE_NV:
            /* OK, fallthrough */
            break;
         default:
            _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(transform)");
            return;
      }

      ctx->VertexProgram.TrackMatrix[address / 4] = matrix;
      ctx->VertexProgram.TrackMatrixTransform[address / 4] = transform;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTrackMatrixNV(target)");
      return;
   }
}


void
_mesa_ProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   struct program *prog;
   struct fragment_program *fragProg;
   GLint i;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   if (!prog || prog->Target != GL_FRAGMENT_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glProgramNamedParameterNV");
      return;
   }

   if (len <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramNamedParameterNV");
      return;
   }

   fragProg = (struct fragment_program *) prog;
   for (i = 0; i < fragProg->NumParameters; i++) {
      if (!_mesa_strncmp(fragProg->Parameters[i].Name,
                         (const char *) name, len) &&
          fragProg->Parameters[i].Name[len] == 0) {
         ASSERT(!fragProg->Parameters[i].Constant);
         fragProg->Parameters[i].Values[0] = x;
         fragProg->Parameters[i].Values[1] = y;
         fragProg->Parameters[i].Values[2] = z;
         fragProg->Parameters[i].Values[3] = w;
         return;
      }
   }

   _mesa_error(ctx, GL_INVALID_VALUE, "glProgramNamedParameterNV");
}


void
_mesa_ProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const float v[])
{
   _mesa_ProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}


void
_mesa_ProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name,
                                GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   _mesa_ProgramNamedParameter4fNV(id, len, name, (GLfloat)x, (GLfloat)y, 
                                   (GLfloat)z, (GLfloat)w);
}


void
_mesa_ProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name,
                                 const double v[])
{
   _mesa_ProgramNamedParameter4fNV(id, len, name,
                                   (GLfloat)v[0], (GLfloat)v[1],
                                   (GLfloat)v[2], (GLfloat)v[3]);
}


void
_mesa_GetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLfloat *params)
{
   struct program *prog;
   struct fragment_program *fragProg;
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->_CurrentProgram)
      ASSERT_OUTSIDE_BEGIN_END(ctx);

   prog = (struct program *) _mesa_HashLookup(ctx->Shared->Programs, id);
   if (!prog || prog->Target != GL_FRAGMENT_PROGRAM_NV) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramNamedParameterNV");
      return;
   }

   if (len <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramNamedParameterNV");
      return;
   }

   fragProg = (struct fragment_program *) prog;
   for (i = 0; i < fragProg->NumParameters; i++) {
      if (!_mesa_strncmp(fragProg->Parameters[i].Name,
                         (const char *) name, len) &&
          fragProg->Parameters[i].Name[len] == 0) {
         ASSERT(!fragProg->Parameters[i].Constant);
         params[0] = fragProg->Parameters[i].Values[0];
         params[1] = fragProg->Parameters[i].Values[1];
         params[2] = fragProg->Parameters[i].Values[2];
         params[3] = fragProg->Parameters[i].Values[3];
         return;
      }
   }

   _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramNamedParameterNV");
}


void
_mesa_GetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name,
                                   GLdouble *params)
{
   GLfloat floatParams[4];
   _mesa_GetProgramNamedParameterfvNV(id, len, name, floatParams);
   COPY_4V(params, floatParams);
}
