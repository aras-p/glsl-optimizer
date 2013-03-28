/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
  
#include <pthread.h>
#include "main/imports.h"
#include "main/enums.h"
#include "main/shaderobj.h"
#include "program/prog_parameter.h"
#include "program/program.h"
#include "program/programopt.h"
#include "tnl/tnl.h"
#include "glsl/ralloc.h"

#include "brw_context.h"
#include "brw_wm.h"

static unsigned
get_new_program_id(struct intel_screen *screen)
{
   static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
   pthread_mutex_lock(&m);
   unsigned id = screen->program_id++;
   pthread_mutex_unlock(&m);
   return id;
}

static void brwBindProgram( struct gl_context *ctx,
			    GLenum target, 
			    struct gl_program *prog )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      break;
   }
}

static struct gl_program *brwNewProgram( struct gl_context *ctx,
				      GLenum target, 
				      GLuint id )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct brw_vertex_program *prog = CALLOC_STRUCT(brw_vertex_program);
      if (prog) {
	 prog->id = get_new_program_id(brw->intel.intelScreen);

	 return _mesa_init_vertex_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   case GL_FRAGMENT_PROGRAM_ARB: {
      struct brw_fragment_program *prog = CALLOC_STRUCT(brw_fragment_program);
      if (prog) {
	 prog->id = get_new_program_id(brw->intel.intelScreen);

	 return _mesa_init_fragment_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }
}

static void brwDeleteProgram( struct gl_context *ctx,
			      struct gl_program *prog )
{
   _mesa_delete_program( ctx, prog );
}


static GLboolean
brwIsProgramNative(struct gl_context *ctx,
		   GLenum target,
		   struct gl_program *prog)
{
   return true;
}

static GLboolean
brwProgramStringNotify(struct gl_context *ctx,
		       GLenum target,
		       struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct gl_fragment_program *fprog = (struct gl_fragment_program *) prog;
      struct brw_fragment_program *newFP = brw_fragment_program(fprog);
      const struct brw_fragment_program *curFP =
         brw_fragment_program_const(brw->fragment_program);

      if (newFP == curFP)
	 brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      newFP->id = get_new_program_id(brw->intel.intelScreen);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      struct brw_vertex_program *newVP = brw_vertex_program(vprog);
      const struct brw_vertex_program *curVP =
         brw_vertex_program_const(brw->vertex_program);

      if (newVP == curVP)
	 brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      if (newVP->program.IsPositionInvariant) {
	 _mesa_insert_mvp_code(ctx, &newVP->program);
      }
      newVP->id = get_new_program_id(brw->intel.intelScreen);

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }

   brw_add_texrect_params(prog);

   return true;
}

void
brw_add_texrect_params(struct gl_program *prog)
{
   for (int texunit = 0; texunit < BRW_MAX_TEX_UNIT; texunit++) {
      if (!(prog->TexturesUsed[texunit] & (1 << TEXTURE_RECT_INDEX)))
         continue;

      int tokens[STATE_LENGTH] = {
         STATE_INTERNAL,
         STATE_TEXRECT_SCALE,
         texunit,
         0,
         0
      };

      _mesa_add_state_reference(prog->Parameters, (gl_state_index *)tokens);
   }
}

/* Per-thread scratch space is a power-of-two multiple of 1KB. */
int
brw_get_scratch_size(int size)
{
   int i;

   for (i = 1024; i < size; i *= 2)
      ;

   return i;
}

void
brw_get_scratch_bo(struct intel_context *intel,
		   drm_intel_bo **scratch_bo, int size)
{
   drm_intel_bo *old_bo = *scratch_bo;

   if (old_bo && old_bo->size < size) {
      drm_intel_bo_unreference(old_bo);
      old_bo = NULL;
   }

   if (!old_bo) {
      *scratch_bo = drm_intel_bo_alloc(intel->bufmgr, "scratch bo", size, 4096);
   }
}

