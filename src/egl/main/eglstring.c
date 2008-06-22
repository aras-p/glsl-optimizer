/**
 * String utils.
 */

#include <stdlib.h>
#include <string.h>
#include "eglstring.h"


char *
_eglstrdup(const char *s)
{
   if (s) {
      int l = strlen(s);
      char *s2 = malloc(l + 1);
      if (s2)
         strcpy(s2, s);
      return s2;
   }
   return NULL;
}



