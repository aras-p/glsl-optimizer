/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_fs.h"
#include "brw_vec4.h"
#include "glsl/glsl_types.h"
#include "glsl/ir_optimization.h"

using namespace brw;

/** @file brw_fs_schedule_instructions.cpp
 *
 * List scheduling of FS instructions.
 *
 * The basic model of the list scheduler is to take a basic block,
 * compute a DAG of the dependencies (RAW ordering with latency, WAW
 * ordering with latency, WAR ordering), and make a list of the DAG heads.
 * Heuristically pick a DAG head, then put all the children that are
 * now DAG heads into the list of things to schedule.
 *
 * The heuristic is the important part.  We're trying to be cheap,
 * since actually computing the optimal scheduling is NP complete.
 * What we do is track a "current clock".  When we schedule a node, we
 * update the earliest-unblocked clock time of its children, and
 * increment the clock.  Then, when trying to schedule, we just pick
 * the earliest-unblocked instruction to schedule.
 *
 * Note that often there will be many things which could execute
 * immediately, and there are a range of heuristic options to choose
 * from in picking among those.
 */

static bool debug = false;

class instruction_scheduler;

class schedule_node : public exec_node
{
public:
   schedule_node(backend_instruction *inst, instruction_scheduler *sched);
   void set_latency_gen4();
   void set_latency_gen7(bool is_haswell);

   backend_instruction *inst;
   schedule_node **children;
   int *child_latency;
   int child_count;
   int parent_count;
   int child_array_size;
   int unblocked_time;
   int latency;

   /**
    * Which iteration of pushing groups of children onto the candidates list
    * this node was a part of.
    */
   unsigned cand_generation;

   /**
    * This is the sum of the instruction's latency plus the maximum delay of
    * its children, or just the issue_time if it's a leaf node.
    */
   int delay;
};

void
schedule_node::set_latency_gen4()
{
   int chans = 8;
   int math_latency = 22;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
      this->latency = 1 * chans * math_latency;
      break;
   case SHADER_OPCODE_RSQ:
      this->latency = 2 * chans * math_latency;
      break;
   case SHADER_OPCODE_INT_QUOTIENT:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_LOG2:
      /* full precision log.  partial is 2. */
      this->latency = 3 * chans * math_latency;
      break;
   case SHADER_OPCODE_INT_REMAINDER:
   case SHADER_OPCODE_EXP2:
      /* full precision.  partial is 3, same throughput. */
      this->latency = 4 * chans * math_latency;
      break;
   case SHADER_OPCODE_POW:
      this->latency = 8 * chans * math_latency;
      break;
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      /* minimum latency, max is 12 rounds. */
      this->latency = 5 * chans * math_latency;
      break;
   default:
      this->latency = 2;
      break;
   }
}

