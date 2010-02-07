#include "state_tracker/st_manager.h"

PUBLIC const int st_api_OpenGL_ES2 = 1;

PUBLIC const struct st_module st_module_OpenGL_ES2 = {
   .api = ST_API_OPENGL_ES2,
   .create_api = st_manager_create_api
};
