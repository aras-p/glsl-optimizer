/* this file uses the C++ global constructor mechanism to automatically
   initialize global data

   __attribute__((constructor)) allows to do this in C, but is GCC-only
*/

#include "u_half.h"

struct util_gctor_t
{
	util_gctor_t()
	{
		util_half_init_tables();
	}
};

static struct util_gctor_t util_gctor;
