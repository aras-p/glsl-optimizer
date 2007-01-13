
#ifndef SLANG_VARTABLE_H
#define SLANG_VARTABLE_H


typedef struct slang_var_table_ slang_var_table;

struct slang_variable_;

extern slang_var_table *
_slang_push_var_table(slang_var_table *parent);

extern slang_var_table *
_slang_pop_var_table(slang_var_table *t);

extern void
_slang_add_variable(slang_var_table *t, struct slang_variable_ *v);

extern struct slang_variable_ *
_slang_find_variable(const slang_var_table *t, slang_atom name);

extern GLint
_slang_alloc_var(slang_var_table *t, GLint size);

extern void
_slang_reserve_var(slang_var_table *t, GLint r, GLint size);

extern GLint
_slang_alloc_temp(slang_var_table *t, GLint size);

extern void
_slang_free_temp(slang_var_table *t, GLint r, GLint size);

extern GLboolean
_slang_is_temp(slang_var_table *t, GLint r);


#endif /* SLANG_VARTABLE_H */
