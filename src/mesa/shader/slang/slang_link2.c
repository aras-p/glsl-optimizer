/*
 * Mesa 3-D graphics library
 * Version:  6.6
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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
 * \file slang_link.c
 * slang linker
 * \author Michal Krol
 */

#include "imports.h"
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "program.h"
#include "shaderobjects.h"
#include "slang_link.h"




#define RELEASE_GENERIC(x)\
   (**x)._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_CONTAINER(x)\
   (**x)._generic._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_PROGRAM(x)\
   (**x)._container._generic._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_SHADER(x)\
   (**x)._generic._unknown.Release ((struct gl2_unknown_intf **) (x))



static struct gl2_unknown_intf **
lookup_handle(GLcontext * ctx, GLhandleARB handle, enum gl2_uiid uiid,
              const char *function)
{
   struct gl2_unknown_intf **unk;

   /*
    * Note: _mesa_HashLookup() requires non-zero input values, so the
    * passed-in handle value must be checked beforehand.
    */
   if (handle == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, function);
      return NULL;
   }
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   unk = (struct gl2_unknown_intf **) _mesa_HashLookup(ctx->Shared->GL2Objects,
                                                       handle);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   if (unk == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, function);
   }
   else {
      unk = (**unk).QueryInterface(unk, uiid);
      if (unk == NULL)
         _mesa_error(ctx, GL_INVALID_OPERATION, function);
   }
   return unk;
}

#define GET_GENERIC(x, handle, function)\
   struct gl2_generic_intf **x = (struct gl2_generic_intf **)\
                                 lookup_handle (ctx, handle, UIID_GENERIC, function);

#define GET_CONTAINER(x, handle, function)\
   struct gl2_container_intf **x = (struct gl2_container_intf **)\
                                   lookup_handle (ctx, handle, UIID_CONTAINER, function);

#define GET_PROGRAM(x, handle, function)\
   struct gl2_program_intf **x = (struct gl2_program_intf **)\
                                 lookup_handle (ctx, handle, UIID_PROGRAM, function);

#define GET_SHADER(x, handle, function)\
   struct gl2_shader_intf **x = (struct gl2_shader_intf **)\
                                lookup_handle (ctx, handle, UIID_SHADER, function);


static void
prelink(GLhandleARB programObj, struct gl_linked_program *linked)
{
   GET_CURRENT_CONTEXT(ctx);

   linked->VertexProgram = NULL;
   linked->FragmentProgram = NULL;

   if (programObj != 0) {
      GET_PROGRAM(program, programObj, "glUseProgramObjectARB(program)");

      if (program == NULL)
         return;

      /* XXX terrible hack to find the real vertex/fragment programs */
      {
         GLuint handle;
         GLsizei cnt, i;
         cnt = (**program)._container.GetAttachedCount((struct gl2_container_intf **) (program));

         for (i = 0; i < cnt; i++) {
            struct gl2_generic_intf **x
               = (**program)._container.GetAttached((struct gl2_container_intf **) program, i);
            handle = (**x).GetName(x);
            {
               struct gl_program *prog;
               GET_SHADER(sha, handle, "foo");
               if (sha && (*sha)->Program) {
                  prog = (*sha)->Program;
                  if (prog->Target == GL_VERTEX_PROGRAM_ARB)
                     linked->VertexProgram = (struct gl_vertex_program *) prog;
                  else if (prog->Target == GL_FRAGMENT_PROGRAM_ARB)
                     linked->FragmentProgram = (struct gl_fragment_program *) prog;
               }
            }
#if 0
            if (linked->VertexProgram)
               printf("Found vert prog %p %d\n",
                      linked->VertexProgram,
                      linked->VertexProgram->Base.NumInstructions);
            if (linked->FragmentProgram)
               printf("Found frag prog %p %d\n",
                      linked->FragmentProgram,
                      linked->FragmentProgram->Base.NumInstructions);
#endif
            RELEASE_GENERIC(x);
         }
      }

   }
}



void
_slang_link2(GLcontext *ctx,
             GLhandleARB programObj,
             struct gl_linked_program *linked)
{
   struct gl_vertex_program *vertProg;
   struct gl_fragment_program *fragProg;

   prelink(programObj, linked);

   vertProg = linked->VertexProgram;
   fragProg = linked->FragmentProgram;

   /* free old linked data, if any */
   if (linked->NumUniforms > 0) {
      GLuint i;
      for (i = 0; i < linked->NumUniforms; i++) {
         _mesa_free((char *) linked->Uniforms[i].Name);
         linked->Uniforms[i].Name = NULL;
         linked->Uniforms[i].Value = NULL;
      }
      linked->NumUniforms = 0;
   }

   /*
    * Find uniforms.
    * XXX what about dups?
    */
   if (vertProg) {
      GLuint i;
      for (i = 0; i < vertProg->Base.Parameters->NumParameters; i++) {
         struct gl_program_parameter *p
            = vertProg->Base.Parameters->Parameters + i;
         if (p->Name) {
            struct gl_uniform *u = linked->Uniforms + linked->NumUniforms;
            u->Name = _mesa_strdup(p->Name);
            u->Value = &vertProg->Base.Parameters->ParameterValues[i][0];
            linked->NumUniforms++;
            assert(linked->NumUniforms < MAX_UNIFORMS);
         }
      }
   }
   if (fragProg) {
      GLuint i;
      for (i = 0; i < fragProg->Base.Parameters->NumParameters; i++) {
         struct gl_program_parameter *p
            = fragProg->Base.Parameters->Parameters + i;
         if (p->Name) {
            struct gl_uniform *u = linked->Uniforms + linked->NumUniforms;
            u->Name = _mesa_strdup(p->Name);
            u->Value = &fragProg->Base.Parameters->ParameterValues[i][0];
            linked->NumUniforms++;
            assert(linked->NumUniforms < MAX_UNIFORMS);
         }
      }
   }

   /* For varying:
    * scan both programs for varyings, rewrite programs so they agree
    * on locations of varyings.
    */

   /**
    * Linking should _copy_ the vertex and fragment shader code,
    * rewriting varying references as we go along...
    */

   linked->LinkStatus = (vertProg && fragProg);
}