void
schedule_node::set_latency_gen7(bool is_haswell)
{
   switch (inst->opcode) {
   case BRW_OPCODE_MAD:
      /* 2 cycles
       *  (since the last two src operands are in different register banks):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 3 cycles on IVB, 4 on HSW
       *  (since the last two src operands are in the same register bank):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 18 cycles on IVB, 16 on HSW
       *  (since the last two src operands are in different register banks):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,5,1>F                     { align16 WE_normal 1Q };
       *
       * 20 cycles on IVB, 18 on HSW
       *  (since the last two src operands are in the same register bank):
       * mad(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       */

      /* Our register allocator doesn't know about register banks, so use the
       * higher latency.
       */
      latency = is_haswell ? 16 : 18;
      break;

   case BRW_OPCODE_LRP:
      /* 2 cycles
       *  (since the last two src operands are in different register banks):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 3 cycles on IVB, 4 on HSW
       *  (since the last two src operands are in the same register bank):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       *
       * 16 cycles on IVB, 14 on HSW
       *  (since the last two src operands are in different register banks):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g3.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       *
       * 16 cycles
       *  (since the last two src operands are in the same register bank):
       * lrp(8) g4<1>F g2.2<4,4,1>F.x  g2<4,4,1>F.x g2.1<4,4,1>F.x { align16 WE_normal 1Q };
       * mov(8) null   g4<4,4,1>F                     { align16 WE_normal 1Q };
       */

      /* Our register allocator doesn't know about register banks, so use the
       * higher latency.
       */
      latency = 14;
      break;

   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      /* 2 cycles:
       * math inv(8) g4<1>F g2<0,1,0>F      null       { align1 WE_normal 1Q };
       *
       * 18 cycles:
       * math inv(8) g4<1>F g2<0,1,0>F      null       { align1 WE_normal 1Q };
       * mov(8)      null   g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * Same for exp2, log2, rsq, sqrt, sin, cos.
       */
      latency = is_haswell ? 14 : 16;
      break;

   case SHADER_OPCODE_POW:
      /* 2 cycles:
       * math pow(8) g4<1>F g2<0,1,0>F   g2.1<0,1,0>F  { align1 WE_normal 1Q };
       *
       * 26 cycles:
       * math pow(8) g4<1>F g2<0,1,0>F   g2.1<0,1,0>F  { align1 WE_normal 1Q };
       * mov(8)      null   g4<8,8,1>F                 { align1 WE_normal 1Q };
       */
      latency = is_haswell ? 22 : 24;
      break;

   case SHADER_OPCODE_TEX:
   case SHADER_OPCODE_TXD:
   case SHADER_OPCODE_TXF:
   case SHADER_OPCODE_TXL:
      /* 18 cycles:
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       *
       * 697 +/-49 cycles (min 610, n=26):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * So the latency on our first texture load of the batchbuffer takes
       * ~700 cycles, since the caches are cold at that point.
       *
       * 840 +/- 92 cycles (min 720, n=25):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * On the second load, it takes just an extra ~140 cycles, and after
       * accounting for the 14 cycles of the MOV's latency, that makes ~130.
       *
       * 683 +/- 49 cycles (min = 602, n=47):
       * mov(8)  g115<1>F   0F                         { align1 WE_normal 1Q };
       * mov(8)  g114<1>F   0F                         { align1 WE_normal 1Q };
       * send(8) g4<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * send(8) g50<1>UW   g114<8,8,1>F
       *   sampler (10, 0, 0, 1) mlen 2 rlen 4         { align1 WE_normal 1Q };
       * mov(8)  null       g4<8,8,1>F                 { align1 WE_normal 1Q };
       *
       * The unit appears to be pipelined, since this matches up with the
       * cache-cold case, despite there being two loads here.  If you replace
       * the g4 in the MOV to null with g50, it's still 693 +/- 52 (n=39).
       *
       * So, take some number between the cache-hot 140 cycles and the
       * cache-cold 700 cycles.  No particular tuning was done on this.
       *
       * I haven't done significant testing of the non-TEX opcodes.  TXL at
       * least looked about the same as TEX.
       */
      latency = 200;
      break;

   case SHADER_OPCODE_TXS:
      /* Testing textureSize(sampler2D, 0), one load was 420 +/- 41
       * cycles (n=15):
       * mov(8)   g114<1>UD  0D                        { align1 WE_normal 1Q };
       * send(8)  g6<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 1) mlen 1 rlen 4        { align1 WE_normal 1Q };
       * mov(16)  g6<1>F     g6<8,8,1>D                { align1 WE_normal 1Q };
       *
       *
       * Two loads was 535 +/- 30 cycles (n=19):
       * mov(16)   g114<1>UD  0D                       { align1 WE_normal 1H };
       * send(16)  g6<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 2) mlen 2 rlen 8        { align1 WE_normal 1H };
       * mov(16)   g114<1>UD  0D                       { align1 WE_normal 1H };
       * mov(16)   g6<1>F     g6<8,8,1>D               { align1 WE_normal 1H };
       * send(16)  g8<1>UW    g114<8,8,1>F
       *   sampler (10, 0, 10, 2) mlen 2 rlen 8        { align1 WE_normal 1H };
       * mov(16)   g8<1>F     g8<8,8,1>D               { align1 WE_normal 1H };
       * add(16)   g6<1>F     g6<8,8,1>F   g8<8,8,1>F  { align1 WE_normal 1H };
       *
       * Since the only caches that should matter are just the
       * instruction/state cache containing the surface state, assume that we
       * always have hot caches.
       */
      latency = 100;
      break;

   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD:
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
   case VS_OPCODE_PULL_CONSTANT_LOAD:
      /* testing using varying-index pull constants:
       *
       * 16 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       *
       * ~480 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       *
       * ~620 cycles:
       * mov(8)  g4<1>D  g2.1<0,1,0>F                  { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       * send(8) g4<1>F  g4<8,8,1>D
       *   data (9, 2, 3) mlen 1 rlen 1                { align1 WE_normal 1Q };
       * mov(8)  null    g4<8,8,1>F                    { align1 WE_normal 1Q };
       *
       * So, if it's cache-hot, it's about 140.  If it's cache cold, it's
       * about 460.  We expect to mostly be cache hot, so pick something more
       * in that direction.
       */
      latency = 200;
      break;

   case SHADER_OPCODE_GEN7_SCRATCH_READ:
      /* Testing a load from offset 0, that had been previously written:
       *
       * send(8) g114<1>UW g0<8,8,1>F data (0, 0, 0) mlen 1 rlen 1 { align1 WE_normal 1Q };
       * mov(8)  null      g114<8,8,1>F { align1 WE_normal 1Q };
       *
       * The cycles spent seemed to be grouped around 40-50 (as low as 38),
       * then around 140.  Presumably this is cache hit vs miss.
       */
      latency = 50;
      break;

   case SHADER_OPCODE_UNTYPED_ATOMIC:
      /* Test code:
       *   mov(8)    g112<1>ud       0x00000000ud       { align1 WE_all 1Q };
       *   mov(1)    g112.7<1>ud     g1.7<0,1,0>ud      { align1 WE_all };
       *   mov(8)    g113<1>ud       0x00000000ud       { align1 WE_normal 1Q };
       *   send(8)   g4<1>ud         g112<8,8,1>ud
       *             data (38, 5, 6) mlen 2 rlen 1      { align1 WE_normal 1Q };
       *
       * Running it 100 times as fragment shader on a 128x128 quad
       * gives an average latency of 13867 cycles per atomic op,
       * standard deviation 3%.  Note that this is a rather
       * pessimistic estimate, the actual latency in cases with few
       * collisions between threads and favorable pipelining has been
       * seen to be reduced by a factor of 100.
       */
      latency = 14000;
      break;

   case SHADER_OPCODE_UNTYPED_SURFACE_READ:
      /* Test code:
       *   mov(8)    g112<1>UD       0x00000000UD       { align1 WE_all 1Q };
       *   mov(1)    g112.7<1>UD     g1.7<0,1,0>UD      { align1 WE_all };
       *   mov(8)    g113<1>UD       0x00000000UD       { align1 WE_normal 1Q };
       *   send(8)   g4<1>UD         g112<8,8,1>UD
       *             data (38, 6, 5) mlen 2 rlen 1      { align1 WE_normal 1Q };
       *   .
       *   . [repeats 8 times]
       *   .
       *   mov(8)    g112<1>UD       0x00000000UD       { align1 WE_all 1Q };
       *   mov(1)    g112.7<1>UD     g1.7<0,1,0>UD      { align1 WE_all };
       *   mov(8)    g113<1>UD       0x00000000UD       { align1 WE_normal 1Q };
       *   send(8)   g4<1>UD         g112<8,8,1>UD
       *             data (38, 6, 5) mlen 2 rlen 1      { align1 WE_normal 1Q };
       *
       * Running it 100 times as fragment shader on a 128x128 quad
       * gives an average latency of 583 cycles per surface read,
       * standard deviation 0.9%.
       */
      latency = is_haswell ? 300 : 600;
      break;

   default:
      /* 2 cycles:
       * mul(8) g4<1>F g2<0,1,0>F      0.5F            { align1 WE_normal 1Q };
       *
       * 16 cycles:
       * mul(8) g4<1>F g2<0,1,0>F      0.5F            { align1 WE_normal 1Q };
       * mov(8) null   g4<8,8,1>F                      { align1 WE_normal 1Q };
       */
      latency = 14;
      break;
   }
}

