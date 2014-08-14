# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.

if BUILD_SHARED
if HAVE_COMPAT_SYMLINKS
all-local : .libs/install-mesa-links

.libs/install-mesa-links : $(lib_LTLIBRARIES)
	$(AM_V_GEN)$(MKDIR_P) $(top_builddir)/$(LIB_DIR);	\
	for f in $(join $(addsuffix .libs/,$(dir $(lib_LTLIBRARIES))),$(notdir $(lib_LTLIBRARIES:%.la=%.$(LIB_EXT)*))); do \
		if test -h .libs/$$f; then			\
			cp -d $$f $(top_builddir)/$(LIB_DIR);	\
		else						\
			ln -f $$f $(top_builddir)/$(LIB_DIR);	\
		fi;						\
	done && touch $@
endif
endif
