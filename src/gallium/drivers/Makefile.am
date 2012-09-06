AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	$(DEFINES)

AM_CFLAGS = $(VISIBILITY_CFLAGS)

noinst_LTLIBRARIES =

SUBDIRS = .

################################################################################

if HAVE_GALAHAD_GALLIUM

noinst_LTLIBRARIES += galahad/libgalahad.la

galahad_libgalahad_la_SOURCES = \
	galahad/glhd_objects.c \
	galahad/glhd_context.c \
	galahad/glhd_screen.c

endif

################################################################################

if HAVE_IDENTITY_GALLIUM

noinst_LTLIBRARIES += identity/libidentity.la

identity_libidentity_la_SOURCES = \
	identity/id_objects.c \
	identity/id_context.c \
	identity/id_screen.c

endif

################################################################################

if HAVE_NOOP_GALLIUM

# Meta-driver which combines whichever software rasterizers have been
# built into a single convenience library.

noinst_LTLIBRARIES += noop/libnoop.la

noop_libnoop_la_SOURCES = \
	noop/noop_pipe.c \
	noop/noop_state.c

endif

################################################################################

if NEED_RADEON_GALLIUM

SUBDIRS += radeon

endif

################################################################################

SUBDIRS += $(GALLIUM_MAKE_DIRS)