void brwInitFragProgFuncs( struct dd_function_table *functions )
{
   assert(functions->ProgramStringNotify == _tnl_program_string); 

   functions->BindProgram = brwBindProgram;
   functions->NewProgram = brwNewProgram;
   functions->DeleteProgram = brwDeleteProgram;
   functions->IsProgramNative = brwIsProgramNative;
   functions->ProgramStringNotify = brwProgramStringNotify;

   functions->NewShader = brw_new_shader;
   functions->NewShaderProgram = brw_new_shader_program;
   functions->LinkShader = brw_link_shader;
}

void
brw_init_shader_time(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   const int max_entries = 4096;
   brw->shader_time.bo = drm_intel_bo_alloc(intel->bufmgr, "shader time",
                                            max_entries * SHADER_TIME_STRIDE,
                                            4096);
   brw->shader_time.shader_programs = rzalloc_array(brw, struct gl_shader_program *,
                                                    max_entries);
   brw->shader_time.programs = rzalloc_array(brw, struct gl_program *,
                                             max_entries);
   brw->shader_time.types = rzalloc_array(brw, enum shader_time_shader_type,
                                          max_entries);
   brw->shader_time.cumulative = rzalloc_array(brw, uint64_t,
                                               max_entries);
   brw->shader_time.max_entries = max_entries;
}

static int
compare_time(const void *a, const void *b)
{
   uint64_t * const *a_val = a;
   uint64_t * const *b_val = b;

   /* We don't just subtract because we're turning the value to an int. */
   if (**a_val < **b_val)
      return -1;
   else if (**a_val == **b_val)
      return 0;
   else
      return 1;
}

static void
get_written_and_reset(struct brw_context *brw, int i,
                      uint64_t *written, uint64_t *reset)
{
   enum shader_time_shader_type type = brw->shader_time.types[i];
   assert(type == ST_VS || type == ST_FS8 || type == ST_FS16);

   /* Find where we recorded written and reset. */
   int wi, ri;

   for (wi = i; brw->shader_time.types[wi] != type + 1; wi++)
      ;

   for (ri = i; brw->shader_time.types[ri] != type + 2; ri++)
      ;

   *written = brw->shader_time.cumulative[wi];
   *reset = brw->shader_time.cumulative[ri];
}

static void
print_shader_time_line(const char *stage, const char *name,
                       int shader_num, uint64_t time, uint64_t total)
{
   printf("%-6s%-6s", stage, name);

   if (shader_num != -1)
      printf("%4d: ", shader_num);
   else
      printf("    : ");

   printf("%16lld (%7.2f Gcycles)      %4.1f%%\n",
          (long long)time,
          (double)time / 1000000000.0,
          (double)time / total * 100.0);
}

