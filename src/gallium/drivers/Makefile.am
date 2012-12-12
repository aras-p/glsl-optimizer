AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	$(DEFINES)

AM_CFLAGS = $(VISIBILITY_CFLAGS)

noinst_LTLIBRARIES =

SUBDIRS = . trace rbug

################################################################################

noinst_LTLIBRARIES += galahad/libgalahad.la

galahad_libgalahad_la_SOURCES = \
	galahad/glhd_objects.c \
	galahad/glhd_context.c \
	galahad/glhd_screen.c

################################################################################

noinst_LTLIBRARIES += identity/libidentity.la

identity_libidentity_la_SOURCES = \
	identity/id_objects.c \
	identity/id_context.c \
	identity/id_screen.c

################################################################################

# Meta-driver which combines whichever software rasterizers have been
# built into a single convenience library.

noinst_LTLIBRARIES += noop/libnoop.la

noop_libnoop_la_SOURCES = \
	noop/noop_pipe.c \
	noop/noop_state.c

################################################################################

if HAVE_GALLIUM_R600

SUBDIRS += radeon

else

if HAVE_GALLIUM_RADEONSI

SUBDIRS += radeon

endif

endif

################################################################################

if HAVE_GALLIUM_FREEDRENO

SUBDIRS += freedreno

endif

################################################################################

if HAVE_GALLIUM_I915

SUBDIRS += i915

endif

################################################################################

if HAVE_GALLIUM_ILO

SUBDIRS += ilo

endif

################################################################################

if HAVE_GALLIUM_NOUVEAU

SUBDIRS += nouveau nv30 nv50 nvc0

endif

################################################################################

if HAVE_GALLIUM_SVGA

SUBDIRS += svga

endif

################################################################################

if HAVE_GALLIUM_R300

SUBDIRS += r300

endif

################################################################################

if HAVE_GALLIUM_R600

SUBDIRS += r600

endif

################################################################################

if HAVE_GALLIUM_RADEONSI

SUBDIRS += radeonsi

endif

################################################################################

if NEED_GALLIUM_SOFTPIPE_DRIVER

SUBDIRS += softpipe

endif

################################################################################

if NEED_GALLIUM_LLVMPIPE_DRIVER

SUBDIRS += llvmpipe

endif
