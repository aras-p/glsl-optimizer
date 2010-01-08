#include <radeon_bocs_wrapper.h>
#include <radeon_bo_int_drm.h>

void radeon_bo_debug(struct radeon_bo *bo,
		     const char *op)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;

    fprintf(stderr, "%s %p 0x%08X 0x%08X 0x%08X\n",
            op, bo, bo->handle, boi->size, boi->cref);
}

struct radeon_bo *radeon_bo_open(struct radeon_bo_manager *bom,
				 uint32_t handle,
				 uint32_t size,
				 uint32_t alignment,
				 uint32_t domains,
				 uint32_t flags)
{
    struct radeon_bo *bo;
    bo = bom->funcs->bo_open(bom, handle, size, alignment, domains, flags);
    return bo;
}

void radeon_bo_ref(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    boi->cref++;
    boi->bom->funcs->bo_ref(boi);
}

struct radeon_bo *radeon_bo_unref(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    boi->cref--;
    return boi->bom->funcs->bo_unref(boi);
}

int radeon_bo_map(struct radeon_bo *bo, int write)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->bom->funcs->bo_map(boi, write);
}

int radeon_bo_unmap(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->bom->funcs->bo_unmap(boi);
}

int radeon_bo_wait(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    if (!boi->bom->funcs->bo_wait)
	return 0;
    return boi->bom->funcs->bo_wait(boi);
}

int radeon_bo_is_busy(struct radeon_bo *bo,
		      uint32_t *domain)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->bom->funcs->bo_is_busy(boi, domain);
}

int radeon_bo_set_tiling(struct radeon_bo *bo,
			 uint32_t tiling_flags, uint32_t pitch)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->bom->funcs->bo_set_tiling(boi, tiling_flags, pitch);
}

int radeon_bo_get_tiling(struct radeon_bo *bo,
			  uint32_t *tiling_flags, uint32_t *pitch)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->bom->funcs->bo_get_tiling(boi, tiling_flags, pitch);
}

int radeon_bo_is_static(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    if (boi->bom->funcs->bo_is_static)
	return boi->bom->funcs->bo_is_static(boi);
    return 0;
}

int radeon_bo_is_referenced_by_cs(struct radeon_bo *bo,
				  struct radeon_cs *cs)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    return boi->cref > 1;
}

uint32_t radeon_bo_get_handle(struct radeon_bo *bo)
{
    return bo->handle;
}

uint32_t radeon_bo_get_src_domain(struct radeon_bo *bo)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    uint32_t src_domain;

    src_domain = boi->space_accounted & 0xffff;
    if (!src_domain)
	src_domain = boi->space_accounted >> 16;

    return src_domain;
}
