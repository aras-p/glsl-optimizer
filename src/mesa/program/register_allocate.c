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

/** @file register_allocate.c
 *
 * Graph-coloring register allocator.
 *
 * The basic idea of graph coloring is to make a node in a graph for
 * every thing that needs a register (color) number assigned, and make
 * edges in the graph between nodes that interfere (can't be allocated
 * to the same register at the same time).
 *
 * During the "simplify" process, any any node with fewer edges than
 * there are registers means that that edge can get assigned a
 * register regardless of what its neighbors choose, so that node is
 * pushed on a stack and removed (with its edges) from the graph.
 * That likely causes other nodes to become trivially colorable as well.
 *
 * Then during the "select" process, nodes are popped off of that
 * stack, their edges restored, and assigned a color different from
 * their neighbors.  Because they were pushed on the stack only when
 * they were trivially colorable, any color chosen won't interfere
 * with the registers to be popped later.
 *
 * The downside to most graph coloring is that real hardware often has
 * limitations, like registers that need to be allocated to a node in
 * pairs, or aligned on some boundary.  This implementation follows
 * the paper "Retargetable Graph-Coloring Register Allocation for
 * Irregular Architectures" by Johan Runeson and Sven-Olof Nyström.
 *
 * In this system, there are register classes each containing various
 * registers, and registers may interfere with other registers.  For
 * example, one might have a class of base registers, and a class of
 * aligned register pairs that would each interfere with their pair of
 * the base registers.  Each node has a register class it needs to be
 * assigned to.  Define p(B) to be the size of register class B, and
 * q(B,C) to be the number of registers in B that the worst choice
 * register in C could conflict with.  Then, this system replaces the
 * basic graph coloring test of "fewer edges from this node than there
 * are registers" with "For this node of class B, the sum of q(B,C)
 * for each neighbor node of class C is less than pB".
 *
 * A nice feature of the pq test is that q(B,C) can be computed once
 * up front and stored in a 2-dimensional array, so that the cost of
 * coloring a node is constant with the number of registers.  We do
 * this during ra_set_finalize().
 */

#include <stdbool.h>

#include "util/ralloc.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/bitset.h"
#include "register_allocate.h"

#define NO_REG ~0

struct ra_reg {
   BITSET_WORD *conflicts;
   unsigned int *conflict_list;
   unsigned int conflict_list_size;
   unsigned int num_conflicts;
};

struct ra_regs {
   struct ra_reg *regs;
   unsigned int count;

   struct ra_class **classes;
   unsigned int class_count;

   bool round_robin;
};

struct ra_class {
   /**
    * Bitset indicating which registers belong to this class.
    *
    * (If bit N is set, then register N belongs to this class.)
    */
   BITSET_WORD *regs;

   /**
    * p(B) in Runeson/Nyström paper.
    *
    * This is "how many regs are in the set."
    */
   unsigned int p;

   /**
    * q(B,C) (indexed by C, B is this register class) in
    * Runeson/Nyström paper.  This is "how many registers of B could
    * the worst choice register from C conflict with".
    */
   unsigned int *q;
};

struct ra_node {
   /** @{
    *
    * List of which nodes this node interferes with.  This should be
    * symmetric with the other node.
    */
   BITSET_WORD *adjacency;
   unsigned int *adjacency_list;
   unsigned int adjacency_list_size;
   unsigned int adjacency_count;
   /** @} */

   unsigned int class;

   /* Register, if assigned, or NO_REG. */
   unsigned int reg;

   /**
    * Set when the node is in the trivially colorable stack.  When
    * set, the adjacency to this node is ignored, to implement the
    * "remove the edge from the graph" in simplification without
    * having to actually modify the adjacency_list.
    */
   bool in_stack;

   /**
    * The q total, as defined in the Runeson/Nyström paper, for all the
    * interfering nodes not in the stack.
    */
   unsigned int q_total;

   /* For an implementation that needs register spilling, this is the
    * approximate cost of spilling this node.
    */
   float spill_cost;
};

struct ra_graph {
   struct ra_regs *regs;
   /**
    * the variables that need register allocation.
    */
   struct ra_node *nodes;
   unsigned int count; /**< count of nodes. */

   unsigned int *stack;
   unsigned int stack_count;
};

/**
 * Creates a set of registers for the allocator.
 *
 * mem_ctx is a ralloc context for the allocator.  The reg set may be freed
 * using ralloc_free().
 */
