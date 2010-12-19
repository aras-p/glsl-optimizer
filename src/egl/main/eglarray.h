#ifndef EGLARRAY_INCLUDED
#define EGLARRAY_INCLUDED


#include "egltypedefs.h"


typedef EGLBoolean (*_EGLArrayForEach)(void *elem, void *foreach_data);


struct _egl_array {
   const char *Name;
   EGLint MaxSize;

   void **Elements;
   EGLint Size;
};


extern _EGLArray *
_eglCreateArray(const char *name, EGLint init_size);


PUBLIC void
_eglDestroyArray(_EGLArray *array, void (*free_cb)(void *));


extern void
_eglAppendArray(_EGLArray *array, void *elem);


extern void
_eglEraseArray(_EGLArray *array, EGLint i, void (*free_cb)(void *));


void *
_eglFindArray(_EGLArray *array, void *elem);


PUBLIC EGLint
_eglFilterArray(_EGLArray *array, void **data, EGLint size,
                _EGLArrayForEach filter, void *filter_data);


EGLint
_eglFlattenArray(_EGLArray *array, void *buffer, EGLint elem_size, EGLint size,
                 _EGLArrayForEach flatten);


static INLINE EGLint
_eglGetArraySize(_EGLArray *array)
{
   return (array) ? array->Size : 0;
}


#endif /* EGLARRAY_INCLUDED */
