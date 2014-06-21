# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.

if BUILD_SHARED
if HAVE_COMPAT_SYMLINKS
all-local : .libs/install-gallium-links

.libs/install-gallium-links : $(dri_LTLIBRARIES) $(egl_LTLIBRARIES) $(lib_LTLIBRARIES)
	$(AM_V_GEN)$(MKDIR_P) $(top_builddir)/$(LIB_DIR);	\
	link_dir=$(top_builddir)/$(LIB_DIR)/gallium;		\
	if test x$(egl_LTLIBRARIES) != x; then			\
		link_dir=$(top_builddir)/$(LIB_DIR)/egl;	\
	fi;							\
	$(MKDIR_P) $$link_dir;					\
	file_list=$(dri_LTLIBRARIES:%.la=.libs/%.so);		\
	file_list+=$(egl_LTLIBRARIES:%.la=.libs/%.$(LIB_EXT)*);	\
	file_list+=$(lib_LTLIBRARIES:%.la=.libs/%.$(LIB_EXT)*);	\
	for f in $$file_list; do 				\
		if test -h .libs/$$f; then			\
			cp -d $$f $$link_dir;			\
		else						\
			ln -f $$f $$link_dir;			\
		fi;						\
	done && touch $@
endif
endif
