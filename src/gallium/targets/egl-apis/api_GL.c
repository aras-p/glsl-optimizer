#include "state_tracker/st_gl_api.h"

#if FEATURE_GL
PUBLIC struct st_api *
st_api_create_OpenGL(void)
{
   return st_gl_api_create();
}
#endif

#if FEATURE_ES1
PUBLIC struct st_api *
st_api_create_OpenGL_ES1(void)
{
   return st_gl_api_create_es1();
}
#endif

#if FEATURE_ES2
PUBLIC struct st_api *
st_api_create_OpenGL_ES2(void)
{
   return st_gl_api_create_es2();
}
#endif
