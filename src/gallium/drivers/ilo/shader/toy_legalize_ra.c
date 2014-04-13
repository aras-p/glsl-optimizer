/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include <stdlib.h> /* for qsort() */
#include "toy_compiler.h"
#include "toy_legalize.h"

/**
 * Live interval of a VRF register.
 */
struct linear_scan_live_interval {
   int vrf;
   int startpoint;
   int endpoint;

   /*
    * should this be assigned a consecutive register of the previous
    * interval's?
    */
   bool consecutive;

   int reg;

   struct list_head list;
};

/**
 * Linear scan.
 */
struct linear_scan {
   struct linear_scan_live_interval *intervals;
   int max_vrf, num_vrfs;

   int num_regs;

   struct list_head active_list;
   int *free_regs;
   int num_free_regs;

   int *vrf_mapping;
};

/**
 * Return a chunk of registers to the free register pool.
 */
static void
linear_scan_free_regs(struct linear_scan *ls, int reg, int count)
{
   int i;

   for (i = 0; i < count; i++)
      ls->free_regs[ls->num_free_regs++] = reg + count - 1 - i;
}

static int
linear_scan_compare_regs(const void *elem1, const void *elem2)
{
   const int *reg1 = elem1;
   const int *reg2 = elem2;

   /* in reverse order */
   return (*reg2 - *reg1);
}

/**
 * Allocate a chunk of registers from the free register pool.
 */
static int
linear_scan_allocate_regs(struct linear_scan *ls, int count)
{
   bool sorted = false;
   int reg;

   /* simple cases */
   if (count > ls->num_free_regs)
      return -1;
   else if (count == 1)
      return ls->free_regs[--ls->num_free_regs];

   /* TODO a free register pool */
   /* TODO reserve some regs for spilling */
   while (true) {
      bool found = false;
      int start;

      /*
       * find a chunk of registers that have consecutive register
       * numbers
       */
      for (start = ls->num_free_regs - 1; start >= count - 1; start--) {
         int i;

         for (i = 1; i < count; i++) {
            if (ls->free_regs[start - i] != ls->free_regs[start] + i)
               break;
         }

         if (i >= count) {
            found = true;
            break;
         }
      }

      if (found) {
         reg = ls->free_regs[start];

         if (start != ls->num_free_regs - 1) {
            start++;
            memmove(&ls->free_regs[start - count],
                    &ls->free_regs[start],
                    sizeof(*ls->free_regs) * (ls->num_free_regs - start));
         }
         ls->num_free_regs -= count;
         break;
      }
      else if (!sorted) {
         /* sort and retry */
         qsort(ls->free_regs, ls->num_free_regs, sizeof(*ls->free_regs),
               linear_scan_compare_regs);
         sorted = true;
      }
      else {
         /* failed */
         reg = -1;
         break;
      }
   }

   return reg;
}

/**
 * Add an interval to the active list.
 */
static void
linear_scan_add_active(struct linear_scan *ls,
                       struct linear_scan_live_interval *interval)
{
   struct linear_scan_live_interval *pos;

   /* keep the active list sorted by endpoints */
   LIST_FOR_EACH_ENTRY(pos, &ls->active_list, list) {
      if (pos->endpoint >= interval->endpoint)
         break;
   }

   list_addtail(&interval->list, &pos->list);
}

/**
 * Remove an interval from the active list.
 */
static void
linear_scan_remove_active(struct linear_scan *ls,
                          struct linear_scan_live_interval *interval)
{
   list_del(&interval->list);
}

/**
 * Remove intervals that are no longer active from the active list.
 */
static void
linear_scan_expire_active(struct linear_scan *ls, int pc)
{
   struct linear_scan_live_interval *interval, *next;

   LIST_FOR_EACH_ENTRY_SAFE(interval, next, &ls->active_list, list) {
      /*
       * since we sort intervals on the active list by their endpoints, we
       * know that this and the rest of the intervals are still active.
       */
      if (interval->endpoint >= pc)
         break;

      linear_scan_remove_active(ls, interval);

      /* recycle the reg */
      linear_scan_free_regs(ls, interval->reg, 1);
   }
}

