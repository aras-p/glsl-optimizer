default:
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


clean:
	rm -rf array_cache core math swrast swrast_setup tnl_dd tnl
	rm -rf *.tag

