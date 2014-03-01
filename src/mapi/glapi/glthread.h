#ifndef GLTHREAD_H
#define GLTHREAD_H

#include "u_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _glthread_InitTSD(tsd)               u_tsd_init(tsd);
#define _glthread_DestroyTSD(tsd)            u_tsd_destroy(tsd);
#define _glthread_GetTSD(tsd)                u_tsd_get(tsd);
#define _glthread_SetTSD(tsd, ptr)           u_tsd_set(tsd, ptr);

typedef struct u_tsd _glthread_TSD;

#ifdef __cplusplus
}
#endif

#endif /* GLTHREAD_H */
