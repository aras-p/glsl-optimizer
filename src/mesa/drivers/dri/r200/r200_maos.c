

/* If using new packets, can choose either verts or arrays.
 * Otherwise, must use verts.
 */
#include "r200_context.h"
#define R200_MAOS_VERTS 0
#if (R200_MAOS_VERTS) || (R200_OLD_PACKETS)
#include "r200_maos_verts.c"
#else
#include "r200_maos_arrays.c"
#endif
