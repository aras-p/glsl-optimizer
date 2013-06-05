AUTOMAKE_OPTIONS = subdir-objects


SUBDIRS = . galahad identity noop trace rbug

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

SUBDIRS += nouveau

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