static void
brw_report_shader_time(struct brw_context *brw)
{
   if (!brw->shader_time.bo || !brw->shader_time.num_entries)
      return;

   uint64_t scaled[brw->shader_time.num_entries];
   uint64_t *sorted[brw->shader_time.num_entries];
   uint64_t total_by_type[ST_FS16 + 1];
   memset(total_by_type, 0, sizeof(total_by_type));
   double total = 0;
   for (int i = 0; i < brw->shader_time.num_entries; i++) {
      uint64_t written = 0, reset = 0;
      enum shader_time_shader_type type = brw->shader_time.types[i];

      sorted[i] = &scaled[i];

      switch (type) {
      case ST_VS_WRITTEN:
      case ST_VS_RESET:
      case ST_FS8_WRITTEN:
      case ST_FS8_RESET:
      case ST_FS16_WRITTEN:
      case ST_FS16_RESET:
         /* We'll handle these when along with the time. */
         scaled[i] = 0;
         continue;

      case ST_VS:
      case ST_FS8:
      case ST_FS16:
         get_written_and_reset(brw, i, &written, &reset);
         break;

      default:
         /* I sometimes want to print things that aren't the 3 shader times.
          * Just print the sum in that case.
          */
         written = 1;
         reset = 0;
         break;
      }

      uint64_t time = brw->shader_time.cumulative[i];
      if (written) {
         scaled[i] = time / written * (written + reset);
      } else {
         scaled[i] = time;
      }

      switch (type) {
      case ST_VS:
      case ST_FS8:
      case ST_FS16:
         total_by_type[type] += scaled[i];
         break;
      default:
         break;
      }

      total += scaled[i];
   }

   if (total == 0) {
      printf("No shader time collected yet\n");
      return;
   }

   qsort(sorted, brw->shader_time.num_entries, sizeof(sorted[0]), compare_time);

   printf("\n");
   printf("type          ID      cycles spent                   %% of total\n");
   for (int s = 0; s < brw->shader_time.num_entries; s++) {
      const char *shader_name;
      const char *stage;
      /* Work back from the sorted pointers times to a time to print. */
      int i = sorted[s] - scaled;

      if (scaled[i] == 0)
         continue;

      int shader_num = -1;
      if (brw->shader_time.shader_programs[i]) {
         shader_num = brw->shader_time.shader_programs[i]->Name;

         /* The fixed function fragment shader generates GLSL IR with a Name
          * of 0, and nothing else does.
          */
         if (shader_num == 0 &&
             (brw->shader_time.types[i] == ST_FS8 ||
              brw->shader_time.types[i] == ST_FS16)) {
            shader_name = "ff";
            shader_num = -1;
         } else {
            shader_name = "glsl";
         }
      } else if (brw->shader_time.programs[i]) {
         shader_num = brw->shader_time.programs[i]->Id;
         if (shader_num == 0) {
            shader_name = "ff";
            shader_num = -1;
         } else {
            shader_name = "prog";
         }
      } else {
         shader_name = "other";
      }

      switch (brw->shader_time.types[i]) {
      case ST_VS:
         stage = "vs";
         break;
      case ST_FS8:
         stage = "fs8";
         break;
      case ST_FS16:
         stage = "fs16";
         break;
      default:
         stage = "other";
         break;
      }

      print_shader_time_line(stage, shader_name, shader_num,
                             scaled[i], total);
   }

   printf("\n");
   print_shader_time_line("total", "vs", -1, total_by_type[ST_VS], total);
   print_shader_time_line("total", "fs8", -1, total_by_type[ST_FS8], total);
   print_shader_time_line("total", "fs16", -1, total_by_type[ST_FS16], total);
}

static void
brw_collect_shader_time(struct brw_context *brw)
{
   if (!brw->shader_time.bo)
      return;

   /* This probably stalls on the last rendering.  We could fix that by
    * delaying reading the reports, but it doesn't look like it's a big
    * overhead compared to the cost of tracking the time in the first place.
    */
   drm_intel_bo_map(brw->shader_time.bo, true);

   uint32_t *times = brw->shader_time.bo->virtual;

   for (int i = 0; i < brw->shader_time.num_entries; i++) {
      brw->shader_time.cumulative[i] += times[i * SHADER_TIME_STRIDE / 4];
   }

   /* Zero the BO out to clear it out for our next collection.
    */
   memset(times, 0, brw->shader_time.bo->size);
   drm_intel_bo_unmap(brw->shader_time.bo);
}

void
brw_collect_and_report_shader_time(struct brw_context *brw)
{
   brw_collect_shader_time(brw);

   if (brw->shader_time.report_time == 0 ||
       get_time() - brw->shader_time.report_time >= 1.0) {
      brw_report_shader_time(brw);
      brw->shader_time.report_time = get_time();
   }
}

/**
 * Chooses an index in the shader_time buffer and sets up tracking information
 * for our printouts.
 *
 * Note that this holds on to references to the underlying programs, which may
 * change their lifetimes compared to normal operation.
 */
int
brw_get_shader_time_index(struct brw_context *brw,
                          struct gl_shader_program *shader_prog,
                          struct gl_program *prog,
                          enum shader_time_shader_type type)
{
   struct gl_context *ctx = &brw->intel.ctx;

   int shader_time_index = brw->shader_time.num_entries++;
   assert(shader_time_index < brw->shader_time.max_entries);
   brw->shader_time.types[shader_time_index] = type;

   _mesa_reference_shader_program(ctx,
                                  &brw->shader_time.shader_programs[shader_time_index],
                                  shader_prog);

   _mesa_reference_program(ctx,
                           &brw->shader_time.programs[shader_time_index],
                           prog);

   return shader_time_index;
}

void
brw_destroy_shader_time(struct brw_context *brw)
{
   drm_intel_bo_unreference(brw->shader_time.bo);
   brw->shader_time.bo = NULL;
}
