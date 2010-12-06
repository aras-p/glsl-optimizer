#include <stdlib.h>
#include <string.h>

#include "egllog.h"
#include "eglarray.h"


/**
 * Grow the size of the array.
 */
static EGLBoolean
_eglGrowArray(_EGLArray *array)
{
   EGLint new_size;
   void **elems;

   new_size = array->MaxSize;
   while (new_size <= array->Size)
      new_size *= 2;

   elems = realloc(array->Elements, new_size * sizeof(array->Elements[0]));
   if (!elems) {
      _eglLog(_EGL_DEBUG, "failed to grow %s array to %d",
            array->Name, new_size);
      return EGL_FALSE;
   }

   array->Elements = elems;
   array->MaxSize = new_size;

   return EGL_TRUE;
}


/**
 * Create an array.
 */
_EGLArray *
_eglCreateArray(const char *name, EGLint init_size)
{
   _EGLArray *array;

   array = calloc(1, sizeof(*array));
   if (array) {
      array->Name = name;
      array->MaxSize = (init_size > 0) ? init_size : 1;
      if (!_eglGrowArray(array)) {
         free(array);
         array = NULL;
      }
   }

   return array;
}


/**
 * Destroy an array, optionally free the data.
 */
void
_eglDestroyArray(_EGLArray *array, void (*free_cb)(void *))
{
   if (free_cb) {
      EGLint i;
      for (i = 0; i < array->Size; i++)
         free_cb(array->Elements[i]);
   }
   free(array->Elements);
   free(array);
}


/**
 * Append a element to an array.
 */
void
_eglAppendArray(_EGLArray *array, void *elem)
{
   if (array->Size >= array->MaxSize && !_eglGrowArray(array))
      return;

   array->Elements[array->Size++] = elem;
}


/**
 * Erase an element from an array.
 */
void
_eglEraseArray(_EGLArray *array, EGLint i, void (*free_cb)(void *))
{
   if (free_cb)
      free_cb(array->Elements[i]);
   if (i < array->Size - 1) {
      memmove(&array->Elements[i], &array->Elements[i + 1],
            (array->Size - i - 1) * sizeof(array->Elements[0]));
   }
   array->Size--;
}


/**
 * Find in an array for the given element.
 */
void *
_eglFindArray(_EGLArray *array, void *elem)
{
   EGLint i;

   if (!array)
      return NULL;

   for (i = 0; i < array->Size; i++)
      if (array->Elements[i] == elem)
         return elem;
   return NULL;
}


/**
 * Filter an array and return the number of filtered elements.
 */
EGLint
_eglFilterArray(_EGLArray *array, void **data, EGLint size,
                _EGLArrayForEach filter, void *filter_data)
{
   EGLint count = 0, i;

   if (!array)
      return 0;

   if (filter) {
      for (i = 0; i < array->Size; i++) {
         if (filter(array->Elements[i], filter_data)) {
            if (data && count < size)
               data[count] = array->Elements[i];
            count++;
         }
         if (data && count >= size)
            break;
      }
   }
   else {
      if (data) {
         count = (size < array->Size) ? size : array->Size;
         memcpy(data, array->Elements, count * sizeof(array->Elements[0]));
      }
      else {
         count = array->Size;
      }
   }

   return count;
}


/**
 * Flatten an array by converting array elements into another form and store
 * them in a buffer.
 */
EGLint
_eglFlattenArray(_EGLArray *array, void *buffer, EGLint elem_size, EGLint size,
                 _EGLArrayForEach flatten)
{
   EGLint i, count;

   if (!array)
      return 0;

   count = array->Size;
   if (buffer) {
      /* do not exceed buffer size */
      if (count > size)
         count = size;
      for (i = 0; i < count; i++)
         flatten(array->Elements[i],
               (void *) ((char *) buffer + elem_size * i));
   }

   return count;
}
