/* Source-output precision inference. Unlike Mesa's
 * lower_precision pass this keeps 32-bit IR types and annotates declarations;
 * the source printers decide where half/float conversions are required. */
#include "ir.h"
#include "ir_hierarchical_visitor.h"
#include <unordered_map>

static unsigned normalized(unsigned p)
{
   return p == GLSL_PRECISION_LOW ? GLSL_PRECISION_MEDIUM : p;
}

static unsigned higher(unsigned a, unsigned b)
{
   a = normalized(a);
   b = normalized(b);
   if (a == GLSL_PRECISION_NONE) return b;
   if (b == GLSL_PRECISION_NONE) return a;
   return a < b ? a : b; // Mesa orders HIGH before MEDIUM.
}

unsigned ir_rvalue::get_precision() const
{
   if (const ir_dereference *d = as_dereference())
      return normalized(d->precision());
   if (const ir_texture *t = as_texture()) {
      switch (t->op) {
      case ir_txs: case ir_lod: case ir_query_levels: case ir_texture_samples:
         return GLSL_PRECISION_HIGH;
      default:
         return t->sampler ? t->sampler->get_precision() : GLSL_PRECISION_NONE;
      }
   }
   return source_precision;
}

void ir_rvalue::update_precision()
{
   if (const ir_swizzle *s = as_swizzle()) {
      source_precision = s->val->get_precision();
   }
   if (const ir_expression *e = as_expression()) {
      if (e->operation == ir_unop_b2f) {
         source_precision = GLSL_PRECISION_MEDIUM;
         return;
      }
      unsigned p = GLSL_PRECISION_NONE;
      for (unsigned i = 0; i < e->num_operands; ++i) {
         // Selection conditions and vector indices do not set result precision.
         if ((e->operation == ir_triop_csel && i == 0) ||
             (e->operation == ir_binop_vector_extract && i == 1) ||
             (e->operation == ir_triop_vector_insert && i == 2))
            continue;
         p = higher(p, e->operands[i]->get_precision());
      }
      source_precision = p;
   }
}

bool ir_precision_conversion(const ir_rvalue *lhs, const ir_rvalue *rhs)
{
   unsigned a = lhs->get_precision(), b = rhs->get_precision();
   return a != GLSL_PRECISION_NONE && b != GLSL_PRECISION_NONE && a != b;
}

static unsigned call_precision(ir_call *call)
{
   unsigned p = call->callee->return_precision;
   if (p != GLSL_PRECISION_NONE)
      return normalized(p);
   const char *name = call->callee->function_name();
   if (call->callee->is_builtin() &&
       (!strcmp(name, "textureSize") || !strcmp(name, "textureQueryLod") ||
        !strcmp(name, "textureQueryLevels") || !strcmp(name, "textureSamples")))
      return GLSL_PRECISION_HIGH;
   foreach_two_lists(formal, &call->callee->parameters, actual, &call->actual_parameters) {
      ir_variable *param = (ir_variable *) formal;
      ir_rvalue *arg = (ir_rvalue *) actual;
      // Texture results follow the sampler, never the coordinate or LOD.
      if (call->callee->is_builtin() && param->type->without_array()->is_sampler())
         return arg->get_precision();
      unsigned q = param->data.precision;
      p = higher(p, q == GLSL_PRECISION_NONE ? arg->get_precision() : q);
   }
   return p;
}

class precision_visitor : public ir_hierarchical_visitor {
public:
   std::unordered_map<ir_variable *, unsigned> candidates;

   ir_visitor_status visit_leave(ir_expression *e) override {
      e->update_precision();
      return visit_continue;
   }
   ir_visitor_status visit_leave(ir_swizzle *s) override {
      s->update_precision();
      return visit_continue;
   }

   void infer(ir_variable *var, unsigned p) {
      if (var && (var->data.precision == GLSL_PRECISION_NONE || var->data.precision_inferred))
         candidates[var] = higher(candidates[var], p);
   }

   ir_visitor_status visit_leave(ir_assignment *a) override {
      // A struct field's precision belongs to its type, not its containing var.
      if (ir_dereference_variable *d = a->lhs->as_dereference_variable())
         infer(d->var, a->rhs->get_precision());
      return visit_continue;
   }

   ir_visitor_status visit_leave(ir_call *call) override {
      if (call->return_deref)
         infer(call->return_deref->var, call_precision(call));
      return visit_continue;
   }
};

bool propagate_precision(exec_list *instructions)
{
   bool progress = false, changed;
   do {
      precision_visitor v;
      visit_list_elements(&v, instructions);
      changed = false;
      // Combine all assignments before choosing precision, independent of order.
      for (const auto &candidate : v.candidates) {
         ir_variable *var = candidate.first;
         unsigned p = higher(var->data.precision, candidate.second);
         if (p != var->data.precision) {
            var->data.precision = p;
            var->data.precision_inferred = true;
            changed = true;
         }
      }
      progress |= changed;
   } while (changed);
   return progress;
}
