#pragma once

#include <math.h>

static inline float truncf (float x)
{
	if (x < 0.0f)
		return ceilf(x); 
	else
		return floorf(x);
}

static inline float exp2f (float x)
{
	return powf (2.0f, x);
}

static inline float log2f (float x)
{
	return logf (x) / logf (2.0f);
}