class instruction_scheduler {
public:
   instruction_scheduler(backend_visitor *v, int grf_count,
                         instruction_scheduler_mode mode)
   {
      this->bv = v;
      this->mem_ctx = ralloc_context(NULL);
      this->grf_count = grf_count;
      this->instructions.make_empty();
      this->instructions_to_schedule = 0;
      this->post_reg_alloc = (mode == SCHEDULE_POST);
      this->mode = mode;
      this->time = 0;
      if (!post_reg_alloc) {
         this->remaining_grf_uses = rzalloc_array(mem_ctx, int, grf_count);
         this->grf_active = rzalloc_array(mem_ctx, bool, grf_count);
      } else {
         this->remaining_grf_uses = NULL;
         this->grf_active = NULL;
      }
   }

   ~instruction_scheduler()
   {
      ralloc_free(this->mem_ctx);
   }
   void add_barrier_deps(schedule_node *n);
   void add_dep(schedule_node *before, schedule_node *after, int latency);
   void add_dep(schedule_node *before, schedule_node *after);

   void run(exec_list *instructions);
   void add_inst(backend_instruction *inst);
   void compute_delay(schedule_node *node);
   virtual void calculate_deps() = 0;
   virtual schedule_node *choose_instruction_to_schedule() = 0;

   /**
    * Returns how many cycles it takes the instruction to issue.
    *
    * Instructions in gen hardware are handled one simd4 vector at a time,
    * with 1 cycle per vector dispatched.  Thus SIMD8 pixel shaders take 2
    * cycles to dispatch and SIMD16 (compressed) instructions take 4.
    */
   virtual int issue_time(backend_instruction *inst) = 0;

   virtual void count_remaining_grf_uses(backend_instruction *inst) = 0;
   virtual void update_register_pressure(backend_instruction *inst) = 0;
   virtual int get_register_pressure_benefit(backend_instruction *inst) = 0;

   void schedule_instructions(backend_instruction *next_block_header);

   void *mem_ctx;

   bool post_reg_alloc;
   int instructions_to_schedule;
   int grf_count;
   int time;
   exec_list instructions;
   backend_visitor *bv;

   instruction_scheduler_mode mode;

   /**
    * Number of instructions left to schedule that reference each vgrf.
    *
    * Used so that we can prefer scheduling instructions that will end the
    * live intervals of multiple variables, to reduce register pressure.
    */
   int *remaining_grf_uses;

   /**
    * Tracks whether each VGRF has had an instruction scheduled that uses it.
    *
    * This is used to estimate whether scheduling a new instruction will
    * increase register pressure.
    */
   bool *grf_active;
};

class fs_instruction_scheduler : public instruction_scheduler
{
public:
   fs_instruction_scheduler(fs_visitor *v, int grf_count,
                            instruction_scheduler_mode mode);
   void calculate_deps();
   bool is_compressed(fs_inst *inst);
   schedule_node *choose_instruction_to_schedule();
   int issue_time(backend_instruction *inst);
   fs_visitor *v;

   void count_remaining_grf_uses(backend_instruction *inst);
   void update_register_pressure(backend_instruction *inst);
   int get_register_pressure_benefit(backend_instruction *inst);
};

fs_instruction_scheduler::fs_instruction_scheduler(fs_visitor *v,
                                                   int grf_count,
                                                   instruction_scheduler_mode mode)
   : instruction_scheduler(v, grf_count, mode),
     v(v)
{
}

void
fs_instruction_scheduler::count_remaining_grf_uses(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;

   if (!remaining_grf_uses)
      return;

   if (inst->dst.file == GRF)
      remaining_grf_uses[inst->dst.reg]++;

   for (int i = 0; i < 3; i++) {
      if (inst->src[i].file != GRF)
         continue;

      remaining_grf_uses[inst->src[i].reg]++;
   }
}

void
fs_instruction_scheduler::update_register_pressure(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;

   if (!remaining_grf_uses)
      return;

   if (inst->dst.file == GRF) {
      remaining_grf_uses[inst->dst.reg]--;
      grf_active[inst->dst.reg] = true;
   }

   for (int i = 0; i < 3; i++) {
      if (inst->src[i].file == GRF) {
         remaining_grf_uses[inst->src[i].reg]--;
         grf_active[inst->src[i].reg] = true;
      }
   }
}

int
fs_instruction_scheduler::get_register_pressure_benefit(backend_instruction *be)
{
   fs_inst *inst = (fs_inst *)be;
   int benefit = 0;

   if (inst->dst.file == GRF) {
      if (remaining_grf_uses[inst->dst.reg] == 1)
         benefit += v->virtual_grf_sizes[inst->dst.reg];
      if (!grf_active[inst->dst.reg])
         benefit -= v->virtual_grf_sizes[inst->dst.reg];
   }

   for (int i = 0; i < 3; i++) {
      if (inst->src[i].file != GRF)
         continue;

      if (remaining_grf_uses[inst->src[i].reg] == 1)
         benefit += v->virtual_grf_sizes[inst->src[i].reg];
      if (!grf_active[inst->src[i].reg])
         benefit -= v->virtual_grf_sizes[inst->src[i].reg];
   }

   return benefit;
}

class vec4_instruction_scheduler : public instruction_scheduler
{
public:
   vec4_instruction_scheduler(vec4_visitor *v, int grf_count);
   void calculate_deps();
   schedule_node *choose_instruction_to_schedule();
   int issue_time(backend_instruction *inst);
   vec4_visitor *v;

   void count_remaining_grf_uses(backend_instruction *inst);
   void update_register_pressure(backend_instruction *inst);
   int get_register_pressure_benefit(backend_instruction *inst);
};

