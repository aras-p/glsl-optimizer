/* clang --emit-llvm llvm_entry.c |llvm-as |opt -std-compile-opts |llvm2cpp -for=Shader -gen-module -funcname=createBaseShader */
/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

/* clang --emit-llvm llvm_builtins.c |llvm-as |opt -std-compile-opts |llvm-dis */
typedef __attribute__(( ocu_vector_type(4) )) float float4;

void from_array(float4 (*res)[16], float (*ainputs)[16][4],
                int count, int num_attribs)
{
   for (int i = 0; i < count; ++i) {
      for (int j = 0; j < num_attribs; ++j) {
         float4 vec;
         vec.x = ainputs[i][j][0];
         vec.y = ainputs[i][j][1];
         vec.z = ainputs[i][j][2];
         vec.w = ainputs[i][j][3];
         res[i][j] = vec;
      }
   }
}

void from_consts(float4 *res, float (*ainputs)[4],
                 int count)
{
   for (int i = 0; i < count; ++i) {
      float4 vec;
      vec.x = ainputs[i][0];
      vec.y = ainputs[i][1];
      vec.z = ainputs[i][2];
      vec.w = ainputs[i][3];
      res[i] = vec;
   }
}

void to_array(float (*dests)[4], float4 *in, int num_attribs)
{
   for (int i = 0; i < num_attribs; ++i) {
      float  *rd = dests[i];
      float4  ri = in[i];
      rd[0] = ri.x;
      rd[1] = ri.y;
      rd[2] = ri.z;
      rd[3] = ri.w;
   }
}


struct ShaderInput
{
   float4  *dests;
   float4  *inputs;
   float4  *temps;
   float4  *consts;
   int     kilmask;
};

extern void execute_shader(struct ShaderInput *input);

void run_vertex_shader(void *inputs,
                       void *results,
                       float (*aconsts)[4],
                       int num_vertices,
                       int num_inputs,
                       int num_attribs,
                       int num_consts)
{
   float4  consts[32];
   float4  temps[128];//MAX_PROGRAM_TEMPS

   struct ShaderInput args;
   args.dests  = results;
   args.inputs = inputs;

   /*printf("XXX LLVM run_vertex_shader vertices = %d, inputs = %d, attribs = %d, consts = %d\n",
     num_vertices, num_inputs, num_attribs, num_consts);*/
   from_consts(consts, aconsts, num_consts);
   args.consts = consts;
   args.temps = temps;

   execute_shader(&args);
}


struct pipe_sampler_state;
struct softpipe_tile_cache;

#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */

struct tgsi_sampler
{
   const struct pipe_sampler_state *state;
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct tgsi_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       float lodbias,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
   void *pipe; /*XXX temporary*/
   struct softpipe_tile_cache *cache;
};


int run_fragment_shader(float x, float y,
                        float4 (*results)[16],
                        float4 (*inputs)[16],
                        int num_inputs,
                        float (*aconsts)[4],
                        int num_consts,
                        struct tgsi_sampler *samplers)
{
   float4  consts[32];
   float4  temps[128];//MAX_PROGRAM_TEMPS
   struct ShaderInput args;
   int mask = 0;
   args.kilmask = 0;

   from_consts(consts, aconsts, num_consts);
   args.consts = consts;
   args.temps = temps;
   //printf("AAAAAAAAAAAAAAAAAAAAAAA FRAGMENT SHADER %f %f\n", x, y);
   for (int i = 0; i < 4; ++i) {
      args.inputs  = inputs[i];
      args.dests   = results[i];
      mask = args.kilmask;
      args.kilmask = 0;
      execute_shader(&args);
      args.kilmask = mask | (args.kilmask << i);
   }
   return ~args.kilmask;
}

