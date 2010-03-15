/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul, Tungsten Graphics, Inc.
 * All Rights Reserved.
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
 * \mainpage
 *
 * Stand-alone Shading Language compiler.  
 * Basically, a command-line program which accepts GLSL shaders and emits
 * vertex/fragment programs (GPU instructions).
 *
 * This file is basically just a Mesa device driver but instead of building
 * a shared library we build an executable.
 *
 * We can emit programs in three different formats:
 *  1. ARB-style (GL_ARB_vertex/fragment_program)
 *  2. NV-style (GL_NV_vertex/fragment_program)
 *  3. debug-style (a slightly more sophisticated, internal format)
 *
 * Note that the ARB and NV program languages can't express all the
 * features that might be used by a fragment program (examples being
 * uniform and varying vars).  So, the ARB/NV programs that are
 * emitted aren't always legal programs in those languages.
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/framebuffer.h"
#include "main/shaders.h"
#include "shader/shader_api.h"
#include "shader/prog_print.h"
#include "drivers/common/driverfuncs.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "swrast/s_triangle.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"


static const char *Prog = "glslcompiler";


struct options {
   GLboolean LineNumbers;
   gl_prog_print_mode Mode;
   const char *VertFile;
   const char *FragFile;
   const char *OutputFile;
   GLboolean Params;
   struct gl_sl_pragmas Pragmas;
};

static struct options Options;


/**
 * GLSL compiler driver context. (kind of an artificial thing for now)
 */
struct compiler_context
{
   GLcontext MesaContext;
   int foo;
};

typedef struct compiler_context CompilerContext;



static void
UpdateState(GLcontext *ctx, GLuint new_state)
{
   /* easy - just propogate */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
}



static GLboolean
CreateContext(void)
{
   struct dd_function_table ddFuncs;
   GLvisual *vis;
   GLframebuffer *buf;
   GLcontext *ctx;
   CompilerContext *cc;

   vis = _mesa_create_visual(GL_FALSE, GL_FALSE, /* RGB */
                             8, 8, 8, 8,  /* color */
                             0, 0,  /* z, stencil */
                             0, 0, 0, 0, 1);  /* accum */
   buf = _mesa_create_framebuffer(vis);

   cc = calloc(1, sizeof(*cc));
   if (!vis || !buf || !cc) {
      if (vis)
         _mesa_destroy_visual(vis);
      if (buf)
         _mesa_destroy_framebuffer(buf);
      return GL_FALSE;
   }

   _mesa_init_driver_functions(&ddFuncs);
   ddFuncs.GetString = NULL;/*get_string;*/
   ddFuncs.UpdateState = UpdateState;
   ddFuncs.GetBufferSize = NULL;

   ctx = &cc->MesaContext;
   _mesa_initialize_context(ctx, vis, NULL, &ddFuncs, cc);
   _mesa_enable_sw_extensions(ctx);

   if (!_swrast_CreateContext( ctx ) ||
       !_vbo_CreateContext( ctx ) ||
       !_tnl_CreateContext( ctx ) ||
       !_swsetup_CreateContext( ctx )) {
      _mesa_destroy_visual(vis);
      _mesa_free_context_data(ctx);
      free(cc);
      return GL_FALSE;
   }
   TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;
   _swsetup_Wakeup( ctx );

   /* Override the context's default pragma settings */
   ctx->Shader.DefaultPragmas = Options.Pragmas;

   _mesa_make_current(ctx, buf, buf);

   return GL_TRUE;
}


static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;
   _mesa_ShaderSourceARB(shader, 1, (const GLchar **) &text, NULL);
   _mesa_CompileShaderARB(shader);
   _mesa_GetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      _mesa_GetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "%s: problem compiling shader: %s\n", Prog, log);
      exit(1);
   }
   else {
      printf("Shader compiled OK\n");
   }
}


/**
 * Read a shader from a file.
 */
static void
ReadShader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "%s: Unable to open shader file %s\n", Prog, filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   /*
   printf("%s: read %d bytes from shader file %s\n", Prog, n, filename);
   */
   if (n > 0) {
      buffer[n] = 0;
      LoadAndCompileShader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}


#if 0
static void
CheckLink(GLuint prog)
{
   GLint stat;
   _mesa_GetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      _mesa_GetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "%s: Linker error:\n%s\n", Prog, log);
   }
   else {
      fprintf(stderr, "%s: Link success!\n", Prog);
   }
}
#endif


static void
PrintShaderInstructions(GLuint shader, FILE *f)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   struct gl_program *prog = sh->Program;
   _mesa_fprint_program_opt(stdout, prog, Options.Mode, Options.LineNumbers);
   if (Options.Params)
      _mesa_print_program_parameters(ctx, prog);
}