vec4_instruction_scheduler::vec4_instruction_scheduler(vec4_visitor *v,
                                                       int grf_count)
   : instruction_scheduler(v, grf_count, SCHEDULE_POST),
     v(v)
{
}

void
vec4_instruction_scheduler::count_remaining_grf_uses(backend_instruction *be)
{
}

void
vec4_instruction_scheduler::update_register_pressure(backend_instruction *be)
{
}

int
vec4_instruction_scheduler::get_register_pressure_benefit(backend_instruction *be)
{
   return 0;
}

schedule_node::schedule_node(backend_instruction *inst,
                             instruction_scheduler *sched)
{
   struct brw_context *brw = sched->bv->brw;

   this->inst = inst;
   this->child_array_size = 0;
   this->children = NULL;
   this->child_latency = NULL;
   this->child_count = 0;
   this->parent_count = 0;
   this->unblocked_time = 0;
   this->cand_generation = 0;
   this->delay = 0;

   /* We can't measure Gen6 timings directly but expect them to be much
    * closer to Gen7 than Gen4.
    */
   if (!sched->post_reg_alloc)
      this->latency = 1;
   else if (brw->gen >= 6)
      set_latency_gen7(brw->is_haswell);
   else
      set_latency_gen4();
}

void
instruction_scheduler::add_inst(backend_instruction *inst)
{
   schedule_node *n = new(mem_ctx) schedule_node(inst, this);

   assert(!inst->is_head_sentinel());
   assert(!inst->is_tail_sentinel());

   this->instructions_to_schedule++;

   inst->remove();
   instructions.push_tail(n);
}

/** Recursive computation of the delay member of a node. */
void
instruction_scheduler::compute_delay(schedule_node *n)
{
   if (!n->child_count) {
      n->delay = issue_time(n->inst);
   } else {
      for (int i = 0; i < n->child_count; i++) {
         if (!n->children[i]->delay)
            compute_delay(n->children[i]);
         n->delay = MAX2(n->delay, n->latency + n->children[i]->delay);
      }
   }
}

/**
 * Add a dependency between two instruction nodes.
 *
 * The @after node will be scheduled after @before.  We will try to
 * schedule it @latency cycles after @before, but no guarantees there.
 */
void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after,
			       int latency)
{
   if (!before || !after)
      return;

   assert(before != after);

   for (int i = 0; i < before->child_count; i++) {
      if (before->children[i] == after) {
	 before->child_latency[i] = MAX2(before->child_latency[i], latency);
	 return;
      }
   }

   if (before->child_array_size <= before->child_count) {
      if (before->child_array_size < 16)
	 before->child_array_size = 16;
      else
	 before->child_array_size *= 2;

      before->children = reralloc(mem_ctx, before->children,
				  schedule_node *,
				  before->child_array_size);
      before->child_latency = reralloc(mem_ctx, before->child_latency,
				       int, before->child_array_size);
   }

   before->children[before->child_count] = after;
   before->child_latency[before->child_count] = latency;
   before->child_count++;
   after->parent_count++;
}

void
instruction_scheduler::add_dep(schedule_node *before, schedule_node *after)
{
   if (!before)
      return;

   add_dep(before, after, before->latency);
}

/**
 * Sometimes we really want this node to execute after everything that
 * was before it and before everything that followed it.  This adds
 * the deps to do so.
 */
void
instruction_scheduler::add_barrier_deps(schedule_node *n)
{
   schedule_node *prev = (schedule_node *)n->prev;
   schedule_node *next = (schedule_node *)n->next;

   if (prev) {
      while (!prev->is_head_sentinel()) {
	 add_dep(prev, n, 0);
	 prev = (schedule_node *)prev->prev;
      }
   }

   if (next) {
      while (!next->is_tail_sentinel()) {
	 add_dep(n, next, 0);
	 next = (schedule_node *)next->next;
      }
   }
}

/* instruction scheduling needs to be aware of when an MRF write
 * actually writes 2 MRFs.
 */
bool
fs_instruction_scheduler::is_compressed(fs_inst *inst)
{
   return (v->dispatch_width == 16 &&
	   !inst->force_uncompressed &&
	   !inst->force_sechalf);
}

