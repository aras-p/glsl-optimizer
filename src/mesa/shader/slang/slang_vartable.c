
#include "imports.h"
#include "slang_compile.h"
#include "slang_compile_variable.h"
#include "slang_vartable.h"
#include "slang_ir.h"


static int dbg = 0;


typedef enum {
   FREE,
   VAR,
   TEMP
} TempState;

static int Level = 0;

struct slang_var_table_
{
   int level;
   int num_entries;
   slang_variable **vars;  /* array [num_entries] */

   TempState temps[MAX_PROGRAM_TEMPS];

   struct slang_var_table_ *parent;
};



/**
 * Create new table, put at head, return ptr to it.
 */
slang_var_table *
_slang_push_var_table(slang_var_table *parent)
{
   slang_var_table *t
      = (slang_var_table *) _mesa_calloc(sizeof(slang_var_table));
   if (t) {
      t->level = Level++;
      t->parent = parent;
      if (parent) {
         /* copy the info indicating which temp regs are in use */
         memcpy(t->temps, parent->temps, sizeof(t->temps));
      }
      if (dbg) printf("Pushing level %d\n", t->level);
   }
   return t;
}


/**
 * Destroy given table, return ptr to parent
 */
slang_var_table *
_slang_pop_var_table(slang_var_table *t)
{
   slang_var_table *parent = t->parent;
   int i;

   if (dbg) printf("Popping level %d\n", t->level);

   /* free the storage allocated for each variable */
   for (i = 0; i < t->num_entries; i++) {
      slang_ir_storage *store = (slang_ir_storage *) t->vars[i]->aux;
      if (dbg) printf("  Free var %s\n", (char*) t->vars[i]->a_name);
      assert(t->temps[store->Index] == VAR);
      t->temps[store->Index] = FREE;
      store->Index = -1;
   }
   if (t->parent) {
      /* just verify that any remaining allocations in this scope 
       * were for temps
       */
      for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
         if (t->temps[i] && !t->parent->temps[i]) {
            if (dbg) printf("  Free reg %d\n", i);
            assert(t->temps[i] == TEMP);
         }
      }
   }

   if (t->vars)
      free(t->vars);
   free(t);
   Level--;
   return parent;
}


/**
 * Add a new variable to the given symbol table.
 */
void
_slang_add_variable(slang_var_table *t, slang_variable *v)
{
   assert(t);
   if (dbg) printf("Adding var %s\n", (char *) v->a_name);
   t->vars = realloc(t->vars, (t->num_entries + 1) * sizeof(slang_variable *));
   t->vars[t->num_entries] = v;
   t->num_entries++;
}


/**
 * Look for variable by name in given table.
 * If not found, parent table will be searched.
 */
slang_variable *
_slang_find_variable(const slang_var_table *t, slang_atom name)
{
   while (1) {
      int i;
      for (i = 0; i < t->num_entries; i++) {
         if (t->vars[i]->a_name == name)
            return t->vars[i];
      }
      if (t->parent)
         t = t->parent;
      else
         return NULL;
   }
}


static GLint
alloc_reg(slang_var_table *t, GLint size, GLboolean isTemp)
{
   const GLuint sz4 = (size + 3) / 4;
   GLuint i, j;
   assert(size > 0); /* number of floats */

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      GLuint found = 0;
      for (j = 0; j < sz4; j++) {
         if (!t->temps[i + j]) {
            found++;
         }
         else {
            break;
         }
      }
      if (found == sz4) {
         /* found block of size/4 free regs */
         for (j = 0; j < sz4; j++)
            t->temps[i + j] = isTemp ? TEMP : VAR;
         return i;
      }
   }
   return -1;
}


/**
 * Allocate temp register(s) for storing a variable.
 */
GLint
_slang_alloc_var(slang_var_table *t, GLint size)
{
   int i = alloc_reg(t, size, GL_FALSE);
   if (dbg) printf("Alloc var %d (level %d)\n", i, t->level);
   return i;
}


void
_slang_reserve_var(slang_var_table *t, GLint r, GLint size)
{
   const GLint sz4 = (size + 3) / 4;
   GLint i;
   for (i = 0; i < sz4; i++) {
      t->temps[r + i] = VAR;
   }
}


/**
 * Allocate temp register(s) for storing an unnamed intermediate value.
 */
GLint
_slang_alloc_temp(slang_var_table *t, GLint size)
{
   int i = alloc_reg(t, size, GL_TRUE);
   if (dbg) printf("Alloc temp %d (level %d)\n", i, t->level);
   return i;
}


void
_slang_free_temp(slang_var_table *t, GLint r, GLint size)
{
   const GLuint sz4 = (size + 3) / 4;
   GLuint i;
   assert(size > 0);
   assert(r >= 0);
   assert(r < MAX_PROGRAM_TEMPS);
   if (dbg) printf("Free temp %d (level %d)\n", r, t->level);
   for (i = 0; i < sz4; i++) {
      assert(t->temps[r + i] == TEMP);
      t->temps[r + i] = FREE;
   }
}


GLboolean
_slang_is_temp(slang_var_table *t, GLint r)
{
   assert(r >= 0);
   assert(r < MAX_PROGRAM_TEMPS);
   if (t->temps[r] == TEMP)
      return GL_TRUE;
   else
      return GL_FALSE;
}