struct ra_regs *
ra_alloc_reg_set(void *mem_ctx, unsigned int count)
{
   unsigned int i;
   struct ra_regs *regs;

   regs = rzalloc(mem_ctx, struct ra_regs);
   regs->count = count;
   regs->regs = rzalloc_array(regs, struct ra_reg, count);

   for (i = 0; i < count; i++) {
      regs->regs[i].conflicts = rzalloc_array(regs->regs, BITSET_WORD,
                                              BITSET_WORDS(count));
      BITSET_SET(regs->regs[i].conflicts, i);

      regs->regs[i].conflict_list = ralloc_array(regs->regs, unsigned int, 4);
      regs->regs[i].conflict_list_size = 4;
      regs->regs[i].conflict_list[0] = i;
      regs->regs[i].num_conflicts = 1;
   }

   return regs;
}

/**
 * The register allocator by default prefers to allocate low register numbers,
 * since it was written for hardware (gen4/5 Intel) that is limited in its
 * multithreadedness by the number of registers used in a given shader.
 *
 * However, for hardware without that restriction, densely packed register
 * allocation can put serious constraints on instruction scheduling.  This
 * function tells the allocator to rotate around the registers if possible as
 * it allocates the nodes.
 */
void
ra_set_allocate_round_robin(struct ra_regs *regs)
{
   regs->round_robin = true;
}

static void
ra_add_conflict_list(struct ra_regs *regs, unsigned int r1, unsigned int r2)
{
   struct ra_reg *reg1 = &regs->regs[r1];

   if (reg1->conflict_list_size == reg1->num_conflicts) {
      reg1->conflict_list_size *= 2;
      reg1->conflict_list = reralloc(regs->regs, reg1->conflict_list,
				     unsigned int, reg1->conflict_list_size);
   }
   reg1->conflict_list[reg1->num_conflicts++] = r2;
   BITSET_SET(reg1->conflicts, r2);
}

void
ra_add_reg_conflict(struct ra_regs *regs, unsigned int r1, unsigned int r2)
{
   if (!BITSET_TEST(regs->regs[r1].conflicts, r2)) {
      ra_add_conflict_list(regs, r1, r2);
      ra_add_conflict_list(regs, r2, r1);
   }
}

/**
 * Adds a conflict between base_reg and reg, and also between reg and
 * anything that base_reg conflicts with.
 *
 * This can simplify code for setting up multiple register classes
 * which are aggregates of some base hardware registers, compared to
 * explicitly using ra_add_reg_conflict.
 */
void
ra_add_transitive_reg_conflict(struct ra_regs *regs,
			       unsigned int base_reg, unsigned int reg)
{
   int i;

   ra_add_reg_conflict(regs, reg, base_reg);

   for (i = 0; i < regs->regs[base_reg].num_conflicts; i++) {
      ra_add_reg_conflict(regs, reg, regs->regs[base_reg].conflict_list[i]);
   }
}

unsigned int
ra_alloc_reg_class(struct ra_regs *regs)
{
   struct ra_class *class;

   regs->classes = reralloc(regs->regs, regs->classes, struct ra_class *,
			    regs->class_count + 1);

   class = rzalloc(regs, struct ra_class);
   regs->classes[regs->class_count] = class;

   class->regs = rzalloc_array(class, BITSET_WORD, BITSET_WORDS(regs->count));

   return regs->class_count++;
}

void
ra_class_add_reg(struct ra_regs *regs, unsigned int c, unsigned int r)
{
   struct ra_class *class = regs->classes[c];

   BITSET_SET(class->regs, r);
   class->p++;
}

/**
 * Returns true if the register belongs to the given class.
 */
static bool
reg_belongs_to_class(unsigned int r, struct ra_class *c)
{
   return BITSET_TEST(c->regs, r);
}

/**
 * Must be called after all conflicts and register classes have been
 * set up and before the register set is used for allocation.
 * To avoid costly q value computation, use the q_values paramater
 * to pass precomputed q values to this function.
 */
void
ra_set_finalize(struct ra_regs *regs, unsigned int **q_values)
{
   unsigned int b, c;

   for (b = 0; b < regs->class_count; b++) {
      regs->classes[b]->q = ralloc_array(regs, unsigned int, regs->class_count);
   }

   if (q_values) {
      for (b = 0; b < regs->class_count; b++) {
         for (c = 0; c < regs->class_count; c++) {
            regs->classes[b]->q[c] = q_values[b][c];
	 }
      }
      return;
   }

   /* Compute, for each class B and C, how many regs of B an
    * allocation to C could conflict with.
    */
   for (b = 0; b < regs->class_count; b++) {
      for (c = 0; c < regs->class_count; c++) {
	 unsigned int rc;
	 int max_conflicts = 0;

	 for (rc = 0; rc < regs->count; rc++) {
	    int conflicts = 0;
	    int i;

            if (!reg_belongs_to_class(rc, regs->classes[c]))
	       continue;

	    for (i = 0; i < regs->regs[rc].num_conflicts; i++) {
	       unsigned int rb = regs->regs[rc].conflict_list[i];
	       if (BITSET_TEST(regs->classes[b]->regs, rb))
		  conflicts++;
	    }
	    max_conflicts = MAX2(max_conflicts, conflicts);
	 }
	 regs->classes[b]->q[c] = max_conflicts;
      }
   }
}