/**
 * Spill an interval.
 */
static void
linear_scan_spill(struct linear_scan *ls,
                  struct linear_scan_live_interval *interval,
                  bool is_active)
{
   assert(!"no spilling support");
}

/**
 * Spill a range of intervals.
 */
static void
linear_scan_spill_range(struct linear_scan *ls, int first, int count)
{
   int i;

   for (i = 0; i < count; i++) {
      struct linear_scan_live_interval *interval = &ls->intervals[first + i];

      linear_scan_spill(ls, interval, false);
   }
}

/**
 * Perform linear scan to allocate registers for the intervals.
 */
static bool
linear_scan_run(struct linear_scan *ls)
{
   int i;

   i = 0;
   while (i < ls->num_vrfs) {
      struct linear_scan_live_interval *first = &ls->intervals[i];
      int reg, count;

      /*
       * GEN6_OPCODE_SEND may write to multiple consecutive registers and we need to
       * support that
       */
      for (count = 1; i + count < ls->num_vrfs; count++) {
         const struct linear_scan_live_interval *interval =
            &ls->intervals[i + count];

         if (interval->startpoint != first->startpoint ||
             !interval->consecutive)
            break;
      }

      reg = linear_scan_allocate_regs(ls, count);

      /* expire intervals that are no longer active and try again */
      if (reg < 0) {
         linear_scan_expire_active(ls, first->startpoint);
         reg = linear_scan_allocate_regs(ls, count);
      }

      /* have to spill some intervals */
      if (reg < 0) {
         struct linear_scan_live_interval *last_active =
            container_of(ls->active_list.prev,
                  (struct linear_scan_live_interval *) NULL, list);

         /* heuristically spill the interval that ends last */
         if (count > 1 || last_active->endpoint < first->endpoint) {
            linear_scan_spill_range(ls, i, count);
            i += count;
            continue;
         }

         /* make some room for the new interval */
         linear_scan_spill(ls, last_active, true);
         reg = linear_scan_allocate_regs(ls, count);
         if (reg < 0) {
            assert(!"failed to spill any register");
            return false;
         }
      }

      while (count--) {
         struct linear_scan_live_interval *interval = &ls->intervals[i++];

         interval->reg = reg++;
         linear_scan_add_active(ls, interval);

         ls->vrf_mapping[interval->vrf] = interval->reg;

         /*
          * this should and must be the case because of how we initialized the
          * intervals
          */
         assert(interval->vrf - first->vrf == interval->reg - first->reg);
      }
   }

   return true;
}

/**
 * Add a new interval.
 */
static void
linear_scan_add_live_interval(struct linear_scan *ls, int vrf, int pc)
{
   if (ls->intervals[vrf].vrf)
      return;

   ls->intervals[vrf].vrf = vrf;
   ls->intervals[vrf].startpoint = pc;

   ls->num_vrfs++;
   if (vrf > ls->max_vrf)
      ls->max_vrf = vrf;
}

/**
 * Perform (oversimplified?) live variable analysis.
 */
static void
linear_scan_init_live_intervals(struct linear_scan *ls,
                                struct toy_compiler *tc)
{
   const struct toy_inst *inst;
   int pc, do_pc, while_pc;

   pc = 0;
   do_pc = -1;
   while_pc = -1;

