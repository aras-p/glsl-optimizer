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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_unused_structs.h"
#include "glsl_types.h"

struct struct_entry : public exec_node
{
	struct_entry(const glsl_type *type_) : type(type_) { }
	const glsl_type *type;
};


bool
ir_struct_usage_visitor::has_struct_entry(const glsl_type *t) const
{
	assert(t);
	foreach_iter(exec_list_iterator, iter, this->struct_list) {
		struct_entry *entry = (struct_entry *)iter.get();
		if (entry->type == t)
			return true;
	}
	return false;
}


ir_visitor_status
ir_struct_usage_visitor::visit(ir_dereference_variable *ir)
{
	const glsl_type* t = ir->type;
	if (t->base_type == GLSL_TYPE_STRUCT)
	{
		if (!has_struct_entry (t))
		{
			struct_entry *entry = new(mem_ctx) struct_entry(t);
			this->struct_list.push_tail (entry);
		}
	}
	return visit_continue;
}

/*
bool
do_unused_function_removal(exec_list *instructions)
{
	ir_struct_usage_visitor v;
	v.run (instructions);

	bool progress = false;
	foreach_iter(exec_list_iterator, iter, *instructions) {
		ir_instruction *ir = (ir_instruction *)iter.get();
		ir_function* func = ir->as_function();
		if (!func)
			continue;

		if (!strcmp(func->name, "main"))
			continue;

		foreach_list_safe(node, &func->get_signatures()) {
			ir_function_signature *const sig = (ir_function_signature *const) node;
			if (sig->is_built_in)
				continue;
			if (!v.has_function_entry (sig))
			{
				sig->remove();
				progress = true;
			}
		}
	}
	return progress;
}
*/
