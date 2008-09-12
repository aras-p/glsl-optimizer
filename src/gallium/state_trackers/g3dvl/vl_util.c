#include "vl_util.h"
#include <assert.h>

unsigned int vlRoundUpPOT(unsigned int x)
{
	unsigned int i;

	assert(x > 0);

	--x;

	for (i = 1; i < sizeof(unsigned int) * 8; i <<= 1)
		x |= x >> i;

	return x + 1;
}