   tc_head(tc);
   while ((inst = tc_next_no_skip(tc)) != NULL) {
      const int startpoint = (pc <= while_pc) ? do_pc : pc;
      const int endpoint = (pc <= while_pc) ? while_pc : pc;
      int vrf, i;

      /*
       * assume all registers used in this outermost loop are live through out
       * the whole loop
       */
      if (inst->marker) {
         if (pc > while_pc) {
            struct toy_inst *inst2;
            int loop_level = 1;

            assert(inst->opcode == TOY_OPCODE_DO);
            do_pc = pc;
            while_pc = pc + 1;

            /* find the matching GEN6_OPCODE_WHILE */
            LIST_FOR_EACH_ENTRY_FROM(inst2, tc->iter_next,
                  &tc->instructions, list) {
               if (inst2->marker) {
                  assert(inst->opcode == TOY_OPCODE_DO);
                  loop_level++;
                  continue;
               }

               if (inst2->opcode == GEN6_OPCODE_WHILE) {
                  loop_level--;
                  if (!loop_level)
                     break;
               }
               while_pc++;
            }
         }

         continue;
      }

      if (inst->dst.file == TOY_FILE_VRF) {
         int num_dst;

         /* TODO this is a hack */
         if (inst->opcode == GEN6_OPCODE_SEND ||
             inst->opcode == GEN6_OPCODE_SENDC) {
            const uint32_t mdesc = inst->src[1].val32;
            int response_length = (mdesc >> 20) & 0x1f;

            num_dst = response_length;
            if (num_dst > 1 && inst->exec_size == GEN6_EXECSIZE_16)
               num_dst /= 2;
         }
         else {
            num_dst = 1;
         }

         vrf = inst->dst.val32 / TOY_REG_WIDTH;

         for (i = 0; i < num_dst; i++) {
            /* first use */
            if (!ls->intervals[vrf].vrf)
               linear_scan_add_live_interval(ls, vrf, startpoint);

            ls->intervals[vrf].endpoint = endpoint;
            ls->intervals[vrf].consecutive = (i > 0);

            vrf++;
         }
      }

      for (i = 0; i < Elements(inst->src); i++) {
         if (inst->src[i].file != TOY_FILE_VRF)
            continue;

         vrf = inst->src[i].val32 / TOY_REG_WIDTH;

         /* first use */
         if (!ls->intervals[vrf].vrf)
            linear_scan_add_live_interval(ls, vrf, startpoint);

         ls->intervals[vrf].endpoint = endpoint;
      }

      pc++;
   }
}

/**
 * Clean up after performing linear scan.
 */
static void
linear_scan_cleanup(struct linear_scan *ls)
{
   FREE(ls->vrf_mapping);
   FREE(ls->intervals);
   FREE(ls->free_regs);
}

static int
linear_scan_compare_live_intervals(const void *elem1, const void *elem2)
{
   const struct linear_scan_live_interval *interval1 = elem1;
   const struct linear_scan_live_interval *interval2 = elem2;

   /* make unused elements appear at the end */
   if (!interval1->vrf)
      return 1;
   else if (!interval2->vrf)
      return -1;

   /* sort by startpoints first, and then by vrf */
   if (interval1->startpoint != interval2->startpoint)
      return (interval1->startpoint - interval2->startpoint);
   else
      return (interval1->vrf - interval2->vrf);

}

/**
 * Prepare for linear scan.
 */
static bool
linear_scan_init(struct linear_scan *ls, int num_regs,
                 struct toy_compiler *tc)
{
   int num_intervals, i;

   memset(ls, 0, sizeof(*ls));

   /* this may be much larger than ls->num_vrfs... */
   num_intervals = tc->next_vrf;
   ls->intervals = CALLOC(num_intervals, sizeof(ls->intervals[0]));
   if (!ls->intervals)
      return false;

   linear_scan_init_live_intervals(ls, tc);
   /* sort intervals by startpoints */
   qsort(ls->intervals, num_intervals, sizeof(*ls->intervals),
         linear_scan_compare_live_intervals);

   ls->num_regs = num_regs;
   ls->num_free_regs = num_regs;

   ls->free_regs = MALLOC(ls->num_regs * sizeof(*ls->free_regs));
   if (!ls->free_regs) {
      FREE(ls->intervals);
      return false;
   }

   /* add in reverse order as we will allocate from the tail */
   for (i = 0; i < ls->num_regs; i++)
      ls->free_regs[i] = num_regs - i - 1;

