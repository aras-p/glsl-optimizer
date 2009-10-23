

void brw_buffer_subdata()
{
      if (intel->intelScreen->kernel_exec_fencing) {
	 drm_intel_gem_bo_map_gtt(bo);
	 memcpy((char *)bo->virtual + offset, index_buffer->ptr, ib_size);
	 drm_intel_gem_bo_unmap_gtt(bo);
      } else {
	 dri_bo_subdata(bo, offset, ib_size, index_buffer->ptr);
      }
}
