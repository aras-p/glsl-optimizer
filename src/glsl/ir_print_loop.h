/* Recover source-level for initializers without changing Mesa IR or the
 * values read by initialization. Shared by printers. */
#ifndef IR_PRINT_LOOP_H
#define IR_PRINT_LOOP_H

#include "ir_hierarchical_visitor.h"
#include "loop_analysis.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

inline loop_variable* printed_for_induction(ir_loop* loop, loop_variable_state* state)
{
   if (state) {
      foreach_in_list(loop_variable, induction, &state->induction_variables)
         if (induction->first_assignment == loop->body_instructions.get_tail())
            return induction;
   }
   return nullptr;
}

inline bool printable_for_loop(ir_loop* loop, loop_variable_state* state)
{
   if (!state || state->terminators.length() != 1 ||
       !printed_for_induction(loop, state) || state->num_loop_jumps != 1)
      return false;
   auto* term = static_cast<loop_terminator*>(state->terminators.get_head());
   exec_node* first = loop->body_instructions.get_head();
   while (!first->is_tail_sentinel() && static_cast<ir_instruction*>(first)->as_variable())
      first = first->next;
   return !term->continue_from_then && term->ir == first;
}

class print_loop_initializers {
   class references : public ir_hierarchical_visitor {
   public:
      std::unordered_map<ir_variable*, unsigned> counts;
      std::vector<ir_loop*> loops;
      ir_visitor_status visit(ir_dereference_variable* ir) override {
         ++counts[ir->var];
         return visit_continue;
      }
      ir_visitor_status visit_enter(ir_loop* ir) override {
         loops.push_back(ir);
         return visit_continue;
      }
   };
   class initializer_dependencies : public ir_hierarchical_visitor {
   public:
      std::unordered_set<ir_variable*> variables;
      bool movable = true;
      ir_visitor_status visit(ir_dereference_variable* ir) override {
         variables.insert(ir->var);
         // Restrict movement to ordinary local, parameter and read-only inputs.
         switch (ir->var->data.mode) {
         case ir_var_auto: case ir_var_temporary:
         case ir_var_function_in: case ir_var_const_in:
         case ir_var_function_out: case ir_var_function_inout:
         case ir_var_uniform: case ir_var_shader_in: break;
         default: movable = false; break;
         }
         if (ir->var->data.memory_volatile) movable = false;
         return visit_continue;
      }
      ir_visitor_status visit_enter(ir_texture*) override {
         movable = false;
         return visit_continue_with_parent;
      }
      ir_visitor_status visit_enter(ir_expression* ir) override {
         // Derivatives/interpolation depend on execution context, not just
         // variable values. Buffer operations also need memory-effect analysis.
         switch (ir->operation) {
         case ir_unop_dFdx: case ir_unop_dFdx_coarse: case ir_unop_dFdx_fine:
         case ir_unop_dFdy: case ir_unop_dFdy_coarse: case ir_unop_dFdy_fine:
         case ir_unop_interpolate_at_centroid:
         case ir_binop_interpolate_at_offset: case ir_binop_interpolate_at_sample:
         case ir_unop_get_buffer_size: case ir_unop_ssbo_unsized_array_length:
         case ir_binop_ubo_load: movable = false; break;
         default: break;
         }
         return visit_continue;
      }
   };

   class dependency_writes : public ir_hierarchical_visitor {
   public:
      const std::unordered_set<ir_variable*>& dependencies;
      bool modified = false;
      dependency_writes(const std::unordered_set<ir_variable*>& dependencies)
         : dependencies(dependencies) {}
      ir_visitor_status visit_enter(ir_assignment* ir) override {
         // Includes partial writes, array elements, and assignments in nested
         // branches/loops. Any write to the containing variable is a barrier.
         if (dependencies.count(ir->lhs->variable_referenced())) modified = true;
         return visit_continue;
      }
      ir_visitor_status visit_enter(ir_call*) override {
         // Calls can write out/inout parameters or globals. Do not speculate.
         modified = true;
         return visit_continue_with_parent;
      }
   };

   static bool can_move_initializer(ir_assignment* init, ir_loop* loop)
   {
      initializer_dependencies dependencies;
      init->rhs->accept(&dependencies);
      if (!dependencies.movable) return false;
      dependency_writes writes(dependencies.variables);
      for (exec_node* n = init->next; n != loop; n = n->next) {
         static_cast<ir_instruction*>(n)->accept(&writes);
         if (writes.modified) return false;
      }
      return true;
   }

public:
   std::unordered_map<ir_loop*, ir_assignment*> initializers;
   std::unordered_set<ir_instruction*> omitted;

   print_loop_initializers(exec_list* instructions, loop_state* state)
   {
      references all;
      visit_list_elements(&all, instructions);
      for (ir_loop* loop : all.loops) {
         auto* ls = state->get(loop);
         if (!printable_for_loop(loop, ls)) continue;
         auto* induction = printed_for_induction(loop, ls);
         ir_variable* var = induction->var;
         if ((var->data.mode != ir_var_auto && var->data.mode != ir_var_temporary) ||
             !var->type->is_scalar()) continue;

         // Search within this block. Moving initialization is safe only when
         // intervening statements cannot change any value read by its RHS.
         ir_assignment* init = nullptr;
         for (exec_node* n = loop->prev; !n->is_head_sentinel(); n = n->prev) {
            auto* assignment = static_cast<ir_instruction*>(n)->as_assignment();
            if (assignment && assignment->whole_variable_written() == var) {
               if (!assignment->condition && can_move_initializer(assignment, loop))
                  init = assignment;
               break;
            }
         }
         if (!init) continue;

         // Require a declaration in this block, and no uses outside the loop
         // except the initializer's LHS (including no self-reference on RHS).
         bool declared = false;
         for (exec_node* n = loop->prev; !n->is_head_sentinel(); n = n->prev)
            if (n == var) { declared = true; break; }
         if (!declared) continue;
         references body;
         loop->accept(&body);
         if (all.counts[var] != body.counts[var] + 1) continue;

         initializers[loop] = init;
         omitted.insert(var);
         omitted.insert(init);
      }
   }
};

#endif