static GLuint
CompileShader(const char *filename, GLenum type)
{
   GLuint shader;

   assert(type == GL_FRAGMENT_SHADER ||
          type == GL_VERTEX_SHADER);

   shader = _mesa_CreateShader(type);
   ReadShader(shader, filename);

   return shader;
}


static void
Usage(void)
{
   printf("Mesa GLSL stand-alone compiler\n");
   printf("Usage:\n");
   printf("  --vs FILE          vertex shader input filename\n");
   printf("  --fs FILE          fragment shader input filename\n");
   printf("  --arb              emit ARB-style instructions\n");
   printf("  --nv               emit NV-style instructions\n");
   printf("  --debug            force #pragma debug(on)\n");
   printf("  --nodebug          force #pragma debug(off)\n");
   printf("  --opt              force #pragma optimize(on)\n");
   printf("  --noopt            force #pragma optimize(off)\n");
   printf("  --number, -n       emit line numbers (if --arb or --nv)\n");
   printf("  --output, -o FILE  output filename\n");
   printf("  --params           also emit program parameter info\n");
   printf("  --help             display this information\n");
}


static void
ParseOptions(int argc, char *argv[])
{
   int i;

   Options.LineNumbers = GL_FALSE;
   Options.Mode = PROG_PRINT_DEBUG;
   Options.VertFile = NULL;
   Options.FragFile = NULL;
   Options.OutputFile = NULL;
   Options.Params = GL_FALSE;
   Options.Pragmas.IgnoreOptimize = GL_FALSE;
   Options.Pragmas.IgnoreDebug = GL_FALSE;
   Options.Pragmas.Debug = GL_FALSE;
   Options.Pragmas.Optimize = GL_TRUE;

   if (argc == 1) {
      Usage();
      exit(0);
   }

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--vs") == 0) {
         Options.VertFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--fs") == 0) {
         Options.FragFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--arb") == 0) {
         Options.Mode = PROG_PRINT_ARB;
      }
      else if (strcmp(argv[i], "--nv") == 0) {
         Options.Mode = PROG_PRINT_NV;
      }
      else if (strcmp(argv[i], "--debug") == 0) {
         Options.Pragmas.IgnoreDebug = GL_TRUE;
         Options.Pragmas.Debug = GL_TRUE;
      }
      else if (strcmp(argv[i], "--nodebug") == 0) {
         Options.Pragmas.IgnoreDebug = GL_TRUE;
         Options.Pragmas.Debug = GL_FALSE;
      }
      else if (strcmp(argv[i], "--opt") == 0) {
         Options.Pragmas.IgnoreOptimize = GL_TRUE;
         Options.Pragmas.Optimize = GL_TRUE;
      }
      else if (strcmp(argv[i], "--noopt") == 0) {
         Options.Pragmas.IgnoreOptimize = GL_TRUE;
         Options.Pragmas.Optimize = GL_FALSE;
      }
      else if (strcmp(argv[i], "--number") == 0 ||
               strcmp(argv[i], "-n") == 0) {
         Options.LineNumbers = GL_TRUE;
      }
      else if (strcmp(argv[i], "--output") == 0 ||
               strcmp(argv[i], "-o") == 0) {
         Options.OutputFile = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--params") == 0) {
         Options.Params = GL_TRUE;
      }
      else if (strcmp(argv[i], "--help") == 0) {
         Usage();
         exit(0);
      }
      else {
         printf("Unknown option: %s\n", argv[i]);
         Usage();
         exit(1);
      }
   }

   if (Options.Mode == PROG_PRINT_DEBUG) {
      /* always print line numbers when emitting debug-style output */
      Options.LineNumbers = GL_TRUE;
   }
}


int
main(int argc, char *argv[])
{
   GLuint shader = 0;

   ParseOptions(argc, argv);

   if (!CreateContext()) {
      fprintf(stderr, "%s: Failed to create compiler context\n", Prog);
      exit(1);
   }

   if (Options.VertFile) {
      shader = CompileShader(Options.VertFile, GL_VERTEX_SHADER);
   }
   else if (Options.FragFile) {
      shader = CompileShader(Options.FragFile, GL_FRAGMENT_SHADER);
   }

   if (shader) {
      if (Options.OutputFile) {
         fclose(stdout);
         /*stdout =*/ freopen(Options.OutputFile, "w", stdout);
      }
      if (stdout) {
         PrintShaderInstructions(shader, stdout);
      }
      if (Options.OutputFile) {
         fclose(stdout);
      }
   }

   return 0;
}