void
fs_instruction_scheduler::calculate_deps()
{
   const bool gen6plus = v->brw->gen >= 6;

   /* Pre-register-allocation, this tracks the last write per VGRF (so
    * different reg_offsets within it can interfere when they shouldn't).
    * After register allocation, reg_offsets are gone and we track individual
    * GRF registers.
    */
   schedule_node *last_grf_write[grf_count];
   schedule_node *last_mrf_write[BRW_MAX_MRF];
   schedule_node *last_conditional_mod[2] = { NULL, NULL };
   schedule_node *last_accumulator_write = NULL;
   /* Fixed HW registers are assumed to be separate from the virtual
    * GRFs, so they can be tracked separately.  We don't really write
    * to fixed GRFs much, so don't bother tracking them on a more
    * granular level.
    */
   schedule_node *last_fixed_grf_write = NULL;
   int reg_width = v->dispatch_width / 8;

   /* The last instruction always needs to still be the last
    * instruction.  Either it's flow control (IF, ELSE, ENDIF, DO,
    * WHILE) and scheduling other things after it would disturb the
    * basic block, or it's FB_WRITE and we should do a better job at
    * dead code elimination anyway.
    */
   schedule_node *last = (schedule_node *)instructions.get_tail();
   add_barrier_deps(last);

   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));

   /* top-to-bottom dependencies: RAW and WAW. */
   foreach_list(node, &instructions) {
      schedule_node *n = (schedule_node *)node;
      fs_inst *inst = (fs_inst *)n->inst;

      if (inst->opcode == FS_OPCODE_PLACEHOLDER_HALT ||
         inst->has_side_effects())
         add_barrier_deps(n);

      /* read-after-write deps. */
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
            if (post_reg_alloc) {
               for (int r = 0; r < reg_width * inst->regs_read(v, i); r++)
                  add_dep(last_grf_write[inst->src[i].reg + r], n);
            } else {
               add_dep(last_grf_write[inst->src[i].reg], n);
            }
	 } else if (inst->src[i].file == HW_REG &&
		    (inst->src[i].fixed_hw_reg.file ==
		     BRW_GENERAL_REGISTER_FILE)) {
	    if (post_reg_alloc) {
               int size = reg_width;
               if (inst->src[i].fixed_hw_reg.vstride == BRW_VERTICAL_STRIDE_0)
                  size = 1;
               for (int r = 0; r < size; r++)
                  add_dep(last_grf_write[inst->src[i].fixed_hw_reg.nr + r], n);
            } else {
               add_dep(last_fixed_grf_write, n);
            }
         } else if (inst->src[i].is_accumulator() && gen6plus) {
            add_dep(last_accumulator_write, n);
	 } else if (inst->src[i].file != BAD_FILE &&
		    inst->src[i].file != IMM &&
		    inst->src[i].file != UNIFORM) {
	    assert(inst->src[i].file != MRF);
	    add_barrier_deps(n);
	 }
      }

      if (inst->base_mrf != -1) {
	 for (int i = 0; i < inst->mlen; i++) {
	    /* It looks like the MRF regs are released in the send
	     * instruction once it's sent, not when the result comes
	     * back.
	     */
	    add_dep(last_mrf_write[inst->base_mrf + i], n);
	 }
      }

      if (inst->reads_flag()) {
	 add_dep(last_conditional_mod[inst->flag_subreg], n);
      }

      if (inst->reads_accumulator_implicitly()) {
         if (gen6plus) {
            add_dep(last_accumulator_write, n);
         } else {
            add_barrier_deps(n);
         }
      }

      /* write-after-write deps. */
      if (inst->dst.file == GRF) {
         if (post_reg_alloc) {
            for (int r = 0; r < inst->regs_written * reg_width; r++) {
               add_dep(last_grf_write[inst->dst.reg + r], n);
               last_grf_write[inst->dst.reg + r] = n;
            }
         } else {
            add_dep(last_grf_write[inst->dst.reg], n);
            last_grf_write[inst->dst.reg] = n;
         }
      } else if (inst->dst.file == MRF) {
	 int reg = inst->dst.reg & ~BRW_MRF_COMPR4;

	 add_dep(last_mrf_write[reg], n);
	 last_mrf_write[reg] = n;
	 if (is_compressed(inst)) {
	    if (inst->dst.reg & BRW_MRF_COMPR4)
	       reg += 4;
	    else
	       reg++;
	    add_dep(last_mrf_write[reg], n);
	    last_mrf_write[reg] = n;
	 }
      } else if (inst->dst.file == HW_REG &&
		 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         if (post_reg_alloc) {
            for (int r = 0; r < reg_width; r++)
               last_grf_write[inst->dst.fixed_hw_reg.nr + r] = n;
         } else {
            last_fixed_grf_write = n;
         }
      } else if (inst->dst.is_accumulator() && gen6plus) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      } else if (inst->dst.file != BAD_FILE) {
	 add_barrier_deps(n);
      }

      if (inst->mlen > 0 && inst->base_mrf != -1) {
	 for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
	    add_dep(last_mrf_write[inst->base_mrf + i], n);
	    last_mrf_write[inst->base_mrf + i] = n;
	 }
      }

      if (inst->writes_flag()) {
	 add_dep(last_conditional_mod[inst->flag_subreg], n, 0);
	 last_conditional_mod[inst->flag_subreg] = n;
      }

      if (inst->writes_accumulator) {
         if (gen6plus) {
            add_dep(last_accumulator_write, n);
            last_accumulator_write = n;
         } else {
            add_barrier_deps(n);
         }
      }
   }

   /* bottom-to-top dependencies: WAR */
   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));
   memset(last_conditional_mod, 0, sizeof(last_conditional_mod));
   last_accumulator_write = NULL;
   last_fixed_grf_write = NULL;

   exec_node *node;
   exec_node *prev;
   for (node = instructions.get_tail(), prev = node->prev;
	!node->is_head_sentinel();
	node = prev, prev = node->prev) {
      schedule_node *n = (schedule_node *)node;
      fs_inst *inst = (fs_inst *)n->inst;

      /* write-after-read deps. */
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
            if (post_reg_alloc) {
               for (int r = 0; r < reg_width * inst->regs_read(v, i); r++)
                  add_dep(n, last_grf_write[inst->src[i].reg + r]);
            } else {
               add_dep(n, last_grf_write[inst->src[i].reg]);
            }
	 } else if (inst->src[i].file == HW_REG &&
		    (inst->src[i].fixed_hw_reg.file ==
		     BRW_GENERAL_REGISTER_FILE)) {
	    if (post_reg_alloc) {
               int size = reg_width;
               if (inst->src[i].fixed_hw_reg.vstride == BRW_VERTICAL_STRIDE_0)
                  size = 1;
               for (int r = 0; r < size; r++)
                  add_dep(n, last_grf_write[inst->src[i].fixed_hw_reg.nr + r]);
            } else {
               add_dep(n, last_fixed_grf_write);
            }
         } else if (inst->src[i].is_accumulator() && gen6plus) {
            add_dep(n, last_accumulator_write);
         } else if (inst->src[i].file != BAD_FILE &&
		    inst->src[i].file != IMM &&
		    inst->src[i].file != UNIFORM) {
	    assert(inst->src[i].file != MRF);
	    add_barrier_deps(n);
	 }
      }

      if (inst->base_mrf != -1) {
	 for (int i = 0; i < inst->mlen; i++) {
	    /* It looks like the MRF regs are released in the send
	     * instruction once it's sent, not when the result comes
	     * back.
	     */
	    add_dep(n, last_mrf_write[inst->base_mrf + i], 2);
	 }
      }

      if (inst->reads_flag()) {
	 add_dep(n, last_conditional_mod[inst->flag_subreg]);
      }

      if (inst->reads_accumulator_implicitly()) {
         if (gen6plus) {
            add_dep(n, last_accumulator_write);
         } else {
            add_barrier_deps(n);
         }
      }

      /* Update the things this instruction wrote, so earlier reads
       * can mark this as WAR dependency.
       */
      if (inst->dst.file == GRF) {
         if (post_reg_alloc) {
            for (int r = 0; r < inst->regs_written * reg_width; r++)
               last_grf_write[inst->dst.reg + r] = n;
         } else {
            last_grf_write[inst->dst.reg] = n;
         }
      } else if (inst->dst.file == MRF) {
	 int reg = inst->dst.reg & ~BRW_MRF_COMPR4;

	 last_mrf_write[reg] = n;

	 if (is_compressed(inst)) {
	    if (inst->dst.reg & BRW_MRF_COMPR4)
	       reg += 4;
	    else
	       reg++;

	    last_mrf_write[reg] = n;
	 }
      } else if (inst->dst.file == HW_REG &&
		 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         if (post_reg_alloc) {
            for (int r = 0; r < reg_width; r++)
               last_grf_write[inst->dst.fixed_hw_reg.nr + r] = n;
         } else {
            last_fixed_grf_write = n;
         }
      } else if (inst->dst.is_accumulator() && gen6plus) {
         last_accumulator_write = n;
      } else if (inst->dst.file != BAD_FILE) {
	 add_barrier_deps(n);
      }

      if (inst->mlen > 0 && inst->base_mrf != -1) {
	 for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
	    last_mrf_write[inst->base_mrf + i] = n;
	 }
      }

      if (inst->writes_flag()) {
	 last_conditional_mod[inst->flag_subreg] = n;
      }

      if (inst->writes_accumulator) {
         if (gen6plus) {
            last_accumulator_write = n;
         } else {
            add_barrier_deps(n);
         }
      }
   }
}

