default: full

all: full subset
	
%.tag: %.doxy
	doxygen $<

FULL = \
	core.doxy \
	math.doxy \
	array_cache.doxy \
	swrast.doxy \
	swrast_setup.doxy \
	tnl.doxy \
	tnl_dd.doxy

full: $(FULL:.doxy=.tag)
	$(foreach FILE,$(FULL),doxygen $(FILE);)

SUBSET = \
	core.doxy \
	math.doxy \
	miniglx.doxy

subset: $(SUBSET:.doxy=.tag)
	$(foreach FILE,$(SUBSET),doxygen $(FILE);)

clean:
	rm -rf $(FULL:.doxy=) $(SUBSET:.doxy=)
	rm -rf *.tag
