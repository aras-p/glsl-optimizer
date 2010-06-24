#include <assert.h>
#include <stdlib.h>

#define _mesa_malloc(x)   malloc(x)
#define _mesa_free(x)     free(x)
#define _mesa_calloc(x)   calloc(1,x)
