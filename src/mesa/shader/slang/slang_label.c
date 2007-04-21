

/**
 * Functions for managing instruction labels.
 * Basically, this is used to manage the problem of forward branches where
 * we have a branch instruciton but don't know the target address yet.
 */


#include "slang_label.h"
#include "slang_mem.h"



slang_label *
_slang_label_new(const char *name)
{
#if !USE_MEMPOOL
   slang_label *l = (slang_label *) _mesa_calloc(sizeof(slang_label));
#else
   slang_label *l = (slang_label *) _slang_alloc(sizeof(slang_label));
#endif
   if (l) {
#if !USE_MEMPOOL
      l->Name = _mesa_strdup(name);
#else
      l->Name = _slang_strdup(name);
#endif
      l->Location = -1;
   }
   return l;
}

/**
 * As above, but suffix the name with a unique number.
 */
slang_label *
_slang_label_new_unique(const char *name)
{
   static int id = 1;
#if !USE_MEMPOOL
   slang_label *l = (slang_label *) _mesa_calloc(sizeof(slang_label));
#else
   slang_label *l = (slang_label *) _slang_alloc(sizeof(slang_label));
#endif
   if (l) {
#if !USE_MEMPOOL
      l->Name = (char *) _mesa_malloc(_mesa_strlen(name) + 10);
#else
      l->Name = (char *) _slang_alloc(_mesa_strlen(name) + 10);
#endif
      if (!l->Name) {
         _mesa_free(l);
         return NULL;
      }
      _mesa_sprintf(l->Name, "%s_%d", name, id);
      id++;
      l->Location = -1;
   }
   return l;
}

void
_slang_label_delete(slang_label *l)
{
#if !USE_MEMPOOL
   if (l->Name)
      _mesa_free(l->Name);
   if (l->References)
      _mesa_free(l->References);
   _mesa_free(l);
#endif
}


void
_slang_label_add_reference(slang_label *l, GLuint inst)
{
   const GLuint oldSize = l->NumReferences * sizeof(GLuint);
   assert(l->Location < 0);
#if !USE_MEMPOOL
   l->References = _mesa_realloc(l->References,
                                 oldSize, oldSize + sizeof(GLuint));
#else
   l->References = _slang_realloc(l->References,
                                  oldSize, oldSize + sizeof(GLuint));
#endif
   if (l->References) {
      l->References[l->NumReferences] = inst;
      l->NumReferences++;
   }
}


GLint
_slang_label_get_location(const slang_label *l)
{
   return l->Location;
}


void
_slang_label_set_location(slang_label *l, GLint location,
                          struct gl_program *prog)
{
   GLuint i;

   assert(l->Location < 0);
   assert(location >= 0);

   l->Location = location;

   /* for the instructions that were waiting to learn the label's location: */
   for (i = 0; i < l->NumReferences; i++) {
      const GLuint j = l->References[i];
      prog->Instructions[j].BranchTarget = location;
   }

   if (l->References) {
#if !USE_MEMPOOL
      _mesa_free(l->References);
#endif
      l->References = NULL;
   }
}
