default: full

all: full subset
	
full:
	doxygen tnl_dd.doxy
	doxygen array_cache.doxy
	doxygen math.doxy
	doxygen swrast.doxy
	doxygen swrast_setup.doxy
	doxygen tnl.doxy
	doxygen core.doxy
	echo "Building again, to resolve tags"
	doxygen tnl_dd.doxy
	doxygen array_cache.doxy
	doxygen math.doxy
	doxygen swrast.doxy
	doxygen swrast_setup.doxy
	doxygen tnl.doxy
	doxygen core.doxy

subset:
	doxygen core_subset.doxy
	doxygen math_subset.doxy
	doxygen miniglx.doxy
	echo "Building again, to resolve tags"
	doxygen core_subset.doxy
	doxygen math_subset.doxy
	doxygen miniglx.doxy
	doxygen radeon_subset.doxy

clean:
	rm -rf \
		array_cache \
		core \
		core_subset \
		math \
		math_subset \
		swrast \
		swrast_setup \
		tnl_dd \
		tnl \
		miniglx \
		radeon_subset
	rm -rf *.tag