static void
ra_add_node_adjacency(struct ra_graph *g, unsigned int n1, unsigned int n2)
{
   BITSET_SET(g->nodes[n1].adjacency, n2);

   if (n1 != n2) {
      int n1_class = g->nodes[n1].class;
      int n2_class = g->nodes[n2].class;
      g->nodes[n1].q_total += g->regs->classes[n1_class]->q[n2_class];
   }

   if (g->nodes[n1].adjacency_count >=
       g->nodes[n1].adjacency_list_size) {
      g->nodes[n1].adjacency_list_size *= 2;
      g->nodes[n1].adjacency_list = reralloc(g, g->nodes[n1].adjacency_list,
                                             unsigned int,
                                             g->nodes[n1].adjacency_list_size);
   }

   g->nodes[n1].adjacency_list[g->nodes[n1].adjacency_count] = n2;
   g->nodes[n1].adjacency_count++;
}

struct ra_graph *
ra_alloc_interference_graph(struct ra_regs *regs, unsigned int count)
{
   struct ra_graph *g;
   unsigned int i;

   g = rzalloc(regs, struct ra_graph);
   g->regs = regs;
   g->nodes = rzalloc_array(g, struct ra_node, count);
   g->count = count;

   g->stack = rzalloc_array(g, unsigned int, count);

   for (i = 0; i < count; i++) {
      int bitset_count = BITSET_WORDS(count);
      g->nodes[i].adjacency = rzalloc_array(g, BITSET_WORD, bitset_count);

      g->nodes[i].adjacency_list_size = 4;
      g->nodes[i].adjacency_list =
         ralloc_array(g, unsigned int, g->nodes[i].adjacency_list_size);
      g->nodes[i].adjacency_count = 0;
      g->nodes[i].q_total = 0;

      ra_add_node_adjacency(g, i, i);
      g->nodes[i].reg = NO_REG;
   }

   return g;
}

void
ra_set_node_class(struct ra_graph *g,
		  unsigned int n, unsigned int class)
{
   g->nodes[n].class = class;
}

void
ra_add_node_interference(struct ra_graph *g,
			 unsigned int n1, unsigned int n2)
{
   if (!BITSET_TEST(g->nodes[n1].adjacency, n2)) {
      ra_add_node_adjacency(g, n1, n2);
      ra_add_node_adjacency(g, n2, n1);
   }
}

static bool
pq_test(struct ra_graph *g, unsigned int n)
{
   int n_class = g->nodes[n].class;

   return g->nodes[n].q_total < g->regs->classes[n_class]->p;
}

static void
decrement_q(struct ra_graph *g, unsigned int n)
{
   unsigned int i;
   int n_class = g->nodes[n].class;

   for (i = 0; i < g->nodes[n].adjacency_count; i++) {
      unsigned int n2 = g->nodes[n].adjacency_list[i];
      unsigned int n2_class = g->nodes[n2].class;

      if (n != n2 && !g->nodes[n2].in_stack) {
	 g->nodes[n2].q_total -= g->regs->classes[n2_class]->q[n_class];
      }
   }
}

/**
 * Simplifies the interference graph by pushing all
 * trivially-colorable nodes into a stack of nodes to be colored,
 * removing them from the graph, and rinsing and repeating.
 *
 * If we encounter a case where we can't push any nodes on the stack, then
 * we optimistically choose a node and push it on the stack. We heuristically
 * push the node with the lowest total q value, since it has the fewest
 * neighbors and therefore is most likely to be allocated.
 */
static void
ra_simplify(struct ra_graph *g)
{
   bool progress = true;
   int i;

   while (progress) {
      unsigned int best_optimistic_node = ~0;
      unsigned int lowest_q_total = ~0;

      progress = false;

      for (i = g->count - 1; i >= 0; i--) {
	 if (g->nodes[i].in_stack || g->nodes[i].reg != NO_REG)
	    continue;

	 if (pq_test(g, i)) {
	    decrement_q(g, i);
	    g->stack[g->stack_count] = i;
	    g->stack_count++;
	    g->nodes[i].in_stack = true;
	    progress = true;
	 } else {
	    unsigned int new_q_total = g->nodes[i].q_total;
	    if (new_q_total < lowest_q_total) {
	       best_optimistic_node = i;
	       lowest_q_total = new_q_total;
	    }
	 }
      }

      if (!progress && best_optimistic_node != ~0) {
	 decrement_q(g, best_optimistic_node);
	 g->stack[g->stack_count] = best_optimistic_node;
	 g->stack_count++;
	 g->nodes[best_optimistic_node].in_stack = true;
	 progress = true;
      }
   }
}

