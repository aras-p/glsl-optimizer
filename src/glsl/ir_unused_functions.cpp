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
#include <string.h>

struct function_entry : public exec_node
{
	function_entry(ir_function_signature *func_) : func(func_) { }
	ir_function_signature *func;
};

class ir_function_usage_visitor : public ir_hierarchical_visitor {
public:
	ir_function_usage_visitor(void)
	{
		this->mem_ctx = talloc_new(NULL);
		this->function_list.make_empty();
	}

	~ir_function_usage_visitor(void)
	{
		talloc_free(mem_ctx);
	}

	virtual ir_visitor_status visit_enter(ir_call *);

	bool has_function_entry(const ir_function_signature *var) const;

	/* List of function_entry */
	exec_list function_list;

	void *mem_ctx;
};


bool
ir_function_usage_visitor::has_function_entry(const ir_function_signature *var) const
{
	assert(var);
	foreach_iter(exec_list_iterator, iter, this->function_list) {
		function_entry *entry = (function_entry *)iter.get();
		if (entry->func == var)
			return true;
	}
	return false;
}


ir_visitor_status
ir_function_usage_visitor::visit_enter(ir_call *ir)
{
	if (!has_function_entry (ir->get_callee()))
	{
		function_entry *entry = new(mem_ctx) function_entry(ir->get_callee());
		this->function_list.push_tail (entry);
	}
	return visit_continue;
}


bool
do_unused_function_removal(exec_list *instructions)
{
	ir_function_usage_visitor v;
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
