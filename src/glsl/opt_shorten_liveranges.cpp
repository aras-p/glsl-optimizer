#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"
#include "program/hash_table.h"
#include "replaceInstruction.h"

#include <vector>

enum varstate {
  VAR_UNHANDLED,
  VAR_HANDLED,
  VAR_PTR
};

class ir_shorten_liveranges_visitor : public ir_hierarchical_visitor {
public:
    hash_table *varhash;
    std::vector<ir_variable*> referencedVars;

   ir_shorten_liveranges_visitor()
   {
      varhash = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
   }

   ir_visitor_status visit_enter(ir_assignment *);
   ir_visitor_status visit_leave(ir_assignment *);
   ir_visitor_status visit(ir_dereference_variable *);

   ir_visitor_status visit_enter(ir_loop *);
   ir_visitor_status visit_leave(ir_loop *);
   ir_visitor_status visit_enter(ir_function_signature *);
   ir_visitor_status visit_leave(ir_function_signature *);
   ir_visitor_status visit_enter(ir_function *);
   ir_visitor_status visit_leave(ir_function *);
   ir_visitor_status visit_enter(ir_expression *);
   ir_visitor_status visit_leave(ir_expression *);
   ir_visitor_status visit_enter(ir_texture *);
   ir_visitor_status visit_leave(ir_texture *);
   ir_visitor_status visit_enter(ir_swizzle *);
   ir_visitor_status visit_leave(ir_swizzle *);
   ir_visitor_status visit_enter(ir_dereference_array *);
   ir_visitor_status visit_leave(ir_dereference_array *);
   ir_visitor_status visit_enter(ir_dereference_record *);
   ir_visitor_status visit_leave(ir_dereference_record *);
   ir_visitor_status visit_enter(ir_call *);
   ir_visitor_status visit_leave(ir_call *);
   ir_visitor_status visit_enter(ir_return *);
   ir_visitor_status visit_leave(ir_return *);
   ir_visitor_status visit_enter(ir_discard *);
   ir_visitor_status visit_leave(ir_discard *);
   ir_visitor_status visit_enter(ir_if *);
   ir_visitor_status visit_leave(ir_if *);

   ir_visitor_status visit_enter_generic(ir_instruction *);
   ir_visitor_status visit_leave_generic(ir_instruction *);
};

ir_visitor_status
ir_shorten_liveranges_visitor::visit_enter(ir_assignment *ir)
{
  visit_enter_generic(ir);

  ir_variable *v = ir->whole_variable_written();
  //printf("ir_shorten_liveranges_visitor: ir_assignment_e %s\n", v ? v->name : "NULL");

  if(hash_table_find(varhash, v) == (void*)VAR_UNHANDLED) {
    hash_table_insert(varhash, ir, v);
  }

   return visit_continue;
}

ir_visitor_status
ir_shorten_liveranges_visitor::visit(ir_dereference_variable *ir)
{
  ir_variable *v = ir->variable_referenced();
  //printf("ir_shorten_liveranges_visitor: ir_dereference_variable %s\n", v ? v->name : "NULL");
  
  if(hash_table_find(varhash, v) == (void*)VAR_UNHANDLED) {
    hash_table_insert(varhash, (void*)VAR_HANDLED, v); // used before assigned?!
  } else if(hash_table_find(varhash, v) > (void*)VAR_PTR) {
    referencedVars.push_back(ir->variable_referenced());
  }
  return visit_continue;
}

int level = 0;
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter_generic(ir_instruction *ir)
{
  //printf("ir_shorten_liveranges_visitor: ir_instruction_e %d\n", ir->ir_type);
  level++;

   return visit_continue;
}

ir_visitor_status ir_shorten_liveranges_visitor::visit_leave_generic(ir_instruction *ir)
{
  //printf("ir_shorten_liveranges_visitor: ir_instruction_l %d\n", ir->ir_type);
  level--;

  if(level > 2)
    return visit_continue;

  for(int i=0 ;i<referencedVars.size(); i++) {
    ir_variable *v = referencedVars[i];
    if(hash_table_find(varhash, v) > (void*)VAR_PTR) {
      ir_assignment *stmt = (ir_assignment*)hash_table_find(varhash, v);
      if(ir == stmt)
        continue;
      stmt->remove();
      ir->insert_before(stmt);
      hash_table_insert(varhash, (void*)VAR_HANDLED, v);
      //printf("moved %s here\n", v->name);
    }
  }
  referencedVars.clear();
  return visit_continue;
}

ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_assignment *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_loop *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_loop *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_function_signature *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_function_signature *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_function *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_function *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_expression *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_expression *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_texture *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_texture *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_swizzle *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_swizzle *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_dereference_array *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_dereference_array *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_dereference_record *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_dereference_record *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_call *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_call *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_return *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_return *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_discard *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_discard *ir) { return visit_leave_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_enter(ir_if *ir) { return visit_enter_generic(ir); } 
ir_visitor_status ir_shorten_liveranges_visitor::visit_leave(ir_if *ir) { return visit_leave_generic(ir); } 

bool
do_shorten_liveranges(exec_list *instructions)
{
   ir_shorten_liveranges_visitor v;

   v.run(instructions);
   return true;
}