/**
 * Pops nodes from the stack back into the graph, coloring them with
 * registers as they go.
 *
 * If all nodes were trivially colorable, then this must succeed.  If
 * not (optimistic coloring), then it may return false;
 */
static bool
ra_select(struct ra_graph *g)
{
   int i;
   int start_search_reg = 0;

   while (g->stack_count != 0) {
      unsigned int ri;
      unsigned int r = -1;
      int n = g->stack[g->stack_count - 1];
      struct ra_class *c = g->regs->classes[g->nodes[n].class];

      /* Find the lowest-numbered reg which is not used by a member
       * of the graph adjacent to us.
       */
      for (ri = 0; ri < g->regs->count; ri++) {
         r = (start_search_reg + ri) % g->regs->count;
         if (!reg_belongs_to_class(r, c))
	    continue;

	 /* Check if any of our neighbors conflict with this register choice. */
	 for (i = 0; i < g->nodes[n].adjacency_count; i++) {
	    unsigned int n2 = g->nodes[n].adjacency_list[i];

	    if (!g->nodes[n2].in_stack &&
		BITSET_TEST(g->regs->regs[r].conflicts, g->nodes[n2].reg)) {
	       break;
	    }
	 }
	 if (i == g->nodes[n].adjacency_count)
	    break;
      }

      /* set this to false even if we return here so that
       * ra_get_best_spill_node() considers this node later.
       */
      g->nodes[n].in_stack = false;

      if (ri == g->regs->count)
	 return false;

      g->nodes[n].reg = r;
      g->stack_count--;

      if (g->regs->round_robin)
         start_search_reg = r + 1;
   }

   return true;
}

bool
ra_allocate(struct ra_graph *g)
{
   ra_simplify(g);
   return ra_select(g);
}

unsigned int
ra_get_node_reg(struct ra_graph *g, unsigned int n)
{
   return g->nodes[n].reg;
}

/**
 * Forces a node to a specific register.  This can be used to avoid
 * creating a register class containing one node when handling data
 * that must live in a fixed location and is known to not conflict
 * with other forced register assignment (as is common with shader
 * input data).  These nodes do not end up in the stack during
 * ra_simplify(), and thus at ra_select() time it is as if they were
 * the first popped off the stack and assigned their fixed locations.
 * Nodes that use this function do not need to be assigned a register
 * class.
 *
 * Must be called before ra_simplify().
 */
void
ra_set_node_reg(struct ra_graph *g, unsigned int n, unsigned int reg)
{
   g->nodes[n].reg = reg;
   g->nodes[n].in_stack = false;
}

static float
ra_get_spill_benefit(struct ra_graph *g, unsigned int n)
{
   int j;
   float benefit = 0;
   int n_class = g->nodes[n].class;

   /* Define the benefit of eliminating an interference between n, n2
    * through spilling as q(C, B) / p(C).  This is similar to the
    * "count number of edges" approach of traditional graph coloring,
    * but takes classes into account.
    */
   for (j = 0; j < g->nodes[n].adjacency_count; j++) {
      unsigned int n2 = g->nodes[n].adjacency_list[j];
      if (n != n2) {
	 unsigned int n2_class = g->nodes[n2].class;
	 benefit += ((float)g->regs->classes[n_class]->q[n2_class] /
		     g->regs->classes[n_class]->p);
      }
   }

   return benefit;
}

/**
 * Returns a node number to be spilled according to the cost/benefit using
 * the pq test, or -1 if there are no spillable nodes.
 */
int
ra_get_best_spill_node(struct ra_graph *g)
{
   unsigned int best_node = -1;
   float best_benefit = 0.0;
   unsigned int n;

   /* Consider any nodes that we colored successfully or the node we failed to
    * color for spilling. When we failed to color a node in ra_select(), we
    * only considered these nodes, so spilling any other ones would not result
    * in us making progress.
    */
   for (n = 0; n < g->count; n++) {
      float cost = g->nodes[n].spill_cost;
      float benefit;

      if (cost <= 0.0)
	 continue;

      if (g->nodes[n].in_stack)
         continue;

      benefit = ra_get_spill_benefit(g, n);

      if (benefit / cost > best_benefit) {
	 best_benefit = benefit / cost;
	 best_node = n;
      }
   }

   return best_node;
}

/**
 * Only nodes with a spill cost set (cost != 0.0) will be considered
 * for register spilling.
 */
void
ra_set_node_spill_cost(struct ra_graph *g, unsigned int n, float cost)
{
   g->nodes[n].spill_cost = cost;
}
