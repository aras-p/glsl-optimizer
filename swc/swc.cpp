/*
Copyright (c) 2012, Adobe Systems Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution.

* Neither the name of Adobe Systems Incorporated nor the names of its 
contributors may be used to endorse or promote products derived from 
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_visitor.h"
#include "program.h"
#include "loop_analysis.h"
#include "standalone_scaffolding.h"
#include "glsl_optimizer.h"
#include <AS3/AS3.h>

extern bool glslOptimizerVerbose;

static void
initialize_context(struct gl_context *ctx, gl_api api)
{
   initialize_context_to_defaults(ctx, api);
   ctx->Const.GLSLVersion = 130;
   ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxDrawBuffers = 2;
   ctx->Const.MaxTextureCoordUnits = 4;
   ctx->Driver.NewShader = _mesa_new_shader;
}

static char* loadFile(const char* filename)
{
   FILE* file = fopen(filename, "rt");
   if( !file )
   {
      printf("Failed to open %s for reading\n", filename);
      return 0;
   }
   fprintf(stderr, "Compiling %s\n", filename);

   fseek(file, 0, SEEK_END);
   const int size = ftell(file);
   fseek(file, 0, SEEK_SET);

   char* result = new char[size+1];
   const int count = (int)fread(result, 1, size, file);
   result[count] = 0;

   fclose(file);
   return result;
}

static bool saveFile(const char* filename, const char* data)
{
   int size = (int)strlen(data);

   FILE* file = fopen(filename, "wt");
   if( !file )
   {
      printf( "Failed to open %s for writing\n", filename);
      return false;
   }

   if( 1 != fwrite(data,size,1,file) )
   {
      printf( "Failed to write to %s\n", filename);
      fclose(file);
      return false;
   }

   fclose(file);
   return true;
}

int
main(int argc, char **argv)
{
   #if CMDLINE

   bool vertexShader = false, freename = false, optimize = false, gles = false;
   const char* source = 0;
   char* dest = 0;

   for( int i=1; i < argc; i++ )
   {
      if( argv[i][0] == '-' )
      {
         if( 0 == strcmp("-e", argv[i]) )
            gles = true;
         if( 0 == strcmp("-d", argv[i]) )
            glslOptimizerVerbose = true;
         if( 0 == strcmp("-v", argv[i]) )
            vertexShader = true;
         if( 0 == strcmp("-f", argv[i]) )
            vertexShader = false;
         if( 0 == strcmp("-optimize", argv[i]) )
            optimize = true;
      }
      else
      {
         if( source == 0 )
            source = argv[i];
         else if( dest == 0 )
            dest = argv[i];
      }
   }

   if( !source )
      abort();

   if ( !dest ) {
      dest = (char *) calloc(strlen(source)+5, sizeof(char));
      snprintf(dest, strlen(source)+5, "%s.out", source);
      freename = true;
   }

   const char* originalShader = loadFile(source);
   if( !originalShader )
      abort();

   AS3_DeclareVar(srcstr, String);
   AS3_CopyCStringToVar(srcstr, originalShader, strlen(originalShader));

   inline_as3(
      "srcstr = compileShader(srcstr, %0, %1, %2)\n"
      : : "r"(vertexShader ? 0 : 1), "r"(optimize), "r"(gles)
   );

   char *optimizedShader;
   AS3_MallocString(optimizedShader, srcstr);

   if( !saveFile(dest, optimizedShader) )
      abort();

   return 0;

   #endif

   AS3_GoAsync();
}

extern "C" void compileShader()  __attribute__((used, annotate("as3sig:public function compileShader(src:String, mode:int, optimize:Boolean, gles:Boolean = false):String")));

extern "C" void compileShader()
{
   // Copy the AS3 string to the C heap (must be free'd later)
   char *src = NULL;
   AS3_MallocString(src, src);

   bool gles = false;
   AS3_CopyScalarToVar(gles, gles);


   glslopt_ctx* gContext = glslopt_initialize(gles);

   int mode;
   AS3_GetScalarFromVar(mode, mode);
   const glslopt_shader_type type = mode == 0 ? kGlslOptShaderVertex : kGlslOptShaderFragment;

   glslopt_shader* shader = glslopt_optimize(gContext, type, src, 0);

   const char* optimizedShader = glslopt_get_output(shader);
   AS3_DeclareVar(outputstr, String);
   AS3_CopyCStringToVar(outputstr, optimizedShader, strlen(optimizedShader));

   glslopt_cleanup(gContext);

   inline_as3(
      "import com.adobe.AGALOptimiser.translator.transformations.Utils;\n"
      "if(optimize) {\n"
      "    var shader:Object = null;\n"
      "    try { shader = JSON.parse(outputstr) } catch(e:*) { }\n"
      "    if(shader != null && shader[\"agalasm\"] != null) {\n"
      "        shader = Utils.optimizeShader(shader, mode == 0)\n"
      "        outputstr = JSON.stringify(shader, null, 1)\n"
      "    }\n"
      "}\n"
   );

   AS3_ReturnAS3Var(outputstr);
}
