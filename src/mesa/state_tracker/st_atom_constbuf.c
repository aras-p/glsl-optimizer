/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@vmware.com>
 *   Brian Paul
 */

#include "main/imports.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_upload_mgr.h"
#include "cso_cache/cso_context.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_atom_constbuf.h"
#include "st_program.h"
#include "st_cb_bufferobjects.h"

/**
 * Pass the given program parameters to the graphics pipe as a
 * constant buffer.
 * \param shader_type  either PIPE_SHADER_VERTEX or PIPE_SHADER_FRAGMENT
 */
void st_upload_constants( struct st_context *st,
                          struct gl_program_parameter_list *params,
                          unsigned shader_type)
{
   assert(shader_type == PIPE_SHADER_VERTEX ||
          shader_type == PIPE_SHADER_FRAGMENT ||
          shader_type == PIPE_SHADER_GEOMETRY);

   /* update constants */
   if (params && params->NumParameters) {
      struct pipe_constant_buffer cb;
      const uint paramBytes = params->NumParameters * sizeof(GLfloat) * 4;

      /* Update the constants which come from fixed-function state, such as
       * transformation matrices, fog factors, etc.  The rest of the values in
       * the parameters list are explicitly set by the user with glUniform,
       * glProgramParameter(), etc.
       */
      _mesa_load_state_parameters(st->ctx, params);

      /* We always need to get a new buffer, to keep the drivers simple and
       * avoid gratuitous rendering synchronization.
       * Let's use a user buffer to avoid an unnecessary copy.
       */
      if (st->constbuf_uploader) {
         cb.buffer = NULL;
         cb.user_buffer = NULL;
         u_upload_data(st->constbuf_uploader, 0, paramBytes,
                       params->ParameterValues, &cb.buffer_offset, &cb.buffer);
         u_upload_unmap(st->constbuf_uploader);
      } else {
         cb.buffer = NULL;
         cb.user_buffer = params->ParameterValues;
         cb.buffer_offset = 0;
      }
      cb.buffer_size = paramBytes;

      if (ST_DEBUG & DEBUG_CONSTANTS) {
         debug_printf("%s(shader=%d, numParams=%d, stateFlags=0x%x)\n",
                      __FUNCTION__, shader_type, params->NumParameters,
                      params->StateFlags);
         _mesa_print_parameter_list(params);
      }

      cso_set_constant_buffer(st->cso_context, shader_type, 0, &cb);
      pipe_resource_reference(&cb.buffer, NULL);

      st->state.constants[shader_type].ptr = params->ParameterValues;
      st->state.constants[shader_type].size = paramBytes;
   }
   else if (st->state.constants[shader_type].ptr) {
      /* Unbind. */
      st->state.constants[shader_type].ptr = NULL;
      st->state.constants[shader_type].size = 0;
      cso_set_constant_buffer(st->cso_context, shader_type, 0, NULL);
   }
}


/**
 * Vertex shader:
 */
static void update_vs_constants(struct st_context *st )
{
   struct st_vertex_program *vp = st->vp;
   struct gl_program_parameter_list *params = vp->Base.Base.Parameters;

   st_upload_constants( st, params, PIPE_SHADER_VERTEX );
}


const struct st_tracked_state st_update_vs_constants = {
   "st_update_vs_constants",				/* name */
   {							/* dirty */
      _NEW_PROGRAM_CONSTANTS,                           /* mesa */
      ST_NEW_VERTEX_PROGRAM,				/* st */
   },
   update_vs_constants					/* update */
};



/**
 * Fragment shader:
 */
static void update_fs_constants(struct st_context *st )
{
   struct st_fragment_program *fp = st->fp;
   struct gl_program_parameter_list *params = fp->Base.Base.Parameters;

   st_upload_constants( st, params, PIPE_SHADER_FRAGMENT );
}


const struct st_tracked_state st_update_fs_constants = {
   "st_update_fs_constants",				/* name */
   {							/* dirty */
      _NEW_PROGRAM_CONSTANTS,                           /* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_fs_constants					/* update */
};

/* Geometry shader:
 */
static void update_gs_constants(struct st_context *st )
{
   struct st_geometry_program *gp = st->gp;
   struct gl_program_parameter_list *params;

   if (gp) {
      params = gp->Base.Base.Parameters;
      st_upload_constants( st, params, PIPE_SHADER_GEOMETRY );
   }
}

const struct st_tracked_state st_update_gs_constants = {
   "st_update_gs_constants",				/* name */
   {							/* dirty */
      _NEW_PROGRAM_CONSTANTS,                           /* mesa */
      ST_NEW_GEOMETRY_PROGRAM,				/* st */
   },
   update_gs_constants					/* update */
};

static void st_bind_ubos(struct st_context *st,
                           struct gl_shader *shader,
                           unsigned shader_type)
{
   unsigned i;
   struct pipe_constant_buffer cb = { 0 };

   if (!shader)
      return;

   for (i = 0; i < shader->NumUniformBlocks; i++) {
      struct gl_uniform_buffer_binding *binding;
      struct st_buffer_object *st_obj;

      binding = &st->ctx->UniformBufferBindings[shader->UniformBlocks[i].Binding];
      st_obj = st_buffer_object(binding->BufferObject);

      cb.buffer = st_obj->buffer;

      if (cb.buffer) {
         cb.buffer_offset = binding->Offset;
         cb.buffer_size = cb.buffer->width0 - binding->Offset;

         /* AutomaticSize is FALSE if the buffer was set with BindBufferRange.
          * Take the minimum just to be sure.
          */
         if (!binding->AutomaticSize)
            cb.buffer_size = MIN2(cb.buffer_size, (unsigned) binding->Size);
      }
      else {
         cb.buffer_offset = 0;
         cb.buffer_size = 0;
      }

      cso_set_constant_buffer(st->cso_context, shader_type, 1 + i, &cb);
   }
}

static void bind_vs_ubos(struct st_context *st)
{
   struct gl_shader_program *prog =
      st->ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];

   if (!prog)
      return;

   st_bind_ubos(st, prog->_LinkedShaders[MESA_SHADER_VERTEX], PIPE_SHADER_VERTEX);
}

const struct st_tracked_state st_bind_vs_ubos = {
   "st_bind_vs_ubos",
   {
      0,
      ST_NEW_VERTEX_PROGRAM | ST_NEW_UNIFORM_BUFFER,
   },
   bind_vs_ubos
};

static void bind_fs_ubos(struct st_context *st)
{
   struct gl_shader_program *prog =
      st->ctx->_Shader->CurrentProgram[MESA_SHADER_FRAGMENT];

   if (!prog)
      return;

   st_bind_ubos(st, prog->_LinkedShaders[MESA_SHADER_FRAGMENT], PIPE_SHADER_FRAGMENT);
}

const struct st_tracked_state st_bind_fs_ubos = {
   "st_bind_fs_ubos",
   {
      0,
      ST_NEW_FRAGMENT_PROGRAM | ST_NEW_UNIFORM_BUFFER,
   },
   bind_fs_ubos
};

static void bind_gs_ubos(struct st_context *st)
{
   struct gl_shader_program *prog =
      st->ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];

   if (!prog)
      return;

   st_bind_ubos(st, prog->_LinkedShaders[MESA_SHADER_GEOMETRY], PIPE_SHADER_GEOMETRY);
}

const struct st_tracked_state st_bind_gs_ubos = {
   "st_bind_gs_ubos",
   {
      0,
      ST_NEW_GEOMETRY_PROGRAM | ST_NEW_UNIFORM_BUFFER,
   },
   bind_gs_ubos
};
