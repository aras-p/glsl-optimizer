#ifndef T
{
	if(dst->bpps == 0)
#define T uint8_t
#include "nv04_2d_loops.h"
#undef T
	else if(dst->bpps == 1)
#define T uint16_t
#include "nv04_2d_loops.h"
#undef T
	else if(dst->bpps == 2)
#define T uint32_t
#include "nv04_2d_loops.h"
#undef T
	else
		assert(0);
}
#else
#ifdef SWIZZLED_COPY_LOOPS
{
	if(!dst->pitch)
	{
		if(!src->pitch)
		{
			LOOP_Y
			{
				T* pdst = (T*)mdst + dswy[iy];
				T* psrc = (T*)msrc + sswy[iy];
				LOOP_X
				{
					assert((char*)&psrc[sswx[ix] + 1] <= ((char*)src->bo->map + src->bo->size));
					assert((char*)&pdst[dswx[ix] + 1] <= ((char*)dst->bo->map + dst->bo->size));
					pdst[dswx[ix]] = psrc[sswx[ix]];
				}
			}
		}
		else
		{
			T* psrc = (T*)(msrc + ((dir > 0) ? src->y : (src->y + h - 1)) * src->pitch) + src->x;
			LOOP_Y
			{
				T* pdst = (T*)mdst + dswy[iy];
				LOOP_X
				{
					assert((char*)&psrc[ix + 1] <= ((char*)src->bo->map + src->bo->size));
					assert((char*)&pdst[dswx[ix] + 1] <= ((char*)dst->bo->map + dst->bo->size));
					pdst[dswx[ix]] = psrc[ix];
				}
				psrc = (T*)((char*)psrc + dir * src->pitch);
			}
		}
	}
	else
	{
		T* pdst = (T*)(mdst + ((dir > 0) ? dst->y : (dst->y + h - 1)) * dst->pitch) + dst->x;
		LOOP_Y
		{
			T* psrc = (T*)msrc + sswy[iy];
			LOOP_X
			{
				assert((char*)&psrc[sswx[ix] + 1] <= ((char*)src->bo->map + src->bo->size));
				assert((char*)&pdst[ix + 1] <= ((char*)dst->bo->map + dst->bo->size));
				pdst[ix] = psrc[sswx[ix]];
			}
			pdst = (T*)((char*)pdst + dir * dst->pitch);
		}
	}
}
#endif
#endif
