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
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "shaderobjects.h"
#include "slang_link.h"




static GLboolean
link_varying_vars(struct gl_linked_program *linked, struct gl_program *prog)
{
   GLuint *map, i, firstVarying, newFile;
   GLbitfield varsWritten, varsRead;

   map = (GLuint *) malloc(prog->Varying->NumParameters * sizeof(GLuint));
   if (!map)
      return GL_FALSE;

   for (i = 0; i < prog->Varying->NumParameters; i++) {
      /* see if this varying is in the linked varying list */
      const struct gl_program_parameter *var
         = prog->Varying->Parameters + i;

      GLint j = _mesa_lookup_parameter_index(linked->Varying, -1, var->Name);
      if (j >= 0) {
         /* already in list, check size */
         if (var->Size != linked->Varying->Parameters[j].Size) {
            /* error */
            return GL_FALSE;
         }
      }
      else {
         /* not already in linked list */
         j = _mesa_add_varying(linked->Varying, var->Name, var->Size);
      }
      ASSERT(j >= 0);

      map[i] = j;
   }


   /* Varying variables are treated like other vertex program outputs
    * (and like other fragment program inputs).  The position of the
    * first varying differs for vertex/fragment programs...
    * Also, replace File=PROGRAM_VARYING with File=PROGRAM_INPUT/OUTPUT.
    */
   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      firstVarying = VERT_RESULT_VAR0;
      newFile = PROGRAM_OUTPUT;
   }
   else {
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
      firstVarying = FRAG_ATTRIB_VAR0;
      newFile = PROGRAM_INPUT;
   }

   /* keep track of which varying vars we read and write */
   varsWritten = varsRead = 0x0;

   /* OK, now scan the program/shader instructions looking for varying vars,
    * replacing the old index with the new index.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      GLuint j;

      if (inst->DstReg.File == PROGRAM_VARYING) {
         inst->DstReg.File = newFile;
         inst->DstReg.Index = map[ inst->DstReg.Index ] + firstVarying;
         varsWritten |= (1 << inst->DstReg.Index);
      }

      for (j = 0; j < 3; j++) {
         if (inst->SrcReg[j].File == PROGRAM_VARYING) {
            inst->SrcReg[j].File = newFile;
            inst->SrcReg[j].Index = map[ inst->SrcReg[j].Index ] + firstVarying;
            varsRead |= (1 << inst->SrcReg[j].Index);
         }
      }
      /* XXX update program OutputsWritten, InputsRead */
   }

   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      prog->OutputsWritten |= varsWritten;
   }
   else {
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
      prog->InputsRead |= varsRead;
   }


   free(map);

   return GL_TRUE;
}


static GLboolean
is_uniform(enum register_file file)
{
   return (file == PROGRAM_ENV_PARAM ||
           file == PROGRAM_STATE_VAR ||
           file == PROGRAM_NAMED_PARAM ||
           file == PROGRAM_CONSTANT ||
           file == PROGRAM_UNIFORM);
}


static GLboolean
link_uniform_vars(struct gl_linked_program *linked, struct gl_program *prog)
{
   GLuint *map, i;

   map = (GLuint *) malloc(prog->Parameters->NumParameters * sizeof(GLuint));
   if (!map)
      return GL_FALSE;

   for (i = 0; i < prog->Parameters->NumParameters; i++) {
      /* see if this uniform is in the linked uniform list */
      const struct gl_program_parameter *p = prog->Parameters->Parameters + i;
      const GLfloat *pVals = prog->Parameters->ParameterValues[i];
      GLint j;

      /* sanity check */
      assert(is_uniform(p->Type));

      if (p->Name) {
         j = _mesa_lookup_parameter_index(linked->Uniforms, -1, p->Name);
      }
      else {
         GLuint swizzle;
         ASSERT(p->Type == PROGRAM_CONSTANT);
         if (_mesa_lookup_parameter_constant(linked->Uniforms, pVals,
                                             p->Size, &j, &swizzle)) {
            assert(j >= 0);
         }
         else {
            j = -1;
         }
      }

      if (j >= 0) {
         /* already in list, check size XXX check this */
         assert(p->Size == linked->Uniforms->Parameters[j].Size);
      }
      else {
         /* not already in linked list */
         switch (p->Type) {
         case PROGRAM_ENV_PARAM:
            j = _mesa_add_named_parameter(linked->Uniforms, p->Name, pVals);
         case PROGRAM_CONSTANT:
            j = _mesa_add_named_constant(linked->Uniforms, p->Name, pVals, p->Size);
            break;
         case PROGRAM_STATE_VAR:
            j = _mesa_add_state_reference(linked->Uniforms, (const GLint *) p->StateIndexes);
            break;
         case PROGRAM_UNIFORM:
            j = _mesa_add_uniform(linked->Uniforms, p->Name, p->Size);
            break;
         default:
            abort();
         }

      }
      ASSERT(j >= 0);

      map[i] = j;
   }


   /* OK, now scan the program/shader instructions looking for varying vars,
    * replacing the old index with the new index.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      GLuint j;

      if (is_uniform(inst->DstReg.File)) {
         inst->DstReg.Index = map[ inst->DstReg.Index ];
      }

      for (j = 0; j < 3; j++) {
         if (is_uniform(inst->SrcReg[j].File)) {
            inst->SrcReg[j].Index = map[ inst->SrcReg[j].Index ];
         }
      }
      /* XXX update program OutputsWritten, InputsRead */
   }

   free(map);

   return GL_TRUE;
}


