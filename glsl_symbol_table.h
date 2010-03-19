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

#pragma once
#ifndef GLSL_SYMBOL_TABLE
#define GLSL_SYMBOL_TABLE

#include "symbol_table.h"
#include "ir.h"
#include "glsl_types.h"

/**
 * Facade class for _mesa_symbol_table
 *
 * Wraps the existing \c _mesa_symbol_table data structure to enforce some
 * type safe and some symbol table invariants.
 */
class glsl_symbol_table {
public:
   glsl_symbol_table()
   {
      table = _mesa_symbol_table_ctor();
   }

   ~glsl_symbol_table()
   {
      _mesa_symbol_table_dtor(table);
   }

   void push_scope()
   {
      _mesa_symbol_table_push_scope(table);
   }

   void pop_scope()
   {
      _mesa_symbol_table_pop_scope(table);
   }

   /**
    * \name Methods to add symbols to the table
    *
    * There is some temptation to rename all these functions to \c add_symbol
    * or similar.  However, this breaks symmetry with the getter functions and
    * reduces the clarity of the intention of code that uses these methods.
    */
   /*@{*/
   bool add_variable(const char *name, ir_variable *v)
   {
      return _mesa_symbol_table_add_symbol(table, 0, name, v) == 0;
   }

   bool add_type(const char *name, const glsl_type *t)
   {
      return _mesa_symbol_table_add_symbol(table, 0, name, (void *) t) == 0;
   }

   bool add_function(const char *name, ir_function *f)
   {
      return _mesa_symbol_table_add_symbol(table, 0, name, f) == 0;
   }
   /*@}*/

   /**
    * \name Methods to get symbols from the table
    */
   /*@{*/
   ir_variable *get_variable(const char *name)
   {
      return (ir_variable *) _mesa_symbol_table_find_symbol(table, 0, name);
   }

   glsl_type *get_type(const char *name)
   {
      return (glsl_type *) _mesa_symbol_table_find_symbol(table, 0, name);
   }

   ir_function *get_function(const char *name)
   {
      return (ir_function *) _mesa_symbol_table_find_symbol(table, 0, name);
   }
   /*@}*/

private:
   struct _mesa_symbol_table *table;
};

#endif /* GLSL_SYMBOL_TABLE */
