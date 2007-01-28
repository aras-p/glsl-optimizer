
#include "imports.h"
#include "slang_compile.h"
#include "slang_compile_variable.h"
#include "slang_vartable.h"
#include "slang_ir.h"
#include "prog_instruction.h"


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

   TempState temps[MAX_PROGRAM_TEMPS * 4];  /* per-component state */
   int size[MAX_PROGRAM_TEMPS];     /* For debug only */

   struct slang_var_table_ *parent;
};



/**
 * Create new table, put at head, return ptr to it.
 * XXX we should take a maxTemps parameter to indicate how many temporaries
 * are available for the current shader/program target.
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
         memcpy(t->size, parent->size, sizeof(t->size));
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
      GLint j;
      const GLuint sz = store->Size;
      GLuint comp;
      if (dbg) printf("  Free var %s, size %d at %d\n",
                      (char*) t->vars[i]->a_name, store->Size,
                      store->Index);

      if (sz == 1)
         comp = GET_SWZ(store->Swizzle, 0);
      else
         comp = 0;

      assert(store->Index >= 0);
      for (j = 0; j < sz; j++) {
         assert(t->temps[store->Index * 4 + j + comp] == VAR);
         t->temps[store->Index * 4 + j + comp] = FREE;
      }
      store->Index = -1;
   }
   if (t->parent) {
      /* just verify that any remaining allocations in this scope 
       * were for temps
       */
      for (i = 0; i < MAX_PROGRAM_TEMPS * 4; i++) {
         if (t->temps[i] != FREE && t->parent->temps[i] == FREE) {
            if (dbg) printf("  Free reg %d\n", i/4);
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


/**
 * Allocation helper.
 * \param size  var size in floats
 * \return  position for var, measured in floats
 */
static GLint
alloc_reg(slang_var_table *t, GLint size, GLboolean isTemp)
{
   /* if size == 1, allocate anywhere, else, pos must be multiple of 4 */
   const GLuint step = (size == 1) ? 1 : 4;
   GLuint i, j;
   assert(size > 0); /* number of floats */

   for (i = 0; i < MAX_PROGRAM_TEMPS - size; i += step) {
      GLuint found = 0;
      for (j = 0; j < size; j++) {
         if (i + j < MAX_PROGRAM_TEMPS && t->temps[i + j] == FREE) {
            found++;
         }
         else {
            break;
         }
      }
      if (found == size) {
         /* found block of size free regs */
         if (size > 1)
            assert(i % 4 == 0);
         for (j = 0; j < size; j++)
            t->temps[i + j] = isTemp ? TEMP : VAR;
         printf("t->size[%d] = %d\n", i, size);
         t->size[i] = size;
         return i;
      }
   }
   return -1;
}


/**
 * Allocate temp register(s) for storing a variable.
 * \param size  size needed, in floats
 * \param swizzle  returns swizzle mask for accessing var in register
 * \return  register allocated, or -1
 */
GLboolean
_slang_alloc_var(slang_var_table *t, slang_ir_storage *store)
{
   const int i = alloc_reg(t, store->Size, GL_FALSE);
   if (i < 0)
      return GL_FALSE;

   store->Index = i / 4;
   if (store->Size == 1) {
      const GLuint comp = i % 4;
      store->Swizzle = MAKE_SWIZZLE4(comp, comp, comp, comp);
      if (dbg) printf("Alloc var sz %d at %d.%c (level %d)\n",
                      store->Size, store->Index, "xyzw"[comp], t->level);
   }
   else {
      store->Swizzle = SWIZZLE_NOOP;
      if (dbg) printf("Alloc var sz %d at %d.xyzw (level %d)\n",
                      store->Size, store->Index, t->level);
   }
   return GL_TRUE;
}



/**
 * Allocate temp register(s) for storing an unnamed intermediate value.
 */
GLboolean
_slang_alloc_temp(slang_var_table *t, slang_ir_storage *store)
{
   const int i = alloc_reg(t, store->Size, GL_TRUE);
   if (i < 0)
      return GL_FALSE;

   store->Index = i / 4;
   if (store->Size == 1) {
      const GLuint comp = i % 4;
      store->Swizzle = MAKE_SWIZZLE4(comp, comp, comp, comp);
      if (dbg) printf("Alloc temp sz %d at %d.%c (level %d)\n",
                      store->Size, store->Index, "xyzw"[comp], t->level);
   }
   else {
      store->Swizzle = SWIZZLE_NOOP;
      if (dbg) printf("Alloc temp sz %d at %d.xyzw (level %d)\n",
                      store->Size, store->Index, t->level);
   }
   return GL_TRUE;
}


void
_slang_free_temp(slang_var_table *t, slang_ir_storage *store)
{
   GLuint i;
   GLuint r = store->Index;
   assert(store->Size > 0);
   assert(r >= 0);
   assert(r + store->Size <= MAX_PROGRAM_TEMPS);
   if (dbg) printf("Free temp sz %d at %d (level %d)\n", store->Size, r, t->level);
   if (store->Size == 1) {
      const GLuint comp = GET_SWZ(store->Swizzle, 0);
      assert(store->Swizzle == MAKE_SWIZZLE4(comp, comp, comp, comp));
      assert(comp < 4);
      assert(t->size[r * 4 + comp] == 1);
      assert(t->temps[r * 4 + comp] == TEMP);
      t->temps[r * 4 + comp] = FREE;
   }
   else {
      assert(store->Swizzle == SWIZZLE_NOOP);
      assert(t->size[r*4] == store->Size);
      for (i = 0; i < store->Size; i++) {
         assert(t->temps[r * 4 + i] == TEMP);
         t->temps[r * 4 + i] = FREE;
      }
   }
}


GLboolean
_slang_is_temp(slang_var_table *t, slang_ir_storage *store)
{
   assert(store->Index >= 0);
   assert(store->Index < MAX_PROGRAM_TEMPS);
   GLuint comp;
   if (store->Swizzle == SWIZZLE_NOOP)
      comp = 0;
   else
      comp = GET_SWZ(store->Swizzle, 0);

   if (t->temps[store->Index * 4 + comp] == TEMP)
      return GL_TRUE;
   else
      return GL_FALSE;
}
