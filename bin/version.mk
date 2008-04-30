#!/usr/bin/make -sf
# Print the various Mesa version fields. This is mostly used to add the
# version to configure.

# This reflects that this script is usually called from the toplevel
TOP = .

include $(TOP)/configs/default

version:
	@echo $(MESA_VERSION)
major:
	@echo $(MESA_MAJOR)
minor:
	@echo $(MESA_MINOR)
tiny:
	@echo $(MESA_TINY)
