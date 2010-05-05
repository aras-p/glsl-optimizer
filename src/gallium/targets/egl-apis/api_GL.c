#include "state_tracker/st_gl_api.h"

PUBLIC struct st_api *
st_api_create_OpenGL()
{
   return st_gl_api_create();
}
