SUBDIRS =

##
## Gallium auxiliary module
##

SUBDIRS += auxiliary

##
## Gallium pipe drivers and their respective winsys'
##

SUBDIRS += \
	drivers/galahad \
	drivers/identity \
	drivers/noop \
	drivers/trace \
	drivers/rbug

## freedreno/msm/kgsl
if HAVE_GALLIUM_FREEDRENO
SUBDIRS += drivers/freedreno winsys/freedreno/drm
endif

## i915g/i915
if HAVE_GALLIUM_I915
SUBDIRS += drivers/i915 winsys/i915/drm
endif

## ilo/i965
if HAVE_GALLIUM_ILO
SUBDIRS += drivers/ilo winsys/intel/drm
endif

## nouveau
if HAVE_GALLIUM_NOUVEAU
SUBDIRS += drivers/nouveau winsys/nouveau/drm
endif

## vmwgfx/svga
if HAVE_GALLIUM_SVGA
SUBDIRS += drivers/svga winsys/svga/drm
endif

## r300
if HAVE_GALLIUM_R300
SUBDIRS += drivers/r300
endif

## radeon - linked into r600 and radeonsi
if HAVE_GALLIUM_RADEON_COMMON
SUBDIRS += drivers/radeon
endif

## r600
if HAVE_GALLIUM_R600
SUBDIRS += drivers/r600
endif

## radeonsi
if HAVE_GALLIUM_RADEONSI
SUBDIRS += drivers/radeonsi
endif

## the radeon winsys - linked in by r300, r600 and radeonsi
if NEED_RADEON_DRM_WINSYS
SUBDIRS += winsys/radeon/drm
endif

## swrast/softpipe
if NEED_GALLIUM_SOFTPIPE_DRIVER
SUBDIRS += drivers/softpipe

## swrast/llvmpipe
if NEED_GALLIUM_LLVMPIPE_DRIVER
SUBDIRS += drivers/llvmpipe
endif
endif

## vc4/rpi
if HAVE_GALLIUM_VC4
SUBDIRS += drivers/vc4 winsys/vc4/drm
endif

## the sw winsys'
SUBDIRS += winsys/sw/null

if NEED_WINSYS_XLIB
SUBDIRS += winsys/sw/xlib
endif

if HAVE_DRISW
SUBDIRS += winsys/sw/dri
endif

if HAVE_DRI2
SUBDIRS += winsys/sw/kms-dri
endif

if HAVE_EGL_PLATFORM_FBDEV
SUBDIRS += winsys/sw/fbdev
endif

if HAVE_EGL_PLATFORM_WAYLAND
SUBDIRS += winsys/sw/wayland
endif

if NEED_WINSYS_WRAPPER
SUBDIRS += winsys/sw/wrapper
endif


##
## Gallium state trackers and their users (targets)
##

if NEED_GALLIUM_LOADER
SUBDIRS += targets/pipe-loader
endif

if HAVE_CLOVER
SUBDIRS += state_trackers/clover targets/opencl
endif

if HAVE_DRICOMMON
SUBDIRS += state_trackers/dri targets/dri
endif

## the egl target depends on vega
if HAVE_OPENVG
SUBDIRS += state_trackers/vega
endif

if HAVE_GALLIUM_EGL
SUBDIRS += state_trackers/egl targets/egl-static
endif

if HAVE_GALLIUM_GBM
SUBDIRS += state_trackers/gbm targets/gbm
endif

if HAVE_X11_DRIVER
SUBDIRS += state_trackers/glx/xlib targets/libgl-xlib
endif

if HAVE_ST_OMX
SUBDIRS += state_trackers/omx targets/omx
endif

if HAVE_GALLIUM_OSMESA
SUBDIRS += state_trackers/osmesa targets/osmesa
endif

if HAVE_ST_VDPAU
SUBDIRS += state_trackers/vdpau targets/vdpau
endif

if HAVE_ST_XA
SUBDIRS += state_trackers/xa targets/xa
endif

if HAVE_ST_XVMC
SUBDIRS += state_trackers/xvmc targets/xvmc
endif

##
## Gallium tests
##

if HAVE_GALLIUM_TESTS
SUBDIRS += \
	tests/trivial \
	tests/unit
endif
