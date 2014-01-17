/**************************************************************************
 * 
 * Copyright 2008 VMware, Inc.
 * All Rights Reserved.
 *
 **************************************************************************/


#ifndef ST_CB_DRAWTEX_H
#define ST_CB_DRAWTEX_H


#include "main/compiler.h"

struct dd_function_table;
struct st_context;

extern void
st_init_drawtex_functions(struct dd_function_table *functions);

extern void
st_destroy_drawtex(struct st_context *st);

#endif /* ST_CB_DRAWTEX_H */