static void
free_linked_program_data(GLcontext *ctx, struct gl_linked_program *linked)
{
   if (linked->VertexProgram) {
      if (linked->VertexProgram->Base.Parameters == linked->Uniforms) {
         /* to prevent a double-free in the next call */
         linked->VertexProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &linked->VertexProgram->Base);
      linked->VertexProgram = NULL;
   }

   if (linked->FragmentProgram) {
      if (linked->FragmentProgram->Base.Parameters == linked->Uniforms) {
         /* to prevent a double-free in the next call */
         linked->FragmentProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &linked->FragmentProgram->Base);
      linked->FragmentProgram = NULL;
   }


   if (linked->Uniforms) {
      _mesa_free_parameter_list(linked->Uniforms);
      linked->Uniforms = NULL;
   }

   if (linked->Varying) {
      _mesa_free_parameter_list(linked->Varying);
      linked->Varying = NULL;
   }
}


/**
 * Shader linker.  Currently:
 *
 * 1. The last attached vertex shader and fragment shader are linked.
 * 2. Varying vars in the two shaders are combined so their locations
 *    agree between the vertex and fragment stages.  They're treated as
 *    vertex program output attribs and as fragment program input attribs.
 * 3. Uniform vars (including state references, constants, etc) from the
 *    vertex and fragment shaders are merged into one group.  Recall that
 *    GLSL uniforms are shared by all linked shaders.
 * 4. The vertex and fragment programs are cloned and modified to update
 *    src/dst register references so they use the new, linked uniform/
 *    varying storage locations.
 */
void
_slang_link2(GLcontext *ctx,
             GLhandleARB programObj,
             struct gl_linked_program *linked)
{
   struct gl_vertex_program *vertProg;
   struct gl_fragment_program *fragProg;
   GLuint i;

   free_linked_program_data(ctx, linked);

   linked->Uniforms = _mesa_new_parameter_list();
   linked->Varying = _mesa_new_parameter_list();

   /**
    * Find attached vertex shader, fragment shader
    */
   vertProg = NULL;
   fragProg = NULL;
   for (i = 0; i < linked->NumShaders; i++) {
      if (linked->Shaders[i]->Target == GL_VERTEX_PROGRAM_ARB)
         vertProg = (struct gl_vertex_program *) linked->Shaders[i];
      else if (linked->Shaders[i]->Target == GL_FRAGMENT_PROGRAM_ARB)
         fragProg = (struct gl_fragment_program *) linked->Shaders[i];
      else
         _mesa_problem(ctx, "unexpected shader target in slang_link2()");
   }
   if (!vertProg || !fragProg) {
      /* XXX is it legal to have one but not the other?? */
      /* XXX record error */
      linked->LinkStatus = GL_FALSE;
      return;
   }

   /*
    * Make copies of the vertex/fragment programs now since we'll be
    * changing src/dst registers after merging the uniforms and varying vars.
    */
   linked->VertexProgram = (struct gl_vertex_program *)
      _mesa_clone_program(ctx, &vertProg->Base);
   linked->FragmentProgram = (struct gl_fragment_program *)
      _mesa_clone_program(ctx, &fragProg->Base);

#if 1
   printf("************** orig program\n");
   _mesa_print_program(&fragProg->Base);
   _mesa_print_program_parameters(ctx, &fragProg->Base);
#endif

   link_varying_vars(linked, &linked->VertexProgram->Base);
   link_varying_vars(linked, &linked->FragmentProgram->Base);

   link_uniform_vars(linked, &linked->VertexProgram->Base);
   link_uniform_vars(linked, &linked->FragmentProgram->Base);

   /* The vertex and fragment programs share a common set of uniforms now */
   _mesa_free_parameter_list(linked->VertexProgram->Base.Parameters);
   _mesa_free_parameter_list(linked->FragmentProgram->Base.Parameters);
   linked->VertexProgram->Base.Parameters = linked->Uniforms;
   linked->FragmentProgram->Base.Parameters = linked->Uniforms;

#if 1
   printf("************** linked/cloned\n");
   _mesa_print_program(&linked->FragmentProgram->Base);
   _mesa_print_program_parameters(ctx, &linked->FragmentProgram->Base);
#endif

   linked->LinkStatus = (linked->VertexProgram && linked->FragmentProgram);
}

