/* -*- c++ -*- */
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include "s_expression.h"

s_symbol::s_symbol(const char *tmp, size_t n)
{
   this->str = talloc_strndup (this, tmp, n);
   assert(this->str != NULL);
}

s_list::s_list()
{
}

static s_expression *
read_atom(void *ctx, const char *& src)
{
   s_expression *expr = NULL;

   // Skip leading spaces.
   src += strspn(src, " \v\t\r\n");

   size_t n = strcspn(src, "( \v\t\r\n)");
   if (n == 0)
      return NULL; // no atom

   // Check if the atom is a number.
   char *float_end = NULL;
   double f = glsl_strtod(src, &float_end);
   if (float_end != src) {
      char *int_end = NULL;
      int i = strtol(src, &int_end, 10);
      // If strtod matched more characters, it must have a decimal part
      if (float_end > int_end)
	 expr = new(ctx) s_float(f);
      else
	 expr = new(ctx) s_int(i);
   } else {
      // Not a number; return a symbol.
      expr = new(ctx) s_symbol(src, n);
   }

   src += n;

   return expr;
}

s_expression *
s_expression::read_expression(void *ctx, const char *&src)
{
   assert(src != NULL);

   s_expression *atom = read_atom(ctx, src);
   if (atom != NULL)
      return atom;

   // Skip leading spaces.
   src += strspn(src, " \v\t\r\n");
   if (src[0] == '(') {
      ++src;

      s_list *list = new(ctx) s_list;
      s_expression *expr;

      while ((expr = read_expression(ctx, src)) != NULL) {
	 list->subexpressions.push_tail(expr);
      }
      src += strspn(src, " \v\t\r\n");
      if (src[0] != ')') {
	 printf("Unclosed expression (check your parenthesis).\n");
	 return NULL;
      }
      ++src;
      return list;
   }
   return NULL;
}

void s_int::print()
{
   printf("%d", this->val);
}

void s_float::print()
{
   printf("%f", this->val);
}

void s_symbol::print()
{
   printf("%s", this->str);
}

void s_list::print()
{
   printf("(");
   foreach_iter(exec_list_iterator, it, this->subexpressions) {
      s_expression *expr = (s_expression*) it.get();
      expr->print();
      if (!expr->next->is_tail_sentinel())
	 printf(" ");
   }
   printf(")");
}

// --------------------------------------------------

bool
s_pattern::match(s_expression *expr)
{
   switch (type)
   {
   case EXPR:   *p_expr = expr; break;
   case LIST:   if (expr->is_list())   *p_list   = (s_list *)   expr; break;
   case SYMBOL: if (expr->is_symbol()) *p_symbol = (s_symbol *) expr; break;
   case NUMBER: if (expr->is_number()) *p_number = (s_number *) expr; break;
   case INT:    if (expr->is_int())    *p_int    = (s_int *)    expr; break;
   case STRING:
      s_symbol *sym = SX_AS_SYMBOL(expr);
      if (sym != NULL && strcmp(sym->value(), literal) == 0)
	 return true;
      return false;
   };

   return *p_expr == expr;
}

bool
s_match(s_expression *top, unsigned n, s_pattern *pattern, bool partial)
{
   s_list *list = SX_AS_LIST(top);
   if (list == NULL)
      return false;

   unsigned i = 0;
   foreach_iter(exec_list_iterator, it, list->subexpressions) {
      if (i >= n)
	 return partial; /* More actual items than the pattern expected */

      s_expression *expr = (s_expression *) it.get();
      if (expr == NULL || !pattern[i].match(expr))
	 return false;

      i++;
   }

   if (i < n)
      return false; /* Less actual items than the pattern expected */

   return true;
}