void
vec4_instruction_scheduler::calculate_deps()
{
   const bool gen6plus = v->brw->gen >= 6;

   schedule_node *last_grf_write[grf_count];
   schedule_node *last_mrf_write[BRW_MAX_MRF];
   schedule_node *last_conditional_mod = NULL;
   schedule_node *last_accumulator_write = NULL;
   /* Fixed HW registers are assumed to be separate from the virtual
    * GRFs, so they can be tracked separately.  We don't really write
    * to fixed GRFs much, so don't bother tracking them on a more
    * granular level.
    */
   schedule_node *last_fixed_grf_write = NULL;

   /* The last instruction always needs to still be the last instruction.
    * Either it's flow control (IF, ELSE, ENDIF, DO, WHILE) and scheduling
    * other things after it would disturb the basic block, or it's the EOT
    * URB_WRITE and we should do a better job at dead code eliminating
    * anything that could have been scheduled after it.
    */
   schedule_node *last = (schedule_node *)instructions.get_tail();
   add_barrier_deps(last);

   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));

   /* top-to-bottom dependencies: RAW and WAW. */
   foreach_list(node, &instructions) {
      schedule_node *n = (schedule_node *)node;
      vec4_instruction *inst = (vec4_instruction *)n->inst;

      if (inst->has_side_effects())
         add_barrier_deps(n);

      /* read-after-write deps. */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == GRF) {
            add_dep(last_grf_write[inst->src[i].reg], n);
         } else if (inst->src[i].file == HW_REG &&
                    (inst->src[i].fixed_hw_reg.file ==
                     BRW_GENERAL_REGISTER_FILE)) {
            add_dep(last_fixed_grf_write, n);
         } else if (inst->src[i].is_accumulator() && gen6plus) {
            assert(last_accumulator_write);
            add_dep(last_accumulator_write, n);
         } else if (inst->src[i].file != BAD_FILE &&
                    inst->src[i].file != IMM &&
                    inst->src[i].file != UNIFORM) {
            /* No reads from MRF, and ATTR is already translated away */
            assert(inst->src[i].file != MRF &&
                   inst->src[i].file != ATTR);
            add_barrier_deps(n);
         }
      }

      for (int i = 0; i < inst->mlen; i++) {
         /* It looks like the MRF regs are released in the send
          * instruction once it's sent, not when the result comes
          * back.
          */
         add_dep(last_mrf_write[inst->base_mrf + i], n);
      }

      if (inst->reads_flag()) {
         assert(last_conditional_mod);
         add_dep(last_conditional_mod, n);
      }

      if (inst->reads_accumulator_implicitly()) {
         if (gen6plus) {
            assert(last_accumulator_write);
            add_dep(last_accumulator_write, n);
         } else {
            add_barrier_deps(n);
         }
      }

      /* write-after-write deps. */
      if (inst->dst.file == GRF) {
         add_dep(last_grf_write[inst->dst.reg], n);
         last_grf_write[inst->dst.reg] = n;
      } else if (inst->dst.file == MRF) {
         add_dep(last_mrf_write[inst->dst.reg], n);
         last_mrf_write[inst->dst.reg] = n;
     } else if (inst->dst.file == HW_REG &&
                 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         last_fixed_grf_write = n;
      } else if (inst->dst.is_accumulator() && gen6plus) {
         add_dep(last_accumulator_write, n);
         last_accumulator_write = n;
      } else if (inst->dst.file != BAD_FILE) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0) {
         for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
            add_dep(last_mrf_write[inst->base_mrf + i], n);
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (inst->writes_flag()) {
         add_dep(last_conditional_mod, n, 0);
         last_conditional_mod = n;
      }

      if (inst->writes_accumulator) {
         if (gen6plus) {
            add_dep(last_accumulator_write, n);
            last_accumulator_write = n;
         } else {
            add_barrier_deps(n);
         }
      }
   }

   /* bottom-to-top dependencies: WAR */
   memset(last_grf_write, 0, sizeof(last_grf_write));
   memset(last_mrf_write, 0, sizeof(last_mrf_write));
   last_conditional_mod = NULL;
   last_accumulator_write = NULL;
   last_fixed_grf_write = NULL;

   exec_node *node;
   exec_node *prev;
   for (node = instructions.get_tail(), prev = node->prev;
        !node->is_head_sentinel();
        node = prev, prev = node->prev) {
      schedule_node *n = (schedule_node *)node;
      vec4_instruction *inst = (vec4_instruction *)n->inst;

      /* write-after-read deps. */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == GRF) {
            add_dep(n, last_grf_write[inst->src[i].reg]);
         } else if (inst->src[i].file == HW_REG &&
                    (inst->src[i].fixed_hw_reg.file ==
                     BRW_GENERAL_REGISTER_FILE)) {
            add_dep(n, last_fixed_grf_write);
         } else if (inst->src[i].is_accumulator() && gen6plus) {
            add_dep(n, last_accumulator_write);
         } else if (inst->src[i].file != BAD_FILE &&
                    inst->src[i].file != IMM &&
                    inst->src[i].file != UNIFORM) {
            assert(inst->src[i].file != MRF &&
                   inst->src[i].file != ATTR);
            add_barrier_deps(n);
         }
      }

      for (int i = 0; i < inst->mlen; i++) {
         /* It looks like the MRF regs are released in the send
          * instruction once it's sent, not when the result comes
          * back.
          */
         add_dep(n, last_mrf_write[inst->base_mrf + i], 2);
      }

      if (inst->reads_flag()) {
         add_dep(n, last_conditional_mod);
      }

      if (inst->reads_accumulator_implicitly()) {
         if (gen6plus) {
            add_dep(n, last_accumulator_write);
         } else {
            add_barrier_deps(n);
         }
      }

      /* Update the things this instruction wrote, so earlier reads
       * can mark this as WAR dependency.
       */
      if (inst->dst.file == GRF) {
         last_grf_write[inst->dst.reg] = n;
      } else if (inst->dst.file == MRF) {
         last_mrf_write[inst->dst.reg] = n;
      } else if (inst->dst.file == HW_REG &&
                 inst->dst.fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
         last_fixed_grf_write = n;
      } else if (inst->dst.is_accumulator() && gen6plus) {
         last_accumulator_write = n;
      } else if (inst->dst.file != BAD_FILE) {
         add_barrier_deps(n);
      }

      if (inst->mlen > 0) {
         for (int i = 0; i < v->implied_mrf_writes(inst); i++) {
            last_mrf_write[inst->base_mrf + i] = n;
         }
      }

      if (inst->writes_flag()) {
         last_conditional_mod = n;
      }

      if (inst->writes_accumulator) {
         if (gen6plus) {
            last_accumulator_write = n;
         } else {
            add_barrier_deps(n);
         }
      }
   }
}

