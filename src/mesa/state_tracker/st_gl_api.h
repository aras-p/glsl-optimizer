
#ifndef ST_GL_API_H
#define ST_GL_API_H

#include "state_tracker/st_api.h"

struct st_api *st_gl_api_create(void);
struct st_api *st_gl_api_create_es1(void);
struct st_api *st_gl_api_create_es2(void);

#endif
