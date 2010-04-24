#include "state_tracker/st_gl_api.h"

PUBLIC struct st_api *
st_api_create_OpenGL_ES2()
{
   /* linker magic creates different versions */
   return st_gl_api_create();
}