   list_inithead(&ls->active_list);

   ls->vrf_mapping = CALLOC(ls->max_vrf + 1, sizeof(*ls->vrf_mapping));
   if (!ls->vrf_mapping) {
      FREE(ls->intervals);
      FREE(ls->free_regs);
      return false;
   }

   return true;
}

/**
 * Allocate registers with linear scan.
 */
static void
linear_scan_allocation(struct toy_compiler *tc,
                       int start_grf, int end_grf,
                       int num_grf_per_vrf)
{
   const int num_grfs = end_grf - start_grf + 1;
   struct linear_scan ls;
   struct toy_inst *inst;

   if (!linear_scan_init(&ls, num_grfs / num_grf_per_vrf, tc))
      return;

   if (!linear_scan_run(&ls)) {
      tc_fail(tc, "failed to allocate registers");
      return;
   }


   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      int i;

      if (inst->dst.file == TOY_FILE_VRF) {
         const uint32_t val32 = inst->dst.val32;
         int reg = val32 / TOY_REG_WIDTH;
         int subreg = val32 % TOY_REG_WIDTH;

         /* map to GRF */
         reg = ls.vrf_mapping[reg] * num_grf_per_vrf + start_grf;

         inst->dst.file = TOY_FILE_GRF;
         inst->dst.val32 = reg * TOY_REG_WIDTH + subreg;
      }

      for (i = 0; i < Elements(inst->src); i++) {
         const uint32_t val32 = inst->src[i].val32;
         int reg, subreg;

         if (inst->src[i].file != TOY_FILE_VRF)
            continue;

         reg = val32 / TOY_REG_WIDTH;
         subreg = val32 % TOY_REG_WIDTH;

         /* map to GRF */
         reg = ls.vrf_mapping[reg] * num_grf_per_vrf + start_grf;

         inst->src[i].file = TOY_FILE_GRF;
         inst->src[i].val32 = reg * TOY_REG_WIDTH + subreg;
      }
   }

   linear_scan_cleanup(&ls);
}

/**
 * Trivially allocate registers.
 */
static void
trivial_allocation(struct toy_compiler *tc,
                   int start_grf, int end_grf,
                   int num_grf_per_vrf)
{
   struct toy_inst *inst;
   int max_grf = -1;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      int i;

      if (inst->dst.file == TOY_FILE_VRF) {
         const uint32_t val32 = inst->dst.val32;
         int reg = val32 / TOY_REG_WIDTH;
         int subreg = val32 % TOY_REG_WIDTH;

         reg = reg * num_grf_per_vrf + start_grf - 1;

         inst->dst.file = TOY_FILE_GRF;
         inst->dst.val32 = reg * TOY_REG_WIDTH + subreg;

         if (reg > max_grf)
            max_grf = reg;
      }

      for (i = 0; i < Elements(inst->src); i++) {
         const uint32_t val32 = inst->src[i].val32;
         int reg, subreg;

         if (inst->src[i].file != TOY_FILE_VRF)
            continue;

         reg = val32 / TOY_REG_WIDTH;
         subreg = val32 % TOY_REG_WIDTH;

         reg = reg * num_grf_per_vrf + start_grf - 1;

         inst->src[i].file = TOY_FILE_GRF;
         inst->src[i].val32 = reg * TOY_REG_WIDTH + subreg;

         if (reg > max_grf)
            max_grf = reg;
      }
   }

   if (max_grf + num_grf_per_vrf - 1 > end_grf)
      tc_fail(tc, "failed to allocate registers");
}

/**
 * Allocate GRF registers to VRF registers.
 */
void
toy_compiler_allocate_registers(struct toy_compiler *tc,
                                int start_grf, int end_grf,
                                int num_grf_per_vrf)
{
   if (true)
      linear_scan_allocation(tc, start_grf, end_grf, num_grf_per_vrf);
   else
      trivial_allocation(tc, start_grf, end_grf, num_grf_per_vrf);
}