schedule_node *
fs_instruction_scheduler::choose_instruction_to_schedule()
{
   schedule_node *chosen = NULL;

   if (mode == SCHEDULE_PRE || mode == SCHEDULE_POST) {
      int chosen_time = 0;

      /* Of the instructions ready to execute or the closest to
       * being ready, choose the oldest one.
       */
      foreach_list(node, &instructions) {
         schedule_node *n = (schedule_node *)node;

         if (!chosen || n->unblocked_time < chosen_time) {
            chosen = n;
            chosen_time = n->unblocked_time;
         }
      }
   } else {
      /* Before register allocation, we don't care about the latencies of
       * instructions.  All we care about is reducing live intervals of
       * variables so that we can avoid register spilling, or get SIMD16
       * shaders which naturally do a better job of hiding instruction
       * latency.
       */
      foreach_list(node, &instructions) {
         schedule_node *n = (schedule_node *)node;
         fs_inst *inst = (fs_inst *)n->inst;

         if (!chosen) {
            chosen = n;
            continue;
         }

         /* Most important: If we can definitely reduce register pressure, do
          * so immediately.
          */
         int register_pressure_benefit = get_register_pressure_benefit(n->inst);
         int chosen_register_pressure_benefit =
            get_register_pressure_benefit(chosen->inst);

         if (register_pressure_benefit > 0 &&
             register_pressure_benefit > chosen_register_pressure_benefit) {
            chosen = n;
            continue;
         } else if (chosen_register_pressure_benefit > 0 &&
                    (register_pressure_benefit <
                     chosen_register_pressure_benefit)) {
            continue;
         }

         if (mode == SCHEDULE_PRE_LIFO) {
            /* Prefer instructions that recently became available for
             * scheduling.  These are the things that are most likely to
             * (eventually) make a variable dead and reduce register pressure.
             * Typical register pressure estimates don't work for us because
             * most of our pressure comes from texturing, where no single
             * instruction to schedule will make a vec4 value dead.
             */
            if (n->cand_generation > chosen->cand_generation) {
               chosen = n;
               continue;
            } else if (n->cand_generation < chosen->cand_generation) {
               continue;
            }

            /* On MRF-using chips, prefer non-SEND instructions.  If we don't
             * do this, then because we prefer instructions that just became
             * candidates, we'll end up in a pattern of scheduling a SEND,
             * then the MRFs for the next SEND, then the next SEND, then the
             * MRFs, etc., without ever consuming the results of a send.
             */
            if (v->brw->gen < 7) {
               fs_inst *chosen_inst = (fs_inst *)chosen->inst;

               /* We use regs_written > 1 as our test for the kind of send
                * instruction to avoid -- only sends generate many regs, and a
                * single-result send is probably actually reducing register
                * pressure.
                */
               if (inst->regs_written <= 1 && chosen_inst->regs_written > 1) {
                  chosen = n;
                  continue;
               } else if (inst->regs_written > chosen_inst->regs_written) {
                  continue;
               }
            }
         }

         /* For instructions pushed on the cands list at the same time, prefer
          * the one with the highest delay to the end of the program.  This is
          * most likely to have its values able to be consumed first (such as
          * for a large tree of lowered ubo loads, which appear reversed in
          * the instruction stream with respect to when they can be consumed).
          */
         if (n->delay > chosen->delay) {
            chosen = n;
            continue;
         } else if (n->delay < chosen->delay) {
            continue;
         }

         /* If all other metrics are equal, we prefer the first instruction in
          * the list (program execution).
          */
      }
   }

   return chosen;
}

