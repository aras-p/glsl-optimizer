#ifndef _GGI_MESA_INT_H
#define _GGI_MESA_INT_H

#include <ggi/internal/internal.h>
#include "ggimesa.h"

#define GGI_SYMNAME_PREFIX  "MesaGGIdl_"

extern ggi_extid ggiMesaID;

ggifunc_setmode GGIMesa_setmode;
ggifunc_getapi GGIMesa_getapi;

typedef struct mesa_ext
{
	void (*update_state)(GLcontext *ctx);
	int (*setup_driver)(GGIMesaContext ctx, struct ggi_mesa_info *info);
	void *private;
} mesaext;

#define LIBGGI_MESAEXT(vis) ((mesaext *)LIBGGI_EXT(vis,ggiMesaID))
#define GGIMESA_PRIVATE(vis) ((LIBGGI_MESAEXT(vis)->private))

#endif /* _GGI_MISC_INT_H */