schedule_node *
vec4_instruction_scheduler::choose_instruction_to_schedule()
{
   schedule_node *chosen = NULL;
   int chosen_time = 0;

   /* Of the instructions ready to execute or the closest to being ready,
    * choose the oldest one.
    */
   foreach_list(node, &instructions) {
      schedule_node *n = (schedule_node *)node;

      if (!chosen || n->unblocked_time < chosen_time) {
         chosen = n;
         chosen_time = n->unblocked_time;
      }
   }

   return chosen;
}

int
fs_instruction_scheduler::issue_time(backend_instruction *inst)
{
   if (is_compressed((fs_inst *)inst))
      return 4;
   else
      return 2;
}

int
vec4_instruction_scheduler::issue_time(backend_instruction *inst)
{
   /* We always execute as two vec4s in parallel. */
   return 2;
}

void
instruction_scheduler::schedule_instructions(backend_instruction *next_block_header)
{
   time = 0;

   /* Remove non-DAG heads from the list. */
   foreach_list_safe(node, &instructions) {
      schedule_node *n = (schedule_node *)node;
      if (n->parent_count != 0)
	 n->remove();
   }

   unsigned cand_generation = 1;
   while (!instructions.is_empty()) {
      schedule_node *chosen = choose_instruction_to_schedule();

      /* Schedule this instruction. */
      assert(chosen);
      chosen->remove();
      next_block_header->insert_before(chosen->inst);
      instructions_to_schedule--;
      update_register_pressure(chosen->inst);

      /* Update the clock for how soon an instruction could start after the
       * chosen one.
       */
      time += issue_time(chosen->inst);

      /* If we expected a delay for scheduling, then bump the clock to reflect
       * that as well.  In reality, the hardware will switch to another
       * hyperthread and may not return to dispatching our thread for a while
       * even after we're unblocked.
       */
      time = MAX2(time, chosen->unblocked_time);

      if (debug) {
         fprintf(stderr, "clock %4d, scheduled: ", time);
         bv->dump_instruction(chosen->inst);
      }

      /* Now that we've scheduled a new instruction, some of its
       * children can be promoted to the list of instructions ready to
       * be scheduled.  Update the children's unblocked time for this
       * DAG edge as we do so.
       */
      for (int i = chosen->child_count - 1; i >= 0; i--) {
	 schedule_node *child = chosen->children[i];

	 child->unblocked_time = MAX2(child->unblocked_time,
				      time + chosen->child_latency[i]);

         if (debug) {
            fprintf(stderr, "\tchild %d, %d parents: ", i, child->parent_count);
            bv->dump_instruction(child->inst);
         }

         child->cand_generation = cand_generation;
	 child->parent_count--;
	 if (child->parent_count == 0) {
            if (debug) {
               fprintf(stderr, "\t\tnow available\n");
            }
	    instructions.push_head(child);
	 }
      }
      cand_generation++;

      /* Shared resource: the mathbox.  There's one mathbox per EU on Gen6+
       * but it's more limited pre-gen6, so if we send something off to it then
       * the next math instruction isn't going to make progress until the first
       * is done.
       */
      if (chosen->inst->is_math()) {
	 foreach_list(node, &instructions) {
	    schedule_node *n = (schedule_node *)node;

	    if (n->inst->is_math())
	       n->unblocked_time = MAX2(n->unblocked_time,
					time + chosen->latency);
	 }
      }
   }

   assert(instructions_to_schedule == 0);
}

void
instruction_scheduler::run(exec_list *all_instructions)
{
   backend_instruction *next_block_header =
      (backend_instruction *)all_instructions->head;

   if (debug) {
      fprintf(stderr, "\nInstructions before scheduling (reg_alloc %d)\n",
              post_reg_alloc);
      bv->dump_instructions();
   }

   /* Populate the remaining GRF uses array to improve the pre-regalloc
    * scheduling.
    */
   if (remaining_grf_uses) {
      foreach_list(node, all_instructions) {
         count_remaining_grf_uses((backend_instruction *)node);
      }
   }

   while (!next_block_header->is_tail_sentinel()) {
      /* Add things to be scheduled until we get to a new BB. */
      while (!next_block_header->is_tail_sentinel()) {
	 backend_instruction *inst = next_block_header;
	 next_block_header = (backend_instruction *)next_block_header->next;

	 add_inst(inst);
         if (inst->is_control_flow())
	    break;
      }
      calculate_deps();

      foreach_list(node, &instructions) {
         schedule_node *n = (schedule_node *)node;
         compute_delay(n);
      }

      schedule_instructions(next_block_header);
   }

   if (debug) {
      fprintf(stderr, "\nInstructions after scheduling (reg_alloc %d)\n",
              post_reg_alloc);
      bv->dump_instructions();
   }
}

void
fs_visitor::schedule_instructions(instruction_scheduler_mode mode)
{
   int grf_count;
   if (mode == SCHEDULE_POST)
      grf_count = grf_used;
   else
      grf_count = virtual_grf_count;

   fs_instruction_scheduler sched(this, grf_count, mode);
   sched.run(&instructions);

   if (unlikely(INTEL_DEBUG & DEBUG_WM) && mode == SCHEDULE_POST) {
      fprintf(stderr, "fs%d estimated execution time: %d cycles\n",
             dispatch_width, sched.time);
   }

   invalidate_live_intervals();
}

void
vec4_visitor::opt_schedule_instructions()
{
   vec4_instruction_scheduler sched(this, prog_data->total_grf);
   sched.run(&instructions);

   if (unlikely(debug_flag)) {
      fprintf(stderr, "vec4 estimated execution time: %d cycles\n", sched.time);
   }

   invalidate_live_intervals();
}
